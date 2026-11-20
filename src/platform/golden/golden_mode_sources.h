#ifndef P4_GOLDEN_MODE_SOURCES_H
#define P4_GOLDEN_MODE_SOURCES_H
#include "golden_mode_identity.h"
#include "golden_mode_projection.h"
#include "golden_mode_cache_publish.h"
#include "golden_mode_outputs.h"

typedef struct P4GoldenModeSources {
    char directory[1024];
    P4GoldenDomainMap map;
    P4GoldenSource source;
    P4GoldenExtraSource extra;
    P4GoldenModeIdentity identity;
    P4GoldenModeIdentityArtifacts artifacts;
    P4GoldenModeProjection projection;
    P4GoldenModeOutputs outputs;
    P4GoldenCachePaths paths;
    char *preference_directory; /* SDL-owned until rollback/publication. */
    P4GoldenModeCachePlan cache;
    /* Adapter-owned converted buffers; NULL when no adapter allocated them. */
    void (*release_outputs)(struct P4GoldenModeSources *sources);
} P4GoldenModeSources;
/* Golden-mode loader only, before boot preparation. */
int p4_golden_mode_sources_register(void);
P4GoldenModeSources *p4_golden_mode_sources_context(void);
void p4_golden_mode_sources_rollback(void *context);
#endif
