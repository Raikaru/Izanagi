#ifndef P4PC_GAME_DAMAGE_STAGE_NATIVE_H
#define P4PC_GAME_DAMAGE_STAGE_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the final-damage stage for the translated units:
 * datCalcSkillAmount 0x00235520 and 001f14f0's per-hit resolution. */
int p4_game_damage_native_active(void);
/* Brackets 001f14f0's HP amount call for one resolved hit: the unit it was
 * aimed at and whether a repel delivers it back to the source. */
void p4_game_damage_native_hit_begin(uint32_t skill, uint32_t origin_enemy, uint32_t origin_id, uint32_t reflected);
void p4_game_damage_native_hit_end(void);
int32_t p4_game_damage_native_amount(uint32_t skill, uint32_t source_enemy, uint32_t source_id,
    uint32_t source_status, uint32_t target_enemy, uint32_t target_id, uint32_t target_status,
    uint32_t hit_kind, uint32_t element_valid, uint32_t element_id, int32_t stock);
#endif
