/* utils.h
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

#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <string>
#include <vector>

bool is_stdout_pipe();
void set_binary();

#define LZW_COMPRESSION_BIT_SIZE 9
extern uint64_t BLOCK_SIZE;
#define HASH_BITS (LZW_COMPRESSION_BIT_SIZE + (LZW_COMPRESSION_BIT_SIZE % 8))
#define BLOCK_SIZE_MAX (32767)

constexpr uint8_t used_lzw = 0xCA;
constexpr uint8_t used_huffman = 0xED;
constexpr uint8_t used_plain = 0x00;
constexpr unsigned char magic[] = { 0x1f, 0x9d, LZW_COMPRESSION_BIT_SIZE };

std::string seconds_to_human_readable_dates(uint64_t);

template < typename Type >
Type average(const std::vector<Type> & data)
{
    Type sum = 0;
    for (const auto &val : data) {
        sum = sum + val; // no += for compatibility
    }

    return sum / data.size();
}

#endif //UTILS_H
