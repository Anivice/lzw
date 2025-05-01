/* entropy.cpp
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

#include <fstream>
#include <iostream>
#include <ranges>
#include <thread>
#include <cmath>
#include "log.hpp"
#include "argument_parser.h"

Arguments::predefined_args_t arguments = {
    Arguments::single_arg_t {
        .name = "help",
        .short_name = 'h',
        .value_required = false,
        .explanation = "Show this help message"
    },
    Arguments::single_arg_t {
        .name = "input",
        .short_name = 'i',
        .value_required = true,
        .explanation = "Set input file"
    },
    Arguments::single_arg_t {
        .name = "version",
        .short_name = 'v',
        .value_required = false,
        .explanation = "Get utility version"
    },
    Arguments::single_arg_t {
        .name = "threads",
        .short_name = 'T',
        .value_required = true,
        .explanation = "Multi-thread reading"
    },
};

long double entropy_of(const std::vector<uint8_t>& data, std::map <uint8_t, uint64_t> & frequency_map)
{
    for (const auto & code : data) {
        frequency_map[code]++;
    }

    uint64_t total_tokens = 0;
    for (const auto & freq: frequency_map | std::views::values) {
        total_tokens += freq;
    }

    std::vector < long double > EntropyList;
    EntropyList.reserve(256);
    for (const auto & freq: frequency_map | std::views::values) {
        const auto prob = static_cast<long double>(freq) / static_cast<long double>(total_tokens);
        EntropyList.push_back(prob * log2l(prob));
    }

    long double entropy = 0;
    for (const auto & entropy_sig: EntropyList) {
        entropy += entropy_sig;
    }
    entropy = -entropy;
    return entropy;
}

int main(const int argc, const char ** argv)
{
    uint64_t thread_count = 1;
    constexpr int BLOCK_SIZE = 1024 * 256; // 256 KB
    try {
        const Arguments args(argc, argv, arguments);

        auto build_frequency_map = [](const std::vector<uint8_t> * data, std::map <uint8_t, uint64_t> * frequency_map)
        {
            for (const auto & code : *data) {
                (*frequency_map)[code]++;
            }
        };

        auto print_help = [&]()->void
            {
                std::string path = *argv;
                const auto end = path.find_last_of('/');
                const auto end_w = path.find_last_of('\\');
				if (end != std::string::npos) {
					path = path.substr(end + 1);
				}
				else if (end_w != std::string::npos) {
					path = path.substr(end_w + 1);
				}

                if (const auto last_dot = path.find_last_of('.');
                    last_dot != std::string::npos)
                {
					path = path.substr(0, last_dot);
                }

                std::cout << path << " [OPTIONS]" << std::endl;
                std::cout << "OPTIONS: " << std::endl;
                args.print_help();
            };

        if (static_cast<Arguments::args_t>(args).contains("help")) {
            print_help();
            return EXIT_SUCCESS;
        }

        if (static_cast<Arguments::args_t>(args).contains("version")) {
			std::cout << "Entropy Utility version " << COMPRESS_UTIL_VERSION << std::endl;
			return EXIT_SUCCESS;
		}

        if (static_cast<Arguments::args_t>(args).contains("threads"))
        {
            thread_count = std::strtoul(
            // disregarding all duplications, apply overriding from the last provided option
                static_cast<Arguments::args_t>(args).at("threads").back().c_str(),
                nullptr, 10);
            if (thread_count > std::thread::hardware_concurrency()) {
                debug::log(debug::to_stderr, debug::warning_log,
                    "You are using ", thread_count, " threads, while your hardware concurrency is ",
                    std::thread::hardware_concurrency(), " threads.\n"
                    "You won't have a drastic beneficial effect with these many threads.\n"
                    "Are you SURE? Press Enter to confirm or Ctrl+C to abort > ");
                std::cin.get();
            }
        }

        if (static_cast<Arguments::args_t>(args).contains("input"))
        {
			const auto input_file = static_cast<Arguments::args_t>(args).at("input");

            for (const auto & file : input_file)
            {
                std::ifstream input_file_stream(file, std::ios::binary);
                if (!input_file_stream.is_open()) {
                    throw std::runtime_error("Failed to open input file " + file);
                }

                std::map <uint8_t, uint64_t> all_frequency_map;

                while (!input_file_stream.eof())
                {
                    std::vector < std::vector <uint8_t> > data;
                    std::vector < std::thread > threads;
                    std::vector < std::map <uint8_t, uint64_t> > frequency_map;
                    for (int i = 0; i < thread_count; i++)
                    {
                        std::vector <uint8_t> buffer;
                        buffer.resize(BLOCK_SIZE);
                        input_file_stream.read(reinterpret_cast<char *>(buffer.data()), BLOCK_SIZE);
                        const auto actual = input_file_stream.gcount();
                        if (actual == 0) {
                            break;
                        }
                        buffer.resize(actual);
                        data.emplace_back(std::move(buffer));
                    }

                    frequency_map.resize(data.size());
                    for (int i = 0; i < data.size(); i++) {
                        threads.emplace_back(build_frequency_map, &data[i], &frequency_map[i]);
                    }

                    for (auto & thread: threads) {
                        if (thread.joinable()) {
                            thread.join();
                        }
                    }

                    for (const auto & map: frequency_map)
                    {
                        for (const auto & [sym, freq]: map) {
                            all_frequency_map[sym] += freq;
                        }
                    }
                }

                std::cout << file << ": " << std::fixed << std::setprecision(4)
                          << entropy_of({}, all_frequency_map) << std::endl;
            }

            return EXIT_SUCCESS;
        }

         debug::log(debug::to_stderr, debug::error_log,
             "No meaningful option provided.\n"
             "Use `-h` or `--help` to see detailed help information.\n");
         return EXIT_FAILURE;
    } catch (const std::invalid_argument& e) {
        debug::log(debug::to_stderr, debug::error_log, e.what(), "\n\n",
			"Use `-h` or `--help` to see detailed help information.\n");
        return EXIT_FAILURE;
    } catch (const std::exception &e) {
        debug::log(debug::to_stderr, debug::error_log, e.what(), "\n");
        return EXIT_FAILURE;
    }
}
