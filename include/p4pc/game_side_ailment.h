/* Battle-scoped side ailments (docs/modding/tables.md "Side ailments"): status
 * leaves a table source declares with <status-ailment> rows, held per live
 * DatUnit outside DatUnit.bad. A battle begins at btlInit (00192B90),
 * a unit's state ends when its action exits (001B05D0) and everything ends at
 * battle release (00192CD0) or table-session end; nothing is saved. With no row
 * declared nothing is registered and every call is inert. Units are guest
 * DatUnit addresses. Main-thread battle code only. */
#ifndef P4PC_GAME_SIDE_AILMENT_H
#define P4PC_GAME_SIDE_AILMENT_H
#include "p4pc/game_tables.h"

/* The published definitions, in leaf order. */
size_t p4_side_ailment_count(void);
const P4SideAilmentDef *p4_side_ailment_at(size_t index);
const P4SideAilmentDef *p4_side_ailment_find_leaf(uint32_t leaf);
const P4SideAilmentDef *p4_side_ailment_find_key(const char *key);

/* Observer reasons. LANDED carries RIDER, MOD or TRANSFER; TURN_LOST carries NONE. */
enum {
    P4_SIDE_EVENT_LANDED = 1, P4_SIDE_EVENT_CLEARED = 2, P4_SIDE_EVENT_TURN_LOST = 3
};
enum {
    P4_SIDE_REASON_NONE = 0, P4_SIDE_REASON_RIDER = 1, P4_SIDE_REASON_CURE = 2, P4_SIDE_REASON_MOD = 3,
    P4_SIDE_REASON_RECOVERED = 4, P4_SIDE_REASON_DEFEAT = 5, P4_SIDE_REASON_BATTLE_END = 6,
    P4_SIDE_REASON_RESET = 7, P4_SIDE_REASON_BROKEN = 8, P4_SIDE_REASON_TRANSFER = 9
};

/* 1 and the unit's definition while it holds one, else 0 (also outside battle). */
int p4_side_ailment_query(uint32_t unit, const P4SideAilmentDef **def);
/* Lands leaf on unit (reason RIDER or MOD) and counts one landing. P4_TABLE_*:
 * NOT_READY outside battle or during observer delivery, NOT_FOUND for an
 * unknown leaf, INVALID for a dead unit (0x80000), CONFLICT when the unit holds
 * a side ailment or, for a single-slot leaf, any retail ailment (bad & 0xFFFFF,
 * the 002397D0 test). Down is allowed. */
int p4_side_ailment_set(uint32_t unit, uint32_t leaf, uint32_t reason);
/* Clears the unit's side ailment for reason and returns the cleared leaf, or
 * P4_SIDE_DEFAULT_NONE when it held none. */
uint32_t p4_side_ailment_clear(uint32_t unit, uint32_t reason);
/* Landings of leaf on unit this battle (the P4_SIDE_SMTV_DECAY count). */
uint32_t p4_side_ailment_landings(uint32_t unit, uint32_t leaf);
/* The unit's lost turn (P4_SIDE_LOSE_TURN): reports TURN_LOST; returns 1 when
 * it holds a lose-turn ailment, else 0. Recovery is clear(RECOVERED). */
int p4_side_ailment_turn_lost(uint32_t unit);

/* Per action, before 001ACB10 commits: a landing the action's later hits must
 * see. mark_landed refuses like set except for the landing count. */
int p4_side_ailment_mark_landed(uint32_t unit, uint32_t leaf);
int p4_side_ailment_landed(uint32_t unit, uint32_t *leaf);
void p4_side_ailment_action_end(void);

/* One sink (PortMods binds it). Delivery is synchronous; set/clear/mark from
 * inside it refuse NOT_READY. NULL removes it. */
typedef void (*P4SideAilmentObserver)(uint32_t unit, uint32_t leaf, uint32_t event, uint32_t reason);
void p4_side_ailment_set_observer(P4SideAilmentObserver observer);

/* A battle is running with published definitions and the gate enabled
 * (set/mark can succeed). */
int p4_side_ailment_active(void);
/* The per-run gate, off by default and at session end: a declaring package
 * enables it for a run that uses its side ailments; while off every reader
 * is inert and set/mark refuse NOT_READY. Refused (NOT_READY) inside a battle. */
int p4_side_ailment_set_enabled(int on);
/* A lock-guard window on one unit for this battle: riders skip it before any
 * draw and set/mark refuse it INVALID, like a dead unit. Refused outside a
 * battle or with the gate off; cleared at unit exit, battle begin/end and
 * session end. */
int p4_side_ailment_protect(uint32_t unit, int on);
int p4_side_ailment_protected(uint32_t unit);
/* Riders (<status-rider> rows) of skill_id: 1 and the published index range
 * [*first, *first + *count) in leaf order, else 0. rider_at reads one;
 * use_rider authorizes it before any field past skill_id is read. */
int p4_side_ailment_riders(uint32_t skill_id, size_t *first, size_t *count);
const P4SideRiderDesc *p4_side_ailment_rider_at(size_t index);
void p4_side_ailment_use_rider(const P4SideRiderDesc *rider);
/* Table-source boot (game_table_boot.c): prepare with the sealed snapshot,
 * publish at runtime publication, end with the table session. */
int p4_game_side_ailment_prepare(const P4GameTables *tables);
void p4_game_side_ailment_publish(void);
void p4_game_side_ailment_session_end(void);
#endif
