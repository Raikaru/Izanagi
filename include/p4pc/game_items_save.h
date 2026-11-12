#ifndef P4PC_GAME_ITEMS_SAVE_H
#define P4PC_GAME_ITEMS_SAVE_H
#include "p4pc/game_items.h"
#include "p4pc/game_table_sources.h"
typedef struct P4GameItemsSave P4GameItemsSave;
/* Exact CPU-sealed views may register before native publication. Transactions
 * refuse until those same views are materialized and their source ledger is
 * frozen. No rows means OK/NULL, without allocating or registering a provider.
 * Both views outlive successful unregister AND actual native-user quiescence;
 * unregister alone never proves image/task lifetime. */
int p4_game_tables_items_save_register(P4GameItems *items,
    const P4GameTableSources *sources, P4GameItemsSave **out);
/* Refuses an active save transaction. On failure the handle remains owned/live. */
int p4_game_tables_items_save_unregister(P4GameItemsSave *save);
#endif
