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

#ifndef SYSTEM4_PAGE_H
#define SYSTEM4_PAGE_H

#include <stdint.h>
#include "vm.h"
#include "ain.h"

struct ain_variable;
enum ain_data_type;

enum page_type {
	LOCAL_PAGE,
	STRUCT_PAGE,
	ARRAY_PAGE
};

#define NR_PAGE_TYPES (ARRAY_PAGE+1)

/*
 * A page is an ordered collection of variables. Pages are used to implement
 * global and local variables, structures and arrays as follows,
 *
 * Global variables: all global variables are stored in a single global page.
 *
 * Local variables: each function invocation is backed by a page storing its
 * local variables.
 *
 * Structures: each struct object is backed by a page storing its members.
 *
 * Arrays: each array object is backed by a page storing its members.
 * Multi-dimensional arrays are implemented as a tree of pages (meaning the
 * whole array is NOT contiguous in memory).
 */
struct page {
	enum page_type type;
	union {
		int index;
		enum ain_data_type a_type;
	};
	int struct_type; // for array pages
	int rank; // for array pages
	int nr_vars;
	union vm_value values[];
};

// variables
union vm_value variable_initval(enum ain_data_type type);
enum ain_data_type variable_type(struct page *page, int varno, enum ain_data_type *struct_type);

// pages
struct page *alloc_page(enum page_type type, int type_index, int nr_vars);
void free_page(struct page *page);
struct page *copy_page(struct page *page);
void delete_page(struct page *page);

// structs
void create_struct(int no, union vm_value *var);

// arrays
struct page *alloc_array(int rank, union vm_value *dimensions, int data_type, int struct_type);
struct page *realloc_array(struct page *src, int rank, union vm_value *dimensions, int data_type, int struct_type);
int array_numof(struct page *page, int rank);
void array_copy(struct page *dst, int dst_i, struct page *src, int src_i, int n);
int array_fill(struct page *dst, int dst_i, int n, union vm_value v);
void array_pushback(struct page **dst, union vm_value v, int data_type, int struct_type);
void array_popback(struct page **dst);
bool array_erase(struct page **page, int i);
void array_insert(struct page **_page, int i, union vm_value v, int data_type, int struct_type);
void array_sort(struct page *page, int compare_fno);

#endif /* SYSTEM4_PAGE_H */