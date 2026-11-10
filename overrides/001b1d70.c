#include "p4pc/mod_entry.h"

/* 001b1d70: retail party/enemy action creation. A p4.battle.formation
 * projector (one at a time) may replace the battle's player-side lineup with
 * other playable characters; the encounter's own party array is never
 * rewritten, so its retail owner still releases it. A character the setup did
 * not create gets one wrapper from datUnitCreatePc, kept for the process and
 * reused by every later projection. A lineup without Yu gives his leader role
 * (the first selected action) to its first fighter. With no projector the one
 * mask test leaves the retail loop over the encounter's party array. */
static u32 btlMainProjectParty(s32 *party, u32 count);
void btlMainUnitCreateInit(void) {
    extern s32 iGpffffb414;
    extern u8 *btlActionCreateWork(s32, u16, u16 *);
    extern u32 datCalcClearBadStatus(s32, u32);
    extern void *mdlFindModelObject(u32, u16, u32);
    extern u8 *btlUnitCreateModelLoadPacket(u8 *, u16, s16);
    extern void SceneDraw_SetDirectionalOnly(u16, u32);
    extern void btlUnitApplyModelState(u8 *, s32);
    extern void btlUnitSetRandomAnimFrame(u8 *);
    extern void mdlRunDataCallback(void *model);
    extern u8 *mdlGetClump(u8 *);
    extern void mdlClumpSetEnvMapState(u8 *);
    extern s32 btlUnitGetMoveStateCode(void);
    extern void btlActionQueueSortKeepingKind(s32);
    extern s32 btlActionQueueMoveToFront(s32);
    extern s32 datCalcStatusElement(s32, s32);
    extern s32 datCalcGetAffinity(u8 *, s32);
    extern u32 datCalcSetBadStatus(s32 unit, u32 badStatus);
    extern void btlCameraAssignFormationSlots(s32, s32);
    extern void btlUnitSetPos(u8 *, f32 *);
    u8 *dataOrModel;
    u8 *unit;
    u32 unitIndex;
    f32 position[3];
    s32 recordOrModel;
    u32 *entry;
    u8 *list;
    s64 previousLoadUID;
    s32 firstStatusElement;
    s32 secondStatusElement;
    u8 *enemyUnit;
    u32 i;
    u8 *actionOrPacket;
    u16 id;
    s32 modelFlags;

    previousLoadUID = 0;
    if ((((BtlBattleState *)D_0076449C)->flags & 0x20000000) == 0) {
        s32 party[4];
        u32 count;
        u8 *firstAction = NULL;
        u16 firstId = 0;

        for (count = 0; count < 4; count++) {
            party[count] = *(s32 *)&((BtlBattleState *)((u32)D_0076449C + count * 4))->encounter.partyUnits[0];
            if (party[count] == 0)
                break;
        }
        if (p4_mod_formation_mask != 0) {
            count = btlMainProjectParty(party, count);
        }
        i = 0;
        while (i < count) {
            recordOrModel = party[i];
            dataOrModel = *(u8* *)&((DatUnitPc *)recordOrModel)->base.unit;
            id = ((DatUnit *)dataOrModel)->id;
            actionOrPacket = btlActionCreateWork(0, id, (u16 *)dataOrModel);
            if (i == 0) {
                firstAction = actionOrPacket;
                firstId = id;
            }
            unit = *(u8* *)&((BtlAction *)actionOrPacket)->unit;
            datCalcClearBadStatus(*(s32 *)&((BtlUnit *)unit)->datUnit, 0xFFF7FFFF);
            ((DatUnit *)MEMBER(u8 *, unit, BtlUnit, datUnit))->flags &= 0xFFDF;
            ((DatUnit *)MEMBER(u8 *, unit, BtlUnit, datUnit))->flags &= 0xFFF7;
            ((DatUnit *)MEMBER(u8 *, unit, BtlUnit, datUnit))->flags &= 0xFFEF;
            recordOrModel = (s32)mdlFindModelObject((u16)(((BtlUnit *)unit)->genus + 1), id, 0);
            *(s32 *)&((BtlUnit *)unit)->layerHandle = recordOrModel;
            if (recordOrModel == 0) {
                actionOrPacket = btlUnitCreateModelLoadPacket(unit, id, 0x36);
                *actionOrPacket = 4;
                *(s64 *)&((BtlPacket *)actionOrPacket)->dependencyUID = previousLoadUID;
                btlPacketEnqueue(actionOrPacket, 1);
                previousLoadUID = *(s64 *)&((BtlPacket *)actionOrPacket)->uid;
            } else {
                modelFlags = *(s32 *)&((BtlUnit *)unit)->flags2 | 2;
                *(s32 *)&((BtlUnit *)unit)->flags2 = modelFlags;
                *(s32 *)&((BtlUnit *)unit)->flags2 = modelFlags | 8;
                ((BtlUnit *)unit)->sceneDrawId = MT_Scene_CreateModelCharRes(id, MEMBER(void *, unit, BtlUnit, layerHandle));
                SceneDraw_SetDirectionalOnly(MEMBER(u16, unit, BtlUnit, sceneDrawId), 1);
                btlUnitApplyModelState(unit, 1);
                btlUnitUpdateStatusMotion(unit, 0);
                btlUnitSetMotion(unit, ((BtlUnit *)unit)->restMotion, 0, ((BtlUnit *)unit)->restRate, ((BtlUnit *)unit)->restOption);
                btlUnitSetRandomAnimFrame(unit);
                dataOrModel = MEMBER(u8 *, unit, BtlUnit, layerHandle);
                if (*(s32 *)&((Model *)dataOrModel)->motion.object == 0) {
                    mdlRunDataCallback(dataOrModel);
                }
                btlUnitApplyModelState(unit, 4);
                btlUnitShowModel(unit);
                ((MdlFileModel *)((BtlUnit *)unit)->layerHandle)->flags |= 0x400;
                mdlClumpSetEnvMapState(mdlGetClump(((BtlUnit *)unit)->layerHandle));
            }
            i++;
        }
        ((BtlBattleState *)D_0076449C)->unkC64 = (u8)i;
        /* btlActionCreateWork makes Yu's action the leader (state 5, the battle's
         * first selected action). A projected lineup without Yu gives that role
         * to its first fighter; with Yu in the lineup nothing changes. */
        if (p4_mod_formation_mask != 0 && ((BtlBattleState *)D_0076449C)->selectedAction == NULL &&
            firstAction != NULL) {
            *(s16 *)&((BtlAction *)firstAction)->state = 5;
            ((BtlBattleState *)D_0076449C)->selectedAction = firstAction;
            p4_mod_formation_leader(firstId);
        }
    }
    list = *(u8* *)&((DatUnitEc *)*(u8* *)&((BtlBattleState *)D_0076449C)->encounterRecord)->base.unit;
    unitIndex = 0;
    while (unitIndex < 6) {
        if (((DatUnit *)list)->id != 0 && datCalcIsDead((s32)list, 0) == 0) {
            btlActionCreateWork(1, MEMBER(s16, list, DatUnit, id), (u16 *)list);
        }
        unitIndex++;
        list += 0x30;
    }
    if ((((BtlBattleState *)D_0076449C)->flags & 0x100) != 0) {
        entry = (u32 *)((u8 *)iGpffffb414 + (u32)((DatUnitEc *)*(u8* *)&((BtlBattleState *)D_0076449C)->encounterRecord)->encountId * 0x18);
        if ((*entry & 0x20) != 0) {
            if ((*entry & 0x10) == 0) {
                ((BtlBattleState *)D_0076449C)->openingSide = 0;
            }
            btlBossCollectTargets((u8 *)0);
        } else {
            ((BtlBattleState *)D_0076449C)->openingSide = btlUnitGetMoveStateCode();
            switch (((BtlBattleState *)D_0076449C)->openingSide) {
            case 0:
                btlActionQueueSortByRank(1);
                btlActionQueueRequeueLeadingEnemies(1);
                if ((((BtlBattleState *)D_0076449C)->flags & 0x20000000) != 0) {
                    btlActionQueueMoveToFront(*(s32 *)&((BtlBattleState *)D_0076449C)->selectedAction);
                }
                break;
            case 1:
                btlActionQueueSortByRank(0);
                btlActionQueueSortKeepingKind(0);
                btlActionQueueMoveToFront(*(s32 *)&((BtlBattleState *)D_0076449C)->selectedAction);
                ((BtlBattleState *)D_0076449C)->queueFlags |= 2;
                if ((*entry & 0x200) == 0) {
                    ((BtlBattleState *)D_0076449C)->flags |= 0x100000;
                }
                break;
            case 2:
                btlActionQueueSortByRank(0);
                btlActionQueueSortKeepingKind(1);
                btlActionQueueRequeueLeadingEnemies(1);
                ((BtlBattleState *)D_0076449C)->queueFlags |= 4;
                if ((*entry & 0x200) == 0) {
                    ((BtlBattleState *)D_0076449C)->flags |= 0x100000;
                }
                break;
            }
        }
        if (((*(u16 *)&((BtlBattleState *)D_0076449C)->encounter.unk00 & 8) != 0) && ((*entry & 0x100) == 0) &&
            ((((BtlBattleState *)D_0076449C)->flags & 0x20000000) == 0)) {
            firstStatusElement = (s16)datCalcStatusElement(-1, 4);
            secondStatusElement = (s16)datCalcStatusElement(-1, 2);
            for (enemyUnit = ((BtlBattleState *)D_0076449C)->unitLists[2]; enemyUnit != NULL; enemyUnit = *(u8* *)&((BtlUnit *)enemyUnit)->nextUnit) {
                if (*(u8* *)&((BtlUnit *)enemyUnit)->datUnit != NULL) {
                    if ((datCalcGetAffinity(*(u8* *)&((BtlUnit *)enemyUnit)->datUnit, secondStatusElement) & 0x27000000) == 0 && (s32)datCalcRandomBelow(100) < 0) {
                        datCalcSetBadStatus(MEMBER(s32, enemyUnit, BtlUnit, datUnit), 2);
                    }
                    if ((datCalcGetAffinity(MEMBER(u8 *, enemyUnit, BtlUnit, datUnit), firstStatusElement) & 0x27000000) == 0 && (s32)datCalcRandomBelow(100) < 0) {
                        datCalcSetBadStatus(MEMBER(s32, enemyUnit, BtlUnit, datUnit), 4);
                    }
                }
            }
        }
    }
    if ((((BtlBattleState *)D_0076449C)->flags & 0x20000000) == 0) {
        btlCameraAssignFormationSlots(1, 0);
        return;
    }
    btlCameraAssignFormationSlots(0, 0);
    position[1] = 0.0f;
    for (enemyUnit = ((BtlBattleState *)D_0076449C)->unitLists[2]; enemyUnit != NULL; enemyUnit = *(u8* *)&((BtlUnit *)enemyUnit)->nextUnit) {
        position[0] = (f32)(((BtlUnit *)enemyUnit)->unk_94 * 0x19 - 0x6D6);
        position[2] = (f32)(((BtlUnit *)enemyUnit)->unk_96 * 0x19 - 0x6D6);
        btlUnitSetPos(enemyUnit, position);
    }
}

static s32 sProjectedUnits[11];

static u32 btlMainProjectParty(s32 *party, u32 count)
{
    extern DatUnitPc *datUnitCreatePc(u16 pcId);
    u32 packed;
    u32 projected;
    u32 n;
    u32 i;
    u32 j;
    s32 result[4];

    packed = 0;
    for (i = 0; i < count; i++) {
        packed |= (u32)(((DatUnit *)((DatUnitPc *)party[i])->base.unit)->id & 0xFF) << (i * 8);
    }
    projected = p4_mod_formation_project(packed, count);
    n = projected >> 24;
    if (projected == 0xFFFFFFFFu || n == 0 || n > 4) {
        return count;
    }
    for (i = 0; i < n; i++) {
        u16 id = (u16)((projected >> (i * 4)) & 0xF);

        result[i] = 0;
        for (j = 0; j < count; j++) {
            if (((DatUnit *)((DatUnitPc *)party[j])->base.unit)->id == id) {
                result[i] = party[j];
                break;
            }
        }
        if (result[i] == 0) {
            if (sProjectedUnits[id] == 0) {
                sProjectedUnits[id] = (s32)datUnitCreatePc(id);
            }
            result[i] = sProjectedUnits[id];
        }
    }
    for (i = 0; i < n; i++) {
        party[i] = result[i];
    }
    return n;
}
