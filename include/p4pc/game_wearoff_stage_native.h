#ifndef P4PC_GAME_WEAROFF_STAGE_NATIVE_H
#define P4PC_GAME_WEAROFF_STAGE_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the ailment wear-off stage for
 * btlSoundSelectCureableStatuses 0x001F6930. Returns the composed chance in
 * percent, clamped to 0..100, for the native datCalcRandomBelow(100) compare. */
int p4_game_wearoff_native_active(void);
int32_t p4_game_wearoff_native_rate(uint32_t enemy, uint32_t id, uint32_t status, uint32_t level, uint32_t luck,
    uint32_t turns, uint32_t limit, int32_t stock, uint32_t afflictions);
#endif
