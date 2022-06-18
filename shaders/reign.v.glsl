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

uniform mat4 local_transform;
uniform mat4 view_transform;
uniform mat4 proj_transform;

const int MAX_BONES = 211;  // see 3d_internal.h
const int NR_WEIGHTS = 4;
uniform bool has_bones;
uniform mat4 bone_matrices[MAX_BONES];

in vec3 vertex_pos;
in vec3 vertex_normal;
in vec2 vertex_uv;
in ivec4 vertex_bone_index;
in vec4 vertex_bone_weight;

out vec2 tex_coord;

void main() {
	mat4 bone_transform = mat4(1.0f);
	if (has_bones) {
		bone_transform = mat4(0.f);
		for (int i = 0; i < NR_WEIGHTS; i++) {
			if (vertex_bone_index[i] >= 0) {
				bone_transform += bone_matrices[vertex_bone_index[i]] * vertex_bone_weight[i];
			}
		}
	}

	gl_Position = proj_transform * view_transform * local_transform * bone_transform * vec4(vertex_pos, 1.0);
	tex_coord = vertex_uv;
}