/* arithmetic.cpp
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

#include <algorithm>
#include "arithmetic.h"
#include <cstdio>
#include <cstdint>

using namespace arithmetic;

Compress::Compress()
{
    int i;
    for ( i = 0; i < NO_OF_CHARS; i++)
    {
        char_to_index[i] = i + 1;
        index_to_char[i + 1] = i;
    }
    for ( i = 0; i <= NO_OF_SYMBOLS; i++)
    {
        freq[i] = 1;
        cum_freq[i] = NO_OF_SYMBOLS - i;
    }
    freq[0] = 0;
}

void Compress::update_tables(const int sym_index)
{
    int i;
    if (cum_freq[0] == MAX_FREQ)
    {
        int cum = 0;
        for ( i = NO_OF_SYMBOLS; i >= 0; i--)
        {
            freq[i] = (freq[i] + 1) / 2;
            cum_freq[i] = cum;
            cum += freq[i];
        }
    }
    for ( i = sym_index; freq[i] == freq[i - 1]; i--) {}
    if (i < sym_index)
    {
        const int ch_i = index_to_char[i];
        const int ch_symbol = index_to_char[sym_index];
        index_to_char[i] = ch_symbol;
        index_to_char[sym_index] = ch_i;
        char_to_index[ch_i] = sym_index;
        char_to_index[ch_symbol] = i;
    }
    freq[i]++;
    while (i > 0)
    {
        i--;
        cum_freq[i]++;
    }
}


Encode::Encode(std::vector<uint8_t> & in_, std::vector<uint8_t> & out_)
    : in(in_), out(out_)
{
    buffer = 0;
    bits_in_buf = 0;

    low = 0;
    high = MAX_VALUE;
    opposite_bits = 0;
    std::ranges::reverse(in);
}

void Encode::encode()
{
    if (in.empty()) {
        return;
    }

    while (true)
    {
        const int ch = get();
        if (ch == EOF) {
            break;
        }
        const int symbol = char_to_index[ch];
        encode_symbol(symbol);
        update_tables(symbol);
    }
    encode_symbol(EOF_SYMBOL);
    end_encoding();
}

void Encode::encode_symbol(const int symbol)
{
    const int range = high - low;
    high = low + (range * cum_freq [symbol - 1]) / cum_freq [0];
    low = low + (range * cum_freq [symbol]) / cum_freq [0];
    for (;;)
    {
        if (high < HALF)
            output_bits(0);
        else if (low >= HALF)
        {
            output_bits(1);
            low -= HALF;
            high -= HALF;
        }
        else if (low >= FIRST_QTR && high < THIRD_QTR)
        {
            opposite_bits++;
            low -= FIRST_QTR;
            high -= FIRST_QTR;
        }
        else
            break;
        low  <<= 1;
        high <<= 1;
    }
}

void Encode::end_encoding()
{
    opposite_bits++;
    output_bits( (low < FIRST_QTR) ? 0 : 1 );

    /* write the leftovers (if any) in the order
       the decoder expects: LSB first                 */
    if (bits_in_buf > 0)
        out.push_back(static_cast<uint8_t>(buffer >> (8 - bits_in_buf)));
}

void Encode::output_bits(const int bit)
{
    write_bit(bit);
    while (opposite_bits > 0)
    {
        write_bit(!bit);
        opposite_bits--;
    }
}

void Encode::write_bit(const int bit)
{
    buffer >>= 1;
    if (bit) buffer |= 0x80;
    if (++bits_in_buf == 8) {
        out.push_back(static_cast<uint8_t>(buffer));
        bits_in_buf = 0;
        buffer      = 0;
    }
}

Decode::Decode(std::vector<uint8_t> & in_, std::vector<uint8_t> & out_)
    : in(in_), out(out_)
{
    buffer = 0;
    bits_in_buf = 0;
    end_decoding = false;

    low = 0;
    high = MAX_VALUE;
    std::ranges::reverse(in);
}

void Decode::load_first_value()
{
    value = 0;
    for (int i = 1; i <= CODE_VALUE; i++)
        value = 2 * value + get_bit();
}

void Decode::decode()
{
    if (in.empty()) {
        return;
    }

    load_first_value();
    while (true)
    {
        const int sym_index = decode_symbol();
        if (sym_index == EOF_SYMBOL) {
            break;
        }
        const int ch = index_to_char[sym_index];
        out.push_back(static_cast<uint8_t>(ch));
        update_tables(sym_index);
    }
}

int Decode::decode_symbol()
{
    int symbol_index;

    const int range = high - low;
    const int cum = ((((value - low) + 1) * cum_freq[0] - 1) / range);
    for (symbol_index = 1; cum_freq[symbol_index] > cum; symbol_index++) {}
    high = low + (range * cum_freq[symbol_index - 1]) / cum_freq[0];
    low = low + (range * cum_freq[symbol_index]) / cum_freq[0];
    for (;;)
    {
        if (high < HALF)
        {
        }
        else if (low >= HALF)
        {
            value -= HALF;
            low -= HALF;
            high -= HALF;
        }
        else if (low >= FIRST_QTR && high < THIRD_QTR)
        {
            value -= FIRST_QTR;
            low -= FIRST_QTR;
            high -= FIRST_QTR;
        }
        else
            break;
        low  <<= 1;
        high <<= 1;
        value = 2 * value + get_bit();
    }
    return symbol_index;
}

int Decode::get_bit()
{
    if (bits_in_buf == 0)
    {
        buffer = get();
        if (buffer == EOF)
        {
            end_decoding = true;
            return 0;
        }

        bits_in_buf= 8;
    }
    const int t = buffer & 1;
    buffer >>= 1;
    bits_in_buf--;
    return t;
}
