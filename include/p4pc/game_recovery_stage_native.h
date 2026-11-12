#ifndef P4PC_GAME_RECOVERY_STAGE_NATIVE_H
#define P4PC_GAME_RECOVERY_STAGE_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the recovery stage for datCalcSkillAmount
 * 0x00235520. The resolved/origin facts come from 001f14f0's damage-stage
 * bracket around the same HP amount call. */
int p4_game_recovery_native_active(void);
int32_t p4_game_recovery_native_amount(uint32_t skill, uint32_t source_enemy, uint32_t source_id,
    uint32_t target_enemy, uint32_t target_id, int32_t stock, uint32_t source_persona, uint32_t source_persona_level);
#endif
