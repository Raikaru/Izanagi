/* Buff stage: the ordered modifier composition datCalcBuffMultiplier
 * 0x00233BC0 runs once per stat multiplier (docs/modding/tables.md "Buff
 * stage"). Main-thread battle code on the shared stage registry. */
#include "p4pc/game_buff_stage.h"
#include "p4pc/game_buff_stage_native.h"
#include "game_stage_internal.h"
#include <stdio.h>

static int invoke(GameStageFn fn, void *user, const void *query, double *value)
{
    return ((P4GameBuffModifier)fn)(user, (const P4GameBuffQuery *)query, value);
}
static GameStage stage = {.name = "buff stage", .trace_variable = "P4PC_BUFF_TRACE", .invoke = invoke};

int p4_game_buff_register(const P4GameBuffModifierDesc *desc, uint32_t *handle)
{
    if (!desc || (desc->kind != P4_BUFF_MULTIPLY && desc->kind != P4_BUFF_ADD)) return P4_DAMAGE_INVALID;
    return game_stage_register(&stage, desc->owner, desc->priority, desc->load_order,
        desc->kind == P4_BUFF_MULTIPLY ? GAME_STAGE_MULTIPLY : GAME_STAGE_ADD, (GameStageFn)desc->modifier,
        desc->user, handle);
}
int p4_game_buff_unregister(uint32_t handle) { return game_stage_unregister(&stage, handle); }
size_t p4_game_buff_count(void) { return game_stage_count(&stage); }

const char *p4_game_buff_stat_name(uint32_t stat)
{
    static const char *const names[] = {"attack", "magic", "evasion", "defense", "accuracy"};
    return stat < sizeof names / sizeof names[0] ? names[stat] : NULL;
}

float p4_game_buff_apply(const P4GameBuffQuery *query)
{
    if (!query) return 1.0f;
    if (stage.composing) {
        fprintf(stderr, "p4pc: buff stage: nested %s multiplier left at its stock value\n",
            p4_game_buff_stat_name(query->stat) ? p4_game_buff_stat_name(query->stat) : "stat");
        return (float)query->stock;
    }
    const int trace = game_stage_trace(&stage);
    if (trace)
        fprintf(stderr, "p4pc: buff stage: %s:%u %s, rank %d%s, stock %.4f\n",
            query->side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party", (unsigned)query->id,
            p4_game_buff_stat_name(query->stat) ? p4_game_buff_stat_name(query->stat) : "?", (int)query->rank,
            query->charged ? ", charged" : "", query->stock);
    double value;
    (void)game_stage_compose(&stage, query, query->stock, &value);
    /* A multiplier stays a non-negative factor the equation can use. */
    const double bounded = value < 0.0 ? 0.0 : value > P4_BUFF_MAX ? P4_BUFF_MAX : value;
    if (trace) fprintf(stderr, "p4pc: buff stage:   final %.4f\n", bounded);
    return (float)bounded;
}

int p4_game_buff_native_active(void) { return stage.count != 0; }
float p4_game_buff_native_factor(uint32_t enemy, uint32_t id, uint32_t stat, int32_t rank, uint32_t charged, float stock)
{
    const P4GameBuffQuery query = {
        .side = enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .id = id, .stat = stat, .rank = rank,
        .charged = charged ? 1u : 0u, .stock = (double)stock,
    };
    return p4_game_buff_apply(&query);
}
