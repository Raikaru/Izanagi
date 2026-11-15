#ifndef P4PC_GAME_PASSIVE_GRANT_H
#define P4PC_GAME_PASSIVE_GRANT_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_passive_grant_native.h"
/* Passive grants (docs/modding/tables.md "Passive grants"): a native table of
 * (scope, key, skill) tuples, each held by one or more owners. A unit has a
 * granted passive exactly when the game's own passive checks would find it in
 * its skill slots: every check the game makes about whether a unit has a
 * passive asks the table after the slots and the accessory, so a skill a unit
 * already has, or that two owners grant, counts once. The ordinary eight-slot
 * list (skill menus, Analyze, fusion, learning, the AI's command lists) is
 * unchanged. Owners hold their tuples independently: a revocation removes only
 * the revoking owner's, and the result never depends on the order of grants.
 * Main-thread game state; nothing is saved. */
typedef enum P4PassiveScope {
    P4_PASSIVE_PARTY = 0,   /* key: party member, DatUnit ID 1 (protagonist) .. 8 (Teddie) */
    P4_PASSIVE_PERSONA = 1, /* key: Persona ID, in battle and in the protagonist's roster */
    P4_PASSIVE_ITEM = 2,    /* key: item ID, for whichever party member has it equipped */
    P4_PASSIVE_ENEMY = 3,   /* key: enemy ID, every enemy of that kind */
    P4_PASSIVE_SCOPES
} P4PassiveScope;
#define P4_PASSIVE_PARTY_MAX 8u
#define P4_PASSIVE_KEY_MAX 0xFFFFu
#define P4_PASSIVE_ITEM_MAX 0x7FFFu    /* datGetEquippedItemByUnit's non-negative s16 */
#define P4_PASSIVE_SKILL_MAX 4095u     /* P4_SKILL_MAX_ID */
#define P4_PASSIVE_GRANT_MAX 256u      /* tuples held by all owners together */
#define P4_PASSIVE_OWNER_SIZE 64u
enum {
    P4_PASSIVE_OK = 0,
    P4_PASSIVE_INVALID = -1, /* scope, key or owner out of range, or not a registered passive skill */
    P4_PASSIVE_FULL = -2,    /* P4_PASSIVE_GRANT_MAX reached */
};
typedef struct P4PassiveGrant {
    uint32_t scope, key, skill;
    uint32_t load_order;
    /* Borrowed from the table: valid only until the next grant, revocation
     * or teardown of any owner (each may move the tuples). Copy it to keep it. */
    const char *owner;
} P4PassiveGrant;
/* 1 when this owner newly holds the tuple, 0 when it already did, else a
 * P4_PASSIVE_* error. load_order identifies the owner; owner names it. The
 * skill must be registered and not active (p4_game_skill_registered,
 * p4_game_skill_active: the retail split, or an added skill once published),
 * so navigator and other active skills are refused. */
int p4_game_passive_grant(const char *owner, uint32_t load_order, uint32_t scope, uint32_t key, uint32_t skill);
/* 1 when this owner held the tuple, 0 when it did not, P4_PASSIVE_INVALID. */
int p4_game_passive_revoke(uint32_t load_order, uint32_t scope, uint32_t key, uint32_t skill);
/* Every tuple of one owner (VM teardown); returns how many. */
size_t p4_game_passive_revoke_owner(uint32_t load_order);
/* Whether any owner holds the tuple. */
int p4_game_passive_held(uint32_t scope, uint32_t key, uint32_t skill);
/* The held tuples in (scope, key, skill, load order) order. */
size_t p4_game_passive_count(void);
int p4_game_passive_at(size_t index, P4PassiveGrant *out);
/* What a unit sees: a party member through its member ID, its Persona and its
 * three equipped items; an enemy through its enemy ID. */
int p4_game_passive_party_has(uint32_t member, uint32_t persona, uint32_t skill);
int p4_game_passive_enemy_has(uint32_t enemy, uint32_t skill);
/* The granted skills one unit sees that are not already among its sources
 * (the slot list and the accessory's skill), each once in ascending order.
 * unit_id is the DatUnit ID: an enemy ID, or the party member. */
typedef int (*P4PassiveVisitFn)(void *user, uint32_t skill);
int p4_game_passive_unit_sources(uint32_t enemy, uint32_t unit_id, const uint8_t *skills, uint32_t skill_count,
    uint32_t accessory_skill, P4PassiveVisitFn visit, void *user);
#endif
