/* utils.h
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

#ifndef UTILS_H
#define UTILS_H

bool is_stdout_pipe();
void set_binary();

#define LZW_COMPRESSION_BIT_SIZE 9
#define BLOCK_SIZE (1024 * 16)

constexpr unsigned char magic[] = { 0x1f, 0x9d, LZW_COMPRESSION_BIT_SIZE };

#endif //UTILS_H
