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
#include <cglm/cglm.h>

#include "system4.h"

#include "gfx/gfx.h"
#include "scene.h"
#include "xsystem4.h"

#include "parts_internal.h"

// Goat sprites are integrated into the scene via a single (virtual) sprite
static struct sprite goat_sprite;

struct parts_render_params {
	Point offset;
	uint8_t alpha;
};

static void parts_render_text(struct parts *parts, struct parts_render_params *params)
{
	parts_recalculate_offset(parts);
	Rectangle rect = {
		.x = params->offset.x + parts->offset.x,
		.y = params->offset.y + parts->offset.y,
		.w = parts->rect.w,
		.h = parts->rect.h
	};
	gfx_render_texture(&parts->states[parts->state].text.texture, &rect);
}

static void parts_render_cg(struct parts *parts, Texture *t, struct parts_render_params *params)
{
	parts_recalculate_offset(parts);

	mat4 mw_transform = GLM_MAT4_IDENTITY_INIT;
	glm_translate(mw_transform, (vec3) { params->offset.x, params->offset.y, 0 });
	// FIXME: need perspective for 3D rotate
	//glm_rotate_x(mw_transform, parts->rotation.x, mw_transform);
	//glm_rotate_y(mw_transform, parts->rotation.y, mw_transform);
	glm_rotate_z(mw_transform, parts->rotation.z, mw_transform);
	glm_scale(mw_transform, (vec3){ parts->scale.x, parts->scale.y, 1.0 });
	glm_translate(mw_transform, (vec3){ parts->offset.x, parts->offset.y, 0 });
	glm_scale(mw_transform, (vec3){ parts->rect.w, parts->rect.h, 1.0 });
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

static void parts_render(struct parts *parts, struct parts_render_params *parent_params)
{
	if (!parts->show)
		return;

	if (parts->linked_to >= 0) {
		struct parts *link_parts = parts_get(parts->linked_to);
		if (!SDL_PointInRect(&parts_prev_pos, &link_parts->pos))
			return;
	}

	struct parts_render_params params = *parent_params;
	// modify params per parts values
	params.alpha *= parts->alpha / 255.0;
	params.offset.x += parts->rect.x;
	params.offset.y += parts->rect.y;

	// render
	struct parts_state *state = &parts->states[parts->state];
	switch (state->type) {
	case PARTS_CG:
		if (state->cg.texture.handle)
			parts_render_cg(parts, &state->cg.texture, &params);
		break;
	case PARTS_TEXT:
		if (state->text.texture.handle)
			parts_render_text(parts, &params);
		break;
	case PARTS_ANIMATION:
		if (state->anim.texture.handle)
			parts_render_cg(parts, &state->anim.texture, &params);
		break;
	case PARTS_NUMERAL:
		if (state->num.texture.handle) {
			parts_render_cg(parts, &state->num.texture, &params);
		}
		break;
	case PARTS_HGAUGE:
	case PARTS_VGAUGE:
		if (state->gauge.texture.handle)
			parts_render_cg(parts, &state->gauge.texture, &params);
		break;
	}

	// render children
	struct parts *child;
	TAILQ_FOREACH(child, &parts->children, parts_list_entry) {
		parts_render(child, &params);
	}
}

void parts_engine_render(possibly_unused struct sprite *_)
{
	struct parts_render_params params = {
		.offset = { 0, 0 },
		.alpha = 255,
	};
	struct parts *parts;
	TAILQ_FOREACH(parts, &parts_list, parts_list_entry) {
		parts_render(parts, &params);
	}
}

void parts_engine_dirty(void)
{
	scene_sprite_dirty(&goat_sprite);
}

void parts_dirty(possibly_unused struct parts *parts)
{
	parts_engine_dirty();
}

void parts_render_init(void)
{
	goat_sprite.z = 0;
	goat_sprite.z2 = 2;
	goat_sprite.has_pixel = true;
	goat_sprite.has_alpha = true;
	goat_sprite.render = parts_engine_render;
	scene_register_sprite(&goat_sprite);
}
