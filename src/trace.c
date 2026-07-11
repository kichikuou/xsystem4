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
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "system4.h"
#include "system4/utfsjis.h"
#include "trace.h"

enum trace_phase {
	TRACE_BEGIN,
	TRACE_END,
};

struct trace_event {
	const char *name;
	const char *cat;
	uint64_t timestamp;
	enum trace_phase phase;
};

// About 24 MiB on 64-bit platforms. Once full, old events are overwritten so
// that a dump always describes the most recent activity.
#define TRACE_BUFFER_CAPACITY (1U << 20)

static char *trace_output_path;
static struct trace_event *trace_buffer;
static size_t trace_head;
static size_t trace_count;
static uint64_t performance_frequency;
static char *func_to_trace;
static int nest;

static void write_json_string(FILE *fp, const char *s)
{
	fputc('"', fp);
	for (; *s; s++) {
		switch (*s) {
		case '"': fputs("\\\"", fp); break;
		case '\\': fputs("\\\\", fp); break;
		case '\b': fputs("\\b", fp); break;
		case '\f': fputs("\\f", fp); break;
		case '\n': fputs("\\n", fp); break;
		case '\r': fputs("\\r", fp); break;
		case '\t': fputs("\\t", fp); break;
		default:
			if ((unsigned char)*s < 0x20)
				fprintf(fp, "\\u%04x", (unsigned char)*s);
			else
				fputc(*s, fp);
		}
	}
	fputc('"', fp);
}

static void write_sjis_json_string(FILE *fp, const char *s)
{
	char *utf8 = sjis2utf(s, 0);
	write_json_string(fp, utf8);
	free(utf8);
}

static void record_event(enum trace_phase phase, const char *name, const char *cat)
{
	size_t index;
	if (trace_count < TRACE_BUFFER_CAPACITY) {
		index = (trace_head + trace_count++) % TRACE_BUFFER_CAPACITY;
	} else {
		index = trace_head;
		trace_head = (trace_head + 1) % TRACE_BUFFER_CAPACITY;
	}
	trace_buffer[index] = (struct trace_event) {
		.name = name,
		.cat = cat,
		.timestamp = SDL_GetPerformanceCounter(),
		.phase = phase,
	};
}

static uint64_t timestamp_us(uint64_t counter)
{
	return counter / performance_frequency * 1000000
		+ counter % performance_frequency * 1000000 / performance_frequency;
}

void trace_init(const char *output_path, const char *function_to_trace)
{
	if (trace_buffer)
		return;
	performance_frequency = SDL_GetPerformanceFrequency();
	trace_output_path = xstrdup(output_path);
	trace_buffer = xcalloc(TRACE_BUFFER_CAPACITY, sizeof(*trace_buffer));
	atexit(trace_dump);

	if (function_to_trace) {
		func_to_trace = utf2sjis(function_to_trace, 0);
	}
}

void trace_begin(const char *name, const char *cat)
{
	if (!trace_buffer)
		return;

	if (func_to_trace) {
		if (nest == 0) {
			if (strcmp(name, func_to_trace))
				return;
		}
		nest++;
	}

	record_event(TRACE_BEGIN, name, cat);
}

void trace_end(void)
{
	if (!trace_buffer)
		return;
	if (func_to_trace) {
		if (nest == 0)
			return;
		nest--;
	}
	record_event(TRACE_END, NULL, NULL);
}

void trace_dump(void)
{
	if (!trace_buffer || !trace_count)
		return;

	FILE *fp = fopen(trace_output_path, "wb");
	if (!fp)
		return;

	fputs("[\n", fp);
	for (size_t i = 0; i < trace_count; i++) {
		struct trace_event *event =
			&trace_buffer[(trace_head + i) % TRACE_BUFFER_CAPACITY];
		fputs("  {", fp);
		if (event->phase == TRACE_BEGIN) {
			fputs("\"name\":", fp);
			write_sjis_json_string(fp, event->name);
			fputs(",\"cat\":", fp);
			write_sjis_json_string(fp, event->cat);
			fputs(",\"ph\":\"B\",", fp);
		} else {
			fputs("\"ph\":\"E\",", fp);
		}
		fprintf(fp, "\"ts\":%llu,\"pid\":1,\"tid\":1}%s\n",
			(unsigned long long)timestamp_us(event->timestamp),
			i + 1 == trace_count ? "" : ",");
	}
	fputs("]\n", fp);
	fclose(fp);
}
