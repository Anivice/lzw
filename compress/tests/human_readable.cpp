#include "utils.h"
#include "log.hpp"

int main()
{
    debug::set_log_level(debug::L_DEBUG_FG);

    for (uint64_t i = 1; i <= 100000; i++) {
        debug::log(debug::to_stderr, debug::debug_log, seconds_to_human_readable_dates(i), "\n");
    }
}