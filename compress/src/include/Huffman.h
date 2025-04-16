#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <cstdint>
#include <vector>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <map>

class Huffman {
private:
    using frequency_map = std::vector < std::pair < uint8_t, uint64_t > >;
    std::vector < uint8_t > & input_data_;
    std::vector < uint8_t > & output_data_;

    struct Node
    {
        std::vector < uint8_t > codes;
        uint8_t original_uint8_code{};
        uint64_t frequency{};
        bool am_i_valid = false;

        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };

    struct single_mixed_node {
        uint8_t code{};
        uint64_t frequency{};
        std::unique_ptr<Node> node;
        bool am_i_valid = false;
    };

    using full_map_t = std::vector < single_mixed_node >;
    full_map_t full_map;
    std::map < uint8_t, std::string, std::less<> > encoded_pairs;
    std::string raw_dump;

public:
    Huffman(
        std::vector < uint8_t > & input_data,
        std::vector < uint8_t > & output_data)
    : input_data_(input_data), output_data_(output_data) {
        if (input_data_.empty()) {
            throw std::invalid_argument("Input data is empty");
        }
    }

    ~Huffman() = default;
    Huffman & operator=(const Huffman &) = delete;
    Huffman(const Huffman &) = delete;
    Huffman & operator=(const Huffman &&) = delete;
    Huffman(Huffman &&) = delete;
    Huffman & operator=(Huffman &&) = delete;

    void count_data_frequencies();
    void build_binary_tree_based_on_the_frequency_map();
    void walk_through_tree();
    void walk_to_next_node(uint64_t, const Node &, const std::string &);
    void encode_using_constructed_pairs();
    [[nodiscard]] std::vector < uint8_t > convert_std_string_to_std_vector_from_raw_dump(uint64_t &) const;
    [[nodiscard]] static uint8_t std_string_to_uint8_t(const std::string &);
    [[nodiscard]] static std::string uint8_t_to_std_string(uint8_t);
    [[nodiscard]] std::vector < uint8_t > export_table();
    void import_table(const std::vector < uint8_t > &);
    void decode_using_constructed_pairs();
};

#endif //HUFFMAN_H
