#ifndef P4PC_GAME_CHANCE_STAGE_H
#define P4PC_GAME_CHANCE_STAGE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_damage_stage.h"
/* Status/KO chance stage (docs/modding/tables.md "Status chance stage"): one
 * ordered composition of registered modifiers per rolled ailment or instant
 * KO attempt, in datCalcInflictStatus 0x00238940, after the native chance
 * (the ratio-of-products formula times the affinity, Fear-boost and passive
 * multipliers, truncated) and before the native 0..99 clamp and the
 * RpRandom() % 100 compare. Guard, immunity and null exits, skills whose
 * base rate is 100 or more and the scripted unconditional KO never reach it.
 * With no modifier registered the function is retail and the stage never
 * runs. Sides use the damage stage's values. */
#define P4_CHANCE_COMPONENT_AILMENT 1u
#define P4_CHANCE_COMPONENT_INSTANT_KO 2u
#define P4_CHANCE_MIN 0
#define P4_CHANCE_MAX 99
typedef enum P4GameChanceKind { P4_CHANCE_MULTIPLY = 0, P4_CHANCE_ADD = 1 } P4GameChanceKind;
/* statuses is the attempted status mask (0x80000 for an instant KO). item is
 * 1 for an item action (the call's options bit 0). source_persona and
 * source_persona_level are the party source's active persona record for a
 * non-item action, else 0. stock is the native truncated chance before the
 * clamp, which may lie outside 0..99. afflictions counts the ailments that
 * landed on the target earlier this battle (game_afflictions_native.h). */
typedef struct P4GameChanceHit {
    uint32_t skill_id;
    uint32_t source_side, source_id;
    uint32_t target_side, target_id;
    uint32_t component, statuses, item;
    int32_t stock;
    uint32_t source_persona, source_persona_level;
    uint32_t afflictions;
} P4GameChanceHit;
typedef int (*P4GameChanceModifier)(void *user, const P4GameChanceHit *hit, double *value);
typedef struct P4GameChanceModifierDesc {
    const char *owner;
    int32_t priority;
    uint32_t load_order;
    P4GameChanceKind kind;
    P4GameChanceModifier modifier;
    void *user;
} P4GameChanceModifierDesc;
/* Same status codes as the damage stage (P4_DAMAGE_OK ... NOT_FOUND). */
int p4_game_chance_register(const P4GameChanceModifierDesc *desc, uint32_t *handle);
int p4_game_chance_unregister(uint32_t handle);
size_t p4_game_chance_count(void);
/* Composes in double and converts once by truncation toward zero, as the
 * native chance conversion; the native clamp to 0..99 then applies.
 * P4PC_CHANCE_TRACE=1 traces each attempt. */
int32_t p4_game_chance_apply(const P4GameChanceHit *hit);
#endif
