/* Collect the exact hashes of the pinned inputs used by the eager converters. */
#include "golden_mode_identity.h"
#include "p4pc/content_digest.h"
#include "p4pc/golden_exp.h"
#include "p4pc/golden_inheritance.h"
#include "p4pc/golden_cmm.h"
#include <stdio.h>
#include <string.h>

static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
static void digest_hex(const uint8_t digest[32], char out[65])
{
    static const char hex[] = "0123456789abcdef";
    for (unsigned i = 0; i < 32; ++i) { out[2*i] = hex[digest[i] >> 4]; out[2*i+1] = hex[digest[i] & 15]; }
    out[64] = 0;
}
int p4_golden_mode_identity_load(P4GoldenModeIdentity *out, P4GoldenModeIdentityArtifacts *artifacts,
                                 const char *directory, const P4GoldenSource *source,
                                 const P4GoldenExtraSource *extra, const P4GoldenDomainMap *map,
                                 char *reason, size_t capacity)
{
    static const char *const names[] = {
        "battle/SKILL.TBL", "battle/UNIT.TBL", "battle/PERSONA.TBL",
        "battle/MSG.TBL", "battle/ENCOUNT.TBL", "init/itemtbl.bin"
    };
    P4GoldenModeIdentityArtifacts built = {0};
    P4GoldenModeIdentitySource inputs[P4_GOLDEN_MODE_SOURCE_MAX - 1];
    char chains[P4_GOLDEN_MODE_SOURCE_MAX - 1][P4_GOLDEN_MODE_CHAIN_MAX];
    char hashes[P4_GOLDEN_MODE_SOURCE_MAX - 1][65];
    char executable[1200], exe_digest[65], expected[65];
    size_t count = 0, n_extra, n_fusion;
    if (!out || !artifacts || !directory || !source || !extra || !map ||
        strcmp(directory, source->directory) ||
        (size_t)snprintf(executable, sizeof executable, "%s/P4G.exe", directory) >= sizeof executable)
        return fail(reason, capacity, "invalid Golden identity inputs");
    if (p4_golden_exp_read(executable, built.experience, exe_digest, reason, capacity) ||
        p4_golden_inheritance_load(directory, map, built.inheritance, sizeof built.inheritance,
                                   &built.inheritance_size, reason, capacity) ||
        p4_golden_cmm_rankup_load(directory, map, built.rankup, reason, capacity) ||
        p4_golden_fusion_load(directory, map, &built.fusion, reason, capacity)) return -1;
    for (size_t i = 0; i < sizeof names / sizeof names[0]; ++i) {
        const P4GoldenTable *t = &source->tables[i];
        if (!t->bytes || !t->size) return fail(reason, capacity, "missing verified Golden battle source");
        p4_content_digest(t->bytes, t->size, hashes[count]); digest_hex(t->digest, expected);
        if (strcmp(hashes[count], expected)) return fail(reason, capacity, "Golden battle source changed");
        if ((size_t)snprintf(chains[count], sizeof chains[count], "data_e.cpk::init_free.bin::%s", names[i]) >=
            sizeof chains[count]) return fail(reason, capacity, "Golden source chain too long");
        inputs[count] = (P4GoldenModeIdentitySource){chains[count], hashes[count]}; ++count;
    }
    const P4GoldenExtraIdentity *extra_ids = p4_golden_extra_identities(&n_extra);
    if (n_extra != P4_GOLDEN_EXTRA_COUNT) return fail(reason, capacity, "invalid extra source registry");
    for (size_t i = 0; i < n_extra; ++i) {
        const P4GoldenExtraTable *t = &extra->tables[i];
        if (!t->bytes || !t->size) return fail(reason, capacity, "missing verified Golden extra source");
        p4_content_digest(t->bytes, t->size, hashes[count]);
        if (strcmp(hashes[count], extra_ids[i].sha256)) return fail(reason, capacity, "Golden extra source changed");
        if ((size_t)snprintf(chains[count], sizeof chains[count], "data_e.cpk::init_free.bin::%s", extra_ids[i].member) >=
            sizeof chains[count]) return fail(reason, capacity, "Golden source chain too long");
        inputs[count] = (P4GoldenModeIdentitySource){chains[count], hashes[count]}; ++count;
    }
    const P4GoldenFusionSource *fusion = p4_golden_fusion_sources(&n_fusion);
    if (n_fusion > P4_GOLDEN_MODE_SOURCE_MAX - count - 3) return fail(reason, capacity, "too many Golden fusion sources");
    for (size_t i = 0; i < n_fusion; ++i) {
        if ((size_t)snprintf(chains[count], sizeof chains[count], "data_e.cpk::init_free.bin::init/fclTable.bin::%s",
                             fusion[i].member) >= sizeof chains[count]) return fail(reason, capacity, "Golden source chain too long");
        inputs[count] = (P4GoldenModeIdentitySource){chains[count], fusion[i].sha256}; ++count;
    }
    inputs[count++] = (P4GoldenModeIdentitySource){
        "data_e.cpk::init_free.bin::init/fclTable.bin::fclCombineSkillTargetLv.ftd",
        p4_golden_inheritance_source_sha256()};
    inputs[count++] = (P4GoldenModeIdentitySource){
        "data_e.cpk::init.bin::event/cmm.bin::cmmRankUpSkillTable.dat",
        p4_golden_cmm_rankup_source_sha256()};
    if (p4_golden_mode_identity_build(out, exe_digest, inputs, count, reason, capacity)) return -1;
    *artifacts = built;
    return 0;
}
