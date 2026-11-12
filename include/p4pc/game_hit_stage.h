#ifndef P4PC_GAME_HIT_STAGE_H
#define P4PC_GAME_HIT_STAGE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_damage_stage.h"
#include "p4pc/game_hit_stage_native.h"
/* Hit stage (docs/modding/tables.md "Hit stage"): one ordered composition
 * of registered modifiers per rolled accuracy or critical chance. Accuracy:
 * datCalcResolveSkillHit 0x0023E6F0, after the native chance (the hit-rate
 * formula, evasion skills and equipment) and its 50..99 clamp, before the
 * RpRandom() % 100 compare; automatic hits never reach it. Critical:
 * datCalcHitResult 0x002411A0, after the native chance (the scaled-ratio
 * formula, the target's halving and the source's doubling skills, critical
 * equipment and buffs) and its
 * 0..100 clamp, before the compare; guaranteed criticals still land first.
 * The RNG draw count never changes. With no modifier registered the
 * functions are retail and the stage never runs. Sides use the damage
 * stage's values. */
#define P4_HIT_MIN 0
#define P4_HIT_MAX 100
typedef enum P4GameHitKind { P4_HIT_MULTIPLY = 0, P4_HIT_ADD = 1 } P4GameHitKind;
/* One rolled chance: which roll (P4_HIT_ACCURACY or P4_HIT_CRITICAL), the
 * skill, source and target, and the native chance in percent. */
typedef struct P4GameHitQuery {
    uint32_t roll;
    uint32_t skill_id;
    uint32_t source_side, source_id;
    uint32_t target_side, target_id;
    int32_t stock;
} P4GameHitQuery;
typedef int (*P4GameHitModifier)(void *user, const P4GameHitQuery *query, double *value);
typedef struct P4GameHitModifierDesc {
    const char *owner;
    int32_t priority;
    uint32_t load_order;
    P4GameHitKind kind;
    P4GameHitModifier modifier;
    void *user;
} P4GameHitModifierDesc;
/* Same status codes as the damage stage (P4_DAMAGE_OK ... NOT_FOUND). */
int p4_game_hit_register(const P4GameHitModifierDesc *desc, uint32_t *handle);
int p4_game_hit_unregister(uint32_t handle);
size_t p4_game_hit_count(void);
/* Composes in double, truncates once and clamps to P4_HIT_MIN..MAX (0 never
 * succeeds, 100 always does). P4PC_HIT_TRACE=1 traces each roll. */
int32_t p4_game_hit_apply(const P4GameHitQuery *query);
#endif
