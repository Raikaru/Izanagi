#ifndef P4PC_GOLDEN_AI_BOOT_H
#define P4PC_GOLDEN_AI_BOOT_H
#include "p4pc/game_table_boot.h"
#include "p4pc/game_mode.h"
#include "p4pc/golden_cache.h"
#include "p4pc/package_boot.h"
#include "p4pc/packages.h"
struct P4GoldenAiProduct;
/* Core-only R4 admission. PS2 is inactive; Golden keeps the independent M7a
 * widened-SKILL1 and M6 publication prerequisites in the adapter registry. */
int p4_golden_ai_adapter_register(P4GameMode mode);
/* Called only by the verified M5 cache publication callback. Copies are owned
 * by the source builder, not by the transient source/declaration buffers. */
int p4_golden_ai_cache_declare(void *context, P4GameTablesDefinitionBuilder *builder,
    const P4GoldenCacheTable *extra, size_t count, char *reason, size_t capacity);
const struct P4GoldenAiProduct *p4_golden_ai_product(const P4GameTablesDefinitionBuilder *builder);
int p4_golden_ai_product_release(const P4GameTablesDefinitionBuilder *builder);
extern const P4PackageHandler p4_golden_ai_boot_provider;
#endif
