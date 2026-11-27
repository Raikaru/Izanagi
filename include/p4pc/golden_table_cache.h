/* Trusted Golden-mode cache admission into the game-table builder. */
#ifndef P4PC_GOLDEN_TABLE_CACHE_H
#define P4PC_GOLDEN_TABLE_CACHE_H
#include "p4pc/game_table_builder.h"
#include "p4pc/golden_cache.h"
/* Loads the complete, verified cache entry into an unsealed builder. The cache
 * contains the nine p4_game_table_input_label() values and every name in extra.
 * Extra names must be unique and sorted. On success extra receives allocated
 * non-native tables owned by the caller; release them with p4_golden_cache_free.
 * On failure extra data is NULL.
 * The builder owns its inputs until destruction. Valid only in Golden mode
 * before inputs are loaded or sealed; prior input requests are preserved. */
int p4_game_tables_builder_golden_cache(P4GameTablesDefinitionBuilder *builder,
    const P4GoldenCachePaths *paths, const P4GoldenCacheIdentity *identity,
    P4GoldenCacheTable *extra, size_t extra_count, char *reason, size_t capacity);
#endif
