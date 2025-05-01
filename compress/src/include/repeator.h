/* repeator.h
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

#ifndef REPEATOR_H
#define REPEATOR_H

#include <vector>
#include <cstdint>

namespace repeator {
    constexpr uint8_t none = 0;
    constexpr uint8_t trimmed = 0x4F;

class repeator {
private:
    std::vector < uint8_t > & input_;
    std::vector < uint8_t > & output_;

    static void encode(std::vector<uint8_t> & input, std::vector<uint8_t> & output);
public:
    repeator(std::vector<uint8_t> & input, std::vector<uint8_t> & output)
        : input_(input), output_(output) {}

    void encode();
    void decode();
};

}

#endif //REPEATOR_H
