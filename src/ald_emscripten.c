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

#include <stdlib.h>
#include <emscripten.h>

#include "system4.h"
#include "little_endian.h"

#include "ald_emscripten.h"
#include "asset_manager.h"

struct ald_emscripten {
	struct archive ar;
	char type;
};

struct ald_data_emscripten {
	struct archive_data data;
	void *buf;
};

EM_ASYNC_JS(void*, ald_load_emscripten, (char type, int no), {
	const data = await Module.shell.ald_load(type, no);
	if (!data)
		return 0;
	const ptr = Module._malloc(data.byteLength);
	Module.HEAPU8.set(data, ptr);
	return ptr;
});

static bool ald_exists(struct archive *ar, int no)
{
	return EM_ASM_INT(
		{ return Module.shell.ald_exists($0, $1); },
		((struct ald_emscripten*)ar)->type, no);
}

static struct archive_data *ald_get(struct archive *ar, int no)
{
	void *buf = ald_load_emscripten(((struct ald_emscripten*)ar)->type, no);
	if (!buf)
		return NULL;

	struct ald_data_emscripten *dfile = calloc(1, sizeof(struct ald_data_emscripten));
	uint32_t hdrsize = LittleEndian_getDW(buf, 0);
	dfile->data.size = LittleEndian_getDW(buf, 4);
	dfile->data.data = (uint8_t*)buf + hdrsize;
	dfile->data.name = (char*)buf + 16;
	dfile->data.no = no;
	dfile->data.archive = ar;
	dfile->buf = buf;

	return &dfile->data;
}

static struct archive_data *ald_get_by_name(struct archive *_ar, const char *name)
{
	WARNING("%s: not implemented", __func__);
	return NULL;
}

static bool ald_load_file(struct archive_data *data)
{
	WARNING("%s: not implemented", __func__);
	return false;
}

static struct archive_data *ald_copy_descriptor(struct archive_data *src)
{
	WARNING("%s: not implemented", __func__);
	return NULL;
}

static void ald_free_data(struct archive_data *data)
{
	if (!data)
		return;
	free(((struct ald_data_emscripten*)data)->buf);
	free(data);
}

static void ald_free(struct archive *ar)
{
	free(ar);
}

static struct archive_ops archive_ops = {
	.exists = ald_exists,
	.get = ald_get,
	.get_by_name = ald_get_by_name,
	.get_by_basename = NULL,
	.load_file = ald_load_file,
	.release_file = NULL,
	.copy_descriptor = ald_copy_descriptor,
	.free_data = ald_free_data,
	.free = ald_free,
};

struct archive *ald_open_emscripten(enum asset_type type, int *error)
{
	struct ald_emscripten *ar = xcalloc(1, sizeof(struct ald_emscripten));
	ar->type = type;
	ar->ar.ops = &archive_ops;
	switch (type) {
	case ASSET_BGM:   ar->type = 'B'; break;
	case ASSET_SOUND: ar->type = 'W'; break;
	case ASSET_CG:    ar->type = 'G'; break;
	case ASSET_DATA:  ar->type = 'D'; break;
	default:
		WARNING("Unhandled asset type: %d", type);
		free(ar);
		return NULL;
	}
	return &ar->ar;
}
