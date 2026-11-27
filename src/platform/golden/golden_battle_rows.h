#ifndef P4_GOLDEN_BATTLE_ROWS_H
#define P4_GOLDEN_BATTLE_ROWS_H
#include "p4pc/game_table_builder.h"
#include "p4pc/golden_cache.h"
int p4_golden_battle_rows_capture(void *context, P4GameTablesDefinitionBuilder *builder,
    const P4GoldenCacheTable *extra, size_t count, char *reason, size_t capacity);
/* Called only after native consumers drain, or for an unused early capture. */
void p4_golden_battle_rows_reset(void);
#endif
