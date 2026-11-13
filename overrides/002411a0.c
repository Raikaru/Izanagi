/* 002411a0: retain critical/weak ordering with a tagged action frame.
 * PortApiData facet: the critical chance passes the hit stage after its
 * 0..100 clamp; the stage never runs while no modifier is registered. */
#include "p4pc/game_elements_native.h"
#include "p4pc/game_hit_stage_native.h"
#include "p4pc/game_skills_native.h"
s32 datCalcHitResult(source, target, skillId, outcome, unusedMode)
u8 *source;
u8 *target;
u16 skillId;
s32 outcome;
s32 unusedMode;
{
    /* Skill rows are addressed by byte offset; DatSkillEntry in datCalc_internal.h documents the
     * 0x28-byte layout. */
    s32 tmp22;
    s32 lvl;
    s32 idx;
    s32 bits;
    u16 element;
    u16 enemyElement;
    s32 res;
    s32 base;
    s32 thresh;
    f32 f20;
    u8 *entry;
    u16 id16;
    s32 bonus;
    s32 scaled;
    s32 total;
    P4GameActionFrame actionFrame;
    s32 hasActionFrame;

    tmp22 = skillId & 0xFFFF;
    if (!p4_game_skill_active(tmp22)) {
        kAssertFail(D_00635938, 0x122E);
    }
    switch (outcome & 0xFFFF) {
    case 0x200:
    case 0x400:
    case 0x100:
    case 2:
    case 4:
        return 1;
    }
    hasActionFrame = p4_game_elements_native_enabled();
    if (hasActionFrame) {
        idx = (u32)skillId % 0x10000U;
        p4_game_elements_native_primary_frame(source, (u32)skillId, 0, &actionFrame);
    } else if ((source != 0) && p4_game_skill_active(tmp22) && ((iGpffffb3b8[(idx = (u32)skillId % 0x10000U) * 0x28] & 2) != 0)) {
        if ((((DatUnit *)source)->flags & 4) != 0) {
            id16 = ((DatUnit *)source)->id;
            enemyElement = iGpffffb3c4[id16 * 0x3C + 0x38];
            lvl = (s16)enemyElement;
        } else {
            element = datGetEquippedItemByUnit((s16)((DatUnit *)source)->id, 0);
            lvl = (s16)datItemElement((s16)element);
        }
    } else {
        idx = (u32)skillId % 0x10000U;
        lvl = p4_game_skill_element(idx, ((s8 *)iGpffffb3b4)[idx * 2]);
    }
    if (hasActionFrame) {
        if (!p4_game_action_element_has_affinity(&actionFrame.element)) return 1;
        res = (s32)p4_game_elements_native_element_affinity(target, &actionFrame);
        /* A physical delivery takes the crit roll (hardmode-design 8.1);
         * Almighty physical keeps its retail test. */
        if (!p4_game_action_frame_physical(&actionFrame) &&
            (actionFrame.element.family != 5u || iGpffffb3b8[idx * 0x28 + 2] != 1))
            return (res & 0x08000000) ? 4 : 1;
    } else {
        bits = (s16)lvl;
        if (bits >= 0x13) {
            kAssertFail(D_00635938, 0xE47);
        }
        bits = 1U << (bits + 1);
        if ((bits & 0xE0001) != 0) {
            return 1;
        }
        res = datCalcGetElementAffinity(target, lvl);
        if (((bits & 2) == 0) && (((bits & 0x40) == 0) || (iGpffffb3b8[idx * 0x28 + 2] != 1))) {
            if (res & 0x08000000) {
                return 4;
            }
            return 1;
        }
    }
    idx *= 0x28;
    entry = iGpffffb3b8;
    if (entry[idx + 0x25] == 0) {
        return (res & 0x08000000) != 0 ? 4 : 1;
    }
    if ((((DatUnit *)target)->flags & 0x80) != 0) {
        return (res & 0x08000000) != 0 ? 4 : 1;
    }
    if ((((DatUnit *)source)->flags & 4) == (((DatUnit *)target)->flags & 4)) {
        return (res & 0x08000000) != 0 ? 4 : 1;
    }
    if ((res & 0x08000000) != 0) {
        return 4;
    }
    if (datCalcIsGuarding((u16 *)target) != 0) {
        return 1;
    }
    base = 0;
    thresh = 0;
    f20 = 1.0f;
    entry = iGpffffb3b8;
    if (entry[idx + 0x25] == 0x64) {
        base = 0x64;
        thresh = 0x64;
    }
    switch (entry[idx + 2]) {
    case 1:
        if (base == 0) {
            base = datScriptRunCalc(DAT_SCRIPT_CALC_SCALED_RATIO, (u16 *)(source), (u16 *)(target), skillId, 0);
        }
        if (datCalcHasFixedSkill(target, 0x206)) {
            f20 = f20 * 0.5f;
        }
        if (datCalcHasFixedSkill(source, 0x207)) {
            f20 = f20 * 2.0f;
        }
        scaled = (s32)((f32)base * f20);
        bonus = 0;
        if (!p4_game_skill_active(tmp22)) {
            kAssertFail(D_00635938, 0xF5E);
        }
        if ((u8)((((DatSkillEntry *)(iGpffffb3b8 + idx))->flags00 & 2) != 0)) {
            if (datCalcHasEquipmentProperty(source, 0x3C)) {
                bonus += 5;
            }
            if (datCalcHasEquipmentProperty(source, 0x3D)) {
                bonus += 10;
            }
            if (datCalcHasEquipmentProperty(source, 0x3E)) {
                bonus += 15;
            }
        }
        if (datCalcGetBuffLevel((DatUnit *)source, 5) > 0) {
            bonus += 7;
        }
        if (datCalcGetBuffLevel((DatUnit *)source, 6) > 0) {
            bonus += 15;
        }
        total = scaled + bonus;
        if (total > 0x64) {
            total = 0x64;
        } else if (total < 0) {
            total = 0;
        }
        if (p4_game_hit_native_active()) {
            total = p4_game_hit_native_rate(P4_HIT_CRITICAL, (u32)(u16)skillId,
                (((DatUnit *)source)->flags & 4) != 0, ((DatUnit *)source)->id,
                (((DatUnit *)target)->flags & 4) != 0, ((DatUnit *)target)->id, total);
        }
        if ((s32)(RpRandom() % 100) < thresh) {
            return 2;
        }
        if ((s32)(RpRandom() % 100) < 0) {
            return 2;
        }
        if ((s32)(RpRandom() % 100) < total) {
            return 2;
        }
        break;
    case 2:
        break;
    default:
        kAssertFail(D_00635938, 0x12EF);
        break;
    }
    return 1;
}
