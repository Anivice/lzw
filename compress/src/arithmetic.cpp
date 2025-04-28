#include "arithmetic.h"
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


Encode::Encode(std::basic_istream<char> & in_, std::basic_ostream<char> & out_)
    : in(in_), out(out_)
{
    buffer = 0;
    bits_in_buf = 0;

    low = 0;
    high = MAX_VALUE;
    opposite_bits = 0;
}

void Encode::encode()
{
    if (in.eof() || out.eof()) {
        return;
    }

    while (true)
    {
        const int ch = in.get();
        if (in.eof()) {
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
        low = 2 * low;
        high = 2 * high;
    }
}

void Encode::end_encoding()
{
    opposite_bits++;
    if (low < FIRST_QTR)
        output_bits(0);
    else
        output_bits(1);

    out.put(static_cast<char>(buffer >> bits_in_buf));
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
    if (bit)
        buffer |= 0x80;
    bits_in_buf++;
    if (bits_in_buf == 8)
    {
        out.put(static_cast<char>(buffer));
        bits_in_buf = 0;
    }
}


Decode::Decode(std::basic_istream<char> & in_, std::basic_ostream<char> & out_)
    : in(in_), out(out_)
{
    buffer = 0;
    bits_in_buf = 0;
    end_decoding = false;

    low = 0;
    high = MAX_VALUE;
}

void Decode::load_first_value()
{
    value = 0;
    for (int i = 1; i <= CODE_VALUE; i++)
        value = 2 * value + get_bit();
}

void Decode::decode()
{
    if (!in || !out) {
        return;
    }
    load_first_value();
    while (true)
    {
        const int sym_index = decode_symbol();
        if ((sym_index == EOF_SYMBOL) || end_decoding)
            break;
        const int ch = index_to_char[sym_index];
        out.put(static_cast<char>(ch));
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
        low = 2 * low;
        high = 2 * high;
        value = 2 * value + get_bit();
    }
    return symbol_index;
}

int Decode::get_bit()
{
    if (bits_in_buf == 0)
    {
        buffer = in.get();
        if (buffer == EOF)
        {
            end_decoding = true;
            return -1;
        }
        bits_in_buf= 8;
    }
    const int t = buffer & 1;
    buffer >>= 1;
    bits_in_buf--;
    return t;
}
