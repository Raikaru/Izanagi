#ifndef P4PC_GAME_SKILLS_SAVE_H
#define P4PC_GAME_SKILLS_SAVE_H
#include "p4pc/game_skills.h"
#include "p4pc/game_table_sources.h"

/* REQUIRED save block p4pc/expanded-skills for a view that adds skills (no
 * added rows: OK without registering). The retail image keeps raw u16 skill
 * IDs in persona records (DatPersonaRecord.skills, +0x0C): the twelve stock
 * slots (0x00797F8C), datStageRecord's staged record (0x0079B6A8), the party
 * members' personas (DatPartySlot 0x00796E50 + 0x54, party IDs 2..10) and the
 * compendium rows (p4_game_personas_native_compendium; an added persona's row
 * rides its own persona block with the same raw IDs). The block pins, by
 * ledger reference, the ID of every added skill those records hold, so a load
 * whose package assigns another ID, or lacks the skill, fails instead of
 * reinterpreting skill slots. It carries no other state: an absent block is a
 * save without added skills. The exact materialized view and frozen ledger
 * outlive successful unregister. */
int p4_game_skills_save_register(const P4GameSkills *skills, const P4GameTableSources *sources);
/* Refuses an active save transaction. */
int p4_game_skills_save_unregister(void);
#endif
