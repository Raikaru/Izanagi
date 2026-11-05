#ifndef P4PC_GAME_DAMAGE_STAGE_H
#define P4PC_GAME_DAMAGE_STAGE_H
#include <stddef.h>
#include <stdint.h>
/* Final-damage stage (docs/modding/tables.md "Final damage stage"): one
 * ordered composition of registered modifiers per eligible HP-damage hit, at
 * the end of datCalcSkillAmount 0x00235520. Eligible: the retail non-fixed
 * HP equation (formula damage after attack/defense/status/passive terms,
 * critical, guard and the 95..105% variance) on a hit whose outcome delivers
 * damage. Fixed, percentage, scripted (hit kind 16) and SP amounts, misses,
 * nulls, drains and healing never reach it. With no modifier registered the
 * equation is retail and the stage never runs. */
#define P4_DAMAGE_SIDE_PARTY 0u
#define P4_DAMAGE_SIDE_ENEMY 1u
/* datCalcHitResult 0x002411A0 codes, as datCalcSkillAmount receives them. */
#define P4_DAMAGE_HIT_NORMAL 1u
#define P4_DAMAGE_HIT_CRITICAL 2u
#define P4_DAMAGE_HIT_WEAK 4u
/* The retail equation's own range: work is clamped to 1..31000 before the
 * 95..105% variance, and a zero result becomes 1. */
#define P4_DAMAGE_MIN 1
#define P4_DAMAGE_MAX 32550
#define P4_DAMAGE_OWNER_SIZE 64u
enum {
    P4_DAMAGE_OK = 0, P4_DAMAGE_INVALID = -1, P4_DAMAGE_BUSY = -2,
    P4_DAMAGE_NOMEM = -3, P4_DAMAGE_NOT_FOUND = -4
};
typedef enum P4GameDamageKind { P4_DAMAGE_MULTIPLY = 0, P4_DAMAGE_ADD = 1 } P4GameDamageKind;
/* One eligible hit. Sides and IDs are DatUnit facts (flags bit 4, id); the
 * status words are each unit's DatUnit.bad ailment word as the equation read
 * it, exact per unit even when a formation repeats an enemy ID. The origin is
 * the unit the hit was aimed at: it differs from the target only when a repel
 * delivered the hit back to its source. resolved is 1 for a hit 001f14f0 is
 * resolving and 0 for an estimate (AI target choice, previews); neither draws
 * RNG in the stage. */
typedef struct P4GameDamageHit {
    uint32_t skill_id;
    uint32_t source_side, source_id, source_status;
    uint32_t target_side, target_id, target_status;
    uint32_t origin_side, origin_id;
    uint32_t hit_kind;
    uint32_t element_valid, element_id;
    uint32_t reflected, resolved;
    int32_t stock;
} P4GameDamageHit;
/* Returns 1 with *value set (a factor for MULTIPLY, an addend for ADD) to
 * apply, 0 to leave the hit alone. A modifier must be a pure function of the
 * hit: no RNG, no game-state writes, no re-entry. A non-finite value, or a
 * negative factor, is ignored for that hit and reported once. */
typedef int (*P4GameDamageModifier)(void *user, const P4GameDamageHit *hit, double *value);
typedef struct P4GameDamageModifierDesc {
    const char *owner;      /* bounded owner (mod) name, for the trace */
    int32_t priority;       /* declared stage priority: lower runs first */
    uint32_t load_order;    /* the owning mod's load position */
    P4GameDamageKind kind;
    P4GameDamageModifier modifier;
    void *user;
} P4GameDamageModifierDesc;
/* Order: priority, then MULTIPLY before ADD, then load order, then
 * registration order. Registration is refused (BUSY) while a hit is being
 * composed. */
int p4_game_damage_register(const P4GameDamageModifierDesc *desc, uint32_t *handle);
int p4_game_damage_unregister(uint32_t handle);
size_t p4_game_damage_count(void);
/* Composes every modifier in order over the stock value in double precision,
 * then converts once: truncation toward zero, as the retail equation's own
 * integer conversion, clamped to P4_DAMAGE_MIN..P4_DAMAGE_MAX. P4PC_DAMAGE_TRACE=1
 * prints the hit, every applied step and the result to stderr. */
int32_t p4_game_damage_apply(const P4GameDamageHit *hit);
#endif
