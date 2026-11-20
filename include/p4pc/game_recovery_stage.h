#ifndef P4PC_GAME_RECOVERY_STAGE_H
#define P4PC_GAME_RECOVERY_STAGE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_damage_stage.h"
/* Recovery stage (docs/modding/tables.md "Recovery stage"): one ordered
 * composition of registered modifiers per eligible HP heal, at the end of
 * datCalcSkillAmount 0x00235520. Eligible: the formula heal (HP effect kind
 * 2, the Dia and Media lines) after Divine Grace and the 95..105% variance,
 * on an outcome that delivers it. Fixed, percentage and full heals, revives,
 * drains, items and SP never reach it. With no modifier registered the
 * equation is retail and the stage never runs. Sides and origins use the
 * damage stage's values; the order, single conversion and refusals are the
 * damage stage's too, in a registry of their own. */
#define P4_RECOVERY_MIN 1
#define P4_RECOVERY_MAX 32550
typedef enum P4GameRecoveryKind { P4_RECOVERY_MULTIPLY = 0, P4_RECOVERY_ADD = 1 } P4GameRecoveryKind;
/* origin is a P4_DAMAGE_ORIGIN_* value from 001f14f0's bracket (unknown for
 * an estimate). source_persona/source_persona_level as in P4GameDamageHit. */
typedef struct P4GameRecoveryHit {
    uint32_t skill_id;
    uint32_t source_side, source_id;
    uint32_t target_side, target_id;
    uint32_t resolved, origin;
    int32_t stock;
    uint32_t source_persona, source_persona_level;
} P4GameRecoveryHit;
typedef int (*P4GameRecoveryModifier)(void *user, const P4GameRecoveryHit *hit, double *value);
typedef struct P4GameRecoveryModifierDesc {
    const char *owner;
    int32_t priority;
    uint32_t load_order;
    P4GameRecoveryKind kind;
    P4GameRecoveryModifier modifier;
    void *user;
} P4GameRecoveryModifierDesc;
/* Same status codes as the damage stage (P4_DAMAGE_OK ... NOT_FOUND). */
int p4_game_recovery_register(const P4GameRecoveryModifierDesc *desc, uint32_t *handle);
int p4_game_recovery_unregister(uint32_t handle);
size_t p4_game_recovery_count(void);
/* Composes in double, converts once by truncation toward zero, clamps to
 * P4_RECOVERY_MIN..P4_RECOVERY_MAX. P4PC_RECOVERY_TRACE=1 traces each heal. */
int32_t p4_game_recovery_apply(const P4GameRecoveryHit *hit);
#endif
