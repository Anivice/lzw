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

cmake_minimum_required(VERSION 3.20)
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

add_compile_definitions(COMPRESS_UTIL_VERSION=\"0.2.8\")

include_directories(src/include)

add_library(external STATIC
        src/Huffman.cpp src/include/Huffman.h
        src/include/lzw.h src/include/lzw.inl
        src/include/utils.h src/utils.cpp
        src/argument_parser.cpp src/include/argument_parser.h
        src/include/numeric.h src/include/numeric.inl
        src/log.cpp src/include/log.hpp
        src/arithmetic.cpp src/include/arithmetic.h
        src/repeator.cpp src/include/repeator.h
        src/transformer.cpp src/include/transformer.h
)

add_executable(compress src/compress.cpp)
add_executable(decompress src/decompress.cpp)
add_executable(entropy src/entropy.cpp)

target_link_libraries(compress PRIVATE external)
target_link_libraries(decompress PRIVATE external)
target_link_libraries(entropy PRIVATE external)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")

    function(add_unit_test NAME)
        add_executable(${NAME}_test tests/${NAME}.cpp)
        target_link_libraries(${NAME}_test external)
        add_test(NAME "${NAME}_test" COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_test)
    endfunction()

    add_unit_test(bitwise)
    add_unit_test(Huffman)
    add_unit_test(human_readable)
    add_unit_test(arithmetic)
    add_unit_test(repeator)
    add_unit_test(transformer)
endif ()
