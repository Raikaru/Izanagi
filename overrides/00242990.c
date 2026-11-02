/* 00242990: preserve substitution policy while classifying full-width families. */
#include "p4pc/game_elements_native.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_enemies_native.h"
#pragma push
/* measured: retail hoists loop-invariant values out of the skill-search loops. */
#pragma opt_loop_invariants on
s32 datCalcResolveSkillId(DatUnit *unit, s32 baseSkill, DatUnit *attacker, u32 statuses)
{
    /* Skill rows are addressed by byte offset; DatSkillEntry in datCalc_internal.h documents the
     * 0x28-byte layout. */
    s32 skillId;
    s32 equipment;
    s32 found;
    u8 *skills;
    s32 priorityIndex;
    s32 secondaryPriorityIndex;
    s32 lightChanceIndex;
    s32 lightIndex;
    s32 darkChanceIndex;
    s32 darkIndex;
    s32 firstFallbackIndex;
    s32 lastFallbackIndex;
    s32 value;
    s32 tmp;
    s32 replacementSkill;

    replacementSkill = -1;
    skillId = baseSkill & 0xFFFF;
    if (p4_game_skill_active(skillId) && (iGpffffb3b8[skillId * 0x28 + 0x24] == 0xD)) {
        return -1;
    }
    if (!(unit->flags & 4)) {
        if (unit->id >= 0xB) {
            kAssertFail(D_00635938, 0xFA5);
        }
        skills = datPersonaGetSkillsByPcId(unit->id);
    } else {
        if (unit->id >= p4_game_enemies_native_limit()) {
            kAssertFail(D_00635938, 0xFA8);
        }
        skills = unit->id * 0x3C + iGpffffb3c4 + 0xE;
    }
    priorityIndex = 0;
    while ((priorityIndex & 0xFFFF) < 8) {
        if (((u16 *)(skills))[(u16)priorityIndex] == 0x231) {
            found = 1;
            goto done1;
        }
        priorityIndex = (priorityIndex + 1) & 0xFFFF;
    }
    if (!(unit->flags & 4)) {
        equipment = (s32)((u32)(u16)datGetEquippedItemByUnit((s16)unit->id, 2) & 0xFFFF);
        if (equipment >= 0) {
            if (datItemGrantedSkill((s16)equipment) == 0x231) {
                found = 1;
                goto done1;
            }
        }
    }
    found = 0;
done1:
    if (found != 0) {
        return 0x231;
    }
    if (!(unit->flags & 4)) {
        if (unit->id >= 0xB) {
            kAssertFail(D_00635938, 0xFA5);
        }
        skills = datPersonaGetSkillsByPcId(unit->id);
    } else {
        if (unit->id >= p4_game_enemies_native_limit()) {
            kAssertFail(D_00635938, 0xFA8);
        }
        skills = unit->id * 0x3C + iGpffffb3c4 + 0xE;
    }
    secondaryPriorityIndex = 0;
    while ((secondaryPriorityIndex & 0xFFFF) < 8) {
        if (((u16 *)(skills))[(u16)secondaryPriorityIndex] == 0x232) {
            found = 1;
            goto done2;
        }
        secondaryPriorityIndex = (secondaryPriorityIndex + 1) & 0xFFFF;
    }
    if (!(unit->flags & 4)) {
        equipment = (s32)((u32)(u16)datGetEquippedItemByUnit((s16)unit->id, 2) & 0xFFFF);
        if (equipment >= 0) {
            if (datItemGrantedSkill((s16)equipment) == 0x232) {
                found = 1;
                goto done2;
            }
        }
    }
    found = 0;
done2:
    if (found != 0) {
        return 0x232;
    }
    if (unit->flags & 0x20) {
        return 0;
    }
    if (p4_game_elements_native_enabled() && p4_game_skill_active(skillId)) {
        value = -1;
        if (statuses & 0x80000u) {
            P4GameActionFrame actionFrame;
            p4_game_elements_native_action_frame(attacker, skillId, P4_ACTION_COMPONENT_INSTANT_KO, 0x80000u, 0, &actionFrame);
            if (actionFrame.element.channel_id == P4_CHANNEL_INSTANT_KO) {
                if (!actionFrame.owned_binding) {
                    /* Retail keys these passives to the attacking skill's
                     * element, not every status which defaults to Dark KO. */
                    p4_game_elements_native_action_frame(attacker, skillId, P4_ACTION_COMPONENT_DAMAGE, 0, 0, &actionFrame);
                }
                value = (s32)actionFrame.element.family;
            }
        }
    } else if ((unit != 0) && p4_game_skill_active(skillId) && ((iGpffffb3b8[(u16)baseSkill * 0x28] & 2) != 0)) {
        if (unit->flags & 4) {
            value = ((DatEnemyUnitInit *)byteAddressAdd(unit->id * 0x3C, iGpffffb3c4))->weaponElement;
        } else {
            value = (s16)datItemElement((s16)((u16)datGetEquippedItemByUnit((s16)unit->id, 0) & 0xFFFF));
        }
    } else {
        value = p4_game_skill_element((u16)baseSkill, ((s8 *)iGpffffb3b4)[((u32)(u16)baseSkill * 2)]);
    }
    if (value == 6) {
        if (unit->flags & 4) {
            tmp = 0;
        } else {
            tmp = (datItemCount(0x340) & 0xFF) > 0;
        }
        if (tmp != 0) {
            replacementSkill = 0x154;
        }
        if (!(unit->flags & 4)) {
            if (unit->id >= 0xB) {
                kAssertFail(D_00635938, 0xFA5);
            }
            skills = datPersonaGetSkillsByPcId(unit->id);
        } else {
            if (unit->id >= p4_game_enemies_native_limit()) {
                kAssertFail(D_00635938, 0xFA8);
            }
            skills = unit->id * 0x3C + iGpffffb3c4 + 0xE;
        }
        lightChanceIndex = 0;
        while ((lightChanceIndex & 0xFFFF) < 8) {
            if (((u16 *)(skills))[(u16)lightChanceIndex] == 0x211) {
                found = 1;
                goto done3;
            }
            lightChanceIndex = (lightChanceIndex + 1) & 0xFFFF;
        }
        if (!(unit->flags & 4)) {
            equipment = (s32)((u32)(u16)datGetEquippedItemByUnit((s16)unit->id, 2) & 0xFFFF);
            if (equipment >= 0) {
                if (datItemGrantedSkill((s16)equipment) == 0x211) {
                    found = 1;
                    goto done3;
                }
            }
        }
        found = 0;
done3:
        if ((found != 0) && ((s32)(RpRandom() % 100U) < 0x32)) {
            replacementSkill = 0x211;
        }
        if (!(unit->flags & 4)) {
            if (unit->id >= 0xB) {
                kAssertFail(D_00635938, 0xFA5);
            }
            skills = datPersonaGetSkillsByPcId(unit->id);
        } else {
            if (unit->id >= p4_game_enemies_native_limit()) {
                kAssertFail(D_00635938, 0xFA8);
            }
            skills = unit->id * 0x3C + iGpffffb3c4 + 0xE;
        }
        lightIndex = 0;
        while ((lightIndex & 0xFFFF) < 8) {
            if (((u16 *)(skills))[(u16)lightIndex] == 0x221) {
                found = 1;
                goto done4;
            }
            lightIndex = (lightIndex + 1) & 0xFFFF;
        }
        if (!(unit->flags & 4)) {
            equipment = (s32)((u32)(u16)datGetEquippedItemByUnit((s16)unit->id, 2) & 0xFFFF);
            if (equipment >= 0) {
                if (datItemGrantedSkill((s16)equipment) == 0x221) {
                    found = 1;
                    goto done4;
                }
            }
        }
        found = 0;
done4:
        if (found != 0) {
            replacementSkill = 0x221;
        }
    } else if (value == 7) {
        if (unit->flags & 4) {
            tmp = 0;
        } else {
            tmp = (datItemCount(0x340) & 0xFF) > 0;
        }
        if (tmp != 0) {
            replacementSkill = 0x154;
        }
        if (!(unit->flags & 4)) {
            if (unit->id >= 0xB) {
                kAssertFail(D_00635938, 0xFA5);
            }
            skills = datPersonaGetSkillsByPcId(unit->id);
        } else {
            if (unit->id >= p4_game_enemies_native_limit()) {
                kAssertFail(D_00635938, 0xFA8);
            }
            skills = unit->id * 0x3C + iGpffffb3c4 + 0xE;
        }
        darkChanceIndex = 0;
        while ((darkChanceIndex & 0xFFFF) < 8) {
            if (((u16 *)(skills))[(u16)darkChanceIndex] == 0x212) {
                found = 1;
                goto done5;
            }
            darkChanceIndex = (darkChanceIndex + 1) & 0xFFFF;
        }
        if (!(unit->flags & 4)) {
            equipment = (s32)((u32)(u16)datGetEquippedItemByUnit((s16)unit->id, 2) & 0xFFFF);
            if (equipment >= 0) {
                if (datItemGrantedSkill((s16)equipment) == 0x212) {
                    found = 1;
                    goto done5;
                }
            }
        }
        found = 0;
done5:
        if ((found != 0) && ((s32)(RpRandom() % 100U) < 0x32)) {
            replacementSkill = 0x212;
        }
        if (!(unit->flags & 4)) {
            if (unit->id >= 0xB) {
                kAssertFail(D_00635938, 0xFA5);
            }
            skills = datPersonaGetSkillsByPcId(unit->id);
        } else {
            if (unit->id >= p4_game_enemies_native_limit()) {
                kAssertFail(D_00635938, 0xFA8);
            }
            skills = unit->id * 0x3C + iGpffffb3c4 + 0xE;
        }
        darkIndex = 0;
        while ((darkIndex & 0xFFFF) < 8) {
            if (((u16 *)(skills))[(u16)darkIndex] == 0x222) {
                found = 1;
                goto done6;
            }
            darkIndex = (darkIndex + 1) & 0xFFFF;
        }
        if (!(unit->flags & 4)) {
            equipment = (s32)((u32)(u16)datGetEquippedItemByUnit((s16)unit->id, 2) & 0xFFFF);
            if (equipment >= 0) {
                if (datItemGrantedSkill((s16)equipment) == 0x222) {
                    found = 1;
                    goto done6;
                }
            }
        }
        found = 0;
done6:
        if (found != 0) {
            replacementSkill = 0x222;
        }
    }
    if ((s16)replacementSkill != -1) {
        goto tail;
    }
    if (!(unit->flags & 0x800)) {
        if (!(unit->flags & 4)) {
            if (unit->id >= 0xB) {
                kAssertFail(D_00635938, 0xFA5);
            }
            skills = datPersonaGetSkillsByPcId(unit->id);
        } else {
            if (unit->id >= p4_game_enemies_native_limit()) {
                kAssertFail(D_00635938, 0xFA8);
            }
            skills = unit->id * 0x3C + iGpffffb3c4 + 0xE;
        }
        firstFallbackIndex = 0;
        while ((firstFallbackIndex & 0xFFFF) < 8) {
            if (((u16 *)(skills))[(u16)firstFallbackIndex] == 0x210) {
                found = 1;
                goto done7;
            }
            firstFallbackIndex = (firstFallbackIndex + 1) & 0xFFFF;
        }
        if (!(unit->flags & 4)) {
            equipment = (s32)((u32)(u16)datGetEquippedItemByUnit((s16)unit->id, 2) & 0xFFFF);
            if (equipment >= 0) {
                if (datItemGrantedSkill((s16)equipment) == 0x210) {
                    found = 1;
                    goto done7;
                }
            }
        }
        found = 0;
done7:
        if (found != 0) {
            replacementSkill = 0x210;
            goto tail;
        }
    }
    if (unit->flags & 0x400) {
        goto tail;
    }
    if (!(unit->flags & 4)) {
        if (unit->id >= 0xB) {
            kAssertFail(D_00635938, 0xFA5);
        }
        skills = datPersonaGetSkillsByPcId(unit->id);
    } else {
        if (unit->id >= p4_game_enemies_native_limit()) {
            kAssertFail(D_00635938, 0xFA8);
        }
        skills = unit->id * 0x3C + iGpffffb3c4 + 0xE;
    }
    lastFallbackIndex = 0;
    while ((lastFallbackIndex & 0xFFFF) < 8) {
        if (((u16 *)(skills))[(u16)lastFallbackIndex] == 0x20F) {
            found = 1;
            goto done8;
        }
        lastFallbackIndex = (lastFallbackIndex + 1) & 0xFFFF;
    }
    if (!(unit->flags & 4)) {
        equipment = (s32)((u32)(u16)datGetEquippedItemByUnit((s16)unit->id, 2) & 0xFFFF);
        if (equipment >= 0) {
            if (datItemGrantedSkill((s16)equipment) == 0x20F) {
                found = 1;
                goto done8;
            }
        }
    }
    found = 0;
done8:
    if (found != 0) {
        replacementSkill = 0x20F;
    }
tail:
    if (((s16)replacementSkill == -1) && (!(unit->flags & 4)) && (unit->id != 1) && (!(unit->flags & 0x400))) {
        if ((s32)(cmmCommunityRecordValue((u16)(cmmCommunityForArcana((s16)unit->id) & 0xFFFF)) & 0xFFFF) >= 9) {
            replacementSkill = 0x20F;
        }
    }
    return replacementSkill;
}
#pragma pop
