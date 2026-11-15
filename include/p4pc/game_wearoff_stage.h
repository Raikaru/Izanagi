#ifndef P4PC_GAME_WEAROFF_STAGE_H
#define P4PC_GAME_WEAROFF_STAGE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_damage_stage.h"
/* Ailment wear-off stage (docs/modding/tables.md "Ailment wear-off stage"):
 * one ordered composition of registered modifiers per recovery roll in
 * btlSoundSelectCureableStatuses 0x001F6930, the decision each unit's own
 * turn makes about the ailments it carries. Party members and enemies alike.
 * Eligible: confusion, fear, rage and exhaustion (limit 3 turns) and
 * enervation and silence (6; halved by skill 0x22F, 1 with 0x230), from the
 * unit's second turn with the ailment until the turn before the limit, which
 * cures without a roll. The native chance (func_00244f60) composes, then the
 * datCalcRandomBelow(100) compare draws exactly as retail. With no modifier
 * registered the function is retail and the stage never runs. Sides use the
 * damage stage's values. */
#define P4_WEAROFF_MIN 0
#define P4_WEAROFF_MAX 100
typedef enum P4GameWearoffKind { P4_WEAROFF_MULTIPLY = 0, P4_WEAROFF_ADD = 1 } P4GameWearoffKind;
/* One roll. status is the ailment's DatUnit.bad bit; level and luck are the
 * unit's own (datCalcGetLevel, datCalcGetUnitStat stat 4); turns counts the
 * unit's own turns with the ailment so far (the native counter); limit is
 * the turn count that cures it without a roll; stock is the native chance in
 * percent; afflictions counts the ailments that landed on the unit this
 * battle, this one included (game_afflictions_native.h). */
typedef struct P4GameWearoffRoll {
    uint32_t side, id;
    uint32_t status;
    uint32_t level, luck;
    uint32_t turns, limit;
    int32_t stock;
    uint32_t afflictions;
} P4GameWearoffRoll;
typedef int (*P4GameWearoffModifier)(void *user, const P4GameWearoffRoll *roll, double *value);
typedef struct P4GameWearoffModifierDesc {
    const char *owner;
    int32_t priority;
    uint32_t load_order;
    P4GameWearoffKind kind;
    P4GameWearoffModifier modifier;
    void *user;
} P4GameWearoffModifierDesc;
/* Same status codes as the damage stage (P4_DAMAGE_OK ... NOT_FOUND). */
int p4_game_wearoff_register(const P4GameWearoffModifierDesc *desc, uint32_t *handle);
int p4_game_wearoff_unregister(uint32_t handle);
size_t p4_game_wearoff_count(void);
/* Composes in double, converts once by truncation toward zero and clamps to
 * P4_WEAROFF_MIN..P4_WEAROFF_MAX: 0 never cures on this roll, 100 always
 * does. P4PC_WEAROFF_TRACE=1 traces each roll. */
int32_t p4_game_wearoff_apply(const P4GameWearoffRoll *roll);
/* "confusion", "fear", "rage", "exhaustion", "enervation", "silence", or NULL. */
const char *p4_game_wearoff_status_name(uint32_t status);
#endif
