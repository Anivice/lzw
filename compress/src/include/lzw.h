/* lzw.h
 *
 * Copyright 2025 Anivice Ives
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LZW_H
#define LZW_H

#include <vector>
#include <array>
#include <cstdint>
#include <ios>
#include <unordered_map>
#include "log.hpp"

template <typename T>
concept unsigned_integral = std::is_integral_v<T> && std::is_unsigned_v<T>;

template < unsigned BitSize > class bitwise_numeric_stack;

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize = BitSize,
    const unsigned RequiredByteBlocks = CurrentBitSize / 8 + (CurrentBitSize % 8 == 0 ? 0 : 1),
    const unsigned AdditionalTailingBits = CurrentBitSize % 8 >
class bitwise_numeric {
private:
    const uint64_t lm_checksum_ =
        ((static_cast<uint64_t>(CurrentBitSize) << 32) | (RequiredByteBlocks))
        ^ static_cast<uint64_t>(AdditionalTailingBits);

    struct byte {
        uint8_t num {};
        uint8_t bit {};
    };
    std::array<byte, RequiredByteBlocks> data { };

    static bool overflow(uint16_t, uint8_t);

public:
    bitwise_numeric(bitwise_numeric &&) noexcept;
    bitwise_numeric(const bitwise_numeric &);
    bitwise_numeric() = default;
	~bitwise_numeric() = default;

    // operations
    bitwise_numeric operator +(const bitwise_numeric &) const;
    bitwise_numeric operator -(const bitwise_numeric &) const;
    bitwise_numeric operator ^(const bitwise_numeric &) const;
    bitwise_numeric operator |(const bitwise_numeric &) const;
    bitwise_numeric operator &(const bitwise_numeric &) const;
	template < unsigned_integral Numeric > bitwise_numeric operator <<(Numeric) const;
    template < unsigned_integral Numeric > bitwise_numeric operator >>(Numeric) const;
    bitwise_numeric & operator ++();
    bitwise_numeric & operator --();
    bitwise_numeric & operator =(bitwise_numeric &&) noexcept;
    bitwise_numeric & operator =(const bitwise_numeric&);

    // utilities
    [[nodiscard]] bool operator ==(const bitwise_numeric&) const;
    [[nodiscard]] bool operator !=(const bitwise_numeric&) const;
    [[nodiscard]] bool operator <(const bitwise_numeric&) const;
    [[nodiscard]] bool operator >(const bitwise_numeric&) const;

    [[nodiscard]] bool operator >=(const bitwise_numeric& other) const
	{
		return operator>(other) || operator==(other);
	}

    [[nodiscard]] bool operator <=(const bitwise_numeric& other) const
    {
        return operator<(other) || operator==(other);
    }

    
    [[nodiscard]] const decltype(data) & dump() const
    {
        return this->data;
    }

    [[nodiscard]] std::vector < uint8_t > dump_to_vector() const
    {
        std::vector < uint8_t > ret;
		ret.resize(RequiredByteBlocks);
		for (unsigned i = 0; i < RequiredByteBlocks; i++)
		{
            ret.push_back(data[i].num);
		}

        return ret;
    }

	template < typename NumericType >
    requires std::is_integral_v < NumericType >
    [[nodiscard]] NumericType export_numeric() const
    {
        NumericType ret { };
		for (unsigned i = 0; i < std::min(static_cast<uint64_t>(RequiredByteBlocks), sizeof(NumericType)); i++)
		{
			((uint8_t*)(&ret))[i] = data[i].num;
		}
		return ret;
    }

    template < unsigned_integral Numeric >
    [[nodiscard]] static bitwise_numeric make_bitwise_numeric(Numeric);

	// loosely structured
    template < typename Numeric >
    requires std::is_integral_v < Numeric >
    [[nodiscard]] static bitwise_numeric make_bitwise_numeric_loosely(Numeric numeric)
    {
		return make_bitwise_numeric(static_cast<std::make_unsigned_t<Numeric>>(numeric));
    }

    [[nodiscard]] bitwise_numeric max_num() const;

    friend class bitwise_numeric_stack<BitSize>;
};

enum endian_t { LITTLE_ENDIAN, BIG_ENDIAN };

template < unsigned BitSize >
class bitwise_numeric_stack {
private:
    std::vector< bitwise_numeric < BitSize > > stack_frame_;
    const unsigned current_bit_size = BitSize;
    const unsigned required_byte_blocks = BitSize / 8 + (BitSize % 8 == 0 ? 0 : 1);
    const unsigned additional_tailing_bits = BitSize % 8;

public:
    void push(const bitwise_numeric<BitSize> & element) {
        stack_frame_.emplace_back(element);
    }

    void pop() {
        stack_frame_.pop_back();
    }

    [[nodiscard]] uint64_t size() const {
        return stack_frame_.size();
    }

    [[nodiscard]] const bitwise_numeric<BitSize> & top() {
        return stack_frame_.back();
	}

    [[nodiscard]] bitwise_numeric<BitSize>& at(const uint64_t index) {
        return stack_frame_[index];
    }

	[[nodiscard]] bitwise_numeric<BitSize>& operator [](const uint64_t index) {
		return stack_frame_[index];
	}

    template < typename Numeric >
    void emplace(const Numeric number)
    {
        using unsigned_type = std::make_unsigned_t<Numeric>;
        push(bitwise_numeric<BitSize>::make_bitwise_numeric(static_cast<unsigned_type>(number)));
    }

    [[nodiscard]] std::vector<uint8_t> dump() const;
    void import(const std::vector<uint8_t> &, uint64_t);
    void lazy_import(const std::vector<uint8_t>&);
    uint64_t hash() const;
};

template <typename Type>
constexpr Type two_power(const Type n)
{
    static_assert(std::is_integral_v<Type>, "Type must be an integral type");
    return static_cast<Type>(0x01) << n;
}

template < 
    unsigned LzwCompressionBitSize,
    unsigned DictionarySize = two_power(LzwCompressionBitSize) >
requires (LzwCompressionBitSize > 8)
class lzw
{
	std::vector < uint8_t > & input_stream_;
    std::vector < uint8_t > & output_stream_;
	std::unordered_map < std::string, bitwise_numeric < LzwCompressionBitSize > > dictionary_;
    bool discarding_this_instance = false;

public:
    explicit lzw(
        std::vector < uint8_t >& input_stream,
        std::vector < uint8_t >& output_stream);

	// forbid any copy/move constructor or assignment
	lzw(const lzw&) = delete;
	lzw(lzw&&) = delete;
	lzw& operator =(const lzw&) = delete;
	lzw& operator =(lzw&&) = delete;

	// destructor
	~lzw() = default;

    // basic operations
	void compress();
	void decompress();
};

#endif //LZW_H

// inline definition for lzw utilities
#include "lzw.inl"
