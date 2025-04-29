/* arithmetic.h
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

#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include <vector>

namespace arithmetic
{
    constexpr int CODE_VALUE = 16;
    constexpr int MAX_VALUE = ((1 << CODE_VALUE) - 1);
    constexpr int MAX_FREQ = 16383;

    constexpr int FIRST_QTR = (MAX_VALUE / 4 + 1);
    constexpr int HALF = (2 * FIRST_QTR);
    constexpr int THIRD_QTR = (3 * FIRST_QTR);

    constexpr int NO_OF_CHARS = 256;
    constexpr int EOF_SYMBOL = (NO_OF_CHARS + 1);
    constexpr int NO_OF_SYMBOLS = (NO_OF_CHARS + 1);

    class Compress
    {
    public:
        unsigned char index_to_char [NO_OF_SYMBOLS + 1]{};
        int char_to_index [NO_OF_CHARS]{};
        int cum_freq [NO_OF_SYMBOLS + 1]{};
        int freq [NO_OF_SYMBOLS + 1]{};

        Compress();
        void update_tables(int sym_index);
    };

    class Encode : public Compress
    {
        int low, high;
        int opposite_bits;
        int buffer;
        int	bits_in_buf;

        std::vector<uint8_t> & in;
        std::vector<uint8_t> & out;

        void write_bit(int bit);
        void output_bits(int bit);
        void end_encoding();
        void encode_symbol(int symbol);
        [[nodiscard]] int get() const;

    public:
        Encode(std::vector<uint8_t> & in_, std::vector<uint8_t> & out_);
        void encode();
    };

    inline int Encode::get() const
    {
        int result = 0;
        if (!in.empty()) {
            result = in.back();
            in.pop_back();
        } else {
            result = EOF;
        }

        return result;
    }

    class Decode : public Compress
    {
        int low, high;
        int value{};
        int buffer;
        int	bits_in_buf;
        bool end_decoding;

        std::vector<uint8_t> & in;
        std::vector<uint8_t> & out;

        int decode_symbol();
        int get_bit();
        void load_first_value();
        [[nodiscard]] int get() const;

    public:
        Decode(std::vector<uint8_t> & in_, std::vector<uint8_t> & out_);
        void decode();
    };

    inline int Decode::get() const
    {
        int result = 0;
        if (!in.empty()) {
            result = in.back();
            in.pop_back();
        } else {
            result = EOF;
        }

        return result;
    }
}

#endif //ARITHMETIC_H
