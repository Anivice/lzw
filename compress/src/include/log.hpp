/* log.hpp
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
 *                     Change History
 * ---------------------------------------------------------------------------------------------------
 *      Modified Content    |                                   Brief
 * ---------------------------------------------------------------------------------------------------
 *        log_level_t       |  Changes its flags to avoid naming conflict in Windows debug mode
 *                          |  This change should not introduce compatibility issues
 */

#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <mutex>
#include <map>
#include <unordered_map>
#include <atomic>
#include <iomanip>

#define construct_simple_type_compare(type)                             \
    template <typename T>                                               \
    struct is_##type : std::false_type {};                              \
    template <>                                                         \
    struct is_##type<type> : std::true_type { };                        \
    template <typename T>                                               \
    constexpr bool is_##type##_v = is_##type<T>::value;

#ifdef __HARDLINK_LOG__
# ifndef __LOG_TO_STDOUT__
#  define LOG_DEV std::cerr
#  define LOG_DEV_FILE (stderr)
# else // __LOG_TO_STDOUT__
#  define LOG_DEV std::cout
#  define LOG_DEV_FILE (stdout)
# endif // __LOG_TO_STDOUT__
#else // __HARDLINK_LOG__
namespace debug {
    extern std::atomic < decltype(std::cout) * > LOG_DEV_ptr;
# define LOG_DEV (*LOG_DEV_ptr)
    extern std::atomic < FILE * > LOG_DEV_FILE;
    void init_as_stdout();
    void init_as_stderr();
}
#endif // __HARDLINK_LOG__

namespace debug {
    template <typename T, typename = void>
    struct is_container : std::false_type
    {
    };

    template <typename T>
    struct is_container<T,
        std::void_t<decltype(std::begin(std::declval<T>())),
        decltype(std::end(std::declval<T>()))>> : std::true_type
    {
    };

    template <typename T>
    constexpr bool is_container_v = is_container<T>::value;

    template <typename T>
    struct is_map : std::false_type
    {
    };

    template <typename Key, typename Value>
    struct is_map<std::map<Key, Value>> : std::true_type
    {
    };

    template <typename T>
    constexpr bool is_map_v = is_map<T>::value;

    template <typename T>
    struct is_unordered_map : std::false_type
    {
    };

    template <typename Key, typename Value, typename Hash, typename KeyEqual,
        typename Allocator>
    struct is_unordered_map<
        std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>>
        : std::true_type
    {
    };

    template <typename T>
    constexpr bool is_unordered_map_v = is_unordered_map<T>::value;

    template <typename T>
    struct is_string : std::false_type
    {
    };

    template <>
    struct is_string<std::basic_string<char>> : std::true_type
    {
    };

    template <>
    struct is_string<const char*> : std::true_type
    {
    };

    template <typename T>
    constexpr bool is_string_v = is_string<T>::value;

    construct_simple_type_compare(bool);

    inline class lower_case_bool_t {} lower_case_bool;
    construct_simple_type_compare(lower_case_bool_t);

    inline class upper_case_bool_t {} upper_case_bool;
    construct_simple_type_compare(upper_case_bool_t);

    inline class debug_log_t {} debug_log;
    construct_simple_type_compare(debug_log_t);

    inline class info_log_t {} info_log;
    construct_simple_type_compare(info_log_t);

    inline class warning_log_t {} warning_log;
    construct_simple_type_compare(warning_log_t);

    inline class error_log_t {} error_log;
    construct_simple_type_compare(error_log_t);

    inline class to_stdout_t {} to_stdout;
    construct_simple_type_compare(to_stdout_t);

    inline class to_stderr_t {} to_stderr;
    construct_simple_type_compare(to_stderr_t);

    template <typename T>
    struct is_pair : std::false_type
    {
    };

    template <typename Key, typename Value>
    struct is_pair<std::pair<Key, Value>> : std::true_type
    {
    };

    template <typename T>
    constexpr bool is_pair_v = is_pair<T>::value;

    extern std::mutex log_mutex;
    template <typename ParamType>
    void _log(const ParamType& param);
    template <typename ParamType, typename... Args>
    void _log(const ParamType& param, const Args&... args);

    /////////////////////////////////////////////////////////////////////////////////////////////

    template <typename... Args> void log(const Args&... args);

    template <typename Container>
	requires (debug::is_container_v<Container> &&
		!debug::is_map_v<Container> &&
        !debug::is_unordered_map_v<Container>)
	void print_container(const Container& container)
    {
        LOG_DEV << "[";
        for (auto it = std::begin(container); it != std::end(container); ++it)
        {
            if (sizeof(*it) == 1 /* 8bit data width */)
            {
                const auto & tmp = *it;
                LOG_DEV << "0x" << std::hex << std::setw(2) << std::setfill('0') << *(unsigned*)(& tmp); // ugly workarounds
            }
            else {
                _log(*it);
            }

            if (std::next(it) != std::end(container)) {
                LOG_DEV << ", ";
            }
        }
        LOG_DEV << "]";
    }

	template <typename Map>
	requires (debug::is_map_v<Map> || debug::is_unordered_map_v<Map>)
	void print_container(const Map & map)
    {
    	LOG_DEV << "{";
    	for (auto it = std::begin(map); it != std::end(map); ++it)
    	{
    		_log(it->first);
    		LOG_DEV << ": ";
    		_log(it->second);
    		if (std::next(it) != std::end(map)) {
    			LOG_DEV << ", ";
    		}
    	}
    	LOG_DEV << "}";
    }

    extern std::string str_true;
    extern std::string str_false;
    enum log_level_t { L_DEBUG_FG = 0, L_INFO_FG, L_WARNING_FG, L_ERROR_FG };
    extern std::atomic < log_level_t > log_level;
    extern std::atomic < log_level_t > current_level;

    template <typename ParamType> void _log(const ParamType& param)
    {
        auto suppression_level_check = [&]()->bool {
            return current_level >= log_level;
        };

        // NOLINTBEGIN(clang-diagnostic-repeated-branch-body)
        if constexpr (debug::is_string_v<ParamType>) {
            if (suppression_level_check()) {
                LOG_DEV << param;
            }
        }
        else if constexpr (debug::is_container_v<ParamType>) {
            if (suppression_level_check()) {
                debug::print_container(param);
            }
        }
        else if constexpr (debug::is_bool_v<ParamType>) {
            if (suppression_level_check()) {
                LOG_DEV << (param ? str_true : str_false);
            }
        }
        else if constexpr (debug::is_lower_case_bool_t_v<ParamType>) {
            str_true = "true";
            str_false = "false";
        }
        else if constexpr (debug::is_upper_case_bool_t_v<ParamType>) {
            str_true = "True";
            str_false = "False";
        }
        else if constexpr (debug::is_debug_log_t_v<ParamType>) {
            current_level = L_DEBUG_FG;

            if (suppression_level_check()) {
                LOG_DEV << "[DEBUG] ";
            }
        }
        else if constexpr (debug::is_info_log_t_v<ParamType>) {
            current_level = L_INFO_FG;

            if (suppression_level_check()) {
                LOG_DEV << "[INFO] ";
            }
        }
        else if constexpr (debug::is_warning_log_t_v<ParamType>) {
            current_level = L_WARNING_FG;

            if (suppression_level_check()) {
                LOG_DEV << "[WARNING] ";
            }
        }
        else if constexpr (debug::is_error_log_t_v<ParamType>) {
            current_level = L_ERROR_FG;

            if (suppression_level_check()) {
                LOG_DEV << "[ERROR] ";
            }
        }
        else if constexpr (debug::is_pair_v<ParamType>) {
            if (suppression_level_check()) {
                LOG_DEV << "<";
                _log(param.first);
                LOG_DEV << ": ";
                _log(param.second);
                LOG_DEV << ">";
            }
        } else if constexpr (debug::is_to_stderr_t_v<ParamType>) {
#ifndef __HARDLINK_LOG__
            debug::LOG_DEV_ptr = &std::cerr;
            debug::LOG_DEV_FILE = stderr;
#endif
        } else if constexpr (debug::is_to_stdout_t_v<ParamType>) {
#ifndef __HARDLINK_LOG__
            debug::LOG_DEV_ptr = &std::cout;
            debug::LOG_DEV_FILE = stdout;
#endif
        }
        else {
            if (suppression_level_check()) {
                LOG_DEV << param;
            }
        }
        // NOLINTEND(clang-diagnostic-repeated-branch-body)
    }

    template <typename ParamType, typename... Args>
    void _log(const ParamType& param, const Args &...args)
    {
        _log(param);
        (_log(args), ...);
    }

    template <typename... Args> void log(const Args &...args)
    {
        setvbuf(LOG_DEV_FILE, nullptr, _IONBF, 0);
        std::lock_guard<std::mutex> lock(log_mutex);
        debug::_log(args...);
        LOG_DEV << std::flush;
        fflush(LOG_DEV_FILE);
    }
}

#endif // LOG_HPP