/* arithmetic.cpp
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

#include "arithmetic.h"
#include <vector>
#include <cstdint>

int main()
{
    // data with various frequencies
    const std::vector < uint8_t > data = {
        'A', 'B', 'C', 'D', 'E', 'F',
        'A',      'C', 'D',      'F',
                  'C', 'D', 'E', 'F', 'G',
        'A',      'C', 'D',      'F',
        'A',      'C',           'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    /*   4    1    5    4    2    5    2    1    1    1    1    1    1    */
    /*  We have 29 symbols, with 13 distinctive symbols */
    };

    std::vector < uint8_t > input = data, output, output2;

    arithmetic::Encode compressor(input, output);
    compressor.encode();

    std::vector<uint8_t> input2 = output;

    arithmetic::Decode decompressor(input2, output2);
    decompressor.decode();
}
