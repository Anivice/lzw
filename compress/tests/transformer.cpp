#include "transformer.h"
#include "log.hpp"

int main()
{
    debug::set_log_level(debug::L_DEBUG_FG);
    const std::string input = "BANANA";
    const auto result = transformer::forward(input);
    debug::log(debug::to_stdout, debug::debug_log, result.first, "\n",
        transformer::inverse(result.first, result.second), "\n");
}
