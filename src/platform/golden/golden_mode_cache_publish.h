#ifndef P4_GOLDEN_MODE_CACHE_PUBLISH_H
#define P4_GOLDEN_MODE_CACHE_PUBLISH_H
#include "p4pc/golden_table_cache.h"

/* All tables are the complete sorted cache entry. Extra names are sorted and
 * identify the non-native entries in that same index. On cache miss, tables
 * provide checked converted bytes; on hit the cache supplies authoritative
 * bytes. declare must consume only the verified extras and transfer any bytes
 * needed after return into a session owner. A failure discards the builder. */
typedef int (*P4GoldenModeDeclareCached)(void *context, P4GameTablesDefinitionBuilder *builder,
                                         const P4GoldenCacheTable *extra, size_t count,
                                         char *reason, size_t capacity);
typedef struct P4GoldenModeCachePlan {
    const P4GoldenCachePaths *paths;
    const P4GoldenCacheIdentity *identity;
    const P4GoldenCacheTable *tables;
    size_t table_count;
    P4GoldenCacheTable *extra;
    size_t extra_count;
    P4GoldenModeDeclareCached declare;
    void *context;
} P4GoldenModeCachePlan;
int p4_golden_mode_cache_publish(const P4GoldenModeCachePlan *plan, P4GameTablesDefinitionBuilder *builder,
                                  char *reason, size_t capacity);
#endif
