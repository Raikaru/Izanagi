#ifndef P4PC_GAME_DAMAGE_STAGE_NATIVE_H
#define P4PC_GAME_DAMAGE_STAGE_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the final-damage stage for the translated units:
 * datCalcSkillAmount 0x00235520 and 001f14f0's per-hit resolution. */
int p4_game_damage_native_active(void);
/* Brackets 001f14f0's HP amount call for one resolved hit: the unit it was
 * aimed at, whether a repel delivers it back to the source, and the command:
 * item for an item action (its options bit 0), weapon for the weapon-property
 * skills (Attack and its follow-ups). The recovery stage reads the same
 * bracket. */
void p4_game_damage_native_hit_begin(uint32_t skill, uint32_t origin_enemy, uint32_t origin_id, uint32_t reflected,
    uint32_t item, uint32_t weapon);
void p4_game_damage_native_hit_end(void);
/* source_persona/source_persona_level: the party source's active persona
 * record (datPersonaGetRecordId/datPersonaGetLevelByPcId), 0 for an enemy. */
int32_t p4_game_damage_native_amount(uint32_t skill, uint32_t source_enemy, uint32_t source_id,
    uint32_t source_status, uint32_t target_enemy, uint32_t target_id, uint32_t target_status,
    uint32_t hit_kind, uint32_t element_valid, uint32_t element_id, int32_t stock,
    uint32_t source_persona, uint32_t source_persona_level);
#endif
