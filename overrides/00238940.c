/* 00238940: tagged status and KO affinity at the original query boundary. */
#include "p4pc/game_elements_native.h"
#include "p4pc/game_skills_native.h"
#pragma push
/* measured: retail hoists loop invariants in this function. */
#pragma opt_loop_invariants on
s32 datCalcInflictStatus(skillId, source, target, options)
u16 skillId;
DatUnit *source;
DatUnit *target;
s32 options;
{
    u16 choices[24];
    s32 offset;
    u8 *entry;
    u8 mode;
    u8 kind;
    u32 statuses;
    s32 success;
    s32 index;
    s32 count;
    u32 choiceCount;
    s32 resistance;
    u32 percentage;
    f32 multiplier;
    f32 percent;
    f32 chance;
    s32 death;
    s32 rate;
    s32 element;
    u8 active;
    s16 message;
    extern s32 datCalcIsGuarding(u16 *unitFlags);
    extern s32 datCalcStatusElement(s32 skillId, s32 statusMask);

    if (!p4_game_skill_registered(skillId & 0xFFFF)) {
        kAssertFail(D_00635938, 0xA30);
    }
    offset = (u32)(u16)skillId * 0x28;
    entry = iGpffffb3b8 + offset;
    if (entry[0x24] == 13) {
        return 0x80000;
    }
    multiplier = 1.0f;
    success = 0;
    statuses = ((DatSkillEntry *)(entry))->statusMask & 0xFFFFFFU;
    if (entry[0x18] == 3) {
        count = 0;
        index = 0;
        while ((index & 0xFFFF) < 24) {
            if (statuses & (1U << (index & 0xFFFF))) {
                choices[count & 0xFFFF] = index;
                count = (count + 1) & 0xFFFF;
            }
            index = (index + 1) & 0xFFFF;
        }
        choiceCount = count & 0xFFFF;
        if (choiceCount == 0) {
            kAssertFail(D_00635938, 0x17);
        }
        statuses = 1U << choices[RpRandom() % choiceCount];
    }
    if (statuses != 0 &&
        ((entry = iGpffffb3b8, mode = entry[MEMBER_OFFSET(DatSkillEntry, mode18) + offset], mode == 1) || mode == 3)) {
        switch (entry[offset + 0x24]) {
        case 14: {
            if ((u8)(((u8 *)target)[0xC] != 0)) {
                goto return_status_0;
            }
            goto return_zero_0;
return_status_0:
            return (s32)statuses;
return_zero_0:
            return 0;
        }
        default:
            break;
        }
        if (datCalcIsGuarding((u16 *)target)) {
            return 0;
        }
        kind = iGpffffb3b8[offset + 0x24];
        switch (kind) {
        case 1:
            if ((u8)((((DatUnit *)target)->bad & 4) != 0)) {
                return (s32)statuses;
            }
            break;
        case 7:
            if (((DatUnit *)target)->flags & 0x1000) {
                statuses = 0;
            }
            return (s32)statuses;
        case 8:
            switch (((DatUnit *)target)->flags & 0x1000) {
            case 0:
                statuses = 0;
                break;
            default:
                break;
            }
            return (s32)statuses;
        case 9:
            if ((u8)((((DatUnit *)target)->bad & 1) != 0)) {
                goto return_status_1;
            }
            goto return_zero_1;
return_status_1:
            return (s32)statuses;
return_zero_1:
            return 0;
        default:
            break;
        }
        if (p4_game_elements_native_enabled()) {
            P4GameActionFrame statusFrame;
            const u32 component = statuses == 0x80000u ? P4_ACTION_COMPONENT_INSTANT_KO : P4_ACTION_COMPONENT_STATUS;
            p4_game_elements_native_action_frame(source, skillId, component, statuses, options, &statusFrame);
            resistance = p4_game_elements_native_affinity(target, &statusFrame);
        } else {
            resistance = datCalcGetAffinity((u8 *)target, (s16)datCalcStatusElement((s16)skillId, (s32)statuses));
        }
        if (resistance & 0x27000000) {
            return 0;
        }
        if (statuses & 0x100000) {
            if ((u8)((((DatUnit *)target)->bad & 0x100000) != 0)) {
                return 0;
            }
            if (((DatUnit *)target)->flags & 8) {
                return 0;
            }
        }
        percentage = resistance & 0xFFFF;
        percent = (f32)percentage;
        multiplier = multiplier * (percent / 100.0f);
        if (!(options & 1)) {
            death = statuses & 0x80000;
            if (death == 0 && datCalcGetBuffLevel(target, 7) > 0) {
                multiplier *= 2.0f;
            }
            if (p4_game_elements_native_enabled()) {
                P4GameActionFrame skillFrame;
                p4_game_elements_native_action_frame(source, skillId, P4_ACTION_COMPONENT_DAMAGE, 0, options, &skillFrame);
                element = (s32)skillFrame.element.family;
            } else {
                element = (s16)datCalcSkillElement(source, skillId);
            }
            switch (element) {
            case 9:
                if (datCalcHasRequestedSkill(source, 0x217)) multiplier *= 1.25f;
                break;
            case 8:
                if (datCalcHasRequestedSkill(source, 0x216)) multiplier *= 1.25f;
                break;
            case 10:
                if (datCalcHasRequestedSkill(source, 0x21A)) multiplier *= 1.25f;
                break;
            case 6:
                if (datCalcHasRequestedSkill(source, 0x21F)) multiplier *= 1.25f;
                break;
            case 7:
                if (datCalcHasRequestedSkill(source, 0x220)) multiplier *= 1.25f;
                break;
            case 13:
                if (datCalcHasRequestedSkill(source, 0x218)) multiplier *= 1.25f;
                break;
            case 15:
                if (datCalcHasRequestedSkill(source, 0x219)) multiplier *= 1.25f;
                break;
            case 11:
                if (datCalcHasRequestedSkill(source, 0x21B)) multiplier *= 1.25f;
                break;
            case 14:
                if (datCalcHasRequestedSkill(source, 0x21D)) multiplier *= 1.25f;
                break;
            }
            if (death == 0 && datCalcHasRequestedSkill(source, 0x21E)) {
                multiplier *= 1.25f;
            }
        }
    }
    if (statuses == 0) {
        return 0;
    }
    if ((s32)iGpffffb3b8[offset + 0x19] >= 100) {
        success = 1;
    } else {
        chance = (f32)datScriptRunCalc(DAT_SCRIPT_CALC_RATIO_OF_PRODUCTS, (u16 *)(source), (u16 *)(target), skillId, (s32)statuses);
        chance = chance * multiplier;
        rate = (s32)chance;
        if (rate > 99) {
            rate = 99;
        } else if (rate < 0) {
            rate = 0;
        }
        if ((s32)(RpRandom() % 100U) < rate) {
            success = 1;
        }
    }
    if (success != 0 && (statuses & 0x80000) != 0 &&
        (s8)(active = (((DatCalcUnitState *)target)->buffFlags & 0x800000) != 0) > 0) {
        statuses &= ~0x80000U;
        if (((DatUnit *)target)->flags & 4) {
            message = 0x79;
        } else {
            message = 0x78;
        }
        ((DatCalcUnitState *)target)->message = message;
        ((DatCalcUnitState *)target)->consumedFlags |= 0x800000;
    }
    switch (success) {
    case 0:
        statuses = 0;
        break;
    default:
        break;
    }
    return (s32)statuses;
}
#pragma pop
