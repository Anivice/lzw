/* utils.cpp
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

#include <utils.h>
#ifdef WIN32
# include <windows.h>
# include <io.h>
# include <fcntl.h>
#elif __unix__
# include <unistd.h>
#endif // WIN32

#include <cstdio>
#include "log.hpp"
#include <sstream>

// bool is_stdout_pipe()
// {
// #ifdef WIN32
//     const DWORD fileType = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
//     return (fileType == FILE_TYPE_PIPE);
// #else
//     return !isatty(fileno(stdout));
// #endif
// }

void set_binary()
{
#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    setvbuf(stdin, nullptr, _IONBF, 0);
    setvbuf(stdout, nullptr, _IONBF, 0);
    DWORD originalMode { };

# ifdef __DEBUG__
    debug::log(debug::to_stderr, debug::debug_log, "Line buffering disabled\n");
# endif // __DEBUG__
    if (const auto hIn = GetStdHandle(STD_INPUT_HANDLE);
        !GetConsoleMode(hIn, &originalMode))
    {
        debug::log(debug::to_stderr, debug::debug_log, "GetConsoleMode failed\n");
    }
    else
    {
        DWORD mode = originalMode;
        mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        if (!SetConsoleMode(hIn, mode)) {
            debug::log(debug::to_stderr, debug::debug_log, "SetConsoleMode failed\n");
        }
    }
#else
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stdin, nullptr, _IONBF, 0);
#endif
}

uint16_t BLOCK_SIZE =  (16 * 1024);

std::string seconds_to_human_readable_dates(uint64_t seconds)
{
    constexpr uint64_t seconds_in_a_day = 24 * 60 * 60;
    constexpr uint64_t seconds_in_a_hour = 60 * 60;
    constexpr uint64_t seconds_in_a_minute = 60;
    const uint64_t days = seconds / seconds_in_a_day;
    seconds = seconds % seconds_in_a_day;
    const uint64_t hours = seconds / seconds_in_a_hour;
    seconds = seconds % seconds_in_a_hour;
    const uint64_t minutes = seconds / seconds_in_a_minute;
    seconds = seconds % seconds_in_a_minute;

    std::stringstream ss;
    ss  << (days == 0 ? "" : std::to_string(days) + "d")
        << (hours == 0 ? "" : std::to_string(hours) + "h")
        << (minutes == 0 ? "" : std::to_string(minutes) + "m")
        << (seconds == 0 ? "0s" : std::to_string(seconds) + "s");

    return ss.str();
}
