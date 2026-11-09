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
#include "asset_manager.h"
#include "parts.h"
#include "parts_internal.h"

enum { SCROLL_NONE, SCROLL_UP, SCROLL_DOWN, SCROLL_THUMB, SCROLL_TRACK };

static int max_pos(struct parts_scrollbar *s)
{
	return max(s->total - s->view, 0);
}

static int travel(struct parts_scrollbar *s)
{
	return s->length - s->leading - s->trailing - s->thumb_length;
}

static int thumb_offset(struct parts_scrollbar *s)
{
	return s->leading + max((int)(travel(s) * s->rate), 0);
}

static int width(struct parts_scrollbar *s)
{
	return s->horizontal ? s->length : s->thickness;
}

static int height(struct parts_scrollbar *s)
{
	return s->horizontal ? s->thickness : s->length;
}

static void set_pos(struct parts_scrollbar *s, int pos)
{
	pos = min(max(pos, 0), max_pos(s));
	float rate = max_pos(s) ? (float)pos / max_pos(s) : 1.f;
	s->dirty |= s->pos != pos || s->rate != rate;
	s->pos = pos;
	s->rate = rate;
}

static void set_rate(struct parts_scrollbar *s, float rate)
{
	rate = min(max(rate, 0.f), 1.f);
	int pos = (int)(max_pos(s) * rate);
	s->dirty |= s->pos != pos || s->rate != rate;
	s->pos = pos;
	s->rate = rate;
}

static void free_resources(struct parts_scrollbar *s)
{
	for (int b = 0; b < 2; b++)
		for (int i = 0; i < 4; i++)
			gfx_delete_texture(&s->buttons[b][i]);
	for (int i = 0; i < PARTS_NR_STATES; i++)
		gfx_delete_texture(&s->thumb[i]);
	gfx_delete_texture(&s->track);
}

void parts_scrollbar_free(struct parts_scrollbar *s)
{
	free_resources(s);
	free_string(s->cg_name);
	free(s);
}

struct parts_scrollbar *parts_get_scrollbar(struct parts *p, bool horizontal)
{
	enum parts_widget_type type = horizontal ? PARTS_WIDGET_HSCROLLBAR : PARTS_WIDGET_VSCROLLBAR;
	if (p->widget_type == type)
		return p->widget.scrollbar;
	parts_widget_reset(p, type, PARTS_CONSTRUCTION_PROCESS);
	p->state = PARTS_STATE_DEFAULT;
	struct parts_scrollbar *s = p->widget.scrollbar = xcalloc(1, sizeof(*s));
	s->horizontal = horizontal;
	s->length = 256;
	s->thickness = s->leading = s->trailing = 24;
	s->total = 100;
	s->view = s->move = 1;
	s->cg_name = make_string("", 0);
	parts_scrollbar_rebuild(p);
	return s;
}

static void load_cg(Texture *t, struct string *base, const char *suffix)
{
	if (!base->size)
		return;
	struct string *name = string_dup(base);
	string_append_cstr(&name, suffix, strlen(suffix));
	struct cg *cg = asset_cg_load_by_name(name->text, NULL);
	free_string(name);
	if (cg) {
		gfx_init_texture_with_cg(t, cg);
		cg_free(cg);
	}
}

void parts_scrollbar_rebuild(struct parts *p)
{
	static const char *states[] = { "／通常", "／オン", "／ダウン", "／無効" };
	static const char *controls[2][3] = {
		{ "／上ボタン", "／下ボタン", "／バー" },
		{ "／左ボタン", "／右ボタン", "／バー" },
	};
	static const char *track = "／背景";
	static bool converted;
	if (!converted) {
		parts_utf2sjis_array(states, sizeof(states) / sizeof(*states));
		parts_utf2sjis_array(controls[0], 3);
		parts_utf2sjis_array(controls[1], 3);
		parts_utf2sjis_array(&track, 1);
		converted = true;
	}
	struct parts_scrollbar *s = p->widget.scrollbar;
	free_resources(s);
	for (int b = 0; b < 3; b++) {
		const char *control = controls[s->horizontal][b];
		struct string *base = string_dup(s->cg_name);
		string_append_cstr(&base, control, strlen(control));
		for (int i = 0; i < (b == 2 ? PARTS_NR_STATES : 4); i++)
			load_cg(b == 2 ? &s->thumb[i] : &s->buttons[b][i], base, states[i]);
		free_string(base);
	}
	load_cg(&s->track, s->cg_name, track);
	int center = s->length - s->leading - s->trailing;
	int thumb_cg_length = s->horizontal ? s->thumb[0].w : s->thumb[0].h;
	s->thumb_length = !max_pos(s) ? 0 : s->thumb[0].handle ? thumb_cg_length
		: max(16, (int)((float)center / max(s->total, 1) * s->view));
	s->length = max(s->length, s->leading + s->trailing + s->thumb_length);
	for (int i = 0; i < PARTS_NR_STATES; i++)
		parts_set_dims(p, &p->states[i].common, width(s), height(s));
	parts_recalculate_hitbox(p);
	s->dirty = true;
}

static void draw_control(struct parts_scrollbar *s, Texture *dst, Texture *src,
		int offset, int length, int shade)
{
	if (s->thickness <= 0 || length <= 0)
		return;
	int x = s->horizontal ? offset : 0;
	int y = s->horizontal ? 0 : offset;
	if (src->handle)
		gfx_copy_with_alpha_map(dst, x, y, src, 0, 0, src->w, src->h);
	else if (s->horizontal)
		gfx_fill_alpha_color(dst, x, y, length, s->thickness, shade, shade, shade, 255);
	else
		gfx_fill_alpha_color(dst, x, y, s->thickness, length, shade, shade, shade, 255);
}

static void extend(struct parts_scrollbar *s, Texture *t, int offset, int *w, int *h)
{
	if (!t->handle)
		return;
	*w = max(*w, (s->horizontal ? offset : 0) + t->w);
	*h = max(*h, (s->horizontal ? 0 : offset) + t->h);
}

void parts_scrollbar_update(struct parts *p)
{
	struct parts_scrollbar *s = p->widget.scrollbar;
	if (!parts_is_visible(p))
		return;
	if (s->pos != s->notified_pos || s->rate != s->notified_rate) {
		parts_msg_push(p, PARTS_MSG_SCROLL, "ii", s->pos, max_pos(s));
		s->notified_pos = s->pos;
		s->notified_rate = s->rate;
	}
	if (!s->dirty)
		return;
	s->dirty = false;
	// Compose the CG controls into the public part's default surface;
	// hover/press states are per control.
	Texture *dst = &p->states[0].common.texture;
	gfx_delete_texture(dst);
	if (s->thickness <= 0 || s->length <= 0)
		return;
	Texture *buttons[2];
	for (int b = 0; b < 2; b++) {
		int region = b + SCROLL_UP;
		int state = !max_pos(s) ? 3 : s->pressed == region ? 2 : s->hover == region ? 1 : 0;
		buttons[b] = &s->buttons[b][state];
	}
	int state = s->pressed >= SCROLL_THUMB ? 2 : s->hover == SCROLL_THUMB ? 1 : 0;
	Texture *thumb = max_pos(s) ? &s->thumb[state] : NULL;
	// The control CGs can be larger than the scrollbar. They are not clipped.
	int w = width(s), h = height(s);
	extend(s, &s->track, s->leading, &w, &h);
	extend(s, buttons[0], 0, &w, &h);
	extend(s, buttons[1], s->length - s->trailing, &w, &h);
	if (thumb)
		extend(s, thumb, thumb_offset(s), &w, &h);
	gfx_init_texture_rgba(dst, w, h, (SDL_Color){0, 0, 0, 0});
	draw_control(s, dst, &s->track, s->leading, s->length - s->leading - s->trailing, 200);
	draw_control(s, dst, buttons[0], 0, s->leading, 150);
	draw_control(s, dst, buttons[1], s->length - s->trailing, s->trailing, 150);
	if (thumb)
		draw_control(s, dst, thumb, thumb_offset(s), s->thumb_length, 150);
	parts_dirty(p);
}

void parts_scrollbar_mouse(struct parts *p, Point pos, bool down,
		bool trigger, bool hovered, int passed_time)
{
	struct parts_scrollbar *s = p->widget.scrollbar;
	int axis = s->horizontal
		? pos.x - p->global.pos.x - p->states[0].common.origin_offset.x
		: pos.y - p->global.pos.y - p->states[0].common.origin_offset.y;
	int region = SCROLL_NONE;
	if (hovered) {
		int thumb = thumb_offset(s);
		region = axis < s->leading ? SCROLL_UP : axis >= s->length - s->trailing ? SCROLL_DOWN
			: axis >= thumb && axis < thumb + s->thumb_length ? SCROLL_THUMB : SCROLL_TRACK;
	}
	if (region != s->hover) {
		s->hover = region;
		s->dirty = true;
	}
	if (!down || !parts_began_click || !parts_is_visible(p)) {
		if (s->pressed) {
			s->pressed = SCROLL_NONE;
			s->dirty = true;
		}
		return;
	}
	if (trigger && hovered && max_pos(s)) {
		s->pressed = region;
		s->press_axis = axis;
		s->repeat_time = 500;
		if (region == SCROLL_TRACK && travel(s) > 0)
			set_rate(s, (float)(axis - s->leading - s->thumb_length / 2) / travel(s));
		s->press_rate = s->rate;
		if (region == SCROLL_UP || region == SCROLL_DOWN)
			set_pos(s, s->pos + (region == SCROLL_UP ? -s->move : s->move));
		s->dirty = true;
	} else if (s->pressed >= SCROLL_THUMB && travel(s) > 0) {
		set_rate(s, s->press_rate + (float)(axis - s->press_axis) / travel(s));
	} else if (s->pressed == SCROLL_UP || s->pressed == SCROLL_DOWN) {
		s->repeat_time -= max(passed_time, 0);
		if (s->repeat_time < 0) {
			if (hovered && region == s->pressed)
				set_pos(s, s->pos + (s->pressed == SCROLL_UP ? -s->move : s->move));
			s->repeat_time = 80;
		}
	}
}

static struct parts_scrollbar *get_scrollbar(int parts_no, bool horizontal)
{
	return parts_get_scrollbar(parts_get(parts_no), horizontal);
}

static void set_total_size(int parts_no, bool horizontal, int size)
{
	struct parts *p = parts_get(parts_no);
	struct parts_scrollbar *s = parts_get_scrollbar(p, horizontal);
	s->total = size;
	set_pos(s, s->pos);
	parts_scrollbar_rebuild(p);
}

static void set_view_size(int parts_no, bool horizontal, int size)
{
	struct parts *p = parts_get(parts_no);
	struct parts_scrollbar *s = parts_get_scrollbar(p, horizontal);
	s->view = size;
	set_pos(s, s->pos);
	parts_scrollbar_rebuild(p);
}

void PE_SetVScrollbarTotalSize(int parts_no, int size)
{
	set_total_size(parts_no, false, size);
}

void PE_SetVScrollbarViewSize(int parts_no, int size)
{
	set_view_size(parts_no, false, size);
}

void PE_SetVScrollbarScrollPos(int parts_no, int pos)
{
	set_pos(get_scrollbar(parts_no, false), pos);
}

void PE_SetVScrollbarScrollRate(int parts_no, float rate)
{
	set_rate(get_scrollbar(parts_no, false), rate);
}

int PE_GetVScrollbarViewSize(int parts_no)
{
	return get_scrollbar(parts_no, false)->view;
}

int PE_GetVScrollbarScrollPos(int parts_no)
{
	return get_scrollbar(parts_no, false)->pos;
}

float PE_GetVScrollbarScrollRate(int parts_no)
{
	return get_scrollbar(parts_no, false)->rate;
}

int PE_GetVScrollbarMoveSizeByButton(int parts_no)
{
	return get_scrollbar(parts_no, false)->move;
}

void PE_SetHScrollbarTotalSize(int parts_no, int size)
{
	set_total_size(parts_no, true, size);
}

void PE_SetHScrollbarViewSize(int parts_no, int size)
{
	set_view_size(parts_no, true, size);
}

void PE_SetHScrollbarScrollPos(int parts_no, int pos)
{
	set_pos(get_scrollbar(parts_no, true), pos);
}

void PE_SetHScrollbarScrollRate(int parts_no, float rate)
{
	set_rate(get_scrollbar(parts_no, true), rate);
}

int PE_GetHScrollbarViewSize(int parts_no)
{
	return get_scrollbar(parts_no, true)->view;
}

int PE_GetHScrollbarScrollPos(int parts_no)
{
	return get_scrollbar(parts_no, true)->pos;
}

float PE_GetHScrollbarScrollRate(int parts_no)
{
	return get_scrollbar(parts_no, true)->rate;
}

int PE_GetHScrollbarMoveSizeByButton(int parts_no)
{
	return get_scrollbar(parts_no, true)->move;
}
