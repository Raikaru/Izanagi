/* 00232680: datCalcSetBadStatus. A landing ailment is counted for the
 * battle's afflictions (include/p4pc/game_afflictions_native.h) while a
 * chance or wear-off modifier is registered; the status word changes exactly
 * as retail. */
#include "p4pc/game_afflictions_native.h"
/* Replace nonzero ailment bits 0..19, and accumulate the independent high status bits. */
u32 datCalcSetBadStatus(DatUnit *unit, u32 badStatus)
{
    u32 low;

    low = badStatus & 0x000fffff;
    if (low != 0) {
        if ((low & P4_AFFLICTION_MASK) != 0 && p4_game_afflictions_native_active()) {
            p4_game_afflictions_native_landed(unit, low);
        }
        unit->bad = (unit->bad & 0xfff00000) | low;
    }
    return unit->bad |= badStatus & 0xfff00000;
}
