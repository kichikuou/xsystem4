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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <ctype.h>
#include <assert.h>

#include "system4/string.h"
#include "system4/utfsjis.h"

#include "vm.h"
#include "vm/heap.h"
#include "vm/page.h"

#include "debugger.h"
#include "little_endian.h"
#include "xsystem4.h"

struct dbg_cmd {
	const char *fullname;
	const char *shortname;
	const char *description;
	unsigned min_args;
	unsigned max_args;
	void (*run)(unsigned nr_args, char **args);
};

static void dbg_cmd_help(unsigned nr_args, char **args);

static void dbg_cmd_backtrace(unsigned nr_args, char **args)
{
	dbg_print_stack_trace();
}

static void dbg_cmd_breakpoint(unsigned nr_args, char **args)
{
	int addr = atoi(args[0]);
	if (addr > 0) {
		dbg_set_address_breakpoint(addr, NULL, NULL);
	} else {
		dbg_set_function_breakpoint(args[0], NULL, NULL);
	}
}

static void dbg_cmd_continue(unsigned nr_args, char **args)
{
	dbg_continue();
}

static struct string *value_to_string(struct ain_type *type, union vm_value value, int recursive)
{
	switch (type->data) {
	case AIN_INT:
		return integer_to_string(value.i);
	case AIN_FLOAT:
		return float_to_string(value.f, 6);
		break;
	case AIN_STRING: {
		struct string *out = cstr_to_string("\"");
		string_append(&out, heap_get_string(value.i));
		string_push_back(&out, '"');
		return out;
	}
	case AIN_STRUCT:
	case AIN_REF_STRUCT: {
		struct page *page = heap_get_page(value.i);
		if (page->nr_vars == 0) {
			return cstr_to_string("{}");
		}
		if (!recursive) {
			return cstr_to_string("{ <...> }");
		}
		struct string *out = cstr_to_string("{ ");
		for (int i = 0; i < page->nr_vars; i++) {
			struct ain_variable *m = &ain->structures[type->struc].members[i];
			if (i) {
				string_append_cstr(&out, "; ", 2);
			}
			string_append_cstr(&out, m->name, strlen(m->name));
			string_append_cstr(&out, " = ", 3);
			struct string *tmp = value_to_string(&m->type, page->values[i], recursive-1);
			string_append(&out, tmp);
			free_string(tmp);
		}
		string_append_cstr(&out, " }", 2);
		return out;
	}
	case AIN_ARRAY_TYPE:
	case AIN_REF_ARRAY_TYPE: {
		struct page *page = heap_get_page(value.i);
		if (!page || page->nr_vars == 0) {
			return cstr_to_string("[]");
		}
		// get member type
		struct ain_type t;
		t.data = variable_type(page, 0, &t.struc, &t.rank);
		t.array_type = NULL;
		if (!recursive) {
			switch (t.data) {
			case AIN_STRUCT:
			case AIN_REF_STRUCT:
			case AIN_ARRAY_TYPE:
			case AIN_REF_ARRAY_TYPE:
				return cstr_to_string("[ <...> ]");
			default:
				break;
			}
		}
		struct string *out = cstr_to_string("[ ");
		for (int i = 0; i < page->nr_vars; i++) {
			if (i) {
				string_append_cstr(&out, "; ", 2);
			}
			struct string *tmp = value_to_string(&t, page->values[i], recursive-1);
			string_append(&out, tmp);
			free_string(tmp);
		}
		string_append_cstr(&out, " ]", 2);
		return out;
	}
	default:
		return cstr_to_string("<unsupported-data-type>");
	}
	return string_ref(&EMPTY_STRING);
}

static void dbg_cmd_locals(unsigned nr_args, char **args)
{
	unsigned frame_no = nr_args > 0 ? atoi(args[0]) : 0;
	if (frame_no < 0 || frame_no >= call_stack_ptr) {
		DBG_ERROR("Invalid frame number: %u", frame_no);
		return;
	}

	struct page *page = heap_get_page(call_stack[call_stack_ptr - (frame_no+1)].page_slot);
	struct ain_function *f = &ain->functions[page->index];
	for (int i = 0; i < f->nr_vars; i++) {
		struct string *value = value_to_string(&f->vars[i].type, page->values[i], 1);
		printf("[%d] %s: %s\n", i, display_sjis0(f->vars[i].name), display_sjis1(value->text));
		free_string(value);
	}
}

static void log_handler(struct breakpoint *bp)
{
	struct ain_function *f = bp->data;
	struct page *locals = local_page();
	printf("%s(", display_sjis0(f->name));
	for (int i = 0; i < f->nr_args; i++) {
		struct string *value = value_to_string(&f->vars[i].type, locals->values[i], 1);
		printf(i > 0 ? ", %s" : "%s", display_sjis0(value->text));
		free_string(value);
	}
	printf(")\n");
}

static void dbg_cmd_log(unsigned nr_args, char **args)
{
	int fno = ain_get_function(ain, args[0]);
	if (fno < 0) {
		DBG_ERROR("No function with name '%s'", args[0]);
		return;
	}
	dbg_set_function_breakpoint(args[0], log_handler, &ain->functions[fno]);
}

struct ain_variable *dbg_get_variable(const char *name, union vm_value *val_out)
{
	struct page *page = local_page();
	struct ain_function *f = &ain->functions[page->index];
	for (int i = 0; i < f->nr_vars; i++) {
		if (!strcmp(f->vars[i].name, name)) {
			*val_out = page->values[i];
			return &f->vars[i];
		}
	}
	for (int i = 0; i < ain->nr_globals; i++) {
		if (!strcmp(ain->globals[i].name, name)) {
			*val_out = global_get(i);
			return &ain->globals[i];
		}
	}
	return NULL;
}

static void dbg_cmd_print(unsigned nr_args, char **args)
{
	union vm_value value;
	struct ain_variable *var = dbg_get_variable(args[0], &value);
	if (var) {
		struct string *v = value_to_string(&var->type, value, 1);
		printf("%s\n", display_sjis0(v->text));
		free_string(v);
		return;
	}
	// TODO: print functions, etc.
	DBG_ERROR("Undefined variable: %s", args[0]);
}

static void dbg_cmd_quit(unsigned nr_args, char **args)
{
	dbg_quit();
}

#ifdef HAVE_SCHEME
static void dbg_cmd_scheme(unsigned nr_args, char **args)
{
	dbg_scm_repl();
}
#endif

static struct dbg_cmd dbg_commands[] = {
	{ "help",       "h",   "[command-name] - Display this message",  0, 1, dbg_cmd_help },
	{ "backtrace",  "bt",  "- Display stack trace",                  0, 0, dbg_cmd_backtrace },
	{ "breakpoint", "bp",  "<function-or-address> - Set breakpoint", 1, 1, dbg_cmd_breakpoint },
	{ "continue",   "c",   "- Resume execution",                     0, 0, dbg_cmd_continue },
	{ "locals",     "l",   "[frame-number] - Print local variables", 0, 1, dbg_cmd_locals },
	{ "log",        NULL,  "<function-name> - Log function calls",   1, 1, dbg_cmd_log },
	{ "print",      "p",   "<variable-name> - Print a variable",     1, 1, dbg_cmd_print },
	{ "quit",       "q",   "- Quit xsystem4",                        0, 0, dbg_cmd_quit },
#ifdef HAVE_SCHEME
	{ "scheme",     "scm", "- Drop into Scheme REPL",                0, 0, dbg_cmd_scheme },
#endif
};

static void dbg_cmd_help(unsigned nr_args, char **args)
{
	NOTICE("Available Commands");
	NOTICE("------------------");
	for (unsigned i = 0; i < sizeof(dbg_commands)/sizeof(*dbg_commands); i++) {
		struct dbg_cmd *cmd = &dbg_commands[i];
		if (!nr_args || !strcmp(args[0], cmd->fullname) || !strcmp(args[0], cmd->shortname)) {
			if (cmd->shortname)
				NOTICE("%s (%s) %s", cmd->fullname, cmd->shortname, cmd->description);
			else
				NOTICE("%s %s", cmd->fullname, cmd->description);
		}
	}
}

/*
 * Get a command by name.
 */
static struct dbg_cmd *dbg_get_command(const char *name)
{
	for (unsigned i = 0; i < sizeof(dbg_commands)/sizeof(*dbg_commands); i++) {
		struct dbg_cmd *cmd = &dbg_commands[i];
		if (!strcmp(name, cmd->fullname) || (cmd->shortname && !strcmp(name, cmd->shortname)))
			return cmd;
	}
	return NULL;
}

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
/*
 * Get a string from the user. The result can be modified but shouldn't be free'd.
 */
static char *cmd_gets(void)
{
	static char *line_read = NULL;
	free(line_read);
	line_read = readline("dbg(cmd)> ");
	if (line_read && *line_read)
		add_history(line_read);
	return line_read;
}
#else
/*
 * Get a string from the user. The result can be modified but shouldn't be free'd.
 */
static char *cmd_gets(void)
{
	static char line[1024];

	printf("dbg(cmd)> ");
	fflush(stdout);
	fgets(line, 1024, stdin);
	return line;
}
#endif

/*
 * Parse a line into an array of words. Modified the input string.
 */
static char **cmd_parse(char *line, unsigned *nr_words)
{
	static char *words[32];

	*nr_words = 0;
	while (*line) {
		// skip whitespace
		while (*line && isspace(*line)) line++;
		if (!*line) break;
		// save word ptr
		words[(*nr_words)++] = line;
		// skip word
		while (*line && !isspace(*line)) line++;
		if (!*line) break;
		*line = '\0';
		line++;
	}
	return words;
}

void dbg_cmd_repl(void)
{
	NOTICE("Entering the debugger REPL. Type 'help' for a list of commands.");
	while (1) {
		char *line = cmd_gets();
		if (!line)
			continue;

		unsigned nr_words;
		char **words = cmd_parse(line, &nr_words);
		if (!nr_words)
			continue;

		struct dbg_cmd *cmd = dbg_get_command(words[0]);
		if (!cmd) {
			NOTICE("Invalid command: %s (type 'help' for a list of commands)", words[0]);
			continue;
		}

		if ((nr_words-1) < cmd->min_args || (nr_words-1) > cmd->max_args) {
			NOTICE("Wrong number of arguments to '%s' command", cmd->fullname);
			continue;
		}

		cmd->run(nr_words-1, words+1);
	}
}
