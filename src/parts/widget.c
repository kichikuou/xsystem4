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

#include "system4.h"
#include "system4/utfsjis.h"
#include "parts.h"
#include "parts_internal.h"

// Replace each UTF-8 string in `strs` with a new CP932 string. The function
// does not free the old strings.
void parts_utf2sjis_array(const char **strs, int n)
{
	for (int i = 0; i < n; i++)
		strs[i] = utf2sjis(strs[i], 0);
}

void parts_widget_free(struct parts *parts)
{
	switch (parts->widget_type) {
	case PARTS_WIDGET_NONE:
		return;
	case PARTS_WIDGET_BUTTON:
		parts_button_free(parts->widget.button);
		break;
	case PARTS_WIDGET_CHECKBOX:
		parts_checkbox_free(parts->widget.checkbox);
		break;
	case PARTS_WIDGET_VSCROLLBAR:
	case PARTS_WIDGET_HSCROLLBAR:
		parts_scrollbar_free(parts->widget.scrollbar);
		break;
	case PARTS_WIDGET_LAYOUTBOX:
		free(parts->widget.layoutbox);
		break;
	case PARTS_WIDGET_RADIOBOX:
		parts_radiobox_free(parts->widget.radiobox);
		break;
	}
	parts->widget_type = PARTS_WIDGET_NONE;
	parts->widget.button = NULL;
}

void parts_widget_reset(struct parts *parts, enum parts_widget_type type,
		enum parts_sprite_type state_type)
{
	parts_widget_free(parts);
	for (int i = 0; i < PARTS_NR_STATES; i++)
		parts_state_reset(&parts->states[i], state_type);
	parts_recalculate_hitbox(parts);
	parts_dirty(parts);
	parts->widget_type = type;
}

void parts_widget_set_type(struct parts *parts, enum parts_widget_type type)
{
	switch (type) {
	case PARTS_WIDGET_NONE:
		break;
	case PARTS_WIDGET_BUTTON:
		parts_get_button(parts);
		break;
	case PARTS_WIDGET_CHECKBOX:
		parts_get_checkbox(parts);
		break;
	case PARTS_WIDGET_VSCROLLBAR:
	case PARTS_WIDGET_HSCROLLBAR:
		parts_get_scrollbar(parts, type == PARTS_WIDGET_HSCROLLBAR);
		break;
	case PARTS_WIDGET_LAYOUTBOX:
		parts_get_layoutbox(parts);
		break;
	case PARTS_WIDGET_RADIOBOX:
		parts_get_radiobox(parts);
		break;
	}
}

void parts_widget_update(struct parts *parts)
{
	if (parts_is_scrollbar(parts))
		parts_scrollbar_update(parts);
}

void parts_widget_end_input(struct parts *parts)
{
	if (parts_is_scrollbar(parts))
		parts_scrollbar_mouse(parts, (Point){0, 0}, false, false, false, 0);
}
