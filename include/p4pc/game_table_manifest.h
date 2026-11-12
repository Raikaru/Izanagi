#ifndef P4PC_GAME_TABLE_MANIFEST_H
#define P4PC_GAME_TABLE_MANIFEST_H
#include "p4pc/packages.h"
#include "p4pc/table_result.h"
/* Borrowed manifest metadata only, NOT a selected provider, file plan, source
 * origin, parsed row/ID claim or native capability. Caller retains fields.
 * owner is the explicit declaration namespace; actual origin is independently
 * captured from the selected provider during startup, never inferred here. */
typedef struct P4GameTableSourceMetadata {
    const char *key, *file;
    char owner[64];
    uint32_t schema, revision;
} P4GameTableSourceMetadata;
/* Pure/no allocation/no boot globals/no I/O. Same field/schema/key validation
 * as startup; only success replaces out. File containment/index existence and
 * XML payload/native dependencies remain their actual startup authorities. */
int p4_game_tables_source_metadata(const P4PackageField *fields, size_t count,
    P4GameTableSourceMetadata *out, char *error, size_t error_capacity);
#endif
