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
#include <string.h>
#include <zlib.h>
#include <emscripten.h>

#include "system4.h"
#include "system4/little_endian.h"

#include "archive_emscripten.h"
#include "asset_manager.h"
#include "xsystem4.h"

EM_JS(bool, archive_exists_emscripten, (struct archive *ar, int no), {
	return Module.shell.assets.exists(ar, no);
});

static bool archive_exists_by_name_emscripten(struct archive *ar, const char *name, int *no_out)
{
	int no = EM_ASM_INT({ return Module.shell.assets.exists_by_name($0, UTF8ToString($1)); }, ar, display_sjis0(name));
	if (no < 0)
		return false;
	*no_out = no;
	return true;
}

EM_ASYNC_JS(void*, archive_load_emscripten, (struct archive *ar, int no, uint32_t *size), {
	const data = await Module.shell.assets.load(ar, no);
	if (!data)
		return 0;
	const ptr = Module._malloc(data.byteLength);
	Module.HEAPU8.set(data, ptr);
	Module.HEAPU32[size >> 2] = data.byteLength;
	return ptr;
});

static struct archive_data *archive_get_emscripten(struct archive *ar, int no)
{
	uint32_t size;
	void *buf = archive_load_emscripten(ar, no, &size);
	if (!buf)
		return NULL;

	struct archive_data *dfile = xcalloc(1, sizeof(struct archive_data));
	dfile->size = size;
	dfile->data = buf;
	dfile->name = NULL;
	dfile->no = no;
	dfile->archive = ar;
	return dfile;
}

EM_ASYNC_JS(void*, archive_load_by_name_emscripten, (struct archive *ar, const char *name, uint32_t *size, int *no_out), {
	const r = await Module.shell.assets.load_by_name(ar, UTF8ToString(name));
	if (!r)
		return 0;
	const ptr = Module._malloc(r.data.byteLength);
	Module.HEAPU8.set(r.data, ptr);
	Module.HEAPU32[size >> 2] = r.data.byteLength;
	Module.HEAP32[no_out >> 2] = r.no;
	return ptr;
});

static struct archive_data *archive_get_by_name_emscripten(struct archive *ar, const char *name)
{
	uint32_t size;
	int no;
	void *buf = archive_load_by_name_emscripten(ar, display_sjis0(name), &size, &no);
	if (!buf)
		return NULL;

	struct archive_data *dfile = xcalloc(1, sizeof(struct archive_data));
	dfile->size = size;
	dfile->data = buf;
	dfile->no = no;
	dfile->name = strdup(name);
	dfile->archive = ar;
	return dfile;
}

static bool archive_load_file_emscripten(struct archive_data *data)
{
	WARNING("%s: not implemented", __func__);
	return false;
}

static struct archive_data *archive_copy_descriptor_emscripten(struct archive_data *src)
{
	WARNING("%s: not implemented", __func__);
	return NULL;
}

static void archive_free_data_emscripten(struct archive_data *data)
{
	if (!data)
		return;
	free(data->data);
	free(data->name);
	free(data);
}

static void archive_free_emscripten(struct archive *ar)
{
	EM_ASM({ Module.shell.assets.close($0); }, ar);
	free(ar);
}

/**** ALD archives ****/

struct ald_emscripten {
	struct archive ar;
	char type;
};

struct ald_data_emscripten {
	struct archive_data data;
	void *buf;
};

EM_ASYNC_JS(void*, ald_load_emscripten, (char type, int no), {
	const data = await Module.shell.assets.ald_load(type, no);
	if (!data)
		return 0;
	const ptr = Module._malloc(data.byteLength);
	Module.HEAPU8.set(data, ptr);
	return ptr;
});

static bool ald_exists(struct archive *ar, int no)
{
	return EM_ASM_INT(
		{ return Module.shell.assets.ald_exists($0, $1); },
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

static struct archive_ops ald_ops = {
	.exists = ald_exists,
	.get = ald_get,
	.get_by_name = ald_get_by_name,
	.get_by_basename = NULL,
	.load_file = archive_load_file_emscripten,
	.release_file = NULL,
	.copy_descriptor = archive_copy_descriptor_emscripten,
	.free_data = ald_free_data,
	.free = ald_free,
};

struct archive *ald_open_emscripten(enum asset_type type, int *error)
{
	struct ald_emscripten *ar = xcalloc(1, sizeof(struct ald_emscripten));
	ar->type = type;
	ar->ar.ops = &ald_ops;
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

/**** Generic archives ****/

static struct archive_ops generic_ops = {
	.exists = archive_exists_emscripten,
	.exists_by_name = archive_exists_by_name_emscripten,
	.exists_by_basename = archive_exists_by_name_emscripten,
	.get = archive_get_emscripten,
	.get_by_name = archive_get_by_name_emscripten,
	.get_by_basename = archive_get_by_name_emscripten,
	.load_file = archive_load_file_emscripten,
	.release_file = NULL,
	.copy_descriptor = archive_copy_descriptor_emscripten,
	.free_data = archive_free_data_emscripten,
	.free = archive_free_emscripten,
};

struct archive *archive_open_emscripten(const char *path, int *error)
{
	struct archive *ar = xcalloc(1, sizeof(struct archive));
	if (!EM_ASM_INT({ return Module.shell.assets.open(UTF8ToString($0), $1); }, path, ar)) {
		free(ar);
		return NULL;
	}
	ar->ops = &generic_ops;
	return ar;
}

/**** AAR archives ****/

static bool aar_inflate_entry(struct archive_data *data, uint8_t *buf, uint32_t size)
{
	uint32_t version = LittleEndian_getDW(buf, 4);
	if (version != 0) {
		WARNING("unknown ZLB version: %u", version);
		return false;
	}
	unsigned long out_size = LittleEndian_getDW(buf, 8);
	uint32_t in_size = LittleEndian_getDW(buf, 12);
	if (in_size + 16 > size) {
		WARNING("Bad ZLB size");
		return false;
	}
	uint8_t *out = xmalloc(out_size);
	if (uncompress(out, &out_size, buf + 16, in_size) != Z_OK) {
		WARNING("uncompress failed");
		free(out);
		return false;
	}
	data->data = out;
	data->size = out_size;
	return true;
}

static struct archive_data *aar_get_by_name(struct archive *ar, const char *name)
{
	uint32_t size;
	int no;
	void *buf = archive_load_by_name_emscripten(ar, display_sjis0(name), &size, &no);
	if (!buf)
		return NULL;

	struct archive_data *dfile = xcalloc(1, sizeof(struct archive_data));
	if (!memcmp(buf, "ZLB\0", 4)) {
		if (!aar_inflate_entry(dfile, buf, size)) {
			free(dfile);
			free(buf);
			return NULL;
		}
		free(buf);
	} else {
		dfile->data = buf;
		dfile->size = size;
	}
	dfile->name = strdup(name);
	dfile->no = no;
	dfile->archive = ar;
	return dfile;
}

static struct archive_ops aar_ops = {
	.exists = archive_exists_emscripten,
	.exists_by_name = archive_exists_by_name_emscripten,
	.exists_by_basename = archive_exists_by_name_emscripten,
	.get = archive_get_emscripten,
	.get_by_name = aar_get_by_name,
	.get_by_basename = NULL,
	.load_file = archive_load_file_emscripten,
	.release_file = NULL,
	.copy_descriptor = archive_copy_descriptor_emscripten,
	.free_data = archive_free_data_emscripten,
	.free = archive_free_emscripten,
};

struct archive *aar_open_emscripten(const char *path, int *error)
{
	struct archive *ar = xcalloc(1, sizeof(struct archive));
	if (!EM_ASM_INT({ return Module.shell.assets.open(UTF8ToString($0), $1); }, path, ar)) {
		free(ar);
		return NULL;
	}
	ar->ops = &aar_ops;
	return ar;
}
