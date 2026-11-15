#ifndef P4PC_GAME_BUFF_STAGE_H
#define P4PC_GAME_BUFF_STAGE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_damage_stage.h"
/* Buff stage (docs/modding/tables.md "Buff stage"): one ordered composition
 * of registered modifiers per stat multiplier datCalcBuffMultiplier
 * 0x00233BC0 returns, after the native value: the side's rank percentage
 * (rank -1..1), Charge or Concentrate (x2.5) on the attack or magic an attack
 * consumes, and Lead boost. Its six callers are the damage equation (attack
 * or magic of the source, defense of the target) and the hit chance
 * (accuracy of the source, evasion of the target), estimates included. With
 * no modifier registered the function is retail and the stage never runs. */
#define P4_BUFF_ATTACK 0u   /* physical attack; Charge */
#define P4_BUFF_MAGIC 1u    /* magic attack; Concentrate */
#define P4_BUFF_EVASION 2u
#define P4_BUFF_DEFENSE 3u  /* damage taken */
#define P4_BUFF_ACCURACY 4u
#define P4_BUFF_MAX 100.0
typedef enum P4GameBuffKind { P4_BUFF_MULTIPLY = 0, P4_BUFF_ADD = 1 } P4GameBuffKind;
/* One multiplier: the unit, the stat, its rank, whether this attack's Charge
 * or Concentrate is up, and the native multiplier. */
typedef struct P4GameBuffQuery {
    uint32_t side, id;
    uint32_t stat;
    int32_t rank;
    uint32_t charged;
    double stock;
} P4GameBuffQuery;
typedef int (*P4GameBuffModifier)(void *user, const P4GameBuffQuery *query, double *value);
typedef struct P4GameBuffModifierDesc {
    const char *owner;
    int32_t priority;
    uint32_t load_order;
    P4GameBuffKind kind;
    P4GameBuffModifier modifier;
    void *user;
} P4GameBuffModifierDesc;
/* Same status codes as the damage stage (P4_DAMAGE_OK ... NOT_FOUND). */
int p4_game_buff_register(const P4GameBuffModifierDesc *desc, uint32_t *handle);
int p4_game_buff_unregister(uint32_t handle);
size_t p4_game_buff_count(void);
/* Composes in double, clamps to 0..P4_BUFF_MAX and converts once to the
 * native single precision. P4PC_BUFF_TRACE=1 traces each query. */
float p4_game_buff_apply(const P4GameBuffQuery *query);
/* "attack", "magic", "evasion", "defense", "accuracy", or NULL. */
const char *p4_game_buff_stat_name(uint32_t stat);
#endif
