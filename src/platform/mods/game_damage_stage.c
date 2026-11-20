/* Final-damage stage: the ordered modifier composition datCalcSkillAmount
 * 0x00235520 runs once per eligible HP-damage hit (docs/modding/tables.md
 * "Final damage stage"). Everything here is main-thread battle code. The
 * order and composition are the shared stage registry's (game_stage.c). */
#include "p4pc/game_damage_stage.h"
#include "p4pc/game_damage_stage_native.h"
#include "game_stage_internal.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int invoke(GameStageFn fn, void *user, const void *hit, double *value)
{
    return ((P4GameDamageModifier)fn)(user, (const P4GameDamageHit *)hit, value);
}
static GameStage stage = {.name = "damage stage", .trace_variable = "P4PC_DAMAGE_TRACE", .invoke = invoke};
/* The resolved hit 001f14f0 brackets: set immediately before its HP amount
 * call and cleared right after, never read outside that call. The recovery
 * stage reads the same bracket for the same call. */
static struct { uint32_t active, skill, origin_side, origin_id, reflected, origin; } bracket;

static const void *offense_owner;
static P4GameOffensePolicy offense_policy;
int p4_game_offense_policy_enable(const void *owner, const P4GameOffensePolicy *policy)
{
    if (!owner || !policy || !isfinite(policy->boost) || !isfinite(policy->amp) ||
        !isfinite(policy->cap) || policy->boost <= 0 || policy->amp <= 0 || policy->cap <= 0)
        return P4_DAMAGE_INVALID;
    if (offense_owner && offense_owner != owner) return P4_DAMAGE_BUSY;
    offense_policy = *policy;
    offense_owner = owner;
    return P4_DAMAGE_OK;
}
void p4_game_offense_policy_disable(const void *owner)
{
    if (owner && offense_owner == owner) offense_owner = NULL;
}
void p4_game_offense_policy_values(float *boost, float *amp, float *cap)
{
    *boost = offense_owner ? offense_policy.boost : 1.25f;
    *amp = offense_owner ? offense_policy.amp : 1.5f;
    *cap = offense_owner ? offense_policy.cap : 1.875f;
}

int p4_game_damage_register(const P4GameDamageModifierDesc *desc, uint32_t *handle)
{
    if (!desc || (desc->kind != P4_DAMAGE_MULTIPLY && desc->kind != P4_DAMAGE_ADD)) return P4_DAMAGE_INVALID;
    return game_stage_register(&stage, desc->owner, desc->priority, desc->load_order,
        desc->kind == P4_DAMAGE_MULTIPLY ? GAME_STAGE_MULTIPLY : GAME_STAGE_ADD, (GameStageFn)desc->modifier,
        desc->user, handle);
}
int p4_game_damage_unregister(uint32_t handle) { return game_stage_unregister(&stage, handle); }
size_t p4_game_damage_count(void) { return game_stage_count(&stage); }

static const char *side_name(uint32_t side) { return side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party"; }
static const char *hit_name(uint32_t kind)
{
    return kind == P4_DAMAGE_HIT_CRITICAL ? "critical" : kind == P4_DAMAGE_HIT_WEAK ? "weak" :
        kind == P4_DAMAGE_HIT_NORMAL ? "normal" : "other";
}
const char *game_stage_origin_name(uint32_t origin)
{
    return origin == P4_DAMAGE_ORIGIN_SKILL ? "skill" : origin == P4_DAMAGE_ORIGIN_ITEM ? "item" :
        origin == P4_DAMAGE_ORIGIN_ATTACK ? "attack" : "unknown";
}
int32_t p4_game_damage_apply(const P4GameDamageHit *hit)
{
    if (!hit) return P4_DAMAGE_MIN;
    if (stage.composing) {
        /* A modifier re-entered the amount equation: the inner estimate keeps
         * its stock value rather than composing modifiers inside modifiers. */
        fprintf(stderr, "p4pc: damage stage: nested hit for skill 0x%x left at its stock value\n",
            (unsigned)hit->skill_id);
        return hit->stock;
    }
    const int trace = game_stage_trace(&stage);
    if (trace)
        fprintf(stderr, "p4pc: damage stage: skill 0x%x %s:%u -> %s:%u (origin %s:%u%s) %s hit, element %s%u, %s, %s, persona %u, stock %d\n",
            (unsigned)hit->skill_id, side_name(hit->source_side), (unsigned)hit->source_id,
            side_name(hit->target_side), (unsigned)hit->target_id, side_name(hit->origin_side),
            (unsigned)hit->origin_id, hit->reflected ? ", reflected" : "", hit_name(hit->hit_kind),
            hit->element_valid ? "" : "none ", (unsigned)hit->element_id,
            hit->resolved ? "resolved" : "estimate", game_stage_origin_name(hit->origin),
            (unsigned)hit->source_persona, (int)hit->stock);
    double value;
    (void)game_stage_compose(&stage, hit, (double)hit->stock, &value);
    const int32_t result = value < (double)P4_DAMAGE_MIN ? P4_DAMAGE_MIN :
        value > (double)P4_DAMAGE_MAX ? P4_DAMAGE_MAX : (int32_t)value;
    if (trace) fprintf(stderr, "p4pc: damage stage:   final %d\n", (int)result);
    return result;
}

int p4_game_damage_native_active(void) { return stage.count != 0; }
void p4_game_damage_native_hit_begin(uint32_t skill, uint32_t origin_enemy, uint32_t origin_id, uint32_t reflected,
    uint32_t item, uint32_t weapon)
{
    bracket.active = 1u; bracket.skill = skill;
    bracket.origin_side = origin_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY;
    bracket.origin_id = origin_id; bracket.reflected = reflected ? 1u : 0u;
    bracket.origin = item ? P4_DAMAGE_ORIGIN_ITEM : weapon ? P4_DAMAGE_ORIGIN_ATTACK : P4_DAMAGE_ORIGIN_SKILL;
}
void p4_game_damage_native_hit_end(void) { memset(&bracket, 0, sizeof bracket); }
void game_amount_bracket(uint32_t skill, GameAmountContext *out)
{
    const int resolved = bracket.active && bracket.skill == skill;
    out->resolved = resolved ? 1u : 0u;
    out->origin = resolved ? bracket.origin : P4_DAMAGE_ORIGIN_UNKNOWN;
    out->origin_side = bracket.origin_side;
    out->origin_id = bracket.origin_id;
    out->reflected = resolved ? bracket.reflected : 0u;
}
int32_t p4_game_damage_native_amount(uint32_t skill, uint32_t source_enemy, uint32_t source_id,
    uint32_t source_status, uint32_t target_enemy, uint32_t target_id, uint32_t target_status,
    uint32_t hit_kind, uint32_t element_valid, uint32_t element_id, int32_t stock,
    uint32_t source_persona, uint32_t source_persona_level)
{
    GameAmountContext context;
    game_amount_bracket(skill, &context);
    P4GameDamageHit hit = {
        .skill_id = skill,
        .source_side = source_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .source_id = source_id,
        .source_status = source_status,
        .target_side = target_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .target_id = target_id,
        .target_status = target_status,
        .hit_kind = hit_kind, .element_valid = element_valid ? 1u : 0u, .element_id = element_id,
        .resolved = context.resolved, .stock = stock, .origin = context.origin,
    };
    hit.origin_side = context.resolved ? context.origin_side : hit.target_side;
    hit.origin_id = context.resolved ? context.origin_id : hit.target_id;
    hit.reflected = context.reflected;
    game_amount_source_persona(&hit.source_persona, &hit.source_persona_level, source_enemy, context.origin,
        source_persona, source_persona_level);
    return p4_game_damage_apply(&hit);
}
void game_amount_source_persona(uint32_t *persona, uint32_t *level, uint32_t source_enemy, uint32_t origin,
    uint32_t record_persona, uint32_t record_level)
{
    const int skill = !source_enemy && (origin == P4_DAMAGE_ORIGIN_SKILL || origin == P4_DAMAGE_ORIGIN_UNKNOWN);
    *persona = skill ? record_persona : 0u;
    *level = skill && record_persona ? record_level : 0u;
}
