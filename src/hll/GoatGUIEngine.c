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

#include <assert.h>

#include "system4/cg.h"
#include "system4/hashtable.h"
#include "system4/string.h"
#include "system4/utfsjis.h"
#include <cglm/cglm.h>

#include "asset_manager.h"
#include "audio.h"
#include "gfx/gfx.h"
#include "input.h"
#include "queue.h"
#include "scene.h"
#include "xsystem4.h"
#include "hll.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

static inline float deg2rad(float deg)
{
	return deg * (M_PI / 180.0);
}

// NOTE: actual value is +1
enum parts_state_type {
	PARTS_STATE_DEFAULT = 0,
	PARTS_STATE_HOVERED = 1,
	PARTS_STATE_CLICKED = 2,
#define PARTS_NR_STATES 3
};

enum parts_motion_type {
	PARTS_MOTION_POS,
	PARTS_MOTION_ALPHA,
	//PARTS_MOTION_CG,
	//PARTS_MOTION_HGUAGE_RATE,
	//PARTS_MOTION_VGUAGE_RATE,
	//PARTS_MOTION_NUMERAL_NUMBER,
	PARTS_MOTION_MAG_X,
	PARTS_MOTION_MAG_Y,
	PARTS_MOTION_ROTATE_X,
	PARTS_MOTION_ROTATE_Y,
	PARTS_MOTION_ROTATE_Z,
	//PARTS_MOTION_VIBRATION_SIZE,
	//PARTS_MOTION_SOUND,
	PARTS_NR_MOTION_TYPES
};

union parts_motion_param {
	int i;
	float f;
	struct {
		int x;
		int y;
	};
};

struct parts_motion {
	TAILQ_ENTRY(parts_motion) entry;
	enum parts_motion_type type;
	union parts_motion_param begin;
	union parts_motion_param end;
	int begin_time;
	int end_time;
};

struct parts_text_line {
	unsigned height;
};

enum parts_type {
	PARTS_CG,
	PARTS_TEXT,
	PARTS_ANIMATION,
};

struct parts_cg {
	Texture texture;
	int no;
};

struct parts_text {
	Texture texture;
	unsigned nr_lines;
	struct parts_text_line *lines;
	unsigned char_space;
	unsigned line_space;
	Point cursor;
	struct text_metrics tm;
};

struct parts_animation {
	Texture texture;
	unsigned cg_no;
	unsigned nr_frames;
	Texture *frames;
	unsigned frame_time;
	unsigned elapsed;
	unsigned current_frame;
};

struct parts_state {
	enum parts_type type;
	union {
		struct parts_cg cg;
		struct parts_text text;
		struct parts_animation anim;
	};
};

struct parts {
	struct sprite sp;
	enum parts_state_type state;
	struct parts_state states[PARTS_NR_STATES];
	TAILQ_ENTRY(parts) parts_list_entry;
	int no;
	int sprite_deform;
	bool clickable;
	int on_cursor_sound;
	int on_click_sound;
	int origin_mode;
	Rectangle rect;
	// The actual hitbox of the sprite (accounting for origin-mode, scale, etc)
	uint8_t alpha;
	Rectangle pos;
	struct { float x, y; } scale;
	struct { float x, y, z; } rotation;
	TAILQ_HEAD(, parts_motion) motion;
};

static void parts_render(struct sprite *sp);

static struct hash_table *parts_table = NULL;
static TAILQ_HEAD(listhead, parts) parts_list = TAILQ_HEAD_INITIALIZER(parts_list);

// true between calls to BeginClick and EndClick
static bool began_click = false;
// true when the left mouse button was down last update
static bool prev_clicking = false;
// the last (fully) clicked parts number
static int clicked_parts = 0;
// the current (partially) clicked parts number
static int click_down_parts = 0;
// the mouse position at last update
static Point prev_pos = {0};

// true after call to BeginMotion until the motion ends or EndMotion is called
static bool is_motion = false;
// the elapsed time of the current motion
static int motion_t = 0;
// the end time of the current motion
static int motion_end_t = 0;

static bool GoatGUIEngine_Init(void)
{
	parts_table = ht_create(1024);
	return 1;
}

static struct text_metrics default_text_metrics = {
	.color = { .r = 255, .g = 255, .b = 255, .a = 255 },
	.outline_color = { .r = 0, .g = 0, .b = 0, .a = 255 },
	.size = 16,
	.weight = FW_NORMAL,
	.face = FONT_GOTHIC,
	.outline_left = 0,
	.outline_up = 0,
	.outline_right = 0,
	.outline_down = 0,
};

static void parts_init(struct parts *parts)
{
	memset(parts, 0, sizeof(struct parts));
	parts->on_cursor_sound = -1;
	parts->on_click_sound = -1;
	parts->origin_mode = 1;
	parts->scale.x = 1.0;
	parts->scale.y = 1.0;
	parts->rotation.x = 0.0;
	parts->rotation.y = 0.0;
	parts->rotation.z = 0.0;
	parts->sp.z = 0;
	parts->sp.z2 = 2;
	parts->sp.render = parts_render;
	TAILQ_INIT(&parts->motion);
}

static struct parts *parts_alloc(void)
{
	struct parts *parts = xcalloc(1, sizeof(struct parts));
	parts_init(parts);
	return parts;
}

static struct parts *parts_get(int parts_no)
{
	struct ht_slot *slot = ht_put_int(parts_table, parts_no, NULL);
	if (slot->value)
		return slot->value;

	struct parts *parts = parts_alloc();
	parts->no = parts_no;
	slot->value = parts;
	TAILQ_INSERT_HEAD(&parts_list, parts, parts_list_entry);
	return parts;
}

static void parts_state_free(struct parts_state *state)
{
	switch (state->type) {
	case PARTS_CG:
		gfx_delete_texture(&state->cg.texture);
		break;
	case PARTS_TEXT:
		free(state->text.lines);
		break;
	case PARTS_ANIMATION:
		for (unsigned i = 0; i < state->anim.nr_frames; i++) {
			gfx_delete_texture(&state->anim.frames[i]);
		}
		break;
	}
}

static void parts_state_reset(struct parts_state *state, enum parts_type type)
{
	parts_state_free(state);
	state->type = type;
	if (type == PARTS_TEXT) {
		state->text.tm = default_text_metrics;
	}
}

static struct parts_cg *parts_get_cg(struct parts *parts, int state)
{
	if (parts->states[state].type != PARTS_CG) {
		parts_state_reset(&parts->states[state], PARTS_CG);
	}
	return &parts->states[state].cg;
}

static struct parts_text *parts_get_text(struct parts *parts, int state)
{
	if (parts->states[state].type != PARTS_TEXT) {
		parts_state_reset(&parts->states[state], PARTS_TEXT);
	}
	return &parts->states[state].text;
}

static struct parts_animation *parts_get_animation(struct parts *parts, int state)
{
	if (parts->states[state].type != PARTS_ANIMATION) {
		parts_state_reset(&parts->states[state], PARTS_ANIMATION);
	}
	return &parts->states[state].anim;
}

static struct parts_motion *parts_motion_alloc(enum parts_motion_type type, int begin_time, int end_time)
{
	struct parts_motion *motion = xcalloc(1, sizeof(struct parts_motion));
	motion->type = type;
	motion->begin_time = begin_time;
	motion->end_time = end_time;
	return motion;
}

static void parts_motion_free(struct parts_motion *motion)
{
	free(motion);
}

static void parts_clear_motion(struct parts *parts)
{
	while (!TAILQ_EMPTY(&parts->motion)) {
		struct parts_motion *motion = TAILQ_FIRST(&parts->motion);
		TAILQ_REMOVE(&parts->motion, motion, entry);
		parts_motion_free(motion);
	}
}

static void parts_clear(struct parts *parts)
{
	parts_clear_motion(parts);
	scene_unregister_sprite(&parts->sp);
	for (int i = 0; i < PARTS_NR_STATES; i++) {
		parts_state_free(&parts->states[i]);
	}
}

static void parts_release(int parts_no)
{
	struct ht_slot *slot = ht_put_int(parts_table, parts_no, NULL);
	if (!slot->value)
		return;

	struct parts *parts = slot->value;
	parts_clear(parts);
	TAILQ_REMOVE(&parts_list, parts, parts_list_entry);
	free(parts);
	slot->value = NULL;
}

static void parts_release_all(void)
{
	while (!TAILQ_EMPTY(&parts_list)) {
		struct parts *parts = TAILQ_FIRST(&parts_list);
		parts_release(parts->no);
	}
}

static void parts_add_motion(struct parts *parts, struct parts_motion *motion)
{
	struct parts_motion *p;
	TAILQ_FOREACH(p, &parts->motion, entry) {
		if (p->begin_time > motion->begin_time) {
			TAILQ_INSERT_BEFORE(p, motion, entry);
			return;
		}
	}
	TAILQ_INSERT_TAIL(&parts->motion, motion, entry);
	if (motion->end_time > motion_end_t)
		motion_end_t = motion->end_time;
	// FIXME? What happens if we add a motion while is_motion=true?
	//        Should we call parts_update_motion here?
}

static inline float motion_progress(struct parts_motion *m, int t)
{
	return (float)(t - m->begin_time) / (m->end_time - m->begin_time);
}

static int motion_calculate_i(struct parts_motion *m, int t)
{
	if (t >= m->end_time)
		return m->end.i;

	const int delta = m->end.i - m->begin.i;
	const float progress = motion_progress(m, t);
	return m->begin.i + (delta * progress);
}

static float motion_calculate_f(struct parts_motion *m, int t)
{
	if (t >= m->end_time)
		return m->end.f;

	const float delta = m->end.f - m->begin.f;
	const float progress = motion_progress(m, t);
	return m->begin.f + (delta * progress);
}

static Point motion_calculate_point(struct parts_motion *m, int t)
{
	if (t >= m->end_time)
		return (Point) { m->end.x, m->end.y };

	const int delta_x = m->end.x - m->begin.x;
	const int delta_y = m->end.y - m->begin.y;
	const float progress = motion_progress(m, t);
	return (Point) {
		m->begin.x + (delta_x * progress),
		m->begin.y + (delta_y * progress)
	};
}

static void origin_mode_offset(int mode, int w, int h, int *_x, int *_y)
{
	int x, y;
	switch (mode) {
	case 1: x = 0;    y = 0;    break;
	case 2: x = -w/2; y = 0;    break;
	case 3: x = -w;   y = -h/2; break;
	case 4: x = 0;    y = -h/2; break;
	case 5: x = -w/2; y = -h/2; break;
	case 6: x = -w;   y = -h/2; break;
	case 7: x = 0;    y = -h;   break;
	case 8: x = -w/2; y = -h;   break;
	case 9: x = -w;   y = -h;   break;
	default:
		// why...
		x = mode;
		y = (3*h)/4;
		break;
	}

	*_x = x;
	*_y = y;
}

static void origin_mode_translate(mat4 dst, int mode, int w, int h)
{
	int x, y;
	origin_mode_offset(mode, w, h, &x, &y);
	glm_translate(dst, (vec3) { x, y, 0 });
}

static void parts_recalculate_pos(struct parts *parts)
{
	int x, y;
	origin_mode_offset(parts->origin_mode, parts->rect.w, parts->rect.h, &x, &y);
	parts->pos = (Rectangle) {
		.x = parts->rect.x + x,
		.y = parts->rect.y + y,
		.w = parts->rect.w,
		.h = parts->rect.h,
	};
}

static void parts_set_pos(struct parts *parts, Point pos)
{
	parts->rect.x = pos.x;
	parts->rect.y = pos.y;
	parts_recalculate_pos(parts);
	scene_sprite_dirty(&parts->sp);
}

static void parts_set_scale_x(struct parts *parts, float mag)
{
	parts->scale.x = mag;
	parts_recalculate_pos(parts);
	scene_sprite_dirty(&parts->sp);
}

static void parts_set_scale_y(struct parts *parts, float mag)
{
	parts->scale.y = mag;
	parts_recalculate_pos(parts);
	scene_sprite_dirty(&parts->sp);
}

static void parts_set_rotation_z(struct parts *parts, float rot)
{
	parts->rotation.z = rot;
	scene_sprite_dirty(&parts->sp);
}

static void parts_set_alpha(struct parts *parts, int alpha)
{
	parts->alpha = max(0, min(255, alpha));
	for (int i = 0; i < PARTS_NR_STATES; i++) {
		struct parts_state *s = &parts->states[i];
		switch (s->type) {
		case PARTS_CG:
			s->cg.texture.alpha_mod = parts->alpha;
			break;
		case PARTS_TEXT:
			s->text.texture.alpha_mod = parts->alpha;
			break;
		case PARTS_ANIMATION:
			for (unsigned i = 0; i < s->anim.nr_frames; i++) {
				s->anim.frames[i].alpha_mod = parts->alpha;
			}
			break;
		}
	}
	scene_sprite_dirty(&parts->sp);
}

static void parts_set_state(struct parts *parts, enum parts_state_type state)
{
	if (parts->state != state) {
		parts->state = state;
		scene_sprite_dirty(&parts->sp);
	}
}

static void parts_render_text(struct parts *parts)
{
	gfx_render_texture(&parts->states[parts->state].text.texture, &parts->rect);
}

static void parts_render_cg(struct parts *parts, Texture *t)
{
	Rectangle *d = &parts->rect;
	mat4 mw_transform = GLM_MAT4_IDENTITY_INIT;
	glm_translate(mw_transform, (vec3) { d->x, d->y, 0 });
	// FIXME: need perspective for 3D rotate
	//glm_rotate_x(mw_transform, parts->rotation.x, mw_transform);
	//glm_rotate_y(mw_transform, parts->rotation.y, mw_transform);
	glm_rotate_z(mw_transform, parts->rotation.z, mw_transform);
	glm_scale(mw_transform, (vec3){ parts->scale.x, parts->scale.y, 1.0 });
	origin_mode_translate(mw_transform, parts->origin_mode, d->w, d->h);
	//glm_scale(mw_transform, (vec3){ d->w * parts->scale.x, d->h * parts->scale.y, 1.0 });
	glm_scale(mw_transform, (vec3){ d->w, d->h, 1.0 });
	mat4 wv_transform = WV_TRANSFORM(config.view_width, config.view_height);

	struct gfx_render_job job = {
		.shader = NULL, // default shader
		.texture = t->handle,
		.world_transform = mw_transform[0],
		.view_transform = wv_transform[0],
		.data = t,
	};

	gfx_render(&job);
}

static void parts_render(struct sprite *sp)
{
	struct parts *parts = (struct parts*)sp;
	switch (parts->states[parts->state].type) {
	case PARTS_CG:
		parts_render_cg(parts, &parts->states[parts->state].cg.texture);
		break;
	case PARTS_TEXT:
		parts_render_text(parts);
		break;
	case PARTS_ANIMATION:
		parts_render_cg(parts, &parts->states[parts->state].anim.texture);
		break;
	}
}

static void parts_update_with_motion(struct parts *parts, struct parts_motion *motion)
{
	switch (motion->type) {
	case PARTS_MOTION_POS:
		parts_set_pos(parts, motion_calculate_point(motion, motion_t));
		break;
	case PARTS_MOTION_ALPHA:
		parts_set_alpha(parts, motion_calculate_i(motion, motion_t));
		break;
	case PARTS_MOTION_MAG_X:
		parts_set_scale_x(parts, motion_calculate_f(motion, motion_t));
		break;
	case PARTS_MOTION_MAG_Y:
		parts_set_scale_y(parts, motion_calculate_f(motion, motion_t));
		break;
		/*
		  case PARTS_MOTION_ROTATE_X:
		  parts->rotation.x = motion_calculate_f(motion, motion_t);
		  sprite_dirty(&parts->sp);
		  break;
		  case PARTS_MOTION_ROTATE_Y:
		  parts->rotation.y = motion_calculate_f(motion, motion_t);
		  sprite_dirty(&parts->sp);
		  break;
		*/
	case PARTS_MOTION_ROTATE_Z:
		parts_set_rotation_z(parts, motion_calculate_f(motion, motion_t));
		break;
	default:
		WARNING("Invalid motion type: %d", motion->type);
	}
}

static void parts_update_all_motion(void)
{
	struct parts *parts;
	TAILQ_FOREACH(parts, &parts_list, parts_list_entry) {
		struct parts_motion *motion;
		TAILQ_FOREACH(motion, &parts->motion, entry) {
			if (motion->begin_time > motion_t)
				break;
			// FIXME? What if a motion begins and ends within the span of another?
			//        This implementation will cancel the earlier motion and remain
			//        at the end-state of the second motion.
			parts_update_with_motion(parts, motion);
		}
	}
}

/*
 * NOTE: If a motion begins at e.g. t=100 with a value of v=0, then that value
 *       becomes the initial value of v at t=0.
 */
static void parts_init_all_motion(void)
{
	struct parts *parts;
	TAILQ_FOREACH(parts, &parts_list, parts_list_entry) {
		struct parts_motion *motion;
		bool initialized[PARTS_NR_MOTION_TYPES] = {0};
		TAILQ_FOREACH(motion, &parts->motion, entry) {
			if (!initialized[motion->type]) {
				parts_update_with_motion(parts, motion);
				initialized[motion->type] = true;
			}
		}
	}
}

static void parts_fini_all_motion(void)
{
	struct parts *parts;
	TAILQ_FOREACH(parts, &parts_list, parts_list_entry) {
		parts_clear_motion(parts);
	}
}

static void parts_update_loop(struct parts *parts, int passed_time)
{
	if (parts->states[parts->state].type != PARTS_ANIMATION)
		return;

	struct parts_animation *anim = &parts->states[parts->state].anim;
	if (passed_time <= 0 || !anim->nr_frames)
		return;

	const unsigned elapsed = anim->elapsed + passed_time;
	const unsigned frame_diff = elapsed / anim->frame_time;
	const unsigned remainder = elapsed % anim->frame_time;

	if (frame_diff > 0) {
		anim->elapsed = remainder;
		anim->current_frame = (anim->current_frame + frame_diff) % anim->nr_frames;
		anim->texture = anim->frames[anim->current_frame];
		scene_sprite_dirty(&parts->sp);
	} else {
		anim->elapsed = elapsed;
	}
}

static void parts_update_mouse(struct parts *parts, Point cur_pos, bool cur_clicking)
{
	if (!began_click || !parts->clickable)
		return;

	bool prev_in = SDL_PointInRect(&prev_pos, &parts->pos);
	bool cur_in = SDL_PointInRect(&cur_pos, &parts->pos);

	if (!cur_in) {
		parts_set_state(parts, PARTS_STATE_DEFAULT);
		return;
	}

	// click down: just remember the parts number for later
	if (cur_clicking && !prev_clicking) {
		click_down_parts = parts->no;
	}

	if (cur_clicking && click_down_parts == parts->no) {
		parts_set_state(parts, PARTS_STATE_CLICKED);
	} else {
		if (!prev_in) {
			audio_play_sound(parts->on_cursor_sound);
		}
		parts_set_state(parts, PARTS_STATE_HOVERED);
	}

	// click event: only if the click down event had same parts number
	if (prev_clicking && !cur_clicking && click_down_parts == parts->no) {
		audio_play_sound(parts->on_click_sound);
		clicked_parts = parts->no;
	}
}

static void GoatGUIEngine_Update(int passed_time, possibly_unused bool message_window_show)
{
	audio_update();

	Point cur_pos;
	bool cur_clicking = key_is_down(VK_LBUTTON);
	mouse_get_pos(&cur_pos.x, &cur_pos.y);

	struct parts *parts;
	TAILQ_FOREACH(parts, &parts_list, parts_list_entry) {
		parts_update_loop(parts, passed_time);
		parts_update_mouse(parts, cur_pos, cur_clicking);
	}

	if (prev_clicking && !cur_clicking) {
		if (!click_down_parts) {
			// TODO: play misclick sound
		}
		click_down_parts = 0;
	}

	prev_clicking = cur_clicking;
	prev_pos = cur_pos;
}

static void parts_add_cg(struct parts *parts, struct cg *cg)
{
	if (parts->rect.w && parts->rect.w != cg->metrics.w)
		WARNING("Width of parts CGs differ: %d / %d", parts->rect.w, cg->metrics.w);
	if (parts->rect.h && parts->rect.h != cg->metrics.h)
		WARNING("Heights of parts CGs differ: %d / %d", parts->rect.h, cg->metrics.h);
	parts->rect.w = cg->metrics.w;
	parts->rect.h = cg->metrics.h;
	parts->sp.has_pixel = true;
	parts->sp.has_alpha = cg->metrics.has_alpha;
}

static bool GoatGUIEngine_SetPartsCG(int parts_no, int cg_no, possibly_unused int sprite_deform, int state)
{
	state--;
	if (state < 0 || state > 2) {
		WARNING("Invalid parts state: %d", state);
		return false;
	}

	struct parts *parts = parts_get(parts_no);
	struct cg *cg = asset_cg_load(cg_no);
	if (!cg)
		return false;
	struct parts_cg *parts_cg = parts_get_cg(parts, state);
	gfx_delete_texture(&parts_cg->texture);
	gfx_init_texture_with_cg(&parts_cg->texture, cg);
	parts_cg->no = cg_no;
	parts_add_cg(parts, cg);
	parts_recalculate_pos(parts);
	scene_sprite_dirty(&parts->sp);
	cg_free(cg);
	return true;
}

static int GoatGUIEngine_GetPartsCGNumber(int parts_no, int state)
{
	state--;
	if (state < 0 || state > 2) {
		WARNING("Invalid parts state: %d", state);
		return false;
	}

	return parts_get_cg(parts_get(parts_no), state)->no;
}

static bool GoatGUIEngine_SetLoopCG(int parts_no, int cg_no, int nr_frames, int frame_time, int state)
{
	state--;
	if (state < 0 || state > 2) {
		WARNING("Invalid parts state: %d", state);
		return false;
	}
	if (nr_frames <= 0 || nr_frames > 10000) {
		WARNING("Invalid frame count: %d", nr_frames);
	}

	struct parts *parts = parts_get(parts_no);
	Texture *frames = xcalloc(nr_frames, sizeof(Texture));
	for (int i = 0; i < nr_frames; i++) {
		struct cg *cg = asset_cg_load(cg_no + i);
		if (!cg) {
			for (int j = 0; j < i; j++) {
				gfx_delete_texture(&frames[j]);
			}
			free(frames);
			return false;
		}
		gfx_init_texture_with_cg(&frames[i], cg);
		parts_add_cg(parts, cg);
		cg_free(cg);
	}

	struct parts_animation *anim = parts_get_animation(parts, state);
	anim->frames = frames; // free previous value?
	anim->nr_frames = nr_frames;
	anim->frame_time = frame_time;
	anim->elapsed = 0;
	anim->current_frame = 0;
	anim->texture = frames[0];
	return true;
}

static int extract_sjis_char(const char *src, char *dst)
{
	if (SJIS_2BYTE(*src)) {
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = '\0';
		return 2;
	}
	dst[0] = src[0];
	dst[1] = '\0';
	return 1;
}

static void parts_text_newline(struct parts_text *text)
{
	const unsigned height = text->lines[text->nr_lines-1].height;
	text->cursor = POINT(0, text->cursor.y + height + text->line_space);
	text->lines = xrealloc_array(text->lines, text->nr_lines, text->nr_lines+1, sizeof(struct parts_text_line));
	text->nr_lines++;
}

static void parts_text_append(struct parts *parts, struct string *text, int state)
{
	struct parts_text *t = parts_get_text(parts, state);

	if (!t->texture.handle) {
		gfx_init_texture_rgba(&t->texture, config.view_width, config.view_height,
				      (SDL_Color){ 0, 0, 0, 0 });
	}

	if (!t->nr_lines) {
		t->lines = xcalloc(1, sizeof(struct parts_text_line));
		t->lines[0].height = 0;
		t->nr_lines = 1;
	}

	const char *msgp = text->text;
	while (*msgp) {
		char c[4];
		int len = extract_sjis_char(msgp, c);
		msgp += len;

		if (c[0] == '\n') {
			parts_text_newline(t);
			continue;
		}

		t->cursor.x += gfx_render_text(&t->texture, t->cursor, c, &t->tm, t->char_space);

		const unsigned old_height = t->lines[t->nr_lines-1].height;
		const unsigned new_height = t->tm.size;
		t->lines[t->nr_lines-1].height = max(old_height, new_height);
	}
}

static void parts_text_clear(struct parts *parts, int state)
{
	struct parts_text *text = parts_get_text(parts, state);
	free(text->lines);
	text->lines = NULL;
	text->nr_lines = 0;
	gfx_delete_texture(&text->texture);
}

static bool GoatGUIEngine_SetText(int parts_no, struct string *text, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	struct parts *parts = parts_get(parts_no);
	parts_text_clear(parts, state);
	parts_text_append(parts, text, state);
	return true;
}

static bool GoatGUIEngine_AddPartsText(int parts_no, struct string *text, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	struct parts *parts = parts_get(parts_no);
	parts_text_append(parts, text, state);
	return true;
}

bool GoatGUIEngine_DeletePartsTopTextLine(int PartsNumber, int State);

static bool GoatGUIEngine_SetFont(int parts_no, int type, int size, int r, int g, int b, float bold_weight, int edge_r, int edge_g, int edge_b, float edge_weight, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	struct parts_text *text = parts_get_text(parts_get(parts_no), state);
	text->tm.face = type;
	text->tm.size = size;
	text->tm.color = (SDL_Color) { r, g, b, 255 };
	text->tm.weight = bold_weight * 1000;
	text->tm.outline_color = (SDL_Color) { edge_r, edge_g, edge_b, 255 };
	text->tm.outline_left = edge_weight;
	text->tm.outline_up = edge_weight;
	text->tm.outline_right = edge_weight;
	text->tm.outline_down = edge_weight;
	return true;
}

static bool GoatGUIEngine_SetPartsFontType(int parts_no, int type, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	parts_get_text(parts_get(parts_no), state)->tm.face = type;
	return true;
}

static bool GoatGUIEngine_SetPartsFontSize(int parts_no, int size, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	parts_get_text(parts_get(parts_no), state)->tm.size = size;
	return true;
}

static bool GoatGUIEngine_SetPartsFontColor(int parts_no, int r, int g, int b, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	parts_get_text(parts_get(parts_no), state)->tm.color = (SDL_Color) { r, g, b, 255 };
	return true;
}

static bool GoatGUIEngine_SetPartsFontBoldWeight(int parts_no, float bold_weight, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	parts_get_text(parts_get(parts_no), state)->tm.weight = bold_weight * 1000;
	return true;
}

static bool GoatGUIEngine_SetPartsFontEdgeColor(int parts_no, int r, int g, int b, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	parts_get_text(parts_get(parts_no), state)->tm.outline_color = (SDL_Color) { r, g, b, 255 };
	return true;
}

static bool GoatGUIEngine_SetPartsFontEdgeWeight(int parts_no, float edge_weight, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	struct parts_text *text = parts_get_text(parts_get(parts_no), state);
	text->tm.outline_left = edge_weight;
	text->tm.outline_up = edge_weight;
	text->tm.outline_right = edge_weight;
	text->tm.outline_down = edge_weight;
	return true;
}

static bool GoatGUIEngine_SetTextCharSpace(int parts_no, int char_space, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	parts_get_text(parts_get(parts_no), state)->char_space = char_space;
	return true;
}

static bool GoatGUIEngine_SetTextLineSpace(int parts_no, int line_space, int state)
{
	state--;
	if (state < 0 || state >= PARTS_NR_STATES)
		return false;

	parts_get_text(parts_get(parts_no), state)->line_space = line_space;
	return true;
}

bool GoatGUIEngine_SetHGaugeCG(int PartsNumber, int CGNumber, int State);
bool GoatGUIEngine_SetHGaugeRate(int PartsNumber, int Numerator, int Denominator, int State);
bool GoatGUIEngine_SetVGaugeCG(int PartsNumber, int CGNumber, int State);
bool GoatGUIEngine_SetVGaugeRate(int PartsNumber, int Numerator, int Denominator, int State);
bool GoatGUIEngine_SetNumeralCG(int PartsNumber, int CGNumber, int State);
bool GoatGUIEngine_SetNumeralLinkedCGNumberWidthWidthList(int PartsNumber, int CGNumber, int Width0, int Width1, int Width2, int Width3, int Width4, int Width5, int Width6, int Width7, int Width8, int Width9, int WidthMinus, int WidthComma, int State);
bool GoatGUIEngine_SetNumeralNumber(int PartsNumber, int Number, int State);
bool GoatGUIEngine_SetNumeralShowComma(int PartsNumber, bool ShowComma, int State);
bool GoatGUIEngine_SetNumeralSpace(int PartsNumber, int NumeralSpace, int State);
bool GoatGUIEngine_SetPartsRectangleDetectionSize(int PartsNumber, int Width, int Height, int State);
bool GoatGUIEngine_SetPartsFlash(int PartsNumber, struct string *pIFlashFileName, int State);
bool GoatGUIEngine_IsPartsFlashEnd(int PartsNumber, int State);
int GoatGUIEngine_GetPartsFlashCurrentFrameNumber(int PartsNumber, int State);
bool GoatGUIEngine_BackPartsFlashBeginFrame(int PartsNumber, int State);
bool GoatGUIEngine_StepPartsFlashFinalFrame(int PartsNumber, int State);

static void GoatGUIEngine_ReleaseParts(int parts_no)
{
	parts_release(parts_no);
}

static void GoatGUIEngine_ReleaseAllParts(void)
{
	parts_release_all();
}

static void GoatGUIEngine_ReleaseAllPartsWithoutSystem(void)
{
	// FIXME: what's the difference?
	parts_release_all();
}

static void GoatGUIEngine_SetPos(int parts_no, int x, int y)
{
	parts_set_pos(parts_get(parts_no), (Point){ x, y });
}

/*
 * NOTE: If a ChipmunkSpriteEngine sprite is at Z=0, it's drawn behind;
 *       otherwise it's drawn in front, regardless of the parts Z-value.
 *       This is implemented by giving GoatGUIEngine sprites a Z-value of
 *       0 and a secondary Z-value of 1 + the value given here.
 */
static void GoatGUIEngine_SetZ(int parts_no, int z)
{
	scene_set_sprite_z2(&parts_get(parts_no)->sp, 0, z+1);
}

static void GoatGUIEngine_SetShow(int parts_no, bool show)
{
	scene_set_sprite_show(&parts_get(parts_no)->sp, show);
}

static void GoatGUIEngine_SetAlpha(int parts_no, int alpha)
{
	parts_set_alpha(parts_get(parts_no), alpha);
}

void GoatGUIEngine_SetPartsDrawFilter(int PartsNumber, int DrawFilter);

static void GoatGUIEngine_SetClickable(int parts_no, bool clickable)
{
	parts_get(parts_no)->clickable = !!clickable;
}

static int GoatGUIEngine_GetPartsX(int parts_no)
{
	return parts_get(parts_no)->rect.x;
}

static int GoatGUIEngine_GetPartsY(int parts_no)
{
	return parts_get(parts_no)->rect.y;
}

static int GoatGUIEngine_GetPartsZ(int parts_no)
{
	return parts_get(parts_no)->sp.z2 - 1;
}

static bool GoatGUIEngine_GetPartsShow(int parts_no)
{
	return scene_get_sprite_show(&parts_get(parts_no)->sp);
}

static int GoatGUIEngine_GetPartsAlpha(int parts_no)
{
	return parts_get(parts_no)->alpha;
}

static bool GoatGUIEngine_GetPartsClickable(int parts_no)
{
	return parts_get(parts_no)->clickable;
}

static void GoatGUIEngine_SetPartsOriginPosMode(int parts_no, int origin_pos_mode)
{
	struct parts *parts = parts_get(parts_no);
	parts->origin_mode = origin_pos_mode;
	parts_recalculate_pos(parts);
	scene_sprite_dirty(&parts->sp);
}

static int GoatGUIEngine_GetPartsOriginPosMode(int parts_no)
{
	return parts_get(parts_no)->origin_mode;
}

void GoatGUIEngine_SetParentPartsNumber(int PartsNumber, int ParentPartsNumber);

static bool GoatGUIEngine_SetPartsGroupNumber(int PartsNumber, int GroupNumber)
{
	// TODO
	return true;
}

static void GoatGUIEngine_SetPartsGroupDecideOnCursor(int GroupNumber, bool DecideOnCursor)
{
	// TODO
}

static void GoatGUIEngine_SetPartsGroupDecideClick(int GroupNumber, bool DecideClick)
{
	// TODO
}

void GoatGUIEngine_SetOnCursorShowLinkPartsNumber(int PartsNumber, int LinkPartsNumber);

static void GoatGUIEngine_SetPartsMessageWindowShowLink(int parts_no, bool message_window_show_link)
{
	// TODO
}

bool GoatGUIEngine_GetPartsMessageWindowShowLink(int PartsNumber);

static bool GoatGUIEngine_SetPartsOnCursorSoundNumber(int parts_no, int sound_no)
{
	if (!asset_exists(ASSET_SOUND, sound_no-1)) {
		WARNING("Invalid sound number: %d", sound_no);
		return false;
	}

	struct parts *parts = parts_get(parts_no);
	parts->on_cursor_sound = sound_no-1;
	return true;
}

static bool GoatGUIEngine_SetPartsClickSoundNumber(int parts_no, int sound_no)
{
	if (!asset_exists(ASSET_SOUND, sound_no-1)) {
		WARNING("Invalid sound number: %d", sound_no);
		return false;
	}

	struct parts *parts = parts_get(parts_no);
	parts->on_click_sound = sound_no-1;
	return true;
}

static bool GoatGUIEngine_SetClickMissSoundNumber(int sound_no)
{
	// TODO
	return true;
}

static void GoatGUIEngine_BeginClick(void)
{
	began_click = true;
}

static void GoatGUIEngine_EndClick(void)
{
	began_click = false;
	clicked_parts = 0;
}

static int GoatGUIEngine_GetClickPartsNumber(void)
{
	return clicked_parts;
}

static void GoatGUIEngine_AddMotionPos(int parts_no, int begin_x, int begin_y, int end_x, int end_y, int begin_t, int end_t)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_motion *motion = parts_motion_alloc(PARTS_MOTION_POS, begin_t, end_t);
	motion->begin.x = begin_x;
	motion->begin.y = begin_y;
	motion->end.x = end_x;
	motion->end.y = end_y;
	parts_add_motion(parts, motion);
}

static void GoatGUIEngine_AddMotionAlpha(int parts_no, int begin_a, int end_a, int begin_t, int end_t)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_motion *motion = parts_motion_alloc(PARTS_MOTION_ALPHA, begin_t, end_t);
	motion->begin.i = begin_a;
	motion->end.i = end_a;
	parts_add_motion(parts, motion);
}

void GoatGUIEngine_AddMotionCG(int PartsNumber, int BeginCGNumber, int NumofCG, int BeginTime, int EndTime);
void GoatGUIEngine_AddMotionHGaugeRate(int PartsNumber, int BeginNumerator, int BeginDenominator, int EndNumerator, int EndDenominator, int BeginTime, int EndTime);
void GoatGUIEngine_AddMotionVGaugeRate(int PartsNumber, int BeginNumerator, int BeginDenominator, int EndNumerator, int EndDenominator, int BeginTime, int EndTime);
void GoatGUIEngine_AddMotionNumeralNumber(int PartsNumber, int BeginNumber, int EndNumber, int BeginTime, int EndTime);

static void GoatGUIEngine_AddMotionMagX(int parts_no, float begin, float end, int begin_t, int end_t)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_motion *motion = parts_motion_alloc(PARTS_MOTION_MAG_X, begin_t, end_t);
	motion->begin.f = begin;
	motion->end.f = end;
	parts_add_motion(parts, motion);
}

static void GoatGUIEngine_AddMotionMagY(int parts_no, float begin, float end, int begin_t, int end_t)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_motion *motion = parts_motion_alloc(PARTS_MOTION_MAG_Y, begin_t, end_t);
	motion->begin.f = begin;
	motion->end.f = end;
	parts_add_motion(parts, motion);
}

static void GoatGUIEngine_AddMotionRotateX(int parts_no, float begin, float end, int begin_t, int end_t)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_motion *motion = parts_motion_alloc(PARTS_MOTION_ROTATE_X, begin_t, end_t);
	motion->begin.f = deg2rad(begin);
	motion->end.f = deg2rad(end);
	parts_add_motion(parts, motion);
}

static void GoatGUIEngine_AddMotionRotateY(int parts_no, float begin, float end, int begin_t, int end_t)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_motion *motion = parts_motion_alloc(PARTS_MOTION_ROTATE_Y, begin_t, end_t);
	motion->begin.f = deg2rad(begin);
	motion->end.f = deg2rad(end);
	parts_add_motion(parts, motion);
}

static void GoatGUIEngine_AddMotionRotateZ(int parts_no, float begin, float end, int begin_t, int end_t)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_motion *motion = parts_motion_alloc(PARTS_MOTION_ROTATE_Z, begin_t, end_t);
	motion->begin.f = deg2rad(begin);
	motion->end.f = deg2rad(end);
	parts_add_motion(parts, motion);
}

void GoatGUIEngine_AddMotionVibrationSize(int PartsNumber, int BeginWidth, int BeginHeight, int BeginTime, int EndTime);
void GoatGUIEngine_AddMotionSound(int SoundNumber, int BeginTime);

static void GoatGUIEngine_BeginMotion(void)
{
	if (began_click)
		return;

	// FIXME: starting a motion seems to clear non-default states
	motion_t = 0;
	is_motion = true;
	parts_init_all_motion();
}

static void GoatGUIEngine_EndMotion(void)
{
	if (began_click)
		return;
	motion_t = 0;
	motion_end_t = 0;
	is_motion = false;
	parts_fini_all_motion();
}

static void GoatGUIEngine_SetMotionTime(int t)
{
	if (!is_motion)
		return;
	if (motion_t == t)
		return;
	motion_t = t;
	parts_update_all_motion();
	if (t >= motion_end_t)
		GoatGUIEngine_EndMotion();
}

static bool GoatGUIEngine_IsMotion(void)
{
	return is_motion;
}

static int GoatGUIEngine_GetMotionEndTime(void)
{
	return motion_end_t;
}

static void GoatGUIEngine_SetPartsMagX(int parts_no, float scale_x)
{
	parts_set_scale_x(parts_get(parts_no), scale_x);
}

static void GoatGUIEngine_SetPartsMagY(int parts_no, float scale_y)
{
	parts_set_scale_y(parts_get(parts_no), scale_y);
}

void GoatGUIEngine_SetPartsRotateX(int parts_no, float rot_x);
void GoatGUIEngine_SetPartsRotateY(int parts_no, float rot_y);

static void GoatGUIEngine_SetPartsRotateZ(int parts_no, float rot_z)
{
	parts_set_rotation_z(parts_get(parts_no), rot_z);
}

void GoatGUIEngine_SetPartsAlphaClipperPartsNumber(int PartsNumber, int AlphaClipperPartsNumber);
void GoatGUIEngine_SetPartsPixelDecide(int PartsNumber, bool PixelDecide);
bool GoatGUIEngine_SetThumbnailReductionSize(int ReductionSize);
bool GoatGUIEngine_SetThumbnailMode(bool Mode);
bool GoatGUIEngine_Save(struct page **SaveDataBuffer);
bool GoatGUIEngine_Load(struct page **SaveDataBuffer);

HLL_LIBRARY(GoatGUIEngine,
	    HLL_EXPORT(Init, GoatGUIEngine_Init),
	    HLL_EXPORT(Update, GoatGUIEngine_Update),
	    HLL_EXPORT(SetPartsCG, GoatGUIEngine_SetPartsCG),
	    HLL_EXPORT(GetPartsCGNumber, GoatGUIEngine_GetPartsCGNumber),
	    HLL_EXPORT(SetLoopCG, GoatGUIEngine_SetLoopCG),
	    HLL_EXPORT(SetText, GoatGUIEngine_SetText),
	    HLL_EXPORT(AddPartsText, GoatGUIEngine_AddPartsText),
	    HLL_TODO_EXPORT(DeletePartsTopTextLine, GoatGUIEngine_DeletePartsTopTextLine),
	    HLL_EXPORT(SetFont, GoatGUIEngine_SetFont),
	    HLL_EXPORT(SetPartsFontType, GoatGUIEngine_SetPartsFontType),
	    HLL_EXPORT(SetPartsFontSize, GoatGUIEngine_SetPartsFontSize),
	    HLL_EXPORT(SetPartsFontColor, GoatGUIEngine_SetPartsFontColor),
	    HLL_EXPORT(SetPartsFontBoldWeight, GoatGUIEngine_SetPartsFontBoldWeight),
	    HLL_EXPORT(SetPartsFontEdgeColor, GoatGUIEngine_SetPartsFontEdgeColor),
	    HLL_EXPORT(SetPartsFontEdgeWeight, GoatGUIEngine_SetPartsFontEdgeWeight),
	    HLL_EXPORT(SetTextCharSpace, GoatGUIEngine_SetTextCharSpace),
	    HLL_EXPORT(SetTextLineSpace, GoatGUIEngine_SetTextLineSpace),
	    HLL_TODO_EXPORT(SetHGaugeCG, GoatGUIEngine_SetHGaugeCG),
	    HLL_TODO_EXPORT(SetHGaugeRate, GoatGUIEngine_SetHGaugeRate),
	    HLL_TODO_EXPORT(SetVGaugeCG, GoatGUIEngine_SetVGaugeCG),
	    HLL_TODO_EXPORT(SetVGaugeRate, GoatGUIEngine_SetVGaugeRate),
	    HLL_TODO_EXPORT(SetNumeralCG, GoatGUIEngine_SetNumeralCG),
	    HLL_TODO_EXPORT(SetNumeralLinkedCGNumberWidthWidthList, GoatGUIEngine_SetNumeralLinkedCGNumberWidthWidthList),
	    HLL_TODO_EXPORT(SetNumeralNumber, GoatGUIEngine_SetNumeralNumber),
	    HLL_TODO_EXPORT(SetNumeralShowComma, GoatGUIEngine_SetNumeralShowComma),
	    HLL_TODO_EXPORT(SetNumeralSpace, GoatGUIEngine_SetNumeralSpace),
	    HLL_TODO_EXPORT(SetPartsRectangleDetectionSize, GoatGUIEngine_SetPartsRectangleDetectionSize),
	    HLL_TODO_EXPORT(SetPartsFlash, GoatGUIEngine_SetPartsFlash),
	    HLL_TODO_EXPORT(IsPartsFlashEnd, GoatGUIEngine_IsPartsFlashEnd),
	    HLL_TODO_EXPORT(GetPartsFlashCurrentFrameNumber, GoatGUIEngine_GetPartsFlashCurrentFrameNumber),
	    HLL_TODO_EXPORT(BackPartsFlashBeginFrame, GoatGUIEngine_BackPartsFlashBeginFrame),
	    HLL_TODO_EXPORT(StepPartsFlashFinalFrame, GoatGUIEngine_StepPartsFlashFinalFrame),
	    HLL_EXPORT(ReleaseParts, GoatGUIEngine_ReleaseParts),
	    HLL_EXPORT(ReleaseAllParts, GoatGUIEngine_ReleaseAllParts),
	    HLL_EXPORT(ReleaseAllPartsWithoutSystem, GoatGUIEngine_ReleaseAllPartsWithoutSystem),
	    HLL_EXPORT(SetPos, GoatGUIEngine_SetPos),
	    HLL_EXPORT(SetZ, GoatGUIEngine_SetZ),
	    HLL_EXPORT(SetShow, GoatGUIEngine_SetShow),
	    HLL_EXPORT(SetAlpha, GoatGUIEngine_SetAlpha),
	    HLL_TODO_EXPORT(SetPartsDrawFilter, GoatGUIEngine_SetPartsDrawFilter),
	    HLL_EXPORT(SetClickable, GoatGUIEngine_SetClickable),
	    HLL_EXPORT(GetPartsX, GoatGUIEngine_GetPartsX),
	    HLL_EXPORT(GetPartsY, GoatGUIEngine_GetPartsY),
	    HLL_EXPORT(GetPartsZ, GoatGUIEngine_GetPartsZ),
	    HLL_EXPORT(GetPartsShow, GoatGUIEngine_GetPartsShow),
	    HLL_EXPORT(GetPartsAlpha, GoatGUIEngine_GetPartsAlpha),
	    HLL_EXPORT(GetPartsClickable, GoatGUIEngine_GetPartsClickable),
	    HLL_EXPORT(SetPartsOriginPosMode, GoatGUIEngine_SetPartsOriginPosMode),
	    HLL_EXPORT(GetPartsOriginPosMode, GoatGUIEngine_GetPartsOriginPosMode),
	    HLL_TODO_EXPORT(SetParentPartsNumber, GoatGUIEngine_SetParentPartsNumber),
	    HLL_EXPORT(SetPartsGroupNumber, GoatGUIEngine_SetPartsGroupNumber),
	    HLL_EXPORT(SetPartsGroupDecideOnCursor, GoatGUIEngine_SetPartsGroupDecideOnCursor),
	    HLL_EXPORT(SetPartsGroupDecideClick, GoatGUIEngine_SetPartsGroupDecideClick),
	    HLL_TODO_EXPORT(SetOnCursorShowLinkPartsNumber, GoatGUIEngine_SetOnCursorShowLinkPartsNumber),
	    HLL_EXPORT(SetPartsMessageWindowShowLink, GoatGUIEngine_SetPartsMessageWindowShowLink),
	    HLL_TODO_EXPORT(GetPartsMessageWindowShowLink, GoatGUIEngine_GetPartsMessageWindowShowLink),
	    HLL_EXPORT(SetPartsOnCursorSoundNumber, GoatGUIEngine_SetPartsOnCursorSoundNumber),
	    HLL_EXPORT(SetPartsClickSoundNumber, GoatGUIEngine_SetPartsClickSoundNumber),
	    HLL_EXPORT(SetClickMissSoundNumber, GoatGUIEngine_SetClickMissSoundNumber),
	    HLL_EXPORT(BeginClick, GoatGUIEngine_BeginClick),
	    HLL_EXPORT(EndClick, GoatGUIEngine_EndClick),
	    HLL_EXPORT(GetClickPartsNumber, GoatGUIEngine_GetClickPartsNumber),
	    HLL_EXPORT(AddMotionPos, GoatGUIEngine_AddMotionPos),
	    HLL_EXPORT(AddMotionAlpha, GoatGUIEngine_AddMotionAlpha),
	    HLL_TODO_EXPORT(AddMotionCG, GoatGUIEngine_AddMotionCG),
	    HLL_TODO_EXPORT(AddMotionHGaugeRate, GoatGUIEngine_AddMotionHGaugeRate),
	    HLL_TODO_EXPORT(AddMotionVGaugeRate, GoatGUIEngine_AddMotionVGaugeRate),
	    HLL_TODO_EXPORT(AddMotionNumeralNumber, GoatGUIEngine_AddMotionNumeralNumber),
	    HLL_EXPORT(AddMotionMagX, GoatGUIEngine_AddMotionMagX),
	    HLL_EXPORT(AddMotionMagY, GoatGUIEngine_AddMotionMagY),
	    HLL_EXPORT(AddMotionRotateX, GoatGUIEngine_AddMotionRotateX),
	    HLL_EXPORT(AddMotionRotateY, GoatGUIEngine_AddMotionRotateY),
	    HLL_EXPORT(AddMotionRotateZ, GoatGUIEngine_AddMotionRotateZ),
	    HLL_TODO_EXPORT(AddMotionVibrationSize, GoatGUIEngine_AddMotionVibrationSize),
	    HLL_TODO_EXPORT(AddMotionSound, GoatGUIEngine_AddMotionSound),
	    HLL_EXPORT(BeginMotion, GoatGUIEngine_BeginMotion),
	    HLL_EXPORT(EndMotion, GoatGUIEngine_EndMotion),
	    HLL_EXPORT(SetMotionTime, GoatGUIEngine_SetMotionTime),
	    HLL_EXPORT(IsMotion, GoatGUIEngine_IsMotion),
	    HLL_EXPORT(GetMotionEndTime, GoatGUIEngine_GetMotionEndTime),
	    HLL_EXPORT(SetPartsMagX, GoatGUIEngine_SetPartsMagX),
	    HLL_EXPORT(SetPartsMagY, GoatGUIEngine_SetPartsMagY),
	    HLL_TODO_EXPORT(SetPartsRotateX, GoatGUIEngine_SetPartsRotateX),
	    HLL_TODO_EXPORT(SetPartsRotateY, GoatGUIEngine_SetPartsRotateY),
	    HLL_EXPORT(SetPartsRotateZ, GoatGUIEngine_SetPartsRotateZ),
	    HLL_TODO_EXPORT(SetPartsAlphaClipperPartsNumber, GoatGUIEngine_SetPartsAlphaClipperPartsNumber),
	    HLL_TODO_EXPORT(SetPartsPixelDecide, GoatGUIEngine_SetPartsPixelDecide),
	    HLL_TODO_EXPORT(SetThumbnailReductionSize, GoatGUIEngine_SetThumbnailReductionSize),
	    HLL_TODO_EXPORT(SetThumbnailMode, GoatGUIEngine_SetThumbnailMode),
	    HLL_TODO_EXPORT(Save, GoatGUIEngine_Save),
	    HLL_TODO_EXPORT(Load, GoatGUIEngine_Load));

