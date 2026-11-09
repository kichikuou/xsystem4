/* Copyright (C) 2026 kichikuou <KichikuouChrome@gmail.com>
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

#include <string.h>

#include "system4.h"
#include "system4/cg.h"
#include "system4/string.h"
#include "system4/utfsjis.h"

#include "asset_manager.h"
#include "parts.h"
#include "parts_internal.h"
#include "xsystem4.h"

void parts_button_style_init(struct parts_button_style *s, int w, int h)
{
	*s = (struct parts_button_style) {
		.w = w, .h = h,
		.r = 200, .g = 200, .b = 200,
		.font = {
			.face = FONT_GOTHIC, .size = 16,
			.color = {0, 0, 0, 255}, .edge_color = {0, 0, 0, 255},
			.scale_x = 1, .space_scale_x = 1,
		},
		.cg_name = make_string("", 0),
		.flat_name = make_string("", 0),
		.text = make_string("", 0),
		.text_position = 5,
	};
}

void parts_button_style_free(struct parts_button_style *s)
{
	free_string(s->cg_name);
	free_string(s->flat_name);
	free_string(s->text);
}

static struct string *resource_name(struct string *base, const char *infix, const char *suffix)
{
	struct string *name = string_dup(base);
	string_append_cstr(&name, infix, strlen(infix));
	string_append_cstr(&name, suffix, strlen(suffix));
	return name;
}

// Load the flat or CG named "<base name><`infix`><suffix of `look`>" into
// `state`. If neither exists, reset `state` to an empty construction process.
enum parts_button_resource parts_button_style_load(struct parts *parts, int state,
		struct parts_button_style *s, const char *infix, int look)
{
	static const char *suffixes[] = { "／通常", "／オン", "／ダウン", "／無効" };
	static bool converted;
	if (!converted) {
		parts_utf2sjis_array(suffixes, sizeof(suffixes) / sizeof(*suffixes));
		converted = true;
	}
	struct string *flat = resource_name(s->flat_name, infix, suffixes[look]);
	if (asset_exists_by_name(ASSET_FLAT, flat->text, NULL)) {
		parts_state_reset(&parts->states[state], PARTS_FLAT);
		parts_flat_load(parts, &parts->states[state].flat, flat);
		free_string(flat);
		return PARTS_BUTTON_FLAT;
	}
	free_string(flat);
	struct string *name = resource_name(s->cg_name, infix, suffixes[look]);
	struct cg *cg = asset_cg_load_by_name(name->text, NULL);
	free_string(name);
	parts_state_reset(&parts->states[state], PARTS_CONSTRUCTION_PROCESS);
	if (!cg)
		return PARTS_BUTTON_GENERATED;
	struct parts_common *c = &parts->states[state].common;
	gfx_init_texture_with_cg(&c->texture, cg);
	parts_set_dims(parts, c, cg->metrics.w, cg->metrics.h);
	cg_free(cg);
	return PARTS_BUTTON_CG;
}

static SDL_Color button_color(struct parts_button_style *s, int offset)
{
	return (SDL_Color) {
		min(max(s->r + offset, 0), 255), min(max(s->g + offset, 0), 255),
		min(max(s->b + offset, 0), 255), 255
	};
}

static void button_background(Texture *t, struct parts_button_style *s, int look)
{
	int top = 10, bottom = -10, light = 40, dark = -90;
	if (look == PARTS_BUTTON_LOOK_DISABLED) {
		top = 40; bottom = 30; light = 30; dark = 40;
	} else if (look == PARTS_STATE_HOVERED) {
		top = 40; bottom = 20;
	} else if (look == PARTS_STATE_CLICKED) {
		top = 0; bottom = -20; light = -50; dark = 40;
	}
	SDL_Color c1 = button_color(s, top), c2 = button_color(s, bottom);
	uint8_t *pixels = xcalloc((size_t)s->w * s->h, 4);
	for (int y = 0; y < s->h; y++) {
		int divisor = max(s->h - 1, 1);
		SDL_Color c = {
			c1.r + (c2.r - c1.r) * y / divisor,
			c1.g + (c2.g - c1.g) * y / divisor,
			c1.b + (c2.b - c1.b) * y / divisor, 255
		};
		for (int x = 0; x < s->w; x++) {
			uint8_t *p = pixels + ((size_t)y * s->w + x) * 4;
			p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;
		}
	}
	gfx_init_texture_with_pixels(t, s->w, s->h, pixels);
	free(pixels);
	if (s->w <= 7 || s->h <= 7)
		return;
	c1 = button_color(s, light);
	c2 = button_color(s, dark);
	gfx_fill(t, 0, 0, s->w, 2, c1.r, c1.g, c1.b);
	gfx_fill(t, 0, s->h - 2, s->w, 2, c2.r, c2.g, c2.b);
	gfx_draw_line(t, 0, 2, 0, s->h - 2, c1.r, c1.g, c1.b);
	gfx_draw_line(t, 1, 2, 1, s->h - 3, c1.r, c1.g, c1.b);
	gfx_draw_line(t, s->w - 2, 2, s->w - 2, s->h - 3, c2.r, c2.g, c2.b);
	gfx_draw_line(t, s->w - 1, 1, s->w - 1, s->h - 3, c2.r, c2.g, c2.b);
}

void parts_button_style_draw_background(struct parts *parts, int state,
		struct parts_button_style *s, int look)
{
	struct parts_common *c = &parts->states[state].common;
	if (s->w <= 0 || s->h <= 0) {
		parts_set_dims(parts, c, 0, 0);
		return;
	}
	button_background(&c->texture, s, look);
	parts_set_dims(parts, c, s->w, s->h);
}

void parts_button_style_measure_text(struct parts_button_style *s, int *width, int *height,
		int *line_height)
{
	// Keep fractional advances until the end of the line, as the renderer
	// does. FNL glyph widths can be fractional after downscaling.
	float w = 0, line_w = 0;
	int font_size = s->font.size;
	int decoration = min(font_size, (int)ceilf(max(s->font.bold_width, s->font.edge_up)));
	int line_h = font_size + (font_size & 1) + decoration * 2;
	int h = line_h;
	for (const char *p = s->text->text; *p; p = sjis_skip_char(p)) {
		if (*p == '\n') {
			w = max(w, line_w);
			line_w = 0;
			h += line_h + s->line_space;
		} else {
			line_w += gfx_size_char(&s->font, p) * config.text_x_scale + s->font.font_spacing;
		}
	}
	w = max(w, line_w) - s->font.font_spacing;
	h -= s->line_space;
	// Both font backends use a four-pixel glyph border. Rendering scales
	// that border but subtracts its unscaled offset, then rounds the origin.
	int overhang = (int)ceilf(4 * max(config.text_x_scale - 1.f, 0.f)) + 1;
	*width = w > 0 ? (int)ceilf(w) + overhang : 0;
	*height = max(h, 0);
	*line_height = line_h;
}

void parts_button_style_draw_text(Texture *t, struct parts_button_style *s, int x, int y,
		int line_height, bool blend)
{
	struct string *text = string_dup(s->text);
	char *line = text->text;
	for (;;) {
		char *end = strchr(line, '\n');
		if (end)
			*end = 0;
		gfx_render_text(t, x, y, line, &s->font, blend);
		if (!end)
			break;
		line = end + 1;
		y += line_height + s->line_space;
	}
	free_string(text);
}

void parts_button_style_draw_label(Texture *t, struct parts_button_style *s, bool pressed)
{
	if (!t->handle || !s->text->size)
		return;
	int w, h, line_h;
	parts_button_style_measure_text(s, &w, &h, &line_h);
	int x = 0, y = 0;
	if (s->text_position >= 1 && s->text_position <= 9) {
		int col = (s->text_position - 1) % 3;
		int row = (s->text_position - 1) / 3;
		x = col == 0 ? 3 : col == 1 ? (t->w - w - 6) / 2 + 3 : t->w - w - 3;
		y = row == 0 ? 3 : row == 1 ? (t->h - h - 6) / 2 + 3 : t->h - h - 3;
	}
	if (pressed) {
		x++;
		y++;
	}
	parts_button_style_draw_text(t, s, x, y, line_h, true);
}

struct parts_button *parts_get_button(struct parts *parts)
{
	if (parts->widget_type == PARTS_WIDGET_BUTTON)
		return parts->widget.button;
	parts_widget_reset(parts, PARTS_WIDGET_BUTTON, PARTS_CONSTRUCTION_PROCESS);
	struct parts_button *b = parts->widget.button = xcalloc(1, sizeof(*b));
	parts_button_style_init(&b->style, 200, 30);
	b->enabled = true;
	parts_button_rebuild(parts);
	return b;
}

void parts_button_free(struct parts_button *b)
{
	parts_button_style_free(&b->style);
	free(b);
}

void parts_button_rebuild(struct parts *parts)
{
	struct parts_button *b = parts->widget.button;
	for (int i = 0; i < PARTS_NR_STATES; i++) {
		int look = b->enabled ? i : PARTS_BUTTON_LOOK_DISABLED;
		Texture *t = &parts->states[i].common.texture;
		switch (parts_button_style_load(parts, i, &b->style, "", look)) {
		case PARTS_BUTTON_FLAT:
			break;
		case PARTS_BUTTON_CG:
			parts_button_style_draw_label(t, &b->style, false);
			break;
		case PARTS_BUTTON_GENERATED:
			parts_button_style_draw_background(parts, i, &b->style, look);
			parts_button_style_draw_label(t, &b->style,
					b->enabled && i == PARTS_STATE_CLICKED);
			break;
		}
	}
	parts_recalculate_hitbox(parts);
	parts_dirty(parts);
}

void PE_SetButtonSize(int parts_no, int width, int height)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_button *b = parts_get_button(parts);
	if (b->style.w == width && b->style.h == height)
		return;
	b->style.w = width;
	b->style.h = height;
	parts_button_rebuild(parts);
}

void PE_SetButtonEnable(int parts_no, bool enabled)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_button *b = parts_get_button(parts);
	if (b->enabled == enabled)
		return;
	b->enabled = enabled;
	parts_button_rebuild(parts);
}

void PE_SetButtonText(int parts_no, struct string *text)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_button *b = parts_get_button(parts);
	if (!strcmp(b->style.text->text, text->text))
		return;
	free_string(b->style.text);
	b->style.text = string_dup(text);
	parts_button_rebuild(parts);
}
