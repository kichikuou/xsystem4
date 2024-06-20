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

uniform sampler2D tex;
uniform vec4 color;  // .a is the discard threshold

in vec2 tex_coord;
out vec4 frag_color;

void main() {
	vec4 texel = texture(tex, tex_coord);
	if (texel.r < color.a)
		discard;
	frag_color = vec4(color.rgb, texel.r);
}
