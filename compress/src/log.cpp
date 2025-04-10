/* log.cpp
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
#include <cstdlib>

std::string getEnvVar(const std::string &key)
{
#ifdef _MSC_VER
    // First, get the size of the environment variable.
    size_t requiredSize = 0;
    getenv_s(&requiredSize, nullptr, 0, key.c_str());
    if (requiredSize == 0) {
        return {};
    }

    // Allocate a buffer for the environment variable value.
    const auto buffer = new char[requiredSize];
    getenv_s(&requiredSize, buffer, requiredSize, key.c_str());
    std::string value(buffer);
    delete[] buffer;
    return value;
#else
    // Fallback for non-MSVC environments using std::getenv.
    const char* val = std::getenv(key.c_str());
    return (val == nullptr) ? std::string() : std::string(val);
#endif
}

std::mutex debug::log_mutex;
std::string debug::str_true = "True";
std::string debug::str_false = "False";
decltype(debug::log_level) debug::log_level = L_INFO_FG;
decltype(debug::current_level) debug::current_level = L_INFO_FG;

class init_log_level {
public:
    init_log_level()
    {
        if (const auto level = getEnvVar("LOG_LEVEL");
            level.empty() || level == "info")
        {
            debug::log_level = debug::L_INFO_FG;
        } else if (level == "debug") {
            debug::log_level = debug::L_DEBUG_FG;
        } else if (level == "warning") {
            debug::log_level = debug::L_WARNING_FG;
        } else if (level == "error") {
            debug::log_level = debug::L_ERROR_FG;
        } else {
            debug::log_level = debug::L_INFO_FG;
        }
    }
} init_log_level_instance;

#ifndef __HARDLINK_LOG__
std::atomic < decltype(std::cout) * > debug::LOG_DEV_ptr = &std::cout;
std::atomic < FILE * > debug::LOG_DEV_FILE = stdout;

void debug::init_as_stdout()
{
    std::lock_guard<std::mutex> lock(debug::log_mutex);
    debug::LOG_DEV_ptr = &std::cout;
    debug::LOG_DEV_FILE = stdout;
}

void debug::init_as_stderr()
{
    std::lock_guard<std::mutex> lock(debug::log_mutex);
    debug::LOG_DEV_ptr = &std::cerr;
    debug::LOG_DEV_FILE = stderr;
}

class init_log_device {
public:
    init_log_device()
    {
        if (const auto device = getEnvVar("LOG_DEV");
            device.empty() || device == "cout" || device == "stdout")
        {
            debug::init_as_stdout();
        } else {
            debug::init_as_stderr();
        }
    }
} init_log_device_instance;
#endif // __HARDLINK_LOG__