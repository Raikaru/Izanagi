/* 00232c70: datCalcGetBattleStat, the stat every battle formula reads. The
 * native stat (an enemy's table row, a party member's Persona total) passes
 * the unit stage before Enervation halves it; the stage never runs while no
 * modifier is registered. */
#include "p4pc/game_unit_stage_native.h"
u8 datCalcGetBattleStat(DatUnit *unit, s32 statIndex)
{
    u8 value;
    s32 statValue;
    u8 halved;

    if (((s32)(statIndex & 0xFFFF) < 0) || ((statIndex & 0xFFFF) >= 5)) {
        kAssertFail(D_00635938, 0x313);
    }
    if ((unit->flags & 4) != 0) {
        value = (u32)datCalcGetUnitStat(unit, statIndex);
        value &= 0xFF;
    } else {
        if (unit->id >= 0xB) {
            kAssertFail(D_00635938, 0x31A);
        }
        value = (u32)datPersonaGetTotalStatById(unit->id, statIndex);
        value &= 0xFF;
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
        value = (u8)p4_game_unit_native_amount((unit->flags & 4) != 0, unit->id,
            P4_UNIT_STRENGTH + (u32)(statIndex & 0xFFFF), (u32)level, value);
    }
    halved = (unit->bad & 0x80) != 0;
    if (!halved) {
        goto no_shift;
    }
    value >>= 1;
no_shift:
    statValue = (u32)value & 0xFF;
    if (statValue > 0) {
        goto clamp_value;
    }
    value = 1;
    goto done_value;
clamp_value:
    if (statValue <= 0x63) {
        goto done_value;
    }
    value = 0x63;
done_value:
    return value;
}
