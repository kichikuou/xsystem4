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

#include "hll.h"

HLL_WARN_UNIMPLEMENTED(true, bool, PastelChime2, InitPastelChime2);
//bool PastelChime2_DungeonDataSave(int nID, struct page **aDungeon);
//bool PastelChime2_DungeonDataLoad(int nID, struct page **aDungeon);
//bool PastelChime2_DungeonDataExist(int nID);
HLL_WARN_UNIMPLEMENTED(false, bool, PastelChime2, DungeonDataToSaveData, int nSaveDataIndex);
HLL_WARN_UNIMPLEMENTED(false, bool, PastelChime2, DungeonDataFromSaveData, int nSaveDataIndex);
//bool PastelChime2_Field_SetSprite(int nDrawFieldSpriteNumber);
//void PastelChime2_Field_GetDoors(struct page **dst, int nFloor);
//void PastelChime2_Field_UpdateDoorNamePlate(struct page **doors, struct page *fxyzPlayer, bool bWalking, int nFloor, float fBlockSize, struct page **rcField, struct page **posMouse);
//void PastelChime2_Field_UpdateObjectNamePlate(struct page **aDungeon, float fEnemyRange, float fTreasureRange, float fTrapRange, float fDefaultRange);
//bool PastelChime2_Field_QuerySelectedObjectPlate(struct page **result, struct page **posMouse, struct page **aDungeon);
//bool PastelChime2_Field_QuerySelectedDoorPlate(struct page **result, struct page **posMouse, struct page **aDungeon);
//void PastelChime2_Field_UpdateEnemyPos(int nDeltaTime, struct page **aDungeon);
//void PastelChime2_Field_CheckObjectHit(struct page **aChangeList, struct page **aDungeon);
//void PastelChime2_Field_UpdateObjectAnime(int nTotalTime, struct page **aDungeon);
//float PastelChime2_Field_CalcY(float x, float z, int nFloor, float fBlockSize);
//void PastelChime2_Field_CalcCamera(float x, float y, float z, float deg, float pitch, float length);
//bool PastelChime2_Field_CheckLook(float px, float py, float qx, float qy, int nFloor, float fBlockSize);
//bool PastelChime2_Field_CalcMove(float *fposNewx, float *fposNewy, float fposOldx, float fposOldy, float fMapDeltaX, float fMapDeltaY, float fR, float delta_time, float fWalkStep, int nFloor, float fBlockSize, bool bCheckDoor);
//void PastelChime2_Field_UpdatePlayersUnit(float fxyzPlayer1x, float fxyzPlayer1y, float fxyzPlayer1z, int nCharaPlayer1, float *fxyzPlayer2x, float *fxyzPlayer2y, float *fxyzPlayer2z, int nCharaPlayer2, float *fxyzPlayer3x, float *fxyzPlayer3y, float *fxyzPlayer3z, int nCharaPlayer3, int nCgIndexPlayer1, bool bWalking);
//void PastelChime2_Field_ClearPlayerMoveQueue(void);
//bool PastelChime2_JudgeHitWall(float fposOldx, float fposOldy, float fposNewx, float fposNewy, float fR, float *fposHitx, float *fposHity, float *lineHitpx, float *lineHitpy, float *lineHitqx, float *lineHitqy, float walllpx, float walllpy, float walllqx, float walllqy, bool wallb);
//bool PastelChime2_AdjustMovePos(float *fposNewx, float *fposNewy, float fposOldx, float fposOldy, float lineWpx, float lineWpy, float lineWqx, float lineWqy);
//bool PastelChime2_JudgeHitPointCircle(float px, float py, float cpx, float cpy, float cr);
//bool PastelChime2_JudgeHitLineCircle(float lpx, float lpy, float lqx, float lqy, float cpx, float cpy, float cr);
//void PastelChime2_Field_UpdateDoors(bool *pbOpen, bool *pbClose, int *pnOpenTex, int *pnCloseTex, float fx1, float fy1, float fz1, float fx2, float fy2, float fz2, float fx3, float fy3, float fz3, float fBlockSize, bool bPlayer2, bool bPlayer3);
//void PastelChime2_AutoDungeonE_Create(int nFloor, int *nEntX, int *nEntY, int nComplex, int nWallArrangeMethod, int nFloorArrangeMethod, int nFieldSizeX, int nFieldSizeY, int nDoorLockPercent, struct page **dci);
//void PastelChime2_TestVMArray(struct page **a);
HLL_WARN_UNIMPLEMENTED( , void, PastelChime2, str_erase_found, struct string **s, struct string **key);
//bool PastelChime2_Field_PickUpRoadShape(struct page **a4, struct page **a3, struct page **a2, struct page **a1, struct page **a0, struct page **ac, int nFloor);
//void PastelChime2_SaveMapPicture(struct string *sFileName);
//bool PastelChime2_ShellOpen(struct string *sFileName);
//bool PastelChime2_File_GetTime(struct string *strFileName, int *nYear, int *nMonth, int *nDayOfWeek, int *nDay, int *nHour, int *nMinute, int *nSecond, int *nMilliseconds);
HLL_WARN_UNIMPLEMENTED(false, bool, PastelChime2, SaveArray, struct string *file_name, struct page **buf);
HLL_WARN_UNIMPLEMENTED(false, bool, PastelChime2, LoadArray, struct string *file_name, struct page **buf);
HLL_WARN_UNIMPLEMENTED(false, bool, PastelChime2, SaveStringArray, struct string *file_name, struct page **buf);
HLL_WARN_UNIMPLEMENTED(false, bool, PastelChime2, LoadStringArray, struct string *file_name, struct page **buf);
HLL_WARN_UNIMPLEMENTED(false, bool, PastelChime2, File_Delete, struct string *file_name);
//bool PastelChime2_File_CreateDummy(struct string *file_name);

HLL_LIBRARY(PastelChime2,
	    HLL_EXPORT(InitPastelChime2, PastelChime2_InitPastelChime2),
	    HLL_TODO_EXPORT(DungeonDataSave, PastelChime2_DungeonDataSave),
	    HLL_TODO_EXPORT(DungeonDataLoad, PastelChime2_DungeonDataLoad),
	    HLL_TODO_EXPORT(DungeonDataExist, PastelChime2_DungeonDataExist),
	    HLL_EXPORT(DungeonDataToSaveData, PastelChime2_DungeonDataToSaveData),
	    HLL_EXPORT(DungeonDataFromSaveData, PastelChime2_DungeonDataFromSaveData),
	    HLL_TODO_EXPORT(Field_SetSprite, PastelChime2_Field_SetSprite),
	    HLL_TODO_EXPORT(Field_GetDoors, PastelChime2_Field_GetDoors),
	    HLL_TODO_EXPORT(Field_UpdateDoorNamePlate, PastelChime2_Field_UpdateDoorNamePlate),
	    HLL_TODO_EXPORT(Field_UpdateObjectNamePlate, PastelChime2_Field_UpdateObjectNamePlate),
	    HLL_TODO_EXPORT(Field_QuerySelectedObjectPlate, PastelChime2_Field_QuerySelectedObjectPlate),
	    HLL_TODO_EXPORT(Field_QuerySelectedDoorPlate, PastelChime2_Field_QuerySelectedDoorPlate),
	    HLL_TODO_EXPORT(Field_UpdateEnemyPos, PastelChime2_Field_UpdateEnemyPos),
	    HLL_TODO_EXPORT(Field_CheckObjectHit, PastelChime2_Field_CheckObjectHit),
	    HLL_TODO_EXPORT(Field_UpdateObjectAnime, PastelChime2_Field_UpdateObjectAnime),
	    HLL_TODO_EXPORT(Field_CalcY, PastelChime2_Field_CalcY),
	    HLL_TODO_EXPORT(Field_CalcCamera, PastelChime2_Field_CalcCamera),
	    HLL_TODO_EXPORT(Field_CheckLook, PastelChime2_Field_CheckLook),
	    HLL_TODO_EXPORT(Field_CalcMove, PastelChime2_Field_CalcMove),
	    HLL_TODO_EXPORT(Field_UpdatePlayersUnit, PastelChime2_Field_UpdatePlayersUnit),
	    HLL_TODO_EXPORT(Field_ClearPlayerMoveQueue, PastelChime2_Field_ClearPlayerMoveQueue),
	    HLL_TODO_EXPORT(JudgeHitWall, PastelChime2_JudgeHitWall),
	    HLL_TODO_EXPORT(AdjustMovePos, PastelChime2_AdjustMovePos),
	    HLL_TODO_EXPORT(JudgeHitPointCircle, PastelChime2_JudgeHitPointCircle),
	    HLL_TODO_EXPORT(JudgeHitLineCircle, PastelChime2_JudgeHitLineCircle),
	    HLL_TODO_EXPORT(Field_UpdateDoors, PastelChime2_Field_UpdateDoors),
	    HLL_TODO_EXPORT(AutoDungeonE_Create, PastelChime2_AutoDungeonE_Create),
	    HLL_TODO_EXPORT(TestVMArray, PastelChime2_TestVMArray),
	    HLL_EXPORT(str_erase_found, PastelChime2_str_erase_found),
	    HLL_TODO_EXPORT(Field_PickUpRoadShape, PastelChime2_Field_PickUpRoadShape),
	    HLL_TODO_EXPORT(SaveMapPicture, PastelChime2_SaveMapPicture),
	    HLL_TODO_EXPORT(ShellOpen, PastelChime2_ShellOpen),
	    HLL_TODO_EXPORT(File_GetTime, PastelChime2_File_GetTime),
	    HLL_EXPORT(SaveArray, PastelChime2_SaveArray),
	    HLL_EXPORT(LoadArray, PastelChime2_LoadArray),
	    HLL_EXPORT(SaveStringArray, PastelChime2_SaveStringArray),
	    HLL_EXPORT(LoadStringArray, PastelChime2_LoadStringArray),
	    HLL_EXPORT(File_Delete, PastelChime2_File_Delete),
	    HLL_TODO_EXPORT(File_CreateDummy, PastelChime2_File_CreateDummy)
	    );
