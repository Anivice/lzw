#include "Huffman.h"
#include "log.hpp"
#include <cstring>

int main()
{
    std::vector < uint8_t > input = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G',
                            'E', 'F', 'G',                'H',
        'A', 'B', 'C', 'D', 'E', 'F', 'G',                'H',
             'B', 'C',
                                                                'S', 'Q',  'P', 'R' };
    debug::set_log_level(debug::L_DEBUG_FG);
    debug::log(debug::to_stderr, debug::debug_log, "Data before encoding: ", input, "\n");

    const auto backup = input;

    std::vector < uint8_t > output, output2;
    Huffman huffman(input, output);
    huffman.compress();
    debug::log(debug::to_stderr, debug::debug_log, "Data after encoding: ", output, "\n");

    Huffman huffman2(output, output2);
    huffman2.decompress();
    debug::log(debug::to_stderr, debug::debug_log, "Data after decoding: ", output2, "\n");

    return std::memcmp(backup.data(), output2.data(), std::min(output2.size(), backup.size()));
}
