/* 00231af0: datUnitInit. An enemy's starting HP and SP (its table row) pass
 * the unit stage as max HP and max SP, so a scaled maximum starts full; the
 * stage never runs while no modifier is registered. The enemy-ID bound
 * follows the published banks, as config/mirror_fixes.txt bounds the retail
 * function. */
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_unit_stage_native.h"
/* Initialize PC flags or copy enemy defaults, clear all buff levels and set both unit IDs. */
u32 datUnitInit(DatUnit* unit, u8 genus, u16 id)
{
    u32 i;
    u32 j;

    switch (genus)
    {
    case UNIT_GENUS_PC:
        if ((id & 0xFFFF) >= 0xB) kAssertFail(D_00635928, 0xD3);
        unit->flags &= ~0x4;
        unit->flags &= ~0x8;
        unit->flags &= ~0x10;
        unit->flags &= ~0x200;
        break;
    case UNIT_GENUS_EC:
        if ((id & 0xFFFF) >= p4_game_enemies_native_limit()) kAssertFail(D_00635928, 0xDA);
        unit->flags = 4;
        unit->level = iGpffffb3c4[id * 0x3c + 3];
        unit->hp = ((DatEnemyUnitInit *)((u8*)(uintptr_t)((u32)id * 0x3c) +
                          (uintptr_t)iGpffffb3c4))->hp;
        unit->sp = ((DatEnemyUnitInit *)((u8*)(uintptr_t)((u32)id * 0x3c) +
                          (uintptr_t)iGpffffb3c4))->sp;
        if (p4_game_unit_native_active()) {
            unit->hp = (u16)p4_game_unit_native_amount(1u, id, P4_UNIT_MAX_HP, unit->level, unit->hp);
            unit->sp = (u16)p4_game_unit_native_amount(1u, id, P4_UNIT_MAX_SP, unit->level, unit->sp);
        }
        unit->bad = 0;
        if (((DatEnemyUnitInit *)(iGpffffb3c4 + id * 0x3c))->flags & 8) unit->flags |= 0x8;
        if (((DatEnemyUnitInit *)(iGpffffb3c4 + id * 0x3c))->flags & 0x10) unit->flags |= 0x10;
        if (((DatEnemyUnitInit *)(iGpffffb3c4 + id * 0x3c))->flags & 0x100) unit->flags |= 0x40;
        if (((DatEnemyUnitInit *)(iGpffffb3c4 + id * 0x3c))->flags & 0x200) unit->flags |= 0x80;
        for (i = 0; i < 8; i++)
        {
            u16 skill = ((DatEnemyUnitInit*)iGpffffb3c4)[id].skills[i];
            switch (skill)
            {
            case 0x1F6:
            case 0x1F7:
            case 0x1F8:
                unit->flags |= 0x100;
                break;
            }
        }
        break;
    default:
        kAssertFail(D_00635928, 0xFE);
        break;
    }
    for (j = 0; j < 0x18; j++) datCalcClearBuffLevel(unit, (u8)j);
    unit->id = id;
    unit->id2 = id;
    unit->flags |= 0x1;
    return 1;
}
