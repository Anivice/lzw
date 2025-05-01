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
#include "repeator.h"
#include "transformer.h"
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
        .explanation = "Disable Huffman compression"
    },
    Arguments::single_arg_t {
        .name = "no-lzw",
        .short_name = 'L',
        .value_required = false,
        .explanation = "Disable LZW compression"
    },
    Arguments::single_arg_t {
        .name = "no-arithmetic",
        .short_name = 'R',
        .value_required = false,
        .explanation = "Disable Arithmetical compression"
    },
    Arguments::single_arg_t {
        .name = "no-lzw-overlay",
        .short_name = 'W',
        .value_required = false,
        .explanation = "Disable LZW compression overlay on Arithmetical compression result"
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
    Arguments::single_arg_t {
        .name = "entropy-threshold",
        .short_name = 'E',
        .value_required = true,
        .explanation = "Set entropy threshold within [0, 8]"
    },
};

std::atomic < unsigned > thread_count = 1;
std::atomic < bool > verbose = false;
std::atomic < uint64_t > lzw_compressed_blocks = 0;
std::atomic < uint64_t > huffman_compressed_blocks = 0;
std::atomic < uint64_t > arithmetic_compressed_blocks = 0;
std::atomic < uint64_t > arithmetic_lzw_compressed_blocks = 0;
std::atomic < uint64_t > raw_blocks = 0;
std::atomic < uint64_t > repeator_blocks = 0;
std::atomic < bool > disable_lzw = false;
std::atomic < bool > disable_huffman = false;
std::atomic < bool > disable_arithmetic = false;
std::atomic < bool > disable_arithmetic_lzw = false;
std::map <uint8_t, uint64_t> global_frequency_map;
std::map <uint8_t, uint64_t> lzw_frequency_map;
std::map <uint8_t, uint64_t> huffman_frequency_map;
std::map <uint8_t, uint64_t> arithmetic_frequency_map;
std::map <uint8_t, uint64_t> arithmetic_lzw_frequency_map;
std::map <uint8_t, uint64_t> repeator_frequency_map;
std::map <uint8_t, uint64_t> raw_frequency_map;
std::atomic < float > entropy_threshold = 7.5;

long double entropy_of(const std::vector<uint8_t>& data, std::map <uint8_t, uint64_t> & frequency_map)
{
    if (frequency_map.empty()) {
        return 0;
    }

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
    return std::abs(entropy); // remove -0.0000
}

inline long double entropy_of(const std::vector<uint8_t>& data)
{
    std::map <uint8_t, uint64_t> frequency_map;
    return entropy_of(data, frequency_map);
}

inline void record_freq(const std::vector<uint8_t>& data, std::map <uint8_t, uint64_t> & frequency_map)
{
    for (const auto & symbol : data) {
        frequency_map[symbol]++;
    }
}

void compress_on_one_block(const std::vector<uint8_t> * in_buffer, std::vector<uint8_t> * out_buffer)
{
    std::vector < std::pair < std::vector<uint8_t> , uint8_t > > size_map;
    std::mutex mutex_in, mutex_out;

    if (verbose) {
        record_freq(*in_buffer, global_frequency_map);
    }

    auto LZW9Compress = [](std::vector<uint8_t> & input, std::vector<uint8_t> & output)->void
    {
        std::vector<uint8_t> compressed_data_lzw_tmp;
        lzw <LZW_COMPRESSION_BIT_SIZE> compressor(input, compressed_data_lzw_tmp);
        compressor.compress();

        const auto data_len_lzw_tmp = static_cast<uint16_t>(compressed_data_lzw_tmp.size());
        output.reserve(compressed_data_lzw_tmp.size() + 2 + output.size());
        output.push_back(reinterpret_cast<const uint8_t*>(&data_len_lzw_tmp)[0]);
        output.push_back(reinterpret_cast<const uint8_t*>(&data_len_lzw_tmp)[1]);
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
        output.push_back(reinterpret_cast<const uint8_t *>(&data_len_arithmetic)[0]);
        output.push_back(reinterpret_cast<const uint8_t *>(&data_len_arithmetic)[1]);
        output.insert(end(output), begin(out), end(out));
    };

    auto CopyOver = [](std::vector < uint8_t > & in_buffer, std::vector < uint8_t > & out_buffer)->void
    {
        const auto raw_block_size = static_cast<uint16_t>(in_buffer.size());
        out_buffer.push_back(reinterpret_cast<const uint8_t *>(&raw_block_size)[0]);
        out_buffer.push_back(reinterpret_cast<const uint8_t *>(&raw_block_size)[1]);
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
        std::vector<uint8_t> in, out; {
            std::lock_guard lock(mutex_in);
            in = *in_buffer;
        }

        ArithmeticCompress(in, out);
        if (!disable_lzw && !disable_arithmetic_lzw // LZW or LZW overlay flag isn't set as disable
            && entropy_of(out) < entropy_threshold) // and the entropy of Arithmetic Compress is very bad
        {
            std::vector<uint8_t> lzw_overlay_out;
            std::vector<uint8_t> lzw_overlay_in;
            lzw_overlay_in.reserve(out.size() - 2); // pop 16bit size header
            lzw_overlay_in.insert(end(lzw_overlay_in), begin(out) + 2, end(out));
            LZW9Compress(lzw_overlay_in, lzw_overlay_out);

            {
                std::lock_guard lock(mutex_out);
                size_map.emplace_back(lzw_overlay_out, used_arithmetic_lzw);
            }
        }

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

    auto repeator = [&]()->void
    {
        std::vector<uint8_t> in, out;

        {
            std::lock_guard lock(mutex_in);
            in = *in_buffer;
        }

        repeator::repeator compressor(in, out);
        compressor.encode();
        const auto block_size = static_cast<uint16_t>(out.size());
        std::vector<uint8_t> final;
        final.push_back(reinterpret_cast<const uint8_t *>(&block_size)[0]);
        final.push_back(reinterpret_cast<const uint8_t *>(&block_size)[1]);
        final.insert(end(final), begin(out), end(out));

        {
            std::lock_guard lock(mutex_out);
            size_map.emplace_back(final, used_repeator);
        }
    };

    bool disable_compression = false;
    if (!(disable_lzw && disable_huffman && disable_arithmetic))
    {
        if (const auto current_entropy = entropy_of(*in_buffer);
            current_entropy > entropy_threshold)
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

    // if (disable_compression) {
    no_compression();
    repeator();
    // }

    // external huffman calculation
   for (auto & thread : thread_compression) {
       if (thread.joinable()) {
           thread.join();
       }
   }

    std::erase_if(size_map, []
        (const std::pair < const std::vector<uint8_t>&, uint8_t > & left)->bool
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

    if (!compression_buffer) {
        throw std::runtime_error("Unknown error occurred");
    }

    out_buffer->reserve(BLOCK_SIZE);
    out_buffer->push_back(compression_method);
    out_buffer->push_back(calculate_8bit(*compression_buffer));
    out_buffer->insert(end(*out_buffer), begin(*compression_buffer), end(*compression_buffer));

    if (verbose)
    {
        if (compression_method == used_lzw) {
            record_freq(*compression_buffer, lzw_frequency_map);
            ++lzw_compressed_blocks;
        } else if (compression_method == used_huffman) {
            record_freq(*compression_buffer, huffman_frequency_map);
            ++huffman_compressed_blocks;
        } else if (compression_method == used_arithmetic) {
            record_freq(*compression_buffer, arithmetic_frequency_map);
            ++arithmetic_compressed_blocks;
        } else if (compression_method == used_arithmetic_lzw) {
            record_freq(*compression_buffer, arithmetic_lzw_frequency_map);
            ++arithmetic_lzw_compressed_blocks;
            ++arithmetic_compressed_blocks;
        } else if (compression_method == used_plain) {
            record_freq(*compression_buffer, raw_frequency_map);
            ++raw_blocks;
        } else if (compression_method == used_repeator) {
            record_freq(*compression_buffer, repeator_frequency_map);
            ++repeator_blocks;
        }
    }
}

std::atomic < int64_t > processed_size = 0;
std::atomic < int64_t > compressed_size = 0;
std::vector < uint8_t > cache;
uint64_t offset = 0;

class EndOfFile final : std::exception {};

bool compress(std::basic_istream<char>& input, std::basic_ostream<char>& output)
{
    auto read = [&](char * buff, const uint64_t length)->uint64_t
    {
        if ((cache.size() - offset) < length)
        {
            uint64_t ret = 0;
            std::memcpy(buff, cache.data() + offset, cache.size() - offset);
            ret += cache.size() - offset;
            cache.resize(128 * 1024);
            input.read(reinterpret_cast<char*>(cache.data()), static_cast<std::streamsize>(cache.size()));
            const auto actual_size = input.gcount();
            cache.resize(actual_size);

            if (actual_size == 0) {
                cache.clear();
                return ret;
            }

            std::vector<uint8_t> result;
            uint64_t primary;
            if (actual_size < 128 * 1024) {
                const auto [result_, primary_] = transformer::forward_cpu(cache);
                result = result_;
                primary = primary_;
            } else {
                const auto [result_, primary_] = transformer::forward(cache);
                result = result_;
                primary = primary_;
            }

            cache.clear();
            cache.push_back(reinterpret_cast<const uint8_t*>(&primary)[0]);
            cache.push_back(reinterpret_cast<const uint8_t*>(&primary)[1]);
            cache.push_back(reinterpret_cast<const uint8_t*>(&primary)[2]);
            cache.insert(end(cache), begin(result), end(result));

            const auto rsize = std::min(cache.size(), length - ret);
            std::memcpy(buff + ret, cache.data(), rsize);
            ret += rsize;
            offset = rsize;
            return ret;
        }
        else
        {
            std::memcpy(buff, cache.data() + offset, length);
            offset += length;
            return length;
        }
    };

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
        const auto actual_size =
            read(reinterpret_cast<char*>(in_buffer.data()), static_cast<std::streamsize>(in_buffer.size()));
        if (actual_size == 0) {
            in_buffer.clear();
            break;
        }
        if (verbose) {
            processed_size += static_cast<long>(actual_size);
        }
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
        if (!out_buffer.empty())
        {
            if (verbose) {
                compressed_size += static_cast<int64_t>(out_buffer.size());
            }
            output.write(reinterpret_cast<char*>(out_buffer.data()), static_cast<std::streamsize>(out_buffer.size()));
        }
    }

    return true;
}

void compress_from_stdin()
{
    // Set stdin and stdout to binary mode
    set_binary();
    std::cout.write(reinterpret_cast<const char *>(magic), sizeof(magic));
    std::cout.write(reinterpret_cast<char *>(&BLOCK_SIZE), sizeof(BLOCK_SIZE));

    if (verbose) {
        debug::log(debug::to_stderr, debug::info_log, "\n");
        compressed_size += sizeof(magic) + sizeof(BLOCK_SIZE);
    }

    const auto before = std::chrono::system_clock::now();

    while (!std::cin.eof() || !cache.empty())
    {
        compress(std::cin, std::cout);
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
    const auto before = std::chrono::system_clock::now();

    output_file.write(reinterpret_cast<const char *>(magic), sizeof(magic));
    output_file.write(reinterpret_cast<char *>(&BLOCK_SIZE), sizeof(BLOCK_SIZE));

    if (verbose) {
        debug::log(debug::to_stderr, debug::info_log, "\n");
        compressed_size += sizeof(magic) + sizeof(BLOCK_SIZE);
    }

    while (!input_file.eof() || !cache.empty())
    {
        compress(input_file, output_file);
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

template < typename Type >
std::string literalize(const Type & type)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(4) << type;
    return ss.str();
}

template < typename Type >
uint64_t find_max(const std::vector<Type> & data)
{
    Type len = 0;
    for (const auto & element : data)
    {
        if (element > len) {
            len = element;
        }
    }

    return len;
}

using table_t = std::vector<std::pair<std::string, std::pair < std::string, std::string> >>;
void print_table(const std::string & title, table_t & content)
{
    const std::string split = is_utf8() ? "│" : "|";
    const std::string head_joint = is_utf8() ? "┬" : "-";
    const std::string head = is_utf8() ? "─" : "-";

    // 1. find max length of entry and keys
    std::vector<uint64_t> key_len, val_len, unit_len;
    for (const auto & [key, val] : content)
    {
        key_len.push_back(key.size());
        val_len.push_back(val.first.size());
        unit_len.push_back(val.second.size());
    }

    auto make_str = [](const uint64_t len, const std::string & key)->std::string
    {
        std::string result;
        for (uint64_t i = 0; i < len; i++) {
            result.append(key);
        }

        return result;
    };

    const auto max_key_len = find_max(key_len);
    const auto max_val_len = find_max(val_len);
    const auto max_unit_len = find_max(unit_len);
    const auto title_starting = (max_key_len + max_val_len + max_unit_len + 4 - title.size()) / 2;
    debug::log(debug::to_stderr, debug::info_log, std::string(title_starting, ' '), title, "\n");
    debug::log(debug::to_stderr, debug::info_log,
        make_str(max_key_len + 2, head), head_joint,
        make_str(max_val_len + 2 + max_unit_len, head), "\n");
    for (const auto & [key, val] : content)
    {
        debug::log(debug::to_stderr, debug::info_log,
            key, std::string(max_key_len - key.size(), ' '),
            "  ", split, " ",
            std::string(max_val_len - val.first.size(), ' '), val.first, " ", val.second, "\n");
    }
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
                std::map<uint8_t, uint64_t> compressed_data_freq;
                auto add_freq_map = [&](const std::map<uint8_t, uint64_t> & freq_map)->void
                {
                    for (const auto & [sym, freq] : freq_map) {
                        compressed_data_freq[sym] += freq;
                    }
                };

                add_freq_map(lzw_frequency_map);
                add_freq_map(huffman_frequency_map);
                add_freq_map(arithmetic_frequency_map);
                add_freq_map(arithmetic_lzw_frequency_map);
                add_freq_map(raw_frequency_map);
                add_freq_map(repeator_frequency_map);

                const auto compressed_entropy = entropy_of({}, compressed_data_freq);
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
                const auto total_blocks = lzw_compressed_blocks + huffman_compressed_blocks + arithmetic_compressed_blocks + raw_blocks + repeator_blocks;
                const auto compressed_blocks = lzw_compressed_blocks + huffman_compressed_blocks + arithmetic_compressed_blocks + repeator_blocks;
                const auto compression_ratio = (static_cast<double>(processed_size) - static_cast<double>(compressed_size)) / static_cast<double>(processed_size);
                const auto CORatio = static_cast<double>(compressed_size) / static_cast<double>(processed_size);
                const auto CRRatio = (raw_blocks != 0 ? static_cast<double>(compressed_blocks) / static_cast<double>(raw_blocks) : NAN);
                const auto CAPercentage = (raw_blocks != 0 ? static_cast<double>(compressed_blocks) / static_cast<double>(total_blocks) * 100.0 : NAN);

                const auto processed_size_literal = literalize(processed_size * 8);
                const auto compressed_size_literal = literalize(compressed_size * 8);
                const auto compression_ratio_literal = literalize(compression_ratio * 100.0);
                const auto CORatio_literal = literalize(CORatio * 100.0);
                const auto BLOCK_SIZE_literal = literalize(BLOCK_SIZE);
                const auto total_blocks_literal = literalize(total_blocks);
                const auto compressed_blocks_literal = literalize(compressed_blocks);
                const auto lzw_compressed_blocks_literal = literalize(lzw_compressed_blocks);
                const auto lzw_entropy_literal = literalize(entropy_of({}, lzw_frequency_map));
                const auto huffman_compressed_blocks_literal = literalize(huffman_compressed_blocks);
                const auto huffman_entropy_literal = literalize(entropy_of({}, huffman_frequency_map));
                const auto arithmetic_compressed_blocks_literal = literalize(arithmetic_compressed_blocks);
                const auto arithmetic_entropy_literal = literalize(entropy_of({}, arithmetic_frequency_map));
                const auto arithmetic_lzw_compressed_blocks_literal = literalize(arithmetic_lzw_compressed_blocks);
                const auto arithmetic_lzw_entropy_literal = literalize(entropy_of({}, arithmetic_lzw_frequency_map));
                const auto raw_blocks_literal = literalize(raw_blocks);
                const auto raw_entropy_literal = literalize(entropy_of({}, raw_frequency_map));
                const auto CRRatio_literal = literalize(CRRatio * 100);
                const auto CAPercentage_literal = literalize(CAPercentage);
                const auto entropy_literal = literalize(entropy);
                const auto compressed_entropy_literal = literalize(compressed_entropy);
                const auto numerical_bits_expectation_literal = literalize(numerical_bits_expectation);
                const auto expectation_ratio_literal = literalize(expectation_ratio * 100);

                table_t table;

                auto add_entry = [&](std::string key,
                    std::string value,
                    std::string val_unt)->void
                {
                    table.emplace_back(
                        std::make_pair<std::string, std::pair<std::string, std::string>>
                        (std::move(key), std::make_pair<std::string, std::string>(std::move(value), std::move(val_unt))));
                };

                auto split_add = [&](const std::string & key, const std::string & literal)
                {
                    const auto literal_int = literal.substr(0, literal.find_first_of('.'));
                    const auto literal_fraq = literal.substr(literal.find_first_of('.') + 1);
                    add_entry(key, literal_int, literal_fraq);
                };

                add_entry("Original Size", processed_size_literal, "Bits");
                add_entry("Compressed Size", compressed_size_literal, "Bits");
                add_entry("Compression Ratio", compression_ratio_literal, "%");
                add_entry("Compressed/Original", CORatio_literal, "%");
                add_entry("Block Size", BLOCK_SIZE_literal,
                    "Bytes" + ((BLOCK_SIZE > 1024) ? " (" + std::to_string(BLOCK_SIZE / 1024) + " KB)" : ""));
                add_entry("Total Blocks", total_blocks_literal, "");
                add_entry("Compressed Blocks", compressed_blocks_literal, "");
                add_entry(" - LZW Blocks", lzw_compressed_blocks_literal, "");
                split_add("   - LZW Compressed Entropy", lzw_entropy_literal);
                add_entry(" - Huffman Blocks", huffman_compressed_blocks_literal, "");
                split_add("   - Huffman Compressed Entropy", huffman_entropy_literal);
                add_entry(" - Arithmetic Blocks", arithmetic_compressed_blocks_literal, "");
                add_entry("   - Arithmetic LZW'd Blocks", arithmetic_lzw_compressed_blocks_literal, "");
                split_add("     - Arithmetic LZW'd Entropy", arithmetic_lzw_entropy_literal);
                add_entry("   - Arithmetic Bare Blocks", literalize(arithmetic_compressed_blocks - arithmetic_lzw_compressed_blocks), "");
                split_add("     - Arithmetic Bare Entropy", arithmetic_entropy_literal);
                add_entry(" - Repeator Blocks", literalize(repeator_blocks), "");
                split_add("   - Repeator Entropy", literalize(entropy_of({}, repeator_frequency_map)));
                add_entry("Raw Blocks", raw_blocks_literal, "");
                split_add(" - Raw Block Entropy", raw_entropy_literal);
                add_entry("Compressed/Raw", CRRatio_literal, "%");
                add_entry("Compressed/All", CAPercentage_literal, "%");
                add_entry("Original Data Entropy", entropy_literal, "");
                add_entry("Compressed Data Entropy", compressed_entropy_literal, "");
                add_entry("Expectation", numerical_bits_expectation_literal, "Bits");
                add_entry("Compressed/Expectation", expectation_ratio_literal, "%");
                print_table("SUMMARY", table);
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

        disable_lzw = static_cast<Arguments::args_t>(args).contains("no-lzw");
        disable_huffman = static_cast<Arguments::args_t>(args).contains("no-huffman");
        disable_arithmetic = static_cast<Arguments::args_t>(args).contains("no-arithmetic");
        disable_arithmetic_lzw = static_cast<Arguments::args_t>(args).contains("no-lzw-overlay");

        if (static_cast<Arguments::args_t>(args).contains("archive")) {
            disable_arithmetic = disable_lzw = disable_huffman = true;
        }

        if (static_cast<Arguments::args_t>(args).contains("block-size"))
        {
            const auto block_size_literal =
                static_cast<Arguments::args_t>(args).at("block-size").back();
            BLOCK_SIZE = static_cast<uint16_t>(std::strtoul(block_size_literal.c_str(), nullptr, 10));
            if (BLOCK_SIZE > BLOCK_SIZE_MAX) {
                throw std::runtime_error("Block size too large, maximum " + std::to_string(BLOCK_SIZE_MAX) + " Bytes");
            }
        }

        if (static_cast<Arguments::args_t>(args).contains("entropy-threshold"))
        {
            const auto entropy_threshold_literal =
                static_cast<Arguments::args_t>(args).at("entropy-threshold").back();
            entropy_threshold = std::strtof(entropy_threshold_literal.c_str(), nullptr);
            if (entropy_threshold < 0 or entropy_threshold > 8) {
                throw std::runtime_error("Invalid entropy threshold " + std::to_string(entropy_threshold)
                    + ": Threshold is within the interval [0, 8]");
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
