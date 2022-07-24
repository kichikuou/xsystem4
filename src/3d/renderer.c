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

#include <cglm/cglm.h>

#include "system4.h"
#include "system4/cg.h"
#include "system4/hashtable.h"

#include "3d_internal.h"
#include "asset_manager.h"
#include "reign.h"
#include "sact.h"

enum {
	COLOR_TEXTURE_UNIT,
	SPECULAR_TEXTURE_UNIT,
	LIGHT_TEXTURE_UNIT,
	NORMAL_TEXTURE_UNIT,
};

static GLuint load_shader(const char *vertex_shader_path, const char *fragment_shader_path)
{
	GLuint program = glCreateProgram();
	GLuint vertex_shader = gfx_load_shader_file(vertex_shader_path, GL_VERTEX_SHADER);
	GLuint fragment_shader = gfx_load_shader_file(fragment_shader_path, GL_FRAGMENT_SHADER);

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// In OpenGL < 3.2, Attribute location 0 is special. Make sure it's assigned
	// to the vertex position.
	glBindAttribLocation(program, ATTR_VERTEX_POS, "vertex_pos");

	glLinkProgram(program);

	GLint link_success;
	glGetProgramiv(program, GL_LINK_STATUS, &link_success);
	if (!link_success) {
		GLint len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
		char *infolog = xmalloc(len + 1);
		glGetProgramInfoLog(program, len, NULL, infolog);
		ERROR("Failed to link shader %s, %s: %s", vertex_shader_path, fragment_shader_path, infolog);
	}
	return program;
}

static void init_billboard_mesh(struct RE_renderer *r)
{
	static const GLfloat vertices[] = {
		// x,    y,   z,    u,   v
		-1.0,  2.0, 0.0,  0.0, 0.0,
		-1.0,  0.0, 0.0,  0.0, 1.0,
		 1.0,  2.0, 0.0,  1.0, 0.0,
		 1.0,  0.0, 0.0,  1.0, 1.0,
	};
	glGenVertexArrays(1, &r->billboard_vao);
	glBindVertexArray(r->billboard_vao);
	glGenBuffers(1, &r->billboard_attr_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, r->billboard_attr_buffer);

	glEnableVertexAttribArray(ATTR_VERTEX_POS);
	glVertexAttribPointer(ATTR_VERTEX_POS, 3, GL_FLOAT, GL_FALSE, 20, (const void *)0);
	glEnableVertexAttribArray(r->vertex_uv);
	glVertexAttribPointer(r->vertex_uv, 2, GL_FLOAT, GL_FALSE, 20, (const void *)12);
	glDisableVertexAttribArray(r->vertex_light_uv);
	glVertexAttrib2f(r->vertex_light_uv, 0.0, 0.0);
	glDisableVertexAttribArray(r->vertex_normal);
	glVertexAttrib3f(r->vertex_normal, 0.0, 0.0, 1.0);
	glDisableVertexAttribArray(r->vertex_tangent);
	glVertexAttrib3f(r->vertex_tangent, 1.0, 0.0, 0.0);
	glDisableVertexAttribArray(r->vertex_bone_index);
	glVertexAttribI4i(r->vertex_bone_index, 0, 0, 0, 0);
	glDisableVertexAttribArray(r->vertex_bone_weight);
	glVertexAttrib4f(r->vertex_bone_weight, 0.0, 0.0, 0.0, 0.0);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

struct RE_renderer *RE_renderer_new(struct texture *texture)
{
	struct RE_renderer *r = xcalloc(1, sizeof(struct RE_renderer));

	r->program = load_shader("shaders/reign.v.glsl", "shaders/reign.f.glsl");
	r->world_transform = glGetUniformLocation(r->program, "world_transform");
	r->view_transform = glGetUniformLocation(r->program, "view_transform");
	r->texture = glGetUniformLocation(r->program, "tex");
	r->local_transform = glGetUniformLocation(r->program, "local_transform");
	r->proj_transform = glGetUniformLocation(r->program, "proj_transform");
	r->normal_transform = glGetUniformLocation(r->program, "normal_transform");
	r->alpha_mod = glGetUniformLocation(r->program, "alpha_mod");
	r->has_bones = glGetUniformLocation(r->program, "has_bones");
	r->bone_matrices = glGetUniformLocation(r->program, "bone_matrices");
	r->ambient = glGetUniformLocation(r->program, "ambient");
	for (int i = 0; i < NR_DIR_LIGHTS; i++) {
		char buf[64];
		sprintf(buf, "dir_lights[%d].dir", i);
		r->dir_lights[i].dir = glGetUniformLocation(r->program, buf);
		sprintf(buf, "dir_lights[%d].diffuse", i);
		r->dir_lights[i].diffuse = glGetUniformLocation(r->program, buf);
		sprintf(buf, "dir_lights[%d].globe_diffuse", i);
		r->dir_lights[i].globe_diffuse = glGetUniformLocation(r->program, buf);
	}
	r->specular_light_dir = glGetUniformLocation(r->program, "specular_light_dir");
	r->specular_strength = glGetUniformLocation(r->program, "specular_strength");
	r->specular_shininess = glGetUniformLocation(r->program, "specular_shininess");
	r->use_specular_map = glGetUniformLocation(r->program, "use_specular_map");
	r->specular_texture = glGetUniformLocation(r->program, "specular_texture");
	r->rim_exponent = glGetUniformLocation(r->program, "rim_exponent");
	r->rim_color = glGetUniformLocation(r->program, "rim_color");
	r->view_pos = glGetUniformLocation(r->program, "view_pos");
	r->use_light_map = glGetUniformLocation(r->program, "use_light_map");
	r->light_texture = glGetUniformLocation(r->program, "light_texture");
	r->use_normal_map = glGetUniformLocation(r->program, "use_normal_map");
	r->normal_texture = glGetUniformLocation(r->program, "normal_texture");
	r->vertex_normal = glGetAttribLocation(r->program, "vertex_normal");
	r->vertex_uv = glGetAttribLocation(r->program, "vertex_uv");
	r->vertex_light_uv = glGetAttribLocation(r->program, "vertex_light_uv");
	r->vertex_tangent = glGetAttribLocation(r->program, "vertex_tangent");
	r->vertex_bone_index = glGetAttribLocation(r->program, "vertex_bone_index");
	r->vertex_bone_weight = glGetAttribLocation(r->program, "vertex_bone_weight");

	glGenRenderbuffers(1, &r->depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, r->depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, texture->w, texture->h);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	init_billboard_mesh(r);
	r->billboard_textures = ht_create(256);
	return r;
}

bool RE_renderer_load_billboard_texture(struct RE_renderer *r, int cg_no)
{
	if (ht_get_int(r->billboard_textures, cg_no, NULL))
		return true;

	struct cg *cg = asset_cg_load(cg_no);
	if (!cg)
		return false;

	struct billboard_texture *bt = xcalloc(1, sizeof(struct billboard_texture));
	glGenTextures(1, &bt->texture);
	glBindTexture(GL_TEXTURE_2D, bt->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cg->metrics.w, cg->metrics.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, cg->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	cg_free(cg);
	ht_put_int(r->billboard_textures, cg_no, bt);
	return true;
}

static void free_billboard_texture(void *value)
{
	struct billboard_texture *bt = value;
	glDeleteTextures(1, &bt->texture);
	free(bt);
}

void RE_renderer_free(struct RE_renderer *r)
{
	glDeleteProgram(r->program);
	glDeleteRenderbuffers(1, &r->depth_buffer);
	ht_foreach_value(r->billboard_textures, free_billboard_texture);
	ht_free_int(r->billboard_textures);
	glDeleteVertexArrays(1, &r->billboard_vao);
	glDeleteBuffers(1, &r->billboard_attr_buffer);
	free(r);
}

static void calc_view_matrix(struct RE_camera *camera, mat4 out)
{
	vec3 front = { 0.0, 0.0, -1.0 };
	vec3 euler = {
		glm_rad(camera->pitch),
		glm_rad(camera->yaw),
		glm_rad(camera->roll)
	};
	mat4 rot;
	glm_euler(euler, rot);
	glm_mat4_mulv3(rot, front, 0.0, front);
	vec3 up = {0.0, 1.0, 0.0};
	glm_look(camera->pos, front, up, out);
}

static void render_model(struct RE_instance *inst, struct RE_renderer *r)
{
	struct model *model = inst->model;
	if (!inst->model)
		return;

	if (inst->local_transform_needs_update)
		RE_instance_update_local_transform(inst);

	glUniformMatrix4fv(r->local_transform, 1, GL_FALSE, inst->local_transform[0]);
	glUniformMatrix3fv(r->normal_transform, 1, GL_FALSE, inst->normal_transform[0]);
	glUniform1f(r->alpha_mod, inst->alpha);
	vec3 ambient;
	glm_vec3_add(inst->plugin->global_ambient, inst->ambient, ambient);
	glUniform3fv(r->ambient, 1, ambient);

	for (int i = 0; i < model->nr_meshes; i++) {
		struct mesh *mesh = &model->meshes[i];
		struct material *material = &model->materials[mesh->material];

		GLboolean use_specular_map = GL_FALSE;
		if (inst->plugin->specular_mode) {
			glUniform1f(r->specular_strength, material->specular_strength);
			glUniform1f(r->specular_shininess, material->specular_shininess);
			if (material->specular_map) {
				glActiveTexture(GL_TEXTURE0 + SPECULAR_TEXTURE_UNIT);
				glBindTexture(GL_TEXTURE_2D, material->specular_map);
				glUniform1i(r->specular_texture, SPECULAR_TEXTURE_UNIT);
				use_specular_map = GL_TRUE;
			}
		} else {
			glUniform1f(r->specular_strength, 0.0);
			glUniform1f(r->specular_shininess, 0.0);
		}
		glUniform1i(r->use_specular_map, use_specular_map);

		glUniform1f(r->rim_exponent, material->rim_exponent);
		glUniform3fv(r->rim_color, 1, material->rim_color);

		glActiveTexture(GL_TEXTURE0 + COLOR_TEXTURE_UNIT);
		glBindTexture(GL_TEXTURE_2D, material->color_map);
		glUniform1i(r->texture, COLOR_TEXTURE_UNIT);

		if (material->light_map && inst->plugin->light_map_mode) {
			glUniform1i(r->use_light_map, GL_TRUE);
			glActiveTexture(GL_TEXTURE0 + LIGHT_TEXTURE_UNIT);
			glBindTexture(GL_TEXTURE_2D, material->light_map);
			glUniform1i(r->light_texture, LIGHT_TEXTURE_UNIT);
		} else {
			glUniform1i(r->use_light_map, GL_FALSE);
		}

		if (material->normal_map && inst->draw_bump && inst->plugin->bump_mode) {
			glUniform1i(r->use_normal_map, GL_TRUE);
			glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_UNIT);
			glBindTexture(GL_TEXTURE_2D, material->normal_map);
			glUniform1i(r->normal_texture, NORMAL_TEXTURE_UNIT);
		} else {
			glUniform1i(r->use_normal_map, GL_FALSE);
		}

		glBindVertexArray(mesh->vao);
		glDrawArrays(GL_TRIANGLES, 0, mesh->nr_vertices);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

static void render_static_model(struct RE_instance *inst, struct RE_renderer *r)
{
	glUniform1i(r->has_bones, GL_FALSE);
	render_model(inst, r);
}

static void render_skinned_model(struct RE_instance *inst, struct RE_renderer *r)
{
	struct model *model = inst->model;
	if (!model)
		return;

	if (model->nr_bones > 0) {
		glUniform1i(r->has_bones, GL_TRUE);
		glUniformMatrix4fv(r->bone_matrices, model->nr_bones, GL_FALSE, inst->bone_transforms[0][0]);
	} else {
		glUniform1i(r->has_bones, GL_FALSE);
	}
	render_model(inst, r);
}

static void render_billboard(struct RE_instance *inst, struct RE_renderer *r, mat4 view_mat)
{
	int cg_no = inst->motion->current_frame;
	struct billboard_texture *bt = ht_get_int(r->billboard_textures, cg_no, NULL);
	if (!bt)
		return;

	mat4 local_transform;
	glm_translate_make(local_transform, inst->pos);
	mat3 rot;
	glm_mat4_pick3t(view_mat, rot);
	glm_mat4_ins3(rot, local_transform);
	glm_scale(local_transform, inst->scale);
	mat3 normal_transform;
	// This should be safe because billboards do not have non-uniform scaling.
	glm_mat4_pick3(local_transform, normal_transform);

	vec3 ambient;
	glm_vec3_add(inst->plugin->global_ambient, inst->ambient, ambient);
	glUniform3fv(r->ambient, 1, ambient);

	glUniformMatrix4fv(r->local_transform, 1, GL_FALSE, local_transform[0]);
	glUniformMatrix3fv(r->normal_transform, 1, GL_FALSE, normal_transform[0]);
	glUniform1i(r->has_bones, GL_FALSE);
	glUniform1f(r->alpha_mod, inst->alpha);
	glUniform1f(r->specular_strength, 0.0);
	glUniform1f(r->specular_shininess, 0.0);
	glUniform1i(r->use_specular_map, GL_FALSE);
	glUniform1f(r->rim_exponent, 0.0);
	glUniform1i(r->use_light_map, GL_FALSE);
	glUniform1i(r->use_normal_map, GL_FALSE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bt->texture);
	glUniform1i(r->texture, 0);
	glBindVertexArray(r->billboard_vao);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static void render_back_cg(struct texture *dst, struct RE_back_cg *bcg, struct RE_renderer *r)
{
	if (!bcg->texture.handle)
		return;
	int sw = bcg->texture.w;
	int sh = bcg->texture.h;
	gfx_copy_stretch_blend(dst, bcg->x, bcg->y, sw * bcg->mag, sh * bcg->mag, &bcg->texture, 0, 0, sw, sh, bcg->blend_rate * 255);
}

static void setup_lights(struct RE_plugin *plugin)
{
	struct RE_renderer *r = plugin->renderer;
	glUniform3fv(r->view_pos, 1, plugin->camera.pos);
	int light_index = 0;
	for (int i = 0; i < plugin->nr_instances; i++) {
		struct RE_instance *inst = plugin->instances[i];
		if (!inst)
			continue;
		switch (inst->type) {
		case RE_ITYPE_DIRECTIONAL_LIGHT:
			if (light_index >= NR_DIR_LIGHTS) {
				WARNING("too many directional lights");
				break;
			}
			glUniform3fv(r->dir_lights[light_index].dir, 1, inst->vec);
			glUniform3fv(r->dir_lights[light_index].diffuse, 1, inst->diffuse);
			glUniform3fv(r->dir_lights[light_index].globe_diffuse, 1, inst->globe_diffuse);
			light_index++;
			break;
		case RE_ITYPE_SPECULAR_LIGHT:
			glUniform3fv(r->specular_light_dir, 1, inst->vec);
			break;
		default:
			break;
		}
	}
	for (; light_index < NR_DIR_LIGHTS; light_index++) {
		const vec3 zero = {0.0, 0.0, 0.0};
		glUniform3fv(r->dir_lights[light_index].dir, 1, zero);
		glUniform3fv(r->dir_lights[light_index].diffuse, 1, zero);
		glUniform3fv(r->dir_lights[light_index].globe_diffuse, 1, zero);
	}
}

void RE_render(struct sact_sprite *sp)
{
	struct RE_plugin *plugin = (struct RE_plugin *)sp->plugin;
	struct RE_renderer *r = plugin->renderer;
	if (!r)
		return;
	sprite_dirty(sp);
	struct texture *texture = sprite_get_texture(sp);

	GLuint fbo = gfx_set_framebuffer(GL_DRAW_FRAMEBUFFER, texture, 0, 0, texture->w, texture->h);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, r->depth_buffer);
	if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		ERROR("Incomplete framebuffer");

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw background CGs.
	glDisable(GL_DEPTH_TEST);
	for (int i = 0; i < RE_NR_BACK_CGS; i++)
		render_back_cg(texture, &plugin->back_cg[i], r);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	mat4 view_transform;
	calc_view_matrix(&plugin->camera, view_transform);

	// Tweak the projection transform so that the rendering result is vertically
	// flipped. If we render the scene normally, the resulting image will be
	// bottom-up (the first pixel is at the bottom-left), but we want a top-down
	// image (the first pixel is at the top-left). This changes the coordinate
	// system from right-handed to left-handed, we also need to reverse the
	// winding order.
	plugin->proj_transform[1][1] *= -1;
	glFrontFace(GL_CW);

	glUseProgram(r->program);
	glUniformMatrix4fv(r->view_transform, 1, GL_FALSE, view_transform[0]);
	glUniformMatrix4fv(r->proj_transform, 1, GL_FALSE, plugin->proj_transform[0]);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	setup_lights(plugin);

	for (int i = 0; i < plugin->nr_instances; i++) {
		struct RE_instance *inst = plugin->instances[i];
		if (!inst || !inst->draw)
			continue;
		switch (inst->type) {
		case RE_ITYPE_STATIC:
			render_static_model(inst, r);
			break;
		case RE_ITYPE_SKINNED:
			render_skinned_model(inst, r);
			break;
		case RE_ITYPE_BILLBOARD:
			render_billboard(inst, r, view_transform);
			break;
		default:
			break;
		}
	}

	plugin->proj_transform[1][1] *= -1;
	glFrontFace(GL_CCW);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glUseProgram(0);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	gfx_reset_framebuffer(GL_DRAW_FRAMEBUFFER, fbo);
}
