#ifndef P4PC_GAME_HIT_STAGE_NATIVE_H
#define P4PC_GAME_HIT_STAGE_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the hit stage for datCalcResolveSkillHit
 * 0x0023E6F0 (accuracy) and datCalcHitResult 0x002411A0 (critical). Returns
 * the composed chance in percent, clamped to 0..100, for the native
 * RpRandom() % 100 compare. */
#define P4_HIT_ACCURACY 1u
#define P4_HIT_CRITICAL 2u
int p4_game_hit_native_active(void);
int32_t p4_game_hit_native_rate(uint32_t roll, uint32_t skill, uint32_t source_enemy, uint32_t source_id,
    uint32_t target_enemy, uint32_t target_id, int32_t stock);
#endif
