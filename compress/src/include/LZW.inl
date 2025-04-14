#ifndef LZW_INL
#define LZW_INL

#include <iostream>
#include <ostream>
#include <algorithm>
#include <cstring>

#include "lzw.h"
#include "log.hpp"

#define COMPUTE_8_BIT_COMPLIMENT(bit) ((uint8_t)(0xFF >> (8 - (bit))))

template < unsigned BitSize, unsigned RequiredBytes = BitSize / 8 + (BitSize % 8 == 0 ? 0 : 1) >
[[nodiscard]]
std::array < uint8_t, RequiredBytes >
bitcopy(
    const uint64_t,
    const uint8_t,
    const uint8_t,
    const std::vector < uint8_t >&);

template < unsigned BitSize >
[[nodiscard]]
std::vector < uint8_t >
bitcopy(
    const uint64_t bit_start,
    const uint8_t bit_copy,
    const std::vector < uint8_t > & data)
{
	std::vector < uint8_t > ret{};
	const uint64_t byte_starting = bit_start / 8;
	const uint8_t bit_starting = bit_start % 8;
    const auto array_from_bitcopy = bitcopy<BitSize>(byte_starting, bit_starting, bit_copy, data);
	ret.resize(bit_copy / 8 + (bit_copy % 8 == 0 ? 0 : 1));
	std::memcpy(ret.data(), array_from_bitcopy.data(), ret.size() * sizeof(uint8_t));
    return ret;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bool
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
overflow(
    const uint16_t data,
    const uint8_t compute_bit_sz)
{
    // first, compute compliment
    const uint16_t compliment = COMPUTE_8_BIT_COMPLIMENT(compute_bit_sz);
    // second, see if data overflow
    return ((data & (~compliment)) > 0) || (data > (1 << BitSize) - 1);
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
bitwise_numeric(bitwise_numeric &&self) noexcept
{
    if (lm_checksum_ != self.lm_checksum_) {
        return;
    }

    this->data = self.data;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
bitwise_numeric(const bitwise_numeric &self)
{
    if (lm_checksum_ != self.lm_checksum_) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    memcpy(this->data.data(), self.data.data(), this->data.size() * sizeof(byte));
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator+(const bitwise_numeric & other) const
{
    if (lm_checksum_ != other.lm_checksum_) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    bitwise_numeric <BitSize> ret;
    ret.data = this->data;

    int carry = 0;
    for (unsigned i = 0; i < ret.data.size(); i++)
    {
        uint16_t byte_result =
            static_cast<uint16_t>(other.data[i].num) +
            static_cast<uint16_t>(ret.data[i].num) + carry;
        carry = overflow(byte_result, ret.data[i].bit) ? 1 : 0;
        ret.data[i].num = byte_result & 0xFF;
    }

    if (carry) {
        throw std::overflow_error("Bitwise overflow");
    }

    return ret;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator-(const bitwise_numeric & other) const
{
    if (lm_checksum_ != other.lm_checksum_) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    bitwise_numeric <BitSize> ret;
    ret.data = this->data;

    int carry = 0;
    for (unsigned i = 0; i < ret.data.size(); i++)
    {
        uint16_t byte_result =
            static_cast<uint16_t>(ret.data[i].num) -
            static_cast<uint16_t>(other.data[i].num) - carry;
        carry = overflow(byte_result, ret.data[i].bit) ? 1 : 0;
        ret.data[i].num = byte_result & 0xFF;
    }

    if (carry) {
        throw std::overflow_error("Bitwise overflow");
    }

    return ret;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bool bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator==(const bitwise_numeric & other) const
{
    if (lm_checksum_ != other.lm_checksum_) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }


	for (unsigned i = 0; i < RequiredByteBlocks; i++)
	{
		if ((this->data[i].num & COMPUTE_8_BIT_COMPLIMENT(this->data[i].bit)) != 
            (other.data[i].num & COMPUTE_8_BIT_COMPLIMENT(other.data[i].bit))) 
        {
			return false;
		}
	}

    return true;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bool
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator!=(const bitwise_numeric & other) const
{
    return !this->operator==(other);
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator^(const bitwise_numeric & other) const
{
    if (lm_checksum_ != other.lm_checksum_) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    bitwise_numeric <BitSize> ret;
    ret.data = this->data;

    for (unsigned i = 0; i < this->data.size(); i++)
    {
        const uint16_t complement = COMPUTE_8_BIT_COMPLIMENT(ret.data[i].bit);
        ret.data[i].num ^= (other.data[i].num & complement);
        ret.data[i].num &= complement;
    }

    return ret;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator&(const bitwise_numeric & other) const
{
    if (lm_checksum_ != other.lm_checksum_) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    bitwise_numeric <BitSize> ret;
    ret.data = this->data;

    for (unsigned i = 0; i < this->data.size(); i++)
    {
        const uint16_t complement = COMPUTE_8_BIT_COMPLIMENT(ret.data[i].bit);
        ret.data[i].num &= (other.data[i].num & complement);
    }

    return ret;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits> &
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator=(bitwise_numeric && other) noexcept
{
    if (lm_checksum_ != other.lm_checksum_) {
        return *this;
    }

    this->data = other.data;
    return *this;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>&
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator =(const bitwise_numeric& other)
{
    if (lm_checksum_ != other.lm_checksum_) {
        return *this;
    }

    this->data = other.data;
    return *this;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bool
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator <(const bitwise_numeric& other) const
{
	if (lm_checksum_ != other.lm_checksum_) {
		throw std::runtime_error("Operation between two bitwise-unequal numbers");
	}

    if (this->operator==(other))
    {
        return false;
    }

	for (int64_t i = RequiredByteBlocks - 1; i >= 0; --i)
	{
		if ((other.data[i].num & COMPUTE_8_BIT_COMPLIMENT(other.data[i].bit)) >
            (this->data[i].num & COMPUTE_8_BIT_COMPLIMENT(this->data[i].bit)))
        {
            return false;
		}
	}

	return true;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bool
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator >(const bitwise_numeric& other) const
{
    if (lm_checksum_ != other.lm_checksum_) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    if (this->operator==(other))
    {
        return false;
    }

    for (int64_t i = RequiredByteBlocks - 1; i >= 0; --i)
    {
        const auto other_8_bit = other.data[i].num & COMPUTE_8_BIT_COMPLIMENT(other.data[i].bit);
		const auto my_8_bit = this->data[i].num & COMPUTE_8_BIT_COMPLIMENT(this->data[i].bit);
        if (my_8_bit > other_8_bit) {
            return true;
        } else if (my_8_bit < other_8_bit) {
            return false;
        } else {
	        continue;
        }
    }

    return false;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator|(const bitwise_numeric & other) const
{
    if (lm_checksum_ != other.lm_checksum_) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    bitwise_numeric <BitSize> ret;
    ret.data = this->data;

    for (unsigned i = 0; i < this->data.size(); i++)
    {
        const uint16_t complement = COMPUTE_8_BIT_COMPLIMENT(ret.data[i].bit);
        ret.data[i].num |= (other.data[i].num & complement);
        ret.data[i].num &= complement;
    }

    return ret;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
template < unsigned_integral Numeric >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator <<(const Numeric shifting_bits) const
{
    bitwise_numeric reference;
	reference.data = this->data;
	auto shift_left_1_bit = [&]()
	{
		bool last_carry = false;
        bool least_significant_byte = true;
		for (unsigned i = 0; i < RequiredByteBlocks; i++)
		{
            const bool carry = (reference.data[i].num >> (reference.data[i].bit - 1)) != 0;
            reference.data[i].num <<= 1; // shift
            reference.data[i].num &= COMPUTE_8_BIT_COMPLIMENT(reference.data[i].bit); // clear unwanted bits

            // carry over
            if (!least_significant_byte)
            {
	            // MSB carry over
                reference.data[i].num |= last_carry ? 0x01 : 0x00;
            } else {
                least_significant_byte = false;
            }

            // update carry flag
            last_carry = carry;
		}
	};

	for (Numeric i = 0; i < shifting_bits; ++i)
	{
        shift_left_1_bit();
	}

    return reference;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
template < unsigned_integral Numeric >
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>
bitwise_numeric<BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits>::
operator >>(const Numeric shifting_bits) const
{
    bitwise_numeric reference;
    reference.data = this->data;
    auto shift_right_1_bit = [&]()
    {
    	bool last_carry = false;
    	bool most_significant_byte = true;
    	for (int64_t i = RequiredByteBlocks - 1; i >= 0; --i)
    	{
    		const bool carry = reference.data[i].num & 0x01;
    		reference.data[i].num >>= 1; // shift

            // carry over
    		if (!most_significant_byte)
    		{
    			// LSB carry over
    			reference.data[i].num |= last_carry << (reference.data[i].bit - 1); // shift to MSB then add in
    		} else {
                most_significant_byte = false;
    		}

            // update carry flag
            last_carry = carry;
    	}
    };

    for (Numeric i = 0; i < shifting_bits; ++i)
    {
        shift_right_1_bit();
    }

    return reference;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric < BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits > & 
bitwise_numeric < BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits >::
operator ++()
{
    *this = this->operator+(make_bitwise_numeric_loosely(1));
    return *this;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
bitwise_numeric < BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits >&
bitwise_numeric < BitSize, CurrentBitSize, RequiredByteBlocks, AdditionalTailingBits >::
operator --()
{
    *this = this->operator-(make_bitwise_numeric_loosely(1));
    return *this;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >
template < unsigned_integral Numeric >

bitwise_numeric < 
    BitSize,
    CurrentBitSize,
    RequiredByteBlocks,
    AdditionalTailingBits >

bitwise_numeric < 
    BitSize,
    CurrentBitSize,
    RequiredByteBlocks,
    AdditionalTailingBits >::
make_bitwise_numeric(const Numeric number)
{
    const unsigned tailing_compliment = COMPUTE_8_BIT_COMPLIMENT(AdditionalTailingBits);
    if (RequiredByteBlocks < 1) {
        throw std::runtime_error("Required byte blocks must be greater than 0");
    }
    bitwise_numeric < BitSize > ret;

    // init
    for (unsigned i = 0; i < RequiredByteBlocks; i++)
    {
        ret.data[i].bit = 8;
        ret.data[i].num = 0;
    }

    // copy over
    for (unsigned i = 0; i < std::min(sizeof(Numeric), (uint64_t)RequiredByteBlocks); i++) {
        ret.data[i].num = ((uint8_t*)&number)[i];
        ret.data[i].bit = 8;
    }

    // tailing
    if (AdditionalTailingBits != 0) {
        ret.data[RequiredByteBlocks - 1].bit = AdditionalTailingBits;
        ret.data[RequiredByteBlocks - 1].num &= tailing_compliment;
    }
    return ret;
}

template <
    const unsigned BitSize,
    const unsigned CurrentBitSize,
    const unsigned RequiredByteBlocks,
    const unsigned AdditionalTailingBits >

bitwise_numeric <
    BitSize,
    CurrentBitSize,
    RequiredByteBlocks,
    AdditionalTailingBits >

bitwise_numeric <
    BitSize,
    CurrentBitSize,
    RequiredByteBlocks,
    AdditionalTailingBits >::max_num() const
{
    bitwise_numeric ret = make_bitwise_numeric_loosely(1);

    for (unsigned i = 0; i < BitSize; i++)
    {
        ret = ret << 1u;
        auto bit1 = make_bitwise_numeric_loosely(1);
		ret = ret | bit1;
    }

    return ret;
}

template<unsigned BitSize>
std::vector<uint8_t> bitwise_numeric_stack<BitSize>::dump() const
{
    struct conjunct_result
	{
    	uint8_t conjuncted;
        uint8_t conj_effective_bits;
    	uint8_t left_over;
		uint8_t left_over_effective_bits;
    };

	auto conjunct_two_uint8_t = [&](
        const uint8_t previous,
        const uint8_t previous_effective_bits,
        const uint8_t current,
        const uint8_t current_effective_bits)
	->conjunct_result
	{
        // compute tail size
		const uint8_t tail_size = std::min(
			static_cast<uint8_t>(8 - previous_effective_bits), current_effective_bits);
        // this is the bits in current that is going to be added to the previous, forming a bitwise conjunction
        const uint8_t tailing_current = current
			& COMPUTE_8_BIT_COMPLIMENT(tail_size)
			& COMPUTE_8_BIT_COMPLIMENT(current_effective_bits);
		// now, we add the tailing current to the previous, forming the conjunction we wanted
		const uint8_t conjuncted = static_cast<uint8_t>((tailing_current << previous_effective_bits) | previous);
        // then, check for effective bits in conjuncted
        const uint8_t conj_effective_bits = std::min(
            static_cast<uint8_t>(previous_effective_bits + current_effective_bits), 
            static_cast<uint8_t>(8));
        // then, compute left_over byte
		// first, remove current's tailing bits 
		uint8_t left_over = current
			& COMPUTE_8_BIT_COMPLIMENT(current_effective_bits)
			& (~COMPUTE_8_BIT_COMPLIMENT(tail_size));
		// second, shift left_over to the right, removing the tail
		left_over >>= tail_size;
		// third, compute left_over effective bits
		const uint8_t left_over_effective_bits = current_effective_bits - tail_size;

		return conjunct_result
			{   .conjuncted = conjuncted,
				.conj_effective_bits = conj_effective_bits,
				.left_over = left_over,
				.left_over_effective_bits = left_over_effective_bits
			};
	};

    std::vector<uint8_t> ret{};
	conjunct_result result_holder {};
    bool processed_for_result_holder = false;
    for (const auto & numeric : stack_frame_)
    {
        // result is empty?
        if (!processed_for_result_holder)
        {
            // push the first numeric element
            std::vector<uint8_t> first_element;
			for (const auto& element : numeric.dump())
			{
				first_element.push_back(element.num);
			}

            result_holder.conj_effective_bits = numeric.dump().back().bit;
			result_holder.conjuncted = numeric.dump().back().num;

            ret.insert(ret.end(), first_element.begin(), first_element.end());
            ret.pop_back(); // remove last item since it's recorded in result_holder
            processed_for_result_holder = true;
        }
        else
        {
            // iterate through all bytes
            for (const auto& element : numeric.dump())
            {
                // stitch the bytes
                result_holder = conjunct_two_uint8_t(
                    result_holder.conjuncted,
                    result_holder.conj_effective_bits,
                    element.num,
                    element.bit);

                // update info
                if (result_holder.conj_effective_bits == 8)
                {
					ret.push_back(result_holder.conjuncted);
					result_holder.conj_effective_bits = result_holder.left_over_effective_bits;
					result_holder.conjuncted = result_holder.left_over;
                    result_holder.left_over = 0;
                    result_holder.left_over_effective_bits = 0;
                }
            }
        }
    }

    // if there are something left in the result_holder, add it to the result
    if (result_holder.conj_effective_bits != 0)
    {
        ret.push_back(result_holder.conjuncted & COMPUTE_8_BIT_COMPLIMENT(result_holder.conj_effective_bits));
    }

    return ret;
}


template < unsigned BitSize, unsigned RequiredBytes >
[[nodiscard]]
std::array < uint8_t, RequiredBytes > bitcopy(
    const uint64_t byte_starting, 
    const uint8_t bit_starting,
    const uint8_t bit_size,
    const std::vector < uint8_t > & data)
{
	if (bit_starting > 8)
	{
		throw std::runtime_error("Bit starting must be less than 8");
	}

    const uint8_t first_copy_bits = std::min(static_cast<uint8_t>(8 - bit_starting), bit_size);
    const uint8_t continuous_bytes = (bit_size - first_copy_bits) / 8;
	const uint8_t last_copy_bits = (bit_size - first_copy_bits) % 8;
    std::vector < uint8_t > complete_bytes {};
    uint8_t fist_byte = 0, last_byte = 0;

    // copy fist byte
    fist_byte = data[byte_starting];
	fist_byte &= ~COMPUTE_8_BIT_COMPLIMENT(bit_starting);
	fist_byte >>= bit_starting;
    //unpacked[0] = fist_byte;

	// copy continuous bytes
    complete_bytes.reserve(continuous_bytes);
	for (uint8_t i = 0; i < continuous_bytes; i++)
	{
		complete_bytes.push_back(data[byte_starting + i + 1]);
	}

	// copy last byte
	if (last_copy_bits != 0)
	{
		last_byte = data[byte_starting + continuous_bytes + 1];
		last_byte &= COMPUTE_8_BIT_COMPLIMENT(last_copy_bits);
		//unpacked[unpacked.size() - 1] = last_byte;
	}

	// return the result
	std::vector < uint8_t > result{};

	// we need to stitch the unpacked bytes, logic is exactly like dump()
    struct conjunct_result
    {
        uint8_t conjuncted;
        uint8_t conj_effective_bits;
        uint8_t left_over;
        uint8_t left_over_effective_bits;
    };

    auto conjunct_two_uint8_t = [&](
        const uint8_t previous,
        const uint8_t previous_effective_bits,
        const uint8_t current,
        const uint8_t current_effective_bits)
        ->conjunct_result
        {
            // compute tail size
            const uint8_t tail_size = std::min(
                static_cast<uint8_t>(8 - previous_effective_bits), current_effective_bits);
            // this is the bits in current that is going to be added to the previous, forming a bitwise conjunction
            const uint8_t tailing_current = current
                & COMPUTE_8_BIT_COMPLIMENT(tail_size)
                & COMPUTE_8_BIT_COMPLIMENT(current_effective_bits);
            // now, we add the tailing current to the previous, forming the conjunction we wanted
            const uint8_t conjuncted = static_cast<uint8_t>((tailing_current << previous_effective_bits) | previous);
            // then, check for effective bits in conjuncted
            const uint8_t conj_effective_bits = std::min(
                static_cast<uint8_t>(previous_effective_bits + current_effective_bits),
                static_cast<uint8_t>(8));
            // then, compute left_over byte
            // first, remove current's tailing bits 
            uint8_t left_over = current
                & COMPUTE_8_BIT_COMPLIMENT(current_effective_bits)
                & (~COMPUTE_8_BIT_COMPLIMENT(tail_size));
            // second, shift left_over to the right, removing the tail
            left_over >>= tail_size;
            // third, compute left_over effective bits
            const uint8_t left_over_effective_bits = current_effective_bits - tail_size;

            return conjunct_result
            { .conjuncted = conjuncted,
                .conj_effective_bits = conj_effective_bits,
                .left_over = left_over,
                .left_over_effective_bits = left_over_effective_bits
            };
        };

    conjunct_result result_holder{};
    result_holder.conjuncted = fist_byte;
	result_holder.conj_effective_bits = first_copy_bits;
    auto iterate = [&](const uint8_t numeric, const uint8_t effective_bits)->void
		{
            // stitch the bytes
            result_holder = conjunct_two_uint8_t(
                result_holder.conjuncted,
                result_holder.conj_effective_bits,
                numeric,
                effective_bits);

            // update info
            if (result_holder.conj_effective_bits == 8)
            {
                result.push_back(result_holder.conjuncted);
                result_holder.conj_effective_bits = result_holder.left_over_effective_bits;
                result_holder.conjuncted = result_holder.left_over;
                result_holder.left_over = 0;
                result_holder.left_over_effective_bits = 0;
            }
		};

    // piece complete bytes
    for (const auto& numeric : complete_bytes /* unpacked only contains bits of complete bytes */)
    {
        iterate(numeric, 8);
    }

    // piece last tailing byte (if there is one)
    if (last_copy_bits != 0)
    {
        iterate(last_byte, last_copy_bits);
    }

    // if there are something left in the result_holder, add it to the result
    if (result_holder.conj_effective_bits != 0)
    {
        result.push_back(result_holder.conjuncted & COMPUTE_8_BIT_COMPLIMENT(result_holder.conj_effective_bits));
    }

    // verify
    std::array<uint8_t, BitSize / 8 + (BitSize % 8 == 0 ? 0 : 1)> ret;
	if (result.size() != ret.size())
	{
		throw std::runtime_error("Error when copying bits, possibly a bug");
	}

    // copy result
	std::memcpy(ret.data(), result.data(), ret.size() * sizeof(uint8_t));

    return ret;
}

template<unsigned BitSize>
void bitwise_numeric_stack<BitSize>::import(const std::vector<uint8_t>& data, uint64_t expected_len)
{
    // clear stack
    stack_frame_.clear();

    uint64_t byte_offset = 0;
    uint8_t bit_offset = 0;

	auto add_offset = [&](const uint8_t off)->void {
		// add offset
		bit_offset += off;
		while (bit_offset >= 8)
		{
			byte_offset++;
			bit_offset -= 8;
		}
	};

    auto import_numeric = [&](const std::array<uint8_t, BitSize / 8 + (BitSize % 8 == 0 ? 0 : 1)> & src)
        {
			// create a new numeric
			bitwise_numeric<BitSize> numeric;
			// copy over the data
			for (uint8_t i = 0; i < numeric.dump().size(); i++)
			{
				numeric.data[i].num = src[i];
				numeric.data[i].bit = 8;
			}
            numeric.data.back().bit = additional_tailing_bits;

			// push to stack
			stack_frame_.push_back(numeric);
        };

    // import bits by bits
    for (uint64_t i = 0; i < expected_len; i++)
    {
        // copy bits over
        auto result = 
            bitcopy<BitSize>(
                byte_offset, 
                bit_offset, 
                static_cast<uint8_t>(BitSize), 
                data);
        // increase offset
        add_offset(BitSize);

        // import result
        import_numeric(result);
		//debug::log(result, "\n");
    }
}

template<unsigned BitSize>
void bitwise_numeric_stack<BitSize>::lazy_import(const std::vector<uint8_t> & data)
{
    // packed data to original length
    const auto import_length = data.size() * 8 /* bits */ / BitSize;
    import(data, import_length);
}

template <unsigned BitSize>
uint64_t bitwise_numeric_stack<BitSize>::hash() const
{
    uint64_t crc64_value{};
    uint64_t table[256]{};

    auto init_crc64 = [&]()
        {
            crc64_value = 0xFFFFFFFFFFFFFFFF;
            for (uint64_t i = 0; i < 256; ++i) {
                uint64_t crc = i;
                for (uint64_t j = 8; j--; ) {
                    if (crc & 1)
                        crc = (crc >> 1) ^ 0xC96C5795D7870F42;  // Standard CRC-64 polynomial
                    else
                        crc >>= 1;
                }
                table[i] = crc;
            }
        };

    auto reverse_bytes = [](uint64_t x)->uint64_t
        {
            x = ((x & 0x00000000FFFFFFFFULL) << 32) | ((x & 0xFFFFFFFF00000000ULL) >> 32);
            x = ((x & 0x0000FFFF0000FFFFULL) << 16) | ((x & 0xFFFF0000FFFF0000ULL) >> 16);
            x = ((x & 0x00FF00FF00FF00FFULL) << 8) | ((x & 0xFF00FF00FF00FF00ULL) >> 8);
            return x;
        };

    auto update = [&](const uint8_t* data, const size_t length)
        {
            for (size_t i = 0; i < length; ++i) {
                crc64_value = table[(crc64_value ^ data[i]) & 0xFF] ^ (crc64_value >> 8);
            }
        };

    auto get_checksum = [&](const endian_t endian)->uint64_t
        {
            // add the final complement that ECMA-182 requires
            return (endian == BIG_ENDIAN
                ? reverse_bytes(crc64_value ^ 0xFFFFFFFFFFFFFFFFULL)
                : (crc64_value ^ 0xFFFFFFFFFFFFFFFFULL));
        };

    init_crc64();
    const auto dumped = dump();
	update(dumped.data(), dumped.size());
	return get_checksum(BIG_ENDIAN);
}

template < unsigned LzwCompressionBitSize, unsigned DictionarySize >
    requires (LzwCompressionBitSize > 8)
lzw<LzwCompressionBitSize, DictionarySize>::lzw(
    std::vector < uint8_t >& input_stream,
    std::vector < uint8_t >& output_stream)
: input_stream_(input_stream),
  output_stream_(output_stream)
{
    // Initialize the dictionary
    for (int i = 0; i < 256; ++i) {
        dictionary_[std::string(1, static_cast<char>(i))] = 
            bitwise_numeric<LzwCompressionBitSize>::make_bitwise_numeric_loosely(i);
    }
}

template < unsigned LzwCompressionBitSize, unsigned DictionarySize >
    requires (LzwCompressionBitSize > 8)
void lzw<LzwCompressionBitSize, DictionarySize>::compress()
{
    if (discarding_this_instance)
    {
		throw std::invalid_argument("This instance has been discarded");
    }

    if (input_stream_.empty())
    {
	    return;
    }

    std::string current_string {};
    bitwise_numeric_stack < LzwCompressionBitSize > result_stack;
    bitwise_numeric<LzwCompressionBitSize> next_code = 
        bitwise_numeric < LzwCompressionBitSize>::make_bitwise_numeric_loosely(256);

    // Compression process
    std::ranges::reverse(input_stream_);
    while (!input_stream_.empty()) 
    {
        // Get input symbol while there are input symbols left
        const auto c = static_cast<char>(input_stream_.back());
        input_stream_.pop_back();
        const auto combined_string = current_string + c;
        if (dictionary_.contains(combined_string)) // combined_string is in the table
        {
            // update current string
            current_string = combined_string;
        }
        else
        {
			// Output the code for current string
			result_stack.push(dictionary_.at(current_string));
			// Add combined_string to the dictionary
			if (next_code < next_code.make_bitwise_numeric(DictionarySize)) {
				dictionary_.emplace(combined_string, next_code);
                ++next_code;
			}

			// Update current string to the new character
			current_string = std::string(1, c);
        }
    }

    // Output the last code if any
    if (!current_string.empty()) {
        result_stack.push(dictionary_[current_string]);
    }

	// Write the compressed data to the output stream
    for (const auto dumped_data = result_stack.dump();
        const auto& byte : dumped_data) 
    {
		output_stream_.push_back(byte);
	}

    // discarding this instance
    discarding_this_instance = true;
}

template < unsigned LzwCompressionBitSize, unsigned DictionarySize >
    requires (LzwCompressionBitSize > 8)
void lzw<LzwCompressionBitSize, DictionarySize>::decompress()
{
    if (discarding_this_instance)
    {
        throw std::invalid_argument("This instance has been discarded");
    }

    if (input_stream_.empty())
    {
        return;
    }

	auto dictionary_at = [&](const bitwise_numeric<LzwCompressionBitSize>& index)
	{
		for (const auto& [key, value] : dictionary_)
		{
			if (index == value)
			{
                return key;
			}
		}

		throw std::out_of_range("Code not found in dictionary");
	};

    std::vector<uint8_t> source_dump;
    std::string current_string{};
    bitwise_numeric_stack < LzwCompressionBitSize > source_stack;
    bitwise_numeric<LzwCompressionBitSize> next_code =
        bitwise_numeric < LzwCompressionBitSize>::make_bitwise_numeric_loosely(255);

    // dump source
	std::ranges::reverse(input_stream_);
    while (!input_stream_.empty())
    {
        const auto code = static_cast<uint8_t>(input_stream_.back());
        input_stream_.pop_back();
		source_dump.emplace_back(code);
    }

	// import source to stack
	source_stack.lazy_import(source_dump);

    // The first code is popped out and assigned to current_string
    current_string = static_cast<char>(source_stack[0].export_numeric<uint8_t>());
	output_stream_.push_back(dictionary_.at(current_string).export_numeric<uint8_t>());

    for (uint64_t i = 1; i < source_stack.size(); i++)
    {
        // Get input while there are codes are left to be received
        const auto code = source_stack[i];
        std::string entry;
        // Check if the Decoder has codes to decode
        if (code > next_code)
        {
            // entry = STRING + STRING[0]
            entry = current_string + current_string[0];
        }
        else
        {
            // entry = table[ord(arr[j])]
			entry = dictionary_at(code);
        }

        // append the new string
        output_stream_.insert(output_stream_.end(), entry.begin(), entry.end());

        // Check if the Table is full
		if (dictionary_.size() < DictionarySize)
		{
            ++next_code;
			dictionary_.emplace(current_string + entry[0], next_code);
		}

		// Update current string
		current_string = entry;
    }

    discarding_this_instance = true;
}

#endif // LZW_INL
