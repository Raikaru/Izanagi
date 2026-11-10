/* Playable characters' combat progression for mods (p4.party, docs/modding/lua.md
 * "Party records"). Party ids 2..10 name the party slots' Persona records (the
 * character's level, EXP, five stats and eight skill slots); Yu (1) is not a
 * party record. Reads copy the record; a write is validated whole, queued and
 * applied at the frame safe point outside battle, and then keeps the
 * character's current HP/SP within the new maxima. Game thread only. */
#ifndef P4PC_MOD_PARTY_H
#define P4PC_MOD_PARTY_H

#include <stddef.h>
#include <stdint.h>

typedef struct P4ModPartyRecord {
    uint16_t persona;   /* the record's Persona ID; a write never changes it */
    uint8_t level;      /* 1..99 */
    uint32_t exp;       /* within the level's EXP band of this Persona's curve */
    uint8_t stats[5];   /* St Ma En Ag Lu, each 1..99 */
    uint16_t skills[8]; /* registered skill IDs first, unique, then zeros */
} P4ModPartyRecord;

/* 0, or -1 when id is not 2..10 or the slot has no Persona. */
int p4_mod_party_read(int id, P4ModPartyRecord *out);
/* Validates the whole record now and queues it for the next frame safe point
 * outside battle (p4_mod_safe_point). 0, or -1 with a reason; a refusal queues
 * nothing and the record stays byte-identical. */
int p4_mod_party_queue(size_t owner, int id, const P4ModPartyRecord *record, char *reason, size_t capacity);
/* The safe point's step: applies each queued write, revalidated against the
 * record as it is then (a misfit is the owner's mod failure, never a partial
 * write). Nonzero while a write waits for a battle to end. */
int p4_mod_party_apply_pending(void);
/* An owner's VM teardown drops its queued writes. */
void p4_mod_party_clear_owner(size_t owner);

#endif
