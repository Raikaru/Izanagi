#ifndef P4PC_GOLDEN_ENCOUNTER_NATIVE_H
#define P4PC_GOLDEN_ENCOUNTER_NATIVE_H
#include "p4pc/game_mode.h"
#include "p4pc/golden_encounter_rates.h"
/* The existing four-argument native selector still has a level, not forced
 * Golden modes 3/4. E2 remains refused. This bridge reads the actual projected
 * row with Golden signedness and preserves the projected PS2 suppression. */
static inline int32_t p4_golden_encounter_native_total(const uint8_t *row,
    unsigned column, uint16_t level, int exclude, int multiplier)
{
    return p4_golden_encounter_projected_total(row,column,level,exclude,
        multiplier==100,multiplier==0);
}
#endif
