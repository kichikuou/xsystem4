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

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "system4.h"
#include "system4/buffer.h"
#include "system4/file.h"
#include "system4/string.h"

#include "hll.h"
#include "id_pool.h"
#include "vm/heap.h"
#include "vm/page.h"
#include "xsystem4.h"

enum vm_file_mode {
	VM_FILE_READ = 1,
	VM_FILE_WRITE = 2
};

struct vm_file {
	char *path;
	FILE *fp;
	struct buffer buf;
};

static struct id_pool pool;

static void read_page(struct vm_file *vf, struct page *page);
static void write_page(struct vm_file *vf, struct page *page);

static void read_value(struct vm_file *vf, union vm_value *v, enum ain_data_type type)
{
	switch (type) {
	case AIN_INT:
	case AIN_BOOL:
	case AIN_LONG_INT:
		v->i = buffer_read_int32(&vf->buf);
		break;
	case AIN_FLOAT:
		v->f = buffer_read_float(&vf->buf);
		break;
	case AIN_STRING:
		variable_fini(*v, type);
		v->i = heap_alloc_string(buffer_read_string(&vf->buf));
		break;
	case AIN_STRUCT:
	case AIN_ARRAY_TYPE:
		read_page(vf, heap_get_page(v->i));
		break;
	default:
		VM_ERROR("Unsupported value type %d", type);
	}
}

static void read_page(struct vm_file *vf, struct page *page)
{
	switch (page->type) {
	case STRUCT_PAGE:
		{
			struct ain_struct *s = &ain->structures[page->index];
			for (int i = 0; i < s->nr_members; i++) {
				read_value(vf, &page->values[i], s->members[i].type.data);
			}
		}
		break;
	case ARRAY_PAGE:
		{
			enum ain_data_type type = page->array.rank > 1 ? page->a_type : array_type(page->a_type);
			for (int i = 0; i < page->nr_vars; i++) {
				read_value(vf, &page->values[i], type);
			}
		}
		break;
	default:
		VM_ERROR("Unsupported page type %d", page->type);
	}
}

static void write_value(struct vm_file *vf, union vm_value v, enum ain_data_type type)
{
	switch (type) {
	case AIN_INT:
	case AIN_BOOL:
		buffer_write_int32(&vf->buf, v.i);
		break;
	case AIN_FLOAT:
		buffer_write_float(&vf->buf, v.f);
		break;
	case AIN_STRING:
		buffer_write_cstringz(&vf->buf, heap_get_string(v.i)->text);
		break;
	case AIN_STRUCT:
	case AIN_ARRAY_TYPE:
		write_page(vf, heap_get_page(v.i));
		break;
	default:
		VM_ERROR("Unsupported value type %d", type);
	}
}

static void write_page(struct vm_file *vf, struct page *page)
{
	switch (page->type) {
	case STRUCT_PAGE:
		{
			struct ain_struct *s = &ain->structures[page->index];
			for (int i = 0; i < s->nr_members; i++) {
				write_value(vf, page->values[i], s->members[i].type.data);
			}
		}
		break;
	case ARRAY_PAGE:
		{
			enum ain_data_type type = page->array.rank > 1 ? page->a_type : array_type(page->a_type);
			for (int i = 0; i < page->nr_vars; i++) {
				write_value(vf, page->values[i], type);
			}
		}
		break;
	default:
		VM_ERROR("Unsupported page type %d", page->type);
	}
}

static void vmFile_ModuleInit(void)
{
	id_pool_init(&pool, ID_POOL_HANDLE_BASE);
}

static int vmFile_EnumHandle(void)
{
	return id_pool_count(&pool);
}

static int vmFile_Open(struct string *string, int type)
{
	struct vm_file *vf = xcalloc(1, sizeof(struct vm_file));
	char *path = savedir_path(string->text);
	vf->path = path;

	if (type == VM_FILE_READ) {
		size_t len;
		uint8_t *data = file_read(path, &len);
		if (!data) {
			free(path);
			free(vf);
			return 0;
		}
		buffer_init(&vf->buf, data, len);
	} else if (type == VM_FILE_WRITE) {
		vf->fp = file_open_utf8(path, "wb");
		if (!vf->fp) {
			WARNING("Failed to open file '%s': %s", display_utf0(path), strerror(errno));
			free(path);
			free(vf);
			return 0;
		}
	} else {
		WARNING("Unknown mode in vmFile.Open: %d", type);
		return 0;
	}

	int handle = id_pool_get_unused(&pool);
	id_pool_set(&pool, handle, vf);
	return handle;
}

static void vmFile_Close(int handle)
{
	struct vm_file *vf = id_pool_release(&pool, handle);
	if (!vf)
		return;

	if (vf->fp) {  // write mode
		if (fwrite(vf->buf.buf, vf->buf.index, 1, vf->fp) != 1) {
			WARNING("%s: fwrite failed: %s", display_utf0(vf->path), strerror(errno));
		}
		fclose(vf->fp);
	}
	free(vf->buf.buf);
	free(vf->path);
	free(vf);
}

static void vmFile_ModuleFini(void)
{
	for (int i = id_pool_get_first(&pool); i >= 0; i = id_pool_get_next(&pool, i))
		vmFile_Close(i);
	id_pool_delete(&pool);
}

static int vmFile_Encode(int handle, int seed, int rand)
{
	struct vm_file *vf = id_pool_get(&pool, handle);
	if (!vf)
		return 0;

	size_t len = vf->buf.index;
	int32_t *buf = xmalloc(len + 8);
	memcpy(buf + 2, vf->buf.buf, len);
	free(vf->buf.buf);
	vf->buf.buf = (uint8_t*)buf;
	vf->buf.index += 8;

	int32_t mask = rand ^ ((rand & 1) ? 0xdf3d1b79 : 0x3ddf791b);
	int32_t checksum = 0;
	buf[0] = mask;

	len >>= 2;
	for (int i = 0; i < len; i++) {
		buf[i + 2] ^= mask;
		mask = buf[i + 2] ^ seed;
		checksum ^= mask;
		if (i & 2)
			mask ^= i * -3 ^ 0xf7fef7fe;
		if (i & 4)
			mask = mask << ((3 - i) & 0xf) | mask >> ((i - 3) & 0xf);
	}
	buf[1] = checksum;
	return 1;
}

static int vmFile_Decode(int handle, int seed)
{
	struct vm_file *vf = id_pool_get(&pool, handle);
	if (!vf)
		return 0;

	int32_t mask = buffer_read_int32(&vf->buf);
	int32_t checksum = buffer_read_int32(&vf->buf);

	int32_t *buf = (int32_t*)buffer_strdata(&vf->buf);
	int len = buffer_remaining(&vf->buf) >> 2;

	int32_t sum = 0;
	for (int i = 0; i < len; i++) {
		buf[i] ^= mask;
		mask ^= buf[i] ^ seed;
		sum ^= mask;
		if (i & 2)
			mask ^= i * -3 ^ 0xf7fef7fe;
		if (i & 4)
			mask = mask << ((3 - i) & 0xf) | mask >> ((i - 3) & 0xf);
	}
	if (sum != checksum) {
		WARNING("%s: checksum error", display_utf0(vf->path));
		return 0;
	}
	return 1;
}

//int vmFile_ReadArrayChar(int handle, struct page **pIVMArray);
//int vmFile_ReadArrayShort(int handle, struct page **pIVMArray);
//int vmFile_ReadArrayInt(int handle, struct page **pIVMArray);
//int vmFile_ReadArrayFloat(int handle, struct page **pIVMArray);
//int vmFile_ReadArrayString(int handle, struct page **pIVMArray);
//int vmFile_ReadArrayStruct(int handle, struct page **pIVMArray);

static int vmFile_ReadStruct(int handle, struct page **_page)
{
	struct vm_file *vf = id_pool_get(&pool, handle);
	if (!vf)
		return 0;

	struct page *page = *_page;
	if (page->type != STRUCT_PAGE) {
		VM_ERROR("vmFile.ReadStruct of non-struct");
	}

	read_page(vf, page);
	return 1;
}

//int vmFile_ReadGlobal(int handle, IMainSystem pIMainSystem);
//int vmFile_WriteArrayChar(int handle, struct page *pIVMArray);
//int vmFile_WriteArrayShort(int handle, struct page *pIVMArray);
//int vmFile_WriteArrayInt(int handle, struct page *pIVMArray);
//int vmFile_WriteArrayFloat(int handle, struct page *pIVMArray);
//int vmFile_WriteArrayString(int handle, struct page *pIVMArray);
//int vmFile_WriteArrayStruct(int handle, struct page *pIVMArray);

static int vmFile_WriteStruct(int handle, struct page *page)
{
	struct vm_file *vf = id_pool_get(&pool, handle);
	if (!vf)
		return 0;
	write_page(vf, page);
	return 1;
}

//int vmFile_WriteGlobal(int handle, IMainSystem pIMainSystem);
//int vmFile_SeekChar(int handle, int nPos, int nAbsolute);
//int vmFile_SeekShort(int handle, int nPos, int nAbsolute);
//int vmFile_SeekInt(int handle, int nPos, int nAbsolute);
//int vmFile_SeekFloat(int handle, int nPos, int nAbsolute);
//int vmFile_SeekString(int handle, int nPos, int nAbsolute);
//int vmFile_SizeChar(int handle);
//int vmFile_SizeShort(int handle);
//int vmFile_SizeInt(int handle);
//int vmFile_SizeFloat(int handle);
//int vmFile_SizeString(int handle);
//int vmFile_Copy(struct string *pISString, struct string *pIDString);

static int vmFile_Delete(struct string *string)
{
	char *path = savedir_path(string->text);
	int r = !remove_utf8(path);
	free(path);
	return r;
}

//int vmFile_GetTime(struct string *pIString, struct page **pIVMStruct);
//int vmFile_Find(struct string *pIWildString, struct string **pIString, int *pnCount);
//int vmFile_MakeDirectory(struct string *pIString);

HLL_LIBRARY(vmFile,
	    HLL_EXPORT(_ModuleInit, vmFile_ModuleInit),
	    HLL_EXPORT(_ModuleFini, vmFile_ModuleFini),
	    HLL_EXPORT(EnumHandle, vmFile_EnumHandle),
	    HLL_EXPORT(Open, vmFile_Open),
	    HLL_EXPORT(Close, vmFile_Close),
	    HLL_EXPORT(Encode, vmFile_Encode),
	    HLL_EXPORT(Decode, vmFile_Decode),
	    HLL_TODO_EXPORT(ReadArrayChar, vmFile_ReadArrayChar),
	    HLL_TODO_EXPORT(ReadArrayShort, vmFile_ReadArrayShort),
	    HLL_TODO_EXPORT(ReadArrayInt, vmFile_ReadArrayInt),
	    HLL_TODO_EXPORT(ReadArrayFloat, vmFile_ReadArrayFloat),
	    HLL_TODO_EXPORT(ReadArrayString, vmFile_ReadArrayString),
	    HLL_TODO_EXPORT(ReadArrayStruct, vmFile_ReadArrayStruct),
	    HLL_EXPORT(ReadStruct, vmFile_ReadStruct),
	    HLL_TODO_EXPORT(ReadGlobal, vmFile_ReadGlobal),
	    HLL_TODO_EXPORT(WriteArrayChar, vmFile_WriteArrayChar),
	    HLL_TODO_EXPORT(WriteArrayShort, vmFile_WriteArrayShort),
	    HLL_TODO_EXPORT(WriteArrayInt, vmFile_WriteArrayInt),
	    HLL_TODO_EXPORT(WriteArrayFloat, vmFile_WriteArrayFloat),
	    HLL_TODO_EXPORT(WriteArrayString, vmFile_WriteArrayString),
	    HLL_TODO_EXPORT(WriteArrayStruct, vmFile_WriteArrayStruct),
	    HLL_EXPORT(WriteStruct, vmFile_WriteStruct),
	    HLL_TODO_EXPORT(WriteGlobal, vmFile_WriteGlobal),
	    HLL_TODO_EXPORT(SeekChar, vmFile_SeekChar),
	    HLL_TODO_EXPORT(SeekShort, vmFile_SeekShort),
	    HLL_TODO_EXPORT(SeekInt, vmFile_SeekInt),
	    HLL_TODO_EXPORT(SeekFloat, vmFile_SeekFloat),
	    HLL_TODO_EXPORT(SeekString, vmFile_SeekString),
	    HLL_TODO_EXPORT(SizeChar, vmFile_SizeChar),
	    HLL_TODO_EXPORT(SizeShort, vmFile_SizeShort),
	    HLL_TODO_EXPORT(SizeInt, vmFile_SizeInt),
	    HLL_TODO_EXPORT(SizeFloat, vmFile_SizeFloat),
	    HLL_TODO_EXPORT(SizeString, vmFile_SizeString),
	    HLL_TODO_EXPORT(Copy, vmFile_Copy),
	    HLL_EXPORT(Delete, vmFile_Delete),
	    HLL_TODO_EXPORT(GetTime, vmFile_GetTime),
	    HLL_TODO_EXPORT(Find, vmFile_Find),
	    HLL_TODO_EXPORT(MakeDirectory, vmFile_MakeDirectory)
	    );
