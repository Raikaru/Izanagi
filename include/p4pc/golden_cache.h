/* Per-user Golden table cache. Cache contents are derived from the player's install and must never ship. */
#ifndef P4PC_GOLDEN_CACHE_H
#define P4PC_GOLDEN_CACHE_H

#include <stddef.h>
#include <stdint.h>

/* Entries must be sorted by name with unique ASCII names. Source values are
 * lowercase SHA-256 digests; the source list includes every consumed member's
 * full inventory chain and P4G.exe. Versions covers every converter, spec and
 * schema identifier and version. */
typedef struct P4GoldenCacheIdentityEntry {
    const char *name;
    const char *value;
} P4GoldenCacheIdentityEntry;
typedef struct P4GoldenCacheIdentity {
    const P4GoldenCacheIdentityEntry *sources;
    size_t source_count;
    const P4GoldenCacheIdentityEntry *versions;
    size_t version_count;
} P4GoldenCacheIdentity;

/* Table names are sorted and unique. On store, data and size are borrowed; on load,
 * data is allocated by the cache and released with p4_golden_cache_free(). */
typedef struct P4GoldenCacheTable {
    const char *name;
    uint8_t *data;
    size_t size;
} P4GoldenCacheTable;

/* pref must be the existing SDL preference directory, never a caller-selected package
 * location. install, program and repo are existing directories used to refuse unsafe
 * containment through resolved filesystem paths; a NULL repo means no checkout is known. */
typedef struct P4GoldenCachePaths {
    const char *pref;
    const char *install;
    const char *program;
    const char *repo;
} P4GoldenCachePaths;

/* Returns 0 on a verified hit, 1 on a miss/corruption, -1 on unsafe path or I/O error.
 * load expects a valid bounded array naming the complete eager table set, and
 * leaves all data NULL on a miss/error. Store atomically publishes a complete
 * set, replacing corrupt entries. */
int p4_golden_cache_load(const P4GoldenCachePaths *paths, const P4GoldenCacheIdentity *identity,
                         P4GoldenCacheTable *tables, size_t count, char key[65],
                         char *reason, size_t capacity);
int p4_golden_cache_store(const P4GoldenCachePaths *paths, const P4GoldenCacheIdentity *identity,
                          const P4GoldenCacheTable *tables, size_t count, char key[65],
                          char *reason, size_t capacity);
void p4_golden_cache_free(P4GoldenCacheTable *tables, size_t count);
#endif
