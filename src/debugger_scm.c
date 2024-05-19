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

#define VM_PRIVATE

#include <stdio.h>
#include <assert.h>
#include <chibi/eval.h>

#include "system4/ain.h"
#include "system4/file.h"
#include "system4/instructions.h"
#include "system4/little_endian.h"
#include "system4/string.h"
#include "system4/utfsjis.h"
#include "vm.h"
#include "vm/heap.h"
#include "vm/page.h"

#include "debugger.h"

struct variable {
	enum ain_data_type data_type;
	int struct_type;
	char *name;
	int varno;
	union vm_value *value;
};

static void free_variable(struct variable *v)
{
	free(v->name);
	free(v);
}

static char *to_utf(const char *sjis)
{
	return sjis2utf(sjis, strlen(sjis));
}

static const char *vm_value_string(union vm_value *v)
{
	return to_utf(heap[v->i].s->text);
}

static struct page *get_page(int pageno)
{
	if (!page_index_valid(pageno))
		return NULL;
	return heap_get_page(pageno);
}

static struct variable *get_global_by_name(const char *name)
{
	for (int i = 0; i < ain->nr_globals; i++) {
		if (!strcmp(ain->globals[i].name, name)) {
			struct variable *v = malloc(sizeof(struct variable));
			v->data_type = ain->globals[i].type.data;
			v->struct_type = ain->globals[i].type.struc;
			v->name = to_utf(ain->globals[i].name);
			v->varno = i;
			v->value = &heap[0].page->values[i];
			return v;
		}
	}
	return NULL;
}

static struct variable *get_local_by_name(const char *name)
{
	struct page *page = local_page();
	struct ain_function *f = &ain->functions[page->index];

	for (int i = 0; i < f->nr_vars; i++) {
		if (!strcmp(f->vars[i].name, name)) {
			struct variable *v = malloc(sizeof(struct variable));
			v->data_type = f->vars[i].type.data;
			v->struct_type = f->vars[i].type.struc;
			v->name = to_utf(ain->globals[i].name);
			v->varno = i;
			v->value = &page->values[i];
			return v;
		}
	}
	return NULL;
}

static const char *variable_name(struct page *page, int varno)
{
	switch (page->type) {
	case GLOBAL_PAGE:
		return ain->globals[varno].name;
	case LOCAL_PAGE:
		return ain->functions[page->index].vars[varno].name;
	case STRUCT_PAGE:
		return ain->structures[page->index].members[varno].name;
	case ARRAY_PAGE:
		return "array member";
	case DELEGATE_PAGE:
		return "delegate member";
	}
	return "unknown";
}

static struct variable *page_ref(struct page *page, int i)
{
	if (i < 0 || i >= page->nr_vars)
		return NULL;

	struct variable *v = malloc(sizeof(struct variable));
	v->data_type = variable_type(page, i, &v->struct_type, NULL);
	v->name = to_utf(variable_name(page, i));
	v->varno = i;
	v->value = &page->values[i];
	return v;
}

static sexp dbg_ctx;

static void set_parameter(const char *param, sexp value)
{
	sexp_eval(dbg_ctx, sexp_list2(dbg_ctx, sexp_intern(dbg_ctx, param, -1), value), NULL);
}

static void dbg_scm_breakpoint_handler(struct breakpoint *bp)
{
	set_parameter("breakpoint-count", sexp_make_integer(dbg_ctx, bp->count++));
	// TODO: breakpoint-function parameter with function name, argument count, etc.

	if ((sexp)bp->data == SEXP_FALSE) {
		NOTICE("%s", bp->message);
		dbg_scm_repl();
	} else {
		sexp_apply(dbg_ctx, (sexp)bp->data, SEXP_NULL);
	}
}

static void set_function_breakpoint(const char *name, sexp handler)
{
	if (handler == SEXP_FALSE) {
		dbg_set_function_breakpoint(name, NULL, NULL);
		return;
	}
	if (!sexp_procedurep(handler)) {
		DBG_ERROR("Breakpoint handler must be a procedure or #f");
		return;
	}
	if (dbg_set_function_breakpoint(name, dbg_scm_breakpoint_handler, handler)) {
		sexp_preserve_object(dbg_ctx, handler);
	}
}

static void set_address_breakpoint(uint32_t address, sexp handler)
{
	if (handler == SEXP_FALSE) {
		dbg_set_address_breakpoint(address, NULL, NULL);
		return;
	}
	if (!sexp_procedurep(handler)) {
		DBG_ERROR("Breakpoint handler must be a procedure or #f");
		return;
	}
	if (dbg_set_address_breakpoint(address, dbg_scm_breakpoint_handler, handler)) {
		sexp_preserve_object(dbg_ctx, handler);
	}
}

static struct page *frame_page(int i)
{
	if (i < 0 || i >= call_stack_ptr)
		return NULL;
	return get_page(call_stack[call_stack_ptr - (i+1)].page_slot);
}

// XXX: chibi-ffi doesn't support anonymous structs
#define _page_struct_type array.struct_type
#define _page_rank array.rank

// autogenerated with `chibi-ffi src/debug-ffi.stub`
#include "debug-ffi.c"

void dbg_scm_init(void)
{
	if (!dbg_enabled)
		return;
	dbg_ctx = sexp_make_eval_context(NULL, NULL, NULL, 0, 0);
	sexp_load_standard_env(dbg_ctx, NULL, SEXP_SEVEN);
	sexp_load_standard_ports(dbg_ctx, NULL, stdin, stdout, stderr, 1);
	sexp_init_library(dbg_ctx, NULL, 0, sexp_context_env(dbg_ctx), sexp_version, SEXP_ABI_IDENTIFIER);
	sexp_eval_string(dbg_ctx, "(import (scheme base))", -1, NULL);
	sexp_eval_string(dbg_ctx, "(import (chibi repl))", -1, NULL);
	sexp_eval_string(dbg_ctx, "(define *prompt* (make-parameter \"dbg(scm)> \"))", -1, NULL);
	sexp_eval_string(dbg_ctx, "(define breakpoint-count (make-parameter 0))", -1, NULL);
	sexp_eval_string(dbg_ctx, "(define (repl-make-prompt m) (*prompt*))", -1, NULL);

	if (file_exists("./debugger.scm"))
		sexp_eval_string(dbg_ctx, "(load \"./debugger.scm\")", -1, NULL);
	else
		sexp_eval_string(dbg_ctx, "(load \"" XSYS4_DATA_DIR "/debugger.scm\")", -1, NULL);
}

void dbg_scm_fini(void)
{
	if (!dbg_enabled)
		return;
	sexp_destroy_context(dbg_ctx);
}

void dbg_scm_repl(void)
{
	if (!dbg_enabled)
		return;
	NOTICE("Entering the Scheme debugger REPL. Type '\\exit' to exit.");
	sexp_eval_string(dbg_ctx, "(repl 'escape: #\\\\ 'make-prompt: repl-make-prompt)", -1, NULL);
}


