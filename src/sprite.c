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

#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "system4.h"
#include "system4/cg.h"
#include "system4/file.h"
#include "system4/string.h"
#include "system4/utfsjis.h"

#include "asset_manager.h"
#include "audio.h"
#include "input.h"
#include "gfx/gfx.h"
#include "gfx/font.h"
#include "plugin.h"
#include "sprite.h"
#include "vm.h"
#include "vm/page.h"
#include "xsystem4.h"

bool sact_dirty = true;

LIST_HEAD(listhead, sact_sprite) sprites_with_plugins =
	LIST_HEAD_INITIALIZER(sprites_with_plugins);

void sprite_free(struct sact_sprite *sp)
{
	scene_unregister_sprite(&sp->sp);
	gfx_delete_texture(&sp->texture);
	gfx_delete_texture(&sp->text.texture);
	if (sp->plugin)
		LIST_REMOVE(sp, entry);
	memset(sp, 0, sizeof(struct sact_sprite));
}

static void sprite_init_texture(struct sact_sprite *sp)
{
	if (!sp->texture.handle) {
		if (sp->sp.has_alpha)
			gfx_init_texture_rgba(&sp->texture, sp->rect.w, sp->rect.h, sp->color);
		else
			gfx_init_texture_rgb(&sp->texture, sp->rect.w, sp->rect.h, sp->color);
	}
}

static void sprite_render(struct sprite *_sp)
{
	struct sact_sprite *sp = (struct sact_sprite*)_sp;
	sprite_init_texture(sp);
	gfx_render_texture(&sp->texture, &sp->rect);
	if (sp->text.texture.handle) {
		gfx_render_texture(&sp->text.texture, &sp->rect);
	}
}

struct texture *sprite_get_texture(struct sact_sprite *sp)
{
	sprite_init_texture(sp);
	return &sp->texture;
}

void sprite_set_cg(struct sact_sprite *sp, struct cg *cg)
{
	gfx_delete_texture(&sp->texture);
	gfx_init_texture_with_cg(&sp->texture, cg);
	sp->rect.w = cg->metrics.w;
	sp->rect.h = cg->metrics.h;
	sp->sp.has_pixel = true;
	sp->sp.has_alpha = cg->metrics.has_alpha;
	sp->sp.render = sprite_render;
	sprite_dirty(sp);
}

// XXX: Only used in English Rance VI
void sprite_set_cg_2x(struct sact_sprite *sp, struct cg *cg)
{
	// load CG at 1x
	Texture tmp;
	gfx_init_texture_with_cg(&tmp, cg);

	SDL_Color c = { 0, 0, 0, 255 };
	int w = cg->metrics.w*2;
	int h = cg->metrics.h*2;

	// upscale into sprite texture
	gfx_delete_texture(&sp->texture);
	if (cg->metrics.has_alpha) {
		gfx_init_texture_rgba(&sp->texture, w, h, c);
	} else {
		gfx_init_texture_rgb(&sp->texture, w, h, c);
	}
	gfx_copy_stretch_with_alpha_map(&sp->texture, 0, 0, w, h, &tmp, 0, 0, cg->metrics.w, cg->metrics.h);

	sp->rect.w = w;
	sp->rect.h = h;
	sp->sp.has_pixel = true;
	sp->sp.has_alpha = cg->metrics.has_alpha;
	sp->sp.render = sprite_render;
	sprite_dirty(sp);
	gfx_delete_texture(&tmp);
}

int sprite_set_cg_from_asset(struct sact_sprite *sp, int cg_no)
{
	struct cg *cg = asset_cg_load(cg_no);
	if (!cg)
		return 0;
	sprite_set_cg(sp, cg);
	sp->cg_no = cg_no;
	cg_free(cg);
	return 1;
}

int sprite_set_cg_2x_from_asset(struct sact_sprite *sp, int cg_no)
{
	struct cg *cg = asset_cg_load(cg_no);
	if (!cg)
		return 0;
	sprite_set_cg_2x(sp, cg);
	sp->cg_no = cg_no;
	cg_free(cg);
	return 1;
}

int sprite_set_cg_by_name(struct sact_sprite *sp, const char *name)
{
	struct cg *cg = asset_cg_load_by_name(name, &sp->cg_no);
	if (!cg)
		return 0;
	sprite_set_cg(sp, cg);
	cg_free(cg);
	return 1;
}

int sprite_set_cg_from_file(struct sact_sprite *sp, const char *path)
{
	struct cg *cg = cg_load_file(path);
	if (!cg)
		return 0;
	sprite_set_cg(sp, cg);
	cg_free(cg);
	return 1;
}

int sprite_save_cg(struct sact_sprite *sp, const char *path)
{
	return gfx_save_texture(sprite_get_texture(sp), path, ALCG_QNT);
}

static void _sprite_print(struct sprite *sp)
{
	sprite_print((struct sact_sprite*)sp);
}

/*
 * Attach pixel data to a sprite. The texture is initialized lazily.
 */
void sprite_init(struct sact_sprite *sp, int w, int h, int r, int g, int b, int a)
{
	sp->color = (SDL_Color) { .r = r, .g = g, .b = b, .a = (a >= 0 ? a : 255) };
	sp->rect.w = w;
	sp->rect.h = h;
	gfx_delete_texture(&sp->texture);

	sp->sp.has_pixel = true;
	sp->sp.has_alpha = a >= 0;
	sp->sp.render = sprite_render;
	sp->sp.debug_print = _sprite_print;
	sprite_dirty(sp);
}

void sprite_set_pos(struct sact_sprite *sp, int x, int y)
{
	sp->rect.x = x;
	sp->rect.y = y;
	sprite_dirty(sp);
}

void sprite_set_x(struct sact_sprite *sp, int x)
{
	sp->rect.x = x;
	sprite_dirty(sp);
}

void sprite_set_y(struct sact_sprite *sp, int y)
{
	sp->rect.y = y;
	sprite_dirty(sp);
}

int sprite_get_blend_rate(struct sact_sprite *sp)
{
	return sprite_get_texture(sp)->alpha_mod;
}

void sprite_set_blend_rate(struct sact_sprite *sp, int rate)
{
	rate = max(0, min(255, rate));
	sprite_get_texture(sp)->alpha_mod = rate;
	sp->text.texture.alpha_mod = rate;
	sprite_dirty(sp);
}

int sprite_set_draw_method(struct sact_sprite *sp, enum draw_method method)
{
	sprite_get_texture(sp)->draw_method = method;
	sp->text.texture.draw_method = method;
	sprite_dirty(sp);
	return 1;
}

enum draw_method sprite_get_draw_method(struct sact_sprite *sp)
{
	return sprite_get_texture(sp)->draw_method;
}

void sprite_set_text_home(struct sact_sprite *sp, int x, int y)
{
	sp->text.home = (Point) { .x = x, .y = y };
	sprite_dirty(sp);
}

void sprite_set_text_line_space(struct sact_sprite *sp, int px)
{
	sp->text.line_space = px;
	sprite_dirty(sp);
}

void sprite_set_text_char_space(struct sact_sprite *sp, int px)
{
	sp->text.char_space = px;
	sprite_dirty(sp);
}

void sprite_set_text_pos(struct sact_sprite *sp, int x, int y)
{
	sp->text.pos = (Point) { .x = x, .y = y };
	sprite_dirty(sp);
}

void sprite_text_draw(struct sact_sprite *sp, struct string *text, struct text_style *ts)
{
	if (!sp->text.texture.handle) {
		SDL_Color c;
		if (text_style_has_edge(ts))
			c = ts->edge_color;
		else
			c = ts->color;
		c.a = 0;
		gfx_init_texture_rgba(&sp->text.texture, sp->rect.w, sp->rect.h, c);
		Texture *sp_t = sprite_get_texture(sp);
		sp->text.texture.alpha_mod = sp_t->alpha_mod;
		sp->text.texture.draw_method = sp_t->draw_method;
	}

	ts->font_spacing = sp->text.char_space;
	sp->text.pos.x += gfx_render_text(&sp->text.texture, sp->text.pos.x, sp->text.pos.y, text->text, ts);
	sprite_dirty(sp);
}

void sprite_text_clear(struct sact_sprite *sp)
{
	gfx_delete_texture(&sp->text.texture);
	sprite_dirty(sp);
}

void sprite_text_home(struct sact_sprite *sp, possibly_unused int size)
{
	// FIXME: do something with nTextSize
	sp->text.pos = sp->text.home;
	sprite_dirty(sp);
}

void sprite_text_new_line(struct sact_sprite *sp, int size)
{
	sp->text.pos = POINT(sp->text.home.x, sp->text.pos.y + size + sp->text.line_space);
	sprite_dirty(sp);
}

void sprite_text_copy(struct sact_sprite *dsp, struct sact_sprite *ssp)
{
	sprite_text_clear(dsp);
	if (ssp->text.texture.handle) {
		SDL_Color c = { 0, 0, 0, 0 };
		gfx_init_texture_rgba(&dsp->text.texture, dsp->rect.w, dsp->rect.h, c);
		gfx_copy(&dsp->text.texture, 0, 0, &ssp->text.texture, 0, 0, ssp->text.texture.w, ssp->text.texture.h);
		gfx_copy_amap(&dsp->text.texture, 0, 0, &ssp->text.texture, 0, 0, ssp->text.texture.w, ssp->text.texture.h);
	}
	dsp->text.home = ssp->text.home;
	dsp->text.pos = ssp->text.pos;
	dsp->text.char_space = ssp->text.char_space;
	dsp->text.line_space = ssp->text.line_space;
	sprite_dirty(dsp);
}

bool sprite_is_point_in_rect(struct sact_sprite *sp, int x, int y)
{
	Point p = POINT(x, y);
	return !!SDL_PointInRect(&p, &sp->rect);
}

bool sprite_is_point_in(struct sact_sprite *sp, int x, int y)
{
	if (!sprite_is_point_in_rect(sp, x, y))
		return 0;

	// check alpha
	struct texture *t = sprite_get_texture(sp);
	SDL_Color c = gfx_get_pixel(t, x - sp->rect.x, y - sp->rect.y);
	return !!c.a;
}

int sprite_get_amap_value(struct sact_sprite *sp, int x, int y)
{
	return gfx_get_pixel(sprite_get_texture(sp), x, y).a;
}

void sprite_get_pixel_value(struct sact_sprite *sp, int x, int y, int *r, int *g, int *b)
{
	SDL_Color c = gfx_get_pixel(sprite_get_texture(sp), x, y);
	*r = c.r;
	*g = c.g;
	*b = c.b;
}

void sprite_bind_plugin(struct sact_sprite *sp, struct draw_plugin *plugin)
{
	if (!sp->plugin && plugin)
		LIST_INSERT_HEAD(&sprites_with_plugins, sp, entry);
	else if (sp->plugin && !plugin)
		LIST_REMOVE(sp, entry);
	sp->plugin = plugin;
}

void sprite_call_plugins(void)
{
	struct sact_sprite *sp;
	LIST_FOREACH(sp, &sprites_with_plugins, entry) {
		sp->plugin->update(sp);
	}
}

void gfx_print_color(SDL_Color *c)
{
	printf("(%d,%d,%d,%d)", c->r, c->g, c->b, c->a);
}

void gfx_print_rectangle(Rectangle *r)
{
	printf("{x=%d,y=%d,w=%d,h=%d}", r->x, r->y, r->w, r->h);
}

void gfx_print_point(Point *p)
{
	printf("{x=%d,y=%d}", p->x, p->y);
}

void gfx_print_texture(struct texture *t, int indent)
{
	printf("{\n");
	indent_printf(indent+1, "initialized = %s,\n", t->handle ? "true" : "false");
	indent_printf(indent+1, "size = (%d,%d),\n", t->w, t->h);
	indent_printf(indent+1, "has_alpha = %s,\n", t->has_alpha ? "true" : "false");
	indent_printf(indent+1, "alpha_mod = %d,\n", t->alpha_mod);
	switch (t->draw_method) {
	case DRAW_METHOD_NORMAL:   indent_printf(indent+1, "draw_method = normal\n"); break;
	case DRAW_METHOD_SCREEN:   indent_printf(indent+1, "draw_method = screen\n"); break;
	case DRAW_METHOD_MULTIPLY: indent_printf(indent+1, "draw_method = multiply\n"); break;
	case DRAW_METHOD_ADDITIVE: indent_printf(indent+1, "draw_method = additive\n"); break;
	default:                   indent_printf(indent+1, "draw_method = unknown\n"); break;
	}
	indent_printf(indent, "}");
}

void sprite_print(struct sact_sprite *sp)
{
	int indent = 0;
	indent_printf(indent, "sprite %d = {\n", sp->no);
	indent++;

	indent_printf(indent, "sp = {\n");
	indent++;
	indent_printf(indent, "z = (%d,%d),\n", sp->sp.z, sp->sp.z2);
	indent_printf(indent, "has_pixel = %s,\n", sp->sp.has_pixel ? "true" : "false");
	indent_printf(indent, "has_alpha = %s,\n", sp->sp.has_alpha ? "true" : "false");
	indent_printf(indent, "hidden = %s,\n", sp->sp.hidden ? "true" : "false");
	indent_printf(indent, "in_scene = %s,\n", sp->sp.in_scene ? "true" : "false");
	indent--;
	indent_printf(indent, "},\n");

	indent_printf(indent, "texture = "); gfx_print_texture(&sp->texture, 1); printf(",\n");
	indent_printf(indent, "color = "); gfx_print_color(&sp->color); printf(",\n");
	indent_printf(indent, "rect = "); gfx_print_rectangle(&sp->rect); printf(",\n");

	indent_printf(indent, "text = {\n");
	indent++;
	indent_printf(indent, "texture = "); gfx_print_texture(&sp->text.texture, 2); printf(",\n");
	indent_printf(indent, "home = "); gfx_print_point(&sp->text.home); printf(",\n");
	indent_printf(indent, "pos = "); gfx_print_point(&sp->text.pos); printf(",\n");
	indent_printf(indent, "char_space = %d,\n", sp->text.char_space);
	indent_printf(indent, "line_space = %d,\n", sp->text.line_space);
	if (sp->plugin) {
		if (sp->plugin->debug_print) {
			indent_printf(indent, "plugin = {\n");
			sp->plugin->debug_print(sp, indent + 1);
			indent_printf(indent, "},\n");
		} else {
			indent_printf(indent, "plugin = %s,\n", sp->plugin->name);
		}
	}
	indent--;
	indent_printf(indent, "},\n");

	indent_printf(indent, "cg_no = %d\n", sp->cg_no);
	indent--;

	indent_printf(indent, "}\n");
}
