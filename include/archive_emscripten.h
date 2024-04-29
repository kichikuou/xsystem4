/* Copyright (C) 2024 kichikuou <KichikuouChrome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#ifndef SYSTEM4_ARCHIVE_EMSCRIPTEN_H
#define SYSTEM4_ARCHIVE_EMSCRIPTEN_H

#include "system4/archive.h"

#define ALD_FILEMAX 255

enum asset_type;

struct archive *ald_open_emscripten(enum asset_type type, int *error);
struct archive *dlf_open_emscripten(const char *path, int *error);
struct archive *aar_open_emscripten(const char *path, int *error);

#endif /* SYSTEM4_ARCHIVE_EMSCRIPTEN_H */
