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

uniform sampler2D tex;   // the new scene
uniform sampler2D old;   // the old scene
uniform vec2 resolution; // the screen resolution
uniform float progress;  // effect progress (0..1)

in vec2 tex_coord;
out vec4 frag_color;

#define NR_BANDS 14
#define BAND_HEIGHT 15
#define EFFECT_HEIGHT float(NR_BANDS * BAND_HEIGHT)

void main() {
        // start position of effect
        float top = ((resolution.y + EFFECT_HEIGHT) * progress) - EFFECT_HEIGHT;
        // current pixel x-coordinate
        float p = tex_coord.y * resolution.y;
        // band index
        float band_no = (p - top) / 15.0;
        // offset into the band
        float offset = mod(p - top, 15.0);
        // if offset >= band_no, use new texture
        float new = step(band_no, offset);
        vec3 col = mix(texture(old, tex_coord).rgb, texture(tex, tex_coord).rgb, new);
        frag_color = vec4(col, 1.0);
}
