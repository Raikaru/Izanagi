/* 0023e6f0: preserve retail flow while resolving owned wide action frames. */
#include "p4pc/game_elements_native.h"
#include "p4pc/game_elements_calc_native.h"
#include "p4pc/game_skills_native.h"
s32 datCalcResolveSkillHit(source, target, skillId, options)
u8 *source;
u8 *target;
u16 skillId;
s32 options;
{
    DatUnit *targetRecord = (DatUnit *)target;
    /* Byte-pointer transport is retained; the field expressions name the established unit and table layouts. */
    extern f32 iGpffff8170;
    extern f32 fGpffff80cc;
    s32 result;
    s32 checkedSkill;
    s32 skill;
    s32 offset;
    u16 targetFlags;
    u8 active;
    u16 enemyElement;
    s32 element;
    s32 elementMask;
    s32 counterRate;
    s32 rawElement;
    s32 resistance;
    f32 defenseRate;
    f32 attackRate;
    f32 evadeMultiplier;
    f32 hitMultiplier;
    s32 equipmentBonus;
    s16 evadeCause;
    s32 baseRate;
    P4GameActionFrame actionFrame;
    s32 hasActionFrame;

    checkedSkill = skillId & 0xFFFF;
    if (!p4_game_skill_active(checkedSkill)) {
        kAssertFail(D_00635938, 0x105E);
    }
    targetFlags = *(u16 *)target;
    if (((*(u16 *)source & 4) != (targetFlags & 4)) &&
        (s8)(active = (((DatCalcUnitState *)source)->buffFlags & 0x100000) != 0) > 0) {
        ((DatCalcUnitState *)target)->message = 0xB2;
        return 4;
    }
    skill = (u32)skillId % 0x10000U;
    offset = skill * 0x28;
    switch (iGpffffb3b8[offset + 0x24]) {
    case 1:
        if ((u8)((((DatUnit *)target)->bad & 4) != 0)) {
            result = 1;
            goto finish;
        } else {
            return 4;
        }
    case 7:
        return (targetFlags & 0x1000) ? 4 : 1;
    case 8:
        return (targetFlags & 0x1000) ? 1 : 4;
    case 9:
        if (datCalcIsGuarding((u16 *)target) != 0) {
            return 0x100;
        }
        break;
    case 12:
        if (p4_game_elements_native_enabled()) {
            u32 word;
            s32 status = p4_game_elements_native_profile_affinity(target, 6,
                P4_CHANNEL_INSTANT_KO, P4_ACTION_COMPONENT_INSTANT_KO, &word);
            if (status) p4_game_elements_native_calculation_refuse(status);
            if (word & P4_AFF_WEAK) return 1;
            status = p4_game_elements_native_profile_affinity(target, 7,
                P4_CHANNEL_INSTANT_KO, P4_ACTION_COMPONENT_INSTANT_KO, &word);
            if (status) p4_game_elements_native_calculation_refuse(status);
            return (word & P4_AFF_WEAK) ? 1 : 4;
        }
        if (datCalcGetElementAffinity(target, 6) & 0x08000000) {
            return 1;
        }
        return (datCalcGetElementAffinity(target, 7) & 0x08000000) ? 1 : 4;
    case 14:
        if ((u8)(((u8 *)&((DatUnit *)target)->bad)[0] != 0)) {
            result = 1;
            goto finish;
        } else {
            return 4;
        }
    }
    hasActionFrame = p4_game_elements_native_enabled();
    if (hasActionFrame) {
        p4_game_elements_native_primary_frame(source, skill, (u32)options, &actionFrame);
    } else if (source != NULL && p4_game_skill_active(checkedSkill) &&
        (iGpffffb3b8[offset] & 2)) {
        if (*(u16 *)source & 4) {
            enemyElement = ((DatEnemyUnitInit *)byteAddressAdd(((DatUnit *)source)->id * 0x3C, iGpffffb3c4))->weaponElement;
            rawElement = (s16)enemyElement;
        } else {
            rawElement = (s16)datItemElement(
                (s16)(u16)datGetEquippedItemByUnit((s16)((DatUnit *)(source))->id, 0));
        }
    } else {
        rawElement = p4_game_skill_element(skill, ((s8 *)iGpffffb3b4)[skill * 2]);
    }
    if (hasActionFrame) {
        /* Only the original 0..4 family comparisons below need projection;
         * full leaf and channel remain in actionFrame for affinity. */
        element = !actionFrame.element.valid ? -1 :
            p4_game_action_element_is_physical(&actionFrame.element) ? 0 :
            actionFrame.element.family >= 1u && actionFrame.element.family <= 4u ?
                (s32)actionFrame.element.family : -1;
        elementMask = 0;
    } else {
        element = (s16)rawElement;
        if (element >= 0x13) {
            kAssertFail(D_00635938, 0xE47);
        }
        elementMask = 1 << (element + 1);
    }
    if (hasActionFrame ? p4_game_action_element_has_affinity(&actionFrame.element) :
        !(elementMask & 0xE0001)) {
        if (hasActionFrame ? p4_game_action_element_is_physical(&actionFrame.element) : (elementMask & 2)) {
            if ((s8)(active = (((DatCalcUnitState *)target)->buffFlags & 0x10000) != 0) > 0) {
                ((DatCalcUnitState *)target)->consumedFlags |= 0x10000;
                return 0x200;
            }
            if (!(options & 2) && !(iGpffffb3b8[offset] & 0x40)) {
                counterRate = 0;
                if (datCalcHasFixedSkill(target, 0x1F8) ||
                    (datCalcCountEquipmentProperty(target, 0x4A) & 0xFFFF)) {
                    counterRate = 20;
                    evadeCause = 0x11F8;
                } else {
                    if (datCalcHasFixedSkill(target, 0x1F7) ||
                        (datCalcCountEquipmentProperty(target, 0x49) & 0xFFFF)) {
                        counterRate = 15;
                        evadeCause = 0x11F7;
                    } else {
                        if (datCalcHasFixedSkill(target, 0x1F6) ||
                            (datCalcCountEquipmentProperty(target, 0x48) & 0xFFFF)) {
                            counterRate = 10;
                            evadeCause = 0x11F6;
                        }
                    }
                }
                counterRate = (s16)counterRate;
                if (counterRate != 0 &&
                    (s32)(RpRandom() % 100U) < counterRate) {
                    ((DatCalcUnitState *)target)->message = evadeCause;
                    return 0x200;
                }
            }
        } else if ((hasActionFrame ? actionFrame.element.family != 5u : !(elementMask & 0x40)) &&
            (s8)(active = (((DatCalcUnitState *)target)->buffFlags & 0x20000) != 0) > 0) {
            ((DatCalcUnitState *)target)->consumedFlags |= 0x20000;
            return 0x200;
        }
        resistance = hasActionFrame ? (s32)p4_game_elements_native_element_affinity(target, &actionFrame) :
            datCalcGetElementAffinity(target, rawElement);
        if (resistance & 0x02000000) {
            return 0x200;
        }
        if (resistance & 0x04000000) {
            return 0x400;
        }
        if (resistance & 0x01000000) {
            return 0x100;
        }
    }
    if (iGpffffb3b8[offset + 0xE] == 0) {
        return 4;
    }
    if ((u8)((((DatUnit *)target)->bad & 0x100000) != 0)) {
        return 1;
    }
    if (datCalcIsGuarding((u16 *)target) != 0) {
        return 1;
    }
    if (iGpffffb3b8[offset + 0xE] >= 100) {
        return 1;
    }
    if (*(u16 *)target & 0x40) {
        return 1;
    }
    switch (targetRecord->bad & 0xFFFFF) {
    case 1:
        return 1;
    }
    attackRate = datCalcBuffMultiplier(source, 4, skillId);
    defenseRate = datCalcBuffMultiplier(target, 2, skillId);
    evadeMultiplier = 1.0f;
    hitMultiplier = 1.0f;
    equipmentBonus = 0;
    evadeCause = 0;
    if (!(options & 2)) {
        switch (element) {
        case 0:
            if (datCalcHasFixedSkill(target, 0x1DD)) {
                evadeMultiplier *= iGpffff8170;
                evadeCause = 0x11DD;
            } else if (datCalcHasFixedSkill(target, 0x1DC)) {
                evadeMultiplier *= fGpffff80cc;
                evadeCause = 0x11DC;
            }
            break;
        case 1:
            if (datCalcHasFixedSkill(target, 0x1DF)) {
                evadeMultiplier *= iGpffff8170;
                evadeCause = 0x11DF;
            } else if (datCalcHasFixedSkill(target, 0x1DE)) {
                evadeMultiplier *= fGpffff80cc;
                evadeCause = 0x11DE;
            }
            break;
        case 2:
            if (datCalcHasFixedSkill(target, 0x1E1)) {
                evadeMultiplier *= iGpffff8170;
                evadeCause = 0x11E1;
            } else if (datCalcHasFixedSkill(target, 0x1E0)) {
                evadeMultiplier *= fGpffff80cc;
                evadeCause = 0x11E0;
            }
            break;
        case 4:
            if (datCalcHasFixedSkill(target, 0x1E3)) {
                evadeMultiplier *= iGpffff8170;
                evadeCause = 0x11E3;
            } else if (datCalcHasFixedSkill(target, 0x1E2)) {
                evadeMultiplier *= fGpffff80cc;
                evadeCause = 0x11E2;
            }
            break;
        case 3:
            if (datCalcHasFixedSkill(target, 0x1E5)) {
                evadeMultiplier *= iGpffff8170;
                evadeCause = 0x11E5;
            } else if (datCalcHasFixedSkill(target, 0x1E4)) {
                evadeMultiplier *= fGpffff80cc;
                evadeCause = 0x11E4;
            }
            break;
        }
        if (evadeMultiplier == 1.0f && (hasActionFrame ? (element >= 1 && element <= 4) : (elementMask & 0x3C))) {
            if (datCalcHasFixedSkill(target, 0x1E6)) {
                evadeMultiplier *= fGpffff80cc;
                evadeCause = 0x11E6;
            }
        }
        if (datCalcHasFixedSkill(target, 0x209)) {
            return 1;
        }
        if (datCalcHasFixedSkill(target, 0x208)) {
            evadeMultiplier *= 0.5f;
        }
    }
    baseRate = hasActionFrame ?
        p4_game_elements_calculation_run(DAT_SCRIPT_CALC_HIT_RATE, source, target, skillId, 0, &actionFrame, NULL) :
        datScriptRunCalc(DAT_SCRIPT_CALC_HIT_RATE, (u16 *)source, (u16 *)target, skillId, 0);
    if (!(*(u16 *)target & 4)) {
        switch (element) {
        case 0:
            if (datCalcHasEquipmentProperty(target, 0x74)) {
                equipmentBonus += 10;
            }
            if (datCalcHasEquipmentProperty(target, 0x75)) {
                equipmentBonus += 15;
            }
            if (datCalcHasEquipmentProperty(target, 0x76)) {
                equipmentBonus += 20;
            }
            break;
        }
        switch (element) {
        case 1: case 2: case 3: case 4:
            if (datCalcHasEquipmentProperty(target, 0x83)) {
                equipmentBonus += 10;
            }
            if (datCalcHasEquipmentProperty(target, 0x84)) {
                equipmentBonus += 15;
            }
            if (datCalcHasEquipmentProperty(target, 0x85)) {
                equipmentBonus += 20;
            }
            break;
        }
        switch (element) {
        case 1:
            if (datCalcHasEquipmentProperty(target, 0x77)) {
                equipmentBonus += 10;
            }
            if (datCalcHasEquipmentProperty(target, 0x78)) {
                equipmentBonus += 15;
            }
            if (datCalcHasEquipmentProperty(target, 0x79)) {
                equipmentBonus += 20;
            }
            break;
        case 2:
            if (datCalcHasEquipmentProperty(target, 0x7A)) {
                equipmentBonus += 10;
            }
            if (datCalcHasEquipmentProperty(target, 0x7B)) {
                equipmentBonus += 15;
            }
            if (datCalcHasEquipmentProperty(target, 0x7C)) {
                equipmentBonus += 20;
            }
            break;
        case 3:
            if (datCalcHasEquipmentProperty(target, 0x80)) {
                equipmentBonus += 10;
            }
            if (datCalcHasEquipmentProperty(target, 0x81)) {
                equipmentBonus += 15;
            }
            if (datCalcHasEquipmentProperty(target, 0x82)) {
                equipmentBonus += 20;
            }
            break;
        case 4:
            if (datCalcHasEquipmentProperty(target, 0x7D)) {
                equipmentBonus += 10;
            }
            if (datCalcHasEquipmentProperty(target, 0x7E)) {
                equipmentBonus += 15;
            }
            if (datCalcHasEquipmentProperty(target, 0x7F)) {
                equipmentBonus += 20;
            }
            break;
        }
    }
    baseRate = (s32)(evadeMultiplier * (hitMultiplier *
        (defenseRate * ((f32)baseRate * attackRate)))) - equipmentBonus;
    if (baseRate > 99) {
        baseRate = 99;
    } else if (baseRate < 50) {
        baseRate = 50;
    }
    if ((s32)(RpRandom() % 100U) >= baseRate) {
        ((DatCalcUnitState *)target)->message = evadeCause;
        return 4;
    }
    return 1;
finish:
    return result;
}
