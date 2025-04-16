/* bitwise.cpp
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


#include "lzw.h"
#include "log.hpp"
#include <random>
#include <thread>
#include <vector>

int main()
{
    debug::log_level = debug::L_DEBUG_FG;

    auto random_num = []()->uint64_t
    {
        // Create a random_device object for true random number generation
        std::random_device rd;

        // Generate a random 60-bit number
        const uint64_t lower = rd();  // Get the lower 32 bits
        const uint64_t upper = rd();  // Get the upper 32 bits

        // Combine the two to form a 64-bit random number
        const uint64_t randomNumber = (upper << 32) | (lower & 0xFFFFFFFF);

        // Mask to keep only the lower 60 bits
        const uint64_t random60Bit = randomNumber & ((1ULL << 60) - 1);

        return random60Bit;
    };

    auto test_instance = [&]()
    {
        const auto cnum1 = random_num();
        const auto cnum2 = random_num();

        const auto num1 = bitwise_numeric<60>::make_bitwise_numeric(cnum1);
        const auto num2 = bitwise_numeric<60>::make_bitwise_numeric(cnum2);

        const bool eval_larger = num1 > num2;
        const bool eval_c_larger = cnum1 > cnum2;
        const bool eval_smaller = num1 < num2;
        const bool eval_c_smaller = cnum1 < cnum2;
        const bool eval_equal = num1 == num2;
        const bool eval_c_equal = cnum1 == cnum2;

        if ((eval_larger != eval_c_larger) || (eval_smaller != eval_c_smaller) || (eval_equal != eval_c_equal))
        {
            std::cerr << "Arithmetical compare failed!" << std::endl;
            exit(EXIT_FAILURE);
        }

        constexpr auto compliment = COMPUTE_N_BIT_COMPLIMENT(28, 64);
        const auto cnum3 = random_num() & compliment;
        const auto cnum4 = random_num() & compliment;

        const auto num3 = bitwise_numeric<28>::make_bitwise_numeric(cnum3);
        const auto num4 = bitwise_numeric<28>::make_bitwise_numeric(cnum4);

        const auto num5 = num3 + num4;
        const auto cnum5 = cnum3 + cnum4;
        const auto num6 = num5 - num4;
        const auto cnum6 = cnum5 - cnum4;

        if ((bitwise_numeric<28>::make_bitwise_numeric(cnum5) != num5)
            || (bitwise_numeric<28>::make_bitwise_numeric(cnum6) != num6))
        {
            std::cerr << "Arithmetical operations failed!" << std::endl;
            exit(EXIT_FAILURE);
        }

        auto logger = [&](const uint64_t expected, const uint64_t actual) {
            debug::log(debug::to_stderr, debug::error_log,
                "Bitwise operation failed!",
                " Expected: ", std::hex, expected,
                " Given: ", std::hex, actual, "\n");
            exit(EXIT_FAILURE);
        };


        const uint64_t compliment60 = COMPUTE_N_BIT_COMPLIMENT(60, 64);
        if (const auto actual = (num1 & num2).export_numeric<uint64_t>();
            actual != ((cnum1 & cnum2) & compliment60))
        {
            logger((cnum1 & cnum2) & compliment60, actual);
        }

        if (const auto actual = (num3 | num4).export_numeric<uint64_t>();
            actual != ((cnum3 | cnum4) & compliment))
        {
            logger((cnum3 | cnum4) & compliment, actual);
        }

        if (const auto actual = (num1 ^ num2).export_numeric<uint64_t>();
            actual != ((cnum1 ^ cnum2) & compliment60))
        {
            logger((cnum1 ^ cnum2) & compliment60, actual);
        }

        if (const auto actual = (~num4).export_numeric<uint64_t>();
            actual != ((~cnum4) & compliment))
        {
            logger(((~cnum4) & compliment), actual);
        }
    };

    constexpr uint64_t all_instances = 65536 / 20;
    float last_percentage = 0;
    for (uint64_t i = 0; i < all_instances; ++i)
    {
        std::vector < std::thread > pool;
        for (uint64_t j = 0; j < 20; j++)
        {
            pool.emplace_back([&]()->void {
                for (uint64_t k = 0; k < 4096; ++k) {
                    test_instance();
                }
            });
        }

        for (auto & thread : pool)
        {
            if (thread.joinable()) {
                thread.join();
            }
        }

        const auto percentage = (float)i / (float)all_instances * 100;
        if (percentage - last_percentage > 5 || last_percentage == 0) {
            debug::log(debug::to_stderr, debug::debug_log, percentage, "%\n");
            last_percentage = percentage;
        }
    }

    debug::log(debug::to_stderr, debug::debug_log, "All instances passed\n");
    return EXIT_SUCCESS;
}
