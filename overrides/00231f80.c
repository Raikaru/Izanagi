/* 00231f80: datCalcMaxHp. The native maximum (table value, HP skills and
 * accessories, the party's 999 cap) passes the unit stage; the stage never
 * runs while no modifier is registered. The enemy-ID bound follows the
 * published banks, as config/mirror_fixes.txt bounds the retail function. */
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_unit_stage_native.h"
#pragma push
/* measured: with common-subexpression and propagation optimizations off, retail computes the array
 * index before adding the base address, and the nested guard keeps the retail compare-and-branch. */
#pragma opt_common_subs off
#pragma opt_propagation off
u16 datCalcMaxHp(DatUnit *unit)
{
    /* Enemy rows are 0x3C bytes; offsets +4/+6 hold HP/SP. */
    s32 baseHp;
    s32 bonusHp;
    s32 maximumHp;
    u16 tableHp;
    u8 *base;

    bonusHp = 0;
    baseHp = unit->id;
    if ((unit->flags & 4) != 0) {
        if ((baseHp & 0xFFFF) >= p4_game_enemies_native_limit()) {
            kAssertFail(D_00635938, 0xBF);
        }
        base = iGpffffb3c4;
        tableHp = ((DatEnemyUnitInit *)((u8 *)((u32)byteAddressAdd((u32)((baseHp & 0xFFFF) * 0x3C), (u8 *)((u32)base)))))->hp;
    } else {
        if ((baseHp & 0xFFFF) >= 0xB) {
            kAssertFail(D_00635938, 0xC2);
        }
        tableHp = (u16)datGetLevelTableFirstValue((s16)baseHp);
    }
    baseHp = tableHp & 0xFFFF;
    if (datCalcHasSkill(unit, 0x1F0) != 0) {
        bonusHp += (baseHp * 0xA) / 100;
    }
    if (datCalcHasSkill(unit, 0x1F1) != 0) {
        bonusHp += (baseHp * 0x14) / 100;
    }
    if (datCalcHasSkill(unit, 0x1F2) != 0) {
        bonusHp += (baseHp * 0x1E) / 100;
    }
    if (datCalcHasSkill(unit, 0x22D) != 0) {
        bonusHp += (baseHp * 0x32) / 100;
    }
    if (datCalcHasSkill(unit, 0x20C) != 0) {
        bonusHp += baseHp;
    }
    if (datCalcHasSkill(unit, 0x20D) != 0) {
        bonusHp -= (baseHp * 0x32) / 100;
    }
    bonusHp += (datCalcCountSlotsWithValue((u8 *)unit, 0x1A) & 0xFFFF) * 0xA;
    bonusHp += (datCalcCountSlotsWithValue((u8 *)unit, 0x1B) & 0xFFFF) * 0x14;
    bonusHp += (datCalcCountSlotsWithValue((u8 *)unit, 0x1C) & 0xFFFF) * 0x1E;
    bonusHp += (datCalcCountSlotsWithValue((u8 *)unit, 0x1D) & 0xFFFF) * 0x28;
    bonusHp += (datCalcCountSlotsWithValue((u8 *)unit, 0x1E) & 0xFFFF) * 0x32;
    maximumHp = baseHp + (bonusHp + ((datCalcCountSlotsWithValue((u8 *)unit, 0x5B) & 0xFFFF) * 0x64));
    if ((unit->flags & 4) == 0) {
        if (maximumHp > 0x3E7) {
            maximumHp = 0x3E7;
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
        maximumHp = p4_game_unit_native_amount((unit->flags & 4) != 0, unit->id, P4_UNIT_MAX_HP, (u32)level,
            maximumHp);
    }
    return maximumHp & 0xFFFF;
}
#pragma pop
