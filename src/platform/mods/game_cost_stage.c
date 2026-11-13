/* Skill cost stage: the ordered modifier composition datCalcSkillCost
 * 0x0023D9B0 runs once per nonzero cost (docs/modding/tables.md "Skill cost
 * stage"). Main-thread battle and menu code on the shared stage registry. */
#include "p4pc/game_cost_stage.h"
#include "p4pc/game_cost_stage_native.h"
#include "game_stage_internal.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int invoke(GameStageFn fn, void *user, const void *hit, double *value)
{
    return ((P4GameCostModifier)fn)(user, (const P4GameCostHit *)hit, value);
}
static GameStage stage = {.name = "cost stage", .trace_variable = "P4PC_COST_TRACE", .invoke = invoke};
/* One 00113520 cost row: set immediately before its datCalcSkillCostById
 * call and cleared right after, never read outside that call. */
static struct { uint32_t active, persona, level; } row;

int p4_game_cost_register(const P4GameCostModifierDesc *desc, uint32_t *handle)
{
    if (!desc || (desc->kind != P4_COST_MULTIPLY && desc->kind != P4_COST_ADD)) return P4_DAMAGE_INVALID;
    return game_stage_register(&stage, desc->owner, desc->priority, desc->load_order,
        desc->kind == P4_COST_MULTIPLY ? GAME_STAGE_MULTIPLY : GAME_STAGE_ADD, (GameStageFn)desc->modifier,
        desc->user, handle);
}
int p4_game_cost_unregister(uint32_t handle) { return game_stage_unregister(&stage, handle); }
size_t p4_game_cost_count(void) { return game_stage_count(&stage); }

int32_t p4_game_cost_apply(const P4GameCostHit *hit)
{
    if (!hit) return 1;
    if (stage.composing) {
        fprintf(stderr, "p4pc: cost stage: nested cost for skill 0x%x left at its stock value\n",
            (unsigned)hit->skill_id);
        return hit->stock;
    }
    const int trace = game_stage_trace(&stage);
    if (trace)
        fprintf(stderr, "p4pc: cost stage: skill 0x%x %s:%u %s, persona %u level %u, stock %d\n",
            (unsigned)hit->skill_id, hit->side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party",
            (unsigned)hit->unit_id, hit->resource == P4_COST_RESOURCE_HP ? "hp" : "sp",
            (unsigned)hit->persona, (unsigned)hit->persona_level, (int)hit->stock);
    double value;
    (void)game_stage_compose(&stage, hit, (double)hit->stock, &value);
    /* Round half up once; a nonzero cost stays at least 1. */
    const double rounded = floor(value + 0.5);
    const int32_t result = rounded < 1.0 ? 1 : rounded > (double)P4_COST_MAX ? P4_COST_MAX : (int32_t)rounded;
    if (trace) fprintf(stderr, "p4pc: cost stage:   final %d\n", (int)result);
    return result;
}

int p4_game_cost_native_active(void) { return stage.count != 0; }
void p4_game_cost_native_persona_begin(uint32_t persona, uint32_t persona_level)
{
    row.active = 1u; row.persona = persona; row.level = persona_level;
}
void p4_game_cost_native_persona_end(void) { memset(&row, 0, sizeof row); }
uint32_t p4_game_cost_native_amount(uint32_t skill, uint32_t unit_enemy, uint32_t unit_id, uint32_t resource,
    uint32_t stock, uint32_t record_persona, uint32_t record_level)
{
    const int shown = !unit_enemy && row.active;
    P4GameCostHit hit = {
        .skill_id = skill, .side = unit_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .unit_id = unit_id,
        .resource = resource == 1u ? P4_COST_RESOURCE_HP : P4_COST_RESOURCE_SP,
        .stock = stock > (uint32_t)P4_COST_MAX ? P4_COST_MAX : (int32_t)stock,
        .persona = unit_enemy ? 0u : shown ? row.persona : record_persona,
        .persona_level = unit_enemy ? 0u : shown ? row.level : record_level,
    };
    return (uint32_t)p4_game_cost_apply(&hit);
}
