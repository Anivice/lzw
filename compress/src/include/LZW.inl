#ifndef LZW_INL
#define LZW_INL

#include <iostream>
#include <ostream>

#include "LZW.h"

#define COMPUTE_8BIT_COMPLIMENT(bit) ((uint8_t)(0xFF >> (8 - (bit))))

template <
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits >
bool
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
overflow(
    const uint16_t data_,
    const uint8_t COMPUTE_BIT_SZ)
{
    // first, compute compliment
    const uint16_t compliment = COMPUTE_8BIT_COMPLIMENT(COMPUTE_BIT_SZ);
    // second, see if data overflow
    return ((data_ & (~compliment)) > 0) || (data_ > (1 << BIT_SIZE) - 1);
}

template <
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits >
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
bitwise_numeric(bitwise_numeric &&self) noexcept
{
    if (lm_checksum != self.lm_checksum) {
        return;
    }

    this->data = self.data;
}

template < 
    const unsigned BIT_SIZE,
	const unsigned current_bit_size,
	const unsigned required_byte_blocks,
	const unsigned additional_tailing_bits >
bitwise_numeric < BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
bitwise_numeric(const bitwise_numeric &self)
{
    if (lm_checksum != self.lm_checksum) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    memcpy(this->data.data(), self.data.data(), this->data.size() * sizeof(byte));
}

template <
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits >
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
operator+(bitwise_numeric & other)
{
    if (lm_checksum != other.lm_checksum) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    bitwise_numeric <BIT_SIZE> ret;
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
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits >
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>
bitwise_numeric<BIT_SIZE,current_bit_size, required_byte_blocks, additional_tailing_bits>::
operator-(bitwise_numeric & other)
{
    if (lm_checksum != other.lm_checksum) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    bitwise_numeric <BIT_SIZE> ret;
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
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits >
bool bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
operator==(bitwise_numeric & other)
{
    if (lm_checksum != other.lm_checksum) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    // Simplified comparison using memcmp
    if (std::memcmp(data.data(), other.data.data(), data.size()) != 0) {
        return false;
    }

    return true;
}

template <
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits >
bool
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
operator!=(bitwise_numeric & other)
{
    return !this->operator==(other);
}

template <
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits >
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
operator^(bitwise_numeric & other)
{
    if (lm_checksum != other.lm_checksum) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    bitwise_numeric <BIT_SIZE> ret;
    ret.data = this->data;

    for (unsigned i = 0; i < this->data.size(); i++)
    {
        const uint16_t complement = COMPUTE_8BIT_COMPLIMENT(ret.data[i].bit);
        ret.data[i].num ^= (other.data[i].num & complement);
        ret.data[i].num &= complement;
    }

    return ret;
}

template <
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits >
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
operator&(bitwise_numeric & other)
{
    if (lm_checksum != other.lm_checksum) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    bitwise_numeric <BIT_SIZE> ret;
    ret.data = this->data;

    for (unsigned i = 0; i < this->data.size(); i++)
    {
        const uint16_t complement = COMPUTE_8BIT_COMPLIMENT(ret.data[i].bit);
        ret.data[i].num &= (other.data[i].num & complement);
    }

    return ret;
}

template <
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits>
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits> &
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
operator=(bitwise_numeric && other) noexcept
{
    if (lm_checksum != other.lm_checksum) {
        return *this;
    }

    this->data = other.data;
    return *this;
}

template <
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits>
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>&
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
operator =(const bitwise_numeric& other)
{
    if (lm_checksum != other.lm_checksum) {
        return *this;
    }

    this->data = other.data;
    return *this;
}

template <
    const unsigned BIT_SIZE,
    const unsigned current_bit_size,
    const unsigned required_byte_blocks,
    const unsigned additional_tailing_bits >
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>
bitwise_numeric<BIT_SIZE, current_bit_size, required_byte_blocks, additional_tailing_bits>::
operator|(bitwise_numeric & other)
{
    if (lm_checksum != other.lm_checksum) {
        throw std::runtime_error("Operation between two bitwise-unequal numbers");
    }

    bitwise_numeric <BIT_SIZE> ret;
    ret.data = this->data;

    for (unsigned i = 0; i < this->data.size(); i++)
    {
        const uint16_t complement = COMPUTE_8BIT_COMPLIMENT(ret.data[i].bit);
        ret.data[i].num |= (other.data[i].num & complement);
        ret.data[i].num &= complement;
    }

    return ret;
}

template < 
    const unsigned BIT_SIZE,
	const unsigned current_bit_size,
	const unsigned required_byte_blocks,
	const unsigned additional_tailing_bits >
template < UnsignedIntegral Numeric >

bitwise_numeric < 
    BIT_SIZE,
	current_bit_size,
	required_byte_blocks,
	additional_tailing_bits >

bitwise_numeric < 
    BIT_SIZE,
	current_bit_size,
	required_byte_blocks,
	additional_tailing_bits >::
make_bitwise_numeric(const Numeric number)
{
    const unsigned tailing_compliment = COMPUTE_8BIT_COMPLIMENT(additional_tailing_bits);
    if (required_byte_blocks < 1) {
        throw std::runtime_error("Required byte blocks must be greater than 0");
    }
    bitwise_numeric < BIT_SIZE > ret;

    // init
    for (unsigned i = 0; i < required_byte_blocks; i++)
    {
        ret.data[i].bit = 8;
        ret.data[i].num = 0;
    }

    // copy over
    for (unsigned i = 0; i < std::min(sizeof(Numeric), (uint64_t)required_byte_blocks); i++) {
        ret.data[i].num = ((uint8_t*)&number)[i];
        ret.data[i].bit = 8;
    }

    // tailing
    ret.data[required_byte_blocks - 1].bit = additional_tailing_bits;
    ret.data[required_byte_blocks - 1].num &= tailing_compliment;
    return ret;
}

template<unsigned BIT_SIZE>
std::vector<uint8_t> bitwise_numeric_stack<BIT_SIZE>::dump() const
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
			& COMPUTE_8BIT_COMPLIMENT(tail_size)
			& COMPUTE_8BIT_COMPLIMENT(current_effective_bits);
		// now, we add the tailing current to the previous, forming the conjunction we wanted
		const uint8_t conjuncted = static_cast<uint8_t>((tailing_current << previous_effective_bits) | previous);
        // then, check for effective bits in conjuncted
        const uint8_t conj_effective_bits = std::min(
            static_cast<uint8_t>(previous_effective_bits + current_effective_bits), 
            static_cast<uint8_t>(8));
        // then, compute left_over byte
		// first, remove current's tailing bits 
		uint8_t left_over = current
			& COMPUTE_8BIT_COMPLIMENT(current_effective_bits)
			& (~COMPUTE_8BIT_COMPLIMENT(tail_size));
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
    for (const auto & numeric : string)
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
        ret.push_back(result_holder.conjuncted & COMPUTE_8BIT_COMPLIMENT(result_holder.conj_effective_bits));
    }

    return ret;
}

template<unsigned BIT_SIZE>
void bitwise_numeric_stack<BIT_SIZE>::import(const std::vector<uint8_t> &)
{
}

#endif // LZW_INL
