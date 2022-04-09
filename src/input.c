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

#include <stdbool.h>
#include <SDL.h>
#include "system4.h"
#include "gfx/gfx.h"
#include "gfx/private.h"
#include "input.h"
#include "scene.h"
#include "vm.h"

bool key_state[VK_NR_KEYCODES];
bool joybutton_state[SDL_CONTROLLER_BUTTON_MAX];
static enum sact_keycode sdl_keytable[] = {
	[SDL_SCANCODE_0] = VK_0,
	[SDL_SCANCODE_1] = VK_1,
	[SDL_SCANCODE_2] = VK_2,
	[SDL_SCANCODE_3] = VK_3,
	[SDL_SCANCODE_4] = VK_4,
	[SDL_SCANCODE_5] = VK_5,
	[SDL_SCANCODE_6] = VK_6,
	[SDL_SCANCODE_7] = VK_7,
	[SDL_SCANCODE_8] = VK_8,
	[SDL_SCANCODE_9] = VK_9,
	[SDL_SCANCODE_A] = VK_A,
	[SDL_SCANCODE_B] = VK_B,
	[SDL_SCANCODE_C] = VK_C,
	[SDL_SCANCODE_D] = VK_D,
	[SDL_SCANCODE_E] = VK_E,
	[SDL_SCANCODE_F] = VK_F,
	[SDL_SCANCODE_G] = VK_G,
	[SDL_SCANCODE_H] = VK_H,
	[SDL_SCANCODE_I] = VK_I,
	[SDL_SCANCODE_J] = VK_J,
	[SDL_SCANCODE_K] = VK_K,
	[SDL_SCANCODE_L] = VK_L,
	[SDL_SCANCODE_M] = VK_M,
	[SDL_SCANCODE_N] = VK_N,
	[SDL_SCANCODE_O] = VK_O,
	[SDL_SCANCODE_P] = VK_P,
	[SDL_SCANCODE_Q] = VK_Q,
	[SDL_SCANCODE_R] = VK_R,
	[SDL_SCANCODE_S] = VK_S,
	[SDL_SCANCODE_T] = VK_T,
	[SDL_SCANCODE_U] = VK_U,
	[SDL_SCANCODE_V] = VK_V,
	[SDL_SCANCODE_W] = VK_W,
	[SDL_SCANCODE_X] = VK_X,
	[SDL_SCANCODE_Y] = VK_Y,
	[SDL_SCANCODE_Z] = VK_Z,
	[SDL_SCANCODE_BACKSPACE] = VK_BACK,
	[SDL_SCANCODE_TAB] = VK_TAB,
	[SDL_SCANCODE_CLEAR] = VK_CLEAR,
	[SDL_SCANCODE_RETURN] = VK_RETURN,
	[SDL_SCANCODE_LSHIFT] = VK_SHIFT,
	[SDL_SCANCODE_RSHIFT] = VK_SHIFT,
	[SDL_SCANCODE_LCTRL] = VK_CONTROL,
	[SDL_SCANCODE_RCTRL] = VK_CONTROL,
	[SDL_SCANCODE_LALT] = VK_MENU,
	[SDL_SCANCODE_RALT] = VK_MENU,
	[SDL_SCANCODE_PAUSE] = VK_PAUSE,
	[SDL_SCANCODE_CAPSLOCK] = VK_CAPITAL,
	[SDL_SCANCODE_ESCAPE] = VK_ESCAPE,
	[SDL_SCANCODE_SPACE] = VK_SPACE,
	[SDL_SCANCODE_PAGEUP] = VK_PRIOR,
	[SDL_SCANCODE_PAGEDOWN] = VK_NEXT,
	[SDL_SCANCODE_END] = VK_END,
	[SDL_SCANCODE_HOME] = VK_HOME,
	[SDL_SCANCODE_LEFT] = VK_LEFT,
	[SDL_SCANCODE_UP] = VK_UP,
	[SDL_SCANCODE_RIGHT] = VK_RIGHT,
	[SDL_SCANCODE_DOWN] = VK_DOWN,
	[SDL_SCANCODE_SELECT] = VK_SELECT,
	[SDL_SCANCODE_EXECUTE] = VK_EXECUTE,
	[SDL_SCANCODE_PRINTSCREEN] = VK_SNAPSHOT,
	[SDL_SCANCODE_INSERT] = VK_INSERT,
	[SDL_SCANCODE_DELETE] = VK_DELETE,
	[SDL_SCANCODE_HELP] = VK_HELP,
	[SDL_SCANCODE_KP_0] = VK_NUMPAD0,
	[SDL_SCANCODE_KP_1] = VK_NUMPAD1,
	[SDL_SCANCODE_KP_2] = VK_NUMPAD2,
	[SDL_SCANCODE_KP_3] = VK_NUMPAD3,
	[SDL_SCANCODE_KP_4] = VK_NUMPAD4,
	[SDL_SCANCODE_KP_5] = VK_NUMPAD5,
	[SDL_SCANCODE_KP_6] = VK_NUMPAD6,
	[SDL_SCANCODE_KP_7] = VK_NUMPAD7,
	[SDL_SCANCODE_KP_8] = VK_NUMPAD8,
	[SDL_SCANCODE_KP_9] = VK_NUMPAD9,
	[SDL_SCANCODE_KP_MULTIPLY] = VK_MULTIPLY,
	[SDL_SCANCODE_KP_PLUS] = VK_ADD,
	[SDL_SCANCODE_KP_VERTICALBAR] = VK_SEPARATOR, // ???
	[SDL_SCANCODE_KP_MINUS] = VK_SUBTRACT,
	[SDL_SCANCODE_KP_PERIOD] = VK_DECIMAL,
	[SDL_SCANCODE_KP_DIVIDE] = VK_DIVIDE,
	[SDL_SCANCODE_F1] = VK_F1,
	[SDL_SCANCODE_F2] = VK_F2,
	[SDL_SCANCODE_F3] = VK_F3,
	[SDL_SCANCODE_F4] = VK_F4,
	[SDL_SCANCODE_F5] = VK_F5,
	[SDL_SCANCODE_F6] = VK_F6,
	[SDL_SCANCODE_F7] = VK_F7,
	[SDL_SCANCODE_F8] = VK_F8,
	[SDL_SCANCODE_F9] = VK_F9,
	[SDL_SCANCODE_F10] = VK_F10,
	[SDL_SCANCODE_F11] = VK_F11,
	[SDL_SCANCODE_F12] = VK_F12,
	[SDL_SCANCODE_F13] = VK_F13,
	[SDL_SCANCODE_F14] = VK_F14,
	[SDL_SCANCODE_F15] = VK_F15,
	[SDL_SCANCODE_F16] = VK_F16,
	[SDL_SCANCODE_F17] = VK_F17,
	[SDL_SCANCODE_F18] = VK_F18,
	[SDL_SCANCODE_F19] = VK_F19,
	[SDL_SCANCODE_F20] = VK_F20,
	[SDL_SCANCODE_F21] = VK_F21,
	[SDL_SCANCODE_F22] = VK_F22,
	[SDL_SCANCODE_F23] = VK_F23,
	[SDL_SCANCODE_F24] = VK_F24,
	[SDL_SCANCODE_NUMLOCKCLEAR] = VK_NUMLOCK,
	[SDL_SCANCODE_SCROLLLOCK] = VK_SCROLL,
};

bool mouse_focus = true;
bool keyboard_focus = true;

#define MAX_CONTROLLERS 4
static SDL_GameController *controllers[MAX_CONTROLLERS];

// Stores a mouse button event synthesized from a touch event (valid if
// .timestamp != 0). We defer such events to prevent games from handling
// button down events before querying the pointer position.
static SDL_MouseButtonEvent deferred_synthetic_mouse_event;
#define SYNTHETIC_MOUSE_EVENT_DELAY 20

static enum sact_keycode sdl_to_sact_button(int button)
{
	switch (button) {
	case SDL_BUTTON_LEFT:   return VK_LBUTTON;
	case SDL_BUTTON_RIGHT:  return VK_RBUTTON;
	case SDL_BUTTON_MIDDLE: return VK_MBUTTON;
	default:                return 0;
	}
}

bool key_is_down(enum sact_keycode code)
{
	if (code < 0 || code >= VK_NR_KEYCODES) {
		WARNING("key_is_down: invalid keycode (%d)", code);
		return false;
	}
	return key_state[code];
}

bool joy_key_is_down(uint8_t code)
{
	// Default SACT key mapping in comments below
	switch (code) {
	case 1: // Left Arrow
		return joybutton_state[SDL_CONTROLLER_BUTTON_DPAD_LEFT];
	case 2: // Right Arrow
		return joybutton_state[SDL_CONTROLLER_BUTTON_DPAD_RIGHT];
	case 3: // Up Arrow
		return joybutton_state[SDL_CONTROLLER_BUTTON_DPAD_UP];
	case 4: // Down Arrow
		return joybutton_state[SDL_CONTROLLER_BUTTON_DPAD_DOWN];
	case 5: // Esc
		return joybutton_state[SDL_CONTROLLER_BUTTON_BACK] || joybutton_state[SDL_CONTROLLER_BUTTON_START];
	case 6: // Enter
		return joybutton_state[SDL_CONTROLLER_BUTTON_A];
	case 7: // Space
		return joybutton_state[SDL_CONTROLLER_BUTTON_Y];
	case 8: // Ctrl
		return joybutton_state[SDL_CONTROLLER_BUTTON_B];
	case 9: // a
		return joybutton_state[SDL_CONTROLLER_BUTTON_LEFTSTICK];
	case 10: // z
		return joybutton_state[SDL_CONTROLLER_BUTTON_RIGHTSTICK];
	case 11: // Page Up
		return joybutton_state[SDL_CONTROLLER_BUTTON_LEFTSHOULDER];
	case 12: // Page Down
		return joybutton_state[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER];
	default:
		return false;
	}
}

void key_clear_flag(void)
{
	for (int i = 0; i < VK_NR_KEYCODES; i++) {
		key_state[i] = false;
	}
}

void joy_clear_flag(void)
{
	for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
		joybutton_state[i] = false;
	}
}

static void mouse_event(SDL_MouseButtonEvent *e);

void mouse_get_pos(int *x, int *y)
{
	int wx, wy;
	SDL_PumpEvents();
	SDL_GetMouseState(&wx, &wy);
	*x = (wx - sdl.viewport.x) * sdl.w / sdl.viewport.w;
	*y = (wy - sdl.viewport.y) * sdl.h / sdl.viewport.h;
}

void mouse_set_pos(int x, int y)
{
	int wx = x * sdl.viewport.w / sdl.w + sdl.viewport.x;
	int wy = y * sdl.viewport.h / sdl.h + sdl.viewport.y;
	SDL_WarpMouseInWindow(sdl.window, wx, wy);
}

static int wheel_dir = 0;

void mouse_get_wheel(int *forward, int *back)
{
	*forward = wheel_dir > 0;
	*back = wheel_dir < 0;
}

void mouse_clear_wheel(void)
{
	wheel_dir = 0;
}

bool mouse_show_cursor(bool show)
{
	return SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE) >= 0;
}

static void key_event(SDL_KeyboardEvent *e, bool pressed)
{
	if (e->keysym.scancode >= (sizeof(sdl_keytable)/sizeof(*sdl_keytable)))
		return;
	enum sact_keycode code = sdl_keytable[e->keysym.scancode];
	if (code)
		key_state[code] = pressed;
}

static void mouse_event(SDL_MouseButtonEvent *e)
{
	enum sact_keycode code = sdl_to_sact_button(e->button);
	if (code)
		key_state[code] = e->state == SDL_PRESSED;
}

static void controller_button_event(SDL_ControllerButtonEvent *e)
{
	if (e->state != SDL_PRESSED) {
		NOTICE("UNPRESSED");
	}
	joybutton_state[e->button] = e->state == SDL_PRESSED;
}

static void(*input_handler)(const char*);
static void(*editing_handler)(const char*, int, int);

void register_input_handler(void(*handler)(const char*))
{
	input_handler = handler;
}

void clear_input_handler(void)
{
	input_handler = NULL;
}

void register_editing_handler(void(*handler)(const char*, int, int))
{
	editing_handler = handler;
}

void clear_editing_handler(void)
{
	editing_handler = NULL;
}

void handle_events(void)
{
	// Flush the deferred mouse button event if it's older than 20ms.
	if (deferred_synthetic_mouse_event.timestamp && deferred_synthetic_mouse_event.timestamp + SYNTHETIC_MOUSE_EVENT_DELAY < SDL_GetTicks()) {
		mouse_event(&deferred_synthetic_mouse_event);
		deferred_synthetic_mouse_event.timestamp = 0;
	}

	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			vm_exit(0);
			break;
		case SDL_WINDOWEVENT:
			switch (e.window.event) {
			case SDL_WINDOWEVENT_EXPOSED:
				gfx_swap();
				break;
			case SDL_WINDOWEVENT_ENTER:
				mouse_focus = true;
				break;
			case SDL_WINDOWEVENT_LEAVE:
				mouse_focus = false;
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				keyboard_focus = true;
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				keyboard_focus = false;
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				gfx_update_screen_scale();
				gfx_swap();
				break;
			}
			break;
		case SDL_KEYDOWN:
			if (e.key.keysym.scancode == SDL_SCANCODE_F9)
				vm_stack_trace();
			key_event(&e.key, true);
			break;
		case SDL_KEYUP:
			key_event(&e.key, false);
			break;
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			if (e.button.which == SDL_TOUCH_MOUSEID) {
				if (deferred_synthetic_mouse_event.timestamp)
					mouse_event(&deferred_synthetic_mouse_event);
				deferred_synthetic_mouse_event = e.button;
			} else {
				mouse_event(&e.button);
			}
			break;
		case SDL_MOUSEWHEEL:
			wheel_dir = e.wheel.y;
			break;
		case SDL_CONTROLLERDEVICEADDED:
			if (e.cdevice.which < MAX_CONTROLLERS)
				controllers[e.cdevice.which] = SDL_GameControllerOpen(e.cdevice.which);
			break;
		case SDL_CONTROLLERBUTTONUP:
		case SDL_CONTROLLERBUTTONDOWN:
			controller_button_event(&e.cbutton);
			break;
		case SDL_TEXTINPUT:
			if (input_handler)
				input_handler(e.text.text);
			break;
		case SDL_TEXTEDITING:
			if (editing_handler)
				editing_handler(e.edit.text, e.edit.start, e.edit.length);
			break;
		default:
			break;
		}
	}
}

