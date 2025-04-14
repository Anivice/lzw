#include "log.hpp"
#include "argument_parser.h"
#include "lzw.h"
#include <fstream>

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

int main(const int argc, const char** argv)
{
#if defined(__DEBUG__)
    debug::log_level = debug::L_DEBUG_FG;
#else
    debug::log_level = debug::L_WARNING_FG;
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

        auto compress_file = [&](const std::string& in, const std::string& out)->void
            {
                std::ifstream input_file(in, std::ios::binary);
                std::ofstream output_file(out, std::ios::binary);

                if (!input_file.is_open()) {
                    throw std::runtime_error("Failed to open input file: " + in);
                }

                if (!output_file.is_open()) {
                    throw std::runtime_error("Failed to open output file: " + out);
                }

                while (input_file)
                {
                    std::vector<uint8_t> buffer(4096);
                    std::vector<uint8_t> output;
                    output.reserve(4096);

                    const auto data_len = static_cast<uint16_t>(output.size());
                    input_file.read((char*)(&data_len), sizeof(data_len));
                    input_file.read(reinterpret_cast<char*>(buffer.data()), data_len);
                    const auto bytes_read = input_file.gcount();
                    if (data_len != bytes_read) {
                        throw std::runtime_error("File corrupted!");
                    }
                    buffer.resize(bytes_read);
                    lzw <12> compressor(buffer, output);
                    compressor.decompress();
					output_file.write(reinterpret_cast<const char*>(output.data()),
					    static_cast<std::streamsize>(output.size()));
                }
            };

        if (static_cast<Arguments::args_t>(args).contains("help")) {
            print_help();
            return EXIT_SUCCESS;
        }
        else if (static_cast<Arguments::args_t>(args).contains("BARE"))
        {
			debug::log(debug::to_stderr, debug::error_log,
                "Unknown options:", static_cast<Arguments::args_t>(args).at("BARE"), "\n\n");
			print_help();
			return EXIT_FAILURE;
        }
		else if (static_cast<Arguments::args_t>(args).contains("version")) {
			std::cout << "LZW Utility version " << LZW_UTIL_VERSION << std::endl;
			return EXIT_SUCCESS;
		}
        else if (static_cast<Arguments::args_t>(args).contains("input"))
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
            return EXIT_SUCCESS;
        }
    	else {
			// TODO: Read from stdin
		}

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
