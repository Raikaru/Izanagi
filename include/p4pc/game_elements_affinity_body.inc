/* Shared by the translated retail entry and full-width native queries.
 * The body is the canonical datCalcGetAffinity precedence from decomp363e;
 * base selection is deliberately outside it. Include once, after that unit
 * defines its actual skill/equipment queries. No second combat resolver. */
uint32_t p4_game_elements_native_affinity_apply(void *unit_pointer, int32_t element, uint32_t affinity, uint32_t additional_passives)
{
    u8 *unit = unit_pointer;
    s32 found;
    u32 passiveFlags = additional_passives;
    s32 guarding = 0;
    s32 clearResistance = 0;
    s32 grantResistance = 0;
    if (datCalcIsGuarding((u16 *)unit) != 0) {
        guarding = 1;
    }
    switch (element) {
    case 0:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1B8);
        if (found != 0) {
            passiveFlags |= 0x10000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1B9);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1BA);
        if (found != 0) {
            passiveFlags |= 0x2000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1BB);
        if (found != 0) {
            passiveFlags |= 0x4000000;
        }
        break;
    }
    switch (element) {
    case 1:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1BC);
        if (found != 0) {
            passiveFlags |= 0x10000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1BD);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1BE);
        if (found != 0) {
            passiveFlags |= 0x2000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1BF);
        if (found != 0) {
            passiveFlags |= 0x4000000;
        }
        if (datCalcGetBuffLevel((DatUnit *)unit, 8) > 0) {
            clearResistance = 1;
        }
        if (datCalcGetBuffLevel((DatUnit *)unit, 0xC) > 0) {
            grantResistance = 1;
        }
        break;
    case 2:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1C0);
        if (found != 0) {
            passiveFlags |= 0x10000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1C1);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1C2);
        if (found != 0) {
            passiveFlags |= 0x2000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1C3);
        if (found != 0) {
            passiveFlags |= 0x4000000;
        }
        if (datCalcGetBuffLevel((DatUnit *)unit, 9) > 0) {
            clearResistance = 1;
        }
        if (datCalcGetBuffLevel((DatUnit *)unit, 0xD) > 0) {
            grantResistance = 1;
        }
        break;
    case 3:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1C4);
        if (found != 0) {
            passiveFlags |= 0x10000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1C5);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1C6);
        if (found != 0) {
            passiveFlags |= 0x2000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1C7);
        if (found != 0) {
            passiveFlags |= 0x4000000;
        }
        if (datCalcGetBuffLevel((DatUnit *)unit, 11) > 0) {
            clearResistance = 1;
        }
        if (datCalcGetBuffLevel((DatUnit *)unit, 0xE) > 0) {
            grantResistance = 1;
        }
        break;
    case 4:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1C8);
        if (found != 0) {
            passiveFlags |= 0x10000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1C9);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1CA);
        if (found != 0) {
            passiveFlags |= 0x2000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1CB);
        if (found != 0) {
            passiveFlags |= 0x4000000;
        }
        if (datCalcGetBuffLevel((DatUnit *)unit, 10) > 0) {
            clearResistance = 1;
        }
        if (datCalcGetBuffLevel((DatUnit *)unit, 0xF) > 0) {
            grantResistance = 1;
        }
        break;
    case 6:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1CC);
        if (found != 0) {
            passiveFlags |= 0x10000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1CD);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1CE);
        if (found != 0) {
            passiveFlags |= 0x2000000;
        }
        break;
    case 7:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1CF);
        if (found != 0) {
            passiveFlags |= 0x10000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1D0);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1D1);
        if (found != 0) {
            passiveFlags |= 0x2000000;
        }
        break;
    case 9:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x225);
        if (found != 0) {
            goto acc_9_b;
        }
        found = datCalcHasEquipmentProperty(unit, 0x4C);
        if (found == 0) {
            goto skp_9_b;
        }
acc_9_b:
        passiveFlags |= 0x10000000;
skp_9_b:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1D7);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        break;
    case 8:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x226);
        if (found != 0) {
            goto acc_8_b;
        }
        found = datCalcHasEquipmentProperty(unit, 0x4B);
        if (found == 0) {
            goto skp_8_b;
        }
acc_8_b:
        passiveFlags |= 0x10000000;
skp_8_b:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1D2);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        break;
    case 10:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x227);
        if (found != 0) {
            goto acc_10_b;
        }
        found = datCalcHasEquipmentProperty(unit, 0x4D);
        if (found == 0) {
            goto skp_10_b;
        }
acc_10_b:
        passiveFlags |= 0x10000000;
skp_10_b:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1D5);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        break;
    case 13:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x228);
        if (found != 0) {
            goto acc_13_b;
        }
        found = datCalcHasEquipmentProperty(unit, 0x50);
        if (found == 0) {
            goto skp_13_b;
        }
acc_13_b:
        passiveFlags |= 0x10000000;
skp_13_b:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1D3);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        break;
    case 15:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x22C);
        if (found != 0) {
            goto acc_15_b;
        }
        found = datCalcHasEquipmentProperty(unit, 0x52);
        if (found == 0) {
            goto skp_15_b;
        }
acc_15_b:
        passiveFlags |= 0x10000000;
skp_15_b:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1D4);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        break;
    case 11:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x22A);
        if (found != 0) {
            goto acc_11_b;
        }
        found = datCalcHasEquipmentProperty(unit, 0x4E);
        if (found == 0) {
            goto skp_11_b;
        }
acc_11_b:
        passiveFlags |= 0x10000000;
skp_11_b:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1D6);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        break;
    case 14:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x229);
        if (found != 0) {
            goto acc_14_b;
        }
        found = datCalcHasEquipmentProperty(unit, 0x51);
        if (found == 0) {
            goto skp_14_b;
        }
acc_14_b:
        passiveFlags |= 0x10000000;
skp_14_b:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1D9);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        break;
    case 12:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x22B);
        if (found != 0) {
            goto acc_12_b;
        }
        found = datCalcHasEquipmentProperty(unit, 0x4F);
        if (found == 0) {
            goto skp_12_b;
        }
acc_12_b:
        passiveFlags |= 0x10000000;
skp_12_b:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1D8);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        break;
    case 0:
        break;
    case 5:
        break;
    }
    switch (element) {
    case 8:
    case 10:
    case 0xF:
    case 0xB:
    case 0xE:
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1DA);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
        break;
    }
    if (element != 5) {
        found = datCalcHasRequestedSkill((DatUnit *)unit, 0x1DB);
        if (found != 0) {
            passiveFlags |= 0x1000000;
        }
    }
    if (guarding == 1 && (affinity & 0x8000000) != 0) {
        affinity = (affinity & 0xF7FF0000) | 100;
    }
    if (passiveFlags != 0) {
        if ((passiveFlags & 0x10000000) != 0 && (affinity & 0x7000000) == 0) {
            affinity = (((affinity & 0xF7FFFFFF) | 0x10000000) & 0xFFFF0000) | 0x32;
        }
        if ((passiveFlags & 0x1000000) != 0 && (affinity & 0x6000000) == 0) {
            affinity = (((affinity & 0xE7FFFFFF) | 0x1000000) & 0xFFFF0000) | 0x64;
        }
        if ((passiveFlags & 0x2000000) != 0 && (affinity & 0x4000000) == 0) {
            affinity = (((affinity & 0xE6FFFFFF) | 0x2000000) & 0xFFFF0000) | 0x64;
        }
        if ((passiveFlags & 0x4000000) != 0) {
            affinity = (((affinity & 0xE4FFFFFF) | 0x4000000) & 0xFFFF0000) | 0x64;
        }
    }
    if (clearResistance != 0 && (affinity & 0x17000000) != 0) {
        affinity = (affinity & 0xE8FF0000) | 100;
    }
    if (grantResistance != 0 && ((affinity & 0x8000000) != 0 || (affinity & 0xFF000000) == 0)) {
        affinity = (((affinity & 0xFFFFFF) | 0x10000000) & 0xFFFF0000) | 0x32;
    }
    return (s32)affinity;
}
