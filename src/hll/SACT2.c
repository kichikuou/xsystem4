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

#include <math.h>
#include <time.h>

#include "system4.h"
#include "system4/ain.h"
#include "system4/ald.h"
#include "system4/cg.h"
#include "system4/string.h"

#include "hll.h"
#include "audio.h"
#include "input.h"
#include "queue.h"
#include "gfx/gfx.h"
#include "sact.h"
#include "vm/page.h"
#include "dungeon/dungeon.h"
#include "xsystem4.h"

static struct sact_sprite **sprites = NULL;
static int nr_sprites = 0;

static struct sact_sprite *sact_alloc_sprite(int sp)
{
	sprites[sp] = xcalloc(1, sizeof(struct sact_sprite));
	sprites[sp]->no = sp;
	sprite_dirty(sprites[sp]);
	return sprites[sp];
}

static void sact_free_sprite(struct sact_sprite *sp)
{
	if (sprites[sp->no] == NULL)
		VM_ERROR("Double free of sact_sprite");
	sprites[sp->no] = NULL;
	sprite_free(sp);
	free(sp);
}

static void realloc_sprite_table(int n)
{
	int old_nr_sprites = nr_sprites;
	nr_sprites = n;

	struct sact_sprite **tmp = xrealloc(sprites-1, sizeof(struct sact_sprite*) * (nr_sprites+1));
	sprites = tmp + 1;

	memset(sprites + old_nr_sprites, 0, sizeof(struct sact_sprite*) * (nr_sprites - old_nr_sprites));
}

// NOTE: Used externally by DrawGraph, SengokuRanceFont and dungeon.c
struct sact_sprite *sact_get_sprite(int sp)
{
	if (sp < -1)
		return NULL;
	if (sp >= nr_sprites)
		realloc_sprite_table(sp+256);
	if (!sprites[sp]) {
		return sact_alloc_sprite(sp);
	}
	return sprites[sp];
}

int sact_Init(possibly_unused void *_, possibly_unused int cg_cache_size)
{
	// already initialized
	if (sprites)
		return 1;

	gfx_init();
	gfx_font_init();
	audio_init();

	nr_sprites = 256;
	sprites = xmalloc(sizeof(struct sact_sprite*) * 257);
	memset(sprites, 0, sizeof(struct sact_sprite*) * 257);

	// create sprite for main_surface
	Texture *t = gfx_main_surface();
	struct sact_sprite *sp = sact_create_sprite(0, t->w, t->h, 0, 0, 0, 255);
	sp->texture = *t; // XXX: textures normally shouldn't be copied like this...

	sprites++;
	return 1;
}

void sact_ModuleFini(void)
{
	sact_SP_DeleteAll();
}

//int SACT2_Error(struct string *err);
//int SACT2_WP_GetSP(int sp);
//int SACT2_WP_SetSP(int sp);

int sact_GetScreenWidth(void)
{
	return config.view_width;
}

int sact_GetScreenHeight(void)
{
	return config.view_height;
}

int sact_GetMainSurfaceNumber(void)
{
	return -1;
}

int sact_Update(void)
{
	handle_events();
	dungeon_update();
	if (sact_dirty) {
		sprite_render_scene();
		sprite_flip();
	}
	return 1;
}

HLL_WARN_UNIMPLEMENTED(1, int, SACT2, EffectSetMask, int cg);
//int SACT2_EffectSetMaskSP(int sp);
HLL_WARN_UNIMPLEMENTED( , void, SACT2, QuakeScreen, int amp_x, int amp_y, int time, int key);
//void SACT2_QUAKE_SET_CROSS(int amp_x, int amp_y);
//void SACT2_QUAKE_SET_ROTATION(int amp, int cycle);

int sact_SP_GetUnuseNum(int min)
{
	for (int i = min; i < nr_sprites; i++) {
		if (!sprites[i])
			return i;
	}

	int n = max(min, nr_sprites);
	realloc_sprite_table(n + 256);
	return n;
}

int sact_SP_Count(void)
{
	int count = 0;
	for (int i = 0; i < nr_sprites; i++) {
		if (sprites[i])
			count++;
	}
	return count;
}

int sact_SP_Enum(struct page **array)
{
	int count = 0;
	int size = (*array)->nr_vars;
	for (int i = 0; i < nr_sprites && count < size; i++) {
		if (sprites[i]) {
			(*array)->values[count++].i = i;
		}
	}
	return count;
}

int sact_SP_SetCG(int sp_no, int cg_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp)
		sact_SP_Create(sp_no, 1, 1, 0, 0, 0, 255);
	if (!(sp = sact_get_sprite(sp_no))) {
		WARNING("Failed to create sprite");
		return 0;
	}
	return sprite_set_cg(sp, cg_no);
}

int SACT2_SP_SetCGFromFile(int sp_no, struct string *filename)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp)
		sact_SP_Create(sp_no, 1, 1, 0, 0, 0, 255);
	if (!(sp = sact_get_sprite(sp_no))) {
		WARNING("Failed to create sprite");
		return 0;
	}
	char *path = gamedir_path(filename->text);
	int r = sprite_set_cg_from_file(sp, path);
	free(path);
	return r;
}

//int SACT2_SP_SaveCG(int sp, struct string *filename);

struct sact_sprite *sact_create_sprite(int sp_no, int width, int height, int r, int g, int b, int a)
{
	struct sact_sprite *sp;
	if (sp_no < 0)
		VM_ERROR("Invalid sprite number: %d", sp_no);
	if (sp_no >= nr_sprites) {
		realloc_sprite_table(sp_no+256);
	}
	if (!(sp = sact_get_sprite(sp_no)))
		sp = sact_alloc_sprite(sp_no);

	sprite_init(sp, width, height, r, g, b, a);
	return sp;
}

int sact_SP_Create(int sp_no, int width, int height, int r, int g, int b, int a)
{
	sact_create_sprite(sp_no, width, height, r, g, b, a);
	return 1;
}

int sact_SP_CreatePixelOnly(int sp_no, int width, int height)
{
	sact_create_sprite(sp_no, width, height, 0, 0, 0, -1);
	return 1;
}

//int SACT2_SP_CreateCustom(int sp);

int sact_SP_Delete(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sact_free_sprite(sp);
	return 1;
}

int sact_SP_DeleteAll(void)
{
	for (int i = 0; i < nr_sprites; i++) {
		if (sprites[i]) {
			sact_free_sprite(sprites[i]);
		}
	}
	return 1;
}

int sact_SP_SetPos(int sp_no, int x, int y)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 1;
	sprite_set_pos(sp, x, y);
	return 1;
}

int sact_SP_SetX(int sp_no, int x)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_set_x(sp, x);
	return 1;
}

int sact_SP_SetY(int sp_no, int y)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_set_y(sp, y);
	return 1;
}

int sact_SP_SetZ(int sp_no, int z)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_set_z(sp, z);
	return 1;
}

int sact_SP_GetBlendRate(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_blend_rate(sp);
}

int sact_SP_SetBlendRate(int sp_no, int rate)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_set_blend_rate(sp, rate);
	return 1;
}

int sact_SP_SetShow(int sp_no, bool show)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_set_show(sp, show);
	return 1;
}

int sact_SP_SetDrawMethod(int sp_no, int method)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_set_draw_method(sp, method);
	return 1;
}

int sact_SP_GetDrawMethod(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return DRAW_METHOD_NORMAL;
	return sprite_get_draw_method(sp);
}

int sact_SP_IsUsing(int sp_no)
{
	return sact_get_sprite(sp_no) != NULL;
}

int sact_SP_ExistsAlpha(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_exists_alpha(sp);
}

int sact_SP_GetPosX(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_pos_x(sp);
}

int sact_SP_GetPosY(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_pos_y(sp);
}

int sact_SP_GetWidth(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_width(sp);
}

int sact_SP_GetHeight(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_height(sp);
}

int sact_SP_GetZ(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_z(sp);
}

int sact_SP_GetShow(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_show(sp);
}

int sact_SP_SetTextHome(int sp_no, int x, int y)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_set_text_home(sp, x, y);
	return 1;
}

int sact_SP_SetTextLineSpace(int sp_no, int px)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 1;
	sprite_set_text_line_space(sp, px);
	return 1;
}

int sact_SP_SetTextCharSpace(int sp_no, int px)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 1;
	sprite_set_text_char_space(sp, px);
	return 1;
}

int sact_SP_SetTextPos(int sp_no, int x, int y)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_set_text_pos(sp, x, y);
	return 1;
}

static void init_text_metrics(struct text_metrics *tm, union vm_value *_tm)
{
	*tm = (struct text_metrics) {
		.color = {
			.r = _tm[0].i,
			.g = _tm[1].i,
			.b = _tm[2].i,
			.a = 255
		},
		.outline_color = {
			.r = _tm[10].i,
			.g = _tm[11].i,
			.b = _tm[12].i,
			.a = 255
		},
		.size          = _tm[3].i,
		.weight        = _tm[4].i,
		.face          = _tm[5].i,
		.outline_left  = _tm[6].i,
		.outline_up    = _tm[7].i,
		.outline_right = _tm[8].i,
		.outline_down  = _tm[9].i,
	};
}

int _sact_SP_TextDraw(int sp_no, struct string *text, struct text_metrics *tm)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_text_draw(sp, text, tm);
	return 1;
}

int sact_SP_TextDraw(int sp_no, struct string *text, struct page *_tm)
{
	struct text_metrics tm;
	init_text_metrics(&tm, _tm->values);
	_sact_SP_TextDraw(sp_no, text, &tm);
	return 1;
}

int sact_SP_TextClear(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_text_clear(sp);
	return 1;
}

int sact_SP_TextHome(int sp_no, int size)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_text_home(sp, size);
	return 1;
}

int sact_SP_TextNewLine(int sp_no, int size)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_text_new_line(sp, size);
	return 1;
}

//int SACT2_SP_TextBackSpace(int sp_no);

int sact_SP_TextCopy(int dno, int sno)
{
	struct sact_sprite *dsp = sact_get_sprite(dno);
	struct sact_sprite *ssp = sact_get_sprite(sno);
	if (!ssp)
		return 0;
	if (!dsp) {
		sact_SP_Create(dno, 1, 1, 0, 0, 0, 0);
		if (!(dsp = sact_get_sprite(dno))) {
			WARNING("Failed to create sprite");
			return 0;
		}
	}

	sprite_text_copy(dsp, ssp);
	return 1;
}

int sact_SP_GetTextHomeX(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_text_home_x(sp);
}

int sact_SP_GetTextHomeY(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_text_home_y(sp);
}

int sact_SP_GetTextCharSpace(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_text_char_space(sp);
}

int sact_SP_GetTextPosX(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_text_pos_x(sp);
}

int sact_SP_GetTextPosY(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_text_pos_y(sp);
}

int sact_SP_GetTextLineSpace(int sp_no)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_text_line_space(sp);
}

int sact_SP_IsPtIn(int sp_no, int x, int y)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_is_point_in(sp, x, y);
}

int sact_SP_IsPtInRect(int sp_no, int x, int y)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_is_point_in_rect(sp, x, y);
}

int sact_CG_GetMetrics(int cg_no, struct page **page)
{
	union vm_value *cgm = (*page)->values;
	struct cg_metrics metrics;
	if (!cg_get_metrics(ald[ALDFILE_CG], cg_no - 1, &metrics))
		return 0;
	cgm[0].i = metrics.w;
	cgm[1].i = metrics.h;
	cgm[2].i = metrics.bpp;
	cgm[3].i = metrics.has_pixel;
	cgm[4].i = metrics.has_alpha;
	cgm[5].i = metrics.pixel_pitch;
	cgm[6].i = metrics.alpha_pitch;
	return 1;
}

int sact_SP_GetAMapValue(int sp_no, int x, int y)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	return sprite_get_amap_value(sp, x, y);
}

int sact_SP_GetPixelValue(int sp_no, int x, int y, int *r, int *g, int *b)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp) return 0;
	sprite_get_pixel_value(sp, x, y, r, g, b);
	return 1;
}

int sact_GAME_MSG_GetNumOf(void)
{
	return ain->nr_messages;
}

//void SACT2_GAME_MSG_Get(int index, struct string **text);

void sact_IntToZenkaku(struct string **s, int value, int figures, int zero_pad)
{
	int i;
	char buf[512];

	string_clear(*s);
	i = int_to_cstr(buf, 512, value, figures, zero_pad, true);
	string_append_cstr(s, buf, i);
}

void sact_IntToHankaku(struct string **s, int value, int figures, int zero_pad)
{
	int i;
	char buf[512];

	string_clear(*s);
	i = int_to_cstr(buf, 512, value, figures, zero_pad, false);
	string_append_cstr(s, buf, i);
}

//int SACT2_StringPopFront(struct string **dst, struct string **src);

int sact_Mouse_GetPos(int *x, int *y)
{
	handle_events();
	mouse_get_pos(x, y);
	return mouse_focus && keyboard_focus;
}

int sact_Mouse_SetPos(int x, int y)
{
	handle_events();
	mouse_set_pos(x, y);
	return mouse_focus && keyboard_focus;
}

void sact_Joypad_ClearKeyDownFlag(int n)
{
	// TODO
}

int sact_Joypad_IsKeyDown(int num, int key)
{
	return 0; // TODO
}

//int SACT2_Joypad_GetNumof(void);
HLL_WARN_UNIMPLEMENTED( , void, SACT2, JoypadQuake_Set, int num, int type, int magnitude);

bool sact_Joypad_GetAnalogStickStatus(int num, int type, float *degree, float *power)
{
	//hll_unimplemented_warning(SACT2, Joypad_GetAnalogStickStatus);
	*degree = 0.0;
	*power  = 0.0;
	return true;
}

bool sact_Joypad_GetDigitalStickStatus(int num, int type, bool *left, bool *right, bool *up, bool *down)
{
	//hll_unimplemented_warning(SACT2, Joypad_GetDigitalStickStatus);
	*left  = false;
	*right = false;
	*up    = false;
	*down  = false;
	return 1;
}

int sact_Key_ClearFlag(void)
{
	key_clear_flag();
	return 1;
}

int sact_Key_IsDown(int keycode)
{
	handle_events();
	return key_is_down(keycode);
}

int sact_CG_IsExist(int cg_no)
{
	return ald[ALDFILE_CG] && archive_exists(ald[ALDFILE_CG], cg_no - 1);
}

//int  SACT2_CSV_Load(struct string *filename);
//int  SACT2_CSV_Save(void);
//int  SACT2_CSV_SaveAs(struct string *filename);
//int  SACT2_CSV_CountLines(void);
//int  SACT2_CSV_CountColumns(void);
//void SACT2_CSV_Get(struct string **s, int line, int column);
//int  SACT2_CSV_Set(int line, int column, struct string *data);
//int  SACT2_CSV_GetInt(int line, int column);
//void SACT2_CSV_SetInt(int line, int column, int data);
//void SACT2_CSV_Realloc(int lines, int columns);

int sact_Music_IsExist(int n)
{
	return bgm_exists(n - 1);
}

int sact_Music_Prepare(int ch, int n)
{
	return bgm_prepare(ch, n - 1);
}

HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_SetLoopCount, int ch, int count);
HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_GetLoopCount, int ch);
//int SACT2_Music_SetLoopStartPos(int ch, int pos);
//int SACT2_Music_SetLoopEndPos(int ch, int pos);

HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_StopFade, int ch);
HLL_WARN_UNIMPLEMENTED(0, int, SACT2, Music_IsFade, int ch);
HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_Pause, int ch);
HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_Restart, int ch);
HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_IsPause, int ch);
HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_GetPos, int ch);
HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_GetLength, int ch);
HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_GetSamplePos, int ch);
HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_GetSampleLength, int ch);
HLL_WARN_UNIMPLEMENTED(1, int, SACT2, Music_Seek, int ch, int pos);

int sact_Sound_IsExist(int n)
{
	return wav_exists(n - 1);
}

int sact_Sound_Prepare(int ch, int n)
{
	return wav_prepare(ch, n - 1);
}

//int SACT2_Sound_SetLoopCount(int ch, int count);
//int SACT2_Sound_GetLoopCount(int ch);

//int SACT2_Sound_StopFade(int ch);
//int SACT2_Sound_IsFade(int ch);
//int SACT2_Sound_GetPos(int ch);
//int SACT2_Sound_GetLength(int ch);

//int SACT2_Sound_GetVolume(int ch);

//int SACT2_Sound_GetGroupNum(int ch);
//bool SACT2_Sound_PrepareFromFile(int ch, struct string *filename);

void sact_System_GetDate(int *year, int *month, int *mday, int *wday)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	*year  = tm->tm_year;
	*month = tm->tm_mon;
	*mday  = tm->tm_mday;
	*wday  = tm->tm_wday;
}

void sact_System_GetTime(int *hour, int *min, int *sec, int *ms)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	*hour = tm->tm_hour;
	*min  = tm->tm_min;
	*sec  = ts.tv_sec;
	*ms   = ts.tv_nsec / 1000000;
}

//void SACT2_CG_RotateRGB(int dst, int dx, int dy, int w, int h, int rotate_type);

void sact_CG_BlendAMapBin(int dst, int dx, int dy, int src, int sx, int sy, int w, int h, int border)
{
	struct sact_sprite *dsp = sact_get_sprite(dst);
	struct sact_sprite *ssp = sact_get_sprite(src);
	if (!dsp) return;
	struct texture *dtx = sprite_get_texture(dsp);
	struct texture *stx = ssp ? sprite_get_texture(ssp) : NULL;
	gfx_copy_use_amap_border(dtx, dx, dy, stx, sx, sy, w, h, border);
	sprite_dirty(dsp);
}

//void SACT2_Debug_Pause(void);
//void SACT2_Debug_GetFuncStack(struct string **s, int nest);
HLL_WARN_UNIMPLEMENTED(0, int, SACT2, SP_SetBrightness, int sp_no, int brightness);
HLL_WARN_UNIMPLEMENTED(0, int, SACT2, SP_GetBrightness, int sp_no);

#define SACT_EXPORTS \
	    HLL_EXPORT(_ModuleFini, sact_ModuleFini), \
	    HLL_EXPORT(Init, sact_Init), \
	    HLL_TODO_EXPORT(Error, SACT2_Error), \
	    HLL_EXPORT(SetWP, sact_SetWP), \
	    HLL_EXPORT(SetWP_Color, sact_SetWP_Color), \
	    HLL_TODO_EXPORT(WP_GetSP, SACT2_WP_GetSP), \
	    HLL_TODO_EXPORT(WP_SetSP, SACT2_WP_SetSP), \
	    HLL_EXPORT(GetScreenWidth, sact_GetScreenWidth), \
	    HLL_EXPORT(GetScreenHeight, sact_GetScreenHeight), \
	    HLL_EXPORT(GetMainSurfaceNumber, sact_GetMainSurfaceNumber), \
	    HLL_EXPORT(Update, sact_Update), \
	    HLL_EXPORT(Effect, sact_Effect), \
	    HLL_EXPORT(EffectSetMask, SACT2_EffectSetMask), \
	    HLL_TODO_EXPORT(EffectSetMaskSP, SACT2_EffectSetMaskSP), \
	    HLL_EXPORT(QuakeScreen, SACT2_QuakeScreen), \
	    HLL_TODO_EXPORT(QUAKE_SET_CROSS, SACT2_QUAKE_SET_CROSS), \
	    HLL_TODO_EXPORT(QUAKE_SET_ROTATION, SACT2_QUAKE_SET_ROTATION), \
	    HLL_EXPORT(SP_GetUnuseNum, sact_SP_GetUnuseNum), \
	    HLL_EXPORT(SP_Count, sact_SP_Count), \
	    HLL_EXPORT(SP_Enum, sact_SP_Enum), \
	    HLL_EXPORT(SP_GetMaxZ, sact_SP_GetMaxZ), \
	    HLL_EXPORT(SP_SetCG, sact_SP_SetCG), \
	    HLL_EXPORT(SP_SetCGFromFile, SACT2_SP_SetCGFromFile), \
	    HLL_TODO_EXPORT(SP_SaveCG, SACT2_SP_SaveCG), \
	    HLL_EXPORT(SP_Create, sact_SP_Create), \
	    HLL_EXPORT(SP_CreatePixelOnly, sact_SP_CreatePixelOnly), \
	    HLL_TODO_EXPORT(SP_CreateCustom, SACT2_SP_CreateCustom), \
	    HLL_EXPORT(SP_Delete, sact_SP_Delete), \
	    HLL_EXPORT(SP_SetPos, sact_SP_SetPos), \
	    HLL_EXPORT(SP_SetX, sact_SP_SetX), \
	    HLL_EXPORT(SP_SetY, sact_SP_SetY), \
	    HLL_EXPORT(SP_SetZ, sact_SP_SetZ), \
	    HLL_EXPORT(SP_SetBlendRate, sact_SP_SetBlendRate), \
	    HLL_EXPORT(SP_SetShow, sact_SP_SetShow), \
	    HLL_EXPORT(SP_SetDrawMethod, sact_SP_SetDrawMethod), \
	    HLL_EXPORT(SP_IsUsing, sact_SP_IsUsing), \
	    HLL_EXPORT(SP_ExistAlpha, sact_SP_ExistsAlpha), \
	    HLL_EXPORT(SP_GetPosX, sact_SP_GetPosX), \
	    HLL_EXPORT(SP_GetPosY, sact_SP_GetPosY), \
	    HLL_EXPORT(SP_GetWidth, sact_SP_GetWidth), \
	    HLL_EXPORT(SP_GetHeight, sact_SP_GetHeight), \
	    HLL_EXPORT(SP_GetZ, sact_SP_GetZ), \
	    HLL_EXPORT(SP_GetBlendRate, sact_SP_GetBlendRate), \
	    HLL_EXPORT(SP_GetShow, sact_SP_GetShow), \
	    HLL_EXPORT(SP_GetDrawMethod, sact_SP_GetDrawMethod), \
	    HLL_EXPORT(SP_SetTextHome, sact_SP_SetTextHome), \
	    HLL_EXPORT(SP_SetTextLineSpace, sact_SP_SetTextLineSpace), \
	    HLL_EXPORT(SP_SetTextCharSpace, sact_SP_SetTextCharSpace), \
	    HLL_EXPORT(SP_SetTextPos, sact_SP_SetTextPos), \
	    HLL_EXPORT(SP_TextDraw, sact_SP_TextDraw), \
	    HLL_EXPORT(SP_TextClear, sact_SP_TextClear), \
	    HLL_EXPORT(SP_TextHome, sact_SP_TextHome), \
	    HLL_EXPORT(SP_TextNewLine, sact_SP_TextNewLine), \
	    HLL_TODO_EXPORT(SP_TextBackSpace, SACT2_SP_TextBackSpace), \
	    HLL_EXPORT(SP_TextCopy, sact_SP_TextCopy), \
	    HLL_EXPORT(SP_GetTextHomeX, sact_SP_GetTextHomeX), \
	    HLL_EXPORT(SP_GetTextHomeY, sact_SP_GetTextHomeY), \
	    HLL_EXPORT(SP_GetTextCharSpace, sact_SP_GetTextCharSpace), \
	    HLL_EXPORT(SP_GetTextPosX, sact_SP_GetTextPosX), \
	    HLL_EXPORT(SP_GetTextPosY, sact_SP_GetTextPosY), \
	    HLL_EXPORT(SP_GetTextLineSpace, sact_SP_GetTextLineSpace), \
	    HLL_EXPORT(SP_IsPtIn, sact_SP_IsPtIn), \
	    HLL_EXPORT(SP_IsPtInRect, sact_SP_IsPtInRect), \
	    HLL_EXPORT(GAME_MSG_GetNumof, sact_GAME_MSG_GetNumOf), \
	    HLL_TODO_EXPORT(GAME_MSG_Get, SACT2_GAME_MSG_Get), \
	    HLL_EXPORT(IntToZenkaku, sact_IntToZenkaku), \
	    HLL_EXPORT(IntToHankaku, sact_IntToHankaku), \
	    HLL_TODO_EXPORT(StringPopFront, SACT2_StringPopFront), \
	    HLL_EXPORT(Mouse_GetPos, sact_Mouse_GetPos), \
	    HLL_EXPORT(Mouse_SetPos, sact_Mouse_SetPos), \
	    HLL_EXPORT(Mouse_ClearWheel, mouse_clear_wheel), \
	    HLL_EXPORT(Mouse_GetWheel, mouse_get_wheel), \
	    HLL_EXPORT(Joypad_ClearKeyDownFlag, sact_Joypad_ClearKeyDownFlag), \
	    HLL_EXPORT(Joypad_IsKeyDown, sact_Joypad_IsKeyDown), \
	    HLL_TODO_EXPORT(Joypad_GetNumof, SACT2_Joypad_GetNumof), \
	    HLL_EXPORT(JoypadQuake_Set, SACT2_JoypadQuake_Set), \
	    HLL_EXPORT(Joypad_GetAnalogStickStatus, sact_Joypad_GetAnalogStickStatus), \
	    HLL_EXPORT(Joypad_GetDigitalStickStatus, sact_Joypad_GetDigitalStickStatus), \
	    HLL_EXPORT(Key_ClearFlag, sact_Key_ClearFlag), \
	    HLL_EXPORT(Key_IsDown, sact_Key_IsDown), \
	    HLL_EXPORT(Timer_Get, vm_time), \
	    HLL_EXPORT(CG_IsExist, sact_CG_IsExist), \
	    HLL_EXPORT(CG_GetMetrics, sact_CG_GetMetrics), \
	    HLL_TODO_EXPORT(CSV_Load, SACT2_CSV_Load), \
	    HLL_TODO_EXPORT(CSV_Save, SACT2_CSV_Save), \
	    HLL_TODO_EXPORT(CSV_SaveAs, SACT2_CSV_SaveAs), \
	    HLL_TODO_EXPORT(CSV_CountLines, SACT2_CSV_CountLines), \
	    HLL_TODO_EXPORT(CSV_CountColumns, SACT2_CSV_CountColumns), \
	    HLL_TODO_EXPORT(CSV_Get, SACT2_CSV_Get), \
	    HLL_TODO_EXPORT(CSV_Set, SACT2_CSV_Set), \
	    HLL_TODO_EXPORT(CSV_GetInt, SACT2_CSV_GetInt), \
	    HLL_TODO_EXPORT(CSV_SetInt, SACT2_CSV_SetInt), \
	    HLL_TODO_EXPORT(CSV_Realloc, SACT2_CSV_Realloc), \
	    HLL_EXPORT(Music_IsExist, sact_Music_IsExist), \
	    HLL_EXPORT(Music_Prepare, sact_Music_Prepare), \
	    HLL_EXPORT(Music_Unprepare, bgm_unprepare), \
	    HLL_EXPORT(Music_Play, bgm_play), \
	    HLL_EXPORT(Music_Stop, bgm_stop), \
	    HLL_EXPORT(Music_IsPlay, bgm_is_playing), \
	    HLL_EXPORT(Music_SetLoopCount, SACT2_Music_SetLoopCount), \
	    HLL_EXPORT(Music_GetLoopCount, SACT2_Music_GetLoopCount), \
	    HLL_TODO_EXPORT(Music_SetLoopStartPos, SACT2_Music_SetLoopStartPos), \
	    HLL_TODO_EXPORT(Music_SetLoopEndPos, SACT2_Music_SetLoopEndPos), \
	    HLL_EXPORT(Music_Fade, bgm_fade), \
	    HLL_EXPORT(Music_StopFade, SACT2_Music_StopFade), \
	    HLL_EXPORT(Music_IsFade, SACT2_Music_IsFade), \
	    HLL_EXPORT(Music_Pause, SACT2_Music_Pause), \
	    HLL_EXPORT(Music_Restart, SACT2_Music_Restart), \
	    HLL_EXPORT(Music_IsPause, SACT2_Music_IsPause), \
	    HLL_EXPORT(Music_GetPos, SACT2_Music_GetPos), \
	    HLL_EXPORT(Music_GetLength, SACT2_Music_GetLength), \
	    HLL_EXPORT(Music_GetSamplePos, SACT2_Music_GetSamplePos), \
	    HLL_EXPORT(Music_GetSampleLength, SACT2_Music_GetSampleLength), \
	    HLL_EXPORT(Music_Seek, SACT2_Music_Seek), \
	    HLL_EXPORT(Sound_IsExist, sact_Sound_IsExist), \
	    HLL_EXPORT(Sound_GetUnuseChannel, wav_get_unused_channel), \
	    HLL_EXPORT(Sound_Prepare, sact_Sound_Prepare), \
	    HLL_EXPORT(Sound_Unprepare, wav_unprepare), \
	    HLL_EXPORT(Sound_Play, wav_play), \
	    HLL_EXPORT(Sound_Stop, wav_stop), \
	    HLL_EXPORT(Sound_IsPlay, wav_is_playing), \
	    HLL_TODO_EXPORT(Sound_SetLoopCount, SACT2_Sound_SetLoopCount), \
	    HLL_TODO_EXPORT(Sound_GetLoopCount, SACT2_Sound_GetLoopCount), \
	    HLL_EXPORT(Sound_Fade, wav_fade), \
	    HLL_TODO_EXPORT(Sound_StopFade, SACT2_Sound_StopFade), \
	    HLL_TODO_EXPORT(Sound_IsFade, SACT2_Sound_IsFade), \
	    HLL_TODO_EXPORT(Sound_GetPos, SACT2_Sound_GetPos), \
	    HLL_TODO_EXPORT(Sound_GetLength, SACT2_Sound_GetLength), \
	    HLL_EXPORT(Sound_ReverseLR, wav_reverse_LR), \
	    HLL_TODO_EXPORT(Sound_GetVolume, SACT2_Sound_GetVolume), \
	    HLL_EXPORT(Sound_GetTimeLength, wav_get_time_length), \
	    HLL_TODO_EXPORT(Sound_GetGroupNum, SACT2_Sound_GetGroupNum), \
	    HLL_TODO_EXPORT(Sound_PrepareFromFile, SACT2_Sound_PrepareFromFile), \
	    HLL_EXPORT(System_GetDate, sact_System_GetDate), \
	    HLL_EXPORT(System_GetTime, sact_System_GetTime), \
	    HLL_TODO_EXPORT(CG_RotateRGB, SACT2_CG_RotateRGB), \
	    HLL_EXPORT(CG_BlendAMapBin, sact_CG_BlendAMapBin), \
	    HLL_TODO_EXPORT(Debug_Pause, SACT2_Debug_Pause), \
	    HLL_TODO_EXPORT(Debug_GetFuncStack, SACT2_Debug_GetFuncStack), \
	    HLL_EXPORT(SP_GetAMapValue, sact_SP_GetAMapValue), \
	    HLL_EXPORT(SP_GetPixelValue, sact_SP_GetPixelValue), \
	    HLL_EXPORT(SP_SetBrightness, SACT2_SP_SetBrightness), \
	    HLL_EXPORT(SP_GetBrightness, SACT2_SP_GetBrightness)

HLL_WARN_UNIMPLEMENTED( , void, SACTDX, SetVolumeMixerMasterGroupNum, int n);
HLL_WARN_UNIMPLEMENTED( , void, SACTDX, SetVolumeMixerSEGroupNum, int n);
HLL_WARN_UNIMPLEMENTED( , void, SACTDX, SetVolumeMixerBGMGroupNum, int n);
HLL_WARN_UNIMPLEMENTED(0, int,  SACTDX, Sound_GetGroupNumFromDataNum, int n);
//static int SACTDX_SP_CreateCopy(int nSP, int nSrcSp);
//static bool SACTDX_Joypad_GetAnalogStickStatus(int nNum, int nType, ref float pfDegree, ref float pfPower);
//static bool SACTDX_Joypad_GetDigitalStickStatus(int nNum, int nType, ref bool pbLeft, ref bool pbRight, ref bool pbUp, ref bool pbDown);
//static void SACTDX_FFT_rdft(ref array@float a);
//static void SACTDX_FFT_hanning_window(ref array@float a);
//static int SACTDX_Music_AnalyzeSampleData(ref array@float l, ref array@float r, ref array@int src, int chns, int bps);
//static void SACTDX_Key_ClearFlagNoCtrl(void);
//static void SACTDX_Key_ClearFlagOne(int nKeyCode);
//static bool SACTDX_VIEW_SetMode(int nMode);
//static int SACTDX_VIEW_GetMode(void);
//static bool SACTDX_DX_GetUsePower2Texture(void);
//static void SACTDX_DX_SetUsePower2Texture(bool bUse);

#define SACTDX_EXPORTS \
	HLL_EXPORT(SetVolumeMixerMasterGroupNum, SACTDX_SetVolumeMixerMasterGroupNum), \
	HLL_EXPORT(SetVolumeMixerSEGroupNum, SACTDX_SetVolumeMixerSEGroupNum), \
	HLL_EXPORT(SetVolumeMixerBGMGroupNum, SACTDX_SetVolumeMixerBGMGroupNum), \
	HLL_EXPORT(Sound_GetGroupNumFromDataNum, SACTDX_Sound_GetGroupNumFromDataNum), \
	HLL_TODO_EXPORT(SP_CreateCopy, SACTDX_SP_CreateCopy),	\
	HLL_TODO_EXPORT(Joypad_GetAnalogStickStatus, SACTDX_Joypad_GetAnalogStickStatus), \
	HLL_TODO_EXPORT(GetDigitalStickStatus, SACTDX_GetDigitalStickStatus), \
	HLL_TODO_EXPORT(FFT_rdft, SACTDX_FFT_rdft),		\
	HLL_TODO_EXPORT(FFT_hanning_window, SACTDX_FFT_hanning_window),	\
	HLL_TODO_EXPORT(Music_AnalyzeSampleData, SACTDX_Music_AnalyzeSampleData), \
	HLL_TODO_EXPORT(Key_ClearFlagNoCtrl, SACTDX_Key_ClearFlagNoCtrl), \
	HLL_TODO_EXPORT(Key_ClearFlagOne, SACTDX_Key_ClearFlagOne), \
	HLL_EXPORT(TRANS_Begin, sact_TRANS_Begin),	    \
	HLL_EXPORT(TRANS_Update, sact_TRANS_Update),	    \
	HLL_EXPORT(TRANS_End, sact_TRANS_End),	\
	HLL_TODO_EXPORT(VIEW_SetMode, SACTDX_VIEW_SetMode),	\
	HLL_TODO_EXPORT(VIEW_GetMode, SACTDX_VIEW_GetMode),	\
	HLL_TODO_EXPORT(DX_GetUsePower2Texture, SACTDX_DX_GetUsePower2Texture),	\
	HLL_TODO_EXPORT(DX_SetUsePower2Texture, SACTDX_DX_SetUsePower2Texture)

HLL_LIBRARY(SACT2, SACT_EXPORTS);
HLL_LIBRARY(SACTDX, SACT_EXPORTS, SACTDX_EXPORTS);
