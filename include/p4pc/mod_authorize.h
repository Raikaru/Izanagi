/* Operation authorizers (p4.authorize). A native seam asks every enabled
 * authorizer of one closed operation, in load order, before the game commits
 * the player's operation; the first denial wins and its reason is reported.
 * Seams test p4_mod_authorize_mask (include/p4pc/mod_entry.h) first, so with
 * no authorizer nothing else runs. */
#ifndef P4PC_MOD_AUTHORIZE_H
#define P4PC_MOD_AUTHORIZE_H
#include <stddef.h>
#include "p4pc/mod_events.h"

/* Closed operation registry; the bit is 1u << operation. */
enum P4ModOperation {
    P4_MOD_AUTH_FUSION = 0,    /* ctx: kind (fusion row id), materials */
    P4_MOD_AUTH_ENCOUNTER = 1, /* ctx: encounter, source */
    P4_MOD_AUTH_BATTLE_PROTECTION_ITEM = 2, /* ctx: character, item, element */
    P4_MOD_AUTH_WEAPON_FUSION = 3, /* ctx: stage, character, vessel, persona, signature */
    P4_MOD_AUTH_RETRY = 4,     /* ctx: encounter, source (include/p4pc/mod_defeat.h) */
    P4_MOD_AUTH_COUNT
};
/* weapon_fusion stages: OFFER before its Velvet command row is shown (vessel,
 * persona and signature 0), COMMIT inside the forge transaction before any
 * preflight, fee or consumption. */
enum { P4_MOD_WEAPON_FUSION_OFFER = 0, P4_MOD_WEAPON_FUSION_COMMIT = 1 };
typedef struct P4ModAuthorizer P4ModAuthorizer;
/* 1 allows, 0 denies (reason filled, NUL-terminated), -1 is a callback failure
 * that enters controlled mod recovery. The context holds copied scalars. */
typedef int (*P4ModAuthorizeFn)(const P4ModEvent *context, char *reason, size_t capacity, void *user);

const char *p4_mod_operation_name(int operation);
/* Game thread, outside authorization dispatch. NULL for an unknown op. */
P4ModAuthorizer *p4_mod_authorizer_add(size_t owner, const char *operation, P4ModAuthorizeFn callback, void *user);
int p4_mod_authorizer_enable(P4ModAuthorizer *authorizer, int enabled);
int p4_mod_authorizer_remove(P4ModAuthorizer *authorizer);
void p4_mod_authorizers_remove_owner(size_t owner);
/* After every VM/client released its handles, like p4_mod_events_clear. */
void p4_mod_authorizers_clear(void);
/* 1 allowed, 0 denied. A failing authorizer never authorizes: it enters
 * p4_mod_failure. The reason may be NULL/0 when the seam shows none. */
int p4_mod_authorize(int operation, const P4ModEvent *context, char *reason, size_t capacity);
/* 1 allowed, 0 denied with the first denier's reason in reason (may be NULL). */
int p4_mod_authorize_weapon_fusion(int stage, int character, int vessel, int persona, int signature,
    char *reason, size_t capacity);

/* Battle formation projector (p4.battle.formation): at most ONE at a time; a
 * second registration is a refused conflict. The battle's own lineup (playable
 * character IDs 1..10, battle order) goes in; the projector writes 1..4 unique
 * IDs and returns their count, 0 to keep the lineup, or -1 for a callback
 * failure (controlled mod recovery). */
typedef struct P4ModFormation P4ModFormation;
typedef int (*P4ModFormationFn)(const uint8_t *ids, int count, uint8_t *out, void *user);
P4ModFormation *p4_mod_formation_add(size_t owner, P4ModFormationFn callback, void *user);
int p4_mod_formation_enable(P4ModFormation *formation, int enabled);
int p4_mod_formation_remove(P4ModFormation *formation);
void p4_mod_formation_remove_owner(size_t owner);
#endif
