/* Measured English fusion sources. Payloads remain in the owner's memory/cache. */
#include "p4pc/golden_fusion_source.h"
#include "p4pc/golden_fusion_convert.h"
#include "p4pc/golden_recipe.h"
#include "p4pc/golden_source.h"
#include "p4pc/content_digest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const P4GoldenFusionSource sources[] = {
    {"fclCombineSkillLimit.ftd", "98db5d368834fe28e9ab6a89303df47af7c7c5b30a48a1abe297d96e2bff2b0c"},
    {"fclCombineSkillAffinity.ftd", "08375912619898b5e97fce629a0507e0b75906dac257eb3888e4c33032718610"},
    {"fclCombineLift.ftd", "9adb5118a865d8184a822bd4ab51de8fcd73f6a3f2de9e17d20da35f3fd99538"},
    {"fclCombineTable_Normal.ftd", "4f3b9449013605a46c8d1ce852f0424ffcadac6438bed9ddd229ab59914ba046"},
    {"fclCombineTable_Third_1.ftd", "4f3b9449013605a46c8d1ce852f0424ffcadac6438bed9ddd229ab59914ba046"},
    {"fclCombineTable_Third_2.ftd", "d167f6db53cae01f2e57a14899334c21725b27f1739b88b7c02adf24c173243a"},
    {"fclCombineTable_NormalSP.ftd", "e546c2965e800fb0dade33dda452935c0c81eb9908e6c030db3729a304f05e01"},
    {"fclCombineTable_ThirdSP.ftd", "6b07456200fcfdecabadfc4c4e899982a265b5e8286502dd48e64ebffa26a91e"},
    {"fclCombineTable_Fourth.ftd", "034552c6dd663338f6665e0c1a7fa7b29c47713f9f8ee12caa195c35a49a04b6"},
    {"fclCombineTable_Fifth.ftd", "698d8d294e7498aa5ceaf96d360afa7f269b92e4fbea8d26f17ef7f334da7644"},
    {"fclCombineTable_Sixth.ftd", "d56dd90175206c5a672d423c3a5103a0497665cd74f5f57a9f3c0507359e3067"},
    {"fclCombineTable_Twelfth.ftd", "809779f8fcb4d9266380675e215808e8ce36ea4f47854148b4d01e44e33c513a"},
};
const P4GoldenFusionSource *p4_golden_fusion_sources(size_t *count)
{
    if (count) *count = sizeof sources / sizeof sources[0];
    return sources;
}
int p4_golden_fusion_load(const char *directory, const P4GoldenDomainMap *map,
                           P4GoldenFusionTables *out, char *reason, size_t capacity)
{
    static const unsigned recipe_rows[] = {3, 1, 4, 4, 4, 1}, materials[] = {2, 3, 4, 5, 6, 12};
    P4GoldenFusionTables built = {0};
    P4GoldenCpk cpk = {0};
    P4GoldenCpkEntry entry;
    const uint8_t *pack, *ftd;
    uint8_t *bin = NULL;
    size_t bin_size, pack_size, ftd_size, written, recipe_index = 0;
    char path[1100], digest[65], scratch[160];
    char *why = reason && capacity ? reason : scratch;
    size_t room = reason && capacity ? capacity : sizeof scratch;
    int result = -1;
    if (!directory || !map || !out || (size_t)snprintf(path, sizeof path, "%s/data_e.cpk", directory) >= sizeof path) {
        snprintf(why, room, "invalid fusion source path or output"); return -1;
    }
    if (p4_golden_cpk_open(&cpk, path, why, room)) return -1;
    if (p4_golden_cpk_find(&cpk, "init_free.bin", &entry) ||
        p4_golden_cpk_read(&cpk, &entry, &bin, &bin_size, why, room) ||
        p4_golden_bin_find(bin, bin_size, "init/fclTable.bin", &pack, &pack_size)) {
        snprintf(why, room, "cannot read fusion table archive"); goto done;
    }
    for (unsigned i = 0; i < sizeof sources / sizeof sources[0]; ++i) {
        if (p4_golden_pak2_find(pack, pack_size, sources[i].member, &ftd, &ftd_size)) {
            snprintf(why, room, "missing fusion source %s", sources[i].member); goto done;
        }
        p4_content_digest(ftd, ftd_size, digest);
        if (strcmp(digest, sources[i].sha256)) {
            snprintf(why, room, "unsupported fusion source %s", sources[i].member); goto done;
        }
        if (i < 6) {
            P4GoldenFusionKind kind = i == 0 ? P4_GOLDEN_FUSION_LIMIT : i == 1 ? P4_GOLDEN_FUSION_AFFINITY :
                                      i == 2 ? P4_GOLDEN_FUSION_GATE : P4_GOLDEN_FUSION_CHART;
            uint8_t *dest = i == 0 ? built.limits : i == 1 ? built.affinities : i == 2 ? built.gates : built.charts[i - 3];
            size_t cap = i == 0 ? sizeof built.limits : i == 1 ? sizeof built.affinities :
                         i == 2 ? sizeof built.gates : sizeof built.charts[0];
            if (p4_golden_fusion_convert(map, kind, ftd, ftd_size, dest, cap, &written, why, room)) goto done;
        } else {
            P4GoldenSpan payload;
            uint32_t rows;
            unsigned group = i - 6;
            if (p4_golden_record_table(ftd, ftd_size, 0, &payload, &rows) || rows != recipe_rows[group] ||
                payload.size != (size_t)rows * 44) {
                snprintf(why, room, "invalid fusion recipe source %s", sources[i].member); goto done;
            }
            for (uint32_t j = 0; j < rows; ++j) {
                uint8_t discarded[32];
                uint8_t *dest = group < 5 ? built.recipes[recipe_index] : discarded;
                int rc = p4_golden_recipe_convert(map, payload.bytes + j * 44, 44, materials[group], dest, why, room);
                if (group == 5 && rc == 1) { ++built.unsupported_twelfth; continue; }
                if (rc || group >= 5) goto done;
                ++recipe_index;
                ++built.recipe_counts[group];
            }
        }
    }
    *out = built;
    if (reason && capacity) reason[0] = 0;
    result = 0;
done:
    free(bin);
    p4_golden_cpk_close(&cpk);
    return result;
}
