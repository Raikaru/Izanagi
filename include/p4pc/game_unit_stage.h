#ifndef P4PC_GAME_UNIT_STAGE_H
#define P4PC_GAME_UNIT_STAGE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_damage_stage.h"
#include "p4pc/game_unit_stage_native.h"
/* Unit stage (docs/modding/tables.md "Unit stage"): one ordered composition
 * of registered modifiers per unit value the game derives. Max HP and max SP
 * (datCalcMaxHp 0x00231F80, datCalcMaxSp 0x00232290) after the native table
 * value, HP/SP skills and accessories and the party's 999 cap; an enemy's
 * starting HP and SP (datUnitInit 0x00231AF0) from its table row, under the
 * same stat; and the five battle stats (datCalcGetBattleStat 0x00232C70,
 * which every battle formula reads) before Enervation's halving. With no
 * modifier registered the functions are retail and the stage never runs. */
#define P4_UNIT_PARTY_POOL_MAX 999   /* the party's HP/SP cap */
#define P4_UNIT_ENEMY_POOL_MAX 65535 /* an enemy's u16 HP/SP */
#define P4_UNIT_STAT_MIN 1
#define P4_UNIT_STAT_MAX 99
typedef enum P4GameUnitKind { P4_UNIT_MULTIPLY = 0, P4_UNIT_ADD = 1 } P4GameUnitKind;
/* One value: the unit's side and ID, which value (P4_UNIT_MAX_HP ...
 * P4_UNIT_LUCK), the unit's level and the native value. */
typedef struct P4GameUnitQuery {
    uint32_t side, id;
    uint32_t stat;
    uint32_t level;
    int32_t stock;
} P4GameUnitQuery;
typedef int (*P4GameUnitModifier)(void *user, const P4GameUnitQuery *query, double *value);
typedef struct P4GameUnitModifierDesc {
    const char *owner;
    int32_t priority;
    uint32_t load_order;
    P4GameUnitKind kind;
    P4GameUnitModifier modifier;
    void *user;
} P4GameUnitModifierDesc;
/* Same status codes as the damage stage (P4_DAMAGE_OK ... NOT_FOUND). */
int p4_game_unit_register(const P4GameUnitModifierDesc *desc, uint32_t *handle);
int p4_game_unit_unregister(uint32_t handle);
size_t p4_game_unit_count(void);
/* Composes in double and truncates once, then clamps: max HP 1.., max SP
 * 0.., to P4_UNIT_PARTY_POOL_MAX for the party and P4_UNIT_ENEMY_POOL_MAX
 * for an enemy; a battle stat to P4_UNIT_STAT_MIN..MAX.
 * P4PC_UNIT_TRACE=1 traces each value. */
int32_t p4_game_unit_apply(const P4GameUnitQuery *query);
/* "max_hp", "max_sp", "strength", "magic", "endurance", "agility", "luck", or NULL. */
const char *p4_game_unit_stat_name(uint32_t stat);
#endif
