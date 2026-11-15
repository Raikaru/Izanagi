#ifndef P4_GOLDEN_MODE_DECLARE_BUILDER_H
#define P4_GOLDEN_MODE_DECLARE_BUILDER_H
#include "golden_mode_declarations.h"
#include "p4pc/game_table_builder.h"
/* Capture the exact verified cache key as a native declaration source. */
int p4_golden_mode_cache_origin(P4GameMode mode, P4GameTablesDefinitionBuilder *builder,
                                 uint32_t *source, char *reason, size_t capacity);
/* After verified cache admission, bind every expanded declaration to that
 * exact cache key and the builder's actual native source ledger. On any error
 * the caller discards the entire unexposed builder. */
int p4_golden_mode_declare_to_builder(P4GameMode mode, P4GameTablesDefinitionBuilder *builder,
                                      const P4GoldenDomainMap *map, const P4GoldenModeDeclarationInputs *inputs,
                                      char *reason, size_t capacity);
#endif
