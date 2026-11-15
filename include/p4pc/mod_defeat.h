/* Battle defeat handler (p4.battle.defeat, capability battle.defeat@1).
 *
 * A field battle that the party loses ends in FldEvent_FieldToBattle
 * (0x00172e00) state 12, which starts the Game Over procedure when the field
 * party's leader is dead. Before that state runs, the one registered handler
 * answers the defeat: let the Game Over run, hold it one more frame, or retry
 * the battle from its start. A retry copies back the battle-start snapshot
 * taken when btlTaskCreate (0x00192e90) was called with that field event's
 * setup (every playable character's DatUnit record and the setup's enemy
 * groups) and re-creates the battle from the same setup. The operation
 * authorizers of "retry" (include/p4pc/mod_authorize.h) are asked once per
 * defeat; a refused retry is never performed.
 *
 * Game thread only. The seams are native observers created with the first
 * handler and enabled only while an enabled handler exists. */
#ifndef P4PC_MOD_DEFEAT_H
#define P4PC_MOD_DEFEAT_H
#include <stddef.h>
#include <stdint.h>

enum { P4_MOD_DEFEAT_GAME_OVER = 0, P4_MOD_DEFEAT_HOLD = 1, P4_MOD_DEFEAT_RETRY = 2 };
/* collision: a shadow touched on the field (FldEventWork returnMode 0);
 * field: a field script's encounter request (00174680, returnMode 1). */
enum { P4_MOD_DEFEAT_COLLISION = 0, P4_MOD_DEFEAT_FIELD = 1 };
/* A hold longer than this many polls lets the Game Over run. */
#define P4_MOD_DEFEAT_HOLD_LIMIT 18000u

typedef struct P4ModDefeatMember {
    uint16_t id, hp, sp;
    uint32_t ailments;
} P4ModDefeatMember;
typedef struct P4ModDefeat {
    uint32_t encounter;
    int source;
    uint32_t polls;     /* calls for this defeat before this one */
    int allowed;        /* the retry authorizers allow a retry and a snapshot exists */
    const char *reason; /* why not, "" when allowed */
    int members;        /* the fighters' battle-start values a retry restores */
    P4ModDefeatMember party[4];
} P4ModDefeat;

/* Returns a P4_MOD_DEFEAT_* answer, or -1 for a callback failure (controlled
 * mod recovery). */
typedef int (*P4ModDefeatFn)(const P4ModDefeat *defeat, void *user);
typedef struct P4ModDefeatHandler P4ModDefeatHandler;
/* At most one handler; NULL with *reason for a second one, unavailable
 * seams or a call during the handler's own dispatch. */
P4ModDefeatHandler *p4_mod_defeat_add(size_t owner, P4ModDefeatFn callback, void *user, const char **reason);
int p4_mod_defeat_enable(P4ModDefeatHandler *handler, int enabled);
int p4_mod_defeat_remove(P4ModDefeatHandler *handler);
void p4_mod_defeat_remove_owner(size_t owner);

/* The seams' bodies, called by their native observers (and by the native
 * test): the battle-start snapshot for a btlTaskCreate setup address, and
 * the defeat check for a FldEvent_FieldToBattle task. The latter returns the
 * answer it applied, or -1 when the call was not a defeat. */
void p4_mod_defeat_battle_begin(uint32_t setup);
int p4_mod_defeat_field(uint32_t task);
#endif
