#ifndef P4_GOLDEN_MODE_DECLARE_CACHED_H
#define P4_GOLDEN_MODE_DECLARE_CACHED_H
#include "golden_mode_cache_publish.h"
#include "p4pc/golden_domains.h"
/* Context points to the verified domain map. The named cache extras contain
 * complete port-indexed rows. Integer IDs and resource revisions are LE u32;
 * icon records are owner[64], key[96], revision u32; Persona records are
 * model_source u32 followed by the native metadata/growth/model/candidate/
 * affinity byte rows. decl/text is count u32 followed by table/id/role/size
 * u32 and size encoded bytes for each entry. No process pointers are serialized. */
int p4_golden_mode_declare_cached(void *context, P4GameTablesDefinitionBuilder *builder,
                                   const P4GoldenCacheTable *extra, size_t count,
                                   char *reason, size_t capacity);
#endif
