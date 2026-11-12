/* Persona stock and Social Link views for mods (include/p4pc/mod_persona.h).
 * The stock is the roster at 0x00797F8C (0x30-byte DatPersonaRecord: active bit
 * 0 of flags +0, ID +2, level +4, EXP +8, eight skill halfwords +0xC, five stat
 * bytes +0x1C); the lead slot index is the halfword at 0x00797F88. The
 * compendium copy is p4_game_personas_native_compendium's row with the same
 * layout. Reads only: no game function here writes or draws RNG. */
#include "p4pc/mod_persona.h"
#include "p4pc/game_personas_native.h"
#include "p4pc/guest.h"
#include <string.h>

extern int32_t datPersonaRosterCapacity(void);
extern uint8_t datPersonaGetArcana(uint16_t persona);
extern int32_t cmmCommunityValueForArcana(int32_t arcana);

enum { RECORD = 0x30, ID = 2, LEVEL = 4, EXP = 8, SKILLS = 0xC, STATS = 0x1C };
#define ROSTER 0x00797F8Cu
#define LEAD_SLOT 0x00797F88u

static uint16_t half(const uint8_t *p)
{
    uint16_t v;
    memcpy(&v, p, 2);
    return v;
}

int p4_mod_persona_stock(P4ModStockEntry *out, int capacity)
{
    if (!out || capacity <= 0) return 0;
    int slots = datPersonaRosterCapacity();
    if (slots > P4_MOD_STOCK_MAX) slots = P4_MOD_STOCK_MAX;
    int lead = (int16_t)half((const uint8_t *)P4_G2H(LEAD_SLOT));
    uint32_t limit = p4_game_personas_native_limit();
    int count = 0;
    for (int slot = 0; slot < slots && count < capacity; ++slot) {
        const uint8_t *record = (const uint8_t *)P4_G2H(ROSTER + (uint32_t)slot * RECORD);
        uint16_t persona = half(record + ID);
        if (!(half(record) & 1) || persona == 0 || persona >= limit) continue;
        P4ModStockEntry *e = &out[count++];
        memset(e, 0, sizeof *e);
        e->slot = (uint8_t)slot;
        e->lead = slot == lead;
        e->persona = persona;
        e->arcana = datPersonaGetArcana(persona);
        e->level = record[LEVEL];
        memcpy(&e->exp, record + EXP, 4);
        memcpy(e->stats, record + STATS, 5);
        memcpy(e->skills, record + SKILLS, sizeof e->skills);
        const uint8_t *row = (const uint8_t *)P4_G2H(p4_game_personas_native_compendium(persona));
        if ((half(row) & 1) && half(row + ID) == persona) {
            e->registered = 1;
            e->registered_level = row[LEVEL];
        }
    }
    return count;
}

int p4_mod_social_rank(int arcana)
{
    if (arcana < 0 || arcana > 0xFF) return 0;
    int rank = cmmCommunityValueForArcana(arcana);
    return rank < 0 ? 0 : rank > 10 ? 10 : rank;
}
