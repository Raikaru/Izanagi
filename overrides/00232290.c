/* 00232290: datCalcMaxSp. The native maximum (table value, SP skills and
 * accessories, the party's 999 cap) passes the unit stage; the stage never
 * runs while no modifier is registered. The enemy-ID bound follows the
 * published banks, as config/mirror_fixes.txt bounds the retail function. */
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_unit_stage_native.h"
u16 datCalcMaxSp(DatUnit *unit)
{
    /* Enemy rows are 0x3C bytes; offsets +4/+6 hold HP/SP. */
    s32 baseSp;
    s32 bonusSp;
    s32 maximumSp;
    u16 tableSp;
    u8 *base;

    bonusSp = 0;
    baseSp = unit->id;
    if ((unit->flags & 4) != 0) {
        if ((baseSp & 0xFFFF) >= p4_game_enemies_native_limit()) {
            kAssertFail(D_00635938, 0xD9);
        }
        base = iGpffffb3c4;
        tableSp = ((DatEnemyUnitInit *)((u8 *)((u32)byteAddressAdd((u32)((baseSp & 0xFFFF) * 0x3C), (u8 *)((u32)base)))))->sp;
    } else {
        if ((baseSp & 0xFFFF) >= 0xB) {
            kAssertFail(D_00635938, 0xDC);
        }
        tableSp = (u16)datGetLevelTableSecondValue((s16)baseSp);
    }
    baseSp = tableSp & 0xFFFF;
    if (datCalcHasSkill(unit, 0x1F3) != 0) {
        bonusSp += (baseSp * 0xA) / 100;
    }
    if (datCalcHasSkill(unit, 0x1F4) != 0) {
        bonusSp += (baseSp * 0x14) / 100;
    }
    if (datCalcHasSkill(unit, 0x1F5) != 0) {
        bonusSp += (baseSp * 0x1E) / 100;
    }
    if (datCalcHasSkill(unit, 0x22E) != 0) {
        bonusSp += (baseSp * 0x32) / 100;
    }
    if (datCalcHasSkill(unit, 0x20C) != 0) {
        bonusSp -= (baseSp * 0x32) / 100;
    }
    if (datCalcHasSkill(unit, 0x20D) != 0) {
        bonusSp += baseSp;
    }
    bonusSp += (datCalcCountSlotsWithValue((u8 *)unit, 0x1F) & 0xFFFF) * 0xA;
    bonusSp += (datCalcCountSlotsWithValue((u8 *)unit, 0x20) & 0xFFFF) * 0x14;
    bonusSp += (datCalcCountSlotsWithValue((u8 *)unit, 0x21) & 0xFFFF) * 0x1E;
    bonusSp += (datCalcCountSlotsWithValue((u8 *)unit, 0x22) & 0xFFFF) * 0x28;
    bonusSp += (datCalcCountSlotsWithValue((u8 *)unit, 0x23) & 0xFFFF) * 0x32;
    maximumSp = baseSp + (bonusSp + ((datCalcCountSlotsWithValue((u8 *)unit, 0x8A) & 0xFFFF) * 0x64));
    if ((unit->flags & 4) == 0) {
        if (maximumSp > 0x3E7) {
            maximumSp = 0x3E7;
        }
    }
    if (p4_game_unit_native_active()) {
        /* datCalcGetLevel's reads without its asserts: a party slot may be empty. */
        s32 level = 0;
        if ((unit->flags & 4) != 0 || unit->id == 1) {
            level = unit->level;
        } else if (unit->id >= 2 && unit->id < 0xB) {
            level = (s32)datPersonaGetRecord(unit->id);
            level = datPersonaGetLevel(level) & 0xFF;
        }
        maximumSp = p4_game_unit_native_amount((unit->flags & 4) != 0, unit->id, P4_UNIT_MAX_SP, (u32)level,
            maximumSp);
    }
    return maximumSp & 0xFFFF;
}
