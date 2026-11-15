/* Pinned English Golden fusion archive inputs and whole-table projections. */
#ifndef P4PC_GOLDEN_FUSION_SOURCE_H
#define P4PC_GOLDEN_FUSION_SOURCE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct P4GoldenFusionSource {
    const char *member, *sha256;
} P4GoldenFusionSource;
typedef struct P4GoldenFusionTables {
    uint8_t limits[18], affinities[532], gates[208], charts[3][784];
    uint8_t recipes[16][32];
    unsigned recipe_counts[5], unsupported_twelfth;
} P4GoldenFusionTables;
/* Immutable pinned source registry in processing order, for the manifest. */
const P4GoldenFusionSource *p4_golden_fusion_sources(size_t *count);
/* Opens the owner's English archive, checks each source hash and converts all
 * supported rows. The 12-material rows are counted as unsupported, not
 * truncated. On failure the output is untouched. */
int p4_golden_fusion_load(const char *directory, const P4GoldenDomainMap *map,
                           P4GoldenFusionTables *out, char *reason, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
