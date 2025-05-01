/* decompress.cpp
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

#include "log.hpp"
#include "argument_parser.h"
#include "lzw.h"
#include "utils.h"
#include <fstream>
#include <thread>
#include <chrono>
#include "Huffman.h"
#include <filesystem>
#include "arithmetic.h"
#include "repeator.h"
#include <functional>

namespace fs = std::filesystem;

Arguments::predefined_args_t arguments = {
    Arguments::single_arg_t {
        .name = "help",
        .short_name = 'h',
        .value_required = false,
        .explanation = "Show this help message"
    },
    Arguments::single_arg_t {
        .name = "output",
        .short_name = 'o',
        .value_required = true,
        .explanation = "Set output file"
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
        .explanation = "Multi-thread decompression"
    },
    Arguments::single_arg_t {
        .name = "verbose",
        .short_name = 'V',
        .value_required = false,
        .explanation = "Enable verbose mode"
    },
    Arguments::single_arg_t {
        .name = "decompress",
        .short_name = 'd',
        .value_required = false,
        .explanation = "This flag is deprecated and has no effect"
    },
};

std::atomic < unsigned > thread_count = 1;
std::atomic < bool > verbose = false;
std::atomic < uint64_t > processed_size = 0;

#define BUFFER_HEALTH_CHECK(input, in_buffer) {     \
    if (!(input).good()) {                          \
        (in_buffer).clear();                        \
        break;                                      \
    }                                               \
}

bool decompress(std::basic_istream<char>& input, std::basic_ostream<char>& output)
{
    if (!input.good()) {
        return false;
    }

    std::vector < std::pair < uint8_t /* method */, std::vector<uint8_t> > > in_buffers;
    std::vector < std::vector<uint8_t> > out_buffers;
    std::vector < std::thread > threads;
    in_buffers.resize(thread_count);
    out_buffers.resize(thread_count);

    // read in queue
    for (unsigned i = 0; i < thread_count; ++i)
    {
        auto & in_buffer = in_buffers[i];

        auto read_block = [&]()->bool
        {
            uint8_t method = 0;
            input.read(reinterpret_cast<char*>(&method), sizeof(method));
            if (!input.good()) {
                return false;
            }

            uint8_t checksum = 0;
            input.read(reinterpret_cast<char*>(&checksum), sizeof(checksum));
            if (!input.good()) {
                return false;
            }

            uint16_t block_size = 0;
            input.read(reinterpret_cast<char*>(&block_size), sizeof(block_size));
            if (!input.good()) {
                return false;
            }

            in_buffer.first = method;
            in_buffer.second.resize(block_size);
            input.read(reinterpret_cast<char*>(in_buffer.second.data()), block_size);
            if (const auto actual_size = input.gcount(); actual_size != block_size) {
                return false;
            }

            std::vector<uint8_t> data_pool;
            data_pool.reserve(block_size + 2);
            data_pool.push_back(reinterpret_cast<char *>(&block_size)[0]);
            data_pool.push_back(reinterpret_cast<char *>(&block_size)[1]);
            data_pool.insert(end(data_pool), begin(in_buffer.second), end(in_buffer.second));
            if (!pass_for_8bit(data_pool, checksum)) {
                throw std::runtime_error("File corrupted on block with method " + std::to_string(method));
            }

            return true;
        };

        if (!read_block()) {
            in_buffer.second.clear();
            break;
        }

        processed_size += in_buffer.second.size() + 3;
    }

    auto decompress_lzw_block = [](std::vector < uint8_t > * in_buffer,
        std::vector < uint8_t > * out_buffer)->void
    {
        lzw <LZW_COMPRESSION_BIT_SIZE> decompressor(*in_buffer, *out_buffer);
        decompressor.decompress();
    };

    auto decompress_huffman_lzw_block = [&](std::vector < uint8_t > * in_buffer,
    std::vector < uint8_t > * out_buffer)->void
    {
        std::vector < uint8_t > lzw_decompressed;
        decompress_lzw_block(in_buffer, &lzw_decompressed);
        Huffman HuffmanDecompressor(lzw_decompressed, *out_buffer);
        HuffmanDecompressor.decompress();
    };

    auto decompress_arithmetic_block = [](std::vector < uint8_t > * in_buffer,
    std::vector < uint8_t > * out_buffer)->void
    {
        arithmetic::Decode decompressor(*in_buffer, *out_buffer);
        decompressor.decode();
    };

    auto decompress_arithmetic_lzw_block = [&](std::vector < uint8_t > * in_buffer,
        std::vector < uint8_t > * out_buffer)->void
    {
        std::vector < uint8_t > lzw_decompressed;
        decompress_lzw_block(in_buffer, &lzw_decompressed);
        decompress_arithmetic_block(&lzw_decompressed, out_buffer);
    };

    auto raw_copy_over = [&](std::vector < uint8_t > * in_buffer,
    std::vector < uint8_t > * out_buffer)->void
    {
        out_buffer->reserve(out_buffer->size() + in_buffer->size());
        out_buffer->insert(end(*out_buffer), begin(*in_buffer), end(*in_buffer));
        in_buffer->clear();
    };

    auto decompress_repeator = [&](std::vector < uint8_t > * in_buffer,
    std::vector < uint8_t > * out_buffer)->void
    {
        out_buffer->reserve(out_buffer->size() + in_buffer->size());
        repeator::repeator decompressor(*in_buffer, *out_buffer);
        decompressor.decode();
        in_buffer->clear();
    };

    using decompress_function_type = std::function<void(std::vector<uint8_t>*, std::vector<uint8_t>*)>;
    std::map < uint8_t, decompress_function_type > decoder_map;
    decoder_map.emplace(used_plain, raw_copy_over);
    decoder_map.emplace(used_huffman, decompress_huffman_lzw_block);
    decoder_map.emplace(used_lzw, decompress_lzw_block);
    decoder_map.emplace(used_arithmetic, decompress_arithmetic_block);
    decoder_map.emplace(used_arithmetic_lzw, decompress_arithmetic_lzw_block);
    decoder_map.emplace(used_repeator, decompress_repeator);

    // create workers
    for (unsigned i = 0; i < thread_count; ++i)
    {
        if (!in_buffers[i].second.empty())
        {
            try {
                threads.emplace_back(decoder_map.at(in_buffers[i].first), &in_buffers[i].second, &out_buffers[i]);
            } catch (std::out_of_range&) {
                throw std::runtime_error("Unknown compression method, corrupted data?");
            }
            // if (in_buffers[i].first == used_lzw) {
            //     threads.emplace_back(decompress_lzw_block, &in_buffers[i].second, &out_buffers[i]);
            // } else if (in_buffers[i].first == used_huffman) {
            //     threads.emplace_back(decompress_huffman_lzw_block, &in_buffers[i].second, &out_buffers[i]);
            // } else if (in_buffers[i].first == used_arithmetic) {
            //     threads.emplace_back(decompress_arithmetic_block, &in_buffers[i].second, &out_buffers[i]);
            // } else if (in_buffers[i].first == used_plain) {
            //     threads.emplace_back(raw_copy_over, &in_buffers[i].second, &out_buffers[i]);
            // } else if (in_buffers[i].first == used_arithmetic_lzw) {
            //     threads.emplace_back(decompress_arithmetic_lzw_block, &in_buffers[i].second, &out_buffers[i]);
            // } else {
            //     throw std::runtime_error("Unknown compression method, corrupted data?");
            // }
        }
    }

    // waiting for them to finish
    for (auto & thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // write data in order
    for (unsigned i = 0; i < thread_count; ++i) {
        auto & out_buffer = out_buffers[i];
        if (!out_buffer.empty()) {
            output.write(reinterpret_cast<char*>(out_buffer.data()), static_cast<std::streamsize>(out_buffer.size()));
        }
    }

    return true;
}

void decompress_from_stdin()
{
    // Set stdin and stdout to binary mode
    set_binary();
    char magick_buff[3];
    std::cin.read(magick_buff, sizeof(magick_buff));
    if (std::memcmp(magick_buff, magic, sizeof(magick_buff)) != 0) {
        throw std::runtime_error("Decompression failed due to invalid magick number");
    }

    std::cin.read(reinterpret_cast<char *>(&BLOCK_SIZE), sizeof(BLOCK_SIZE));
    if (BLOCK_SIZE > BLOCK_SIZE_MAX) {
        throw std::runtime_error("Decompression failed due to invalid block size (decompression bomb?)");
    }

    if (verbose) {
        debug::log(debug::to_stderr, debug::info_log, "\n");
        processed_size += sizeof(magic) + sizeof(BLOCK_SIZE);
    }

    const auto before = std::chrono::system_clock::now();

    while (std::cin.good())
    {
        if (!decompress(std::cin, std::cout)) {
            break;
        }

        if (std::stringstream ss;
            verbose && speed_from_time(before, ss, processed_size))
        {
            debug::log(debug::to_stderr,
                debug::cursor_off,
                debug::clear_line,
                debug::info_log, ss.str(), "\n");
        }
    }

    if (verbose) {
        debug::log(debug::to_stderr, debug::cursor_on);
    }

    std::cout.flush();
}

void decompress_file(const std::string& in, const std::string& out)
{
    fs::path input_path(in);
    uintmax_t original_size;
    try {
        // Get the file size
        original_size = fs::file_size(input_path);
    } catch (const fs::filesystem_error&) {
        throw;
    }

    std::ifstream input_file(in, std::ios::binary);
    std::ofstream output_file(out, std::ios::binary);

    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open input file: " + in);
    }

    if (!output_file.is_open()) {
        throw std::runtime_error("Failed to open output file: " + out);
    }

    char magick_buff[3];
    input_file.read(magick_buff, sizeof(magick_buff));
    if (std::memcmp(magick_buff, magic, sizeof(magick_buff)) != 0) {
        throw std::runtime_error("Decompression failed due to invalid magick number");
    }

    input_file.read(reinterpret_cast<char *>(&BLOCK_SIZE), sizeof(BLOCK_SIZE));
    if (BLOCK_SIZE > BLOCK_SIZE_MAX) {
        throw std::runtime_error("Decompression failed due to invalid block size (decompression bomb?)");
    }

    if (verbose) {
        debug::log(debug::to_stderr, debug::info_log, "\n");
        processed_size += sizeof(magic) + sizeof(BLOCK_SIZE);
    }

    std::vector < uint64_t > seconds_left_sample_space;
    const auto before = std::chrono::system_clock::now();

    while (input_file)
    {
        if (!decompress(input_file, output_file)) {
            break;
        }

        if (std::stringstream ss;
            verbose && speed_from_time(before, ss, processed_size, original_size, &seconds_left_sample_space))
        {
            debug::log(debug::to_stderr,
                debug::cursor_off,
                debug::clear_line,
                debug::info_log, ss.str(), "\n");
        }
    }

    if (verbose) {
        debug::log(debug::to_stderr, debug::cursor_on);
    }

    input_file.close();
    output_file.close();
}

int main(const int argc, const char** argv)
{
#if defined(__DEBUG__)
    set_log_level(debug::L_DEBUG_FG);
#else
    set_log_level(debug::L_WARNING_FG);
#endif

    try {
        const Arguments args(argc, argv, arguments);

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

        if (static_cast<Arguments::args_t>(args).contains("BARE"))
        {
			debug::log(debug::to_stderr, debug::error_log,
                "Unknown options:", static_cast<Arguments::args_t>(args).at("BARE"), "\n\n");
			print_help();
			return EXIT_FAILURE;
        }

        if (static_cast<Arguments::args_t>(args).contains("version")) {
			std::cout << "Decompress Utility version " << COMPRESS_UTIL_VERSION << std::endl;
			return EXIT_SUCCESS;
		}

        if (static_cast<Arguments::args_t>(args).contains("threads"))
        {
            thread_count = std::strtoul(
            // disregarding all duplications, apply overriding from the last-provided option
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

        verbose = static_cast<Arguments::args_t>(args).contains("verbose");
        if (verbose) {
            debug::set_log_level(debug::L_INFO_FG);
            debug::log(debug::to_stderr, debug::info_log, "Verbose mode enabled\n");
        }

        if (static_cast<Arguments::args_t>(args).contains("input"))
        {
			const auto input_file = static_cast<Arguments::args_t>(args).at("input");
			if (input_file.size() != 1) {
				throw std::invalid_argument("Multiple input files provided");
			}

            if (!static_cast<Arguments::args_t>(args).contains("output"))
            {
				throw std::invalid_argument("Output file not provided");
            }

			const auto output_file = static_cast<Arguments::args_t>(args).at("output");
            if (output_file.size() != 1) {
                throw std::invalid_argument("Multiple output files provided");
            }

			decompress_file(input_file[0], output_file[0]);
            return EXIT_SUCCESS;
        }

        decompress_from_stdin();
        return EXIT_SUCCESS;
    } catch (const std::invalid_argument& e) {
        debug::log(debug::to_stderr, debug::error_log, e.what(), "\n\n",
			"Use `-h` or `--help` to see detailed help information.\n");
        return EXIT_FAILURE;
    } catch (const std::exception &e) {
        debug::log(debug::to_stderr, debug::error_log, e.what(), "\n");
        return EXIT_FAILURE;
    }
}
