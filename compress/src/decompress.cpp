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
        .explanation = "Get LZW utility version"
    },
    Arguments::single_arg_t {
        .name = "threads",
        .short_name = 'T',
        .value_required = true,
        .explanation = "Multi-thread compression"
    },
    Arguments::single_arg_t {
        .name = "verbose",
        .short_name = 'V',
        .value_required = false,
        .explanation = "Enable verbose mode"
    },
};

std::atomic < unsigned > thread_count = 1;
std::atomic < bool > verbose = false;

void decompress_on_one_block(std::vector<uint8_t> * in_buffer, std::vector<uint8_t> * out_buffer)
{
    lzw <LZW_COMPRESSION_BIT_SIZE> decompressor(*in_buffer, *out_buffer);
    decompressor.decompress();
}

std::atomic < uint64_t > processed_size = 0;

bool decompress(std::basic_istream<char>& input, std::basic_ostream<char>& output)
{
    if (!input.good()) {
        return false;
    }

    std::vector < std::vector<uint8_t> > in_buffers;
    std::vector < std::vector<uint8_t> > out_buffers;
    std::vector < std::thread > threads;
    in_buffers.resize(thread_count);
    out_buffers.resize(thread_count);

    // read in queue
    for (unsigned i = 0; i < thread_count; ++i)
    {
        auto & in_buffer = in_buffers[i];
        uint16_t block_size = 0;
        uint8_t method = 0;

        input.read(reinterpret_cast<char*>(&method), sizeof(method));
        if (!input.good()) {
            in_buffer.clear();
            break;
        }

        input.read(reinterpret_cast<char*>(&block_size), sizeof(block_size));
        if (!input.good()) {
            in_buffer.clear();
            break;
        }

        // compressed block
        if (method == used_lzw)
        {
            in_buffer.resize(BLOCK_SIZE);
            input.read(reinterpret_cast<char*>(in_buffer.data()), block_size);
            const auto actual_size = input.gcount();
            if (actual_size != block_size) {
                throw std::runtime_error("Decompression failed, corrupted data?");
            }

            in_buffer.resize(actual_size);
        }
        // negative compression ratio
        else if (method == 0)
        {
            in_buffer.clear();
            auto & out_buffer = out_buffers[i];
            out_buffer.resize(BLOCK_SIZE);
            input.read(reinterpret_cast<char*>(out_buffer.data()), BLOCK_SIZE);
            const auto actual_size = input.gcount();
            out_buffer.resize(actual_size);
        }
        else {
            throw std::runtime_error("Decompression failed, unknown compression method. Corrupted data?");
        }
    }

    // create workers
    for (unsigned i = 0; i < thread_count; ++i) {
        if (!in_buffers[i].empty()) {
            threads.emplace_back(decompress_on_one_block, &in_buffers[i], &out_buffers[i]);
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
            processed_size += out_buffer.size();
            output.write(reinterpret_cast<char*>(out_buffer.data()), static_cast<std::streamsize>(out_buffer.size()));
        }
    }

    return true;
}

void decompress_from_stdin()
{
    if (!is_stdout_pipe()) {
        throw std::runtime_error("Compression data cannot be written to console");
    }

    // Set stdin and stdout to binary mode
    set_binary();
    char magick_buff[3];
    std::cin.read(magick_buff, sizeof(magick_buff));
    if (std::memcmp(magick_buff, magic, sizeof(magick_buff)) != 0) {
        throw std::runtime_error("Decompression failed due to invalid magick number");
    }

    while (std::cin.good())
    {
        if (!decompress(std::cin, std::cout)) {
            break;
        }
    }
}

void decompress_file(const std::string& in, const std::string& out)
{
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

    const auto before = std::chrono::system_clock::now();

    while (input_file)
    {
        if (!decompress(input_file, output_file)) {
            break;
        }

        const auto after = std::chrono::system_clock::now();
        if (const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
            verbose && processed_size > 0 && duration > 0)
        {
            std::stringstream ss;
            if (const auto bps = processed_size * 8 / duration * 1000;
                bps > 1024 * 1024)
            {
                ss << processed_size * 8 << " bits processed, speed " << bps / 1024 / 1024 << " Mbps";
            } else if (bps > 10 * 1024) {
                ss << processed_size * 8 << " bits processed, speed " << bps / 1024 << " Kbps";
            } else {
                ss << processed_size * 8 << " bits processed, speed " << bps << " bps";
            }

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
			std::cout << "LZW Utility version " << LZW_UTIL_VERSION << std::endl;
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
                    "You are using ", thread_count, " threads, are you SURE?\n"
                    "Press Enter to confirm or Ctrl+C to abort > ");
                getchar();
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
