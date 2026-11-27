#ifndef P4_GOLDEN_MODE_OUTPUTS_H
#define P4_GOLDEN_MODE_OUTPUTS_H
#include "golden_mode_projection.h"
#include "golden_mode_identity.h"
#include "golden_mode_image.h"
#include "p4pc/golden_extra_source.h"
#include "p4pc/golden_cache.h"

/* Complete index with explicitly absent unapproved entries. Initialize to
 * zero before prepare and free before reuse. Source/projection/artifacts are
 * borrowed through publication; packed files and skill aux are owned. */
#define P4_GOLDEN_OUTPUT_NATIVE_COUNT 9u
#define P4_GOLDEN_OUTPUT_EXTRA_COUNT (25u + P4_GOLDEN_IMAGE_EXTRA_COUNT)
typedef struct P4GoldenModeOutputs {
    P4GoldenCacheTable tables[P4_GOLDEN_OUTPUT_NATIVE_COUNT + P4_GOLDEN_OUTPUT_EXTRA_COUNT];
    P4GoldenCacheTable extra[P4_GOLDEN_OUTPUT_EXTRA_COUNT];
    uint8_t *owned[15];
    size_t table_count, extra_count;
} P4GoldenModeOutputs;
int p4_golden_mode_outputs_prepare(P4GoldenModeOutputs *, const P4GoldenDomainMap *,
    const P4GoldenSource *, const P4GoldenExtraSource *, const P4GoldenModeProjection *,
    const P4GoldenModeIdentityArtifacts *, char *, size_t);
/* A later approved adapter may fill a named absent entry; it owns the bytes. */
int p4_golden_mode_outputs_set(P4GoldenModeOutputs *, const char *, uint8_t *, size_t);
void p4_golden_mode_outputs_free(P4GoldenModeOutputs *);
#endif
