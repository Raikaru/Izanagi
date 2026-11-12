/* 00245030: one original term evaluator, with local tagged affinity context. */
#include "p4pc/game_elements_calc_native.h"
#include "p4pc/game_skills_native.h"
static f32 p4_calc_float(u32 term, s32 parameter, const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
f32 datScriptCalcFloat(u32 term, s32 parameter)
{ return p4_calc_float(term, parameter, NULL, NULL); }
static f32 p4_calc_float(u32 term, s32 parameter,
    const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    f32 result;
    u16 *unit;
    u8 *command;
    s32 affinityPercent;
    s16 diff;
    s16 element;
    s16 step;
    s16 tier;
    s16 sourceLevel;
    s16 targetLevel;

    switch (term) {
    case 1:
        K_ASSERT(D_00881460[0] != 0, 0xA5);
        result = (f32)(u32)datCalcGetLevel(D_00881460[0]);
        break;
    case 2:
        K_ASSERT(D_00881464[0] != 0, 0xA9);
        result = (f32)(u32)datCalcGetLevel(D_00881464[0]);
        break;
    case 3:
        K_ASSERT(D_00881460[0] != 0, 0xAD);
        result = (f32)(u32)datCalcGetHp(D_00881460[0]);
        break;
    case 4:
        K_ASSERT(D_00881464[0] != 0, 0xB1);
        result = (f32)(u32)datCalcGetHp(D_00881464[0]);
        break;
    case 5:
        K_ASSERT(D_00881460[0] != 0, 0xB5);
        result = (f32)(u32)datCalcMaxHp(D_00881460[0]);
        break;
    case 6:
        K_ASSERT(D_00881464[0] != 0, 0xB9);
        result = (f32)(u32)datCalcMaxHp(D_00881464[0]);
        break;
    case 7:
        K_ASSERT(D_00881460[0] != 0, 0xBD);
        result = (f32)(u32)D_00881460[0][5];
        break;
    case 8:
        K_ASSERT(D_00881464[0] != 0, 0xC1);
        result = (f32)(u32)D_00881464[0][5];
        break;
    case 9:
        K_ASSERT(D_00881460[0] != 0, 0xC5);
        result = (f32)(u32)datCalcMaxSp(D_00881460[0]);
        break;
    case 10:
        K_ASSERT(D_00881464[0] != 0, 0xC9);
        result = (f32)(u32)datCalcMaxSp(D_00881464[0]);
        break;
    case 11:
        K_ASSERT(D_00881460[0] != 0, 0xCD);
        result = (f32)(u32)((p4_frame ? p4_game_elements_native_affinity(D_00881460[0], p4_frame) :
            (u32)datCalcGetAffinity((u8 *)D_00881460[0],
                (s16)datCalcSkillElement((u8 *)D_00881460[0], D_0088146C[0]))) & 0xFFFF) / 100.0f;
        break;
    case 12:
        K_ASSERT(D_00881460[0] != 0, 0xD3);
        K_ASSERT(D_00881464[0] != 0, 0xD4);
        /* Only this actual target-affinity term consumes the caller's policy.
         * Source affinity and unrelated status/HP/stat terms never reuse it. */
        result = (f32)(u32)((p4_frame ? p4_game_elements_native_affinity_query(D_00881464[0], p4_frame, p4_query) :
            (u32)datCalcGetAffinity((u8 *)D_00881464[0],
                (s16)datCalcSkillElement((u8 *)D_00881460[0], D_0088146C[0]))) & 0xFFFF) / 100.0f;
        break;
    case 15:
    case 16:
        if (term == 0xF) {
            unit = D_00881460[0];
        } else {
            unit = D_00881464[0];
        }
        if (p4_frame) {
            affinityPercent = (s32)p4_game_elements_native_affinity(unit, p4_frame);
        } else {
            element = (s16)datCalcStatusElement((s16)D_0088146C[0], D_00881470[0]);
            K_ASSERT(element < 0x13, 0xE3);
            switch (element) {
            case -1:
            case 0x10:
            case 0x11:
                affinityPercent = 0x64;
                break;
            default:
                affinityPercent = datCalcGetAffinity((u8 *)unit, element);
                break;
            }
        }
        result = (f32)(u32)(affinityPercent & 0xFFFF) / 100.0f;
        break;
    case 13:
        K_ASSERT(parameter < 5, 0xF7);
        K_ASSERT(D_00881460[0] != 0, 0xF8);
        result = (f32)(u32)datCalcGetBattleStat(D_00881460[0], parameter & 0xFFFF);
        break;
    case 14:
        K_ASSERT(parameter < 5, 0x101);
        K_ASSERT(D_00881464[0] != 0, 0x102);
        result = (f32)(u32)datCalcGetBattleStat(D_00881464[0], parameter & 0xFFFF);
        break;
    case 17:
        K_ASSERT(p4_game_skill_active((s32)D_0088146C[0]), 0x106);
        if (!p4_frame && p4_game_elements_native_enabled()) p4_game_elements_native_skill_use(D_0088146C[0]);
        command = &DAT_007644A8[D_0088146C[0] * 0x28];
        if (command[0] & 2) {
            K_ASSERT(D_00881460[0] != 0, 0x109);
            result = (f32)(u32)datCalcGetWeaponProperty(D_00881460[0], 1);
        } else {
            result = (f32)(u32)command[0xE];
        }
        break;
    case 18:
        K_ASSERT(p4_game_skill_active((s32)D_0088146C[0]), 0x110);
        if (!p4_frame && p4_game_elements_native_enabled()) p4_game_elements_native_skill_use(D_0088146C[0]);
        result = (f32)(u32)DAT_007644A8[D_0088146C[0] * 0x28 + 0x19];
        break;
    case 19:
        K_ASSERT(p4_game_skill_active((s32)D_0088146C[0]), 0x114);
        if (!p4_frame && p4_game_elements_native_enabled()) p4_game_elements_native_skill_use(D_0088146C[0]);
        command = &DAT_007644A8[D_0088146C[0] * 0x28];
        if ((command[0] & 2) || (command[0x11] == 0x10)) {
            K_ASSERT(D_00881460[0] != 0, 0x118);
            result = (f32)(u32)datCalcGetWeaponProperty(D_00881460[0], 0);
        } else if (D_00881470[0] == 1) {
            result = (f32)((DatSkillEntry *)command)->power;
        } else if (D_00881470[0] == 2) {
            result = (f32)((DatSkillEntry *)command)->spPower;
        } else {
            kAssertFail(D_00635980, 0x123);
        }
        break;
    case 20:
        K_ASSERT(p4_game_skill_active((s32)D_0088146C[0]), 0x128);
        if (!p4_frame && p4_game_elements_native_enabled()) p4_game_elements_native_skill_use(D_0088146C[0]);
        command = &DAT_007644A8[D_0088146C[0] * 0x28];
        if (command[0] & 2) {
            K_ASSERT(D_00881460[0] != 0, 0x12B);
            result = (f32)(u32)datCalcGetWeaponProperty(D_00881460[0], 2);
        } else {
            result = (f32)(u32)command[0x25];
        }
        break;
    case 21:
        K_ASSERT(p4_game_skill_active((s32)D_0088146C[0]), 0x132);
        result = 100.0f;
        break;
    case 22:
        result = 31000.0f;
        break;
    case 23:
        K_ASSERT(D_00881460[0] != 0, 0x139);
        K_ASSERT(D_00881464[0] != 0, 0x13A);
        sourceLevel = (s16)(datCalcGetLevel(D_00881460[0]) & 0xFF);
        targetLevel = (s16)(datCalcGetLevel(D_00881464[0]) & 0xFF);
        diff = (s16)(sourceLevel - targetLevel);
        if (diff < -0x1F) {
            diff = -0x1F;
        } else if (diff > 0x1F) {
            diff = 0x1F;
        }
        if ((datCalcEnemyHasTableFlag(D_00881460[0]) == 0) && (datCalcEnemyHasTableFlag(D_00881464[0]) == 0)) {
            result = ((f32 *)DAT_007644E4)[(s16)(diff + 0x1F)];
        } else {
            result = ((f32 *)DAT_007644E4)[(s16)(diff + 0x5E)];
        }
        break;
    case 24:
        K_ASSERT(D_00881460[0] != 0, 0x160);
        K_ASSERT(D_00881464[0] != 0, 0x161);
        sourceLevel = (s16)(datCalcGetLevel(D_00881460[0]) & 0xFF);
        targetLevel = (s16)(datCalcGetLevel(D_00881464[0]) & 0xFF);
        if (targetLevel < sourceLevel) {
            step = (s16)((sourceLevel - targetLevel) - 1);
        } else {
            step = (s16)((sourceLevel - targetLevel) + 1);
        }
        step /= 3;
        if (step < -6) {
            step = -6;
        } else if (step > 6) {
            step = 6;
        }
        result = ((f32 *)(DAT_007644E8 + 0x18))[step];
        break;
    case 25:
        K_ASSERT(D_00881460[0] != 0, 0x173);
        tier = (s16)((s16)(datCalcGetBattleStat(D_00881460[0], PERSONA_STAT_MAG) & 0xFF) / 10);
        if (tier > 9) {
            tier = 9;
        }
        result = (f32)((s16 *)DAT_007644F0)[tier];
        break;
    case 26:
        K_ASSERT(D_00881460[0] != 0, 0x17B);
        tier = (s16)((s16)(datCalcGetBattleStat(D_00881460[0], PERSONA_STAT_MAG) & 0xFF) / 5);
        if (tier > 0x14) {
            tier = 0x14;
        }
        result = (f32)((s16 *)DAT_007644F4)[tier];
        break;
    case 27:
        result = (f32)(((BtlBattleState *)DAT_0076449c)->partyActionCount + 1);
        break;
    case 28:
        result = (f32)D_00881468[0];
        break;
    case 29:
        K_ASSERT(D_00881460[0] != 0, 0x189);
        result = (f32)(u32)datCalcGetArmorProperty(D_00881460[0], 0);
        break;
    case 30:
        K_ASSERT(D_00881460[0] != 0, 0x18D);
        result = (f32)(u32)datCalcGetArmorProperty(D_00881460[0], 1);
        break;
    case 31:
        K_ASSERT(D_00881464[0] != 0, 0x191);
        result = (f32)(u32)datCalcGetArmorProperty(D_00881464[0], 0);
        break;
    case 32:
        K_ASSERT(D_00881464[0] != 0, 0x195);
        result = (f32)(u32)datCalcGetArmorProperty(D_00881464[0], 1);
        break;
    case 33:
        K_ASSERT(D_00881460[0] != 0, 0x199);
        K_ASSERT(D_00881464[0] != 0, 0x19A);
        sourceLevel = (s16)(datCalcGetLevel(D_00881460[0]) & 0xFF);
        targetLevel = (s16)(datCalcGetLevel(D_00881464[0]) & 0xFF);
        diff = (s16)(sourceLevel - targetLevel);
        if (diff < -0x1F) {
            diff = -0x1F;
        } else if (diff > 0x1F) {
            diff = 0x1F;
        }
        if ((datCalcEnemyHasTableFlag(D_00881460[0]) == 0) && (datCalcEnemyHasTableFlag(D_00881464[0]) == 0)) {
            result = ((f32 *)DAT_007644E4)[(s16)(diff + 0x1F)];
        } else {
            result = ((f32 *)DAT_007644E4)[(s16)(diff + 0x5E)];
        }
        break;
    case 34:
        result = (f32)(u32)((BtlBattleState *)DAT_0076449c)->escapeCount;
        break;
    default:
        kAssertFail(D_00635980, 0x1C3);
        break;
    }
    return result;
}
float p4_game_elements_calculation_float(uint32_t term, int32_t parameter,
    const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ return p4_calc_float(term, parameter, frame, query); }
