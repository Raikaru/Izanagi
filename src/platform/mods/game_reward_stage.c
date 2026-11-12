/* Battle reward stage: the ordered modifier composition a won battle's
 * awards pass (docs/modding/tables.md "Reward stage"). Main-thread battle
 * code on the shared stage registry. */
#include "p4pc/game_reward_stage.h"
#include "p4pc/game_reward_stage_native.h"
#include "game_stage_internal.h"
#include <stdio.h>

static int invoke(GameStageFn fn, void *user, const void *query, double *value)
{
    return ((P4GameRewardModifier)fn)(user, (const P4GameRewardQuery *)query, value);
}
static GameStage stage = {.name = "reward stage", .trace_variable = "P4PC_REWARD_TRACE", .invoke = invoke};

int p4_game_reward_register(const P4GameRewardModifierDesc *desc, uint32_t *handle)
{
    if (!desc || (desc->kind != P4_REWARD_MULTIPLY && desc->kind != P4_REWARD_ADD)) return P4_DAMAGE_INVALID;
    return game_stage_register(&stage, desc->owner, desc->priority, desc->load_order,
        desc->kind == P4_REWARD_MULTIPLY ? GAME_STAGE_MULTIPLY : GAME_STAGE_ADD, (GameStageFn)desc->modifier,
        desc->user, handle);
}
int p4_game_reward_unregister(uint32_t handle) { return game_stage_unregister(&stage, handle); }
size_t p4_game_reward_count(void) { return game_stage_count(&stage); }

static int32_t clamp(double value)
{
    return value < P4_REWARD_MIN ? P4_REWARD_MIN : value > P4_REWARD_MAX ? P4_REWARD_MAX : (int32_t)value;
}

int32_t p4_game_reward_apply(const P4GameRewardQuery *query)
{
    if (!query) return P4_REWARD_MIN;
    const int32_t stock = clamp((double)query->stock);
    if (stage.composing) {
        fprintf(stderr, "p4pc: reward stage: nested award left at its stock value\n");
        return stock;
    }
    static const char *const recipients[] = {"team", "hero", "party", "persona"};
    const int trace = game_stage_trace(&stage);
    if (trace)
        fprintf(stderr, "p4pc: reward stage: %s %s:%u level %u%s, stock %d\n",
            query->reward == P4_REWARD_MONEY ? "money" : "exp",
            query->recipient < 4 ? recipients[query->recipient] : "?", (unsigned)query->id, (unsigned)query->level,
            query->equipped ? ", equipped" : "", (int)query->stock);
    double value;
    (void)game_stage_compose(&stage, query, (double)query->stock, &value);
    /* Truncate once like the native awards, then keep an amount the game can pay. */
    const int32_t result = clamp(value);
    if (trace) fprintf(stderr, "p4pc: reward stage:   final %d\n", (int)result);
    return result;
}

int p4_game_reward_native_active(void) { return stage.count != 0; }
static int32_t exp_award(uint32_t recipient, uint32_t id, uint32_t level, uint32_t equipped, int32_t stock)
{
    const P4GameRewardQuery query = {
        .reward = P4_REWARD_EXP, .recipient = recipient, .id = id, .level = level, .equipped = equipped,
        .stock = stock,
    };
    return p4_game_reward_apply(&query);
}
int32_t p4_game_reward_native_hero(uint32_t level, int32_t stock)
{
    return exp_award(P4_REWARD_HERO, 1u, level, 0u, stock);
}
int32_t p4_game_reward_native_party(uint32_t member, uint32_t level, int32_t stock)
{
    return exp_award(P4_REWARD_PARTY, member, level, 0u, stock);
}
int32_t p4_game_reward_native_persona(uint32_t persona, uint32_t level, uint32_t equipped, int32_t stock)
{
    return exp_award(P4_REWARD_PERSONA, persona, level, equipped ? 1u : 0u, stock);
}
int32_t p4_game_reward_native_money(int32_t stock)
{
    const P4GameRewardQuery query = {.reward = P4_REWARD_MONEY, .stock = stock};
    return p4_game_reward_apply(&query);
}
