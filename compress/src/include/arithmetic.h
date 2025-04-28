#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include <iostream>

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
        unsigned char index_to_char [NO_OF_SYMBOLS]{};
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

        std::basic_istream<char> & in;
        std::basic_ostream<char> & out;

        void write_bit( int bit);
        void output_bits(int bit);
        void end_encoding();
        void encode_symbol(int symbol);

    public:
        Encode(std::basic_istream<char> & in_, std::basic_ostream<char> & out_);
        void encode();
    };

    class Decode : public Compress
    {
        int low, high;
        int value{};
        int buffer;
        int	bits_in_buf;
        bool end_decoding;

        std::basic_istream<char> & in;
        std::basic_ostream<char> & out;

        int decode_symbol();
        int get_bit();
        void load_first_value();

    public:
        Decode(std::basic_istream<char> & in_, std::basic_ostream<char> & out_);
        void decode();
    };
}

#endif //ARITHMETIC_H
