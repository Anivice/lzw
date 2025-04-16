/* lzw.inl
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

#ifndef LZW_INL
#define LZW_INL

#include <iostream>
#include <algorithm>
#include <cstring>
#include "lzw.h"

template < unsigned LzwCompressionBitSize, unsigned DictionarySize >
    requires (LzwCompressionBitSize > 8)
lzw<LzwCompressionBitSize, DictionarySize>::lzw(
    std::vector < uint8_t >& input_stream,
    std::vector < uint8_t >& output_stream)
: input_stream_(input_stream),
  output_stream_(output_stream)
{
    // Initialize the dictionary
    for (int i = 0; i < 256; ++i) {
        dictionary_.emplace(std::string(1, static_cast<char>(i)),
            bitwise_numeric<LzwCompressionBitSize>::make_bitwise_numeric_loosely(i));
    }
}

template < unsigned LzwCompressionBitSize, unsigned DictionarySize >
    requires (LzwCompressionBitSize > 8)
void lzw<LzwCompressionBitSize, DictionarySize>::compress()
{
    if (discarding_this_instance)
    {
		throw std::invalid_argument("This instance has been discarded");
    }

    if (input_stream_.empty())
    {
	    return;
    }

    std::string current_string {};
    bitwise_numeric_stack < LzwCompressionBitSize > result_stack;
    bitwise_numeric<LzwCompressionBitSize> next_code = 
        bitwise_numeric < LzwCompressionBitSize>::make_bitwise_numeric_loosely(256);

    // Compression process
    std::ranges::reverse(input_stream_);
    while (!input_stream_.empty()) 
    {
        // Get input symbol while there are input symbols left
        const auto c = static_cast<char>(input_stream_.back());
        input_stream_.pop_back();
        if (const auto combined_string = current_string + c;
            dictionary_.contains(combined_string)) // combined_string is in the table
        {
            // update current string
            current_string = combined_string;
        }
        else
        {
			// Output the code for current string
			result_stack.push(dictionary_.at(current_string));
			// Add combined_string to the dictionary
			if (next_code < next_code.make_bitwise_numeric(DictionarySize)) {
				dictionary_.emplace(combined_string, next_code);
                ++next_code;
			}

			// Update current string to the new character
			current_string = std::string(1, c);
        }
    }

    // Output the last code if any
    if (!current_string.empty()) {
        result_stack.push(dictionary_[current_string]);
    }

	// Write the compressed data to the output stream
    for (const auto dumped_data = result_stack.dump();
        const auto& byte : dumped_data) 
    {
		output_stream_.push_back(byte);
	}

    // discarding this instance
    discarding_this_instance = true;
}

namespace std
{
    template < unsigned LzwCompressionBitSize >
    struct hash < bitwise_numeric<LzwCompressionBitSize> >
    {
        size_t operator()(const bitwise_numeric<LzwCompressionBitSize>& b) const {
            return static_cast<size_t>(b.template export_numeric<size_t>());
        }
    };
}

template < unsigned LzwCompressionBitSize, unsigned DictionarySize >
    requires (LzwCompressionBitSize > 8)
void lzw<LzwCompressionBitSize, DictionarySize>::decompress()
{
    if (discarding_this_instance)
    {
        throw std::invalid_argument("This instance has been discarded");
    }

    if (input_stream_.empty())
    {
        return;
    }

    std::unordered_map < bitwise_numeric < LzwCompressionBitSize >, std::string > dictionary_flipped;
    std::vector<uint8_t> source_dump;
    std::string current_string{};
    bitwise_numeric_stack < LzwCompressionBitSize > source_stack;
    bitwise_numeric<LzwCompressionBitSize> next_code =
        bitwise_numeric < LzwCompressionBitSize>::make_bitwise_numeric_loosely(255);

    // dump source
	std::ranges::reverse(input_stream_);
    while (!input_stream_.empty())
    {
        const auto code = static_cast<uint8_t>(input_stream_.back());
        input_stream_.pop_back();
		source_dump.emplace_back(code);
    }

	// import source to stack
	source_stack.lazy_import(source_dump);

    // Initialize the flipped dictionary
    for (int i = 0; i < 256; ++i) {
        dictionary_flipped.emplace(bitwise_numeric<LzwCompressionBitSize>::make_bitwise_numeric_loosely(i),
            std::string(1, static_cast<char>(i)));
    }

    // The first code is popped out and assigned to current_string
    current_string = static_cast<char>(source_stack[0].template export_numeric_force<uint8_t>());
	output_stream_.push_back(dictionary_.at(current_string).template export_numeric_force<uint8_t>());

    for (uint64_t i = 1; i < source_stack.size(); i++)
    {
        // Get input while there are codes are left to be received
        const auto code = source_stack[i];
        std::string entry;
        // Check if the Decoder has codes to decode
        if (code > next_code)
        {
            // entry = STRING + STRING[0]
            entry = current_string + current_string[0];
        }
        else
        {
            // entry = table[ord(arr[j])]
			entry = dictionary_flipped.at(code);
        }

        // append the new string
        output_stream_.insert(output_stream_.end(), entry.begin(), entry.end());

        // Check if the Table is full
		if (dictionary_.size() < DictionarySize)
		{
            ++next_code;
			dictionary_.emplace(current_string + entry[0], next_code);
		    dictionary_flipped.emplace(next_code, current_string + entry[0]);
		}

		// Update current string
		current_string = entry;
    }

    discarding_this_instance = true;
}

#endif // LZW_INL
