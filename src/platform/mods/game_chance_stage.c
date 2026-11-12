/* Status/KO chance stage: the ordered modifier composition datCalcInflictStatus
 * 0x00238940 runs once per rolled attempt (docs/modding/tables.md "Status
 * chance stage"). Main-thread battle code on the shared stage registry. */
#include "p4pc/game_chance_stage.h"
#include "p4pc/game_chance_stage_native.h"
#include "game_stage_internal.h"
#include <stdio.h>

static int invoke(GameStageFn fn, void *user, const void *hit, double *value)
{
    return ((P4GameChanceModifier)fn)(user, (const P4GameChanceHit *)hit, value);
}
static GameStage stage = {.name = "chance stage", .trace_variable = "P4PC_CHANCE_TRACE", .invoke = invoke};

int p4_game_chance_register(const P4GameChanceModifierDesc *desc, uint32_t *handle)
{
    if (!desc || (desc->kind != P4_CHANCE_MULTIPLY && desc->kind != P4_CHANCE_ADD)) return P4_DAMAGE_INVALID;
    return game_stage_register(&stage, desc->owner, desc->priority, desc->load_order,
        desc->kind == P4_CHANCE_MULTIPLY ? GAME_STAGE_MULTIPLY : GAME_STAGE_ADD, (GameStageFn)desc->modifier,
        desc->user, handle);
}
int p4_game_chance_unregister(uint32_t handle) { return game_stage_unregister(&stage, handle); }
size_t p4_game_chance_count(void) { return game_stage_count(&stage); }

int32_t p4_game_chance_apply(const P4GameChanceHit *hit)
{
    if (!hit) return P4_CHANCE_MIN;
    if (stage.composing) {
        fprintf(stderr, "p4pc: chance stage: nested attempt for skill 0x%x left at its stock value\n",
            (unsigned)hit->skill_id);
        return hit->stock;
    }
    const int trace = game_stage_trace(&stage);
    if (trace)
        fprintf(stderr, "p4pc: chance stage: skill 0x%x %s:%u -> %s:%u %s 0x%x%s, persona %u, afflictions %u, stock %d\n",
            (unsigned)hit->skill_id, hit->source_side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party",
            (unsigned)hit->source_id, hit->target_side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party",
            (unsigned)hit->target_id, hit->component == P4_CHANCE_COMPONENT_INSTANT_KO ? "instant-ko" : "ailment",
            (unsigned)hit->statuses, hit->item ? ", item" : "", (unsigned)hit->source_persona,
            (unsigned)hit->afflictions, (int)hit->stock);
    double value;
    (void)game_stage_compose(&stage, hit, (double)hit->stock, &value);
    /* Truncate once like the native conversion; the caller's 0..99 clamp
     * follows, so only keep the value inside a range that conversion holds. */
    const int32_t result = value < -1e6 ? -1000000 : value > 1e6 ? 1000000 : (int32_t)value;
    if (trace) fprintf(stderr, "p4pc: chance stage:   final %d\n", (int)result);
    return result;
}

int p4_game_chance_native_active(void) { return stage.count != 0; }
int32_t p4_game_chance_native_rate(uint32_t skill, uint32_t source_enemy, uint32_t source_id,
    uint32_t target_enemy, uint32_t target_id, uint32_t statuses, uint32_t item, int32_t stock,
    uint32_t source_persona, uint32_t source_persona_level, uint32_t afflictions)
{
    const int persona = !source_enemy && !item;
    const P4GameChanceHit hit = {
        .skill_id = skill,
        .source_side = source_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .source_id = source_id,
        .target_side = target_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .target_id = target_id,
        .component = statuses == 0x80000u ? P4_CHANCE_COMPONENT_INSTANT_KO : P4_CHANCE_COMPONENT_AILMENT,
        .statuses = statuses, .item = item ? 1u : 0u, .stock = stock,
        .source_persona = persona ? source_persona : 0u,
        .source_persona_level = persona && source_persona ? source_persona_level : 0u,
        .afflictions = afflictions,
    };
    return p4_game_chance_apply(&hit);
}
