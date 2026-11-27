#include "../src/platform/golden/golden_mode_identity.h"
#include "../src/platform/golden/golden_mode_literal_identity.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void)
{
    const char *a = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const char *b = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    P4GoldenModeIdentitySource sources[] = {{"data_e.cpk::z", a}, {"data_e.cpk::a", b}};
    P4GoldenModeIdentity out; char reason[160];
    CHECK(!p4_golden_mode_identity_build(&out, a, sources, 2, reason, sizeof reason));
    CHECK(out.identity.source_count == 3 && !strcmp(out.identity.sources[0].name, "P4G.exe"));
    CHECK(!strcmp(out.identity.sources[1].name, "data_e.cpk::a"));
    CHECK(!strcmp(out.identity.sources[1].value, b));
    CHECK(!strcmp(out.identity.sources[2].name, "data_e.cpk::z"));
    for (size_t i = 1; i < out.identity.version_count; ++i)
        CHECK(strcmp(out.identity.versions[i-1].name, out.identity.versions[i].name) < 0);
    const struct { const char *name, *version; } specs[] = {
        {"spec:battle-tables", "2"}, {"spec:cmm-tables", "1"}, {"spec:cmp-tables", "1"},
        {"spec:containers", "1"}, {"spec:exp-pe", "1"}, {"spec:fcl-tables", "2"},
        {"spec:golden-domains", "1"},
        {"spec:l1-image-literals", P4_GOLDEN_LITERAL_MANIFEST_SHA256}};
    for (size_t j = 0; j < sizeof specs / sizeof specs[0]; ++j) {
        int found = 0;
        for (size_t i = 0; i < out.identity.version_count; ++i)
            if (!strcmp(out.identity.versions[i].name, specs[j].name) &&
                !strcmp(out.identity.versions[i].value, specs[j].version)) found = 1;
        CHECK(found);
    }
    int declarations_v2 = 0, skill_v2 = 0;
    for (size_t i = 0; i < out.identity.version_count; ++i) {
        if (!strcmp(out.identity.versions[i].name, "converter:declarations") &&
            !strcmp(out.identity.versions[i].value, "2")) declarations_v2 = 1;
        if (!strcmp(out.identity.versions[i].name, "converter:battle-skill") &&
            !strcmp(out.identity.versions[i].value, "3")) skill_v2 = 1;
    }
    CHECK(declarations_v2 && skill_v2);
    sources[1].chain = sources[0].chain;
    CHECK(p4_golden_mode_identity_build(&out, a, sources, 2, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "duplicate"));
    sources[1].chain = "P4G.exe";
    CHECK(p4_golden_mode_identity_build(&out, a, sources, 2, reason, sizeof reason) == -1);
    sources[1].chain = "data_e.cpk::a"; sources[1].sha256 = "wrong";
    CHECK(p4_golden_mode_identity_build(&out, a, sources, 2, reason, sizeof reason) == -1);
    puts("golden_mode_identity: ok"); return 0;
}
