#include "absolute_precision_type.h"

void absolute_precision_type::assert_valid_char(const char c)
{
    if (!((c >= '0') && (c <= '9'))) {
        throw std::invalid_argument("absolute_precision_type::assert_valid_char(): Invalid numerical data");
    }
}

char absolute_precision_type::add(char left, char right, bool & carry)
{
    assert_valid_char(left);
    assert_valid_char(right);

    left -= '0';
    right -= '0';

    char result = static_cast<char>(left + right);
    if (result >= 10) {
        result -= 10;
        carry = true;
    } else {
        carry = false;
    }

    return static_cast<char>(result + '0');
}

char absolute_precision_type::sub(char left, char right, bool & carry)
{
    assert_valid_char(left);
    assert_valid_char(right);

    left -= '0';
    right -= '0';

    char result = static_cast<char>(left - right);
    if (result < 0) {
        result += 10;
        carry = true;
    } else {
        carry = false;
    }

    return static_cast<char>(result + '0');
}

void absolute_precision_type::padding_string_len(std::string & str1, std::string & str2)
{
    const auto max = std::max(str1.size(), str2.size());
    const auto padding_on_str1 = max - str1.size();
    const auto padding_on_str2 = max - str2.size();

    str1.insert(str1.end(), padding_on_str1, '0');
    str2.insert(str2.end(), padding_on_str2, '0');
}

void absolute_precision_type::trim()
{
    for (auto i = static_cast<int64_t>(frac.size() - 1); i >= 0; i--) {
        if (frac[i] == '0') {
            frac.pop_back();
        } else {
            return;
        }
    }
}

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

    ret.trim();
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

    ret.trim();
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

    for (auto i = static_cast<int64_t>(shorter.frac.size() - 1); i >= 0; --i)
    {
        std::string current_result;
        char last_mul_carry = '0';
        const auto sig = shorter.frac[i];
        for (auto j = static_cast<int64_t>(longer.frac.size() - 1); j >= 0; --j)
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

absolute_precision_type absolute_precision_type::operator/(const uint64_t other_) const
{
    if (other_ == 1) {
        return *this;
    }

    auto balance = [](std::string & left, std::string & right)
    {
        const auto max = std::max(left.size(), right.size());
        left.insert(left.begin(), max - left.size(), '0');
        right.insert(right.begin(), max - right.size(), '0');
    };

    auto ulltostr = [](uint64_t data)->std::string
    {
        std::string ret;
        while (data != 0) {
            ret += std::to_string(data % 10);
            data /= 10;
        }

        return ret;
    };

    // if left > right?
    auto larger_than = [](const std::string& left, const std::string& right)->bool
    {
        const auto left_literal = strtoll(left.c_str(), nullptr, 10);
        const auto right_literal = strtoll(right.c_str(), nullptr, 10);
        return left_literal > right_literal;
    };

    // wrap to CPU binary calculation
    auto mod = [&ulltostr](
        const std::string& target,
        const std::string& divisor,
        std::string & result,
        std::string & remainder)->bool
    {
        const auto target_literal = std::strtoull(target.c_str(), nullptr, 10);
        const auto divisor_literal = std::strtoull(divisor.c_str(), nullptr, 10);
        result = ulltostr(target_literal / divisor_literal);
        remainder = ulltostr(target_literal % divisor_literal);
        return target_literal % divisor_literal == 0;
    };

    auto this_ = (this->is_one ? std::string(64, '9') : this->frac);
    auto const_this_ = this_;
    std::string that_ = ulltostr(other_);

    std::string result;
    uint64_t index = 0;
    const auto max_result_size = const_this_.size() * 4;
    std::string current_session_target;

    while (true)
    {
        if (result.size() > max_result_size) {
            break;
        }

        current_session_target += const_this_[index];
        index++;
        if (!larger_than(current_session_target, that_))
        {
            result += '0';
            if (index > const_this_.size()) {
                const_this_ += '0'; // padding
            }
        } else {
            std::string remainder, quo;
            balance(current_session_target, that_);
            const auto reminder_is_zero = mod(current_session_target, that_, quo, remainder);
            result += quo;
            if (!reminder_is_zero) {
                current_session_target = remainder;
            } else if (index == const_this_.size()) {
                break;
            } else {
                current_session_target.clear();
            }
        }
    }

    absolute_precision_type ret;
    ret.frac = result;
    ret.is_one = false;
    ret.trim();
    return ret;
}

absolute_precision_type & absolute_precision_type::operator+=(const absolute_precision_type & other)
{
    *this = this->operator+(other);
    return *this;
}

absolute_precision_type & absolute_precision_type::operator-=(const absolute_precision_type & other)
{
    *this = this->operator-(other);
    return *this;
}

absolute_precision_type & absolute_precision_type::operator*=(const absolute_precision_type & other)
{
    *this = this->operator*(other);
    return *this;
}

absolute_precision_type & absolute_precision_type::operator/=(const uint64_t other)
{
    *this = this->operator/(other);
    return *this;
}

bool absolute_precision_type::operator==(const absolute_precision_type & other) const
{
    auto this_ = *this;
    auto other_ = other;

    this_.trim();
    other_.trim();

    return (this_.frac == other_.frac) && (this_.is_one == other_.is_one);
}

bool absolute_precision_type::operator!=(const absolute_precision_type & other) const
{
    return !(*this == other);
}

bool absolute_precision_type::operator<(const absolute_precision_type & other) const
{
    if (this->is_one) {
        return false;
    } else if (other.is_one) {
        return true;
    }

    auto this_ = *this;
    auto other_ = other;

    this_.trim();
    other_.trim();

    padding_string_len(this_.frac, other_.frac);
    for (auto i = static_cast<int64_t>(this_.frac.size() - 1); i >= 0; --i)
    {
        if (this_.frac[i] > other_.frac[i]) {
            return false;
        }
    }

    return true;
}

bool absolute_precision_type::operator>(const absolute_precision_type & other) const
{
    return !((*this < other) && (*this == other));
}

bool absolute_precision_type::operator<=(const absolute_precision_type & other) const
{
    return !(other > *this);
}

bool absolute_precision_type::operator>=(const absolute_precision_type & other) const
{
    return !(other < *this);
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
