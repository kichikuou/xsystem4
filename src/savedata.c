/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "cJSON.h"

#include "system4.h"
#include "system4/ain.h"
#include "system4/file.h"
#include "system4/string.h"

#include "savedata.h"
#include "vm.h"
#include "vm/heap.h"
#include "vm/page.h"
#include "xsystem4.h"

static int current_global;

#define invalid_save_data(msg, data) {					\
		char *str = data ? cJSON_Print(data) : strdup("NULL");	\
		if (!str) str = strdup("PRINTING FAILED");		\
		WARNING("Invalid save data (%d): " msg ": %s", current_global, data); \
		free(str);						\
	}

cJSON *page_to_json(struct page *page)
{
	if (!page)
		return cJSON_CreateNull();

	cJSON *values = cJSON_CreateArray();
	for (int i = 0; i < page->nr_vars; i++) {
		cJSON_AddItemToArray(values, vm_value_to_json(variable_type(page, i, NULL, NULL), page->values[i]));
	}
	return values;
}

// FIXME: non-page reference types occupy two vm_values
cJSON *vm_value_to_json(enum ain_data_type type, union vm_value val)
{
	switch (type) {
	case AIN_INT:
	case AIN_BOOL:
	case AIN_FUNC_TYPE:
	case AIN_DELEGATE:
	case AIN_LONG_INT:
		return cJSON_CreateNumber(val.i);
	case AIN_FLOAT:
		return cJSON_CreateNumber(val.f);
	case AIN_STRING:
		return cJSON_CreateString(heap[val.i].s->text);
	case AIN_STRUCT:
	case AIN_ARRAY_TYPE:
		return page_to_json(heap_get_page(val.i));
	case AIN_REF_TYPE:
		return cJSON_CreateNumber(-1);
	default:
		WARNING("Unhandled type: %d", ain_strtype(ain, type, -1));
		return cJSON_CreateNull();
	}
}

struct global_save_data {
	cJSON *save;
	cJSON *globals;
};

static void init_global_save_data(struct global_save_data *data, const char *keyname)
{
	data->save = cJSON_CreateObject();
	data->globals = cJSON_CreateArray();
	cJSON_AddStringToObject(data->save, "key", keyname);
	cJSON_AddItemToObject(data->save, "globals", data->globals);
}

static void add_global(struct global_save_data *data, int global, union vm_value val)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddNumberToObject(obj, "index", global);
	cJSON_AddItemToObject(obj, "value", vm_value_to_json(ain->globals[global].type.data, val));
	cJSON_AddItemToArray(data->globals, obj);
}

int save_json(const char *filename, cJSON *json)
{
	char *path = savedir_path(filename);
	FILE *f = file_open_utf8(path, "w");
	if (!f) {
		WARNING("Failed to open save file: %s: %s", display_utf0(filename), strerror(errno));
		free(path);
		return 0;
	}
	free(path);

	char *str = cJSON_Print(json);

	if (fwrite(str, strlen(str), 1, f) != 1) {
		WARNING("Failed to write save file: %s", strerror(errno));
		free(str);
		return 0;
	}
	if (fclose(f)) {
		WARNING("Error writing save to file: %s: %s", display_utf0(filename), strerror(errno));
		free(str);
		return 0;
	}
	free(str);
	return 1;

}

static int write_save_data(struct global_save_data *data, const char *filename)
{
	int r = save_json(filename, data->save);
	cJSON_Delete(data->save);
	return r;
}

int save_globals(const char *keyname, const char *filename)
{
	struct global_save_data data;
	init_global_save_data(&data, keyname);

	for (int i = 0; i < ain->nr_globals; i++) {
		add_global(&data, i, global_get(i));
	}

	return write_save_data(&data, filename);
}

static int get_group_index(const char *name)
{
	for (int i = 0; i < ain->nr_global_groups; i++) {
		if (!strcmp(ain->global_group_names[i], name))
			return i;
	}
	return -1;
}

int save_group(const char *keyname, const char *filename, const char *group_name, int *n)
{
	int group;

	if ((group = get_group_index(group_name)) < 0) {
		WARNING("Unregistered global group: %s", display_sjis0(group_name));
		return 0;
	}

	struct global_save_data data;
	init_global_save_data(&data, keyname);

	*n = 0;
	for (int i = 0; i < ain->nr_globals; i++) {
		if (ain->globals[i].group_index != group)
			continue;
		add_global(&data, i, global_get(i));
		(*n)++;
	}

	return write_save_data(&data, filename);
}

union vm_value json_to_vm_value(enum ain_data_type type, enum ain_data_type struct_type, int array_rank, cJSON *json);

void get_array_dims(cJSON *json, int rank, union vm_value *dims)
{
	cJSON *array = json;
	for (int i = 0; i < rank; i++) {
		dims[i].i = cJSON_GetArraySize(array);
		array = cJSON_GetArrayItem(array, 0);
	}
}

void json_load_page(struct page *page, cJSON *vars, bool call_dtors)
{
	int i = 0;
	cJSON *v;
	cJSON_ArrayForEach(v, vars) {
		int struct_type, array_rank;
		enum ain_data_type data_type = variable_type(page, i, &struct_type, &array_rank);
		union vm_value val = json_to_vm_value(data_type, struct_type, array_rank, v);
		if (call_dtors)
			variable_set(page, i, data_type, val);
		else
			page->values[i] = val;
		i++;
	}
}

union vm_value json_to_vm_value(enum ain_data_type type, enum ain_data_type struct_type, int array_rank, cJSON *json)
{
	char *str;
	int slot;
	union vm_value *dims;
	struct page *page;
	switch (type) {
	case AIN_INT:
	case AIN_BOOL:
	case AIN_LONG_INT:
		if (!cJSON_IsNumber(json)) {
			invalid_save_data("Not a number", json);
			return vm_int(0);
		}
		return vm_int(json->valueint);
	case AIN_FLOAT:
		if (!cJSON_IsNumber(json)) {
			invalid_save_data("Not a number", json);
			return vm_float(0);
		}
		return vm_float(json->valuedouble);
	case AIN_STRING:
		slot = heap_alloc_slot(VM_STRING);
		if (!cJSON_IsString(json)) {
			invalid_save_data("Not a string", json);
			heap[slot].s = string_ref(&EMPTY_STRING);
			return vm_int(slot);
		}
		str = cJSON_GetStringValue(json);
		if (!str[0]) {
			heap[slot].s = string_ref(&EMPTY_STRING);
		} else {
			heap[slot].s = make_string(str, strlen(str));
		}
		return vm_int(slot);
	case AIN_STRUCT:
		slot = alloc_struct(struct_type);
		if (!cJSON_IsArray(json) || cJSON_GetArraySize(json) != ain->structures[struct_type].nr_members) {
			invalid_save_data("Not an array", json);
		} else {
			json_load_page(heap[slot].page, json, false);
		}
		return vm_int(slot);
	case AIN_ARRAY_TYPE:
		slot = heap_alloc_slot(VM_PAGE);
		if (cJSON_IsNull(json)) {
			heap[slot].page = NULL;
			return vm_int(slot);
		}
		if (!cJSON_IsArray(json)) {
			invalid_save_data("Not an array", json);
			heap[slot].page = NULL;
			return vm_int(slot);
		}
		dims = xmalloc(sizeof(union vm_value) * array_rank);
		get_array_dims(json, array_rank, dims);
		page = alloc_array(array_rank, dims, type, struct_type, false);
		heap[slot].page = page;
		free(dims);
		json_load_page(heap[slot].page, json, false);
		return vm_int(slot);
	case AIN_REF_TYPE:
		return vm_int(-1);
	default:
		WARNING("Unhandled data type: %s", ain_strtype(ain, type, -1));
		return vm_int(-1);
	}
}

static cJSON *read_save_file(const char *filename)
{
	FILE *f;
	long len;
	char *buf;
	char *path = savedir_path(filename);

	if (!(f = file_open_utf8(path, "r"))) {
		WARNING("Failed to open save file: %s: %s", display_utf0(filename), strerror(errno));
		free(path);
		return NULL;
	}
	free(path);

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);

	buf = xmalloc(len+1);
	buf[len] = '\0';
	if (fread(buf, len, 1, f) != 1) {
		WARNING("Failed to read save file: %s", display_utf0(filename));
		free(buf);
		return 0;
	}
	fclose(f);

	cJSON *r = cJSON_Parse(buf);
	free(buf);
	return r;
}

int load_globals(const char *keyname, const char *filename, const char *group_name, int *n)
{
	int retval = 0;
	cJSON *save = read_save_file(filename);
	if (!save)
		return 0;
	if (!cJSON_IsObject(save)) {
		invalid_save_data("Not an object", save);
		goto cleanup;
	}

	cJSON *key = cJSON_GetObjectItem(save, "key");
	if (!key || strcmp(keyname, cJSON_GetStringValue(key)))
		VM_ERROR("Attempted to load save data with wrong key: %s", display_sjis0(keyname));

	if (group_name) {
		// TODO?
	}

	cJSON *globals = cJSON_GetObjectItem(save, "globals");
	if (!globals || !cJSON_IsArray(globals)) {
		invalid_save_data("Not an array", globals);
		goto cleanup;
	}

	cJSON *g;
	cJSON_ArrayForEach(g, globals) {
		cJSON *index, *value;
		if (!(index = cJSON_GetObjectItem(g, "index"))) {
			invalid_save_data("Missing index", g);
			goto cleanup;
		}
		if (!(value = cJSON_GetObjectItem(g, "value"))) {
			invalid_save_data("Missing value", g);
			goto cleanup;
		}
		if (!cJSON_IsNumber(index)) {
			invalid_save_data("Not a number", index);
			goto cleanup;
		}

		int i = index->valueint;
		if (i < 0 || i > ain->nr_globals) {
			invalid_save_data("Invalid global index", index);
			goto cleanup;
		}
		current_global = i;

		bool call_dtors = false; // Destructors for old objects are not called.
		global_set(i, json_to_vm_value(ain->globals[i].type.data, ain->globals[i].type.struc, ain->globals[i].type.rank, value), call_dtors);
		if (n)
			(*n)++;
	}

	retval = 1;
cleanup:
	cJSON_Delete(save);
	return retval;
}

int delete_save_file(const char *filename)
{
	char *path = savedir_path(filename);
	if (!file_exists(path)) {
		free(path);
		return 0;
	}
	if (remove(path)) {
		WARNING("remove(\"%s\"): %s", display_utf0(path), strerror(errno));
		free(path);
		return 0;
	}
	free(path);
	return 1;
}
