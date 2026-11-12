/* Unit stage: the ordered modifier composition a unit's max HP/SP, an
 * enemy's starting HP/SP and the battle stats pass (docs/modding/tables.md
 * "Unit stage"). Main-thread game code on the shared stage registry. */
#include "p4pc/game_unit_stage.h"
#include "game_stage_internal.h"
#include <stdio.h>

static int invoke(GameStageFn fn, void *user, const void *query, double *value)
{
    return ((P4GameUnitModifier)fn)(user, (const P4GameUnitQuery *)query, value);
}
static GameStage stage = {.name = "unit stage", .trace_variable = "P4PC_UNIT_TRACE", .invoke = invoke};

int p4_game_unit_register(const P4GameUnitModifierDesc *desc, uint32_t *handle)
{
    if (!desc || (desc->kind != P4_UNIT_MULTIPLY && desc->kind != P4_UNIT_ADD)) return P4_DAMAGE_INVALID;
    return game_stage_register(&stage, desc->owner, desc->priority, desc->load_order,
        desc->kind == P4_UNIT_MULTIPLY ? GAME_STAGE_MULTIPLY : GAME_STAGE_ADD, (GameStageFn)desc->modifier,
        desc->user, handle);
}
int p4_game_unit_unregister(uint32_t handle) { return game_stage_unregister(&stage, handle); }
size_t p4_game_unit_count(void) { return game_stage_count(&stage); }

const char *p4_game_unit_stat_name(uint32_t stat)
{
    static const char *const names[] = {"max_hp", "max_sp", "strength", "magic", "endurance", "agility", "luck"};
    return stat < sizeof names / sizeof names[0] ? names[stat] : NULL;
}

static int32_t clamp(const P4GameUnitQuery *query, double value)
{
    double low, high;
    if (query->stat == P4_UNIT_MAX_HP || query->stat == P4_UNIT_MAX_SP) {
        low = query->stat == P4_UNIT_MAX_HP ? 1.0 : 0.0;
        high = query->side == P4_DAMAGE_SIDE_ENEMY ? P4_UNIT_ENEMY_POOL_MAX : P4_UNIT_PARTY_POOL_MAX;
    } else {
        low = P4_UNIT_STAT_MIN;
        high = P4_UNIT_STAT_MAX;
    }
    return value < low ? (int32_t)low : value > high ? (int32_t)high : (int32_t)value;
}

int32_t p4_game_unit_apply(const P4GameUnitQuery *query)
{
    if (!query) return 0;
    if (stage.composing) {
        fprintf(stderr, "p4pc: unit stage: nested %s left at its stock value\n",
            p4_game_unit_stat_name(query->stat) ? p4_game_unit_stat_name(query->stat) : "value");
        return query->stock;
    }
    const int trace = game_stage_trace(&stage);
    if (trace)
        fprintf(stderr, "p4pc: unit stage: %s:%u %s, level %u, stock %d\n",
            query->side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party", (unsigned)query->id,
            p4_game_unit_stat_name(query->stat) ? p4_game_unit_stat_name(query->stat) : "?",
            (unsigned)query->level, (int)query->stock);
    double value;
    (void)game_stage_compose(&stage, query, (double)query->stock, &value);
    /* Truncate once like the native values, then keep one the unit can hold. */
    const int32_t result = clamp(query, value);
    if (trace) fprintf(stderr, "p4pc: unit stage:   final %d\n", (int)result);
    return result;
}

int p4_game_unit_native_active(void) { return stage.count != 0; }
int32_t p4_game_unit_native_amount(uint32_t enemy, uint32_t id, uint32_t stat, uint32_t level, int32_t stock)
{
    const P4GameUnitQuery query = {
        .side = enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .id = id, .stat = stat, .level = level,
        .stock = stock,
    };
    return p4_game_unit_apply(&query);
}
