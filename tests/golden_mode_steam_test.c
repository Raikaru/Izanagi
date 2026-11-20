/* Owner-machine qualification: read-only, no converted payloads are written. */
#include "p4pc/golden_source.h"
#include "p4pc/golden_exp.h"
#include "p4pc/golden_inheritance.h"
#include "p4pc/golden_skill_convert.h"
#include "p4pc/golden_cmm.h"
#include "p4pc/golden_item_convert.h"
#include "p4pc/golden_names.h"
#include "p4pc/golden_encounter_convert.h"
#include "p4pc/golden_persona_convert.h"
#include "p4pc/golden_enemy_convert.h"
#include "p4pc/golden_fusion_source.h"
#include "p4pc/golden_extra_source.h"
#include "../src/platform/golden/golden_mode_identity.h"
#include <stdio.h>
#include <string.h>


/* Shapes from cmp_tables.md; no payload or player-visible text is logged. */
static const struct { const char *name; uint32_t rows, stride; } camp_tables[] = {
    {"cmpBookHelp.ctd", 3, 64}, {"cmpCalendarItem.ctd", 39, 64},
    {"cmpCalTable.ctd", 74, 12}, {"cmpCmuDressTable.ctd", 13, 2},
    {"cmpConfigHelp.ctd", 92, 64}, {"cmpConfigItem.ctd", 92, 64},
    {"cmpConfigKeyHelp.ctd", 6, 64}, {"cmpConfigPCItem.ctd", 46, 64},
    {"cmpEquipDress.ctd", 4, 64}, {"cmpEquipParam.ctd", 5, 64},
    {"cmpEquipSortTable.ctd", 4, 24}, {"cmpItemList.ctd", 1, 64},
    {"cmpItemSortTable.ctd", 12, 24}, {"cmpItemType.ctd", 8, 64},
    {"cmpPersonaParam.ctd", 5, 64}, {"cmpPsGradTable.ctd", 256, 10},
    {"cmpPsSkillAddTable.ctd", 7, 20}, {"cmpQuestItem.ctd", 69, 64},
    {"cmpQuestTable.ctd", 69, 16}, {"cmpSystemHelp.ctd", 9, 64},
    {"cmpSystemMenu.ctd", 8, 64}, {"cmpTypeID_ACC.ctd", 1, 4},
    {"cmpTypeID_ARMOR.ctd", 1, 4}, {"cmpTypeID_BEETLE.ctd", 1, 4},
    {"cmpTypeID_BOOK.ctd", 21, 4}, {"cmpTypeID_BUTTERFLY.ctd", 1, 4},
    {"cmpTypeID_DRESS.ctd", 11, 4}, {"cmpTypeID_EV_ITEM.ctd", 2, 4},
    {"cmpTypeID_FISH.ctd", 12, 4}, {"cmpTypeID_FOOD.ctd", 2, 4},
    {"cmpTypeID_GEM.ctd", 2, 4}, {"cmpTypeID_ITEM.ctd", 260, 4},
    {"cmpTypeID_LEAF.ctd", 1, 4}, {"cmpTypeID_MATERIAL.ctd", 1, 4},
    {"cmpTypeID_SKL_CARD.ctd", 1, 4}, {"cmpTypeID_VEGETABLE.ctd", 15, 4},
    {"cmpTypeID_WEAPON.ctd", 2, 4},
};

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s (%s)\n", __LINE__, #x, reason); goto fail; } } while (0)
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    static uint8_t item_rows[2560 * 68], item_names[2560 * 24];
    static uint8_t battle_names[624 * 19];
    static uint8_t encounter[84 * 348];
    static uint8_t persona_rows[256 * 70];
    static uint8_t enemy_rows[368 * 60];
    static P4GoldenModeIdentity identity;
    static P4GoldenModeIdentityArtifacts artifacts;
    P4GoldenDomainMap map;
    P4GoldenSource source = {0};
    P4GoldenExtraSource extra = {0};
    static uint8_t skill_elements[624 * 2], skill_metadata[608 * 40];
    size_t written;
    char directory[1024], reason[256] = "";
    if (p4_golden_locate(NULL, directory, sizeof directory)) return 77;
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    CHECK(!p4_golden_load(&source, directory, reason, sizeof reason));
    CHECK(!p4_golden_extra_load(&extra, directory, reason, sizeof reason));
    CHECK(!p4_golden_mode_identity_load(&identity, &artifacts, directory, &source, &extra, &map, reason, sizeof reason));
    CHECK(identity.identity.source_count == 25 && artifacts.inheritance_size == 624);
    CHECK(extra.tables[P4_GOLDEN_EXTRA_EFFECT].span_count == 1 &&
          extra.tables[P4_GOLDEN_EXTRA_MODEL].span_count == 3 &&
          extra.tables[P4_GOLDEN_EXTRA_AICALC].span_count == 11);
    {
        const uint8_t *payload;
        size_t payload_size, count;
        P4GoldenSpan span;
        uint32_t rows;
        const P4GoldenExtraIdentity *identities = p4_golden_extra_identities(&count);
        for (size_t i = 0; i < sizeof camp_tables / sizeof camp_tables[0]; ++i) {
            CHECK(!p4_golden_pak2_find(extra.tables[P4_GOLDEN_EXTRA_CMP].bytes,
                  extra.tables[P4_GOLDEN_EXTRA_CMP].size, camp_tables[i].name, &payload, &payload_size));
            CHECK(!p4_golden_record_table(payload, payload_size, 0, &span, &rows) &&
                  rows == camp_tables[i].rows && span.size == (size_t)rows * camp_tables[i].stride);
        }
        printf("golden_mode_steam: camp_ctd_shapes=%zu\n", sizeof camp_tables / sizeof camp_tables[0]);
        for (size_t i = 0; i < count; ++i)
            printf("golden_mode_steam: init_free.bin::%s sha256=%s\n", identities[i].member, identities[i].sha256);
    }
    CHECK(source.tables[P4_GOLDEN_SKILL].spans[0].size == 624 * 2);
    CHECK(source.tables[P4_GOLDEN_SKILL].spans[1].size == 472 * 44);
    CHECK(!p4_golden_skill_elements(&map, source.tables[P4_GOLDEN_SKILL].spans[0].bytes,
          source.tables[P4_GOLDEN_SKILL].spans[0].size, skill_elements, sizeof skill_elements,
          &written, reason, sizeof reason) && written == sizeof skill_elements);
    CHECK(!p4_golden_skill_metadata(&map, source.tables[P4_GOLDEN_SKILL].spans[1].bytes,
          source.tables[P4_GOLDEN_SKILL].spans[1].size, skill_metadata, sizeof skill_metadata,
          &written, reason, sizeof reason) && written == sizeof skill_metadata);
    {
        const P4GoldenTable *msg = &source.tables[P4_GOLDEN_MSG];
        const struct { int span; P4GoldenDomain domain; unsigned rows, src, dst; } specs[] = {
            {1, P4_GOLDEN_DOMAIN_SKILL, 624, 23, 19},
            {2, P4_GOLDEN_DOMAIN_ENEMY, 368, 21, 21},
            {3, P4_GOLDEN_DOMAIN_PERSONA, 256, 21, 17},
        };
        for (size_t i = 0; i < sizeof specs / sizeof specs[0]; ++i) {
            int rc = p4_golden_names_convert(&map, specs[i].domain, msg->spans[specs[i].span].bytes,
                        msg->spans[specs[i].span].size, specs[i].rows, specs[i].src, specs[i].dst,
                        battle_names, sizeof battle_names, &written, reason, sizeof reason);
            CHECK(!rc || !strcmp(reason, "name does not fit destination width"));
            printf("golden_mode_steam: MSG span=%d rows=%u result=%s\n", specs[i].span, specs[i].rows,
                   rc ? "refused-width" : "converted");
        }
    }
    {
        const P4GoldenSpan *span = &source.tables[P4_GOLDEN_ENCOUNT].spans[3];
        const unsigned first[] = {0, 20, 25}, end[] = {20, 25, 30};
        unsigned empty_categories = 0, affected_rows = 0, zero_totals = 0;
        CHECK(span->size == 68 * 252);
        for (unsigned row_id = 0; row_id < 68; ++row_id) {
            const uint8_t *row = span->bytes + row_id * 252;
            int affected = 0;
            for (unsigned day = 0; day < 2; ++day) {
                if (!row[day] && !row[3 + day] && !row[6 + day]) ++zero_totals;
                for (unsigned category = 0; category < 3; ++category) {
                    int any = 0;
                    for (unsigned j = first[category]; j < end[category]; ++j) {
                        const uint8_t *e = row + 12 + (day * 30 + j) * 4;
                        if ((e[0] || e[1]) && (e[2] || e[3])) any = 1;
                    }
                    if (row[3 * category + day] && !any) { ++empty_categories; affected = 1; }
                }
            }
            affected_rows += (unsigned)affected;
        }
        printf("golden_mode_steam: weighted groups=%u affected_rows=%u empty_nonzero_categories=%u zero_total_days=%u\n",
               68u, affected_rows, empty_categories, zero_totals);
    }
    for (unsigned block = 0; block < 5; ++block) {
        const P4GoldenSpan *span = &source.tables[P4_GOLDEN_ENCOUNT].spans[block];
        CHECK(!p4_golden_encounter_convert(&map, block, span->bytes, span->size, encounter,
                                            sizeof encounter, &written, reason, sizeof reason));
    }
    CHECK(source.tables[P4_GOLDEN_UNIT].spans[0].size == 368 * 60);
    CHECK(!p4_golden_enemy_convert(&map, source.tables[P4_GOLDEN_UNIT].spans[0].bytes,
          source.tables[P4_GOLDEN_UNIT].spans[0].size, enemy_rows, sizeof enemy_rows, reason, sizeof reason));
    CHECK(source.tables[P4_GOLDEN_PERSONA].spans[0].size == 256 * 14);
    {
        const unsigned blocks[] = {1, 2, 4};
        for (unsigned i = 0; i < 3; ++i) {
            const P4GoldenSpan *span = &source.tables[P4_GOLDEN_PERSONA].spans[blocks[i]];
            CHECK(!p4_golden_persona_convert(&map, blocks[i], span->bytes, span->size,
                                              persona_rows, sizeof persona_rows, reason, sizeof reason));
        }
    }
    CHECK(p4_golden_domain_expanded_count(&map, P4_GOLDEN_DOMAIN_SKILL) == 48);
    CHECK(source.tables[P4_GOLDEN_ITEM].size >= 4 &&
          source.tables[P4_GOLDEN_ITEM].bytes[0] == 0 &&
          source.tables[P4_GOLDEN_ITEM].bytes[1] == 10 &&
          source.tables[P4_GOLDEN_ITEM].bytes[2] == 0 &&
          source.tables[P4_GOLDEN_ITEM].bytes[3] == 0);
    CHECK(!p4_golden_items_convert(&map, source.tables[P4_GOLDEN_ITEM].bytes,
          source.tables[P4_GOLDEN_ITEM].size, item_rows, sizeof item_rows,
          item_names, sizeof item_names, reason, sizeof reason));
    CHECK(p4_golden_domain_expanded_count(&map, P4_GOLDEN_DOMAIN_ENEMY) == 32);
    CHECK(p4_golden_domain_expanded_count(&map, P4_GOLDEN_DOMAIN_ITEM) == 1024);
    for (uint32_t i = 0; i < shape.skills; ++i) {
        uint32_t port, back;
        CHECK(!p4_golden_domain_to_port(&map, P4_GOLDEN_DOMAIN_SKILL, i, &port, NULL));
        CHECK(!p4_golden_domain_from_port(&map, P4_GOLDEN_DOMAIN_SKILL, port, &back) && back == i);
    }
    written = artifacts.inheritance_size;
    const P4GoldenFusionTables *fusion = &artifacts.fusion;
    CHECK(fusion->recipe_counts[0] == 3 && fusion->recipe_counts[1] == 1 &&
          fusion->recipe_counts[2] == 4 && fusion->recipe_counts[3] == 4 &&
          fusion->recipe_counts[4] == 4 && fusion->unsupported_twelfth == 1);
    {
        size_t count;
        const P4GoldenFusionSource *inputs = p4_golden_fusion_sources(&count);
        for (size_t i = 0; i < count; ++i)
            printf("golden_mode_steam: init/fclTable.bin::%s sha256=%s\n", inputs[i].member, inputs[i].sha256);
    }
    printf("golden_mode_steam: identity_sources=%zu versions=%zu\n", identity.identity.source_count,
           identity.identity.version_count);
    printf("golden_mode_steam: skills=624 active=472 enemies=368 personas=256 items=2560 rates=%zu exe_sha256=%s\n",
           written, identity.identity.sources[0].value);
    p4_golden_extra_free(&extra);
    p4_golden_free(&source);
    return 0;
fail:
    p4_golden_extra_free(&extra);
    p4_golden_free(&source);
    return 1;
}
