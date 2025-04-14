#  CMakeLists.txt
#
#  Copyright 2025 Anivice Ives
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
# 
#  SPDX-License-Identifier: GPL-3.0-or-later
#

cmake_minimum_required(VERSION 3.10)
project(lzw C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable position-independent code if needed
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set visibility properties
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_compile_definitions(__DEBUG__=\(1\))
    enable_testing()
endif ()

add_compile_definitions(LZW_UTIL_VERSION=\"0.0.1\")

include_directories(src/include)

add_library(log OBJECT src/log.cpp src/include/log.hpp)
add_library(arg_parser OBJECT src/argument_parser.cpp src/include/argument_parser.h)
add_library(lzw OBJECT src/lzw.cpp src/include/lzw.h src/include/utils.h src/utils.cpp)

add_executable(compress src/compress.cpp)
add_executable(decompress src/decompress.cpp)
target_link_libraries(compress PRIVATE log arg_parser lzw)
target_link_libraries(decompress PRIVATE log arg_parser lzw)

add_executable(bitwise tests/bitwise.cpp)
target_link_libraries(bitwise log arg_parser lzw)
add_test(NAME "General_Bitwise_operations" COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bitwise)
