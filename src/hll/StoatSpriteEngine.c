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
#include <ctype.h>

#include "system4.h"
#include "system4/ain.h"
#include "system4/ald.h"
#include "system4/cg.h"
#include "system4/string.h"
#include "system4/utfsjis.h"

#include "hll.h"
#include "audio.h"
#include "input.h"
#include "queue.h"
#include "gfx/gfx.h"
#include "sact.h"
#include "vm/page.h"
#include "xsystem4.h"
#include "sact.h"

HLL_WARN_UNIMPLEMENTED( , void, StoatSpriteEngine, JoypadQuake_Set, int num, int type, int magnitude);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_SetLoopCount, int ch, int count);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_GetLoopCount, int ch);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_StopFade, int ch);
HLL_WARN_UNIMPLEMENTED(0, int,  StoatSpriteEngine, Music_IsFade, int ch);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_Pause, int ch);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_Restart, int ch);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_IsPause, int ch);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_GetPos, int ch);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_GetLength, int ch);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_GetSamplePos, int ch);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_GetSampleLength, int ch);
HLL_WARN_UNIMPLEMENTED(1, int,  StoatSpriteEngine, Music_Seek, int ch, int pos);
HLL_WARN_UNIMPLEMENTED(0, int,  StoatSpriteEngine, SP_SetBrightness, int sp_no, int brightness);
HLL_WARN_UNIMPLEMENTED(0, int,  StoatSpriteEngine, SP_GetBrightness, int sp_no);
HLL_WARN_UNIMPLEMENTED( , void, StoatSpriteEngine, SetVolumeMixerMasterGroupNum, int n);
HLL_WARN_UNIMPLEMENTED( , void, StoatSpriteEngine, SetVolumeMixerSEGroupNum, int n);
HLL_WARN_UNIMPLEMENTED( , void, StoatSpriteEngine, SetVolumeMixerBGMGroupNum, int n);
HLL_WARN_UNIMPLEMENTED(0, int,  StoatSpriteEngine, Sound_GetGroupNumFromDataNum, int n);

static struct text_metrics text_sprite_tm = {
	.color = { .r = 255, .g = 255, .b = 255, .a = 255 },
	.outline_color = { .r = 0, .g = 0, .b = 0, .a = 255 },
	.size = 16,
	.weight = FW_NORMAL,
	.face = FONT_GOTHIC,
	.outline_left = 0,
	.outline_up = 0,
	.outline_right = 0,
	.outline_down = 0
};

static int extract_sjis_char(char *src, char *dst)
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

static bool StoatSpriteEngine_SP_SetTextSprite(int sp_no, struct string *text)
{
	if (text->size < 1)
		return false;

	char s[3];
	extract_sjis_char(text->text, s);
	int w = text_sprite_tm.size;
	int h = text_sprite_tm.size;
	// XXX: System40.exe lies about the height of half-width characters.
	//      E.g. a size 64 letter "H" is reported as 32 pixels tall.
	//      Probably doesn't matter...?
	if (isascii(s[0]))
		w /= 2;
	struct sact_sprite *sp = sact_create_sprite(sp_no, w, h, 0, 0, 0, 0);
	sprite_get_texture(sp); // XXX: force initialization of texture
	gfx_render_text(&sp->texture, (Point) { .x=0, .y=0 }, s, &text_sprite_tm, 0);
	sprite_dirty(sp);
	return true;
}

static void StoatSpriteEngine_SP_SetTextSpriteType(int type)
{
	text_sprite_tm.face = type;
}

static void StoatSpriteEngine_SP_SetTextSpriteSize(int size)
{
	text_sprite_tm.size = size;
}

static void StoatSpriteEngine_SP_SetTextSpriteColor(int r, int g, int b)
{
	text_sprite_tm.color = (SDL_Color) { .r = r, .g = g, .b = b, .a = 255 };
}

static void StoatSpriteEngine_SP_SetTextSpriteBoldWeight(float weight)
{
	//NOTICE("StoatSpriteEngine.SP_SetTextSpriteBoldWeight(%f)", weight);
}

static void StoatSpriteEngine_SP_SetTextSpriteEdgeWeight(float weight)
{
	text_sprite_tm.outline_left = weight;
	text_sprite_tm.outline_up = weight;
	text_sprite_tm.outline_right = weight;
	text_sprite_tm.outline_down = weight;
	//NOTICE("StoatSpriteEngine.SP_SetTextSpriteEdgeWeight(%f)", weight);
}

static void StoatSpriteEngine_SP_SetTextSpriteEdgeColor(int r, int g, int b)
{
	text_sprite_tm.outline_color = (SDL_Color) { .r = r, .g = g, .b = b, .a = 255 };
}

static bool StoatSpriteEngine_SP_SetDashTextSprite(int sp_no, int width, int height)
{
	// TODO: This function just draws a horizontal line on the sprite texture
	NOTICE("StoatSpriteEngine.SP_SetDashTextSprite(%d, %d, %d)", sp_no, width, height);
	return true;
}

//static void StoatSpriteEngine_FPS_SetShow(bool bShow);

static bool StoatSpriteEngine_KEY_GetState(int key)
{
	return key_is_down(key);
}

struct multisprite {
	int type;
	int no;
	int z;
	Point pos;
	Point default_pos;
	int alpha;
	int cg;
	bool cg_dirty;
	bool moving;
	struct {
		Point start;
		Point end;
		int start_alpha;
		int end_alpha;
		int begin_wait_time;
		int time;
		float move_accel;
	} move;
	struct {
		int no;
		int time;
	} trans;
	struct sact_sprite sp;
};

struct sp_table {
	struct multisprite **sprites;
	int nr_sprites;
};

#define NR_SP_TYPES 64

struct sp_table sp_types[NR_SP_TYPES] = {0};

static struct multisprite *multisprite_alloc(int type, int n)
{
	sp_types[type].sprites[n] = xcalloc(1, sizeof(struct multisprite));
	sp_types[type].sprites[n]->type = type;
	sp_types[type].sprites[n]->no = n;
	sp_types[type].sprites[n]->alpha = 255;
	sp_types[type].sprites[n]->z = 1;
	sprite_dirty(&sp_types[type].sprites[n]->sp);
	return sp_types[type].sprites[n];
}

static void realloc_sprite_table(struct sp_table *table, int n)
{
	table->sprites = xrealloc_array(table->sprites, table->nr_sprites, n, sizeof(struct multisprite*));
	table->nr_sprites = n;
}

static struct multisprite *multisprite_get(int type, int n)
{
	if (type < 0 || type >= NR_SP_TYPES)
		return NULL;
	if (n < 0)
		return NULL;
	if (n >= sp_types[type].nr_sprites) {
		realloc_sprite_table(&sp_types[type], n + 256);
	}
	if (!sp_types[type].sprites[n]) {
		multisprite_alloc(type, n);
		sprite_init(&sp_types[type].sprites[n]->sp, 1, 1, 0, 0, 0, 255);
	}
	return sp_types[type].sprites[n];
}

static void multisprite_free(struct multisprite *sp)
{
	if (!sp)
		return;
	if (sp_types[sp->type].sprites[sp->no] == NULL)
		VM_ERROR("Double free of multisprite");
	sp_types[sp->type].sprites[sp->no] = NULL;
	sprite_free(&sp->sp);
	free(sp);

}

static void multisprite_reset_cg(struct multisprite *sp)
{
	sp->cg = 0;
	sp->cg_dirty = true;
}

static int StoatSpriteEngine_Init(void *imain_system, int cg_cache_size)
{
	sact_Init(imain_system, cg_cache_size);
	for (int i = 0; i < NR_SP_TYPES; i++) {
		sp_types[i].nr_sprites = 16;
		sp_types[i].sprites = xcalloc(16, sizeof(struct multisprite*));
	}
	return 1;
}

static void StoatSpriteEngine_ModuleFini(void)
{
	sact_ModuleFini();
	for (int i = 0; i < NR_SP_TYPES; i++) {
		for (int j = 0; j < sp_types[i].nr_sprites; j++) {
			multisprite_free(sp_types[i].sprites[j]);
		}
	}
}

static void multisprite_update(struct multisprite *ms)
{
	sprite_set_z(&ms->sp, ms->z);
	sprite_set_pos(&ms->sp, ms->pos.x, ms->pos.y);
	sprite_set_blend_rate(&ms->sp, ms->alpha);
	if (ms->cg_dirty) {
		if (ms->cg) {
			sprite_set_cg(&ms->sp, ms->cg);
			sprite_set_show(&ms->sp, true);
		} else {
			sprite_set_show(&ms->sp, false);
		}
		ms->cg_dirty = false;
	}
}

static bool StoatSpriteEngine_MultiSprite_SetCG(int type, int n, int cg_no)
{
	if (cg_no && !sact_CG_IsExist(cg_no)) {
		WARNING("Invalid CG number: %d", cg_no);
		return false;
	}

	struct multisprite *ms = multisprite_get(type, n);
	ms->cg = cg_no;
	ms->cg_dirty = true;
	return true;
}

static bool StoatSpriteEngine_MultiSprite_SetPos(int type, int n, int x, int y)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	ms->pos.x = x;
	ms->pos.y = y;
	return true;
}

static bool StoatSpriteEngine_MultiSprite_SetDefaultPos(int type, int n, int x, int y)
{
	struct multisprite *sp = multisprite_get(type, n);
	if (!sp) return false;
	sp->default_pos.x = x;
	sp->default_pos.y = y;
	return true;
}

static bool StoatSpriteEngine_MultiSprite_SetZ(int type, int n, int z)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	ms->z = z;
	return true;
}

static bool StoatSpriteEngine_MultiSprite_SetAlpha(int type, int n, int alpha)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	ms->alpha = alpha;
	return true;
}

static bool StoatSpriteEngine_MultiSprite_SetTransitionInfo(int type, int n, int trans_no, int trans_time)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	// FIXME: when does this transition actually occur?
	ms->trans.no = trans_no;
	ms->trans.time = trans_time;
	return true;
}

//static bool StoatSpriteEngine_MultiSprite_SetLinkedMessageFrame(int nType, int nNum, bool bLink);
//static bool StoatSpriteEngine_MultiSprite_SetParentMessageFrameNum(int nType, int nNum, int nMessageAreaNum);
//static bool StoatSpriteEngine_MultiSprite_SetOriginPosMode(int nType, int nNum, int nMode);
//static bool StoatSpriteEngine_MultiSprite_SetText(int nType, int nNum, string Text);
//static bool StoatSpriteEngine_MultiSprite_SetCharSpace(int nType, int nNum, int nCharSpace);
//static bool StoatSpriteEngine_MultiSprite_CharSpriteProperty_SetSize(int nType, int nNum, int nSize);
//static bool StoatSpriteEngine_MultiSprite_CharSpriteProperty_SetColor(int nType, int nNum, int nR, int nG, int nB);
//static bool StoatSpriteEngine_MultiSprite_CharSpriteProperty_SetBoldWeight(int nType, int nNum, float fBoldWeight);
//static bool StoatSpriteEngine_MultiSprite_CharSpriteProperty_SetEdgeWeight(int nType, int nNum, float fEdgeWeight);
//static bool StoatSpriteEngine_MultiSprite_CharSpriteProperty_SetEdgeColor(int nType, int nNum, int nR, int nG, int nB);

static bool StoatSpriteEngine_MultiSprite_GetTransitionInfo(int type, int n, int *trans_no, int *trans_time)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	*trans_no = ms->trans.no;
	*trans_time = ms->trans.time;
	return true;
}

static int StoatSpriteEngine_MultiSprite_GetCG(int type, int n)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	return ms->cg;
}

static bool StoatSpriteEngine_MultiSprite_GetPos(int type, int n, int *x, int *y)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	*x = ms->pos.x;
	*y = ms->pos.y;
	return true;
}

static bool StoatSpriteEngine_MultiSprite_GetAlpha(int type, int n, int *alpha)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	*alpha = ms->alpha;
	return true;
}

static bool StoatSpriteEngine_MultiSprite_ResetDefaultPos(int type, int n)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	ms->pos = ms->default_pos;
	return true;
}

static bool StoatSpriteEngine_MultiSprite_ResetAllDefaultPos(int type)
{
	for (int i = 0; i < sp_types[type].nr_sprites; i++) {
		if (sp_types[type].sprites[i]) {
			sp_types[type].sprites[i]->pos = sp_types[type].sprites[i]->default_pos;
		}
	}
	return true;
}

static void StoatSpriteEngine_MultiSprite_ResetAllCG(void)
{
	for (int i = 0; i < NR_SP_TYPES; i++) {
		for (int j = 0; j < sp_types[i].nr_sprites; j++) {
			if (sp_types[i].sprites[j]) {
				multisprite_reset_cg(sp_types[i].sprites[j]);
			}
		}
	}
}

static void StoatSpriteEngine_MultiSprite_SetAllShowMessageFrame(bool show)
{
	for (int i = 0; i < sp_types[0].nr_sprites; i++) {
		if (sp_types[0].sprites[i]) {
			sprite_set_show(&sp_types[0].sprites[i]->sp, show);
		}
	}
}

static bool StoatSpriteEngine_MultiSprite_BeginMove(int type, int n, int x0, int y0, int x1, int y1, int t, int begin_wait_time, float move_accel)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	ms->move.start.x = x0;
	ms->move.start.y = y0;
	ms->move.start_alpha = ms->alpha;
	ms->move.end.x = x1;
	ms->move.end.y = y1;
	ms->move.end_alpha = ms->alpha;
	ms->move.time = t;
	ms->move.begin_wait_time = begin_wait_time;
	ms->move.move_accel = move_accel;
	ms->moving = true;
	return true;
}

static bool StoatSpriteEngine_MultiSprite_BeginMoveWithAlpha(int type, int n, int x0, int y0, int a0, int x1, int y1, int a1, int t, int begin_wait_time, float move_accel)
{
	struct multisprite *ms = multisprite_get(type, n);
	if (!ms) return false;
	ms->move.start.x = x0;
	ms->move.start.y = y0;
	ms->move.start_alpha = a0;
	ms->move.end.x = x1;
	ms->move.end.y = y1;
	ms->move.end_alpha = a1;
	ms->move.time = t;
	ms->move.begin_wait_time = begin_wait_time;
	ms->move.move_accel = move_accel;
	ms->moving = true;
	return true;
}

static int StoatSpriteEngine_MultiSprite_GetMaxMoveTotalTime(void)
{
	int t = 0;
	for (int i = 0; i < NR_SP_TYPES; i++) {
		for (int j = 0; j < sp_types[i].nr_sprites; j++) {
			struct multisprite *ms = sp_types[i].sprites[j];
			if (ms && ms->moving) {
				t = max(t, ms->move.time + ms->move.begin_wait_time);
			}
		}
	}
	return t;
}

static void multisprite_set_move_current_time(struct multisprite *ms, int time)
{
	if (!time || time <= ms->move.begin_wait_time) {
		ms->pos = ms->move.start;
		return;
	}

	// adjust for begin wait
	time -= ms->move.begin_wait_time;

	if (time >= ms->move.time) {
		ms->pos = ms->move.end;
		return;
	}

	// calculate (x,y) at time/move_time
	int d_x = ms->move.end.x - ms->move.start.x;
	int d_y = ms->move.end.y - ms->move.start.y;
	double percent = (double)time / ms->move.time;
	// TODO: acceleration
	ms->pos.x = ms->move.start.x + (d_x * percent);
	ms->pos.y = ms->move.start.y + (d_y * percent);
}

static void StoatSpriteEngine_MultiSprite_SetAllMoveCurrentTime(int time)
{
	for (int i = 0; i < NR_SP_TYPES; i++) {
		for (int j = 0; j < sp_types[i].nr_sprites; j++) {
			struct multisprite *ms = sp_types[i].sprites[j];
			if (ms && ms->moving) {
				multisprite_set_move_current_time(ms, time);
			}
		}
	}
}

static void StoatSpriteEngine_MultiSprite_EndAllMove(void)
{
	for (int i = 0; i < NR_SP_TYPES; i++) {
		for (int j = 0; j < sp_types[i].nr_sprites; j++) {
			struct multisprite *ms = sp_types[i].sprites[j];
			if (ms && ms->moving) {
				ms->pos = ms->move.end;
			}
		}
	}
}

static bool StoatSpriteEngine_MultiSprite_UpdateView(void)
{
	for (int i = 0; i < NR_SP_TYPES; i++) {
		for (int j = 0; j < sp_types[i].nr_sprites; j++) {
			if (sp_types[i].sprites[j]) {
				multisprite_update(sp_types[i].sprites[j]);
			}
		}
	}
	return true;
}

//static void StoatSpriteEngine_MultiSprite_Rebuild(void);

static bool StoatSpriteEngine_MultiSprite_Encode(struct page **data)
{
	union vm_value dim = { .i = 1 };
	*data = alloc_array(1, &dim, AIN_ARRAY_INT, 0, false);
	return true;
}

HLL_WARN_UNIMPLEMENTED(true, bool, StoatSpriteEngine, MultiSprite_Decode, struct page **data);

static int StoatSpriteEngine_SYSTEM_IsResetOnce(void)
{
	return 0;
}

HLL_LIBRARY(StoatSpriteEngine,
	    HLL_EXPORT(_ModuleFini, StoatSpriteEngine_ModuleFini), \
	    HLL_EXPORT(SetVolumeMixerMasterGroupNum, StoatSpriteEngine_SetVolumeMixerMasterGroupNum), \
	    HLL_EXPORT(SetVolumeMixerSEGroupNum, StoatSpriteEngine_SetVolumeMixerSEGroupNum), \
	    HLL_EXPORT(SetVolumeMixerBGMGroupNum, StoatSpriteEngine_SetVolumeMixerBGMGroupNum), \
	    HLL_EXPORT(Init, StoatSpriteEngine_Init), \
	    HLL_TODO_EXPORT(Error, SACT2_Error), \
	    HLL_EXPORT(SetWP_Color, sact_SetWP_Color), \
	    HLL_EXPORT(GetScreenWidth, sact_GetScreenWidth), \
	    HLL_EXPORT(GetScreenHeight, sact_GetScreenHeight), \
	    HLL_EXPORT(GetMainSurfaceNumber, sact_GetMainSurfaceNumber), \
	    HLL_EXPORT(Update, sact_Update), \
	    HLL_EXPORT(SP_GetUnuseNum, sact_SP_GetUnuseNum), \
	    HLL_EXPORT(SP_Count, sact_SP_Count), \
	    HLL_EXPORT(SP_Enum, sact_SP_Enum), \
	    HLL_EXPORT(SP_GetMaxZ, sact_SP_GetMaxZ), \
	    HLL_EXPORT(SP_SetCG, sact_SP_SetCG), \
	    HLL_TODO_EXPORT(SP_SetCGFromFile, SACT2_SP_SetCGFromFile), \
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
	    HLL_EXPORT(SP_SetTextSprite, StoatSpriteEngine_SP_SetTextSprite), \
	    HLL_EXPORT(SP_SetTextSpriteType, StoatSpriteEngine_SP_SetTextSpriteType), \
	    HLL_EXPORT(SP_SetTextSpriteSize, StoatSpriteEngine_SP_SetTextSpriteSize), \
	    HLL_EXPORT(SP_SetTextSpriteColor, StoatSpriteEngine_SP_SetTextSpriteColor), \
	    HLL_EXPORT(SP_SetTextSpriteBoldWeight, StoatSpriteEngine_SP_SetTextSpriteBoldWeight), \
	    HLL_EXPORT(SP_SetTextSpriteEdgeWeight, StoatSpriteEngine_SP_SetTextSpriteEdgeWeight), \
	    HLL_EXPORT(SP_SetTextSpriteEdgeColor, StoatSpriteEngine_SP_SetTextSpriteEdgeColor), \
	    HLL_EXPORT(SP_SetDashTextSprite, StoatSpriteEngine_SP_SetDashTextSprite), \
	    HLL_TODO_EXPORT(FPS_SetShow, StoatSpriteEngine_FPS_SetShow), \
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
	    HLL_EXPORT(JoypadQuake_Set, StoatSpriteEngine_JoypadQuake_Set), \
	    HLL_EXPORT(Joypad_GetAnalogStickStatus, sact_Joypad_GetAnalogStickStatus), \
	    HLL_EXPORT(Joypad_GetDigitalStickStatus, sact_Joypad_GetDigitalStickStatus), \
	    HLL_EXPORT(Key_ClearFlag, sact_Key_ClearFlag), \
	    HLL_EXPORT(Key_IsDown, sact_Key_IsDown), \
	    HLL_EXPORT(KEY_GetState, StoatSpriteEngine_KEY_GetState), \
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
	    HLL_EXPORT(Music_SetLoopCount, StoatSpriteEngine_Music_SetLoopCount), \
	    HLL_EXPORT(Music_GetLoopCount, StoatSpriteEngine_Music_GetLoopCount), \
	    HLL_TODO_EXPORT(Music_SetLoopStartPos, SACT2_Music_SetLoopStartPos), \
	    HLL_TODO_EXPORT(Music_SetLoopEndPos, SACT2_Music_SetLoopEndPos), \
	    HLL_EXPORT(Music_Fade, bgm_fade), \
	    HLL_EXPORT(Music_StopFade, StoatSpriteEngine_Music_StopFade), \
	    HLL_EXPORT(Music_IsFade, StoatSpriteEngine_Music_IsFade), \
	    HLL_EXPORT(Music_Pause, StoatSpriteEngine_Music_Pause), \
	    HLL_EXPORT(Music_Restart, StoatSpriteEngine_Music_Restart), \
	    HLL_EXPORT(Music_IsPause, StoatSpriteEngine_Music_IsPause), \
	    HLL_EXPORT(Music_GetPos, StoatSpriteEngine_Music_GetPos), \
	    HLL_EXPORT(Music_GetLength, StoatSpriteEngine_Music_GetLength), \
	    HLL_EXPORT(Music_GetSamplePos, StoatSpriteEngine_Music_GetSamplePos), \
	    HLL_EXPORT(Music_GetSampleLength, StoatSpriteEngine_Music_GetSampleLength), \
	    HLL_EXPORT(Music_Seek, StoatSpriteEngine_Music_Seek), \
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
	    HLL_EXPORT(Sound_GetGroupNumFromDataNum, StoatSpriteEngine_Sound_GetGroupNumFromDataNum), \
	    HLL_TODO_EXPORT(Sound_PrepareFromFile, SACT2_Sound_PrepareFromFile), \
	    HLL_EXPORT(System_GetDate, sact_System_GetDate), \
	    HLL_EXPORT(System_GetTime, sact_System_GetTime), \
	    HLL_TODO_EXPORT(CG_RotateRGB, SACT2_CG_RotateRGB), \
	    HLL_EXPORT(CG_BlendAMapBin, sact_CG_BlendAMapBin), \
	    HLL_TODO_EXPORT(Debug_Pause, SACT2_Debug_Pause), \
	    HLL_TODO_EXPORT(Debug_GetFuncStack, SACT2_Debug_GetFuncStack), \
	    HLL_EXPORT(SP_GetAMapValue, sact_SP_GetAMapValue), \
	    HLL_EXPORT(SP_GetPixelValue, sact_SP_GetPixelValue), \
	    HLL_EXPORT(SP_SetBrightness, StoatSpriteEngine_SP_SetBrightness), \
	    HLL_EXPORT(SP_GetBrightness, StoatSpriteEngine_SP_GetBrightness), \
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
	    HLL_TODO_EXPORT(DX_SetUsePower2Texture, SACTDX_DX_SetUsePower2Texture), \
	    HLL_EXPORT(MultiSprite_SetCG, StoatSpriteEngine_MultiSprite_SetCG), \
	    HLL_EXPORT(MultiSprite_SetPos, StoatSpriteEngine_MultiSprite_SetPos), \
	    HLL_EXPORT(MultiSprite_SetDefaultPos, StoatSpriteEngine_MultiSprite_SetDefaultPos), \
	    HLL_EXPORT(MultiSprite_SetZ, StoatSpriteEngine_MultiSprite_SetZ),	\
	    HLL_EXPORT(MultiSprite_SetAlpha, StoatSpriteEngine_MultiSprite_SetAlpha), \
	    HLL_EXPORT(MultiSprite_SetTransitionInfo, StoatSpriteEngine_MultiSprite_SetTransitionInfo), \
	    HLL_TODO_EXPORT(MultiSprite_SetLinkedMessageFrame, StoatSpriteEngine_MultiSprite_SetLinkedMessageFrame), \
	    HLL_TODO_EXPORT(MultiSprite_SetParentMessageFrameNum, StoatSpriteEngine_MultiSprite_SetParentMessageFrameNum), \
	    HLL_TODO_EXPORT(MultiSprite_SetOriginPosMode, StoatSpriteEngine_MultiSprite_SetOriginPosMode), \
	    HLL_TODO_EXPORT(MultiSprite_SetText, StoatSpriteEngine_MultiSprite_SetText), \
	    HLL_TODO_EXPORT(MultiSprite_SetCharSpace, StoatSpriteEngine_MultiSprite_SetCharSpace), \
	    HLL_TODO_EXPORT(MultiSprite_CharSpriteProperty_SetSize, StoatSpriteEngine_MultiSprite_CharSpriteProperty_SetSize), \
	    HLL_TODO_EXPORT(MultiSprite_CharSpriteProperty_SetColor, StoatSpriteEngine_MultiSprite_CharSpriteProperty_SetColor), \
	    HLL_TODO_EXPORT(MultiSprite_CharSpriteProperty_SetBoldWeight, StoatSpriteEngine_MultiSprite_CharSpriteProperty_SetBoldWeight), \
	    HLL_TODO_EXPORT(MultiSprite_CharSpriteProperty_SetEdgeWeight, StoatSpriteEngine_MultiSprite_CharSpriteProperty_SetEdgeWeight), \
	    HLL_TODO_EXPORT(MultiSprite_CharSpriteProperty_SetEdgeColor, StoatSpriteEngine_MultiSprite_CharSpriteProperty_SetEdgeColor), \
	    HLL_EXPORT(MultiSprite_GetTransitionInfo, StoatSpriteEngine_MultiSprite_GetTransitionInfo), \
	    HLL_EXPORT(MultiSprite_GetCG, StoatSpriteEngine_MultiSprite_GetCG), \
	    HLL_EXPORT(MultiSprite_GetPos, StoatSpriteEngine_MultiSprite_GetPos), \
	    HLL_EXPORT(MultiSprite_GetAlpha, StoatSpriteEngine_MultiSprite_GetAlpha), \
	    HLL_EXPORT(MultiSprite_ResetDefaultPos, StoatSpriteEngine_MultiSprite_ResetDefaultPos), \
	    HLL_EXPORT(MultiSprite_ResetAllDefaultPos, StoatSpriteEngine_MultiSprite_ResetAllDefaultPos), \
	    HLL_EXPORT(MultiSprite_ResetAllCG, StoatSpriteEngine_MultiSprite_ResetAllCG), \
	    HLL_EXPORT(MultiSprite_SetAllShowMessageFrame, StoatSpriteEngine_MultiSprite_SetAllShowMessageFrame), \
	    HLL_EXPORT(MultiSprite_BeginMove, StoatSpriteEngine_MultiSprite_BeginMove), \
	    HLL_EXPORT(MultiSprite_BeginMoveWithAlpha, StoatSpriteEngine_MultiSprite_BeginMoveWithAlpha), \
	    HLL_EXPORT(MultiSprite_GetMaxMoveTotalTime, StoatSpriteEngine_MultiSprite_GetMaxMoveTotalTime), \
	    HLL_EXPORT(MultiSprite_SetAllMoveCurrentTime, StoatSpriteEngine_MultiSprite_SetAllMoveCurrentTime), \
	    HLL_EXPORT(MultiSprite_EndAllMove, StoatSpriteEngine_MultiSprite_EndAllMove), \
	    HLL_EXPORT(MultiSprite_UpdateView, StoatSpriteEngine_MultiSprite_UpdateView), \
	    HLL_TODO_EXPORT(MultiSprite_Rebuild, StoatSpriteEngine_MultiSprite_Rebuild), \
	    HLL_EXPORT(MultiSprite_Encode, StoatSpriteEngine_MultiSprite_Encode), \
	    HLL_EXPORT(MultiSprite_Decode, StoatSpriteEngine_MultiSprite_Decode), \
	    HLL_EXPORT(SYSTEM_IsResetOnce, StoatSpriteEngine_SYSTEM_IsResetOnce));


