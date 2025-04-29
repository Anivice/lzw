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
#include "Huffman.h"
#include "arithmetic.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <ranges>
#include <cmath>

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
        .explanation = "Multi-thread compression"
    },
    Arguments::single_arg_t {
        .name = "verbose",
        .short_name = 'V',
        .value_required = false,
        .explanation = "Enable verbose mode"
    },
    Arguments::single_arg_t {
        .name = "no-huffman",
        .short_name = 'H',
        .value_required = false,
        .explanation = "Disable Huffman compression size comparison"
    },
    Arguments::single_arg_t {
        .name = "no-lzw",
        .short_name = 'L',
        .value_required = false,
        .explanation = "Disable LZW compression size comparison"
    },
    Arguments::single_arg_t {
        .name = "no-arithmetic",
        .short_name = 'R',
        .value_required = false,
        .explanation = "Disable Arithmetical compression size comparison"
    },
    Arguments::single_arg_t {
        .name = "archive",
        .short_name = 'A',
        .value_required = false,
        .explanation = "Disable compression"
    },
    Arguments::single_arg_t {
        .name = "block-size",
        .short_name = 'B',
        .value_required = true,
        .explanation = "Set block size (in bytes, default 16384 (16KB), 32767 Max (32KB - 1))"
    },
};

std::atomic < unsigned > thread_count = 1;
std::atomic < bool > verbose = false;
std::atomic < uint64_t > lzw_compressed_blocks = 0;
std::atomic < uint64_t > huffman_compressed_blocks = 0;
std::atomic < uint64_t > arithmetic_compressed_blocks = 0;
std::atomic < uint64_t > raw_blocks = 0;
std::atomic < bool > disable_lzw = false;
std::atomic < bool > disable_huffman = false;
std::atomic < bool > disable_arithmetic = false;
std::map <uint8_t, uint64_t> global_frequency_map;

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

void compress_on_one_block(const std::vector<uint8_t> * in_buffer, std::vector<uint8_t> * out_buffer)
{
    std::vector < std::pair < std::vector<uint8_t> , uint8_t > > size_map;
    std::mutex mutex_in, mutex_out;

    if (verbose) {
        for (const auto & symbol : *in_buffer) {
            global_frequency_map[symbol]++;
        }
    }

    auto LZW9Compress = [](std::vector<uint8_t> & input, std::vector<uint8_t> & output)->void
    {
        std::vector<uint8_t> compressed_data_lzw_tmp;
        lzw <LZW_COMPRESSION_BIT_SIZE> compressor(input, compressed_data_lzw_tmp);
        compressor.compress();

        const auto data_len_lzw_tmp = static_cast<uint16_t>(compressed_data_lzw_tmp.size());
        output.reserve(compressed_data_lzw_tmp.size() + 2 + output.size());
        output.push_back(((uint8_t*)&data_len_lzw_tmp)[0]);
        output.push_back(((uint8_t*)&data_len_lzw_tmp)[1]);
        output.insert(end(output), begin(compressed_data_lzw_tmp), end(compressed_data_lzw_tmp));
    };

    auto HuffmanCompress = [](std::vector<uint8_t> & input, std::vector<uint8_t> & output)->void
    {
        // try huffman
        Huffman huffmanCompressor(input, output);
        huffmanCompressor.compress();
    };

    auto ArithmeticCompress = [](const std::vector<uint8_t> & input, std::vector<uint8_t> & output)->void
    {
        std::vector<uint8_t> in = input, out;
        arithmetic::Encode compressor(in, out);
        compressor.encode();

        const auto data_len_arithmetic = static_cast<uint16_t>(out.size());
        output.push_back(((uint8_t*)&data_len_arithmetic)[0]);
        output.push_back(((uint8_t*)&data_len_arithmetic)[1]);
        output.insert(end(output), begin(out), end(out));
    };

    auto CopyOver = [](std::vector < uint8_t > & in_buffer, std::vector < uint8_t > & out_buffer)->void
    {
        const auto raw_block_size = static_cast<uint16_t>(in_buffer.size());
        out_buffer.push_back(((uint8_t*)&raw_block_size)[0]);
        out_buffer.push_back(((uint8_t*)&raw_block_size)[1]);
        out_buffer.insert(end(out_buffer), begin(in_buffer), end(in_buffer));
        in_buffer.clear();
    };

    auto compression_lzw_block = [&]()->void
    {
        std::vector<uint8_t> in, out;

        {
            std::lock_guard lock(mutex_in);
            in = *in_buffer;
        }

        LZW9Compress(in, out);

        {
            std::lock_guard lock(mutex_out);
            size_map.emplace_back(out, used_lzw);
        }
    };

    auto compression_huffman_block = [&]()->void
    {
        std::vector<uint8_t> in, out, out2;

        {
            std::lock_guard lock(mutex_in);
            in = *in_buffer;
        }

        HuffmanCompress(in, out);
        LZW9Compress(out, out2);

        {
            std::lock_guard lock(mutex_out);
            size_map.emplace_back(out2, used_huffman);
        }

    };

    auto compression_arithmetic_block = [&]()->void
    {
        std::vector<uint8_t> in, out;

        {
            std::lock_guard lock(mutex_in);
            in = *in_buffer;
        }

        ArithmeticCompress(in, out);

        {
            std::lock_guard lock(mutex_out);
            size_map.emplace_back(out, used_arithmetic);
        }
    };

    auto no_compression = [&]()->void
    {
        std::vector<uint8_t> in, out;

        {
            std::lock_guard lock(mutex_in);
            in = *in_buffer;
        }

        CopyOver(in, out);

        {
            std::lock_guard lock(mutex_out);
            size_map.emplace_back(out, used_plain);
        }
    };

    bool disable_compression = false;
    if (!(disable_lzw && disable_huffman && disable_arithmetic))
    {
        std::map <uint8_t, uint64_t> freq_map;
        if (const auto current_entropy = entropy_of(*in_buffer, freq_map);
            current_entropy > 7)
        {
            disable_compression = true;
        }
    } else {
        disable_compression = true;
    }

    std::vector < std::thread > thread_compression;

    if (!disable_compression && !disable_lzw) {
        thread_compression.emplace_back(compression_lzw_block);
    }

    if (!disable_compression && !disable_huffman) {
        thread_compression.emplace_back(compression_huffman_block);
    }

    if (!disable_compression && !disable_arithmetic) {
        thread_compression.emplace_back(compression_arithmetic_block);
    }

    if (disable_compression) {
        no_compression();
    }

    // external huffman calculation
   for (auto & thread : thread_compression) {
       if (thread.joinable()) {
           thread.join();
       }
   }

    std::erase_if(size_map, []
        (const std::pair < std::vector<uint8_t>&, uint8_t > & left)->bool
    {
        return left.first.empty();
    });

    const std::vector<uint8_t> * compression_buffer = nullptr;
    uint8_t compression_method = 0;

    for (auto & [buffer, flag] : size_map)
    {
        if ((compression_buffer == nullptr) || (compression_buffer->size() > buffer.size())) {
            compression_buffer = &buffer;
            compression_method = flag;
        }
    }

    out_buffer->reserve(BLOCK_SIZE);
    out_buffer->push_back(compression_method);
    out_buffer->insert(end(*out_buffer), begin(*compression_buffer), end(*compression_buffer));

    if (verbose) {
        if (compression_method == used_lzw) {
            ++lzw_compressed_blocks;
        } else if (compression_method == used_huffman) {
            ++huffman_compressed_blocks;
        } else if (compression_method == used_arithmetic) {
            ++arithmetic_compressed_blocks;
        } else if (compression_method == used_plain) {
            ++raw_blocks;
        }
    }
}

std::atomic < int64_t > processed_size = 0;
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

        processed_size += actual_size;
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
    std::cout.write((char*)(&BLOCK_SIZE), sizeof(BLOCK_SIZE));
    compressed_size += sizeof(magic) + sizeof(BLOCK_SIZE);
    while (std::cin.good()) {
        if (!compress(std::cin, std::cout)) {
            break;
        }
    }

    std::cout.flush();
}

void compress_file(const std::string& in, const std::string& out)
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

    std::vector < uint64_t > seconds_left_sample_space;
    const uint64_t sample_size = std::min(static_cast<unsigned long>(original_size / BLOCK_SIZE), 256ul);
    seconds_left_sample_space.reserve(sample_size);
    auto add_sample = [&](const uint64_t sample)->void
    {
        if (seconds_left_sample_space.size() < sample_size) {
            seconds_left_sample_space.push_back(sample);
        } else {
            seconds_left_sample_space.erase(seconds_left_sample_space.begin());
            seconds_left_sample_space.push_back(sample);
        }
    };

    const auto before = std::chrono::system_clock::now();

    output_file.write((char*)(magic), sizeof(magic));
    output_file.write((char*)(&BLOCK_SIZE), sizeof(BLOCK_SIZE));
    compressed_size += sizeof(magic) + sizeof(BLOCK_SIZE);
    while (input_file.good())
    {
        if (!compress(input_file, output_file)) {
            break;
        }

        const auto after = std::chrono::system_clock::now();
        if (const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
            verbose && processed_size > 0 && duration > 0)
        {
            std::stringstream ss;
            const auto bps = processed_size * 8 / duration * 1000;
            uint64_t seconds_left = (original_size - processed_size) / (bps / 8);
            add_sample(seconds_left);

            if (bps > 1024 * 1024)
            {
                ss << processed_size * 8 << " bits processed, speed " << bps / 1024 / 1024 << " Mbps ";
            } else if (bps > 10 * 1024) {
                ss << processed_size * 8 << " bits processed, speed " << bps / 1024 << " Kbps ";
            } else {
                ss << processed_size * 8 << " bits processed, speed " << bps << " bps ";
            }

            ss  << std::fixed << std::setprecision(2)
                << static_cast<long double>(processed_size) / static_cast<long double>(original_size) * 100
                << "% [ETA=" << seconds_to_human_readable_dates(average(seconds_left_sample_space)) << "]";

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
            if (verbose && processed_size > 0)
            {
                const auto entropy = entropy_of({}, global_frequency_map);
                const auto expectation = static_cast<long double>(processed_size) * entropy;
                auto expectation_int = static_cast<uint64_t>(expectation);
                if (expectation - static_cast<long double>(expectation_int) > 0) {
                    expectation_int++;
                }

                const auto numerical_bits_expectation = expectation_int / 8 * 8 + (expectation_int % 8 == 0 ? 0 : 8);
                const auto actual_used_bits = compressed_size * 8;
                const auto expectation_ratio = (numerical_bits_expectation != 0 ?
                    static_cast<long double>(actual_used_bits) / static_cast<long double>(numerical_bits_expectation) : NAN);
                const auto total_blocks = lzw_compressed_blocks + huffman_compressed_blocks + arithmetic_compressed_blocks + raw_blocks;
                const auto compressed_blocks = lzw_compressed_blocks + huffman_compressed_blocks + arithmetic_compressed_blocks;
                const auto compression_ratio = (static_cast<double>(processed_size) - static_cast<double>(compressed_size)) / static_cast<double>(processed_size);
                const auto CORatio = static_cast<double>(compressed_size) / static_cast<double>(processed_size);
                const auto CRRatio = (raw_blocks != 0 ? static_cast<double>(compressed_blocks) / static_cast<double>(raw_blocks) : NAN);
                const auto CAPercentage = (raw_blocks != 0 ? static_cast<double>(compressed_blocks) / static_cast<double>(total_blocks) * 100.0 : NAN);
                std::string performance;

                if (expectation_ratio > 1) {
                    performance = "Not Ideal";
                } else if (expectation_ratio <= 1 && expectation_ratio > 0.9) {
                    performance = "Normal";
                } else if (expectation_ratio <= 0.9 && expectation_ratio > 0.5) {
                    performance = "Healthy";
                } else if (expectation_ratio <= 0.5) {
                    performance = "Largely Exceeded Expectation";
                }

                if (performance.empty()) {
                    performance = "Possibly Mono-Contextual Data";
                }

                debug::log(debug::to_stderr, debug::info_log, "Original size:     ", processed_size * 8, " Bits\n");
                debug::log(debug::to_stderr, debug::info_log, "Compressed size:   ", compressed_size * 8, " Bits\n");
                debug::log(debug::to_stderr, debug::info_log, "Compression ratio: ", std::fixed, std::setprecision(4), compression_ratio * 100.0, "%\n");
                debug::log(debug::to_stderr, debug::info_log, "C/O ratio:         ", std::fixed, std::setprecision(4), CORatio * 100, "\n");
                debug::log(debug::to_stderr, debug::info_log, "Block Size:        ", BLOCK_SIZE , " Bytes (", BLOCK_SIZE / 1024, " KB)\n");
                debug::log(debug::to_stderr, debug::info_log, "Block count:       ", total_blocks, "\n");
                debug::log(debug::to_stderr, debug::info_log, "Compressed blocks: ", compressed_blocks, "\n");
                debug::log(debug::to_stderr, debug::info_log, " - LZW blocks:     ", lzw_compressed_blocks, "\n");
                debug::log(debug::to_stderr, debug::info_log, " - Huffman blocks: ", huffman_compressed_blocks, "\n");
                debug::log(debug::to_stderr, debug::info_log, " - Arithmetic blk: ", arithmetic_compressed_blocks, "\n");
                debug::log(debug::to_stderr, debug::info_log, "Raw blocks:        ", raw_blocks, "\n");
                debug::log(debug::to_stderr, debug::info_log, "C/R ratio:         ", std::fixed, std::setprecision(4), CRRatio * 100, "\n");
                debug::log(debug::to_stderr, debug::info_log, "C/A percentage:    ", std::fixed, std::setprecision(4), CAPercentage, "% \n");
                debug::log(debug::to_stderr, debug::info_log, "Raw data entropy:  ", std::fixed, std::setprecision(4), entropy, "\n");
                debug::log(debug::to_stderr, debug::info_log, "Expectation:       ", numerical_bits_expectation, " Bits\n");
                debug::log(debug::to_stderr, debug::info_log, "AB/EB percentage:  ", expectation_ratio * 100, "%\n");
                debug::log(debug::to_stderr, debug::info_log, "Performance:       ", performance, "\n");
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
			std::cout << "Compress Utility version " << COMPRESS_UTIL_VERSION << std::endl;
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

        verbose = static_cast<Arguments::args_t>(args).contains("verbose");
        if (verbose) {
            debug::set_log_level(debug::L_INFO_FG);
            debug::log(debug::to_stderr, debug::info_log, "Verbose mode enabled\n\n");
        }

        disable_lzw = static_cast<Arguments::args_t>(args).contains("no-lzw");
        disable_huffman = static_cast<Arguments::args_t>(args).contains("no-huffman");
        disable_arithmetic = static_cast<Arguments::args_t>(args).contains("no-arithmetic");

        if (static_cast<Arguments::args_t>(args).contains("archive")) {
            disable_arithmetic = disable_lzw = disable_huffman = true;
        }

        if (static_cast<Arguments::args_t>(args).contains("block-size"))
        {
            const auto block_size_literal =
                static_cast<Arguments::args_t>(args).at("block-size").back();
            BLOCK_SIZE = static_cast<uint16_t>(std::strtoul(block_size_literal.c_str(), nullptr, 10));
            if (BLOCK_SIZE > BLOCK_SIZE_MAX) {
                throw std::runtime_error("Block size too large, maximum " + std::to_string(BLOCK_SIZE_MAX) + " Bytes\n");
            }
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
