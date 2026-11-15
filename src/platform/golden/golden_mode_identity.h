#ifndef P4_GOLDEN_MODE_IDENTITY_H
#define P4_GOLDEN_MODE_IDENTITY_H
#include "p4pc/golden_cache.h"
#include "p4pc/golden_extra_source.h"
#include "p4pc/golden_fusion_source.h"
#include <stddef.h>
#include <stdint.h>
#define P4_GOLDEN_MODE_SOURCE_MAX 32u
#define P4_GOLDEN_MODE_CHAIN_MAX 256u

typedef struct P4GoldenModeIdentitySource {
    const char *chain;
    const char *sha256;
} P4GoldenModeIdentitySource;
typedef struct P4GoldenModeIdentity {
    P4GoldenCacheIdentity identity;
    P4GoldenCacheIdentityEntry sources[P4_GOLDEN_MODE_SOURCE_MAX];
    char names[P4_GOLDEN_MODE_SOURCE_MAX][P4_GOLDEN_MODE_CHAIN_MAX];
    char hashes[P4_GOLDEN_MODE_SOURCE_MAX][65];
} P4GoldenModeIdentity;
typedef struct P4GoldenModeIdentityArtifacts {
    uint8_t experience[396];
    uint8_t inheritance[P4_SKILL_MAX_ID + 1u];
    size_t inheritance_size;
    uint8_t rankup[480];
    P4GoldenFusionTables fusion;
} P4GoldenModeIdentityArtifacts;
/* Canonicalizes full inventory chains and verified source digests. The pinned
 * executable digest is returned by p4_golden_exp_read. Other digests must
 * describe the exact bytes checked by the corresponding source reader.
 * The result owns its strings and must not be copied after construction. */
int p4_golden_mode_identity_build(P4GoldenModeIdentity *out, const char exe_digest[65],
                                  const P4GoldenModeIdentitySource *sources, size_t count,
                                  char *reason, size_t capacity);
/* Only use source/extra returned by their successful pinned loaders. The
 * remaining pinned source readers are run here before identity construction.
 * out owns its backing strings and must remain at the same address. */
int p4_golden_mode_identity_load(P4GoldenModeIdentity *out, P4GoldenModeIdentityArtifacts *artifacts,
                                 const char *directory, const P4GoldenSource *source,
                                 const P4GoldenExtraSource *extra, const P4GoldenDomainMap *map,
                                 char *reason, size_t capacity);
#endif
