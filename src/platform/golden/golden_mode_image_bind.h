#ifndef P4_GOLDEN_MODE_IMAGE_BIND_H
#define P4_GOLDEN_MODE_IMAGE_BIND_H
#include "golden_mode_image.h"
#include "p4pc/game_table_builder.h"
/* Capture only the extras returned by the M5 cache loader, before source seal.
 * Retained bytes and native origin are bound to this exact builder until drain. */
int p4_golden_mode_image_capture(P4GameTablesDefinitionBuilder *,
    const P4GoldenCacheTable *, size_t, char *, size_t);
/* Called after the actual builder seal and ELF image load, before exposure. */
int p4_golden_mode_image_bind(P4GameMode, P4GameTablesDefinitionBuilder *, char *, size_t);
/* Pre-exposure rollback or explicit post-drain session teardown only. */
void p4_golden_mode_image_release(P4GameTablesDefinitionBuilder *);
int p4_golden_mode_image_ready(const P4GameTablesDefinitionBuilder *);
#endif
