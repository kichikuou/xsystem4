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

// enable private VM interface
#define VM_PRIVATE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "cJSON.h"

#include "system4.h"
#include "system4/file.h"
#include "system4/string.h"

#include "savedata.h"
#include "vm.h"
#include "vm/heap.h"
#include "vm/page.h"
#include "xsystem4.h"

/*
 * Save/load VM images as JSON.
 */

static const char * const page_type_strtab[] = {
	[GLOBAL_PAGE] = "globals",
	[LOCAL_PAGE]  = "locals",
	[STRUCT_PAGE] = "struct",
	[ARRAY_PAGE]  = "array"
};

static enum page_type string_to_page_type(const char *str)
{
	if (!strcmp(str, "globals")) return GLOBAL_PAGE;
	if (!strcmp(str, "locals"))  return LOCAL_PAGE;
	if (!strcmp(str, "struct"))  return STRUCT_PAGE;
	if (!strcmp(str, "array"))   return ARRAY_PAGE;
	VM_ERROR("Invalid page type: %s", str);
}

static cJSON *value_to_json(union vm_value v)
{
	return cJSON_CreateNumber(v.i);
}

static int get_number(int i, void *data)
{
	return ((union vm_value*)data)[i].i;
}

static cJSON *resume_page_to_json(struct page *page)
{
	if (!page)
		return cJSON_CreateNull();

	cJSON *json = cJSON_CreateObject();
	cJSON_AddStringToObject(json, "type", page_type_strtab[page->type]);
	cJSON_AddNumberToObject(json, "subtype", page->index);
	if (page->type == ARRAY_PAGE) {
		cJSON_AddNumberToObject(json, "struct-type", page->array.struct_type);
		cJSON_AddNumberToObject(json, "rank", page->array.rank);
	}

	cJSON *values = cJSON_CreateIntArray_cb(page->nr_vars, get_number, page->values);

	cJSON_AddItemToObject(json, "values", values);
	return json;
}

static cJSON *heap_to_json(void)
{
	cJSON *json = cJSON_CreateArray();

	for (size_t i = 0; i < heap_size; i++) {
		if (!heap[i].ref)
			continue;

		cJSON *item = cJSON_CreateArray();
		cJSON_AddItemToArray(item, cJSON_CreateNumber(i));
		cJSON_AddItemToArray(item, cJSON_CreateNumber(heap[i].ref));
		switch (heap[i].type) {
		case VM_PAGE:
			cJSON_AddItemToArray(item, resume_page_to_json(heap[i].page));
			break;
		case VM_STRING:
			cJSON_AddItemToArray(item, cJSON_CreateString(heap[i].s->text));
			break;
		}
		cJSON_AddItemToArray(json, item);
	}
	return json;
}

static cJSON *funcall_to_json(struct function_call *call)
{
	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "function", call->fno);
	cJSON_AddNumberToObject(json, "return-address", call->return_address);
	cJSON_AddNumberToObject(json, "local-page", call->page_slot);
	cJSON_AddNumberToObject(json, "struct-page", call->struct_page);
	return json;
}

static cJSON *call_stack_to_json(void)
{
	cJSON *json = cJSON_CreateArray();
	for (int i = 0; i < call_stack_ptr; i++) {
		cJSON_AddItemToArray(json, funcall_to_json(&call_stack[i]));
	}
	return json;
}

static cJSON *stack_to_json(void)
{
	cJSON *json = cJSON_CreateArray();
	for (int i = 0; i < stack_ptr; i++) {
		cJSON_AddItemToArray(json, value_to_json(stack[i]));
	}
	return json;
}

static cJSON *vm_image_to_json(const char *key)
{
	cJSON *image = cJSON_CreateObject();
	cJSON_AddStringToObject(image, "key", key);
	cJSON_AddItemToObject(image, "heap", heap_to_json());
	cJSON_AddItemToObject(image, "call-stack", call_stack_to_json());
	cJSON_AddItemToObject(image, "stack", stack_to_json());
	cJSON_AddNumberToObject(image, "ip", instr_ptr);
	return image;
}

int vm_save_image(const char *key, const char *path)
{
	cJSON *image = vm_image_to_json(key);
	int r = save_json(path, image);
	cJSON_Delete(image);
	return r;
}

#define _invalid_save_data(file, func, line, fmt, ...)	\
	_vm_error("*ERROR*(%s:%s:%d): " fmt "\n", file, func, line, ##__VA_ARGS__)
#define invalid_save_data(fmt, ...) \
	_invalid_save_data(__FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

static const char *json_strtype(int type)
{
	switch (type & 0xFF) {
	case cJSON_False:
	case cJSON_True:
		return "boolean";
	case cJSON_NULL:   return "null";
	case cJSON_Number: return "number";
	case cJSON_String: return "string";
	case cJSON_Array:  return "array";
	case cJSON_Object: return "object";
	}
	return "unknown-type";
}

static cJSON *_type_check(const char *file, const char *func, int line, int type, cJSON *json)
{
	if (!json)
		_invalid_save_data(file, func, line, "Expected %s but got NULL", json_strtype(type));
	if (!(json->type & type))
		_invalid_save_data(file, func, line, "Expected %s but value is of type %s", json_strtype(type), json_strtype(json->type));
	return json;
}

#define type_check(type, json) _type_check(__FILE__, __func__, __LINE__, type, json)

static void load_page(int slot, cJSON *json)
{
	int struct_type = 0, rank = 0;

	// unpack
	cJSON *type    = type_check(cJSON_String, cJSON_GetObjectItem(json, "type"));
	cJSON *subtype = type_check(cJSON_Number, cJSON_GetObjectItem(json, "subtype"));
	cJSON *values  = type_check(cJSON_Array,  cJSON_GetObjectItem(json, "values"));

	enum page_type page_type = string_to_page_type(cJSON_GetStringValue(type));
	if (page_type == ARRAY_PAGE) {
		cJSON *_struct_type = type_check(cJSON_Number, cJSON_GetObjectItem(json, "struct-type"));
		cJSON *_rank        = type_check(cJSON_Number, cJSON_GetObjectItem(json, "rank"));
		struct_type = _struct_type->valueint;
		rank = _rank->valueint;
	}

	// allocate page
	struct page *page = alloc_page(page_type, subtype->valueint, cJSON_GetArraySize(values));
	page->array.struct_type = struct_type;
	page->array.rank = rank;

	// init page variables
	int i = 0;
	cJSON *item;
	cJSON_ArrayForEach(item, values) {
		page->values[i].i = item->valueint;
		i++;
	}

	heap[slot].page = page;
	heap[slot].type = VM_PAGE;
}

static void load_string(int slot, cJSON *json)
{
	const char *str = cJSON_GetStringValue(json);
	heap[slot].s = make_string(str, strlen(str));
	heap[slot].type = VM_STRING;
}

static void delete_heap(void)
{
	// free heap
	for (size_t i = 0; i < heap_size; i++) {
		if (!heap[i].ref)
			continue;
		switch (heap[i].type) {
		case VM_PAGE:
			if (heap[i].page)
				free_page(heap[i].page);
			break;
		case VM_STRING:
			free_string(heap[i].s);
			break;
		}
		heap[i].ref = 0;
	}

	heap_free_ptr = 0;
	for (size_t i = 0; i < heap_size; i++) {
		heap_free_stack[i] = i;
	}
}

// Allocate a specific heap slot
// XXX: This only works while heap_free_stack[i]=i
//      After calling delete_heap, this can be called for INCREASING indices.
//      Out-of-order allocations would break the above assumption!
static void alloc_heap_slot(int slot)
{
	if ((size_t)slot >= heap_size) {
		size_t next_size = heap_size * 2;
		while ((size_t)slot >= next_size)
			next_size *= 2;
		heap_grow(next_size);
	}
	heap_free_stack[slot] = heap_free_stack[heap_free_ptr];
	heap_free_ptr++;
}

static void load_heap(cJSON *json)
{
	delete_heap();

	cJSON *item;
	cJSON_ArrayForEach(item, json) {
		type_check(cJSON_Array, item);
		if (cJSON_GetArraySize(item) != 3)
			invalid_save_data("Invalid heap data");

		int slot = type_check(cJSON_Number, cJSON_GetArrayItem(item, 0))->valueint;
		int ref  = type_check(cJSON_Number, cJSON_GetArrayItem(item, 1))->valueint;
		cJSON *value = cJSON_GetArrayItem(item, 2);

		alloc_heap_slot(slot);
		heap[slot].ref = ref;

		if (cJSON_IsString(value)) {
			load_string(slot, value);
		} else if (cJSON_IsObject(value)) {
			load_page(slot, value);
		} else if (cJSON_IsNull(value)) {
			heap[slot].type = VM_PAGE;
			heap[slot].page = NULL;
		} else {
			invalid_save_data("Invalid heap data");
		}
	}
}

static void load_call_stack(cJSON *json)
{
	call_stack_ptr = 0;
	cJSON *item;
	cJSON_ArrayForEach(item, json) {
		type_check(cJSON_Object, item);
		call_stack[call_stack_ptr++] = (struct function_call) {
			.fno            = type_check(cJSON_Number, cJSON_GetObjectItem(item, "function"))->valueint,
			.return_address = type_check(cJSON_Number, cJSON_GetObjectItem(item, "return-address"))->valueint,
			.page_slot      = type_check(cJSON_Number, cJSON_GetObjectItem(item, "local-page"))->valueint,
			.struct_page    = type_check(cJSON_Number, cJSON_GetObjectItem(item, "struct-page"))->valueint
		};
	}
}

static void load_stack(cJSON *json)
{
	stack_ptr = 0;

	cJSON *item;
	cJSON_ArrayForEach(item, json) {
		type_check(cJSON_Number, item);
		stack_push_value(vm_int(item->valueint));
	}
}

static cJSON *read_image(const char *keyname, const char *path)
{
	char *full_path = savedir_path(path);
	char *save_file = file_read(full_path, NULL);
	if (!save_file) {
		free(save_file);
		free(full_path);
		return NULL;
	}

	cJSON *save = cJSON_Parse(save_file);
	cJSON *key = type_check(cJSON_String, cJSON_GetObjectItem(save, "key"));
	if (strcmp(keyname, cJSON_GetStringValue(key)))
		invalid_save_data("Key doesn't match");

	free(save_file);
	free(full_path);
	return type_check(cJSON_Object, save);
}

void vm_load_image(const char *key, const char *path)
{
	cJSON *save = read_image(key, path);
	if (!save) {
		VM_ERROR("Failed to read VM image: '%s'", display_sjis0(path));
	}
	cJSON *ip = type_check(cJSON_Number, cJSON_GetObjectItem(save, "ip"));
	load_heap(type_check(cJSON_Array, cJSON_GetObjectItem(save, "heap")));
	load_call_stack(type_check(cJSON_Array, cJSON_GetObjectItem(save, "call-stack")));
	load_stack(type_check(cJSON_Array, cJSON_GetObjectItem(save, "stack")));
	instr_ptr = ip->valueint;
	cJSON_Delete(save);
}

struct page *vm_load_image_comments(const char *key, const char *path, int *success)
{
	cJSON *save = read_image(key, path);
	if (!save) {
		*success = 0;
		return NULL;
	}
	cJSON *comments = cJSON_GetObjectItem(save, "comments");
	if (comments)
		type_check(cJSON_Array, comments);
	if (!comments || cJSON_GetArraySize(comments) == 0) {
		*success = 1;
		return NULL;
		//union vm_value dims = { .i = 0 };
		//return alloc_array(1, &dims, AIN_ARRAY_STRING, 0, false);
	}

	// read comments from JSON array into VM array
	union vm_value dims = { .i = cJSON_GetArraySize(comments) };
	struct page *array = alloc_array(1, &dims, AIN_ARRAY_STRING, 0, false);
	for (int i = 0; i < dims.i; i++) {
		int slot = heap_alloc_slot(VM_STRING);
		cJSON *jstr = type_check(cJSON_String, cJSON_GetArrayItem(comments, i));
		char *str = cJSON_GetStringValue(jstr);
		if (!str[0]) {
			heap[slot].s = string_ref(&EMPTY_STRING);
		} else {
			heap[slot].s = make_string(str, strlen(str));
		}
		array->values[i].i = slot;
	}

	*success = 1;
	cJSON_Delete(save);
	return array;
}

int vm_write_image_comments(const char *key, const char *path, struct page *comments)
{
	cJSON *save = read_image(key, path);
	if (!save) {
		// create blank save
		save = cJSON_CreateObject();
		cJSON_AddStringToObject(save, "key", key);
	}
	cJSON_DeleteItemFromObject(save, "comments");

	// TODO: check that comments is an array of strings
	cJSON *array = cJSON_CreateArray();
	for (int i = 0; i < comments->nr_vars; i++) {
		cJSON *s = cJSON_CreateString(heap_get_string(comments->values[i].i)->text);
		cJSON_AddItemToArray(array, s);
	}
	cJSON_AddItemToObject(save, "comments", array);

	save_json(path, save);
	cJSON_Delete(save);
	return 1;
}
