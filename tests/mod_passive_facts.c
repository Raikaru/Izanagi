/* The game facts the real p4.passive binding and grant table read in the Luau
 * VM witnesses (mod_luau_test, mod_luau_save_test): skills below 440 are active
 * and the retail 576 are registered, the retail enemy bank, member 2's current
 * Persona 0x2a and accessory item 5 (any other member: Persona 0x30, nothing
 * equipped), and a loaded semantic catalog whose Personas are 0x10, 0x2a and
 * 0x30, items 5 and 6 and enemies 40 and 41. passive_facts_catalog_ready = 0
 * stands for the time before the catalog is loaded. */
#include "p4pc/game_data_catalog.h"
#include <stddef.h>
#include <stdint.h>

int passive_facts_catalog_ready = 1;
int p4_game_skill_registered(uint32_t id) { return id < 576u; }
int p4_game_skill_active(uint32_t id) { return id < 440u; }
uint32_t p4_game_enemies_native_limit(void) { return 0x150u; }
uint16_t datPersonaGetRecordId(uint16_t party) { return party == 2u ? 0x2au : 0x30u; }
int16_t datGetEquippedItemByUnit(int16_t party, int16_t slot) { return party == 2 && slot == 2 ? 5 : -1; }
int p4_data_catalog_ready(const char **reason)
{
    if (passive_facts_catalog_ready) return 0;
    if (reason) *reason = "the game has not loaded its tables yet";
    return -1;
}
int p4_data_catalog_index(P4DataKind kind, uint32_t id, size_t *index)
{
    if (!passive_facts_catalog_ready) return -1;
    int found = 0;
    switch (kind) {
    case P4_DATA_PERSONA: found = id == 0x10u || id == 0x2au || id == 0x30u; break;
    case P4_DATA_ITEM: found = id == 5u || id == 6u; break;
    case P4_DATA_ENEMY: found = id == 40u || id == 41u; break;
    default: break;
    }
    if (!found) return -1;
    if (index) *index = id;
    return 0;
}
