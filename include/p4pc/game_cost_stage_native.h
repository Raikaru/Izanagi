#ifndef P4PC_GAME_COST_STAGE_NATIVE_H
#define P4PC_GAME_COST_STAGE_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the skill cost stage for the translated units:
 * datCalcSkillCost 0x0023D9B0 and datPersonaFillSkillCostRow 0x00113520. */
int p4_game_cost_native_active(void);
/* Brackets one cost row of 00113520 around its single datCalcSkillCostById
 * call: the persona record that row shows (possibly a stocked persona)
 * prices the cost of that call. */
void p4_game_cost_native_persona_begin(uint32_t persona, uint32_t persona_level);
void p4_game_cost_native_persona_end(void);
/* record_persona/record_level: the unit's active persona record, used unless
 * a 00113520 bracket encloses this call. */
uint32_t p4_game_cost_native_amount(uint32_t skill, uint32_t unit_enemy, uint32_t unit_id, uint32_t resource,
    uint32_t stock, uint32_t record_persona, uint32_t record_level);
#endif
