/* compress.cpp
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
#include "huffman.h"
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
    Arguments::single_arg_t {
        .name = "huffman-only",
        .short_name = 'H',
        .value_required = false,
        .explanation = "Disable LZW compression size comparison"
    },
    Arguments::single_arg_t {
        .name = "lzw-only",
        .short_name = 'L',
        .value_required = false,
        .explanation = "Disable Huffman compression size comparison"
    },
    Arguments::single_arg_t {
        .name = "archive",
        .short_name = 'A',
        .value_required = false,
        .explanation = "Disable compression"
    },
};

std::atomic < unsigned > thread_count = 1;
std::atomic < bool > verbose = false;
std::atomic < uint64_t > lzw_compressed_blocks = 0;
std::atomic < uint64_t > huffman_compressed_blocks = 0;
std::atomic < uint64_t > raw_blocks = 0;
std::atomic < bool > disable_lzw = false;
std::atomic < bool > disable_huffman = false;

void compress_on_one_block(std::vector<uint8_t> * in_buffer, std::vector<uint8_t> * out_buffer)
{
    std::vector<uint8_t>
    buffer_lzw = *in_buffer, buffer_lzw_output,
    buffer_huffman = *in_buffer,
    buffer_huffman_output,
    buffer_huffman_lzw_output;

    auto LZW9Compress = [](std::vector<uint8_t> & input, std::vector<uint8_t> & output)->uint16_t
    {
        std::vector<uint8_t> compressed_data_lzw_tmp;
        lzw <LZW_COMPRESSION_BIT_SIZE> compressor(input, compressed_data_lzw_tmp);
        compressor.compress();
        const auto data_len_lzw_tmp = static_cast<uint16_t>(compressed_data_lzw_tmp.size());
        output.reserve(compressed_data_lzw_tmp.size() + 2 + output.size());
        output.push_back(((uint8_t*)&data_len_lzw_tmp)[0]);
        output.push_back(((uint8_t*)&data_len_lzw_tmp)[1]);
        output.insert_range(end(output), compressed_data_lzw_tmp);
        const auto data_len_lzw = static_cast<uint16_t>(compressed_data_lzw_tmp.size() + 2);
        return data_len_lzw;
    };

    auto HuffmanCompress = [](std::vector<uint8_t> & input, std::vector<uint8_t> & output)->uint16_t {
        // try huffman
        Huffman huffmanCompressor(input, output);
        huffmanCompressor.compress();
        const auto data_len_huffman = static_cast<uint16_t>(output.size());
        return data_len_huffman;
    };

    if (!disable_huffman)
    {
        // Huffman Encoding
        HuffmanCompress(buffer_huffman, buffer_huffman_output);
        // LZW for huffman in hope that it compress repeated data patterns
        LZW9Compress(buffer_huffman_output, buffer_huffman_lzw_output);
    }

    if (!disable_lzw)
    {
        // Plain LZW
        LZW9Compress(buffer_lzw, buffer_lzw_output);
    }

    uint64_t compressed_data_size
        = std::min(buffer_huffman_lzw_output.size(), buffer_lzw_output.size());

    uint8_t compression_method =
        buffer_huffman_lzw_output.size() > buffer_lzw_output.size() ? used_lzw : used_huffman;

    if (disable_lzw && !disable_huffman) {
        compressed_data_size = buffer_huffman_lzw_output.size();
        compression_method = used_huffman;
    } else if (!disable_lzw && disable_huffman) {
        compressed_data_size = buffer_lzw_output.size();
        compression_method = used_lzw;
    } else if (disable_huffman && disable_lzw) {
        compressed_data_size = UINT64_MAX;
        compression_method = 0;
    } // else? default

    if (compressed_data_size >= in_buffer->size()) // data expanded
    {
        out_buffer->push_back(used_plain);
        const auto raw_block_size = static_cast<uint16_t>(in_buffer->size());
        out_buffer->push_back(((uint8_t*)&raw_block_size)[0]);
        out_buffer->push_back(((uint8_t*)&raw_block_size)[1]);
        out_buffer->insert_range(end(*out_buffer), *in_buffer);
        in_buffer->clear();
        ++raw_blocks;
    }
    else // data compressed
    {
        out_buffer->push_back(compression_method);
        if (compression_method == used_lzw) {
            out_buffer->insert_range(end(*out_buffer), buffer_lzw_output);
            in_buffer->clear();
            ++lzw_compressed_blocks;
        } else if (compression_method == used_huffman) {
            out_buffer->insert_range(end(*out_buffer), buffer_huffman_lzw_output);
            in_buffer->clear();
            ++huffman_compressed_blocks;
        } else // WTF?
        {
            throw std::runtime_error("Invalid compression method, likely internal BUG");
        }
    }
}

std::atomic < int64_t > original_size = 0;
std::atomic < int64_t > compressed_size = 0;

bool compress(std::basic_istream<char>& input, std::basic_ostream<char>& output)
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
        in_buffer.resize(BLOCK_SIZE);
        input.read(reinterpret_cast<char*>(in_buffer.data()), static_cast<std::streamsize>(in_buffer.size()));
        const auto actual_size = input.gcount();
        if (actual_size == 0) {
            in_buffer.clear();
            break;
        }

        original_size += actual_size;
        in_buffer.resize(actual_size);
    }

    // create workers
    for (unsigned i = 0; i < thread_count; ++i)
    {
        if (!in_buffers[i].empty()) {
            threads.emplace_back(compress_on_one_block, &in_buffers[i], &out_buffers[i]);
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
            compressed_size += static_cast<int64_t>(out_buffer.size());
            output.write(reinterpret_cast<char*>(out_buffer.data()), static_cast<std::streamsize>(out_buffer.size()));
        }
    }

    if (verbose && original_size > 0) {
        debug::log(debug::to_stderr, debug::debug_log, "Original size:     ", original_size * 8, " Bits\n");
        debug::log(debug::to_stderr, debug::debug_log, "Compressed size:   ", compressed_size * 8, " Bits\n");
        debug::log(debug::to_stderr, debug::debug_log, "Compression ratio: ",
            (static_cast<double>(original_size) - static_cast<double>(compressed_size))
                / static_cast<double>(original_size) * 100.0, " %\n");
    }

    return true;
}

void compress_from_stdin()
{
    if (!is_stdout_pipe()) {
        throw std::runtime_error("Compression data cannot be written to console");
    }

    // Set stdin and stdout to binary mode
    set_binary();
    std::cout.write((char*)(magic), sizeof(magic));
    while (std::cin.good()) {
        if (!compress(std::cin, std::cout)) {
            break;
        }
    }

    std::cout.flush();
}

void compress_file(const std::string& in, const std::string& out)
{
    std::ifstream input_file(in, std::ios::binary);
    std::ofstream output_file(out, std::ios::binary);

    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open input file: " + in);
    }

    if (!output_file.is_open()) {
        throw std::runtime_error("Failed to open output file: " + out);
    }

    const auto before = std::chrono::system_clock::now();

    output_file.write((char*)(magic), sizeof(magic));
    while (input_file.good()) {
        if (!compress(input_file, output_file)) {
            break;
        }

        const auto after = std::chrono::system_clock::now();
        if (const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
            verbose && original_size > 0 && duration > 0)
        {
            std::stringstream ss;
            if (const auto bps = original_size * 8 / duration * 1000;
                bps > 1024 * 1024)
            {
                ss << original_size * 8 << " bits processed, speed " << bps / 1024 / 1024 << " Mbps";
            } else if (bps > 10 * 1024) {
                ss << original_size * 8 << " bits processed, speed " << bps / 1024 << " Kbps";
            } else {
                ss << original_size * 8 << " bits processed, speed " << bps << " bps";
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

        auto verbose_print = [&]()->void
        {
            if (verbose && original_size > 0) {
                debug::log(debug::to_stderr, debug::info_log, "Original size:     ", original_size * 8, " Bits\n");
                debug::log(debug::to_stderr, debug::info_log, "Compressed size:   ", compressed_size * 8, " Bits\n");
                debug::log(debug::to_stderr, debug::info_log, "Compression ratio: ",
                    (static_cast<double>(original_size) - static_cast<double>(compressed_size))
                        / static_cast<double>(original_size) * 100.0, " %\n");
                debug::log(debug::to_stderr, debug::info_log, "LZW blocks:        ",
                    lzw_compressed_blocks, "\n");
                debug::log(debug::to_stderr, debug::info_log, "Huffman blocks:    ",
                    huffman_compressed_blocks, "\n");
                debug::log(debug::to_stderr, debug::info_log, "Raw blocks:        ",
                    raw_blocks, "\n");
            }
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
			std::cout << "Compress Utility version " << LZW_UTIL_VERSION << std::endl;
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
            debug::log(debug::to_stderr, debug::info_log, "Verbose mode enabled\n\n");
        }

        disable_lzw = static_cast<Arguments::args_t>(args).contains("huffman-only");
        disable_huffman = static_cast<Arguments::args_t>(args).contains("lzw-only");

        if (static_cast<Arguments::args_t>(args).contains("archive")) {
            disable_lzw = disable_huffman = true;
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

			compress_file(input_file[0], output_file[0]);
            verbose_print();
            return EXIT_SUCCESS;
        }

        compress_from_stdin();
        verbose_print();
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
