/* Ailment wear-off stage: the ordered modifier composition
 * btlSoundSelectCureableStatuses 0x001F6930 runs once per recovery roll
 * (docs/modding/tables.md "Ailment wear-off stage"). Main-thread battle code
 * on the shared stage registry. */
#include "p4pc/game_wearoff_stage.h"
#include "p4pc/game_wearoff_stage_native.h"
#include "game_stage_internal.h"
#include <stdio.h>

static int invoke(GameStageFn fn, void *user, const void *roll, double *value)
{
    return ((P4GameWearoffModifier)fn)(user, (const P4GameWearoffRoll *)roll, value);
}
static GameStage stage = {.name = "wear-off stage", .trace_variable = "P4PC_WEAROFF_TRACE", .invoke = invoke};

int p4_game_wearoff_register(const P4GameWearoffModifierDesc *desc, uint32_t *handle)
{
    if (!desc || (desc->kind != P4_WEAROFF_MULTIPLY && desc->kind != P4_WEAROFF_ADD)) return P4_DAMAGE_INVALID;
    return game_stage_register(&stage, desc->owner, desc->priority, desc->load_order,
        desc->kind == P4_WEAROFF_MULTIPLY ? GAME_STAGE_MULTIPLY : GAME_STAGE_ADD, (GameStageFn)desc->modifier,
        desc->user, handle);
}
int p4_game_wearoff_unregister(uint32_t handle) { return game_stage_unregister(&stage, handle); }
size_t p4_game_wearoff_count(void) { return game_stage_count(&stage); }

const char *p4_game_wearoff_status_name(uint32_t status)
{
    switch (status) {
    case 0x02u: return "rage";
    case 0x04u: return "fear";
    case 0x08u: return "silence";
    case 0x10u: return "confusion";
    case 0x40u: return "exhaustion";
    case 0x80u: return "enervation";
    default: return NULL;
    }
}

int32_t p4_game_wearoff_apply(const P4GameWearoffRoll *roll)
{
    if (!roll) return P4_WEAROFF_MIN;
    const int32_t stock = roll->stock < P4_WEAROFF_MIN ? P4_WEAROFF_MIN : roll->stock > P4_WEAROFF_MAX ? P4_WEAROFF_MAX :
        roll->stock;
    if (stage.composing) {
        fprintf(stderr, "p4pc: wear-off stage: nested roll for %s left at its stock value\n",
            p4_game_wearoff_status_name(roll->status) ? p4_game_wearoff_status_name(roll->status) : "an ailment");
        return stock;
    }
    const int trace = game_stage_trace(&stage);
    if (trace)
        fprintf(stderr, "p4pc: wear-off stage: %s:%u %s, level %u, luck %u, turn %u of %u, afflictions %u, stock %d\n",
            roll->side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party", (unsigned)roll->id,
            p4_game_wearoff_status_name(roll->status) ? p4_game_wearoff_status_name(roll->status) : "?",
            (unsigned)roll->level, (unsigned)roll->luck, (unsigned)roll->turns, (unsigned)roll->limit,
            (unsigned)roll->afflictions, (int)roll->stock);
    double value;
    (void)game_stage_compose(&stage, roll, (double)roll->stock, &value);
    /* Truncate once like the native chance, then keep a percent the roll can use. */
    const int32_t result = value < P4_WEAROFF_MIN ? P4_WEAROFF_MIN : value > P4_WEAROFF_MAX ? P4_WEAROFF_MAX : (int32_t)value;
    if (trace) fprintf(stderr, "p4pc: wear-off stage:   final %d\n", (int)result);
    return result;
}

int p4_game_wearoff_native_active(void) { return stage.count != 0; }
int32_t p4_game_wearoff_native_rate(uint32_t enemy, uint32_t id, uint32_t status, uint32_t level, uint32_t luck,
    uint32_t turns, uint32_t limit, int32_t stock, uint32_t afflictions)
{
    const P4GameWearoffRoll roll = {
        .side = enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .id = id, .status = status,
        .level = level, .luck = luck, .turns = turns, .limit = limit, .stock = stock, .afflictions = afflictions,
    };
    return p4_game_wearoff_apply(&roll);
}
