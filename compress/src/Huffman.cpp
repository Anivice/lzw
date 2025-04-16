#include "Huffman.h"

std::unordered_map<uint8_t, uint64_t> Huffman::count_data_frequencies (const std::vector<uint8_t> & data)
{
    std::unordered_map<uint8_t, uint64_t> result;
    for (uint8_t i = 0; i <= 0xFF; i++) {
        result.emplace(i, 0);
    }

    for (const auto & c : data) {
        result.at(c) += 1;
    }

    return result;
}
