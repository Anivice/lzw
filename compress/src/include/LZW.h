/* LZW.h
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

template <typename T>
concept UnsignedIntegral = std::is_integral_v<T> && std::is_unsigned_v<T>;

template < unsigned BIT_SIZE > class bitwise_numeric_stack;

template <
    const unsigned BIT_SIZE,
    const unsigned current_bit_size = BIT_SIZE,
    const unsigned required_byte_blocks = current_bit_size / 8 + (current_bit_size % 8 == 0 ? 0 : 1),
    const unsigned additional_tailing_bits = current_bit_size % 8 >
class bitwise_numeric {
private:
    const uint64_t lm_checksum =
        ((static_cast<uint64_t>(current_bit_size) << 32) | (required_byte_blocks))
        ^ static_cast<uint64_t>(additional_tailing_bits);

    struct byte {
        uint8_t num {};
        uint8_t bit {};
    };
    std::array<byte, required_byte_blocks> data { };

    static bool overflow(uint16_t, uint8_t);

public:
    bitwise_numeric(bitwise_numeric &&) noexcept;
    bitwise_numeric(const bitwise_numeric &);
    bitwise_numeric() = default;
    bitwise_numeric operator +(bitwise_numeric &);
    bitwise_numeric operator -(bitwise_numeric &);
    bool operator ==(bitwise_numeric &);
    bool operator !=(bitwise_numeric &);
    bitwise_numeric operator ^(bitwise_numeric &);
    bitwise_numeric operator |(bitwise_numeric &);
    bitwise_numeric operator &(bitwise_numeric &);
    bitwise_numeric & operator =(bitwise_numeric &&) noexcept;
    bitwise_numeric & operator =(const bitwise_numeric&);
    [[nodiscard]] const decltype(data) & dump() const
    {
        return this->data;
    }

    [[nodiscard]] uint64_t export_numeric() const
    {
		uint64_t ret = 0;
		for (unsigned i = 0; i < required_byte_blocks; i++)
		{
			ret |= (static_cast<uint64_t>(data[i].num) << (i * 8));
		}
		return ret;
    }

    template < UnsignedIntegral Numeric >
    static bitwise_numeric make_bitwise_numeric(Numeric);

    friend class bitwise_numeric_stack<BIT_SIZE>;
};

template < unsigned BIT_SIZE >
class bitwise_numeric_stack {
private:
    std::vector< bitwise_numeric < BIT_SIZE > > string;
    const unsigned current_bit_size = BIT_SIZE;
    const unsigned required_byte_blocks = BIT_SIZE / 8 + (BIT_SIZE % 8 == 0 ? 0 : 1);
    const unsigned additional_tailing_bits = BIT_SIZE % 8;

public:
    void push(const bitwise_numeric<BIT_SIZE> & element) {
        string.emplace_back(element);
    }

    void pop() {
        string.pop_back();
    }

    [[nodiscard]] uint64_t size() const {
        return string.size();
    }

    [[nodiscard]] const bitwise_numeric<BIT_SIZE> & top() {
        return string.back();
	}

    [[nodiscard]] bitwise_numeric<BIT_SIZE>& at(const uint64_t index) {
        return string[index];
    }

	[[nodiscard]] bitwise_numeric<BIT_SIZE>& operator [](const uint64_t index) {
		return string[index];
	}

    template < typename Numeric >
    void emplace(const Numeric number)
    {
        using unsigned_type = std::make_unsigned_t<Numeric>;
        push(bitwise_numeric<BIT_SIZE>::make_bitwise_numeric(static_cast<unsigned_type>(number)));
    }

    [[nodiscard]] std::vector<uint8_t> dump() const;
    void import(const std::vector<uint8_t> &, uint64_t);
};

class LZW
{
};

#endif //LZW_H

#include "LZW.inl"
