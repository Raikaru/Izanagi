#ifndef P4_GAME_KNOWLEDGE_RUNTIME_INTERNAL_H
#define P4_GAME_KNOWLEDGE_RUNTIME_INTERNAL_H
#include "p4pc/game_knowledge.h"
#include "p4pc/game_table_sources.h"

/* Gameplay variant: authorizes the exact issued alias and affinity row before
 * their semantic fields are consumed. CPU compilation and save codecs use the
 * pure public resolver and never manufacture actual-use history. */
int p4_game_knowledge_runtime_resolve(const P4GameKnowledge *state,
    uint32_t enemy, uint32_t native_profile, const P4GameActionElement *element,
    P4KnowledgeKey *out);

/* Save block over the exact sealed source ledger; both stay pinned until a
 * successful unregister. ready means registered for exactly these tables. */
int p4_game_knowledge_save_register(const P4GameTables *tables, const P4GameTableSources *sources);
int p4_game_knowledge_save_ready(const P4GameTables *tables);
int p4_game_knowledge_save_unregister(void);

/* Registers "element-knowledge" (requiring "element") after the save block. */
int p4_game_knowledge_native_boot_stage(const P4GameTables *tables);
/* Quiesced teardown only; frees the published and any prepared state. */
void p4_game_knowledge_native_discard(void);
/* Committed snapshot (NULL before commit) and its observations, which are NULL
 * when that snapshot has no enemy side rows. */
const P4GameTables *p4_game_knowledge_native_snapshot(void);
P4GameKnowledge *p4_game_knowledge_native_active(void);
#endif
