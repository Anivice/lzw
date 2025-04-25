#include "absolute_precision_type.h"

absolute_precision_type absolute_precision_type::operator+(
const absolute_precision_type & other_) const
{
    absolute_precision_type ret = *this;
    absolute_precision_type other = other_;
    padding_string_len(ret.frac, other.frac);

    bool last_carry = false;
    for (long i = static_cast<long>(ret.frac.size() - 1); i >= 0; --i)
    {
        bool carry = false;
        bool carry_in_carry = false;

        ret.frac[i] = add(ret.frac[i], other.frac[i], carry);
        if (last_carry) {
            ret.frac[i] = add(ret.frac[i], '1', carry_in_carry);
        }

        last_carry = carry || carry_in_carry;
    }

    if (last_carry) { // being 1, if all is 0
        bool all_is_0 = true;
        for (const auto c : ret.frac)
        {
            if (c != '0') {
                all_is_0 = false;
                break;
            }
        }

        if (all_is_0 && !ret.is_one) {
            ret.is_one = true;
            return ret;
        }

        // not all is 0, or is already one
        throw std::overflow_error("absolute_precision_type(): Floating point overflow: "
            "Precision level exceeded for number exceeding 1");
    }

    return ret;
}

absolute_precision_type absolute_precision_type::operator-(
    const absolute_precision_type & other_) const
{
    absolute_precision_type ret = *this;
    absolute_precision_type other = other_;
    padding_string_len(ret.frac, other.frac);

    bool last_carry = false;
    for (long i = static_cast<long>(ret.frac.size() - 1); i >= 0; --i)
    {
        bool carry = false;
        bool carry_in_carry = false;

        ret.frac[i] = sub(ret.frac[i], other.frac[i], carry);
        if (last_carry) {
            ret.frac[i] = sub(ret.frac[i], '1', carry_in_carry);
        }

        last_carry = carry || carry_in_carry;
    }

    if (last_carry) {
        if (ret.is_one) {
            ret.is_one = false;
        } else {
            throw std::overflow_error("absolute_precision_type(): Floating point overflow: "
                "Subtraction leading to negative number shrinking below 0");
        }
    }

    return ret;
}

absolute_precision_type absolute_precision_type::operator*(
    const absolute_precision_type & other) const
{
    if (this->is_one && other.is_one) {
        absolute_precision_type ret;
        ret.is_one = true;
        return ret;
    } else if (this->is_one) {
        return other;
    } else if (other.is_one) {
        return *this;
    }

    auto mul = [](char left, char right, char & mul_carry, char & singleton)
    {
        assert_valid_char(left);
        assert_valid_char(right);
        left -= '0';
        right -= '0';
        const auto result = left * right;
        singleton = static_cast<char>(result % 10 + '0');
        mul_carry = static_cast<char>(result / 10 + '0');
    };

    auto find_max = [](const std::vector<std::string> & vec)->uint64_t
    {
        uint64_t ret = 0;
        for (const auto & str : vec) {
            if (str.size() > ret) {
                ret = str.size();
            }
        }

        return ret;
    };

    auto add_two_strings = [](const std::string & left, const std::string & right)->std::string
    {
        if (left.size() != right.size()) {
            throw std::invalid_argument("Provided data has different precisions");
        }

        std::string ret = left;
        bool last_carry = false;
        for (long i = static_cast<long>(left.size() - 1); i >= 0; --i)
        {
            bool carry = false;
            bool carry_in_carry = false;

            ret[i] = add(ret[i], right[i], carry);
            if (last_carry) {
                ret[i] = add(ret[i], '1', carry_in_carry);
            }

            last_carry = carry || carry_in_carry;
        }

        if (last_carry) {
            throw std::overflow_error("absolute_precision_type(): Floating point overflow: "
                "Precision level exceeded for number exceeding 1");
        }

        return ret;
    };

    std::vector < std::string > container;
    absolute_precision_type this_ = *this, that_ = other;
    this_.trim();
    that_.trim();
    const uint64_t new_precision = this_.frac.size() + that_.frac.size();

    absolute_precision_type shorter = (this_.frac.size() < that_.frac.size() ? this_ : that_);
    absolute_precision_type longer = (this_.frac.size() > that_.frac.size() ? this_ : that_);

    for (int64_t i = shorter.frac.size() - 1; i >= 0; --i)
    {
        std::string current_result;
        char last_mul_carry = '0';
        const auto sig = shorter.frac[i];
        for (int64_t j = longer.frac.size() - 1; j >= 0; --j)
        {
            const auto other_sig = longer.frac[j];
            char singleton = '0';
            char mul_carry = '0';

            mul(sig, other_sig, mul_carry, singleton);
            if (last_mul_carry != '0')
            {
                bool carry_carry = false;
                singleton = add(singleton, last_mul_carry, carry_carry);
                if (carry_carry) {
                    mul_carry += 1;
                }
            }

            current_result.push_back(singleton);
            last_mul_carry = mul_carry;
        }

        if (last_mul_carry != '0') {
            current_result.push_back(last_mul_carry);
        }

        std::ranges::reverse(current_result);
        const auto padding = std::string(container.size(), '0');
        current_result += padding;
        container.push_back(current_result);
    }

    const auto new_size = std::max(static_cast<uint64_t>(find_max(container)), new_precision);
    for (auto & s : container) {
        const auto padding = std::string(new_size - s.size(), '0');
        s.insert(s.begin(), padding.begin(), padding.end());
    }

    // now, sum all of them, one by one
    auto result = std::string(new_size, '0');
    for (auto & float_numeric : container) {
        result = add_two_strings(float_numeric, result);
    }

    absolute_precision_type ret;
    ret.frac = result;
    ret.is_one = false;
    ret.trim();
    return ret;
}

std::string absolute_precision_type::to_string() const {
    auto braced = *this;
    braced.trim();
    return (is_one ? "1" : "0." + braced.frac);
}

absolute_precision_type absolute_precision_type::make_absolute_precision_type(const std::string & result)
{
    if (result.empty()) {
        return {};
    }

    absolute_precision_type one;
    one.is_one = true;

    if (result[0] == '1')
    {
        if (result.size() == 1) {
            return one;
        } else {
            throw std::overflow_error("absolute_precision_type(): data precision overflow");
        }
    }

    if (result[0] == '0' && result.size() == 1) {
        return {};
    }

    if (result.size() > 1 && result[0] == '0' && result[1] == '.')
    {
        absolute_precision_type ret;
        ret.is_one = false;
        ret.frac = result.substr(2, result.size() - 2);
        return ret;
    }

    throw std::invalid_argument("absolute_precision_type(): invalid data");
}
