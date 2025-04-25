#ifndef ABSOLUTE_PRECISION_TYPE_H
#define ABSOLUTE_PRECISION_TYPE_H

#include <algorithm>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

// absolute precision for numbers within [0, 1]
class absolute_precision_type
{
private:
    std::string frac = std::string(2, '0');
    bool is_one = false;

    static void assert_valid_char(const char c)
    {
        if (!((c >= '0') && (c <= '9'))) {
            throw std::invalid_argument("absolute_precision_type::assert_valid_char(): Invalid numerical data");
        }
    }

    static char add(char left, char right, bool & carry)
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

    static char sub(char left, char right, bool & carry)
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

    static void padding_string_len(std::string & str1, std::string & str2)
    {
        const auto max = std::max(str1.size(), str2.size());
        const auto padding_on_str1 = max - str1.size();
        const auto padding_on_str2 = max - str2.size();

        str1.insert(str1.end(), padding_on_str1, '0');
        str2.insert(str2.end(), padding_on_str2, '0');
    }

    void trim()
    {
        for (int64_t i = frac.size() - 1; i >= 0; i--) {
            if (frac[i] == '0') {
                frac.pop_back();
            } else {
                return;
            }
        }
    }

public:
    absolute_precision_type() = default;
    absolute_precision_type(const absolute_precision_type&) = default;
    absolute_precision_type(absolute_precision_type&&) noexcept = default;
    absolute_precision_type & operator=(const absolute_precision_type&) = default;

    absolute_precision_type operator+(const absolute_precision_type&) const;
    absolute_precision_type operator-(const absolute_precision_type&) const;
    absolute_precision_type operator*(const absolute_precision_type&) const;
    absolute_precision_type operator/(const absolute_precision_type&) const;
    absolute_precision_type operator%(const absolute_precision_type&) const;
    absolute_precision_type & operator+=(const absolute_precision_type&);
    absolute_precision_type & operator-=(const absolute_precision_type&);
    absolute_precision_type & operator*=(const absolute_precision_type&);
    absolute_precision_type & operator/=(const absolute_precision_type&);
    absolute_precision_type & operator%=(const absolute_precision_type&);
    absolute_precision_type & operator++(int);
    absolute_precision_type & operator--(int);

    bool operator==(const absolute_precision_type&) const;
    bool operator!=(const absolute_precision_type&) const;
    bool operator<(const absolute_precision_type&) const;
    bool operator>(const absolute_precision_type&) const;
    bool operator<=(const absolute_precision_type&) const;
    bool operator>=(const absolute_precision_type&) const;

    [[nodiscard]] std::string to_string() const;

    template < typename FloatingPointType > requires std::is_floating_point_v<FloatingPointType>
    static absolute_precision_type make_absolute_precision_type(FloatingPointType);
    static absolute_precision_type make_absolute_precision_type(const std::string &);
};

template<typename FloatingPointType> requires std::is_floating_point_v<FloatingPointType>
absolute_precision_type absolute_precision_type::make_absolute_precision_type(const FloatingPointType data)
{
    if (data > 1 || data < 0) {
        throw std::overflow_error("absolute_precision_type::make_absolute_precision_type()");
    }

    if (data == 1) {
        absolute_precision_type ret;
        ret.is_one = true;
        return ret;
    }

    auto integer_part = static_cast<uint64_t>(data * 10);
    uint64_t scale = 1;
    while (data - static_cast<FloatingPointType>(integer_part) > 0) {
        integer_part = static_cast<uint64_t>(data * pow(10, scale));
        scale++;
    }

    std::string result;
    while (integer_part > 0) {
        result += static_cast<char>(integer_part % 10 + '0');
        integer_part /= 10;
    }

    std::ranges::reverse(result);
    absolute_precision_type ret;
    ret.is_one = false;
    ret.frac = result;
    return ret;
}

#endif //ABSOLUTE_PRECISION_TYPE_H
