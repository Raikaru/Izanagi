#ifndef P4PC_GAME_PERSONAS_SAVE_H
#define P4PC_GAME_PERSONAS_SAVE_H
#include "p4pc/game_personas.h"
#include "p4pc/game_table_sources.h"

/* REQUIRED save block p4pc/persona-compendium for a view that adds personas
 * (no added rows: OK without registering). The retail image keeps compendium
 * rows 0..255 (tag 16) and stock records with raw persona IDs; the block holds
 * the added personas' compendium rows and pins, by ledger reference, the ID of
 * every added persona the save uses (compendium row, active stock slot or the
 * staged persona record the image saves), so a
 * load whose package assigns another ID fails instead of reinterpreting
 * records. An absent block loads as an empty added compendium. The exact
 * CPU-sealed view and frozen ledger outlive successful unregister; transactions
 * refuse until that view is the published persona snapshot. */
int p4_game_personas_save_register(const P4GamePersonas *personas, const P4GameTableSources *sources);
/* Refuses an active save transaction. */
int p4_game_personas_save_unregister(void);
#endif
