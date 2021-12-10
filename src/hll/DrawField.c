/* Copyright (C) 2021 kichikuou <KichikuouChrome@gmail.com>
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

#include "system4/string.h"

#include "dungeon/dgn.h"
#include "dungeon/dungeon.h"
#include "dungeon/map.h"  // shouldn't be needed
#include "hll.h"

extern struct dungeon_context *current_context;

static struct dgn_cell *dungeon_get_cell(int surface, int x, int y, int z)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn || !dgn_is_in_map(ctx->dgn, x, y, z))
		return NULL;
	return dgn_cell_at(ctx->dgn, x, y, z);
}

static int DrawField_Init(int surface)
{
	return dungeon_init(DRAW_FIELD, surface);
}

static void DrawField_Release(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return;
	dungeon_fini();
}

static void DrawField_SetDrawFlag(int surface, int flag)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return;
	ctx->draw_enabled = flag;
}

static bool DrawField_GetDrawFlag(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return false;
	return ctx->draw_enabled;
}

static int DrawField_LoadFromFile(int surface, struct string *file_name, int num)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return false;
	return dungeon_load_dungeon(ctx, file_name->text, num);
}

static int DrawField_LoadTexture(int surface, struct string *file_name)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return false;
	return dungeon_load_texture(ctx, file_name->text);
}

//int DrawField_LoadEventTexture(int nSurface, struct string *szFileName);
//bool DrawField_BeginLoad(int nSurface, struct string *szFileName, int nNum);
//bool DrawField_IsLoadEnd(int nSurface);
//bool DrawField_StopLoad(int nSurface);
//float DrawField_GetLoadPercent(int nSurface);
//bool DrawField_IsLoadSucceeded(int nSurface);
//bool DrawField_BeginLoadTexture(int nSurface, struct string *szFileName);
//bool DrawField_IsLoadTextureEnd(int nSurface);
//bool DrawField_StopLoadTexture(int nSurface);
//float DrawField_GetLoadTexturePercent(int nSurface);
//bool DrawField_IsLoadTextureSucceeded(int nSurface);
//void DrawField_UpdateTexture(int nSurface);
//void DrawField_SetCamera(int nSurface, float fX, float fY, float fZ, float fAngle, float fAngleP);

static int DrawField_GetMapX(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return -1;
	return ctx->dgn->size_x;
}

static int DrawField_GetMapY(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return -1;
	return ctx->dgn->size_y;
}

static int DrawField_GetMapZ(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return -1;
	return ctx->dgn->size_z;
}

static int DrawField_IsInMap(int surface, int x, int y, int z)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return -1;
	return dgn_is_in_map(ctx->dgn, x, y, z);
}

#define CELL_GETTER(name, defval, expr) \
	static int name(int surface, int x, int y, int z) \
	{ \
		struct dgn_cell *cell = dungeon_get_cell(surface, x, y, z); \
		if (!cell) \
			return defval; \
		return expr; \
	}

CELL_GETTER(DrawField_IsFloor, 0, cell->floor != -1);
CELL_GETTER(DrawField_IsStair, 0, cell->stairs_texture != -1);
CELL_GETTER(DrawField_IsStairN, 0, cell->stairs_texture != -1 && cell->stairs_orientation == 0);
CELL_GETTER(DrawField_IsStairW, 0, cell->stairs_texture != -1 && cell->stairs_orientation == 1);
CELL_GETTER(DrawField_IsStairS, 0, cell->stairs_texture != -1 && cell->stairs_orientation == 2);
CELL_GETTER(DrawField_IsStairE, 0, cell->stairs_texture != -1 && cell->stairs_orientation == 3);
CELL_GETTER(DrawField_IsWallN, 0, cell->north_wall != -1);
CELL_GETTER(DrawField_IsWallW, 0, cell->west_wall != -1);
CELL_GETTER(DrawField_IsWallS, 0, cell->south_wall != -1);
CELL_GETTER(DrawField_IsWallE, 0, cell->east_wall != -1);
CELL_GETTER(DrawField_IsDoorN, 0, cell->north_door != -1);
CELL_GETTER(DrawField_IsDoorW, 0, cell->west_door != -1);
CELL_GETTER(DrawField_IsDoorS, 0, cell->south_door != -1);
CELL_GETTER(DrawField_IsDoorE, 0, cell->east_door != -1);
CELL_GETTER(DrawField_IsEnter, 0, cell->enterable);
CELL_GETTER(DrawField_IsEnterN, 0, cell->enterable_north);
CELL_GETTER(DrawField_IsEnterW, 0, cell->enterable_west);
CELL_GETTER(DrawField_IsEnterS, 0, cell->enterable_south);
CELL_GETTER(DrawField_IsEnterE, 0, cell->enterable_east);
CELL_GETTER(DrawField_GetEvFloor, 0, cell->floor_event);
CELL_GETTER(DrawField_GetEvWallN, 0, cell->north_event);
CELL_GETTER(DrawField_GetEvWallW, 0, cell->west_event);
CELL_GETTER(DrawField_GetEvWallS, 0, cell->south_event);
CELL_GETTER(DrawField_GetEvWallE, 0, cell->east_event);
CELL_GETTER(DrawField_GetEvFloor2, 0, cell->floor_event2);
CELL_GETTER(DrawField_GetEvWallN2, 0, cell->north_event2);
CELL_GETTER(DrawField_GetEvWallW2, 0, cell->west_event2);
CELL_GETTER(DrawField_GetEvWallS2, 0, cell->south_event2);
CELL_GETTER(DrawField_GetEvWallE2, 0, cell->east_event2);
CELL_GETTER(DrawField_GetTexFloor, -1, cell->floor);
CELL_GETTER(DrawField_GetTexCeiling, -1, cell->ceiling);
CELL_GETTER(DrawField_GetTexWallN, -1, cell->north_wall);
CELL_GETTER(DrawField_GetTexWallW, -1, cell->west_wall);
CELL_GETTER(DrawField_GetTexWallS, -1, cell->south_wall);
CELL_GETTER(DrawField_GetTexWallE, -1, cell->east_wall);
CELL_GETTER(DrawField_GetTexDoorN, -1, cell->north_door);
CELL_GETTER(DrawField_GetTexDoorW, -1, cell->west_door);
CELL_GETTER(DrawField_GetTexDoorS, -1, cell->south_door);
CELL_GETTER(DrawField_GetTexDoorE, -1, cell->east_door);

//bool DrawField_GetDoorNAngle(int nSurface, int nX, int nY, int nZ, float *pfAngle);
//bool DrawField_GetDoorWAngle(int nSurface, int nX, int nY, int nZ, float *pfAngle);
//bool DrawField_GetDoorSAngle(int nSurface, int nX, int nY, int nZ, float *pfAngle);
//bool DrawField_GetDoorEAngle(int nSurface, int nX, int nY, int nZ, float *pfAngle);

#define CELL_SETTER(name, type, expr, update_map) \
	static void name(int surface, int x, int y, int z, type value) \
	{ \
		struct dungeon_context *ctx = dungeon_get_context(surface); \
		if (!ctx || !ctx->dgn || !dgn_is_in_map(ctx->dgn, x, y, z)) \
			return; \
		struct dgn_cell *cell = dgn_cell_at(ctx->dgn, x, y, z); \
		expr = value; \
		if (update_map) \
			dungeon_map_update_cell(ctx, x, y, z);	\
	}

//bool DrawField_SetDoorNAngle(int nSurface, int nX, int nY, int nZ, float fAngle);
//bool DrawField_SetDoorWAngle(int nSurface, int nX, int nY, int nZ, float fAngle);
//bool DrawField_SetDoorSAngle(int nSurface, int nX, int nY, int nZ, float fAngle);
//bool DrawField_SetDoorEAngle(int nSurface, int nX, int nY, int nZ, float fAngle);
CELL_SETTER(DrawField_SetEvFloor, int, cell->floor_event, true);
CELL_SETTER(DrawField_SetEvWallN, int, cell->north_event, true);
CELL_SETTER(DrawField_SetEvWallW, int, cell->west_event, true);
CELL_SETTER(DrawField_SetEvWallS, int, cell->south_event, true);
CELL_SETTER(DrawField_SetEvWallE, int, cell->east_event, true);
CELL_SETTER(DrawField_SetEvFloor2, int, cell->floor_event2, false);
CELL_SETTER(DrawField_SetEvWallN2, int, cell->north_event2, false);
CELL_SETTER(DrawField_SetEvWallW2, int, cell->west_event2, false);
CELL_SETTER(DrawField_SetEvWallS2, int, cell->south_event2, false);
CELL_SETTER(DrawField_SetEvWallE2, int, cell->east_event2, false);
//void DrawField_SetEvMag(int surface, int x, int y, int z, float mag);
CELL_SETTER(DrawField_SetEvRate, int, cell->event_blend_rate, false);
CELL_SETTER(DrawField_SetEnter, int, cell->enterable, false);
CELL_SETTER(DrawField_SetEnterN, int, cell->enterable_north, true);
CELL_SETTER(DrawField_SetEnterW, int, cell->enterable_west, true);
CELL_SETTER(DrawField_SetEnterS, int, cell->enterable_south, true);
CELL_SETTER(DrawField_SetEnterE, int, cell->enterable_east, true);
HLL_WARN_UNIMPLEMENTED(false, bool, DrawField, SetDoorNLock, int nSurface, int nX, int nY, int nZ, int nLock);
HLL_WARN_UNIMPLEMENTED(false, bool, DrawField, SetDoorWLock, int nSurface, int nX, int nY, int nZ, int nLock);
HLL_WARN_UNIMPLEMENTED(false, bool, DrawField, SetDoorSLock, int nSurface, int nX, int nY, int nZ, int nLock);
HLL_WARN_UNIMPLEMENTED(false, bool, DrawField, SetDoorELock, int nSurface, int nX, int nY, int nZ, int nLock);
//bool DrawField_GetDoorNLock(int nSurface, int nX, int nY, int nZ, int *pnLock);
//bool DrawField_GetDoorWLock(int nSurface, int nX, int nY, int nZ, int *pnLock);
//bool DrawField_GetDoorSLock(int nSurface, int nX, int nY, int nZ, int *pnLock);
//bool DrawField_GetDoorELock(int nSurface, int nX, int nY, int nZ, int *pnLock);
CELL_SETTER(DrawField_SetTexFloor, int, cell->floor, true);
CELL_SETTER(DrawField_SetTexCeiling, int, cell->ceiling, false);
CELL_SETTER(DrawField_SetTexWallN, int, cell->north_wall, true);
CELL_SETTER(DrawField_SetTexWallW, int, cell->west_wall, true);
CELL_SETTER(DrawField_SetTexWallS, int, cell->south_wall, true);
CELL_SETTER(DrawField_SetTexWallE, int, cell->east_wall, true);
CELL_SETTER(DrawField_SetTexDoorN, int, cell->north_door, true);
CELL_SETTER(DrawField_SetTexDoorW, int, cell->west_door, true);
CELL_SETTER(DrawField_SetTexDoorS, int, cell->south_door, true);
CELL_SETTER(DrawField_SetTexDoorE, int, cell->east_door, true);

//void DrawField_SetTexStair(int nSurface, int nX, int nY, int nZ, int nTexture, int nType);
//void DrawField_SetShadowTexFloor(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetShadowTexCeiling(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetShadowTexWallN(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetShadowTexWallW(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetShadowTexWallS(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetShadowTexWallE(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetShadowTexDoorN(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetShadowTexDoorW(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetShadowTexDoorS(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetShadowTexDoorE(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_CalcShadowMap(int nSurface);
//void DrawField_DrawMap(int nSurface, int nSprite);
//void DrawField_SetMapAllViewFlag(int nSurfacce, int nFlag);
//void DrawField_SetDrawMapFloor(int nSurface, int nFloor);
//void DrawField_DrawLMap(int nSurface, int nSprite);
//void DrawField_SetMapCG(int nSurface, int nIndex, int nSprite);
//void DrawField_SetDrawLMapFloor(int nSurface, int nFloor);
HLL_WARN_UNIMPLEMENTED( , void, DrawField, SetPlayerPos, int nSurface, int nX, int nY, int nZ);
//void DrawField_SetLooked(int nSurface, int nX, int nY, int nZ, bool bFlag);
//bool DrawField_SetHideDrawMapFloor(int nSurface, int nFloor, bool bHide);
//bool DrawField_SetHideDrawMapWall(int nSurface, int nWall, bool bHide);
//void DrawField_SetDrawHalfFlag(int nSurface, int nFlag);
//int DrawField_GetDrawHalfFlag(int nSurface);
//void DrawField_SetInterlaceMode(int nSurface, int nFlag);
//bool DrawField_SetDirect3DMode(int nSurface, int nFlag);
//bool DrawField_GetDirect3DMode(int nSurface);
//void DrawField_SaveDrawSettingFlag(int nDirect3D, int nInterlace, int nHalf);

static void DrawField_SetPerspective(int surface, int width, int height, float near, float far, float deg)
{
	WARNING("DrawField_SetPerspective %dx%d %f-%f %f", width, height, near, far, deg);
}

//void DrawField_SetDrawShadowMap(int nSurface, bool bDrawShadowMap);
//int DrawField_CalcNumofFloor(int nSurface);
//int DrawField_CalcNumofWalk(int nSurface);
//int DrawField_CalcNumofWalk2(int nSurface);
//bool DrawField_IsPVSData(int nSurface);
//float DrawField_CosDeg(float fDeg);
//float DrawField_SinDeg(float fDeg);
//float DrawField_TanDeg(float fDeg);
//float DrawField_Atan2(float fY, float fX);
//bool DrawField_TransPos2DToPos3DOnPlane(int nSurface, int nScreenX, int nScreenY, float fPlaneY, float *pfX, float *pfY, float *pfZ);
//bool DrawField_TransPos3DToPos2D(int nSurace, float fX, float fY, float fZ, int *pnScreenX, int *pnScreenY);

static int DrawField_GetCharaNumMax(int surface)
{
	return 256;
}

static void DrawField_SetCharaSprite(int surface, int num, int sprite)
{
	WARNING("DrawField_SetCharaSprite num=%d, sprite=%d", num, sprite);
}

static void DrawField_SetCharaPos(int surface, int num, float x, float y, float z)
{
	WARNING("DrawField_SetCharaPos num=%d, (%f, %f, %f)", num, x, y, z);
}

static void DrawField_SetCharaCG(int surface, int num, int cg)
{
	WARNING("DrawField_SetCharaCG num=%d, cg=%d", num, cg);
}

static void DrawField_SetCharaCGInfo(int surface, int num, int numofCharaX, int numofCharaY)
{
	WARNING("DrawField_SetCharaCGInfo num=%d, #charaX=%d, #charaY=%d", num, numofCharaX, numofCharaY);
}

//void DrawField_SetCharaZBias(int nSurface, float fZBias0, float fZBias1, float fZBias2, float fZBias3);

static void DrawField_SetCharaShow(int surface, int num, bool show)
{
	WARNING("DrawField_SetCharaShow num=%d, %d", num, show);
}

HLL_WARN_UNIMPLEMENTED( , void, DrawField, SetCenterOffsetY, int nSurface, float fY);
//void DrawField_SetBuilBoard(int nSurface, int nNum, int nSprite);
//void DrawField_SetSphereTheta(int nSurface, float fX, float fY, float fZ);
//void DrawField_SetSphereColor(int nSurface, float fTop, float fBottom);
//bool DrawField_GetSphereTheta(int nSurface, float *fX, float *fY, float *fZ);
//bool DrawField_GetSphereColor(int nSurface, float *fTop, float *fBottom);
//bool DrawField_GetBackColor(int nSurface, int *pnR, int *pnG, int *pnB);
//void DrawField_SetBackColor(int nSurface, int nR, int nG, int nB);
//int DrawField_GetPolyObj(int nSurface, int nX, int nY, int nZ);
//float DrawField_GetPolyObjMag(int nSurface, int nX, int nY, int nZ);
//float DrawField_GetPolyObjRotateH(int nSurface, int nX, int nY, int nZ);
//float DrawField_GetPolyObjRotateP(int nSurface, int nX, int nY, int nZ);
//float DrawField_GetPolyObjRotateB(int nSurface, int nX, int nY, int nZ);
//float DrawField_GetPolyObjOffsetX(int nSurface, int nX, int nY, int nZ);
//float DrawField_GetPolyObjOffsetY(int nSurface, int nX, int nY, int nZ);
//float DrawField_GetPolyObjOffsetZ(int nSurface, int nX, int nY, int nZ);
//void DrawField_SetPolyObj(int nSurface, int nX, int nY, int nZ, int nPolyObj);
//void DrawField_SetPolyObjMag(int nSurface, int nX, int nY, int nZ, float fMag);
//void DrawField_SetPolyObjRotateH(int nSurface, int nX, int nY, int nZ, float fRotateH);
//void DrawField_SetPolyObjRotateP(int nSurface, int nX, int nY, int nZ, float fRotateP);
//void DrawField_SetPolyObjRotateB(int nSurface, int nX, int nY, int nZ, float fRotateB);
//void DrawField_SetPolyObjOffsetX(int nSurface, int nX, int nY, int nZ, float fOffsetX);
//void DrawField_SetPolyObjOffsetY(int nSurface, int nX, int nY, int nZ, float fOffsetY);
//void DrawField_SetPolyObjOffsetZ(int nSurface, int nX, int nY, int nZ, float fOffsetZ);
//void DrawField_SetDrawObjFlag(int nSurface, int nType, bool bFlag);
//bool DrawField_GetDrawObjFlag(int nSurface, int nType);
//int DrawField_GetTexSound(int nSurface, int nType, int nNum);
//int DrawField_GetNumofTexSound(int nSurface, int nType);

HLL_LIBRARY(DrawField,
	    HLL_EXPORT(Init, DrawField_Init),
	    HLL_EXPORT(Release, DrawField_Release),
	    HLL_EXPORT(SetDrawFlag, DrawField_SetDrawFlag),
	    HLL_EXPORT(GetDrawFlag, DrawField_GetDrawFlag),
	    HLL_EXPORT(LoadFromFile, DrawField_LoadFromFile),
	    HLL_EXPORT(LoadTexture, DrawField_LoadTexture),
	    HLL_TODO_EXPORT(LoadEventTexture, DrawField_LoadEventTexture),
	    HLL_TODO_EXPORT(BeginLoad, DrawField_BeginLoad),
	    HLL_TODO_EXPORT(IsLoadEnd, DrawField_IsLoadEnd),
	    HLL_TODO_EXPORT(StopLoad, DrawField_StopLoad),
	    HLL_TODO_EXPORT(GetLoadPercent, DrawField_GetLoadPercent),
	    HLL_TODO_EXPORT(IsLoadSucceeded, DrawField_IsLoadSucceeded),
	    HLL_TODO_EXPORT(BeginLoadTexture, DrawField_BeginLoadTexture),
	    HLL_TODO_EXPORT(IsLoadTextureEnd, DrawField_IsLoadTextureEnd),
	    HLL_TODO_EXPORT(StopLoadTexture, DrawField_StopLoadTexture),
	    HLL_TODO_EXPORT(GetLoadTexturePercent, DrawField_GetLoadTexturePercent),
	    HLL_TODO_EXPORT(IsLoadTextureSucceeded, DrawField_IsLoadTextureSucceeded),
	    HLL_TODO_EXPORT(UpdateTexture, DrawField_UpdateTexture),
	    HLL_TODO_EXPORT(SetCamera, DrawField_SetCamera),
	    HLL_EXPORT(GetMapX, DrawField_GetMapX),
	    HLL_EXPORT(GetMapY, DrawField_GetMapY),
	    HLL_EXPORT(GetMapZ, DrawField_GetMapZ),
	    HLL_EXPORT(IsInMap, DrawField_IsInMap),
	    HLL_EXPORT(IsFloor, DrawField_IsFloor),
	    HLL_EXPORT(IsStair, DrawField_IsStair),
	    HLL_EXPORT(IsStairN, DrawField_IsStairN),
	    HLL_EXPORT(IsStairW, DrawField_IsStairW),
	    HLL_EXPORT(IsStairS, DrawField_IsStairS),
	    HLL_EXPORT(IsStairE, DrawField_IsStairE),
	    HLL_EXPORT(IsWallN, DrawField_IsWallN),
	    HLL_EXPORT(IsWallW, DrawField_IsWallW),
	    HLL_EXPORT(IsWallS, DrawField_IsWallS),
	    HLL_EXPORT(IsWallE, DrawField_IsWallE),
	    HLL_EXPORT(IsDoorN, DrawField_IsDoorN),
	    HLL_EXPORT(IsDoorW, DrawField_IsDoorW),
	    HLL_EXPORT(IsDoorS, DrawField_IsDoorS),
	    HLL_EXPORT(IsDoorE, DrawField_IsDoorE),
	    HLL_EXPORT(IsEnter, DrawField_IsEnter),
	    HLL_EXPORT(IsEnterN, DrawField_IsEnterN),
	    HLL_EXPORT(IsEnterW, DrawField_IsEnterW),
	    HLL_EXPORT(IsEnterS, DrawField_IsEnterS),
	    HLL_EXPORT(IsEnterE, DrawField_IsEnterE),
	    HLL_EXPORT(GetEvFloor, DrawField_GetEvFloor),
	    HLL_EXPORT(GetEvWallN, DrawField_GetEvWallN),
	    HLL_EXPORT(GetEvWallW, DrawField_GetEvWallW),
	    HLL_EXPORT(GetEvWallS, DrawField_GetEvWallS),
	    HLL_EXPORT(GetEvWallE, DrawField_GetEvWallE),
	    HLL_EXPORT(GetEvFloor2, DrawField_GetEvFloor2),
	    HLL_EXPORT(GetEvWallN2, DrawField_GetEvWallN2),
	    HLL_EXPORT(GetEvWallW2, DrawField_GetEvWallW2),
	    HLL_EXPORT(GetEvWallS2, DrawField_GetEvWallS2),
	    HLL_EXPORT(GetEvWallE2, DrawField_GetEvWallE2),
	    HLL_EXPORT(GetTexFloor, DrawField_GetTexFloor),
	    HLL_EXPORT(GetTexCeiling, DrawField_GetTexCeiling),
	    HLL_EXPORT(GetTexWallN, DrawField_GetTexWallN),
	    HLL_EXPORT(GetTexWallW, DrawField_GetTexWallW),
	    HLL_EXPORT(GetTexWallS, DrawField_GetTexWallS),
	    HLL_EXPORT(GetTexWallE, DrawField_GetTexWallE),
	    HLL_EXPORT(GetTexDoorN, DrawField_GetTexDoorN),
	    HLL_EXPORT(GetTexDoorW, DrawField_GetTexDoorW),
	    HLL_EXPORT(GetTexDoorS, DrawField_GetTexDoorS),
	    HLL_EXPORT(GetTexDoorE, DrawField_GetTexDoorE),
	    HLL_TODO_EXPORT(GetDoorNAngle, DrawField_GetDoorNAngle),
	    HLL_TODO_EXPORT(GetDoorWAngle, DrawField_GetDoorWAngle),
	    HLL_TODO_EXPORT(GetDoorSAngle, DrawField_GetDoorSAngle),
	    HLL_TODO_EXPORT(GetDoorEAngle, DrawField_GetDoorEAngle),
	    HLL_TODO_EXPORT(SetDoorNAngle, DrawField_SetDoorNAngle),
	    HLL_TODO_EXPORT(SetDoorWAngle, DrawField_SetDoorWAngle),
	    HLL_TODO_EXPORT(SetDoorSAngle, DrawField_SetDoorSAngle),
	    HLL_TODO_EXPORT(SetDoorEAngle, DrawField_SetDoorEAngle),
	    HLL_EXPORT(SetEvFloor, DrawField_SetEvFloor),
	    HLL_EXPORT(SetEvWallN, DrawField_SetEvWallN),
	    HLL_EXPORT(SetEvWallW, DrawField_SetEvWallW),
	    HLL_EXPORT(SetEvWallS, DrawField_SetEvWallS),
	    HLL_EXPORT(SetEvWallE, DrawField_SetEvWallE),
	    HLL_EXPORT(SetEvFloor2, DrawField_SetEvFloor2),
	    HLL_EXPORT(SetEvWallN2, DrawField_SetEvWallN2),
	    HLL_EXPORT(SetEvWallW2, DrawField_SetEvWallW2),
	    HLL_EXPORT(SetEvWallS2, DrawField_SetEvWallS2),
	    HLL_EXPORT(SetEvWallE2, DrawField_SetEvWallE2),
	    HLL_TODO_EXPORT(SetEvMag, DrawField_SetEvMag),
	    HLL_EXPORT(SetEvRate, DrawField_SetEvRate),
	    HLL_EXPORT(SetEnter, DrawField_SetEnter),
	    HLL_EXPORT(SetEnterN, DrawField_SetEnterN),
	    HLL_EXPORT(SetEnterW, DrawField_SetEnterW),
	    HLL_EXPORT(SetEnterS, DrawField_SetEnterS),
	    HLL_EXPORT(SetEnterE, DrawField_SetEnterE),
	    HLL_EXPORT(SetDoorNLock, DrawField_SetDoorNLock),
	    HLL_EXPORT(SetDoorWLock, DrawField_SetDoorWLock),
	    HLL_EXPORT(SetDoorSLock, DrawField_SetDoorSLock),
	    HLL_EXPORT(SetDoorELock, DrawField_SetDoorELock),
	    HLL_TODO_EXPORT(GetDoorNLock, DrawField_GetDoorNLock),
	    HLL_TODO_EXPORT(GetDoorWLock, DrawField_GetDoorWLock),
	    HLL_TODO_EXPORT(GetDoorSLock, DrawField_GetDoorSLock),
	    HLL_TODO_EXPORT(GetDoorELock, DrawField_GetDoorELock),
	    HLL_EXPORT(SetTexFloor, DrawField_SetTexFloor),
	    HLL_EXPORT(SetTexCeiling, DrawField_SetTexCeiling),
	    HLL_EXPORT(SetTexWallN, DrawField_SetTexWallN),
	    HLL_EXPORT(SetTexWallW, DrawField_SetTexWallW),
	    HLL_EXPORT(SetTexWallS, DrawField_SetTexWallS),
	    HLL_EXPORT(SetTexWallE, DrawField_SetTexWallE),
	    HLL_EXPORT(SetTexDoorN, DrawField_SetTexDoorN),
	    HLL_EXPORT(SetTexDoorW, DrawField_SetTexDoorW),
	    HLL_EXPORT(SetTexDoorS, DrawField_SetTexDoorS),
	    HLL_EXPORT(SetTexDoorE, DrawField_SetTexDoorE),
	    HLL_TODO_EXPORT(SetTexStair, DrawField_SetTexStair),
	    HLL_TODO_EXPORT(SetShadowTexFloor, DrawField_SetShadowTexFloor),
	    HLL_TODO_EXPORT(SetShadowTexCeiling, DrawField_SetShadowTexCeiling),
	    HLL_TODO_EXPORT(SetShadowTexWallN, DrawField_SetShadowTexWallN),
	    HLL_TODO_EXPORT(SetShadowTexWallW, DrawField_SetShadowTexWallW),
	    HLL_TODO_EXPORT(SetShadowTexWallS, DrawField_SetShadowTexWallS),
	    HLL_TODO_EXPORT(SetShadowTexWallE, DrawField_SetShadowTexWallE),
	    HLL_TODO_EXPORT(SetShadowTexDoorN, DrawField_SetShadowTexDoorN),
	    HLL_TODO_EXPORT(SetShadowTexDoorW, DrawField_SetShadowTexDoorW),
	    HLL_TODO_EXPORT(SetShadowTexDoorS, DrawField_SetShadowTexDoorS),
	    HLL_TODO_EXPORT(SetShadowTexDoorE, DrawField_SetShadowTexDoorE),
	    HLL_TODO_EXPORT(CalcShadowMap, DrawField_CalcShadowMap),
	    HLL_TODO_EXPORT(DrawMap, DrawField_DrawMap),
	    HLL_TODO_EXPORT(SetMapAllViewFlag, DrawField_SetMapAllViewFlag),
	    HLL_TODO_EXPORT(SetDrawMapFloor, DrawField_SetDrawMapFloor),
	    HLL_TODO_EXPORT(DrawLMap, DrawField_DrawLMap),
	    HLL_TODO_EXPORT(SetMapCG, DrawField_SetMapCG),
	    HLL_TODO_EXPORT(SetDrawLMapFloor, DrawField_SetDrawLMapFloor),
	    HLL_EXPORT(SetPlayerPos, DrawField_SetPlayerPos),
	    HLL_EXPORT(SetWalked, dungeon_set_walked),
	    HLL_TODO_EXPORT(SetLooked, DrawField_SetLooked),
	    HLL_TODO_EXPORT(SetHideDrawMapFloor, DrawField_SetHideDrawMapFloor),
	    HLL_TODO_EXPORT(SetHideDrawMapWall, DrawField_SetHideDrawMapWall),
	    HLL_TODO_EXPORT(SetDrawHalfFlag, DrawField_SetDrawHalfFlag),
	    HLL_TODO_EXPORT(GetDrawHalfFlag, DrawField_GetDrawHalfFlag),
	    HLL_TODO_EXPORT(SetInterlaceMode, DrawField_SetInterlaceMode),
	    HLL_TODO_EXPORT(SetDirect3DMode, DrawField_SetDirect3DMode),
	    HLL_TODO_EXPORT(GetDirect3DMode, DrawField_GetDirect3DMode),
	    HLL_TODO_EXPORT(SaveDrawSettingFlag, DrawField_SaveDrawSettingFlag),
	    HLL_EXPORT(SetPerspective, DrawField_SetPerspective),
	    HLL_TODO_EXPORT(SetDrawShadowMap, DrawField_SetDrawShadowMap),
	    HLL_TODO_EXPORT(CalcNumofFloor, DrawField_CalcNumofFloor),
	    HLL_TODO_EXPORT(CalcNumofWalk, DrawField_CalcNumofWalk),
	    HLL_TODO_EXPORT(CalcNumofWalk2, DrawField_CalcNumofWalk2),
	    HLL_TODO_EXPORT(IsPVSData, DrawField_IsPVSData),
	    HLL_TODO_EXPORT(CosDeg, DrawField_CosDeg),
	    HLL_TODO_EXPORT(SinDeg, DrawField_SinDeg),
	    HLL_TODO_EXPORT(TanDeg, DrawField_TanDeg),
	    HLL_EXPORT(Sqrt, sqrtf),
	    HLL_TODO_EXPORT(Atan2, DrawField_Atan2),
	    HLL_TODO_EXPORT(TransPos2DToPos3DOnPlane, DrawField_TransPos2DToPos3DOnPlane),
	    HLL_TODO_EXPORT(TransPos3DToPos2D, DrawField_TransPos3DToPos2D),
	    HLL_EXPORT(GetCharaNumMax, DrawField_GetCharaNumMax),
	    HLL_EXPORT(SetCharaSprite, DrawField_SetCharaSprite),
	    HLL_EXPORT(SetCharaPos, DrawField_SetCharaPos),
	    HLL_EXPORT(SetCharaCG, DrawField_SetCharaCG),
	    HLL_EXPORT(SetCharaCGInfo, DrawField_SetCharaCGInfo),
	    HLL_TODO_EXPORT(SetCharaZBias, DrawField_SetCharaZBias),
	    HLL_EXPORT(SetCharaShow, DrawField_SetCharaShow),
	    HLL_EXPORT(SetCenterOffsetY, DrawField_SetCenterOffsetY),
	    HLL_TODO_EXPORT(SetBuilBoard, DrawField_SetBuilBoard),
	    HLL_TODO_EXPORT(SetSphereTheta, DrawField_SetSphereTheta),
	    HLL_TODO_EXPORT(SetSphereColor, DrawField_SetSphereColor),
	    HLL_TODO_EXPORT(GetSphereTheta, DrawField_GetSphereTheta),
	    HLL_TODO_EXPORT(GetSphereColor, DrawField_GetSphereColor),
	    HLL_TODO_EXPORT(GetBackColor, DrawField_GetBackColor),
	    HLL_TODO_EXPORT(SetBackColor, DrawField_SetBackColor),
	    HLL_TODO_EXPORT(GetPolyObj, DrawField_GetPolyObj),
	    HLL_TODO_EXPORT(GetPolyObjMag, DrawField_GetPolyObjMag),
	    HLL_TODO_EXPORT(GetPolyObjRotateH, DrawField_GetPolyObjRotateH),
	    HLL_TODO_EXPORT(GetPolyObjRotateP, DrawField_GetPolyObjRotateP),
	    HLL_TODO_EXPORT(GetPolyObjRotateB, DrawField_GetPolyObjRotateB),
	    HLL_TODO_EXPORT(GetPolyObjOffsetX, DrawField_GetPolyObjOffsetX),
	    HLL_TODO_EXPORT(GetPolyObjOffsetY, DrawField_GetPolyObjOffsetY),
	    HLL_TODO_EXPORT(GetPolyObjOffsetZ, DrawField_GetPolyObjOffsetZ),
	    HLL_TODO_EXPORT(SetPolyObj, DrawField_SetPolyObj),
	    HLL_TODO_EXPORT(SetPolyObjMag, DrawField_SetPolyObjMag),
	    HLL_TODO_EXPORT(SetPolyObjRotateH, DrawField_SetPolyObjRotateH),
	    HLL_TODO_EXPORT(SetPolyObjRotateP, DrawField_SetPolyObjRotateP),
	    HLL_TODO_EXPORT(SetPolyObjRotateB, DrawField_SetPolyObjRotateB),
	    HLL_TODO_EXPORT(SetPolyObjOffsetX, DrawField_SetPolyObjOffsetX),
	    HLL_TODO_EXPORT(SetPolyObjOffsetY, DrawField_SetPolyObjOffsetY),
	    HLL_TODO_EXPORT(SetPolyObjOffsetZ, DrawField_SetPolyObjOffsetZ),
	    HLL_TODO_EXPORT(SetDrawObjFlag, DrawField_SetDrawObjFlag),
	    HLL_TODO_EXPORT(GetDrawObjFlag, DrawField_GetDrawObjFlag),
	    HLL_TODO_EXPORT(GetTexSound, DrawField_GetTexSound),
	    HLL_TODO_EXPORT(GetNumofTexSound, DrawField_GetNumofTexSound)
	    );
