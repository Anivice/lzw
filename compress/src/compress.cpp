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
        bitwise_numeric_stack<128> stack, stack2;
        stack.emplace(0x2423);
        stack.emplace(0x2D21);
        stack.emplace(0x1212);
        stack.emplace(0x3E3);
        stack.emplace(0x9D42);
        stack.emplace(0x1F21);
        stack.emplace(0x3F21);
        stack.emplace(0x12F32);
        stack.emplace(0x1A2);
        stack.emplace(0x131);
        stack.emplace(0x13E32);
        stack.emplace(0x7811);
        auto exported = stack.dump();
        debug::log(exported, "\n");

        stack2.import(exported, stack.size());

		for (uint64_t i = 0; i < stack2.size(); i++)
		{
			std::cout << "    " << stack2[i].export_numeric() << "\n";
		}

        return EXIT_SUCCESS;
    } catch (const std::exception &e) {
        debug::log(debug::to_stderr, debug::error_log, e.what(), "\n");
        return EXIT_FAILURE;
    }
}
