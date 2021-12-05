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

#include "hll.h"

//﻿int DrawField_Init(int nSurface);
//void DrawField_Release(int nSurface);
//void DrawField_SetDrawFlag(int nSurface, int nFlag);
//bool DrawField_GetDrawFlag(int nSurface);
//int DrawField_LoadFromFile(int nSurface, struct string *szFileName, int nNum);
//int DrawField_LoadTexture(int nSurface, struct string *szFileName);
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
//int DrawField_GetMapX(int nSurface);
//int DrawField_GetMapY(int nSurface);
//int DrawField_GetMapZ(int nSurface);
//int DrawField_IsInMap(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsFloor(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsStair(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsStairN(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsStairW(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsStairS(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsStairE(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsWallN(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsWallW(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsWallS(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsWallE(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsDoorN(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsDoorW(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsDoorS(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsDoorE(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsEnter(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsEnterN(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsEnterW(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsEnterS(int nSurface, int nX, int nY, int nZ);
//int DrawField_IsEnterE(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetEvFloor(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetEvWallN(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetEvWallW(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetEvWallS(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetEvWallE(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetEvFloor2(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetEvWallN2(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetEvWallW2(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetEvWallS2(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetEvWallE2(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetTexFloor(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetTexCeiling(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetTexWallN(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetTexWallW(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetTexWallS(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetTexWallE(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetTexDoorN(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetTexDoorW(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetTexDoorS(int nSurface, int nX, int nY, int nZ);
//int DrawField_GetTexDoorE(int nSurface, int nX, int nY, int nZ);
//bool DrawField_GetDoorNAngle(int nSurface, int nX, int nY, int nZ, float *pfAngle);
//bool DrawField_GetDoorWAngle(int nSurface, int nX, int nY, int nZ, float *pfAngle);
//bool DrawField_GetDoorSAngle(int nSurface, int nX, int nY, int nZ, float *pfAngle);
//bool DrawField_GetDoorEAngle(int nSurface, int nX, int nY, int nZ, float *pfAngle);
//bool DrawField_SetDoorNAngle(int nSurface, int nX, int nY, int nZ, float fAngle);
//bool DrawField_SetDoorWAngle(int nSurface, int nX, int nY, int nZ, float fAngle);
//bool DrawField_SetDoorSAngle(int nSurface, int nX, int nY, int nZ, float fAngle);
//bool DrawField_SetDoorEAngle(int nSurface, int nX, int nY, int nZ, float fAngle);
//void DrawField_SetEvFloor(int nSurface, int nX, int nY, int nZ, int nEvent);
//void DrawField_SetEvWallN(int nSurface, int nX, int nY, int nZ, int nEvent);
//void DrawField_SetEvWallW(int nSurface, int nX, int nY, int nZ, int nEvent);
//void DrawField_SetEvWallS(int nSurface, int nX, int nY, int nZ, int nEvent);
//void DrawField_SetEvWallE(int nSurface, int nX, int nY, int nZ, int nEvent);
//void DrawField_SetEvFloor2(int nSurface, int nX, int nY, int nZ, int nEvent);
//void DrawField_SetEvWallN2(int nSurface, int nX, int nY, int nZ, int nEvent);
//void DrawField_SetEvWallW2(int nSurface, int nX, int nY, int nZ, int nEvent);
//void DrawField_SetEvWallS2(int nSurface, int nX, int nY, int nZ, int nEvent);
//void DrawField_SetEvWallE2(int nSurface, int nX, int nY, int nZ, int nEvent);
//void DrawField_SetEvMag(int nSurface, int nX, int nY, int nZ, float fMag);
//void DrawField_SetEvRate(int nSurface, int nX, int nY, int nZ, int nRate);
//void DrawField_SetEnter(int nSurface, int nX, int nY, int nZ, int nFlag);
//void DrawField_SetEnterN(int nSurface, int nX, int nY, int nZ, int nFlag);
//void DrawField_SetEnterW(int nSurface, int nX, int nY, int nZ, int nFlag);
//void DrawField_SetEnterS(int nSurface, int nX, int nY, int nZ, int nFlag);
//void DrawField_SetEnterE(int nSurface, int nX, int nY, int nZ, int nFlag);
//bool DrawField_SetDoorNLock(int nSurface, int nX, int nY, int nZ, int nLock);
//bool DrawField_SetDoorWLock(int nSurface, int nX, int nY, int nZ, int nLock);
//bool DrawField_SetDoorSLock(int nSurface, int nX, int nY, int nZ, int nLock);
//bool DrawField_SetDoorELock(int nSurface, int nX, int nY, int nZ, int nLock);
//bool DrawField_GetDoorNLock(int nSurface, int nX, int nY, int nZ, int *pnLock);
//bool DrawField_GetDoorWLock(int nSurface, int nX, int nY, int nZ, int *pnLock);
//bool DrawField_GetDoorSLock(int nSurface, int nX, int nY, int nZ, int *pnLock);
//bool DrawField_GetDoorELock(int nSurface, int nX, int nY, int nZ, int *pnLock);
//void DrawField_SetTexFloor(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetTexCeiling(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetTexWallN(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetTexWallW(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetTexWallS(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetTexWallE(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetTexDoorN(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetTexDoorW(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetTexDoorS(int nSurface, int nX, int nY, int nZ, int nTexture);
//void DrawField_SetTexDoorE(int nSurface, int nX, int nY, int nZ, int nTexture);
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
//void DrawField_SetPlayerPos(int nSurface, int nX, int nY, int nZ);
//void DrawField_SetWalked(int nSurface, int nX, int nY, int nZ, int nFlag);
//void DrawField_SetLooked(int nSurface, int nX, int nY, int nZ, bool bFlag);
//bool DrawField_SetHideDrawMapFloor(int nSurface, int nFloor, bool bHide);
//bool DrawField_SetHideDrawMapWall(int nSurface, int nWall, bool bHide);
//void DrawField_SetDrawHalfFlag(int nSurface, int nFlag);
//int DrawField_GetDrawHalfFlag(int nSurface);
//void DrawField_SetInterlaceMode(int nSurface, int nFlag);
//bool DrawField_SetDirect3DMode(int nSurface, int nFlag);
//bool DrawField_GetDirect3DMode(int nSurface);
//void DrawField_SaveDrawSettingFlag(int nDirect3D, int nInterlace, int nHalf);
//void DrawField_SetPerspective(int nSurface, int nWidth, int nHeight, float fNear, float fFar, float fDeg);
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
//int DrawField_GetCharaNumMax(int nSurface);
//void DrawField_SetCharaSprite(int nSurface, int nNum, int nSprite);
//void DrawField_SetCharaPos(int nSurface, int nNum, float fX, float fY, float fZ);
//void DrawField_SetCharaCG(int nSurface, int nNum, int nCG);
//void DrawField_SetCharaCGInfo(int nSurface, int nNum, int nNumofCharaX, int nNumofCharaY);
//void DrawField_SetCharaZBias(int nSurface, float fZBias0, float fZBias1, float fZBias2, float fZBias3);
//void DrawField_SetCharaShow(int nSurface, int nNum, bool bShow);
//void DrawField_SetCenterOffsetY(int nSurface, float fY);
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
	    HLL_TODO_EXPORT(Init, DrawField_Init),
	    HLL_TODO_EXPORT(Release, DrawField_Release),
	    HLL_TODO_EXPORT(SetDrawFlag, DrawField_SetDrawFlag),
	    HLL_TODO_EXPORT(GetDrawFlag, DrawField_GetDrawFlag),
	    HLL_TODO_EXPORT(LoadFromFile, DrawField_LoadFromFile),
	    HLL_TODO_EXPORT(LoadTexture, DrawField_LoadTexture),
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
	    HLL_TODO_EXPORT(GetMapX, DrawField_GetMapX),
	    HLL_TODO_EXPORT(GetMapY, DrawField_GetMapY),
	    HLL_TODO_EXPORT(GetMapZ, DrawField_GetMapZ),
	    HLL_TODO_EXPORT(IsInMap, DrawField_IsInMap),
	    HLL_TODO_EXPORT(IsFloor, DrawField_IsFloor),
	    HLL_TODO_EXPORT(IsStair, DrawField_IsStair),
	    HLL_TODO_EXPORT(IsStairN, DrawField_IsStairN),
	    HLL_TODO_EXPORT(IsStairW, DrawField_IsStairW),
	    HLL_TODO_EXPORT(IsStairS, DrawField_IsStairS),
	    HLL_TODO_EXPORT(IsStairE, DrawField_IsStairE),
	    HLL_TODO_EXPORT(IsWallN, DrawField_IsWallN),
	    HLL_TODO_EXPORT(IsWallW, DrawField_IsWallW),
	    HLL_TODO_EXPORT(IsWallS, DrawField_IsWallS),
	    HLL_TODO_EXPORT(IsWallE, DrawField_IsWallE),
	    HLL_TODO_EXPORT(IsDoorN, DrawField_IsDoorN),
	    HLL_TODO_EXPORT(IsDoorW, DrawField_IsDoorW),
	    HLL_TODO_EXPORT(IsDoorS, DrawField_IsDoorS),
	    HLL_TODO_EXPORT(IsDoorE, DrawField_IsDoorE),
	    HLL_TODO_EXPORT(IsEnter, DrawField_IsEnter),
	    HLL_TODO_EXPORT(IsEnterN, DrawField_IsEnterN),
	    HLL_TODO_EXPORT(IsEnterW, DrawField_IsEnterW),
	    HLL_TODO_EXPORT(IsEnterS, DrawField_IsEnterS),
	    HLL_TODO_EXPORT(IsEnterE, DrawField_IsEnterE),
	    HLL_TODO_EXPORT(GetEvFloor, DrawField_GetEvFloor),
	    HLL_TODO_EXPORT(GetEvWallN, DrawField_GetEvWallN),
	    HLL_TODO_EXPORT(GetEvWallW, DrawField_GetEvWallW),
	    HLL_TODO_EXPORT(GetEvWallS, DrawField_GetEvWallS),
	    HLL_TODO_EXPORT(GetEvWallE, DrawField_GetEvWallE),
	    HLL_TODO_EXPORT(GetEvFloor2, DrawField_GetEvFloor2),
	    HLL_TODO_EXPORT(GetEvWallN2, DrawField_GetEvWallN2),
	    HLL_TODO_EXPORT(GetEvWallW2, DrawField_GetEvWallW2),
	    HLL_TODO_EXPORT(GetEvWallS2, DrawField_GetEvWallS2),
	    HLL_TODO_EXPORT(GetEvWallE2, DrawField_GetEvWallE2),
	    HLL_TODO_EXPORT(GetTexFloor, DrawField_GetTexFloor),
	    HLL_TODO_EXPORT(GetTexCeiling, DrawField_GetTexCeiling),
	    HLL_TODO_EXPORT(GetTexWallN, DrawField_GetTexWallN),
	    HLL_TODO_EXPORT(GetTexWallW, DrawField_GetTexWallW),
	    HLL_TODO_EXPORT(GetTexWallS, DrawField_GetTexWallS),
	    HLL_TODO_EXPORT(GetTexWallE, DrawField_GetTexWallE),
	    HLL_TODO_EXPORT(GetTexDoorN, DrawField_GetTexDoorN),
	    HLL_TODO_EXPORT(GetTexDoorW, DrawField_GetTexDoorW),
	    HLL_TODO_EXPORT(GetTexDoorS, DrawField_GetTexDoorS),
	    HLL_TODO_EXPORT(GetTexDoorE, DrawField_GetTexDoorE),
	    HLL_TODO_EXPORT(GetDoorNAngle, DrawField_GetDoorNAngle),
	    HLL_TODO_EXPORT(GetDoorWAngle, DrawField_GetDoorWAngle),
	    HLL_TODO_EXPORT(GetDoorSAngle, DrawField_GetDoorSAngle),
	    HLL_TODO_EXPORT(GetDoorEAngle, DrawField_GetDoorEAngle),
	    HLL_TODO_EXPORT(SetDoorNAngle, DrawField_SetDoorNAngle),
	    HLL_TODO_EXPORT(SetDoorWAngle, DrawField_SetDoorWAngle),
	    HLL_TODO_EXPORT(SetDoorSAngle, DrawField_SetDoorSAngle),
	    HLL_TODO_EXPORT(SetDoorEAngle, DrawField_SetDoorEAngle),
	    HLL_TODO_EXPORT(SetEvFloor, DrawField_SetEvFloor),
	    HLL_TODO_EXPORT(SetEvWallN, DrawField_SetEvWallN),
	    HLL_TODO_EXPORT(SetEvWallW, DrawField_SetEvWallW),
	    HLL_TODO_EXPORT(SetEvWallS, DrawField_SetEvWallS),
	    HLL_TODO_EXPORT(SetEvWallE, DrawField_SetEvWallE),
	    HLL_TODO_EXPORT(SetEvFloor2, DrawField_SetEvFloor2),
	    HLL_TODO_EXPORT(SetEvWallN2, DrawField_SetEvWallN2),
	    HLL_TODO_EXPORT(SetEvWallW2, DrawField_SetEvWallW2),
	    HLL_TODO_EXPORT(SetEvWallS2, DrawField_SetEvWallS2),
	    HLL_TODO_EXPORT(SetEvWallE2, DrawField_SetEvWallE2),
	    HLL_TODO_EXPORT(SetEvMag, DrawField_SetEvMag),
	    HLL_TODO_EXPORT(SetEvRate, DrawField_SetEvRate),
	    HLL_TODO_EXPORT(SetEnter, DrawField_SetEnter),
	    HLL_TODO_EXPORT(SetEnterN, DrawField_SetEnterN),
	    HLL_TODO_EXPORT(SetEnterW, DrawField_SetEnterW),
	    HLL_TODO_EXPORT(SetEnterS, DrawField_SetEnterS),
	    HLL_TODO_EXPORT(SetEnterE, DrawField_SetEnterE),
	    HLL_TODO_EXPORT(SetDoorNLock, DrawField_SetDoorNLock),
	    HLL_TODO_EXPORT(SetDoorWLock, DrawField_SetDoorWLock),
	    HLL_TODO_EXPORT(SetDoorSLock, DrawField_SetDoorSLock),
	    HLL_TODO_EXPORT(SetDoorELock, DrawField_SetDoorELock),
	    HLL_TODO_EXPORT(GetDoorNLock, DrawField_GetDoorNLock),
	    HLL_TODO_EXPORT(GetDoorWLock, DrawField_GetDoorWLock),
	    HLL_TODO_EXPORT(GetDoorSLock, DrawField_GetDoorSLock),
	    HLL_TODO_EXPORT(GetDoorELock, DrawField_GetDoorELock),
	    HLL_TODO_EXPORT(SetTexFloor, DrawField_SetTexFloor),
	    HLL_TODO_EXPORT(SetTexCeiling, DrawField_SetTexCeiling),
	    HLL_TODO_EXPORT(SetTexWallN, DrawField_SetTexWallN),
	    HLL_TODO_EXPORT(SetTexWallW, DrawField_SetTexWallW),
	    HLL_TODO_EXPORT(SetTexWallS, DrawField_SetTexWallS),
	    HLL_TODO_EXPORT(SetTexWallE, DrawField_SetTexWallE),
	    HLL_TODO_EXPORT(SetTexDoorN, DrawField_SetTexDoorN),
	    HLL_TODO_EXPORT(SetTexDoorW, DrawField_SetTexDoorW),
	    HLL_TODO_EXPORT(SetTexDoorS, DrawField_SetTexDoorS),
	    HLL_TODO_EXPORT(SetTexDoorE, DrawField_SetTexDoorE),
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
	    HLL_TODO_EXPORT(SetPlayerPos, DrawField_SetPlayerPos),
	    HLL_TODO_EXPORT(SetWalked, DrawField_SetWalked),
	    HLL_TODO_EXPORT(SetLooked, DrawField_SetLooked),
	    HLL_TODO_EXPORT(SetHideDrawMapFloor, DrawField_SetHideDrawMapFloor),
	    HLL_TODO_EXPORT(SetHideDrawMapWall, DrawField_SetHideDrawMapWall),
	    HLL_TODO_EXPORT(SetDrawHalfFlag, DrawField_SetDrawHalfFlag),
	    HLL_TODO_EXPORT(GetDrawHalfFlag, DrawField_GetDrawHalfFlag),
	    HLL_TODO_EXPORT(SetInterlaceMode, DrawField_SetInterlaceMode),
	    HLL_TODO_EXPORT(SetDirect3DMode, DrawField_SetDirect3DMode),
	    HLL_TODO_EXPORT(GetDirect3DMode, DrawField_GetDirect3DMode),
	    HLL_TODO_EXPORT(SaveDrawSettingFlag, DrawField_SaveDrawSettingFlag),
	    HLL_TODO_EXPORT(SetPerspective, DrawField_SetPerspective),
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
	    HLL_TODO_EXPORT(GetCharaNumMax, DrawField_GetCharaNumMax),
	    HLL_TODO_EXPORT(SetCharaSprite, DrawField_SetCharaSprite),
	    HLL_TODO_EXPORT(SetCharaPos, DrawField_SetCharaPos),
	    HLL_TODO_EXPORT(SetCharaCG, DrawField_SetCharaCG),
	    HLL_TODO_EXPORT(SetCharaCGInfo, DrawField_SetCharaCGInfo),
	    HLL_TODO_EXPORT(SetCharaZBias, DrawField_SetCharaZBias),
	    HLL_TODO_EXPORT(SetCharaShow, DrawField_SetCharaShow),
	    HLL_TODO_EXPORT(SetCenterOffsetY, DrawField_SetCenterOffsetY),
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
