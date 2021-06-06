/* Copyright (C) 2021 kichikuou <KichikuouChrome@gmail.com>
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

#include <math.h>
#include <stdlib.h>
#include "system4.h"

#include "dungeon/dgn.h"
#include "dungeon/dungeon.h"
#include "dungeon/map.h"
#include "gfx/gfx.h"
#include "sact.h"
#include "vm.h"
#include "vm/page.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#define CS 12  // pixel size of a map cell
#define LMAP_WIDTH 32
#define LMAP_HEIGHT 32

enum symbol_type {
	SYMBOL_PLAYER_NORTH = 0,
	SYMBOL_PLAYER_SOUTH = 1,
	SYMBOL_PLAYER_EAST = 2,
	SYMBOL_PLAYER_WEST = 3,
	SYMBOL_CROSS = 4,
	SYMBOL_EXIT = 5,
	SYMBOL_STAIRS_UP = 6,
	SYMBOL_STAIRS_DOWN = 7,
	SYMBOL_EVENT = 8,
	SYMBOL_HEART = 9,
	SYMBOL_WARP = 10,
	SYMBOL_TREASURE = 11,
	SYMBOL_MONSTER = 12,
	SYMBOL_YELLOW_STAR = 13,
	NR_SYMBOLS = 14
};

struct dungeon_map {
	struct texture *textures;
	int nr_textures;
	int symbol_sprites[NR_SYMBOLS];
	int small_map_floor;
	int large_map_floor;
	bool radar;
	bool all_visible;
	int *walk_data;
	int offset_x, offset_y;
};

static Point texture_pos(struct dungeon_context *ctx, int x, int z)
{
	return (Point){
		.x = (ctx->map->offset_x + x) * CS,
		.y = (ctx->map->offset_y + ctx->dgn->size_z - z - 1) * CS
	};
}

struct dungeon_map *dungeon_map_create(void)
{
	struct dungeon_map *map = xcalloc(1, sizeof(struct dungeon_map));
	map->small_map_floor = -1;
	map->large_map_floor = -1;
	return map;
}

void dungeon_map_free(struct dungeon_map *map)
{
	for (int i = 0; i < map->nr_textures; i++)
		gfx_delete_texture(&map->textures[i]);
	free(map->textures);
	free(map->walk_data);
	free(map);
}

void dungeon_map_init(struct dungeon_context *ctx)
{
	struct dungeon_map *map = ctx->map;
	struct dgn *dgn = ctx->dgn;

	map->textures = xcalloc(dgn->size_y, sizeof(struct texture));
	map->nr_textures = dgn->size_y;
	map->walk_data = xcalloc(dgn->size_x * dgn->size_y * dgn->size_z, sizeof(int));
	// Calculate the bounding box on the XZ plane so that map is drawn in the
	// center of the texture.
	uint32_t min_x = dgn->size_x;
	uint32_t min_y = dgn->size_z;
	uint32_t max_x = 0;
	uint32_t max_y = 0;
	for (uint32_t y = 0; y < dgn->size_y; y++) {
		for (uint32_t z = 0; z < dgn->size_z; z++) {
			uint32_t map_y = ctx->dgn->size_z - z - 1;
			for (uint32_t x = 0; x < dgn->size_x; x++) {
				if (dgn_cell_at(ctx->dgn, x, y, z)->floor < 0)
					continue;
				min_x = min(min_x, x);
				min_y = min(min_y, map_y);
				max_x = max(max_x, x);
				max_y = max(max_y, map_y);
			}
		}
	}
	map->offset_x = (LMAP_WIDTH - (max_x - min_x + 1)) / 2 - min_x;
	map->offset_y = (LMAP_HEIGHT - (max_y - min_y + 1) + 1) / 2 - min_y;

	// Initial draw
	for (uint32_t y = 0; y < dgn->size_y; y++) {
		gfx_init_texture_rgba(&map->textures[y], LMAP_WIDTH * CS, LMAP_HEIGHT * CS, (SDL_Color){182, 72, 0, 0});
		for (uint32_t z = 0; z < dgn->size_z; z++) {
			for (uint32_t x = 0; x < dgn->size_x; x++) {
				dungeon_map_update_cell(ctx, x, y, z);
			}
		}
	}
}

static void reveal_cell(struct dungeon_context *ctx, int dgn_x, int dgn_y, int dgn_z)
{
	ctx->map->walk_data[dgn_cell_index(ctx->dgn, dgn_x, dgn_y, dgn_z)] = 1;
	struct texture *dst = &ctx->map->textures[dgn_y];
	Point pos = texture_pos(ctx, dgn_x, dgn_z);
	gfx_fill_amap(dst, pos.x, pos.y, CS, CS, 255);
}

static void draw_hline(struct texture *dst, int x, int y, int w, SDL_Color col)
{
	gfx_fill(dst, x, y, w, 1, col.r, col.g, col.b);
}

static void draw_vline(struct texture *dst, int x, int y, int h, SDL_Color col)
{
	gfx_fill(dst, x, y, 1, h, col.r, col.g, col.b);
}

static void draw_symbol(struct dungeon_context *ctx, struct texture *dst, int x, int y, int type)
{
	struct sact_sprite *sp = sact_get_sprite(ctx->map->symbol_sprites[type]);
	if (!sp || sp->rect.w != CS - 2 || sp->rect.h != CS - 2)
		VM_ERROR("Invalid map cg %d", ctx->map->symbol_sprites[type]);
	struct texture *src = sprite_get_texture(sp);
	gfx_copy(dst, x, y, src, 0, 0, src->w, src->h);
}

void dungeon_map_update_cell(struct dungeon_context *ctx, int dgn_x, int dgn_y, int dgn_z)
{
	struct dgn_cell *cell = dgn_cell_at(ctx->dgn, dgn_x, dgn_y, dgn_z);
	struct texture *dst = &ctx->map->textures[dgn_y];
	Point pos = texture_pos(ctx, dgn_x, dgn_z);
	int x = pos.x;
	int y = pos.y;

	if (cell->floor >= 0)
		gfx_fill(dst, x, y, CS, CS, 0, 0, 0);
	else
		gfx_fill(dst, x, y, CS, CS, 182, 72, 0);

	const SDL_Color wall_colors[2] = {{255, 255, 255, 255}, {255, 0, 0, 255}};
	if (cell->north_wall >= 0)
		draw_hline(dst, x, y, CS, wall_colors[!!cell->enterable_north]);
	if (cell->south_wall >= 0)
		draw_hline(dst, x, y + CS - 1, CS, wall_colors[!!cell->enterable_south]);
	if (cell->west_wall >= 0)
		draw_vline(dst, x, y, CS, wall_colors[!!cell->enterable_west]);
	if (cell->east_wall >= 0)
		draw_vline(dst, x + CS - 1, y, CS, wall_colors[!!cell->enterable_east]);

	const SDL_Color door_color = {255, 0, 0, 255};
	if (cell->north_door >= 0) {
		draw_hline(dst, x, y, CS / 2 - 1, door_color);
		draw_hline(dst, x + CS / 2 + 1, y, CS / 2 - 1, door_color);
	}
	if (cell->south_door >= 0) {
		draw_hline(dst, x, y + CS - 1, CS / 2 - 1, door_color);
		draw_hline(dst, x + CS / 2 + 1, y + CS - 1, CS / 2 - 1, door_color);
	}
	if (cell->west_door >= 0) {
		draw_vline(dst, x, y, CS / 2 - 1, door_color);
		draw_vline(dst, x, y + CS / 2 + 1, CS / 2 - 1, door_color);
	}
	if (cell->east_door >= 0) {
		draw_vline(dst, x + CS - 1, y, CS / 2 - 1, door_color);
		draw_vline(dst, x + CS - 1, y + CS / 2 + 1, CS / 2 - 1, door_color);
	}

	const SDL_Color event_wall_color = {64, 255, 128, 255};
	if (cell->north_event)
		draw_hline(dst, x, y, CS, event_wall_color);
	if (cell->south_event)
		draw_hline(dst, x, y + CS - 1, CS, event_wall_color);
	if (cell->west_event)
		draw_vline(dst, x, y, CS, event_wall_color);
	if (cell->east_event)
		draw_vline(dst, x + CS - 1, y, CS, event_wall_color);

	bool draw_all_symbols = ctx->map->radar || ctx->map->all_visible;
	int symbol = -1;
	switch (cell->floor_event) {
	case 11: if (draw_all_symbols) symbol = SYMBOL_TREASURE; break;
	case 12: if (draw_all_symbols) symbol = SYMBOL_MONSTER; break;
	case 14: case 140:             symbol = SYMBOL_EVENT; break;
	case 15:                       symbol = SYMBOL_HEART; break;
	case 16: if (draw_all_symbols) symbol = SYMBOL_CROSS; break;
	case 17:                       symbol = SYMBOL_EXIT; break;
	case 18:                       symbol = SYMBOL_WARP; break;
	case 28: if (draw_all_symbols) symbol = SYMBOL_YELLOW_STAR; break;
	}
	if (cell->stairs_texture >= 0)
		symbol = SYMBOL_STAIRS_UP;
	else if (dgn_y && dgn_cell_at(ctx->dgn, dgn_x, dgn_y - 1, dgn_z)->stairs_texture >= 0)
		symbol = SYMBOL_STAIRS_DOWN;

	if (symbol >= 0)
		draw_symbol(ctx, dst, x + 1, y + 1, symbol);
}

static int player_symbol(struct dungeon_context *ctx)
{
	const int symbols[] = {
		SYMBOL_PLAYER_NORTH,
		SYMBOL_PLAYER_EAST,
		SYMBOL_PLAYER_SOUTH,
		SYMBOL_PLAYER_WEST,
	};
	return symbols[(int)(round(ctx->camera.angle * 2 / M_PI)) & 3];
}

void dungeon_map_draw(int surface, int sprite)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return;
	struct sact_sprite *sp = sact_get_sprite(sprite);
	if (!sp)
		VM_ERROR("DrawDungeon.DrawMap: invalid sprite %d", sprite);
	sprite_dirty(sp);
	struct texture *dst = sprite_get_texture(sp);

	int player_x = round(ctx->camera.pos[0] / 2.0);
	int player_y = round(ctx->camera.pos[1] / 2.0);
	int player_z = round(ctx->camera.pos[2] / -2.0);
	int level = ctx->map->small_map_floor;
	if (level < 0)
		level = player_y;

	struct texture *src = &ctx->map->textures[level];

	Point pos = texture_pos(ctx, player_x, player_z);
	int x = pos.x - (dst->w - CS) / 2;
	int y = pos.y - (dst->h - CS) / 2;

	gfx_copy_with_alpha_map(dst, 0, 0, src, x, y, dst->w, dst->h);

	draw_symbol(ctx, dst, (dst->w - CS + 2) / 2, (dst->h - CS + 2) / 2, player_symbol(ctx));

	if (ctx->map->all_visible)
		gfx_fill_amap(dst, 0, 0, dst->w, dst->h, 255);
}

void dungeon_map_draw_lmap(int surface, int sprite)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return;
	struct sact_sprite *sp = sact_get_sprite(sprite);
	if (!sp)
		VM_ERROR("DrawDungeon.DrawMap: invalid sprite %d", sprite);
	sprite_dirty(sp);
	struct texture *dst = sprite_get_texture(sp);

	int player_x = round(ctx->camera.pos[0] / 2.0);
	int player_y = round(ctx->camera.pos[1] / 2.0);
	int player_z = round(ctx->camera.pos[2] / -2.0);
	int level = ctx->map->large_map_floor;
	if (level < 0)
		level = player_y;

	struct texture *src = &ctx->map->textures[level];
	gfx_copy_with_alpha_map(dst, 0, 0, src, 0, 0, src->w, src->h);

	if (ctx->map->all_visible)
		gfx_fill_amap(dst, 0, 0, dst->w, dst->h, 255);

	Point pos = texture_pos(ctx, player_x, player_z);
	draw_symbol(ctx, dst, pos.x + 1, pos.y + 1, player_symbol(ctx));
}

static void redraw_event_symbols(struct dungeon_context *ctx)
{
	struct dgn *dgn = ctx->dgn;
	for (uint32_t y = 0; y < dgn->size_y; y++) {
		for (uint32_t z = 0; z < dgn->size_z; z++) {
			for (uint32_t x = 0; x < dgn->size_x; x++) {
				if (dgn_cell_at(dgn, x, y, z)->floor_event)
					dungeon_map_update_cell(ctx, x, y, z);
			}
		}
	}
}

void dungeon_map_set_radar(int surface, int flag)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || ctx->map->radar == flag)
		return;
	ctx->map->radar = flag;
	redraw_event_symbols(ctx);
}

void dungeon_map_set_all_view(int surface, int flag)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || ctx->map->all_visible == flag)
		return;
	ctx->map->all_visible = flag;
	redraw_event_symbols(ctx);
}

void dungeon_map_set_cg(int surface, int index, int sprite)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return;
	if (index < 0 || index >= NR_SYMBOLS)
		VM_ERROR("DrawDungeon.SetMapCG: index %d is out of bounds", index);
	ctx->map->symbol_sprites[index] = sprite;
}

void dungeon_map_set_small_map_floor(int surface, int floor)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return;
	ctx->map->small_map_floor = floor;
}

void dungeon_map_set_large_map_floor(int surface, int floor)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return;
	ctx->map->large_map_floor = floor;
}

void dungeon_map_set_walked(int surface, int x, int y, int z, int flag)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return;
	reveal_cell(ctx, x, y, z);
	if (x > 0 && dgn_cell_at(ctx->dgn, x - 1, y, z)->floor < 0)
		reveal_cell(ctx, x - 1, y, z);
	if (x + 1u < ctx->dgn->size_x && dgn_cell_at(ctx->dgn, x + 1, y, z)->floor < 0)
		reveal_cell(ctx, x + 1, y, z);
	if (z > 0 && dgn_cell_at(ctx->dgn, x, y, z - 1)->floor < 0)
		reveal_cell(ctx, x, y, z - 1);
	if (z + 1u < ctx->dgn->size_z && dgn_cell_at(ctx->dgn, x, y, z + 1)->floor < 0)
		reveal_cell(ctx, x, y, z + 1);
}

int dungeon_map_calc_conquer(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return 0;

	int nr_cells = ctx->dgn->size_x * ctx->dgn->size_y * ctx->dgn->size_z;
	int enterable = 0;
	int walked = 0;
	for (int i = 0; i < nr_cells; i++) {
		if (!ctx->dgn->cells[i].enterable)
			continue;
		enterable++;
		if (ctx->map->walk_data[i])
			walked++;
	}
	return walked * 100 / enterable;
}

/*
 * WalkData serialization format (in Rance VI):
 *
 * struct WalkData {
 *     int version;  // zero
 *     int nr_maps;
 *     struct {
 *         int map_no;
 *         int size_x, size_y, size_z;
 *         int cells[size_x * size_y * size_z];
 *     } maps[nr_maps];
 * };
 */

enum {
	WALK_DATA_NOT_FOUND = -1,
	WALK_DATA_BROKEN = -2,
};

static int find_walk_data(struct page *array, int map_no, struct dgn *dgn)
{
	if (!array)
		return WALK_DATA_BROKEN;
	if (array->type != ARRAY_PAGE || array->a_type != AIN_ARRAY_INT || array->rank != 1)
		VM_ERROR("Not a flat integer array");

	int ptr = 0;
	if (array->nr_vars < 2 || array->values[ptr++].i != 0)
		return WALK_DATA_BROKEN;
	int nr_maps = array->values[ptr++].i;

	for (int i = 0; i < nr_maps; i++) {
		if (array->nr_vars < ptr + 4)
			return WALK_DATA_BROKEN;
		int no = array->values[ptr].i;
		uint32_t size_x = array->values[ptr + 1].i;
		uint32_t size_y = array->values[ptr + 2].i;
		uint32_t size_z = array->values[ptr + 3].i;
		int nr_cells = size_x * size_y * size_z;
		if (no == map_no) {
			if (size_x != dgn->size_x || size_y != dgn->size_y || size_z != dgn->size_z)
				return WALK_DATA_BROKEN;
			if (ptr + 4 + nr_cells > array->nr_vars)
				return WALK_DATA_BROKEN;
			return ptr;
		}
		ptr += 4 + nr_cells;
	}
	return WALK_DATA_NOT_FOUND;
}

bool dungeon_map_load_walk_data(int surface, int map, struct page **page)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return false;
	struct page *array = *page;

	int offset = find_walk_data(array, map, ctx->dgn);
	if (offset < 0)
		return false;

	offset += 4;
	int i = 0;
	for (uint32_t z = 0; z < ctx->dgn->size_z; z++) {
		for (uint32_t y = 0; y < ctx->dgn->size_y; y++) {
			for (uint32_t x = 0; x < ctx->dgn->size_x; x++) {
				if (array->values[offset].i)
					reveal_cell(ctx, x, y, z);
				ctx->map->walk_data[i++] = array->values[offset++].i;
			}
		}
	}

	return true;
}

bool dungeon_map_save_walk_data(int surface, int map, struct page **page)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return false;
	struct page *array = *page;
	int nr_cells = ctx->dgn->size_x * ctx->dgn->size_y * ctx->dgn->size_z;

	int offset = find_walk_data(array, map, ctx->dgn);
	if (offset == WALK_DATA_NOT_FOUND) {
		offset = array->nr_vars;
		array->values[1].i += 1;
		union vm_value dim = { .i = array->nr_vars + 4 + nr_cells };
		*page = array = realloc_array(array, 1, &dim, AIN_ARRAY_INT, 0, false);
	} else if (offset == WALK_DATA_BROKEN) {
		if (array)
			WARNING("Discarding broken walk data");
		union vm_value dim = { .i = 2 + 4 + nr_cells };
		*page = array = realloc_array(array, 1, &dim, AIN_ARRAY_INT, 0, false);
		array->values[0].i = 0;
		array->values[1].i = 1;
		offset = 2;
	}
	array->values[offset++].i = map;
	array->values[offset++].i = ctx->dgn->size_x;
	array->values[offset++].i = ctx->dgn->size_y;
	array->values[offset++].i = ctx->dgn->size_z;
	for (int i = 0; i < nr_cells; i++)
		array->values[offset++].i = ctx->map->walk_data[i];
	return true;
}
