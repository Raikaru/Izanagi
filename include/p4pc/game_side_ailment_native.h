#ifndef P4PC_GAME_SIDE_AILMENT_NATIVE_H
#define P4PC_GAME_SIDE_AILMENT_NATIVE_H
#include <stdint.h>
/* Scalar-only lifecycle boundary of the side-ailment service for the
 * translated battle units (game_side_ailment.h). Inert with no row declared. */
/* btlInit 0x00192B90: the battle, all of its encounter waves. */
void p4_game_side_ailment_native_battle_begin(void);
/* btlActionExitUpdate 0x001B05D0, before the unit is freed: its guest DatUnit. */
void p4_game_side_ailment_native_unit_exit(uint32_t unit);
/* btlRelease 0x00192CD0. */
void p4_game_side_ailment_native_battle_end(void);
/* Battle readers (hardmode-design §4.F; game_side_ailment_battle.c). Units are
 * the live host views of guest DatUnit records. Each returns before reading
 * the unit when no row is declared or no battle runs, so retail flow, RNG
 * draws and results are unchanged. */
/* The unit holds a side ailment or one landed on it this action: the single
 * slot retail tests as DatUnit.bad low bits (002397D0, 00242360). */
int p4_side_ailment_native_occupied(void *unit);
/* The unit holds a side ailment that costs its next own turn: its turn takes
 * the bad state (001F6770), 001A31E0 spends it and it cannot evade (0023E6F0). */
int p4_side_ailment_native_lose_turn(void *unit);
/* 002411A0 physical critical roll: the held ailment's crit bonus, 0 for a
 * resisted hit (affinity word with P4_AFF_RESIST) or without one. */
int32_t p4_side_ailment_native_crit_bonus(void *target, uint32_t affinity);
/* 001F14F0 after a hit's retail statuses: a landed hit (outcome 1, any
 * weak/critical class) rolls each rider of skill_id against target; a
 * success marks the landing for 001F3010 to commit. Only the real resolution
 * calls it, never 002397D0's evaluation callers (00239F50, 0023A1E0). */
void p4_side_ailment_native_riders(uint32_t skill_id, void *source, void *target,
    uint32_t outcome, uint32_t retail_statuses, int32_t options);
/* 001F3010 status apply of one hit packet: an ailment held before it breaks
 * (P4_SIDE_BREAK_ON_HIT) or may move to a physical attacker (transfer) when
 * hp_delta < 0; then this action's landing commits, and a held ailment whose
 * cure mask the removed retail statuses cover clears. source is the hit's
 * acting DatUnit, skill_id its skill. */
void p4_side_ailment_native_commit(void *unit, uint32_t removed_status, int32_t hp_delta,
    void *source, uint32_t skill_id);
/* A defeated unit loses every ailment (001F3010, 001F2EB0). */
void p4_side_ailment_native_defeat(void *unit);
/* 001A31E0 first visit: 1 when the turn is lost to a side ailment (reported). */
int p4_side_ailment_native_spend_turn(void *unit);
/* 001A31E0 end of that turn: the ailment recovers. */
void p4_side_ailment_native_turn_end(void *unit);
/* 001A06D0 turn start: landings marked by earlier actions are dropped. */
void p4_side_ailment_native_action_begin(void);
/* AI status queries: 1 when the unit holds a side ailment and mask asks for
 * every retail ailment (all of 0xFF), so "is it afflicted at all" sees a side
 * ailment while a query for particular retail ailments does not. */
int p4_side_ailment_native_covers(void *unit, uint32_t mask);
#endif
