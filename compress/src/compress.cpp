#include "log.hpp"
#include "argument_parser.h"
#include "lzw.h"

#ifdef WIN32
# include <windows.h>
#elif __unix__
# include <unistd.h>
#endif // WIN32

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
};

#include <sstream>

int main(const int argc, const char** argv)
{
#if defined(__DEBUG__)
    debug::log_level = debug::L_DEBUG_FG;
#else
    debug::log_level = debug::L_WARNING_FG;
#endif

    try {
        const Arguments args(argc, argv, arguments);
        std::string data = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC";
        using vec_t = std::vector<uint8_t>;
		vec_t input_stream(data.begin(), data.end());
		vec_t output_stream_compressed;
        vec_t output_decompressed;
        lzw <9> lzw_decompress(output_stream_compressed, output_decompressed);
        lzw <9> lzw_compress(input_stream, output_stream_compressed);

        // compress
        lzw_compress.compress();
		vec_t backup = output_stream_compressed;
        // decompress
        lzw_decompress.decompress();

        debug::log(debug::to_stderr, debug::info_log, "Original data: ", std::vector<uint8_t>(data.begin(), data.end()), "\n");
        debug::log(debug::to_stderr, debug::info_log, "Compressed data: ", backup, "\n");
        debug::log(debug::to_stderr, debug::info_log, "Decompressed data: ", std::dec, output_decompressed, "\n");
        debug::log(debug::to_stderr, debug::info_log, "Decompressed data length: ", std::dec, output_decompressed.size(), "\n");
        debug::log(debug::to_stderr, debug::info_log, "Original data length: ", std::dec, data.size(), "\n");
        debug::log(debug::to_stderr, debug::info_log, "Compressed data length: ", std::dec, backup.size(), "\n");
        debug::log(debug::to_stderr, debug::info_log, "Compression ratio: ", std::dec, (float)(data.size() - backup.size()) / (float)data.size() * 100, "%\n");
        return EXIT_SUCCESS;
    } catch (const std::exception &e) {
        debug::log(debug::to_stderr, debug::error_log, e.what(), "\n");
        return EXIT_FAILURE;
    }
}
