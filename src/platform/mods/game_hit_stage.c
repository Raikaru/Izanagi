/* Hit stage: the ordered modifier composition each rolled accuracy and
 * critical chance passes (docs/modding/tables.md "Hit stage"). Main-thread
 * battle code on the shared stage registry. */
#include "p4pc/game_hit_stage.h"
#include "game_stage_internal.h"
#include <stdio.h>

static int invoke(GameStageFn fn, void *user, const void *query, double *value)
{
    return ((P4GameHitModifier)fn)(user, (const P4GameHitQuery *)query, value);
}
static GameStage stage = {.name = "hit stage", .trace_variable = "P4PC_HIT_TRACE", .invoke = invoke};

int p4_game_hit_register(const P4GameHitModifierDesc *desc, uint32_t *handle)
{
    if (!desc || (desc->kind != P4_HIT_MULTIPLY && desc->kind != P4_HIT_ADD)) return P4_DAMAGE_INVALID;
    return game_stage_register(&stage, desc->owner, desc->priority, desc->load_order,
        desc->kind == P4_HIT_MULTIPLY ? GAME_STAGE_MULTIPLY : GAME_STAGE_ADD, (GameStageFn)desc->modifier,
        desc->user, handle);
}
int p4_game_hit_unregister(uint32_t handle) { return game_stage_unregister(&stage, handle); }
size_t p4_game_hit_count(void) { return game_stage_count(&stage); }

static int32_t clamp(double value)
{
    return value < P4_HIT_MIN ? P4_HIT_MIN : value > P4_HIT_MAX ? P4_HIT_MAX : (int32_t)value;
}

int32_t p4_game_hit_apply(const P4GameHitQuery *query)
{
    if (!query) return P4_HIT_MIN;
    const int32_t stock = clamp((double)query->stock);
    const char *const roll = query->roll == P4_HIT_CRITICAL ? "critical" : "accuracy";
    if (stage.composing) {
        fprintf(stderr, "p4pc: hit stage: nested %s roll left at its stock value\n", roll);
        return stock;
    }
    const int trace = game_stage_trace(&stage);
    if (trace)
        fprintf(stderr, "p4pc: hit stage: %s skill 0x%x %s:%u -> %s:%u, stock %d\n", roll,
            (unsigned)query->skill_id, query->source_side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party",
            (unsigned)query->source_id, query->target_side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party",
            (unsigned)query->target_id, (int)query->stock);
    double value;
    (void)game_stage_compose(&stage, query, (double)query->stock, &value);
    /* Truncate once like the native chance, then keep a percent the roll can use. */
    const int32_t result = clamp(value);
    if (trace) fprintf(stderr, "p4pc: hit stage:   final %d\n", (int)result);
    return result;
}

int p4_game_hit_native_active(void) { return stage.count != 0; }
int32_t p4_game_hit_native_rate(uint32_t roll, uint32_t skill, uint32_t source_enemy, uint32_t source_id,
    uint32_t target_enemy, uint32_t target_id, int32_t stock)
{
    const P4GameHitQuery query = {
        .roll = roll, .skill_id = skill,
        .source_side = source_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .source_id = source_id,
        .target_side = target_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .target_id = target_id,
        .stock = stock,
    };
    return p4_game_hit_apply(&query);
}
