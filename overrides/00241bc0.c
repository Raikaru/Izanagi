/* 00241bc0: preserve native eligibility before resolving tagged affinity. */
#include "p4pc/game_elements_native.h"
#include "p4pc/game_skills_native.h"
s32 datCalcSkillAppliesToTarget(source, target, skillId, channel, unusedMode)
u8 *source;
u8 *target;
s32 skillId;
s32 channel;
s32 unusedMode;
{
    /* Skill rows are addressed by byte offset; DatSkillEntry in datCalc_internal.h documents the
     * 0x28-byte layout. */
    u16 idx;
    u32 offset;
    u8 *base;
    u8 *entry;
    u32 flag1;
    s16 value;

    idx = (u16)skillId;
    if (!p4_game_skill_active(idx)) {
        kAssertFail(D_00635938, 0x1305);
    }
    flag1 = ((DatUnit *)target)->bad;
    if (((u32)((flag1 & 0x100000) != 0) != 0)) {
        return 0;
    }
    if ((u16)channel != 1) {
        return 0;
    }
    if ((((DatUnit *)target)->flags & 8)) {
        return 0;
    }
    offset = (u32)(u16)skillId * 0x28;
    base = iGpffffb3b8;
    entry = base + offset;
    if (entry[0x11] == 0 && (entry[0x18] != 1 || ((((DatSkillEntry *)(entry))->statusMask & 0x80000) == 0))) {
        return 0;
    }
    if (p4_game_elements_native_enabled()) {
        P4GameActionFrame actionFrame;
        p4_game_elements_native_applicability_frame(source, idx, &actionFrame);
        return p4_game_action_element_has_affinity(&actionFrame.element);
    }
    if (source != 0 && p4_game_skill_active(idx) && ((((DatSkillEntry *)(base + offset))->flags00 & 2))) {
        if ((((DatUnit *)source)->flags & 4)) {
            s32 partyOffset;
            u8 *partyPtr;

            partyOffset = (s32)(((DatUnit *)source)->id * 0x3C);
            partyPtr = iGpffffb3c4 + partyOffset;
            value = (s16)partyPtr[0x38];
        } else {
            value = (s16)datItemElement((s16)((u16)datGetEquippedItemByUnit((s16)((DatUnit *)source)->id, 0) & 0xFFFF));
        }
    } else {
        value = p4_game_skill_element((u16)skillId, ((s8 *)iGpffffb3b4)[((u32)(u16)skillId * 2)]);
    }
    if (value >= 0x13) {
        kAssertFail(D_00635938, 0xE47);
    }
    if (((1 << (value + 1)) & 0xE0001) == 0) {
        goto one;
    }
    return 0;
one:
    return 1;
}
