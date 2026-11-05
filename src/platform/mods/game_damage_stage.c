/* Final-damage stage: the ordered modifier composition datCalcSkillAmount
 * 0x00235520 runs once per eligible HP-damage hit (docs/modding/tables.md
 * "Final damage stage"). Everything here is main-thread battle code. */
#include "p4pc/game_damage_stage.h"
#include "p4pc/game_damage_stage_native.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Modifier {
    P4GameDamageModifierDesc desc;
    char owner[P4_DAMAGE_OWNER_SIZE];
    uint32_t handle, sequence, reported;
} Modifier;

static Modifier *modifiers;
static size_t count, capacity;
static uint32_t next_handle = 1u, next_sequence;
static int composing;
/* The resolved hit 001f14f0 brackets: set immediately before its HP amount
 * call and cleared right after, never read outside that call. */
static struct { uint32_t active, skill, origin_side, origin_id, reflected; } bracket;

static int trace_enabled(void)
{
    static int trace = -1;
    if (trace < 0) {
        const char *value = getenv("P4PC_DAMAGE_TRACE");
        trace = value && value[0] && strcmp(value, "0") ? 1 : 0;
    }
    return trace;
}
static int bounded_owner(const char *owner)
{
    if (!owner || !owner[0]) return 0;
    for (size_t i = 0; owner[i]; ++i)
        if (i + 1u >= P4_DAMAGE_OWNER_SIZE || owner[i] < 0x21 || owner[i] > 0x7e) return 0;
    return 1;
}
/* Priority, then MULTIPLY before ADD, then load order, then registration. */
static int before(const Modifier *a, const Modifier *b)
{
    if (a->desc.priority != b->desc.priority) return a->desc.priority < b->desc.priority;
    if (a->desc.kind != b->desc.kind) return a->desc.kind == P4_DAMAGE_MULTIPLY;
    if (a->desc.load_order != b->desc.load_order) return a->desc.load_order < b->desc.load_order;
    return a->sequence < b->sequence;
}
int p4_game_damage_register(const P4GameDamageModifierDesc *desc, uint32_t *handle)
{
    if (!desc || !handle || !desc->modifier || !bounded_owner(desc->owner) ||
        (desc->kind != P4_DAMAGE_MULTIPLY && desc->kind != P4_DAMAGE_ADD)) return P4_DAMAGE_INVALID;
    if (composing) return P4_DAMAGE_BUSY;
    if (count == capacity) {
        const size_t grown = capacity ? capacity * 2u : 8u;
        Modifier *next = realloc(modifiers, grown * sizeof *next);
        if (!next) return P4_DAMAGE_NOMEM;
        modifiers = next; capacity = grown;
    }
    Modifier entry = {.desc = *desc, .handle = next_handle++, .sequence = next_sequence++};
    strcpy(entry.owner, desc->owner);
    entry.desc.owner = NULL;
    size_t at = count;
    while (at > 0 && before(&entry, &modifiers[at - 1u])) --at;
    memmove(&modifiers[at + 1u], &modifiers[at], (count - at) * sizeof *modifiers);
    modifiers[at] = entry;
    ++count;
    *handle = entry.handle;
    return P4_DAMAGE_OK;
}
int p4_game_damage_unregister(uint32_t handle)
{
    if (composing) return P4_DAMAGE_BUSY;
    for (size_t i = 0; i < count; ++i) {
        if (modifiers[i].handle != handle) continue;
        memmove(&modifiers[i], &modifiers[i + 1u], (count - i - 1u) * sizeof *modifiers);
        if (!--count) { free(modifiers); modifiers = NULL; capacity = 0; }
        return P4_DAMAGE_OK;
    }
    return P4_DAMAGE_NOT_FOUND;
}
size_t p4_game_damage_count(void) { return count; }

static const char *side_name(uint32_t side) { return side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party"; }
static const char *hit_name(uint32_t kind)
{
    return kind == P4_DAMAGE_HIT_CRITICAL ? "critical" : kind == P4_DAMAGE_HIT_WEAK ? "weak" :
        kind == P4_DAMAGE_HIT_NORMAL ? "normal" : "other";
}
int32_t p4_game_damage_apply(const P4GameDamageHit *hit)
{
    if (!hit) return P4_DAMAGE_MIN;
    if (composing) {
        /* A modifier re-entered the amount equation: the inner estimate keeps
         * its stock value rather than composing modifiers inside modifiers. */
        fprintf(stderr, "p4pc: damage stage: nested hit for skill 0x%x left at its stock value\n",
            (unsigned)hit->skill_id);
        return hit->stock;
    }
    const int trace = trace_enabled();
    if (trace)
        fprintf(stderr, "p4pc: damage stage: skill 0x%x %s:%u -> %s:%u (origin %s:%u%s) %s hit, element %s%u, %s, stock %d\n",
            (unsigned)hit->skill_id, side_name(hit->source_side), (unsigned)hit->source_id,
            side_name(hit->target_side), (unsigned)hit->target_id, side_name(hit->origin_side),
            (unsigned)hit->origin_id, hit->reflected ? ", reflected" : "", hit_name(hit->hit_kind),
            hit->element_valid ? "" : "none ", (unsigned)hit->element_id,
            hit->resolved ? "resolved" : "estimate", (int)hit->stock);
    composing = 1;
    double value = (double)hit->stock;
    for (size_t i = 0; i < count; ++i) {
        Modifier *m = &modifiers[i];
        double step = 0.0;
        if (!m->desc.modifier(m->desc.user, hit, &step)) continue;
        if (!isfinite(step) || (m->desc.kind == P4_DAMAGE_MULTIPLY && step < 0.0)) {
            if (!m->reported) {
                fprintf(stderr, "p4pc: damage stage: %s returned an invalid %s; ignored for that hit\n",
                    m->owner, m->desc.kind == P4_DAMAGE_MULTIPLY ? "factor" : "addend");
                m->reported = 1u;
            }
            continue;
        }
        const double prior = value;
        value = m->desc.kind == P4_DAMAGE_MULTIPLY ? value * step : value + step;
        /* Wide bounded intermediate: stays finite whatever the step count. */
        if (value > 1e18) value = 1e18;
        else if (value < -1e18) value = -1e18;
        if (trace)
            fprintf(stderr, "p4pc: damage stage:   %s priority %d load %u %s %.17g: %.17g -> %.17g\n",
                m->owner, (int)m->desc.priority, (unsigned)m->desc.load_order,
                m->desc.kind == P4_DAMAGE_MULTIPLY ? "multiply" : "add", step, prior, value);
    }
    composing = 0;
    const int32_t result = value < (double)P4_DAMAGE_MIN ? P4_DAMAGE_MIN :
        value > (double)P4_DAMAGE_MAX ? P4_DAMAGE_MAX : (int32_t)value;
    if (trace) fprintf(stderr, "p4pc: damage stage:   final %d\n", (int)result);
    return result;
}

int p4_game_damage_native_active(void) { return count != 0; }
void p4_game_damage_native_hit_begin(uint32_t skill, uint32_t origin_enemy, uint32_t origin_id, uint32_t reflected)
{
    bracket.active = 1u; bracket.skill = skill;
    bracket.origin_side = origin_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY;
    bracket.origin_id = origin_id; bracket.reflected = reflected ? 1u : 0u;
}
void p4_game_damage_native_hit_end(void) { memset(&bracket, 0, sizeof bracket); }
int32_t p4_game_damage_native_amount(uint32_t skill, uint32_t source_enemy, uint32_t source_id,
    uint32_t source_status, uint32_t target_enemy, uint32_t target_id, uint32_t target_status,
    uint32_t hit_kind, uint32_t element_valid, uint32_t element_id, int32_t stock)
{
    const int resolved = bracket.active && bracket.skill == skill;
    P4GameDamageHit hit = {
        .skill_id = skill,
        .source_side = source_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .source_id = source_id,
        .source_status = source_status,
        .target_side = target_enemy ? P4_DAMAGE_SIDE_ENEMY : P4_DAMAGE_SIDE_PARTY, .target_id = target_id,
        .target_status = target_status,
        .hit_kind = hit_kind, .element_valid = element_valid ? 1u : 0u, .element_id = element_id,
        .resolved = resolved ? 1u : 0u, .stock = stock,
    };
    hit.origin_side = resolved ? bracket.origin_side : hit.target_side;
    hit.origin_id = resolved ? bracket.origin_id : hit.target_id;
    hit.reflected = resolved ? bracket.reflected : 0u;
    return p4_game_damage_apply(&hit);
}
