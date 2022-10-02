/* Copyright (C) 2022 kichikuou <KichikuouChrome@gmail.com>
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
#include <string.h>
#include <SDL.h>
#include "system4/utfsjis.h"
#include "trace.h"

static FILE *trace_fp;
static uint64_t performance_frequency;
static char *func_to_trace;
static int nest;

void trace_init(const char *output_path, const char *function_to_trace)
{
	if (trace_fp)
		return;
	performance_frequency = SDL_GetPerformanceFrequency() / 1000000;
	trace_fp = fopen(output_path, "wb");
	fputs("[\n", trace_fp);

	if (function_to_trace) {
		func_to_trace = utf2sjis(function_to_trace, 0);
		nest = -1;
	}
}

void trace_begin(const char *name, const char *cat)
{
	if (!trace_fp)
		return;

	if (func_to_trace) {
		if (nest < 0) {
			if (strcmp(name, func_to_trace))
				return;
			nest = 0;
		}
		nest++;
	}

	uint64_t ts = SDL_GetPerformanceCounter() / performance_frequency;
	fprintf(trace_fp, "{\"name\":\"%s\",\"cat\":\"%s\",\"ph\":\"B\",\"ts\":%llu,\"pid\":1,\"tid\":1},\n", name, cat, ts);
}

void trace_end(void)
{
	if (!trace_fp)
		return;
	if (func_to_trace) {
		if (--nest < 0)
			return;
	}
	uint64_t ts = SDL_GetPerformanceCounter() / performance_frequency;
	fprintf(trace_fp, "{\"ph\":\"E\",\"ts\":%llu,\"pid\":1,\"tid\":1},\n", ts);
}
