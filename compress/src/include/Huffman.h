#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <map>

class Huffman {
public:
    Huffman() = default;
    ~Huffman() = default;
    Huffman & operator=(const Huffman &) = delete;
    Huffman(const Huffman &) = delete;
    Huffman & operator=(const Huffman &&) = delete;
    Huffman(Huffman &&) = delete;
    Huffman & operator=(Huffman &&) = delete;

    [[nodiscard]] static
    std::unordered_map < uint8_t, uint64_t > count_data_frequencies(const std::vector<uint8_t> &) ;
};

#endif //HUFFMAN_H
