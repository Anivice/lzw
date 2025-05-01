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
#include <chrono>
#include <sstream>

bool is_stdout_pipe();
void set_binary();

#define LZW_COMPRESSION_BIT_SIZE 9
extern uint16_t BLOCK_SIZE;
#define HASH_BITS (LZW_COMPRESSION_BIT_SIZE + (LZW_COMPRESSION_BIT_SIZE % 8))
#define BLOCK_SIZE_MAX (32767)

constexpr uint8_t used_lzw = 0xCA;
constexpr uint8_t used_huffman = 0xED;
constexpr uint8_t used_arithmetic = 0x77;
constexpr uint8_t used_arithmetic_lzw = used_lzw ^ used_arithmetic;
constexpr uint8_t used_plain = 0x00;
constexpr uint8_t used_repeator = 0x81;
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

bool speed_from_time(
    const decltype(std::chrono::system_clock::now())& before,
    std::stringstream & out,
    uint64_t processed_size,
    uint64_t original_size = 0,
    std::vector < uint64_t > * seconds_left_sample_space = nullptr);

bool is_utf8();
uint8_t calculate_8bit(const std::vector<uint8_t> & data);
bool pass_for_8bit(const std::vector<uint8_t> & data, uint8_t);

#endif //UTILS_H
