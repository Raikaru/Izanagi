/* Recovery stage: the ordered modifier composition datCalcSkillAmount
 * 0x00235520 runs once per eligible formula heal (docs/modding/tables.md
 * "Recovery stage"). Main-thread battle code on the shared stage registry. */
#include "p4pc/game_recovery_stage.h"
#include "p4pc/game_recovery_stage_native.h"
#include "game_stage_internal.h"
#include <stdio.h>

static int invoke(GameStageFn fn, void *user, const void *hit, double *value)
{
    return ((P4GameRecoveryModifier)fn)(user, (const P4GameRecoveryHit *)hit, value);
}
static GameStage stage = {.name = "recovery stage", .trace_variable = "P4PC_RECOVERY_TRACE", .invoke = invoke};

int p4_game_recovery_register(const P4GameRecoveryModifierDesc *desc, uint32_t *handle)
{
    if (!desc || (desc->kind != P4_RECOVERY_MULTIPLY && desc->kind != P4_RECOVERY_ADD)) return P4_DAMAGE_INVALID;
    return game_stage_register(&stage, desc->owner, desc->priority, desc->load_order,
        desc->kind == P4_RECOVERY_MULTIPLY ? GAME_STAGE_MULTIPLY : GAME_STAGE_ADD, (GameStageFn)desc->modifier,
        desc->user, handle);
}
int p4_game_recovery_unregister(uint32_t handle) { return game_stage_unregister(&stage, handle); }
size_t p4_game_recovery_count(void) { return game_stage_count(&stage); }

int32_t p4_game_recovery_apply(const P4GameRecoveryHit *hit)
{
    if (!hit) return P4_RECOVERY_MIN;
    if (stage.composing) {
        fprintf(stderr, "p4pc: recovery stage: nested heal for skill 0x%x left at its stock value\n",
            (unsigned)hit->skill_id);
        return hit->stock;
    }
    const int trace = game_stage_trace(&stage);
    if (trace)
        fprintf(stderr, "p4pc: recovery stage: skill 0x%x %s:%u -> %s:%u, %s, %s, persona %u, stock %d\n",
            (unsigned)hit->skill_id, hit->source_side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party",
            (unsigned)hit->source_id, hit->target_side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party",
            (unsigned)hit->target_id, hit->resolved ? "resolved" : "estimate", game_stage_origin_name(hit->origin),
            (unsigned)hit->source_persona, (int)hit->stock);
    double value;
    (void)game_stage_compose(&stage, hit, (double)hit->stock, &value);
    const int32_t result = value < (double)P4_RECOVERY_MIN ? P4_RECOVERY_MIN :
        value > (double)P4_RECOVERY_MAX ? P4_RECOVERY_MAX : (int32_t)value;
    if (trace) fprintf(stderr, "p4pc: recovery stage:   final %d\n", (int)result);
    return result;
}

int p4_game_recovery_native_active(void) { return stage.count != 0; }
int32_t p4_game_recovery_native_amount(uint32_t skill, uint32_t source_enemy, uint32_t source_id,
    uint32_t target_enemy, uint32_t target_id, int32_t stock, uint32_t source_persona, uint32_t source_persona_level)
{
    GameAmountContext context;
    game_amount_bracket(skill, &context);
    P4GameRecoveryHit hit = {
        .skill_id = skill,
        .source_side = source_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .source_id = source_id,
        .target_side = target_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .target_id = target_id,
        .resolved = context.resolved, .origin = context.origin, .stock = stock,
    };
    game_amount_source_persona(&hit.source_persona, &hit.source_persona_level, source_enemy, context.origin,
        source_persona, source_persona_level);
    return p4_game_recovery_apply(&hit);
}
