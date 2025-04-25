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
    uint64_t max_allowed_precision_level = 256;

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
    static absolute_precision_type make_absolute_precision_type(const std::string &);
};

#endif //ABSOLUTE_PRECISION_TYPE_H
