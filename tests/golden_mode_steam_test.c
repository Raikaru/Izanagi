/* Owner-machine qualification: read-only, no converted payloads are written. */
#include "p4pc/golden_source.h"
#include "p4pc/golden_exp.h"
#include "p4pc/golden_inheritance.h"
#include "p4pc/golden_skill_convert.h"
#include "p4pc/golden_cmm.h"
#include "p4pc/golden_item_convert.h"
#include "p4pc/golden_items.h"
#include "p4pc/golden_encounter_rates.h"
#include "p4pc/golden_names.h"
#include "p4pc/golden_encounter_convert.h"
#include "p4pc/golden_persona_convert.h"
#include "p4pc/golden_enemy_convert.h"
#include "p4pc/golden_fusion_source.h"
#include "p4pc/golden_extra_source.h"
#include "../src/platform/golden/golden_mode_identity.h"
#include "../src/platform/golden/golden_mode_image.h"
#include "../src/platform/golden/golden_mode_outputs.h"
#include "p4pc/game_table_inputs.h"
#include <stdio.h>
#include <string.h>

static const char *const input_labels[] = {"init/itemtbl.bin", "battle/SKILL.TBL", "battle/UNIT.TBL", "battle/MSG.TBL", "battle/PERSONA.TBL", "battle/ENCOUNT.TBL", "battle/EFFECT.TBL", "battle/MODEL.TBL", "battle/AICALC.TBL"};
const char *p4_game_table_input_label(P4GameTableInputKind kind)
{ return (unsigned)kind < P4_TABLE_INPUT_COUNT ? input_labels[kind] : NULL; }
static const P4GoldenCacheTable *output_table(const P4GoldenModeOutputs *out, const char *name)
{
    for (size_t i = 0; i < out->table_count; ++i)
        if (!strcmp(out->tables[i].name, name)) return &out->tables[i];
    return NULL;
}
static uint32_t item_word(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
typedef struct ItemListProbe { const uint8_t *rows; unsigned visited[2560], count; } ItemListProbe;
static uint32_t item_flags(void *p, uint32_t id)
{ return item_word(((ItemListProbe *)p)->rows + id * 68u + 4); }
static uint32_t item_owned(void *p, uint32_t id) { (void)p; (void)id; return 1; }
static int item_emit(void *p, uint32_t id, uint32_t quantity)
{
    ItemListProbe *probe = p;
    if (id >= 2560 || quantity != 1 || probe->visited[id]++) return -1;
    ++probe->count; return 0;
}

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
    static uint8_t battle_names[624 * 23];
    static uint8_t encounter[84 * 348];
    static uint8_t persona_rows[256 * 70];
    static uint8_t enemy_rows[368 * 60];
    static P4GoldenModeIdentity identity;
    static P4GoldenModeIdentityArtifacts artifacts;
    P4GoldenDomainMap map;
    P4GoldenSource source = {0};
    P4GoldenExtraSource extra = {0};
    P4GoldenModeProjection projection = {0};
    P4GoldenModeOutputs outputs = {0};
    static uint8_t skill_elements[624 * 2], skill_metadata[608 * 40];
    static uint8_t semantic_ids[624 * 4], inheritance_groups[624];
    size_t written;
    char directory[1024], reason[256] = "";
    if (p4_golden_locate(NULL, directory, sizeof directory)) return 77;
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    CHECK(!p4_golden_load(&source, directory, reason, sizeof reason));
    CHECK(!p4_golden_extra_load(&extra, directory, reason, sizeof reason));
    CHECK(!p4_golden_mode_identity_load(&identity, &artifacts, directory, &source, &extra, &map, reason, sizeof reason));
    size_t literal_count, archive_sources = 0;
    const P4GoldenLiteralSource *literals = p4_golden_image_literal_sources(&literal_count);
    for (size_t i = 0; i < literal_count; ++i) if (literals[i].member_chain[0]) {
        int found = 0; ++archive_sources;
        for (size_t j = 0; j < identity.identity.source_count; ++j)
            if (!strcmp(identity.identity.sources[j].name, literals[i].member_chain) &&
                !strcmp(identity.identity.sources[j].value, literals[i].sha256)) found = 1;
        CHECK(found);
    }
    CHECK(identity.identity.source_count == 26 + archive_sources && artifacts.inheritance_size == 624);
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
    CHECK(!p4_golden_skill_semantics(skill_elements, 624, semantic_ids, sizeof semantic_ids,
        inheritance_groups, sizeof inheritance_groups));
    for (uint32_t golden = 472; golden < 624; ++golden) {
        uint32_t port;
        CHECK(!p4_golden_domain_to_port(&map, P4_GOLDEN_DOMAIN_SKILL, golden, &port, NULL));
        CHECK(source.tables[P4_GOLDEN_SKILL].spans[0].bytes[golden * 2u] == 0xFF);
        CHECK(!memcmp(semantic_ids + port * 4u, "\xff\xff\xff\xff", 4));
        CHECK(skill_elements[port * 2u] == 0xFF && inheritance_groups[port] == 18);
        CHECK(skill_elements[port * 2u + 1u] ==
            source.tables[P4_GOLDEN_SKILL].spans[0].bytes[golden * 2u + 1u]);
    }
    CHECK(extra.tables[P4_GOLDEN_EXTRA_MODEL].spans[0].size == 11u * 382u);
    CHECK(extra.tables[P4_GOLDEN_EXTRA_MODEL].spans[1].size == 368u * 232u);
    CHECK(extra.tables[P4_GOLDEN_EXTRA_MODEL].spans[2].size == 256u * 88u);
    printf("golden_mode_steam: passive_none=152 MODEL0_rows=11 motion_slots_per_row=32 raw_bytes=4202\n");
    CHECK(!p4_golden_skill_metadata(&map, source.tables[P4_GOLDEN_SKILL].spans[1].bytes,
          source.tables[P4_GOLDEN_SKILL].spans[1].size, skill_metadata, sizeof skill_metadata,
          &written, reason, sizeof reason) && written == sizeof skill_metadata);
    {
        static const unsigned expected[16] = {445,16,5,1,1,0,2,1,1,0,0,0,0,0,0,0};
        unsigned selectors[16] = {0};
        for (unsigned id = 0; id < 472; ++id) {
            const uint8_t *raw = source.tables[P4_GOLDEN_SKILL].spans[1].bytes + id * 44u;
            uint32_t port, flags = (uint32_t)raw[0] | (uint32_t)raw[1] << 8 |
                (uint32_t)raw[2] << 16 | (uint32_t)raw[3] << 24;
            ++selectors[(flags >> 9) & 15u];
            CHECK(!p4_golden_domain_to_port(&map,P4_GOLDEN_DOMAIN_SKILL,id,&port,NULL));
            CHECK(skill_metadata[port * 40u] == raw[0] &&
                !memcmp(skill_metadata + port * 40u + 1, raw + 4, 3) &&
                !memcmp(skill_metadata + port * 40u + 4, raw + 8, 36));
        }
        CHECK(!memcmp(selectors,expected,sizeof selectors));
        printf("golden_mode_steam: SKILL1_rows=472 native_rows=608 widened_selector_cohorts=verified\n");
    }
    CHECK(!p4_golden_mode_project(&map,&source,&projection,reason,sizeof reason));
    CHECK(!p4_golden_mode_outputs_prepare(&outputs,&map,&source,&extra,&projection,&artifacts,reason,sizeof reason));
    {
        P4GoldenSpan spans[8];
        const P4GoldenCacheTable *model = output_table(&outputs,"battle/MODEL.TBL");
        const P4GoldenCacheTable *effect = output_table(&outputs,"battle/EFFECT.TBL");
        const P4GoldenCacheTable *raw_effect = output_table(&outputs,"rows/effect0");
        const P4GoldenCacheTable *raw_skill = output_table(&outputs,"rows/skill1");
        const P4GoldenSpan *original_effect = &extra.tables[P4_GOLDEN_EXTRA_EFFECT].spans[0];
        const P4GoldenSpan *original_skill = &source.tables[P4_GOLDEN_SKILL].spans[1];
        CHECK(model && p4_golden_tbl_spans(model->data,model->size,spans,8) == 3);
        for (unsigned i = 0; i < 3; ++i) {
            const P4GoldenSpan *raw = &extra.tables[P4_GOLDEN_EXTRA_MODEL].spans[i];
            CHECK(spans[i].size == raw->size && !memcmp(spans[i].bytes,raw->bytes,raw->size));
        }
        CHECK(raw_effect && raw_effect->size == 472 * 12 &&
            !memcmp(raw_effect->data,original_effect->bytes,original_effect->size));
        CHECK(raw_skill && raw_skill->size == 472 * 44 &&
            !memcmp(raw_skill->data,original_skill->bytes,original_skill->size));
        CHECK(effect && p4_golden_tbl_spans(effect->data,effect->size,spans,8) == 1 && spans[0].size == 440 * 4);
        const P4GoldenCacheTable *presentation = output_table(&outputs,"decl/skill-presentation");
        CHECK(presentation && presentation->size == 608 * 4);
        for (uint32_t id = 0; id < 472; ++id) {
            uint32_t port;
            CHECK(!p4_golden_domain_to_port(&map,P4_GOLDEN_DOMAIN_SKILL,id,&port,NULL));
            CHECK(!memcmp(presentation->data + port * 4,original_effect->bytes + id * 12,2) &&
                !memcmp(presentation->data + port * 4 + 2,original_effect->bytes + id * 12 + 4,2));
            if (id < 440) CHECK(!memcmp(spans[0].bytes + id * 4,presentation->data + port * 4,4));
        }
        printf("golden_mode_steam: full_projection=converted MODEL0_bytes=4202 EFFECT0_rows=472 SKILL1_rows=472 raw_roundtrip=exact\n");
    }
    {
        const P4GoldenTable *msg = &source.tables[P4_GOLDEN_MSG];
        const struct { int span; P4GoldenDomain domain; unsigned rows, src, dst; } specs[] = {
            {1, P4_GOLDEN_DOMAIN_SKILL, 624, 23, 23},
            {2, P4_GOLDEN_DOMAIN_ENEMY, 368, 21, 21},
            {3, P4_GOLDEN_DOMAIN_PERSONA, 256, 21, 21},
        };
        for (size_t i = 0; i < sizeof specs / sizeof specs[0]; ++i) {
            int rc = p4_golden_names_convert(&map, specs[i].domain, msg->spans[specs[i].span].bytes,
                        msg->spans[specs[i].span].size, specs[i].rows, specs[i].src, specs[i].dst,
                        battle_names, sizeof battle_names, &written, reason, sizeof reason);
            CHECK(!rc && written == (size_t)specs[i].rows * specs[i].dst);
            printf("golden_mode_steam: MSG span=%d rows=%u result=converted\n", specs[i].span, specs[i].rows);
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
    {
        unsigned kinds[10] = {0}, cards = 0, blank = 0, bits = 0, routes[6] = {0};
        for (unsigned id = 0; id < 2560; ++id) {
            const uint8_t *row = item_rows + id * 68u;
            const uint8_t *original = source.tables[P4_GOLDEN_ITEM].bytes + 16 + id * 68u;
            uint32_t kind = item_word(row), skill = item_word(row + 0x24), golden = 0;
            CHECK(kind < 10 && p4_golden_item_category(kind) >= 0); ++kinds[kind];
            CHECK(!memcmp(row, source.tables[P4_GOLDEN_ITEM].bytes + 16 + id * 68u, 0x24));
            if (skill) CHECK(!p4_golden_domain_from_port(&map, P4_GOLDEN_DOMAIN_SKILL, skill, &golden));
            CHECK(golden == item_word(original + 0x24) && !memcmp(row + 0x28, original + 0x28, 28));
            const uint8_t *sk = golden < 472 ? source.tables[P4_GOLDEN_SKILL].spans[1].bytes + golden * 44u : NULL;
            ++routes[p4_golden_item_route(kind, golden, sk)];
            if (kind == 6) { if (golden) ++cards; else ++blank; }
            bits |= item_word(row + 4);
        }
        CHECK(!memcmp(item_rows, projection.item_metadata.data, sizeof item_rows));
        for (unsigned kind = 0; kind < 10; ++kind) CHECK(kinds[kind] == 256);
        CHECK(cards == 253 && blank == 3 && (bits & 0x1ff80000u) == 0x1ff80000u && !(bits & 0xe0000000u));
        P4GoldenItemSort sort;
        const P4GoldenExtraTable *cmp = &extra.tables[P4_GOLDEN_EXTRA_CMP];
        CHECK(!p4_golden_item_sort_open(&sort, cmp->bytes, cmp->size));
        size_t range_count = 0;
        for (unsigned i = 0; i < 12; ++i) range_count += sort.camp[i].ranges.size / 4u;
        for (unsigned i = 0; i < 4; ++i) range_count += sort.equipment[i].ranges.size / 4u;
        CHECK(range_count == 334);
        ItemListProbe probe = {0}; probe.rows = item_rows;
        CHECK(p4_golden_item_list(&sort, -1, item_flags, item_owned, item_emit, &probe) > 0);
        memset(probe.visited, 0, sizeof probe.visited); probe.count = 0;
        CHECK(p4_golden_item_list(&sort, 3, item_flags, item_owned, item_emit, &probe) > 0);
        for (unsigned id = 0; id < 2560; ++id) if (probe.visited[id]) {
            int group = p4_golden_item_camp_group(item_flags(&probe, id));
            CHECK(group >= 0 && p4_golden_item_sell_group((unsigned)group));
        }
        unsigned gc, pc;
        CHECK(!p4_golden_encounter_matrix(source.tables[P4_GOLDEN_ENCOUNT].spans[3].bytes,
            source.tables[P4_GOLDEN_ENCOUNT].spans[3].size,
            projection.encounter[3].data, projection.encounter[3].size, &gc, &pc));
        CHECK(gc == 6528 && pc == 3264);
        printf("golden_mode_steam: ITEM_rows=2560 kinds=10 categories=8 cards=253 blank_cards=3 expanded_bits=10 routes=%u/%u/%u/%u/%u/%u camp_groups=12 equipment_groups=4 shop_selectors=6 P10_groups=68 golden_cells=%u projected_cells=%u zero_totals=0\n",
            routes[0],routes[1],routes[2],routes[3],routes[4],routes[5],gc,pc);
    }
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
          fusion->recipe_counts[4] == 4 && fusion->recipe_counts[5] == 1 &&
          fusion->recipes[16].material_count == 12);
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
    p4_golden_mode_outputs_free(&outputs);
    p4_golden_mode_projection_free(&projection);
    p4_golden_extra_free(&extra);
    p4_golden_free(&source);
    return 0;
fail:
    p4_golden_mode_outputs_free(&outputs);
    p4_golden_mode_projection_free(&projection);
    p4_golden_extra_free(&extra);
    p4_golden_free(&source);
    return 1;
}
