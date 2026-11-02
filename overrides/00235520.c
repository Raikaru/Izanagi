/* 00235520: one canonical original amount equation for both native ABIs.
 * Owning frame/query are caller-local; unchanged mode never creates a frame. */
#include "p4pc/game_elements_calc_native.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_enemies_native.h"
/* Later0023df70 is also called by the following status routine in this unit. */
extern s32 datCalcUsesWeaponProperties(s32 skillId);
static s32 p4_calc_amount(u32 skillId, u8 *source, u8 *target, u16 hitCount, u16 outcome, u16 hitKind, s32 options, u8 channel, const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query, P4GameActionFrame *p4_storage);
s32 datCalcSkillAmount(skillId, source, target, hitCount, outcome, hitKind, options, channel)
u16 skillId;
u8 *source;
u8 *target;
u16 hitCount;
u16 outcome;
u16 hitKind;
s32 options;
u8 channel;
{
    if (p4_game_elements_native_enabled()) {
        P4GameActionFrame storage;
        return p4_calc_amount(skillId, source, target, hitCount, outcome, hitKind, options, channel, NULL, NULL, &storage);
    }
    return p4_calc_amount(skillId, source, target, hitCount, outcome, hitKind, options, channel, NULL, NULL, NULL);
}
static const P4GameActionFrame *p4_amount_frame(u32 skill, u8 *source, s32 options,
    const P4GameActionFrame **current, P4GameActionFrame *storage)
{
    if (!*current && storage) {
        p4_game_elements_native_primary_frame(source, skill, (u32)options, storage);
        *current = storage;
    }
    return *current;
}
static s32 p4_calc_amount(u32 skillId, u8 *source, u8 *target, u16 hitCount,
    u16 outcome, u16 hitKind, s32 options, u8 channel,
    const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query, P4GameActionFrame *p4_storage)
{
    DatUnit *sourceRecord = (DatUnit *)source;
    DatUnit *targetRecord = (DatUnit *)target;
    /* Byte-pointer transport is retained; the field expressions name the established unit and table layouts. */
    extern s32 datCalcElementBit(s16 element);
    extern f32 iGpffff8170;
    extern f32 fGpffff82c0;
    extern f32 fGpffff82c4;
    extern f32 fGpffff82c8;
    extern f32 fGpffff80bc;
    extern f32 fGpffff82cc;
    extern f32 fGpffff811c;
    extern f32 fGpffff809c;
    s32 checkedSkill;
    f32 attack;
    f32 defense;
    f32 status;
    f32 passive;
    f32 elemental;
    u16 current;
    u16 maximum;
    s32 fixed;
    s16 power;
    u16 effect;
    s16 bonus;
    s32 rawElement;
    s32 element;
    s32 elementMask;
    s32 offset;
    s32 work;
    s32 healing;
    s32 damage;
    u32 targetStatus;
    u8 *record;

    checkedSkill = (u16)skillId;
    if (checkedSkill < 0 || !p4_game_skill_registered(checkedSkill)) {
        kAssertFail(D_00635938, 0x711);
    }
    if (!p4_game_skill_active(checkedSkill)) {
        kAssertFail(D_00635938, 0x712);
    }
    attack = 1.0f;
    defense = 1.0f;
    status = 1.0f;
    passive = 1.0f;
    healing = 0;
    damage = 0;
    current = 0;
    maximum = 0;
    fixed = 1;
    if (p4_storage && !p4_frame) p4_game_elements_native_skill_use(skillId);
    switch ((u32)channel) {
    case 1:
        record = byteAddressAdd(((u32)skillId % 0x10000U) * 0x28, iGpffffb3b8);
        power = ((DatSkillEntry *)record)->power;
        effect = ((DatSkillEntry *)record)->effectKind;
        bonus = 0;
        current = ((DatUnit *)target)->hp;
        maximum = datCalcMaxHp((DatUnit *)target);
        break;
    case 2:
        record = byteAddressAdd(((u32)skillId % 0x10000U) * 0x28, iGpffffb3b8);
        power = ((DatSkillEntry *)record)->spPower;
        effect = ((DatSkillEntry *)record)->spEffectKind;
        bonus = 0;
        current = ((DatUnit *)target)->sp;
        maximum = datCalcMaxSp((DatUnit *)target);
        break;
    default:
        kAssertFail(D_00635938, 0x731);
        break;
    }
    switch (effect) {
    case 0:
        break;
    case 5:
        healing = power + bonus;
        break;
    case 15:
        healing = power;
        if (healing == 0) {
            kAssertFail(D_00635938, 0x17);
        }
        healing = operandOrderAdd((s32)(RpRandom() % healing), (s16)bonus);
        break;
    case 9:
        healing = operandOrderAdd((s32)(current * power / 100), (s16)bonus);
        if (healing <= 0) {
            healing = 1;
        }
        break;
    case 11:
        healing = operandOrderAdd((s32)(maximum * power / 100), (s16)bonus);
        if (healing <= 0) {
            healing = 1;
        }
        break;
    case 2:
/* Read the damage class from the skill table. */
        switch ((iGpffffb3b8[(u32)(u16)skillId * sizeof(DatSkillEntry) + MEMBER_OFFSET(DatSkillEntry, damageClass)])) {
        case 1:
            /* Kind 1 is the PHYSICAL_DAMAGE handler, reused here for healing (dual use, see DatScriptCalcKind). */
            healing = p4_game_elements_calculation_run(DAT_SCRIPT_CALC_PHYSICAL_DAMAGE, (u16 *)source, (u16 *)target, skillId, channel, p4_amount_frame(skillId, source, options, &p4_frame, p4_storage), p4_query);
            break;
        case 2:
            healing = p4_game_elements_calculation_run(DAT_SCRIPT_CALC_TERMS_13_AND_1A, (u16 *)source, (u16 *)target, skillId, channel, p4_frame, p4_query);
            break;
        default:
            kAssertFail(D_00635938, 0x75E);
            break;
        }
        healing += bonus;
        if (healing <= 0) {
            healing = 1;
        }
        fixed = 0;
        break;
    case 7:
        healing = 50;
        healing += bonus;
        if (healing <= 0) {
            healing = 1;
        }
        fixed = 0;
        break;
    case 4:
    case 13:
        damage = power + bonus;
        if (effect == 13 && current < damage) {
            damage = current;
        }
        break;
    case 8:
        damage = operandOrderAdd((s32)(current * power / 100), (s16)bonus);
        if (damage <= 0 && power > 0) {
            damage = 1;
        }
        break;
    case 10:
        damage = operandOrderAdd((s32)(maximum * power / 100), (s16)bonus);
        if (damage <= 0 && power > 0) {
            damage = 1;
        }
        break;
    case 1:
    case 12:
    case 14:
        switch ((iGpffffb3b8[(u32)(u16)skillId * sizeof(DatSkillEntry) + MEMBER_OFFSET(DatSkillEntry, damageClass)])) {
        case 1:
            damage = p4_game_elements_calculation_run(DAT_SCRIPT_CALC_PHYSICAL_DAMAGE, (u16 *)source, (u16 *)target, skillId, channel, p4_amount_frame(skillId, source, options, &p4_frame, p4_storage), p4_query);
            break;
        case 2:
            damage = p4_game_elements_calculation_run(DAT_SCRIPT_CALC_MAGIC_DAMAGE, (u16 *)source, (u16 *)target, skillId, channel, p4_amount_frame(skillId, source, options, &p4_frame, p4_storage), p4_query);
            break;
        default:
            kAssertFail(D_00635938, 0x7A7);
            break;
        }
        damage += bonus;
        switch (effect) {
        case 12:
        case 13:
            if (current < damage) {
                damage = current;
            }
            break;
        }
        if (effect == 14 && hitCount > 1) {
            damage /= hitCount;
        }
        if (damage <= 0) {
            damage = 1;
        }
        fixed = 0;
        break;
    case 6:
        damage = 50;
        damage += bonus;
        if (damage <= 0) {
            damage = 1;
        }
        fixed = 0;
        break;
    case 3:
        if (power + bonus < current) {
            damage = current - (power + bonus);
        }
        break;
    case 16:
        damage = operandOrderAdd((s32)p4_game_elements_calculation_run(DAT_SCRIPT_CALC_PHYSICAL_DAMAGE_SCALED, (u16 *)source, (u16 *)target, skillId, channel, p4_amount_frame(skillId, source, options, &p4_frame, p4_storage), p4_query), (s16)bonus);
        if (damage <= 0) {
            damage = 1;
        }
        fixed = 0;
        break;
    default:
        kAssertFail(D_00635938, 0x7DE);
        break;
    }
    if (fixed == 0 && damage > 0 && !(options & 1)) {
        p4_amount_frame(skillId, source, options, &p4_frame, p4_storage);
        if (p4_frame) {
            const P4GameActionElement *resolved = &p4_frame->element;
            const int physical = p4_game_action_element_is_physical(resolved);
            /* Coverage selector only; never a semantic leaf or packed mask.
             * Existing physical and Fire..Wind effects retain their one body. */
            rawElement = !resolved->valid ? -1 : physical ? 0 :
                resolved->family >= 1u && resolved->family <= 4u ? (s32)resolved->family : 5;
            elementMask = physical ? 2 : 0;
        } else {
            rawElement = (s16)datCalcSkillElement((DatUnit *)source, skillId);
            elementMask = datCalcElementBit(rawElement);
        }
        offset = ((u32)skillId % 0x10000U) * 0x28;
        switch (iGpffffb3b8[offset + 2]) {
        case 1:
            if (effect != 16) {
                attack = datCalcBuffMultiplier(source, 0, skillId);
            }
            defense = datCalcBuffMultiplier(target, 3, skillId);
            if (effect != 16) {
                ((DatCalcUnitState *)source)->consumedFlags |= 0x40000;
            }
            break;
        case 2:
            if (effect != 16) {
                attack = datCalcBuffMultiplier(source, 1, skillId);
            }
            defense = datCalcBuffMultiplier(target, 3, skillId);
            if (effect != 16) {
                ((DatCalcUnitState *)source)->consumedFlags |= 0x80000;
            }
            break;
        default:
            kAssertFail(D_00635938, 0x803);
            break;
        }
        element = (s16)rawElement;
        switch (element) {
        case 1:
            elemental = 1.0f;
            if ((datCalcAccessoryProperty((DatUnit *)source) & 0xFFFF) == 0xBD) {
                elemental *= 1.0f + datCalcAccessoryFraction((DatUnit *)source);
            }
            if (datCalcHasRequestedSkill((DatUnit *)source, 0x1E7) || (datCalcCountEquipmentProperty(source, 0x44) & 0xFFFF)) {
                elemental *= 1.25f;
            }
            if (datCalcHasRequestedSkill((DatUnit *)source, 0x1E8)) {
                elemental *= 1.5f;
            }
            if (elemental > 1.875f) {
                elemental = 1.875f;
            }
            passive *= elemental;
            if (datGetFlag(0x1435)) {
                if (datGetFlag(0x15C5)) {
                    passive *= 2.0f;
                } else {
                    passive *= fGpffff82c4;
                }
            }
            break;
        case 2:
            elemental = 1.0f;
            if ((datCalcAccessoryProperty((DatUnit *)source) & 0xFFFF) == 0xBE) {
                elemental *= 1.0f + datCalcAccessoryFraction((DatUnit *)source);
            }
            if (datCalcHasRequestedSkill((DatUnit *)source, 0x1E9) || (datCalcCountEquipmentProperty(source, 0x45) & 0xFFFF)) {
                elemental *= 1.25f;
            }
            if (datCalcHasRequestedSkill((DatUnit *)source, 0x1EA)) {
                elemental *= 1.5f;
            }
            if (elemental > 1.875f) {
                elemental = 1.875f;
            }
            passive *= elemental;
            if (datGetFlag(0x1435)) {
                if (datGetFlag(0x15C6)) {
                    passive *= 2.0f;
                } else {
                    passive *= fGpffff82c4;
                }
            }
            break;
        case 3:
            elemental = 1.0f;
            if ((datCalcAccessoryProperty((DatUnit *)source) & 0xFFFF) == 0xC0) {
                elemental *= 1.0f + datCalcAccessoryFraction((DatUnit *)source);
            }
            if (datCalcHasRequestedSkill((DatUnit *)source, 0x1EB) || (datCalcCountEquipmentProperty(source, 0x46) & 0xFFFF)) {
                elemental *= 1.25f;
            }
            if (datCalcHasRequestedSkill((DatUnit *)source, 0x1EC)) {
                elemental *= 1.5f;
            }
            if (elemental > 1.875f) {
                elemental = 1.875f;
            }
            passive *= elemental;
            if (datGetFlag(0x1435)) {
                if (datGetFlag(0x15C8)) {
                    passive *= 2.0f;
                } else {
                    passive *= fGpffff82c4;
                }
            }
            break;
        case 4:
            elemental = 1.0f;
            if ((datCalcAccessoryProperty((DatUnit *)source) & 0xFFFF) == 0xBF) {
                elemental *= 1.0f + datCalcAccessoryFraction((DatUnit *)source);
            }
            if (datCalcHasRequestedSkill((DatUnit *)source, 0x1ED) || (datCalcCountEquipmentProperty(source, 0x47) & 0xFFFF)) {
                elemental *= 1.25f;
            }
            if (datCalcHasRequestedSkill((DatUnit *)source, 0x1EE)) {
                elemental *= 1.5f;
            }
            if (elemental > 1.875f) {
                elemental = 1.875f;
            }
            passive *= elemental;
            if (datGetFlag(0x1435)) {
                if (datGetFlag(0x15C7)) {
                    passive *= 2.0f;
                } else {
                    passive *= fGpffff82c4;
                }
            }
            break;
        }
        if (datCalcHasRequestedSkill((DatUnit *)target, 0x209)) {
            passive *= 0.5f;
        }
        if (datGetFlag(0x1011)) {
            if (*(u16 *)source & 4) {
                attack *= 1.5f;
            }
        } else if (datGetFlag(0x1010) && (*(u16 *)source & 4)) {
            attack *= iGpffff8170;
        }
        if (hitKind == 2) {
            if (!(*(u16 *)source & 4)) {
                attack *= fGpffff82c8;
            } else {
                attack *= fGpffff80bc;
            }
        }
        if (datCalcIsGuarding((u16 *)target)) {
            defense *= fGpffff82cc;
        }
        targetStatus = ((DatUnit *)target)->bad;
        if ((u8)((targetStatus & 0x100000) != 0)) {
            status *= 1.25f;
        }
        if ((elementMask & 2) && (u8)((((DatUnit *)source)->bad & 0x20) != 0)) {
            status *= 0.5f;
        }
        if ((u8)((((DatUnit *)source)->bad & 2) != 0)) {
            status *= 2.0f;
        }
        if ((u8)((targetStatus & 0x40) != 0)) {
            status *= 1.5f;
        } else if ((u8)((targetStatus & 2) != 0)) {
            status *= 2.0f;
        }
        if (*(u16 *)source & 4) {
            if (((DatUnit *)source)->id >= p4_game_enemies_native_limit()) {
                kAssertFail(D_00635938, 0x8C0);
            }
            if (((DatEnemyUnitInit *)(iGpffffb3c4 + sourceRecord->id * 0x3C))->flags & 0x20) {
                attack *= 0.75f;
            }
        }
        switch (iGpffffb3b8[offset + 0x24]) {
        case 3:
            if ((u8)((targetRecord->bad & 0x100000) != 0)) {
                attack *= 2.0f;
            }
            break;
        }
        if (!(*(u16 *)target & 4)) {
            switch (element) {
            case 0:
                if (datCalcHasEquipmentProperty(target, 0x5F)) {
                    attack *= fGpffff82c0;
                }
                if (datCalcHasEquipmentProperty(target, 0x60)) {
                    attack *= fGpffff811c;
                }
                if (datCalcHasEquipmentProperty(target, 0x61)) {
                    attack *= fGpffff809c;
                }
                break;
            }
            switch (element) {
            case 1:
            case 2:
            case 3:
            case 4:
                if (datCalcHasEquipmentProperty(target, 0x6E)) {
                    attack *= fGpffff82c0;
                }
                if (datCalcHasEquipmentProperty(target, 0x6F)) {
                    attack *= fGpffff811c;
                }
                if (datCalcHasEquipmentProperty(target, 0x70)) {
                    attack *= fGpffff809c;
                }
                break;
            }
            switch (element) {
            case 1:
                if ((datCalcAccessoryProperty((DatUnit *)target) & 0xFFFF) == 0xC1) {
                    attack *= 1.0f - datCalcAccessoryFraction((DatUnit *)target);
                }
                if (datCalcHasEquipmentProperty(target, 0x62)) {
                    attack *= fGpffff82c0;
                }
                if (datCalcHasEquipmentProperty(target, 0x63)) {
                    attack *= fGpffff811c;
                }
                if (datCalcHasEquipmentProperty(target, 0x64)) {
                    attack *= fGpffff809c;
                }
                break;
            case 2:
                if ((datCalcAccessoryProperty((DatUnit *)target) & 0xFFFF) == 0xC2) {
                    attack *= 1.0f - datCalcAccessoryFraction((DatUnit *)target);
                }
                if (datCalcHasEquipmentProperty(target, 0x65)) {
                    attack *= fGpffff82c0;
                }
                if (datCalcHasEquipmentProperty(target, 0x66)) {
                    attack *= fGpffff811c;
                }
                if (datCalcHasEquipmentProperty(target, 0x67)) {
                    attack *= fGpffff809c;
                }
                break;
            case 3:
                if ((datCalcAccessoryProperty((DatUnit *)target) & 0xFFFF) == 0xC4) {
                    attack *= 1.0f - datCalcAccessoryFraction((DatUnit *)target);
                }
                if (datCalcHasEquipmentProperty(target, 0x6B)) {
                    attack *= fGpffff82c0;
                }
                if (datCalcHasEquipmentProperty(target, 0x6C)) {
                    attack *= fGpffff811c;
                }
                if (datCalcHasEquipmentProperty(target, 0x6D)) {
                    attack *= fGpffff809c;
                }
                break;
            case 4:
                if ((datCalcAccessoryProperty((DatUnit *)target) & 0xFFFF) == 0xC3) {
                    attack *= 1.0f - datCalcAccessoryFraction((DatUnit *)target);
                }
                if (datCalcHasEquipmentProperty(target, 0x68)) {
                    attack *= fGpffff82c0;
                }
                if (datCalcHasEquipmentProperty(target, 0x69)) {
                    attack *= fGpffff811c;
                }
                if (datCalcHasEquipmentProperty(target, 0x6A)) {
                    attack *= fGpffff809c;
                }
                break;
            }
        }
        work = (s32)(passive * (status * (defense * ((f32)damage * attack))));
        if (hitKind == 16) {
            damage = 500;
        } else {
            if (work > 31000) {
                work = 31000;
            } else if (work <= 0) {
                work = 1;
            }
            damage = (u32)(work * (RpRandom() % 11U + 95U)) / 100U;
        }
        if (damage <= 0) {
            damage = 1;
        }
    }
    if (fixed == 0 && healing > 0 && !(options & 1)) {
        if (datCalcHasRequestedSkill((DatUnit *)source, 0x20E)) {
            passive *= 1.5f;
        }
        work = (s32)(passive * (status * (defense * ((f32)healing * attack))));
        if (work > 31000) {
            work = 31000;
        } else if (work <= 0) {
            work = 1;
        }
        healing = (u32)(work * (RpRandom() % 11U + 95U)) / 100U;
        if (healing <= 0) {
            healing = 1;
        }
    }
    if (outcome == 0x400 && damage > 0) {
        healing = damage;
        damage = 0;
    }
    switch (outcome) {
    case 0x100:
    case 2:
    case 4:
        healing = 0;
        damage = 0;
        break;
    }
    if (damage > 0) {
        healing = -damage;
    }
    return healing;
}
int32_t p4_game_elements_native_amount(uint32_t skill_id, void *source, void *target,
    uint32_t hit_count, uint32_t outcome, uint32_t hit_kind, int32_t options, uint32_t channel,
    const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{
    if (!p4_game_elements_native_enabled() || !frame || frame->skill_id != skill_id || frame->element.valid > 1u ||
        !p4_game_skill_active(skill_id)) p4_game_elements_native_calculation_refuse(P4_TABLE_INVALID);
    return p4_calc_amount(skill_id, source, target, (u16)hit_count, (u16)outcome, (u16)hit_kind,
        options, (u8)channel, frame, query, NULL);
}
