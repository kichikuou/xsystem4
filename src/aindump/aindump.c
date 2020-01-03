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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include "little_endian.h"
#include "system4.h"
#include "system4/ain.h"
#include "system4/instructions.h"
#include "system4/string.h"
#include "system4/utfsjis.h"

void disassemble_ain(struct ain *ain, FILE *out);

static void usage(void)
{
	puts("Usage: aindump <options> <ainfile>");
	puts("    Display information from AIN files.");
	puts("");
	puts("    -h, --help               Display this message and exit");
	puts("    -c, --code               Dump code section");
	puts("    -f, --functions          Dump functions section");
	puts("    -g, --globals            Dump globals section");
	puts("    -S, --structures         Dump structures section");
	puts("    -m, --messages           Dump messages section");
	puts("    -l, --libraries          Dump libraries section");
	puts("        --function-types     Dump function type section");
	puts("        --global-group-names Dump global group names section");
	puts("    -e, --enums              Dump enums section");
	//puts("    -j, --json               Dump to JSON format"); // TODO
}

static void print_sjis(FILE *f, const char *s)
{
	char *u = sjis2utf(s, strlen(s));
	fprintf(f, "%s", u);
	free(u);
}

static void ain_dump_version(FILE *f, struct ain *ain)
{
	fprintf(f, "AIN VERSION %d\n", ain->version);
}

static void print_arglist(FILE *f, struct ain *ain, struct ain_variable *args, int nr_args)
{
	if (!nr_args) {
		fprintf(f, "(void)");
		return;
	}
	fputc('(', f);
	for (int i = 0; i < nr_args; i++) {
		if (args[i].data_type == AIN_VOID)
			continue;
		if (i > 0)
			fprintf(f, ", ");
		print_sjis(f, ain_strtype(ain, args[i].data_type, args[i].struct_type));
		fputc(' ', f);
		print_sjis(f, args[i].name);
	}
	fputc(')', f);
}

static void print_function(FILE *out, struct ain *ain, struct ain_function *f)
{
	print_sjis(out, ain_strtype(ain, f->data_type, f->struct_type));
	fputc(' ', out);
	print_sjis(out, f->name);
	print_arglist(out, ain, f->vars, f->nr_args);
	fputc('\n', out);
}

static void ain_dump_functions(FILE *f, struct ain *ain)
{
	for (int i = 0; i < ain->nr_functions; i++) {
		print_function(f, ain, &ain->functions[i]);
	}
}

static void ain_dump_globals(FILE *f, struct ain *ain)
{
	for (int i = 0; i < ain->nr_globals; i++) {
		struct ain_global *g = &ain->globals[i];
		if (g->data_type == AIN_VOID)
			continue;
		print_sjis(f, ain_strtype(ain, g->data_type, g->struct_type));
		fputc(' ', f);
		print_sjis(f, g->name);
		fputc('\n', f);
	}
}

static void print_structure(FILE *f, struct ain *ain, struct ain_struct *s)
{
	fprintf(f, "struct ");
	print_sjis(f, s->name);
	fprintf(f, " {\n");
	for (int i = 0; i < s->nr_members; i++) {
		struct ain_variable *m = &s->members[i];
		if (m->data_type == AIN_VOID)
			continue;
		fprintf(f, "    ");
		print_sjis(f, ain_strtype(ain, m->data_type, m->struct_type));
		fputc(' ', f);
		print_sjis(f, m->name);
		fprintf(f, ";\n");
	}
	fprintf(f, "};\n\n");
}

static void ain_dump_structures(FILE *f, struct ain *ain)
{
	for (int i = 0; i < ain->nr_structures; i++) {
		print_structure(f, ain, &ain->structures[i]);
	}
}

static void ain_dump_messages(FILE *f, struct ain *ain)
{
	for (int i = 0; i < ain->nr_messages; i++) {
		print_sjis(f, ain->messages[i]->text);
		//fprintf(f, "\n---\n");
	}
}

static void ain_dump_libraries(FILE *out, struct ain *ain)
{
	for (int i = 0; i < ain->nr_libraries; i++) {
		fprintf(out, "--- ");
		print_sjis(out, ain->libraries[i].name);
		fprintf(out, " ---\n");
		for (int j = 0; j < ain->libraries[i].nr_functions; j++) {
			struct ain_hll_function *f = &ain->libraries[i].functions[j];
			print_sjis(out, ain_strtype(ain, f->data_type, -1));
			fputc(' ', out);
			print_sjis(out, f->name);
			fputc('(', out);
			for (int k = 0; k < f->nr_arguments; k++) {
				struct ain_hll_argument *a = &f->arguments[k];
				if (a->data_type == AIN_VOID)
					continue;
				if (k > 0)
					fprintf(out, ", ");
				print_sjis(out, ain_strtype(ain, a->data_type, -1));
				fputc(' ', out);
				print_sjis(out, a->name);
			}
			fprintf(out, ")\n");
		}
	}
}

static void ain_dump_strings(FILE *f, struct ain *ain)
{
	for (int i = 0; i < ain->nr_strings; i++) {
		print_sjis(f, ain->strings[i]->text);
		//fprintf(f, "\n---\n");
	}
}

static void ain_dump_filenames(FILE *f, struct ain *ain)
{
	for (int i = 0; i < ain->nr_filenames; i++) {
		print_sjis(f, ain->filenames[i]);
		fputc('\n', f);
	}
}

static void ain_dump_functypes(FILE *f, struct ain *ain)
{
	for (int i = 0; i < ain->nr_function_types; i++) {
		struct ain_function_type *t = &ain->function_types[i];
		fprintf(f, "functype ");
		print_sjis(f, ain_strtype(ain, t->data_type, t->struct_type));
		fputc(' ', f);
		print_sjis(f, t->name);
		print_arglist(f, ain, t->variables, t->nr_arguments);
		fputc('\n', f);
	}
}

static void ain_dump_global_group_names(FILE *f, struct ain *ain)
{
	for (int i = 0; i < ain->nr_global_groups; i++) {
		print_sjis(f, ain->global_group_names[i]);
		fputc('\n', f);
	}
}

static void ain_dump_enums(FILE *f, struct ain *ain)
{
	for (int i = 0; i < ain->nr_enums; i++) {
		print_sjis(f, ain->enums[i]);
		fputc('\n', f);
	}
}

int main(int argc, char *argv[])
{
	bool dump_version = false;
	bool dump_code = false;
	bool dump_functions = false;
	bool dump_globals = false;
	bool dump_structures = false;
	bool dump_messages = false;
	bool dump_libraries = false;
	bool dump_strings = false;
	bool dump_filenames = false;
	bool dump_functypes = false;
	bool dump_global_group_names = false;
	bool dump_enums = false;
	char *output_file = NULL;
	FILE *output = stdout;
	int err = AIN_SUCCESS;
	struct ain *ain;

	while (1) {
		static struct option long_options[] = {
			{ "help",               no_argument,       0, 'h' },
			{ "ain-version",        no_argument,       0, 'V' },
			{ "code",               no_argument,       0, 'c' },
			{ "functions",          no_argument,       0, 'f' },
			{ "globals",            no_argument,       0, 'g' },
			{ "structures",         no_argument,       0, 'S' },
			{ "messages",           no_argument,       0, 'm' },
			{ "libraries",          no_argument,       0, 'l' },
			{ "strings",            no_argument,       0, 's' },
			{ "filenames",          no_argument,       0, 'F' },
			{ "function-types",     no_argument,       0, 't' },
			{ "global-group-names", no_argument,       0, 'r' },
			{ "enums",              no_argument,       0, 'e' },
			{ "output",             required_argument, 0, 'o' },
		};
		int option_index = 0;
		int c;

		c = getopt_long(argc, argv, "hVcfgSmlsFeo:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'V':
			dump_version = true;
			break;
		case 'c':
			dump_code = true;
			break;
		case 'f':
			dump_functions = true;
			break;
		case 'g':
			dump_globals = true;
			break;
		case 'S':
			dump_structures = true;
			break;
		case 'm':
			dump_messages = true;
			break;
		case 'l':
			dump_libraries = true;
			break;
		case 's':
			dump_strings = true;
			break;
		case 'F':
			dump_filenames = true;
			break;
		case 't':
			dump_functypes = true;
			break;
		case 'G':
			dump_global_group_names = true;
			break;
		case 'e':
			dump_enums = true;
			break;
		case 'o':
			output_file = xstrdup(optarg);
			break;
		case '?':
			ERROR("Unkown command line argument");
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage();
		ERROR("Wrong number of arguments.\n");
		return 1;
	}

	if (output_file) {
		if (!(output = fopen(output_file, "w"))) {
			ERROR("Failed to open output file '%s': %s", output_file, strerror(errno));
		}
		free(output_file);
	}

	if (!(ain = ain_open(argv[0], &err))) {
		ERROR("Failed to open ain file: %s\n", ain_strerror(err));
		return 1;
	}

	if (dump_version)
		ain_dump_version(output, ain);
	if (dump_structures)
		ain_dump_structures(output, ain);
	if (dump_functypes)
		ain_dump_functypes(output, ain);
	if (dump_global_group_names)
		ain_dump_global_group_names(output, ain);
	if (dump_globals)
		ain_dump_globals(output, ain);
	if (dump_libraries)
		ain_dump_libraries(output, ain);
	if (dump_functions)
		ain_dump_functions(output, ain);
	if (dump_messages)
		ain_dump_messages(output, ain);
	if (dump_strings)
		ain_dump_strings(output, ain);
	if (dump_filenames)
		ain_dump_filenames(output, ain);
	if (dump_enums)
		ain_dump_enums(output, ain);
	if (dump_code)
		disassemble_ain(ain, output);

	ain_free(ain);
}