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

    static void assert_valid_char(char c);
    static char add(char left, char right, bool & carry);
    static char sub(char left, char right, bool & carry);
    static void padding_string_len(std::string & str1, std::string & str2);
    void trim();

public:
    absolute_precision_type() = default;
    absolute_precision_type(const absolute_precision_type&) = default;
    absolute_precision_type(absolute_precision_type&&) noexcept = default;
    absolute_precision_type & operator=(const absolute_precision_type&) = default;

    absolute_precision_type operator+(const absolute_precision_type&) const;
    absolute_precision_type operator-(const absolute_precision_type&) const;
    absolute_precision_type operator*(const absolute_precision_type&) const;
    absolute_precision_type operator/(uint64_t) const;
    absolute_precision_type & operator+=(const absolute_precision_type&);
    absolute_precision_type & operator-=(const absolute_precision_type&);
    absolute_precision_type & operator*=(const absolute_precision_type&);
    absolute_precision_type & operator/=(uint64_t);

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
