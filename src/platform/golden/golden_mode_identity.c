/* Canonical identities for Golden mode's eager table converters. */
#include "golden_mode_identity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Keep this sorted and update it whenever a consumed converter or spec changes;
 * the cache identity must invalidate rather than reuse outputs from old rules. */
static const P4GoldenCacheIdentityEntry versions[] = {
    {"build:steam", "10836033"},
    {"converter:battle-encounter", "1"}, {"converter:battle-enemy", "1"},
    {"converter:battle-item", "1"}, {"converter:battle-names", "1"},
    {"converter:battle-persona", "1"}, {"converter:battle-skill", "1"},
    {"converter:cache-publisher", "1"}, {"converter:cached-declarations", "1"},
    {"converter:cmm-rankup", "1"}, {"converter:declarations", "2"},
    {"converter:domain-map", "1"}, {"converter:exp-pe", "1"},
    {"converter:fusion", "1"}, {"converter:inheritance", "1"},
    {"converter:output-assembly", "1"}, {"converter:recipe", "1"},
    {"converter:source-adapter", "2"}, {"converter:whole-projection", "1"},
    {"producer:golden-mode", "3"},
    {"schema:golden-tables", "2"}, {"spec:battle-tables", "1"},
    {"spec:cmm-tables", "1"}, {"spec:cmp-tables", "1"},
    {"spec:containers", "1"}, {"spec:exp-pe", "1"},
    {"spec:fcl-tables", "1"}, {"spec:golden-domains", "1"},
};
static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
static int hash_valid(const char *s)
{
    if (!s || strlen(s) != 64) return 0;
    for (; *s; ++s) if (!((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f'))) return 0;
    return 1;
}
static int chain_valid(const char *s)
{
    size_t n = s ? strlen(s) : 0;
    if (!n || n >= P4_GOLDEN_MODE_CHAIN_MAX) return 0;
    for (; *s; ++s) if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') ||
        (*s >= '0' && *s <= '9') || *s == '_' || *s == '-' || *s == '.' || *s == '/' || *s == ':')) return 0;
    return 1;
}
static int compare(const void *a, const void *b)
{
    return strcmp(((const P4GoldenCacheIdentityEntry *)a)->name, ((const P4GoldenCacheIdentityEntry *)b)->name);
}
int p4_golden_mode_identity_build(P4GoldenModeIdentity *out, const char exe_digest[65],
                                  const P4GoldenModeIdentitySource *input, size_t count,
                                  char *reason, size_t capacity)
{
    if (!out || !hash_valid(exe_digest) || !input || !count || count >= P4_GOLDEN_MODE_SOURCE_MAX)
        return fail(reason, capacity, "invalid Golden source identity");
    for (size_t i = 0; i < count; ++i) {
        if (!chain_valid(input[i].chain) || !strcmp(input[i].chain, "P4G.exe") || !hash_valid(input[i].sha256))
            return fail(reason, capacity, "invalid Golden inventory chain or digest");
        for (size_t j = 0; j < i; ++j)
            if (!strcmp(input[i].chain, input[j].chain)) return fail(reason, capacity, "duplicate Golden inventory chain");
    }
    memset(out, 0, sizeof *out);
    strcpy(out->names[0], "P4G.exe"); memcpy(out->hashes[0], exe_digest, 65);
    out->sources[0] = (P4GoldenCacheIdentityEntry){out->names[0], out->hashes[0]};
    for (size_t i = 0; i < count; ++i) {
        strcpy(out->names[i+1], input[i].chain); memcpy(out->hashes[i+1], input[i].sha256, 65);
        out->sources[i+1] = (P4GoldenCacheIdentityEntry){out->names[i+1], out->hashes[i+1]};
    }
    qsort(out->sources, count + 1, sizeof out->sources[0], compare);
    out->identity = (P4GoldenCacheIdentity){out->sources, count + 1, versions, sizeof versions / sizeof versions[0]};
    if (reason && capacity) reason[0] = 0;
    return 0;
}
