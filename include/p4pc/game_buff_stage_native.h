#ifndef P4PC_GAME_BUFF_STAGE_NATIVE_H
#define P4PC_GAME_BUFF_STAGE_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the buff stage for datCalcBuffMultiplier
 * 0x00233BC0: the composed multiplier in the native single precision. */
int p4_game_buff_native_active(void);
float p4_game_buff_native_factor(uint32_t enemy, uint32_t id, uint32_t stat, int32_t rank, uint32_t charged, float stock);
#endif
