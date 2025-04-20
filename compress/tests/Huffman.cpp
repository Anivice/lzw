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
