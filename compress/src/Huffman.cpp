#include "Huffman.h"
#include <vector>
#include <algorithm>
#include "log.hpp"
#include <sstream>

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
        single_mixed_node& data2)->void
    {
        uint64_t freq_combined = 0;
        if (data1.node == nullptr)
        {
            Node node;
            node.original_uint8_code = data1.code;
            node.frequency = data1.frequency;
            node.am_i_valid = true;

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
            node.am_i_valid = true;
            parent.right = std::make_unique<Node>(std::move(node));
            freq_combined += data2.frequency;
        } else {
            parent.right = std::move(data2.node);
            freq_combined += parent.right->frequency;
        }

        parent.original_uint8_code = 0;
        parent.frequency = freq_combined;
        parent.am_i_valid = true;
    };

    auto construct_dangling_nodes_based_on_full_map = [&](
        const uint64_t max_freq)->void
    {
        if (full_map.empty()) {
            return;
        }

        std::vector < single_mixed_node * > acceptable_map;
        acceptable_map.reserve(full_map.size());
        std::vector < std::unique_ptr < Node > > returned_nodes;
        auto it_full = full_map.end() - 1;
        bool found = false;
        for (;it_full > full_map.begin(); --it_full)
        {
            if (it_full->frequency <= max_freq) {
                acceptable_map.emplace_back(&*it_full);
            } else {
                found = true;
                break;
            }
        }

        if (it_full == full_map.begin() && !found) {
            acceptable_map.emplace_back(&*it_full);
        }

        auto it = acceptable_map.begin();
        while (true)
        {
            if ((it == acceptable_map.end()) || ((it + 1) == acceptable_map.end())) {
                break;
            }

            returned_nodes.emplace_back(std::make_unique<Node>());
            construct_two_branches_with_one_root_based_on_provided_two_data(
                *returned_nodes.back(), **it, **(it +1));
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
        construct_dangling_nodes_based_on_full_map(freq_threshold);
        freq_threshold = find_second_least_in_map();
    }
}

void Huffman::walk_through_tree()
{
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

std::vector<uint8_t> Huffman::export_table() const
{
    std::vector<uint8_t> ret;
    return ret;
}

void Huffman::import_table(const std::vector<uint8_t> &)
{
}

void Huffman::decode_using_constructed_pairs()
{
}
