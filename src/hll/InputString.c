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

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "system4.h"
#include "system4/utfsjis.h"
#include "system4/string.h"

#include "gfx/types.h"
#include "gfx/gfx.h"
#include "hll.h"
#include "input.h"

#ifdef __EMSCRIPTEN__

EM_JS(void, InputString_ClearResultString, (void), {
	Module.shell.input.ClearResultString();
});

static struct string *InputString_GetResultString(void)
{
	char *utf8 = EM_ASM_PTR({
		const s = Module.shell.input.GetResultString();
		return s ? stringToNewUTF8(s) : 0;
	});
	if (!utf8)
		string_ref(&EMPTY_STRING);
	char *sjis = utf2sjis(utf8, 0);
	struct string *str = cstr_to_string(sjis);
	free(sjis);
	free(utf8);
	return str;
}

static void InputString_SetFont(int size, struct string *name, int weight)
{
	char *font_name = sjis2utf(name->text, name->size);
	EM_ASM({
		Module.shell.input.SetFont($0, UTF8ToString($1), $2);
	}, size, font_name, weight);
	free(font_name);
}

EM_JS(void, InputString_SetPos, (int x, int y), {
	Module.shell.input.SetPos(x, y);
});

EM_JS(void, InputString_Begin, (void), {
	Module.shell.input.Begin();
});

EM_JS(void, InputString_End, (void), {
	Module.shell.input.End();
});

EM_JS(void, handle_input, (const char *text), {
	Module.shell.input.addText(UTF8ToString(text));
});

static void InputString_OpenIME(void)
{
	EM_ASM({ Module.shell.input.OpenIME(); });
	// Also enable SDL's text input mode, otherwise we won't be able to get
	// input that isn't via IME.
	register_input_handler(handle_input);
	SDL_StartTextInput();
}

static void InputString_CloseIME(void)
{
	EM_ASM({ Module.shell.input.CloseIME(); });
	SDL_StopTextInput();
	clear_input_handler();
}

#else //  __EMSCRIPTEN__

static int font_size;
static int font_weight;
static Point pos;

static struct string *result;

static void InputString_ClearResultString(void)
{
	if (result)
		free_string(result);
	result = string_ref(&EMPTY_STRING);
}

static struct string *InputString_GetResultString(void)
{
	struct string *s = string_ref(result);
	InputString_ClearResultString();
	return s;
}

static void InputString_SetFont(int nSize, possibly_unused struct string *pIName, int nWeight)
{
	font_size = nSize;
	font_weight = nWeight;
}

static void InputString_SetPos(int nX, int nY)
{
	pos.x = nX;
	pos.y = nY;
}

static void InputString_Begin(void)
{
	InputString_ClearResultString();
}

static void InputString_End(void)
{
	InputString_ClearResultString();
}

static void handle_input(const char *text)
{
	char *u = utf2sjis(text, 0);
	string_append_cstr(&result, u, strlen(u));
	free(u);
}

// TODO: This function has to draw the text at the given coordinates/size/weight.
//       Need some kind of interface for creating textures that are always rendered
//       on top of the scene. Should ideally be independent of SACT.
//static void handle_editing(const char *text, int start, int length)
//{
//}

static void InputString_OpenIME(void)
{
	register_input_handler(handle_input);
//	register_editing_handler(handle_editing);
	SDL_StartTextInput();
}

static void InputString_CloseIME(void)
{
	SDL_StopTextInput();
	clear_input_handler();
	clear_editing_handler();
}

#endif // __EMSCRIPTEN__

HLL_LIBRARY(InputString,
	    HLL_EXPORT(SetFont, InputString_SetFont),
	    HLL_EXPORT(SetPos, InputString_SetPos),
	    HLL_EXPORT(Begin, InputString_Begin),
	    HLL_EXPORT(End, InputString_End),
	    HLL_EXPORT(OpenIME, InputString_OpenIME),
	    HLL_EXPORT(CloseIME, InputString_CloseIME),
	    HLL_EXPORT(GetResultString, InputString_GetResultString),
	    HLL_EXPORT(ClearResultString, InputString_ClearResultString));
