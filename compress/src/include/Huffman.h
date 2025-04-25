/* Huffman.h
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


#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <cstdint>
#include <vector>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <map>

class Huffman {
public:
    using frequency_map = std::vector < std::pair < uint8_t, uint64_t > >;

private:
    std::vector < uint8_t > & input_data_;
    std::vector < uint8_t > & output_data_;

    struct Node
    {
        uint8_t original_uint8_code{};
        uint64_t frequency{};
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        uint64_t depth = 0;
    };

    struct single_mixed_node {
        uint8_t code{};
        uint64_t frequency{};
        std::unique_ptr<Node> node;
        bool am_i_valid = false;
        bool am_i_selected = false;
    };

    using full_map_t = std::vector < single_mixed_node >;
    full_map_t full_map;
    std::map < uint8_t, std::string, std::less<> > encoded_pairs;
    std::string raw_dump;
    frequency_map frequency_map_;

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
    void convert_input_to_raw_dump(uint64_t bits);
    void decode_using_constructed_pairs();

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

    void compress();
    void decompress();

    [[nodiscard]] frequency_map get_frequency_map() const { return frequency_map_; }
};

#endif //HUFFMAN_H
