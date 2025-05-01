/* repeator.cpp
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

#include "repeator.h"
#include <utility>
#include <ranges>
#include <algorithm>

namespace repeator
{
    void repeator::encode(std::vector<uint8_t> & input, std::vector<uint8_t> & output)
    {
        if (input.empty()) {
            return;
        }

        const auto in = input[0];
        for (const auto & c: input)
        {
            if (in != c)
            {
                output.push_back(none);
                uint16_t len = input.size();
                output.push_back(reinterpret_cast<uint8_t *>(&len)[0]);
                output.push_back(reinterpret_cast<uint8_t *>(&len)[1]);
                output.insert(end(output), begin(input), end(input));
                return;
            }
        }

        output.push_back(trimmed);
        uint16_t len = input.size();
        output.push_back(reinterpret_cast<uint8_t *>(&len)[0]);
        output.push_back(reinterpret_cast<uint8_t *>(&len)[1]);
        output.push_back(in);
    }

    void repeator::encode() {
        encode(input_, output_);
    }

    void repeator::decode()
    {
        if (input_.size() < 3) {
            return;
        }

        std::ranges::reverse(input_);

        auto getc = [&]()->int
        {
            if (input_.empty()) {
                return -1;
            }

            const auto c = input_.back();
            input_.pop_back();
            return c;
        };

        while (!input_.empty())
        {
            const auto method = getc();
            uint16_t len = 0;
            reinterpret_cast<uint8_t *>(&len)[0] = getc();
            reinterpret_cast<uint8_t *>(&len)[1] = getc();

            if (method == none) {
                output_.reserve(len + output_.size());
                for (int i = 0; i < len; i++) {
                    output_.push_back(getc());
                }
            } else if (method == trimmed) {
                const uint8_t cc = getc();
                std::vector<uint8_t> tmp(len, cc);
                output_.insert(end(output_), begin(tmp), end(tmp));
            }
        }
    }
} // repeator
