#ifndef P4PC_GAME_CHANCE_STAGE_NATIVE_H
#define P4PC_GAME_CHANCE_STAGE_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the status/KO chance stage for datCalcInflictStatus
 * 0x00238940. Returns the composed chance before the native 0..99 clamp. */
int p4_game_chance_native_active(void);
int32_t p4_game_chance_native_rate(uint32_t skill, uint32_t source_enemy, uint32_t source_id,
    uint32_t target_enemy, uint32_t target_id, uint32_t statuses, uint32_t item, int32_t stock,
    uint32_t source_persona, uint32_t source_persona_level, uint32_t afflictions);
#endif
