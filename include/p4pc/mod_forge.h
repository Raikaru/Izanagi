/* Weapon fusion (forge) transaction for mods (p4.forge, docs/modding/lua.md
 * "Weapon fusion transaction"): consume one stock Persona, one unequipped
 * vessel weapon and a fee, and add one weapon item, all or nothing.
 * A request is validated whole and queued; the next frame safe point outside
 * battle asks the weapon_fusion authorizer (stage COMMIT), revalidates against
 * the state as it is then, and commits or refuses. Game thread only. */
#ifndef P4PC_MOD_FORGE_H
#define P4PC_MOD_FORGE_H

#include <stddef.h>
#include <stdint.h>

typedef struct P4ModForge {
    uint8_t character;  /* party character ID: 1 Yu, 2 Yosuke, 3 Chie, 4 Yukiko, 6 Kanji, 7 Naoto, 8 Teddie */
    uint8_t signature;  /* passed to the authorizer; no other meaning to the core */
    uint16_t persona;   /* a carried, non-lead Persona */
    uint16_t vessel;    /* a weapon the character may equip, at least one unequipped copy */
    uint16_t result;    /* a registered weapon the character may equip, fewer than 99 owned */
    uint32_t fee;       /* yen, at most the money held */
} P4ModForge;

/* Outcome of the owner's last applied request: FORGED, or REFUSED by an
 * authorizer or a recruitment change (reason describes the refusal). */
enum { P4_MOD_FORGE_NONE = 0, P4_MOD_FORGE_FORGED = 1, P4_MOD_FORGE_REFUSED = 2 };

/* Validates now and queues for the next frame safe point outside battle.
 * 0, or -1 with a reason; a refusal queues nothing. One request at a time. */
int p4_mod_forge_queue(size_t owner, const P4ModForge *forge, char *reason, size_t capacity);
/* The unequipped count of item when it is a vessel character could give
 * (capability forge.vessel@1): character fights, has joined, and item passes
 * the vessel check queue makes (a known item row, weapon band, the character's
 * permission bit); 0 otherwise. The item's display name plays no part, as in queue. A
 * read: it queues and changes nothing. */
uint32_t p4_mod_forge_vessel_count(int character, uint32_t item);
/* The safe point's step. A recruitment change refuses without spending; other
 * requests that no longer fit the state are the owner's mod failure, never a
 * partial write. Nonzero while waiting for a battle to end. */
int p4_mod_forge_apply_pending(void);
/* Takes the owner's last outcome (P4_MOD_FORGE_*), clearing it; reason gets the
 * refusal text. NONE while nothing has completed since the last take. */
int p4_mod_forge_take_result(size_t owner, P4ModForge *forge, char *reason, size_t capacity);
/* 1 while the owner has a request queued. */
int p4_mod_forge_pending(size_t owner);
/* An owner's VM teardown drops its queued request and outcome. */
void p4_mod_forge_clear_owner(size_t owner);

#endif
