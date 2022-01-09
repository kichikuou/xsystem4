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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <dirent.h>
#include <limits.h>
#include <assert.h>

#include "system4.h"
#include "system4/ald.h"
#include "system4/afa.h"
#include "system4/cg.h"
#include "system4/file.h"
#include "system4/hashtable.h"
#include "system4/utfsjis.h"

#include "xsystem4.h"
#include "asset_manager.h"

// On 32-bit android, mmap may fail to get a large contiguous virtual address
// range. (See https://stackoverflow.com/questions/30180268)
#if defined(__ANDROID__) && defined(__arm__)
#define ARCHIVE_OPEN_FLAGS 0
#else
#define ARCHIVE_OPEN_FLAGS ARCHIVE_MMAP
#endif

enum archive_type {
	AR_TYPE_ALD,
	AR_TYPE_AFA,
	AR_TYPE_MAX
};

const char *asset_strtype(enum asset_type type)
{
	switch (type) {
	case ASSET_BGM:   return "BGM";
	case ASSET_SOUND: return "Sound";
	case ASSET_VOICE: return "Voice";
	case ASSET_CG:    return "CG";
	case ASSET_FLAT:  return "Flat";
	case ASSET_PACT:  return "Pact";
	case ASSET_DATA:  return "Data";
	}
	return "Invalid";
}

static struct archive *archives[ASSET_TYPE_MAX] = {0};

static void ald_init(enum asset_type type, char **files, int count)
{
	if (count < 1)
		return;
	if (archives[type])
		WARNING("Multiple asset archives for type %s", asset_strtype(type));
	int error = ARCHIVE_SUCCESS;
	archives[type] = ald_open(files, count, ARCHIVE_OPEN_FLAGS, &error);
	if (error)
		ERROR("Failed to open ALD file: %s", archive_strerror(error));
	for (int i = 0; i < count; i++) {
		free(files[i]);
	}
}

static void afa_init(enum asset_type type, char *file)
{
	if (!file)
		return;
	if (archives[type])
		WARNING("Multiple asset archives for type %s", asset_strtype(type));
	int error = ARCHIVE_SUCCESS;
	archives[type] = (struct archive*)afa_open(file, ARCHIVE_OPEN_FLAGS, &error);
	if (error != ARCHIVE_SUCCESS) {
		ERROR("Failed to open AFA file: %s", archive_strerror(error));
	}
	free(file);
}

static char *get_base_name(const char *ain_filename)
{
	char *path = sjis2utf(ain_filename, 0);
	char *dot = strrchr(path, '.');
	if (dot)
		*dot = '\0';
	return path;
}

void asset_manager_init(void)
{
	char *ald_filenames[ASSET_TYPE_MAX][ALD_FILEMAX] = {0};
	int ald_count[ASSET_TYPE_MAX] = {0};
	char *afa_filenames[ASSET_TYPE_MAX] = {0};

	DIR *dir;
	struct dirent *d;

	if (!(dir = opendir(config.game_dir))) {
		ERROR("Failed to open directory: %s", config.game_dir);
	}

	char *base = get_base_name(config.ain_filename);
	size_t base_len = strlen(base);

	// get ALD filenames
	while ((d = readdir(dir))) {
		// archive filename must begin with ain filename
		if (strncmp(base, d->d_name, base_len))
			continue;
		const char *ext = file_extension(d->d_name);
		if (!ext)
			continue;
		if (!strcasecmp(ext, "ald")) {
			int dno = toupper(d->d_name[base_len+1]) - 'A';
			if (dno < 0 || dno >= ALD_FILEMAX) {
				WARNING("Invalid ALD index: %s", d->d_name);
				continue;
			}

			switch (d->d_name[base_len]) {
			case 'b':
			case 'B':
				ald_filenames[ASSET_BGM][dno] = path_join(config.game_dir, d->d_name);
				ald_count[ASSET_BGM] = max(ald_count[ASSET_BGM], dno+1);
				break;
			case 'g':
			case 'G':
				ald_filenames[ASSET_CG][dno] = path_join(config.game_dir, d->d_name);
				ald_count[ASSET_CG] = max(ald_count[ASSET_CG], dno+1);
				break;
			case 'w':
			case 'W':
				ald_filenames[ASSET_SOUND][dno] = path_join(config.game_dir, d->d_name);
				ald_count[ASSET_SOUND] = max(ald_count[ASSET_SOUND], dno+1);
				break;
			case 'd':
			case 'D':
				ald_filenames[ASSET_DATA][dno] = path_join(config.game_dir, d->d_name);
				ald_count[ASSET_DATA] = max(ald_count[ASSET_DATA], dno+1);
				break;
			default:
				WARNING("Unhandled ALD file: %s", d->d_name);
				break;
			}
		} else if (!strcasecmp(ext, "bgi")) {
			if (config.bgi_path) {
				WARNING("Multiple bgi files");
				continue;
			}
			config.bgi_path = path_join(config.game_dir, d->d_name);
		} else if (!strcasecmp(ext, "wai")) {
			if (config.wai_path) {
				WARNING("Multiple wai files");
				continue;
			}
			config.wai_path = path_join(config.game_dir, d->d_name);
		} else if (!strcasecmp(ext, "ex")) {
			if (config.ex_path) {
				WARNING("Multiple ex files");
				continue;
			}
			config.ex_path = path_join(config.game_dir, d->d_name);
		} else if (!strcasecmp(ext, "afa")) {
			const char *type = d->d_name + base_len;
			if (!strcmp(type, "BA.afa")) {
				afa_filenames[ASSET_BGM] = path_join(config.game_dir, d->d_name);
			} else if (!strcmp(type, "WA.afa") || !strcmp(type, "Sound.afa")) {
				afa_filenames[ASSET_SOUND] = path_join(config.game_dir, d->d_name);
			} else if (!strcmp(type, "Voice.afa")) {
				afa_filenames[ASSET_VOICE] = path_join(config.game_dir, d->d_name);
			} else if (!strcmp(type, "CG.afa")) {
				afa_filenames[ASSET_CG] = path_join(config.game_dir, d->d_name);
			} else if (!strcmp(type, "Flat.afa")) {
				afa_filenames[ASSET_FLAT] = path_join(config.game_dir, d->d_name);
			} else if (!strcmp(type, "Pact.afa")) {
				afa_filenames[ASSET_PACT] = path_join(config.game_dir, d->d_name);
			}
		}
	}

	// open ALD archives
	ald_init(ASSET_BGM, ald_filenames[ASSET_BGM], ald_count[ASSET_BGM]);
	ald_init(ASSET_SOUND, ald_filenames[ASSET_SOUND], ald_count[ASSET_SOUND]);
	ald_init(ASSET_VOICE, ald_filenames[ASSET_VOICE], ald_count[ASSET_VOICE]);
	ald_init(ASSET_CG, ald_filenames[ASSET_CG], ald_count[ASSET_CG]);
	ald_init(ASSET_FLAT, ald_filenames[ASSET_FLAT], ald_count[ASSET_FLAT]);
	ald_init(ASSET_PACT, ald_filenames[ASSET_PACT], ald_count[ASSET_PACT]);
	ald_init(ASSET_DATA, ald_filenames[ASSET_DATA], ald_count[ASSET_DATA]);

	// open AFA archives
	afa_init(ASSET_BGM, afa_filenames[ASSET_BGM]);
	afa_init(ASSET_SOUND, afa_filenames[ASSET_SOUND]);
	afa_init(ASSET_VOICE, afa_filenames[ASSET_VOICE]);
	afa_init(ASSET_CG, afa_filenames[ASSET_CG]);
	afa_init(ASSET_FLAT, afa_filenames[ASSET_FLAT]);
	afa_init(ASSET_PACT, afa_filenames[ASSET_PACT]);
	afa_init(ASSET_DATA, afa_filenames[ASSET_DATA]);

	free(base);
	closedir(dir);
}

bool asset_exists(enum asset_type type, int no)
{
	return archives[type] && archive_exists(archives[type], no);
}

struct archive_data *asset_get(enum asset_type type, int no)
{
	if (!archives[type])
		return NULL;
	return archive_get(archives[type], no);
}

static struct hash_table *cg_index = NULL;

// Convert the last sequence of digits in a string to an integer.
static int cg_name_to_int(const char *name)
{
	size_t len = strlen(name);
	int i;
	for (i = len - 1; i >= 0 && !isdigit(name[i]); i--);
	if (i < 0)
		return -1;
	assert(isdigit(name[i]));
	for (; i > 0 && isdigit(name[i-1]); i--);
	assert(isdigit(name[i]));

	if (!(i = atoi(name+i)))
		return -1;
	return i;
}

static void add_cg_to_index(struct archive_data *data, possibly_unused void *_)
{
	int logical_no = cg_name_to_int(data->name);
	if (logical_no < 0) {
		WARNING("Can't determine logical index for CG: %s", data->name);
		return;
	}

	struct ht_slot *slot = ht_put_int(cg_index, logical_no, NULL);
	if (slot->value)
		ERROR("Duplicate CG numbers");
	slot->value = (void*)(uintptr_t)(data->no + 1);
}

/*
 * NOTE: Starting in Shaman's Sanctuary, .afa files are used for CGs but the
 *       library APIs still use integers rather than names to reference them.
 *       Thus it is necessary to parse file names and create an index.
 */
void asset_cg_index_init(void)
{
	if (!archives[ASSET_CG])
		return;

	cg_index = ht_create(4096);
	archive_for_each(archives[ASSET_CG], add_cg_to_index, NULL);
}

static int cg_translate_index(int no)
{
	if (!cg_index)
		return no;
	return (uintptr_t)ht_get_int(cg_index, no, NULL);
}

struct cg *asset_cg_load(int no)
{
	if (!archives[ASSET_CG])
		return NULL;
	if (!(no = cg_translate_index(no)))
		return NULL;
	return cg_load(archives[ASSET_CG], no - 1);
}

bool asset_cg_exists(int no)
{
	if (!archives[ASSET_CG])
		return false;
	if (!(no = cg_translate_index(no)))
		return false;
	return true;
}

bool asset_cg_get_metrics(int no, struct cg_metrics *metrics)
{
	if (!archives[ASSET_CG])
		return false;
	if (!(no = cg_translate_index(no)))
		return NULL;
	return cg_get_metrics(archives[ASSET_CG], no - 1, metrics);
}

