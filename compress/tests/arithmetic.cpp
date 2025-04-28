#include "arithmetic.h"
#include <vector>
#include <sstream>

int main()
{
    // data with various frequencies
    const std::vector < char > data = {
        'A', 'B', 'C', 'D', 'E', 'F',
        'A',      'C', 'D',      'F',
                  'C', 'D', 'E', 'F', 'G',
        'A',      'C', 'D',      'F',
        'A',      'C',           'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    /*   4    1    5    4    2    5    2    1    1    1    1    1    1    */
    /*  We have 29 symbols, with 13 distinctive symbols */
    };

    std::stringstream input, output;
    for (const char c : data) {
        input << c;
    }

    arithmetic::Encode compressor(input, output);
    compressor.encode();

    const auto str = output.str();

    arithmetic::Decode decompressor(output, input);
    decompressor.decode();

    std::cout << input.str() << std::endl;
}
