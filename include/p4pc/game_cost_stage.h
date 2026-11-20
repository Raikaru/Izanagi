#ifndef P4PC_GAME_COST_STAGE_H
#define P4PC_GAME_COST_STAGE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_damage_stage.h"
/* Skill cost stage (docs/modding/tables.md "Skill cost stage"): one ordered
 * composition of registered modifiers per nonzero skill cost, at the end of
 * datCalcSkillCost 0x0023D9B0, after its percentage/flat cost, the Spell
 * Master/Arms Master halving and the minimum of 1. Every cost reader goes
 * through that function: the battle skill list draw 00205170, availability
 * 0023ddc0, the deduction in 001f14f0, AI 001e8d80 and the Camp/Persona
 * cost rows (0010f600 via 00113520). A zero cost (an enemy's HP cost, a
 * free enemy) never reaches it. With no modifier registered the function is
 * retail and the stage never runs. Sides use the damage stage's values. */
#define P4_COST_RESOURCE_HP 1u
#define P4_COST_RESOURCE_SP 2u
#define P4_COST_MAX 65535
typedef enum P4GameCostKind { P4_COST_MULTIPLY = 0, P4_COST_ADD = 1 } P4GameCostKind;
/* resource is the skill row's cost kind (+3): HP or SP. persona/persona_level
 * name the persona whose cost is priced: the party unit's active persona
 * record, or the record a Persona/Camp cost row shows (00113520), which may
 * be a stocked persona; 0 for an enemy. */
typedef struct P4GameCostHit {
    uint32_t skill_id;
    uint32_t side, unit_id;
    uint32_t resource;
    int32_t stock;
    uint32_t persona, persona_level;
} P4GameCostHit;
typedef int (*P4GameCostModifier)(void *user, const P4GameCostHit *hit, double *value);
typedef struct P4GameCostModifierDesc {
    const char *owner;
    int32_t priority;
    uint32_t load_order;
    P4GameCostKind kind;
    P4GameCostModifier modifier;
    void *user;
} P4GameCostModifierDesc;
/* Same status codes as the damage stage (P4_DAMAGE_OK ... NOT_FOUND). */
int p4_game_cost_register(const P4GameCostModifierDesc *desc, uint32_t *handle);
int p4_game_cost_unregister(uint32_t handle);
size_t p4_game_cost_count(void);
/* Composes in double and converts once by rounding half up (a cost is a
 * count of HP or SP, and V's per-step percentages are meant as rounded
 * amounts), clamped to 1..P4_COST_MAX. P4PC_COST_TRACE=1 traces each cost. */
int32_t p4_game_cost_apply(const P4GameCostHit *hit);
#endif
