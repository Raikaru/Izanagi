/* Collect the exact hashes of the pinned inputs used by the eager converters. */
#include "golden_mode_identity.h"
#include "golden_mode_image.h"
#include "p4pc/content_digest.h"
#include "p4pc/golden_exp.h"
#include "p4pc/golden_inheritance.h"
#include "p4pc/golden_cmm.h"
#include <stdio.h>
#include <stdlib.h>
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
static int load_literals(const char *directory, const char *executable,
    P4GoldenModeIdentityArtifacts *out, char exe_digest[65], char *reason, size_t capacity)
{
    size_t count, pe_count = 0;
    const P4GoldenLiteralSource *sources = p4_golden_image_literal_sources(&count);
    P4GoldenPEDataSpan spans[P4_GOLDEN_LITERAL_SOURCE_COUNT];
    for (size_t i = 0; i < count; ++i) if (sources[i].section[0])
        spans[pe_count++] = (P4GoldenPEDataSpan){sources[i].section, sources[i].sha256,
            sources[i].offset, sources[i].read_size, sources[i].output_offset};
    if (p4_golden_exp_image_read(executable, out->experience, spans, pe_count,
        out->literal_bytes, sizeof out->literal_bytes, exe_digest, reason, capacity)) return -1;
    for (size_t i = 0; i < count; ++i) if (sources[i].section[0]) {
        const P4GoldenLiteralSource *s = sources + i;
        if (s->output_offset > sizeof out->literal_bytes ||
            s->size > sizeof out->literal_bytes - s->output_offset)
            return fail(reason, capacity, "invalid proven literal output range");
        uint8_t *destination = out->literal_bytes + s->output_offset;
        if (!s->converter) {
            if (s->read_size < s->size) return fail(reason, capacity, "invalid literal copy shape");
        } else if (s->converter == 1 && s->size == 84 && s->read_size == 168 &&
                   i + 1 < count && !strcmp(sources[i + 1].name, "image/l1/005dc974-remap") &&
                   sources[i + 1].read_size == 8) {
            if (p4_golden_image_normalize_queues(destination, out->literal_bytes + sources[i + 1].output_offset,
                UINT64_C(0x140000000), 0x1e41b2c0u, 0x00796520u, destination))
                return fail(reason, capacity, "unsupported Golden renderer queue references");
        } else return fail(reason, capacity, "unsupported proven literal converter");
    }
    for (size_t i = 0; i < count; ++i) if (sources[i].constant) {
        const P4GoldenLiteralSource *s = sources + i;
        if (s->output_offset > sizeof out->literal_bytes ||
            s->size > sizeof out->literal_bytes - s->output_offset)
            return fail(reason, capacity, "invalid authored literal output range");
        memcpy(out->literal_bytes + s->output_offset, s->constant, s->size);
    }
    P4GoldenCpk cpk = {0}; P4GoldenCpkEntry entry;
    uint8_t *bin = NULL;
    const uint8_t *pack, *member;
    size_t bin_size, pack_size, member_size;
    char path[1200], digest[65];
    int result = -1;
    if ((size_t)snprintf(path, sizeof path, "%s/data.cpk", directory) >= sizeof path)
        return fail(reason, capacity, "invalid literal archive path");
    if (p4_golden_cpk_open(&cpk, path, reason, capacity)) return -1;
    if (p4_golden_cpk_find(&cpk, "init_free.bin", &entry) ||
        p4_golden_cpk_read(&cpk, &entry, &bin, &bin_size, reason, capacity) ||
        p4_golden_bin_find(bin, bin_size, "init/fclTable.bin", &pack, &pack_size)) {
        fail(reason, capacity, "cannot read proven literal archive"); goto done;
    }
    for (size_t i = 0; i < count; ++i) if (sources[i].member_chain[0]) {
        const P4GoldenLiteralSource *s = sources + i;
        static const char prefix[] = "data.cpk::init_free.bin::init/fclTable.bin::";
        if (strncmp(s->member_chain, prefix, sizeof prefix - 1u) ||
            p4_golden_pak2_find(pack, pack_size, s->member_chain + sizeof prefix - 1u, &member, &member_size)) {
            fail(reason, capacity, "missing proven literal member"); goto done;
        }
        p4_content_digest(member, member_size, digest);
        if (strcmp(digest, s->sha256)) {
            fail(reason, capacity, "unsupported proven literal member digest"); goto done;
        }
        P4GoldenSpan payload; uint32_t rows;
        if (s->offset != 16u || p4_golden_record_table(member, member_size, 0, &payload, &rows) ||
            rows != s->rows || payload.size != s->size ||
            (uint64_t)rows * s->stride != payload.size ||
            s->output_offset > sizeof out->literal_bytes ||
            s->size > sizeof out->literal_bytes - s->output_offset) {
            fail(reason, capacity, "invalid proven literal payload shape"); goto done;
        }
        memcpy(out->literal_bytes + s->output_offset, payload.bytes, payload.size);
    }
    result = 0;
done:
    free(bin); p4_golden_cpk_close(&cpk);
    return result;
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
    if (load_literals(directory, executable, &built, exe_digest, reason, capacity) ||
        p4_golden_levels_read(executable, built.levels, expected, reason, capacity) ||
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
    if (count == P4_GOLDEN_MODE_SOURCE_MAX - 1u)
        return fail(reason, capacity, "too many Golden level sources");
    p4_content_digest(built.levels,sizeof built.levels,hashes[count]);
    inputs[count] = (P4GoldenModeIdentitySource){"P4G.exe::level-base-hp-sp",hashes[count]};
    ++count;
    size_t literal_count;
    const P4GoldenLiteralSource *literal_sources = p4_golden_image_literal_sources(&literal_count);
    for (size_t i = 0; i < literal_count; ++i) if (literal_sources[i].member_chain[0]) {
        if (count == P4_GOLDEN_MODE_SOURCE_MAX - 1u)
            return fail(reason, capacity, "too many proven literal sources");
        inputs[count++] = (P4GoldenModeIdentitySource){literal_sources[i].member_chain, literal_sources[i].sha256};
    }
    if (p4_golden_mode_identity_build(out, exe_digest, inputs, count, reason, capacity)) return -1;
    *artifacts = built;
    return 0;
}
