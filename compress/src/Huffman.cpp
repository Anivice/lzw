/* Huffman.cpp
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


#include "Huffman.h"
#include <vector>
#include <algorithm>
#include "log.hpp"
#include <sstream>
#include <bitset>
#include <ranges>
#include "numeric.h"

void Huffman::count_data_frequencies()
{
    frequency_map frequency_map_;
    frequency_map_.resize(256);
    for (const auto & c : input_data_) {
        frequency_map_[c].second += 1;
        frequency_map_[c].first = c;
    }

    std::ranges::sort(frequency_map_,
        [](const std::pair<uint8_t, uint64_t>& a,
                    const std::pair<uint8_t, uint64_t>& b)->bool
                {
                    return a.second > b.second;
                });
    std::erase_if(frequency_map_,
                  [](const std::pair<uint8_t, uint64_t>& pair) {
                      return pair.second == 0;
                  });

    // construct a dynamic full map
    for (const auto & [byte, freq] : frequency_map_)
    {
        single_mixed_node node;
        node.code = byte;
        node.frequency = freq;
        node.node = nullptr;
        node.am_i_valid = true;
        full_map.emplace_back(std::move(node));
    }
}

void Huffman::build_binary_tree_based_on_the_frequency_map()
{
    auto construct_two_branches_with_one_root_based_on_provided_two_data = [&](
        Node & parent,
        single_mixed_node& data1,
        single_mixed_node& data2,
        const uint64_t child_depth)->void
    {
        uint64_t freq_combined = 0;
        if (data1.node == nullptr)
        {
            Node node;
            node.original_uint8_code = data1.code;
            node.frequency = data1.frequency;

            parent.left = std::make_unique<Node>(std::move(node));
            freq_combined += data1.frequency;
        } else {
            parent.left = std::move(data1.node);
            freq_combined += parent.left->frequency;
        }

        if (data2.node == nullptr)
        {
            Node node;
            node.original_uint8_code = data2.code;
            node.frequency = data2.frequency;
            parent.right = std::make_unique<Node>(std::move(node));
            freq_combined += data2.frequency;
        } else {
            parent.right = std::move(data2.node);
            freq_combined += parent.right->frequency;
        }

        parent.original_uint8_code = 0;
        parent.frequency = freq_combined;
        parent.depth = child_depth + 1;
    };

    auto construct_dangling_nodes_based_on_full_map = [&](
        uint64_t max_freq)->uint64_t
    {
        if (full_map.empty()) {
            return 0;
        }

        uint64_t lowest_possible_depth = UINT64_MAX;
        // find out the lowest depth and the second-lowest depth
        // use map to remove duplication automatically
        std::vector < uint64_t > depths;
        for (const auto & node : full_map)
        {
            if (node.node != nullptr) {
                if (node.node->depth < lowest_possible_depth) {
                    lowest_possible_depth = node.node->depth;
                }

                depths.emplace_back(node.node->depth);
            } else {
                lowest_possible_depth = 0;
                depths.emplace_back(0);
            }
        }

        std::ranges::sort(depths, [](const uint64_t a, const uint64_t b) {
            return a < b;
        });

        bool lowest_possible_depth_found = false;
        for (const auto & depth : depths) {
            if (depth > lowest_possible_depth) {
                lowest_possible_depth = depth + 1; // I don't fucking care if it's the third one, it already surpasses the second
                lowest_possible_depth_found = true;
                break;
            }
        }

        if (!lowest_possible_depth_found) {
            lowest_possible_depth += 1;
        }

        std::vector < single_mixed_node * > acceptable_map;
        acceptable_map.reserve(full_map.size());
        std::vector < std::unique_ptr < Node > > returned_nodes;

        auto assign_acceptable_nodes = [&](single_mixed_node & it_full)->void
        {
            uint64_t current_depth = 0;
            if (it_full.node != nullptr) {
                current_depth = it_full.node->depth;
            }

            // merge the lowest depth possible
            if (current_depth < lowest_possible_depth)
            {
                if (!it_full.am_i_selected) {
                    acceptable_map.emplace_back(&it_full);
                    it_full.am_i_selected = true;
                }
            }
        };

        // select least frequencies of similar depths
        uint64_t last_selected_map_size = 0;
        while (acceptable_map.size() < 2)
        {
            auto it_full = full_map.end() - 1;
            bool max_freq_reached = false;
            for (;it_full > full_map.begin(); --it_full)
            {
                if (it_full->frequency <= max_freq) // least frequency
                {
                    assign_acceptable_nodes(*it_full);
                } else {
                    max_freq_reached = true;
                    break;
                }
            }

            if (it_full == full_map.begin() && !max_freq_reached) {
                // !max_freq_reached but it'a already at the beginning
                assign_acceptable_nodes(*it_full);
            }

            if (last_selected_map_size == acceptable_map.size())
            {
                lowest_possible_depth++;
                max_freq++;
            }

            last_selected_map_size = acceptable_map.size();
        }

        auto it = acceptable_map.begin();
        while (true)
        {
            if ((it == acceptable_map.end()) || ((it + 1) == acceptable_map.end())) {
                break;
            }

            uint64_t preceding_depth = 0;
            if ((*it)->node != nullptr) {
                preceding_depth += (*it)->node->depth;
            }

            if ((*(it + 1))->node != nullptr) {
                preceding_depth += (*(it + 1))->node->depth;
            }

            returned_nodes.emplace_back(std::make_unique<Node>());
            construct_two_branches_with_one_root_based_on_provided_two_data(
                *returned_nodes.back(), **it, **(it +1), preceding_depth);
            (*it)->am_i_valid = false;
            (*(it + 1))->am_i_valid = false;
            it += 2;
        }

        // insert new nodes
        for (auto & node : returned_nodes)
        {
            single_mixed_node new_node;
            new_node.code = 0;
            new_node.frequency = node->frequency;
            new_node.am_i_valid = true;
            new_node.node = std::move(node);
            full_map.emplace_back(std::move(new_node));
        }

        // remove old nodes
        std::erase_if(full_map,
              [](const single_mixed_node& node) {
                  return node.am_i_valid == false;
              });

        // sort map again
        std::ranges::sort(full_map,
        [](const single_mixed_node& a, const single_mixed_node& b)->bool
            {
                return a.frequency > b.frequency;
            });

        // reset selection bit
        for (auto & node : full_map) {
            node.am_i_selected = false;
        }

        return max_freq;
    };

    auto find_second_least_in_map = [&]()->uint64_t
    {
        if (full_map.empty()) {
            return 0;
        }

        if (full_map.size() == 1) {
            return full_map.back().frequency;
        }

        uint64_t last_frequency = full_map.back().frequency;
        for (auto it = full_map.end() - 1; it > full_map.begin(); --it)
        {
            if (it->frequency > last_frequency) {
                return it->frequency;
            }

            last_frequency = it->frequency;
        }

        return full_map.front().frequency;
    };

    uint64_t freq_threshold = find_second_least_in_map();
    while (full_map.size() != 1)
    {
        const auto returned_max_freq = construct_dangling_nodes_based_on_full_map(freq_threshold);
        const auto freq_threshold_new = find_second_least_in_map();
        freq_threshold = std::max(freq_threshold_new, returned_max_freq);
    }
}

void Huffman::walk_through_tree()
{
    if (full_map.empty()) { // nothing
        return;
    }

    if (full_map.front().node == nullptr) // one node and one node only
    {
        if (full_map.size() != 1) {
            throw std::runtime_error("Huffman doesn't have a single node, possibly internal bug");
        }

        encoded_pairs.emplace(full_map.front().code, "1");
        return;
    }

    walk_to_next_node(0, *full_map.front().node, "");
}

void Huffman::walk_to_next_node(const uint64_t depth, const Node & node, const std::string & prefix)
{
    if (node.left == nullptr && node.right == nullptr) {
        encoded_pairs.emplace(node.original_uint8_code, prefix);
    }

    if (node.left != nullptr) {
        walk_to_next_node(depth + 1, *node.left, prefix + "0");
    }

    if (node.right != nullptr) {
        walk_to_next_node(depth + 1, *node.right, prefix + "1");
    }
}

void Huffman::encode_using_constructed_pairs()
{
    std::ranges::reverse(input_data_);
    auto get = [&](uint8_t & c)->void {
        c = input_data_.back();
        input_data_.pop_back();
    };

    std::stringstream ss;
    raw_dump.reserve(input_data_.size() * 8);
    while (!input_data_.empty())
    {
        uint8_t c;
        get(c);
        ss << encoded_pairs.at(c);
    }

    raw_dump = ss.str();
}

std::vector<uint8_t> Huffman::convert_std_string_to_std_vector_from_raw_dump(uint64_t & bitSize) const
{
    std::vector<uint8_t> ret;
    ret.reserve(raw_dump.size() / 8 + raw_dump.size() % 8);
    for (int i = 0; i < raw_dump.size() / 8; i ++)
    {
        const std::string byte_stream = raw_dump.substr(i * 8, 8);
        const uint8_t byte = std_string_to_uint8_t(byte_stream);
        ret.push_back(byte);
    }

    if (raw_dump.size() % 8 != 0) {
        const std::string byte_stream = raw_dump.substr(raw_dump.size() / 8 * 8);
        const uint8_t byte = std_string_to_uint8_t(byte_stream);
        ret.push_back(byte);
    }

    bitSize = raw_dump.size();
    return ret;
}

uint8_t Huffman::std_string_to_uint8_t(const std::string & str)
{
    if (str.size() > 8) {
        throw std::invalid_argument("String too long, must be less than or equal to 8");
    }

    uint8_t ret = 0;
    const auto bit_offset = str.size() - 1;
    for (size_t i = 0; i < str.size(); i++) {
        ret |= (0x01 & (str[i] == '1' ? 0x01 : 0x00)) << (bit_offset - i);
    }

    return ret;
}

std::string Huffman::uint8_t_to_std_string(const uint8_t byte)
{
    std::string ret;

    for (size_t i = 0; i < 8; i++) {
        const uint8_t compliment = 0x01 << (7 - i);
        const uint8_t bit_position = byte & compliment;
        ret += bit_position == 0 ? "0" : "1";
    }

    return ret;
}

std::vector<uint8_t> Huffman::export_table()
{
    std::vector<uint8_t> ret;
    std::unordered_map < uint8_t, uint8_t > table_entry_and_key_size;
    std::string value_entry;
    bitwise_numeric_stack<4> byteStreamLength;

    auto str_to_uint8 = [](const std::string & bitStream)->std::vector<uint8_t>
    {
        std::vector<uint8_t> ret_str_to_uint8;
        for (int i = 0; i < bitStream.size() / 8; i ++)
        {
            const std::string byte_stream = bitStream.substr(i * 8, 8);
            const uint8_t encoded_byte = std_string_to_uint8_t(byte_stream);
            ret_str_to_uint8.push_back(encoded_byte);
        }

        if (bitStream.size() % 8 != 0) {
            const std::string byte_stream = bitStream.substr(bitStream.size() / 8 * 8);
            const uint8_t encoded_byte = std_string_to_uint8_t(byte_stream);
            ret_str_to_uint8.push_back(encoded_byte);
        }

        return ret_str_to_uint8;
    };

    // dump table
    for (const auto & [byte, bitStream] : encoded_pairs)
    {
        if (bitStream.size() > 0xF /* Worst case for a 64 KB long data, and our utility is using 16 KB */) {
            throw std::invalid_argument(R"(Bitstream too long, must be less than 16 bits (WTF data did you provide???))");
        }
        const auto bitSize = static_cast<uint8_t>(bitStream.size());
        table_entry_and_key_size.emplace(byte, bitSize);
        value_entry += bitStream;
    }

    // dump key size
    for (int i = 0; i < 256; i++) {
        uint8_t bitSize = 0;
        if (table_entry_and_key_size.contains(i)) {
            bitSize = table_entry_and_key_size.at(i);
        }
        byteStreamLength.emplace(bitSize);
    }

    // dump key size to vectorized data
    const auto byteStreamLengthVec = byteStreamLength.dump();

    // code byte stream
    const auto val_vec_entry = str_to_uint8(value_entry);

    // [ (256 * 4 / 8 = 128 Bytes) ] [ Encoding Byte Stream ]
    // 1. push each byte stream encoding length (16 bits max)
    ret.insert(ret.end(), begin(byteStreamLengthVec), end(byteStreamLengthVec));
    // 2. push encoding byte stream data
    ret.insert(ret.end(), begin(val_vec_entry), end(val_vec_entry));
    return ret;
}

void Huffman::import_table(const std::vector<uint8_t> & table_)
{
    std::streamsize get_off = 0;
    auto uint8_to_str = [](const std::vector<uint8_t> & byteStream, const uint64_t bits)->std::string
    {
        std::string ret_uint8_to_str;
        for (int i = 0; i < bits / 8; i ++)
        {
            const std::string byte_stream = uint8_t_to_std_string(byteStream[i]);
            ret_uint8_to_str += byte_stream;
        }

        if (bits % 8 != 0) {
            const uint64_t tailing_bits = bits % 8;
            std::string byte_stream = uint8_t_to_std_string(byteStream.back());
            byte_stream = byte_stream.substr(byte_stream.size() - tailing_bits, tailing_bits);
            ret_uint8_to_str += byte_stream;
        }

        return ret_uint8_to_str;
    };

    // 1. import bit string length
    std::vector<uint8_t> length_data;
    bitwise_numeric_stack<4> byteStreamLength;
    constexpr auto bytes_in_byte_stream = 128;
    length_data.insert(end(length_data),
        begin(table_), begin(table_) + bytes_in_byte_stream);
    byteStreamLength.import(length_data, 256);
    get_off += bytes_in_byte_stream;

    // 2. copy encoding table
    // 2.1 copy raw data
    std::vector<uint8_t> table_vals;
    table_vals.insert(end(table_vals), begin(table_) + get_off, end(table_));
    // 2.2 get accumulated bit size
    uint64_t bits_total = 0;
    for (uint64_t i = 0; i < byteStreamLength.size(); i++) {
        bits_total += byteStreamLength[i].export_numeric<uint64_t>();
    }

    // recover bit stream
    const std::string bitStream = uint8_to_str(table_vals, bits_total);

    // assign bit stream
    uint64_t bit_stream_offset = 0;
    for (int i = 0; i < 256; i++)
    {
        const auto & byte = byteStreamLength[i];
        const auto current_bit_size = byte.export_numeric<uint8_t>();
        if (current_bit_size != 0)
        {
            encoded_pairs[i] = bitStream.substr(bit_stream_offset, current_bit_size);
            bit_stream_offset += current_bit_size;
        }
    }
}

void Huffman::convert_input_to_raw_dump(const uint64_t bits)
{
    raw_dump.reserve(bits);
    for (int i = 0; i < bits / 8; i++)
    {
        const std::string bit_stream = uint8_t_to_std_string(input_data_[i]);
        raw_dump.append(bit_stream);
    }

    if (bits % 8 != 0) {
        const uint64_t tailing_bits = bits % 8;
        const uint8_t tailing_byte = input_data_[bits / 8];
        std::string bit_stream = uint8_t_to_std_string(tailing_byte);
        bit_stream = bit_stream.substr(bit_stream.size() - tailing_bits, tailing_bits);
        raw_dump.append(bit_stream);
    }
}

void Huffman::decode_using_constructed_pairs()
{
    uint64_t offset = 0;
    uint64_t current_bit_size = 1;

    std::map < std::string, uint8_t > flipped_pairs;
    for (const auto & [byte, bitStream] : encoded_pairs) {
        flipped_pairs.emplace(bitStream, byte);
    }

    auto can_find_reference = [&](const uint64_t bit_offset, const uint64_t bit_size, uint8_t & decoded)->bool
    {
        const std::string current_reference = raw_dump.substr(bit_offset, bit_size);
        const auto reference = flipped_pairs.find(current_reference);
        if (reference == flipped_pairs.end()) {
            return false;
        }

        decoded = (reference->second);
        return true;
    };

    while (offset < raw_dump.size())
    {
        uint8_t decoded = 0;
        while (!can_find_reference(offset, current_bit_size, decoded)) {
            current_bit_size++;
        }

        output_data_.push_back(decoded);
        offset += current_bit_size;
        current_bit_size = 1;
    }
}

void Huffman::compress()
{
    uint64_t bits = 0;
    count_data_frequencies();
    build_binary_tree_based_on_the_frequency_map();
    walk_through_tree();
    encode_using_constructed_pairs();
    auto data = convert_std_string_to_std_vector_from_raw_dump(bits);
    const auto table = export_table();
    const auto table_size = static_cast<uint16_t>(table.size());

    output_data_.push_back(((uint8_t*)&table_size)[0]);
    output_data_.push_back(((uint8_t*)&table_size)[1]);
    output_data_.insert(end(output_data_), begin(table), end(table));

    // I need only 18 bits, since Max Block Size = 32 KB - 1, and 3 * 8 == 24, so 3 bytes is sufficient
    output_data_.push_back(((uint8_t*)&bits)[0]);
    output_data_.push_back(((uint8_t*)&bits)[1]);
    output_data_.push_back(((uint8_t*)&bits)[2]);
    output_data_.insert(end(output_data_), begin(data), end(data));
}

void Huffman::decompress()
{
    uint64_t read_offset = 0;
    uint16_t table_size = 0;
    std::memcpy(&table_size, input_data_.data(), sizeof(uint16_t));
    read_offset += sizeof(uint16_t);

    std::vector<uint8_t> table;
    table.resize(table_size);
    std::memcpy(table.data(), input_data_.data() + read_offset, table_size);
    read_offset += table_size;

    import_table(table);

    uint32_t bits = 0;
    std::memcpy(&bits, input_data_.data() + read_offset, 3);
    read_offset += 3;

    input_data_.erase(begin(input_data_), begin(input_data_) + static_cast<int64_t>(read_offset));
    convert_input_to_raw_dump(bits);
    decode_using_constructed_pairs();
}
