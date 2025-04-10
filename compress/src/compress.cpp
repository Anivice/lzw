#include "log.hpp"
#include "argument_parser.h"
#include "LZW.h"

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
        bitwise_numeric_stack<6> stack;
        stack.emplace(0x24);
        stack.emplace(0x0D);
        stack.emplace(0x21);
        stack.emplace(0x0E);
        stack.emplace(0x0D);
        stack.emplace(0x12);
        stack.emplace(0x30);
        stack.emplace(0x1F);
        stack.emplace(0x1A);
        stack.emplace(0x13);
        stack.emplace(0x1E);  
        stack.emplace(0x08);
        auto exported = stack.dump();

        return EXIT_SUCCESS;
    } catch (const std::exception &e) {
        debug::log(debug::to_stderr, debug::error_log, e.what(), "\n");
        return EXIT_FAILURE;
    }
}
