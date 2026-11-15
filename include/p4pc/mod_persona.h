/* Read-only views of Yu's Persona stock and the Social Link ranks for mods
 * (p4.persona.stock, p4.social.rank; docs/modding/lua.md "Persona stock and
 * Social Links"). Every read copies scalars out of the game's own records;
 * nothing returns or keeps a guest pointer, and nothing writes. Game thread. */
#ifndef P4PC_MOD_PERSONA_H
#define P4PC_MOD_PERSONA_H

#include <stdint.h>

/* The stock is at most twelve slots (datPersonaRosterCapacity 0x0010B5B0). */
#define P4_MOD_STOCK_MAX 12

typedef struct P4ModStockEntry {
    uint8_t slot;        /* 0-based roster slot */
    uint8_t lead;        /* 1 for the equipped (lead) Persona */
    uint16_t persona;    /* Persona ID */
    uint8_t arcana;      /* datPersonaGetArcana, the game's own arcana number */
    uint8_t level;       /* 1..99 */
    uint32_t exp;
    uint8_t stats[5];    /* St Ma En Ag Lu, the record's own values */
    uint16_t skills[8];  /* the eight slots as stored, zeros included */
    uint8_t registered;  /* 1 when the compendium holds this Persona */
    uint8_t registered_level; /* the compendium copy's level, 0 when unregistered */
} P4ModStockEntry;

/* Fills up to capacity entries in slot order; returns the count (0..12). */
int p4_mod_persona_stock(P4ModStockEntry *out, int capacity);
/* The rank (0..10) of the Social Link of this arcana, as
 * cmmCommunityValueForArcana (0x00107890) answers: 0 when it has no record. */
int p4_mod_social_rank(int arcana);

#endif
