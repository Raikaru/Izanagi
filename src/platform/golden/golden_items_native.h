#ifndef P4_GOLDEN_ITEMS_CACHE_H
#define P4_GOLDEN_ITEMS_CACHE_H
#include "golden_mode_cache_publish.h"
#include "p4pc/golden_items_native.h"
int p4_golden_items_capture(void *, P4GameTablesDefinitionBuilder *,
    const P4GoldenCacheTable *, size_t, char *, size_t);
#endif
