#include "p4pc/game_table_xml.h"
#include "p4pc/packages.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static P4GameTableXmlTarget candidate(P4GameTablesDefinitionBuilder *builder)
{
    memset(builder, 0, sizeof *builder);
    assert(p4_game_tables_builder_init(builder, NULL, 91) == P4_TABLE_OK);
    uint32_t source;
    assert(p4_game_table_sources_native(builder->sources, "fixture:xml-data", 1, 37, &source) == P4_TABLE_OK);
    return (P4GameTableXmlTarget){builder, "xml", source, 37};
}
static void destroy(P4GameTablesDefinitionBuilder *builder)
{
    assert(p4_game_tables_builder_destroy(builder) == P4_TABLE_OK);
}
/* Fail-stop runtime boundaries: this parser fixture never loads, consumes or
 * admits a native guest row or enters an interactive frontend. */
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected XML runtime failure: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ (void)provider; fprintf(stderr, "Unexpected XML origin use: %s\n", reason); abort(); }
static void hex(char *out, const void *bytes, size_t size)
{
    static const char digits[] = "0123456789abcdef";
    const uint8_t *in = bytes;
    for (size_t i = 0; i < size; ++i) {
        out[2 * i] = digits[in[i] >> 4]; out[2 * i + 1] = digits[in[i] & 15];
    }
    out[2 * size] = 0;
}
static void valid(void)
{
    char bytes[3000], error[200];
    int length = snprintf(bytes, sizeof bytes,
        "<p4-tables schema=\"1\">"
        "<element id=\"19\" flags=\"7\" family=\"0\" order=\"0\" key=\"xml:leaf\" display-name=\"Slash &amp; other\" short-name=\"Slash\" "
        "icon-owner=\"art\" icon-key=\"art:slash\" icon-revision=\"15\" "
        "header-icon-owner=\"art\" header-icon-key=\"art:slash-header\" header-icon-revision=\"16\"/>"
        "<channel id=\"19\" kind=\"0\" key=\"xml:channel\" display-name=\"Custom damage\"/>"
        "<affinity kind=\"1\" entity=\"1\" profile=\"0x80000000\" element=\"19\" channel=\"19\" value=\"100\" scalar=\"100\"/>"
        "<action source-kind=\"%u\" source-id=\"120\" component=\"0\" element=\"19\" channel=\"19\" key=\"xml:action\"/>"
        "<alias kind=\"1\" entity=\"1\" native-profile=\"5\" profile=\"2147483648\" knowledge=\"4294967295\" key=\"xml:alias\"/>"
        "<effect source-kind=\"%u\" source-id=\"440\" element=\"19\" channel=\"19\" flags=\"0x10000000\" key=\"xml:effect\"/>"
        "<damage-cover source-kind=\"%u\" source-id=\"12\" element=\"19\" channel=\"0\" percent=\"25\" "
        "family=\"xml:lining\" key=\"xml:cover\"/>"
        "<offensive-effect source-kind=\"0\" source-id=\"440\" element=\"19\" flags=\"3\" key=\"xml:offense\"/>"
        "<element id=\"22\" flags=\"12\" family=\"22\" order=\"22\" key=\"xml:freeze\" display-name=\"Freeze\" short-name=\"Frz\"/>"
        "<element id=\"23\" flags=\"12\" family=\"23\" order=\"23\" key=\"xml:shock\" display-name=\"Shock\" short-name=\"Shk\"/>"
        "<status-ailment element=\"22\" flags=\"7\" cure-mask=\"0x16\" default-from=\"2\" crit-bonus=\"100\" enemy-scale=\"50\" key=\"xml:freeze-ailment\"/>"
        "<status-ailment element=\"23\" flags=\"9\" cure-mask=\"0xFF\" transfer=\"25\" key=\"xml:shock-ailment\"/>"
        "<status-rider skill=\"0x1A\" element=\"22\" rate=\"10\" key=\"xml:bufu-freeze\"/>"
        "</p4-tables>", (unsigned)P4_ACTION_ELEMENT_SKILL, (unsigned)P4_AFFINITY_EFFECT_SKILL,
        (unsigned)P4_AFFINITY_EFFECT_EQUIPPED_ITEM);
    assert(length > 0 && (size_t)length < sizeof bytes);
    P4GameTablesDefinitionBuilder builder;
    P4GameTableXmlTarget target = candidate(&builder);
    P4GameTables *tables = builder.tables; P4GameTableSources *sources = builder.sources;
    assert(p4_game_tables_source_xml(&target, (const uint8_t *)bytes, (size_t)length, error, sizeof error) == P4_TABLE_OK && !*error);
    assert(!p4_game_tables_revision(tables));
    assert(p4_game_tables_freeze(tables, 91) == P4_TABLE_OK);
    const P4ElementDesc *leaf = p4_game_tables_find_element(tables, 19);
    assert(leaf && leaf->revision == 37 && !strcmp(leaf->display_name, "Slash & other"));
    assert(!strcmp(leaf->icon.owner, "art") && !strcmp(leaf->icon.key, "art:slash") && leaf->icon.revision == 15);
    assert(!strcmp(leaf->header_icon.owner, "art") && !strcmp(leaf->header_icon.key, "art:slash-header") &&
        leaf->header_icon.revision == 16);
    const P4AffinityProfileAlias *alias = p4_game_tables_affinity_alias_at(tables, 0);
    assert(alias && alias->profile_id == 0x80000000u && alias->knowledge_id == UINT32_MAX);
    const P4DamageCoverBinding *cover = p4_game_tables_find_damage_cover(tables, P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 12, 19, 0);
    assert(cover && cover->percent == 25 && !strcmp(cover->family, "xml:lining") && cover->revision == 37);
    const P4OffensiveEffectBinding *offense = p4_game_tables_find_offensive_effect(tables, P4_AFFINITY_EFFECT_SKILL, 440, 19);
    assert(offense && offense->flags == (P4_OFFENSE_BOOST | P4_OFFENSE_AMP) && offense->revision == 37);
    const P4SideAilmentDef *freeze = p4_game_tables_find_side_ailment(tables, 22);
    const P4SideAilmentDef *shock = p4_game_tables_find_side_ailment(tables, 23);
    assert(freeze && freeze->flags == 7u && freeze->cure_mask == 0x16 && freeze->default_from == 2 &&
        freeze->crit_bonus == 100 && freeze->revision == 37 && !strcmp(freeze->key, "xml:freeze-ailment"));
    assert(shock && shock->flags == (P4_SIDE_LOSE_TURN | P4_SIDE_BREAK_ON_HIT) && shock->cure_mask == 0xFF &&
        shock->default_from == P4_SIDE_DEFAULT_NONE && !shock->crit_bonus && shock->transfer == 25 &&
        !shock->enemy_scale); /* Optional ones. */
    assert(!freeze->transfer && freeze->enemy_scale == 50);
    size_t rider_first, rider_count;
    assert(p4_game_tables_find_side_riders(tables, 0x1A, &rider_first, &rider_count) && rider_count == 1);
    const P4SideRiderDesc *rider = p4_game_tables_side_rider_at(tables, rider_first);
    assert(rider && rider->leaf == 22 && rider->rate == 10 && rider->revision == 37 &&
        !strcmp(rider->key, "xml:bufu-freeze"));
    assert(p4_game_table_sources_freeze(sources) == P4_TABLE_OK);
    P4TableSourceResolved offense_origin;
    const uint32_t offense_ids[5] = {P4_AFFINITY_EFFECT_SKILL, 440, 19, 0, 0};
    assert(!p4_game_table_sources_resolve_entry(sources, tables, P4_TABLE_SOURCE_OFFENSE,
        offense_ids, &offense_origin));
    assert(!strcmp(offense_origin.entry->key, "xml:offense") &&
        !strcmp(offense_origin.origin->source_key, "fixture:xml-data"));
    assert(p4_game_table_sources_require_owner(tables, NULL, "xml", sources) == P4_TABLE_OK);
    const P4TableSourceOrigin *origin = p4_game_table_sources_origin_at(sources, tables, "xml", 0);
    assert(origin && !strcmp(origin->source_key, "fixture:xml-data") && origin->schema == 1 && origin->revision == 37);
    P4TableSourceCursor cursor = {0}; P4TableSourceEntry entry; unsigned count = 0, covers = 0, sides = 0, riders = 0,
        offenses = 0;
    while (p4_game_table_sources_next(P4_TABLE_SOURCE_REGISTRY, tables, &cursor, &entry) == P4_TABLE_OK) {
        ++count; assert(!strcmp(entry.owner, "xml") && entry.schema == 1);
        assert(entry.revision == (entry.domain == P4_TABLE_SOURCE_AFFINITY ? 91u : 37u));
        if (entry.domain == P4_TABLE_SOURCE_COVER) {
            ++covers; assert(!strcmp(entry.key, "xml:cover") && entry.ids[0] == P4_AFFINITY_EFFECT_EQUIPPED_ITEM &&
                entry.ids[1] == 12 && entry.ids[2] == 19 && entry.ids[3] == 0);
        }
        if (entry.domain == P4_TABLE_SOURCE_SIDE_AILMENT) {
            ++sides; assert(entry.ids[0] == 22 + (sides - 1) && !entry.ids[1]);
        }
        if (entry.domain == P4_TABLE_SOURCE_SIDE_RIDER) {
            ++riders; assert(!strcmp(entry.key, "xml:bufu-freeze") && entry.ids[0] == 0x1A && entry.ids[1] == 22 && !entry.ids[2]);
        }
        if (entry.domain == P4_TABLE_SOURCE_OFFENSE) {
            ++offenses; assert(!strcmp(entry.key, "xml:offense") && entry.ids[0] == P4_AFFINITY_EFFECT_SKILL &&
                entry.ids[1] == 440 && entry.ids[2] == 19 && !entry.ids[3]);
        }
    }
    assert(count == 13 && covers == 1 && sides == 2 && riders == 1 && offenses == 1);
    destroy(&builder);
}
static void families(void)
{
    P4GameTablesDefinitionBuilder builder;
    P4GameTableXmlTarget target = candidate(&builder);
    uint8_t skill[40] = {2}, presentation[4] = {1, 0, 2, 0};
    P4ItemRow item = {.kind = 1, .flags = 0x12345678u, .stat_bonus = {0, 0, -3, 0, 0}};
    char skill_hex[81], presentation_hex[9], item_hex[137], bytes[4096], error[200];
    hex(skill_hex, skill, sizeof skill); hex(presentation_hex, presentation, sizeof presentation);
    hex(item_hex, &item, sizeof item);
    int length = snprintf(bytes, sizeof bytes,
        "<p4-tables schema=\"1\">"
        "<skill id=\"1024\" active=\"1\" element=\"4294967295\" element-aux=\"255\" inheritance-group=\"18\" inheritance-rate=\"-128\" "
        "key=\"xml:skill\" metadata=\"%s\" presentation=\"%s\" effect-source=\"1\"/>"
        "<item id=\"1536\" key=\"xml:item\" metadata=\"%s\" icon-owner=\"art\" icon-key=\"art:item\" icon-revision=\"15\" "
        "require0-id=\"7\" require0-quantity=\"2\"/>"
        "<category id=\"6\" key=\"xml:cards\" filter-a=\"64\" filter-b=\"128\" stock-policy=\"1\" "
        "icon-owner=\"art\" icon-key=\"art:cards\" icon-revision=\"15\" action-owner=\"cards\" action-key=\"cards:apply\" action-revision=\"9\"/>"
        "<string table=\"0\" id=\"1536\" role=\"0\" bytes=\"41808000\"/>"
        "<string table=\"0\" id=\"1536\" role=\"1\" bytes=\"440a4500\"/>"
        "<string table=\"1\" id=\"1024\" role=\"0\" bytes=\"5300\"/>"
        "<string table=\"1\" id=\"1024\" role=\"1\" bytes=\"4400\"/>"
        "</p4-tables>", skill_hex, presentation_hex, item_hex);
    assert(length > 0 && (size_t)length < sizeof bytes);
    assert(!p4_game_tables_source_xml(&target, (const uint8_t *)bytes, (size_t)length, error, sizeof error) && !*error);
    assert(p4_game_skills_count(builder.skills) == 1 && p4_game_tables_items_count(builder.items) == 1);
    assert(p4_game_tables_items_category_count(builder.items) == 1);
    /* Do not weaken immutable getters just to inspect an unsealed candidate.
     * The actual Root fixture checks full rows after genuine input-backed seal. */
    assert(!p4_game_skills_at(builder.skills, 0) && !p4_game_tables_items_at(builder.items, 0) &&
        !p4_game_tables_items_category_at(builder.items, 0));
    assert(p4_game_tables_text_count(builder.names) == 2 && p4_game_tables_text_count(builder.descriptions) == 2);
    assert(p4_game_tables_text_profile(builder.names)->encoding == P4_TEXT_ENCODING_US_RAW_NATIVE &&
        p4_game_tables_text_profile(builder.descriptions)->encoding == P4_TEXT_ENCODING_US_NATIVE);
    assert(!p4_game_skills_revision(builder.skills) && !p4_game_tables_items_revision(builder.items) &&
        !p4_game_tables_text_revision(builder.names) && !builder.input_count && !builder.native_uses);
    /* Actual pure text seals prove decoded two-byte glyphs/newline survived;
     * no source open, C action, guest allocation or materialization occurred. */
    assert(!p4_game_tables_text_seal(builder.names, 91) && !p4_game_tables_text_seal(builder.descriptions, 91));
    P4GameTextDefinition definition;
    assert(!p4_game_tables_text_definition_find(builder.names, P4_TEXT_ITEM, 1536, P4_TEXT_NAME, &definition));
    const uint8_t expected[] = {'A', 0x80, 0x80, 0};
    assert(definition.size == sizeof expected && !memcmp(definition.bytes, expected, sizeof expected));
    destroy(&builder);
}
static int enemy_xml(P4GameTablesDefinitionBuilder *builder, const char *id, const char *metadata,
    const char *extra, char *error, size_t error_size)
{
    uint8_t affinity[32] = {1}, model[232] = {2}, ai[164] = {3};
    char affinity_hex[65], model_hex[465], ai_hex[329], bytes[2048];
    hex(affinity_hex, affinity, sizeof affinity); hex(model_hex, model, sizeof model); hex(ai_hex, ai, sizeof ai);
    P4GameTableXmlTarget target = candidate(builder);
    int length = snprintf(bytes, sizeof bytes, "<p4-tables schema=\"1\">"
        "<enemy id=\"%s\" model-source=\"335\" key=\"xml:enemy\" metadata=\"%s\" affinity=\"%s\" "
        "model=\"%s\" ai=\"%s\"%s/>"
        "<string table=\"3\" id=\"336\" role=\"0\" bytes=\"536861646f7700\"/>"
        "</p4-tables>", id, metadata, affinity_hex, model_hex, ai_hex, extra);
    assert(length > 0 && (size_t)length < sizeof bytes);
    return p4_game_tables_source_xml(&target, (const uint8_t *)bytes, (size_t)length, error, error_size);
}
static void enemies(void)
{
    uint8_t metadata[60] = {7};
    char metadata_hex[121], error[200];
    hex(metadata_hex, metadata, sizeof metadata);
    P4GameTablesDefinitionBuilder builder;
    assert(!enemy_xml(&builder, "336", metadata_hex, "", error, sizeof error) && !*error);
    const uint32_t inputs = (1u << P4_TABLE_INPUT_UNIT) | (1u << P4_TABLE_INPUT_MODEL) |
        (1u << P4_TABLE_INPUT_AICALC) | (1u << P4_TABLE_INPUT_MSG);
    assert(p4_game_enemies_count(builder.enemies) == 1 && !p4_game_enemies_at(builder.enemies, 0));
    assert((builder.input_mask & inputs) == inputs && !builder.input_count);
    assert(!p4_game_enemies_semantic_revision(builder.enemies) && p4_game_tables_text_count(builder.names) == 1);
    destroy(&builder);
    /* Retail rows, inexact bank rows and unknown attributes are refused. */
    assert(enemy_xml(&builder, "335", metadata_hex, "", error, sizeof error) != P4_TABLE_OK && *error);
    destroy(&builder);
    assert(enemy_xml(&builder, "336", metadata_hex + 2, "", error, sizeof error) != P4_TABLE_OK && *error);
    destroy(&builder);
    assert(enemy_xml(&builder, "336", metadata_hex, " extra=\"1\"", error, sizeof error) != P4_TABLE_OK && *error);
    destroy(&builder);
}
static int persona_xml(P4GameTablesDefinitionBuilder *builder, const char *id, const char *metadata,
    const char *extra, char *error, size_t error_size)
{
    uint8_t growth[70] = {1}, model[88] = {2}, candidates[4] = {3}, affinity[32] = {4};
    char growth_hex[141], model_hex[177], candidates_hex[9], affinity_hex[65], bytes[1100];
    hex(growth_hex, growth, sizeof growth); hex(model_hex, model, sizeof model);
    hex(candidates_hex, candidates, sizeof candidates); hex(affinity_hex, affinity, sizeof affinity);
    P4GameTableXmlTarget target = candidate(builder);
    int length = snprintf(bytes, sizeof bytes, "<p4-tables schema=\"1\">"
        "<persona id=\"%s\" model-source=\"255\" key=\"xml:persona\" metadata=\"%s\" growth=\"%s\" "
        "model=\"%s\" candidates=\"%s\" affinity=\"%s\"%s/>"
        "<string table=\"2\" id=\"256\" role=\"0\" bytes=\"4964656e7469747900\"/>"
        "</p4-tables>", id, metadata, growth_hex, model_hex, candidates_hex, affinity_hex, extra);
    assert(length > 0 && (size_t)length < sizeof bytes);
    return p4_game_tables_source_xml(&target, (const uint8_t *)bytes, (size_t)length, error, error_size);
}
static void personas(void)
{
    uint8_t metadata[14] = {7};
    char metadata_hex[29], error[200];
    hex(metadata_hex, metadata, sizeof metadata);
    P4GameTablesDefinitionBuilder builder;
    assert(!persona_xml(&builder, "256", metadata_hex, "", error, sizeof error) && !*error);
    const uint32_t inputs = (1u << P4_TABLE_INPUT_PERSONA) | (1u << P4_TABLE_INPUT_MODEL) | (1u << P4_TABLE_INPUT_MSG) |
        (1u << P4_TABLE_INPUT_UNIT);
    assert(p4_game_personas_count(builder.personas) == 1 && !p4_game_personas_at(builder.personas, 0));
    assert((builder.input_mask & inputs) == inputs && !builder.input_count);
    assert(!p4_game_personas_semantic_revision(builder.personas) && p4_game_tables_text_count(builder.names) == 1);
    destroy(&builder);
    /* Retail rows, the 511 ceiling, inexact bank rows and unknown attributes are refused. */
    assert(persona_xml(&builder, "255", metadata_hex, "", error, sizeof error) != P4_TABLE_OK && *error);
    destroy(&builder);
    assert(persona_xml(&builder, "512", metadata_hex, "", error, sizeof error) != P4_TABLE_OK && *error);
    destroy(&builder);
    assert(persona_xml(&builder, "256", metadata_hex + 2, "", error, sizeof error) != P4_TABLE_OK && *error);
    destroy(&builder);
    assert(persona_xml(&builder, "256", metadata_hex, " extra=\"1\"", error, sizeof error) != P4_TABLE_OK && *error);
    destroy(&builder);
}
/* A retail-derived row names the entity's own (row, column) and requests the
 * actual UNIT input; its value is only read from that input at seal. */
static void retail_rows(void)
{
    const char *xml = "<p4-tables schema=\"1\">"
        "<element id=\"19\" flags=\"15\" family=\"0\" order=\"0\" key=\"xml:slash\" display-name=\"Slash\" short-name=\"Slash\"/>"
        "<affinity kind=\"1\" entity=\"1\" profile=\"1\" element=\"19\" channel=\"0\" retail-row=\"1\" retail-element=\"0\"/>"
        "<affinity kind=\"1\" entity=\"1\" profile=\"1\" element=\"6\" channel=\"0\" value=\"0x08000000\" scalar=\"125\"/>"
        "</p4-tables>";
    P4GameTablesDefinitionBuilder builder;
    P4GameTableXmlTarget target = candidate(&builder); char error[200];
    assert(p4_game_tables_source_xml(&target, (const uint8_t *)xml, strlen(xml), error, sizeof error) == P4_TABLE_OK && !*error);
    assert(builder.input_mask & (1u << P4_TABLE_INPUT_UNIT));
    assert(p4_game_tables_affinity_retail_pending(builder.tables) == 1);
    assert(p4_game_tables_freeze(builder.tables, 91) == P4_TABLE_NOT_READY);
    uint8_t enemies[2 * 32] = {0}, personas[32] = {0};
    enemies[32] = 0x0A; enemies[33] = 0x10; /* Row 1 Phys: resist 50%. */
    assert(p4_game_tables_affinity_resolve_retail(builder.tables, enemies, sizeof enemies, personas, sizeof personas) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(builder.tables, 91) == P4_TABLE_OK);
    const P4AffinityRow *row = p4_game_tables_find_affinity(builder.tables, P4_ENTITY_ENEMY, 1, 1, 19, P4_CHANNEL_DAMAGE);
    assert(row && row->value == (P4_AFF_RESIST | 50) && row->scalar == 50);
    destroy(&builder);
}
#define SKILL_ZERO_ROW "00000000000000000000000000000000000000000000000000000000000000000000000000000000"
#define SKILL_HEAD "<p4-tables schema=\"1\"><skill id=\"1024\" active=\"1\" element=\"2\" element-aux=\"0\" " \
    "inheritance-group=\"0\" inheritance-rate=\"0\" key=\"xml:dive\" "
/* A derived skill names a retail anchor, the fields it overrides and its
 * delivery; the anchor's row is copied at seal (game_skills_test). Each
 * override reaches its own range check, and the delivery its passive check. */
static void derived_skills(void)
{
    static const char *const accepted[] = {
        SKILL_HEAD "derive=\"0x7a\" power=\"9999\" cost=\"999\" accuracy=\"100\" critical=\"0\" delivery=\"physical\" "
            "effect-source=\"0xa9\"/></p4-tables>",
        SKILL_HEAD "derive=\"0x7a\" delivery=\"magic\"/></p4-tables>",
    };
    static const char *const refused[] = {
        SKILL_HEAD "derive=\"0x7a\" power=\"10000\"/></p4-tables>",
        SKILL_HEAD "derive=\"0x7a\" cost=\"1000\"/></p4-tables>",
        SKILL_HEAD "derive=\"0x7a\" accuracy=\"0\"/></p4-tables>",
        SKILL_HEAD "derive=\"0x7a\" critical=\"101\"/></p4-tables>",
        ("<p4-tables schema=\"1\"><skill id=\"1024\" active=\"0\" element=\"2\" element-aux=\"0\" inheritance-group=\"0\" "
         "inheritance-rate=\"0\" key=\"xml:dive\" metadata=\"" SKILL_ZERO_ROW "\" presentation=\"00000000\" "
         "delivery=\"physical\"/></p4-tables>"),
    };
    for (size_t i = 0; i < sizeof accepted / sizeof *accepted; ++i) {
        P4GameTablesDefinitionBuilder builder;
        P4GameTableXmlTarget target = candidate(&builder); char error[200];
        assert(p4_game_tables_source_xml(&target, (const uint8_t *)accepted[i], strlen(accepted[i]), error,
            sizeof error) == P4_TABLE_OK && !*error);
        assert(p4_game_skills_count(builder.skills) == 1);
        destroy(&builder);
    }
    for (size_t i = 0; i < sizeof refused / sizeof *refused; ++i) {
        P4GameTablesDefinitionBuilder builder;
        P4GameTableXmlTarget target = candidate(&builder); char error[200];
        assert(p4_game_tables_source_xml(&target, (const uint8_t *)refused[i], strlen(refused[i]), error,
            sizeof error) != P4_TABLE_OK && *error);
        destroy(&builder);
    }
}
static void invalid(void)
{
    const char *cases[] = {
        "<p4-tables schema=\"1\"/>",
        "<p4-tables schema=\"2\"/>",
        "<p4-tables schema=\"1\" extra=\"0\"/>",
        "<p4-tables schema=\"1\"><unknown/></p4-tables>",
        "<p4-tables schema=\"1\"><element><nested/></element></p4-tables>",
        "<p4-tables schema=\"1\"><channel id=\"-1\" kind=\"0\" key=\"xml:c\" display-name=\"C\"/></p4-tables>",
        /* A header icon is all-or-none, like the skill icon. */
        ("<p4-tables schema=\"1\"><element id=\"19\" flags=\"7\" family=\"0\" order=\"0\" key=\"xml:h\" display-name=\"H\" "
         "short-name=\"H\" header-icon-owner=\"art\" header-icon-key=\"art:h\"/></p4-tables>"),
        "<p4-tables schema=\"1\"><channel id=\"4294967296\" kind=\"0\" key=\"xml:c\" display-name=\"C\"/></p4-tables>",
        "<p4-tables schema=\"1\"><channel id=\"19\" kind=\"0\" key=\"xml:c\" display-name=\"C\" unknown=\"1\"/></p4-tables>",
        "<p4-tables schema=\"1\"><channel id=\"19\" id=\"20\" kind=\"0\" key=\"xml:c\" display-name=\"C\"/></p4-tables>",
        "<p4-tables schema=\"1\"><channel id=\"19\" kind=\"0\" key=\"xml:c\" display-name=\"C\"/>not-whitespace</p4-tables>",
        "<p4-tables schema=\"1\"><channel id=\"19\" kind=\"0\" key=\"xml:c\" display-name=\"C\"/>",
        "<!DOCTYPE p4-tables SYSTEM \"not-authorized\"><p4-tables schema=\"1\"/>",
        "<?not-supported value?><p4-tables schema=\"1\"/>",
        ("<p4-tables schema=\"1\"><channel id=\"19\" kind=\"0\" key=\"xml:c\" display-name=\"C\"/>"
         "<channel id=\"19\" kind=\"0\" key=\"xml:duplicate\" display-name=\"C\"/></p4-tables>"),
        "<p4-tables schema=\"1\"><string table=\"0\" id=\"1536\" role=\"0\" bytes=\"410\"/></p4-tables>",
        "<p4-tables schema=\"1\"><string table=\"0\" id=\"1536\" role=\"0\" bytes=\"GG00\"/></p4-tables>",
        "<p4-tables schema=\"1\"><string table=\"0\" id=\"1536\" role=\"0\" bytes=\"4100\" extra=\"1\"/></p4-tables>",
        "<p4-tables schema=\"1\"><string table=\"0\" id=\"1536\" role=\"0\" bytes=\"410a00\"/></p4-tables>",
        /* Retail-derived affinity: both halves, never mixed with an authored value. */
        "<p4-tables schema=\"1\"><affinity kind=\"1\" entity=\"1\" profile=\"1\" element=\"6\" channel=\"0\" retail-row=\"1\"/></p4-tables>",
        "<p4-tables schema=\"1\"><affinity kind=\"1\" entity=\"1\" profile=\"1\" element=\"6\" channel=\"0\" retail-element=\"0\"/></p4-tables>",
        ("<p4-tables schema=\"1\"><affinity kind=\"1\" entity=\"1\" profile=\"1\" element=\"6\" channel=\"0\" retail-row=\"1\" "
         "retail-element=\"0\" value=\"100\" scalar=\"100\"/></p4-tables>"),
        ("<p4-tables schema=\"1\"><affinity kind=\"1\" entity=\"1\" profile=\"1\" element=\"6\" channel=\"0\" retail-row=\"1\" "
         "retail-element=\"0\" scalar=\"100\"/></p4-tables>"),
        "<p4-tables schema=\"1\"><affinity kind=\"1\" entity=\"1\" profile=\"1\" element=\"6\" channel=\"0\" retail-row=\"1\" retail-element=\"16\"/></p4-tables>",
        /* Damage cover: every field, nothing else, a percent of 1..99 and the owner's family. */
        "<p4-tables schema=\"1\"><damage-cover source-kind=\"1\" source-id=\"12\" element=\"6\" channel=\"0\" percent=\"25\" key=\"xml:c\"/></p4-tables>",
        ("<p4-tables schema=\"1\"><damage-cover source-kind=\"1\" source-id=\"12\" element=\"6\" channel=\"0\" percent=\"25\" "
         "family=\"xml:f\" key=\"xml:c\" extra=\"1\"/></p4-tables>"),
        ("<p4-tables schema=\"1\"><damage-cover source-kind=\"1\" source-id=\"12\" element=\"6\" channel=\"0\" percent=\"100\" "
         "family=\"xml:f\" key=\"xml:c\"/></p4-tables>"),
        ("<p4-tables schema=\"1\"><damage-cover source-kind=\"1\" source-id=\"12\" element=\"6\" channel=\"0\" percent=\"25\" "
         "family=\"other:f\" key=\"xml:c\"/></p4-tables>"),
        /* Status ailment: element, flags, cure-mask and key; default-from and crit-bonus are optional. */
        "<p4-tables schema=\"1\"><status-ailment element=\"22\" flags=\"1\" key=\"xml:s\"/></p4-tables>",
        "<p4-tables schema=\"1\"><status-ailment element=\"22\" flags=\"1\" cure-mask=\"0x16\" crit-bonus=\"\" key=\"xml:s\"/></p4-tables>",
        "<p4-tables schema=\"1\"><status-ailment element=\"22\" flags=\"1\" cure-mask=\"0x16\" crit-bonus=\"101\" key=\"xml:s\"/></p4-tables>",
        "<p4-tables schema=\"1\"><status-ailment element=\"22\" flags=\"1\" cure-mask=\"0x16\" key=\"xml:s\" extra=\"1\"/></p4-tables>",
        "<p4-tables schema=\"1\"><status-ailment element=\"22\" flags=\"1\" cure-mask=\"0x16\" transfer=\"101\" key=\"xml:s\"/></p4-tables>",
        "<p4-tables schema=\"1\"><status-ailment element=\"22\" flags=\"1\" cure-mask=\"0x16\" enemy-scale=\"0\" key=\"xml:s\"/></p4-tables>",
        "<p4-tables schema=\"1\"><status-ailment element=\"22\" flags=\"1\" cure-mask=\"0x16\" enemy-scale=\"101\" key=\"xml:s\"/></p4-tables>",
        "<p4-tables schema=\"1\"><status-ailment element=\"22\" flags=\"16\" cure-mask=\"0x16\" key=\"xml:s\"/></p4-tables>",
        /* Status rider: skill, element, rate and key, all required. */
        "<p4-tables schema=\"1\"><status-rider skill=\"26\" element=\"22\" key=\"xml:r\"/></p4-tables>",
        "<p4-tables schema=\"1\"><status-rider skill=\"26\" element=\"22\" rate=\"0\" key=\"xml:r\"/></p4-tables>",
        "<p4-tables schema=\"1\"><status-rider skill=\"26\" element=\"22\" rate=\"10\" key=\"xml:r\" extra=\"1\"/></p4-tables>",
        /* A skill's effect source is a retail active skill (1..439), never 0 or a passive. */
        ("<p4-tables schema=\"1\"><skill id=\"1024\" active=\"1\" element=\"2\" element-aux=\"255\" inheritance-group=\"18\" "
         "inheritance-rate=\"0\" key=\"xml:s\" metadata=\"" SKILL_ZERO_ROW "\" presentation=\"00000000\" effect-source=\"0\"/></p4-tables>"),
        ("<p4-tables schema=\"1\"><skill id=\"1024\" active=\"1\" element=\"2\" element-aux=\"255\" inheritance-group=\"18\" "
         "inheritance-rate=\"0\" key=\"xml:s\" metadata=\"" SKILL_ZERO_ROW "\" presentation=\"00000000\" effect-source=\"440\"/></p4-tables>"),
        /* Derived skills: a retail active anchor, no rows of their own, overrides in range, a known delivery. */
        SKILL_HEAD "derive=\"0x7a\" metadata=\"" SKILL_ZERO_ROW "\"/></p4-tables>",
        SKILL_HEAD "derive=\"440\"/></p4-tables>",
        SKILL_HEAD "derive=\"0\"/></p4-tables>",
        SKILL_HEAD "derive=\"0x7a\" power=\"0\"/></p4-tables>",
        SKILL_HEAD "derive=\"0x7a\" accuracy=\"101\"/></p4-tables>",
        SKILL_HEAD "derive=\"0x7a\" delivery=\"ranged\"/></p4-tables>",
        SKILL_HEAD "metadata=\"" SKILL_ZERO_ROW "\" presentation=\"00000000\" power=\"80\"/></p4-tables>"
    };
    for (size_t i = 0; i < sizeof cases / sizeof *cases; ++i) {
        P4GameTablesDefinitionBuilder builder;
        P4GameTableXmlTarget target = candidate(&builder); char error[200];
        P4GameTables *tables = builder.tables; P4GameTableSources *sources = builder.sources;
        assert(p4_game_tables_source_xml(&target, (const uint8_t *)cases[i], strlen(cases[i]), error, sizeof error) != P4_TABLE_OK);
        assert(*error && !p4_game_tables_revision(tables) && !p4_game_table_sources_is_sealed(sources));
        destroy(&builder); /* Never reuse a partially decoded candidate. */
    }
    P4GameTablesDefinitionBuilder builder;
    P4GameTableXmlTarget target = candidate(&builder); char error[200];
    const uint8_t nul[] = {'<', 0, '>'};
    assert(p4_game_tables_source_xml(&target, nul, sizeof nul, error, sizeof error) == P4_TABLE_INVALID && *error);
    assert(p4_game_tables_source_xml(&target, nul, P4_GAME_TABLE_SOURCE_XML_MAX + 1u, error, sizeof error) == P4_TABLE_INVALID);
    destroy(&builder);
}
/* Derived items: derive instead of metadata, named overrides in their field
 * widths, and the ITEM and MODEL inputs requested. */
static int item_xml(P4GameTablesDefinitionBuilder *builder, const char *attributes, char *error, size_t error_size)
{
    char bytes[1024];
    P4GameTableXmlTarget target = candidate(builder);
    int length = snprintf(bytes, sizeof bytes, "<p4-tables schema=\"1\"><item id=\"1600\" key=\"xml:forged\" %s "
        "icon-owner=\"art\" icon-key=\"art:item\" icon-revision=\"15\"/></p4-tables>", attributes);
    assert(length > 0 && (size_t)length < sizeof bytes);
    return p4_game_tables_source_xml(&target, (const uint8_t *)bytes, (size_t)length, error, error_size);
}
static int anchor_icon_xml(P4GameTablesDefinitionBuilder *builder, const char *attributes, char *error, size_t error_size)
{
    char bytes[1024];
    P4GameTableXmlTarget target = candidate(builder);
    int length = snprintf(bytes, sizeof bytes, "<p4-tables schema=\"1\"><item id=\"1600\" key=\"xml:forged\" %s/></p4-tables>", attributes);
    assert(length > 0 && (size_t)length < sizeof bytes);
    return p4_game_tables_source_xml(&target, (const uint8_t *)bytes, (size_t)length, error, error_size);
}
static void derived_items(void)
{
    char error[200];
    P4GameTablesDefinitionBuilder builder;
    assert(!item_xml(&builder, "derive=\"0x004\" value=\"0x24\" element=\"2\" price=\"1200\" property=\"255\" "
        "stat-bonus=\"-1,2,0,0,5\" available=\"-1,3\" moon-gates=\"0,0x10\" category-mask=\"0x20\" "
        "primary-value=\"65535\" secondary-value=\"0\" granted-skill=\"4294967295\" accessory-percent=\"10\" right-model=\"9\"",
        error, sizeof error) && !*error);
    const uint32_t inputs = (1u << P4_TABLE_INPUT_ITEM) | (1u << P4_TABLE_INPUT_MODEL);
    assert(p4_game_tables_items_count(builder.items) == 1 && (builder.input_mask & inputs) == inputs);
    destroy(&builder);
    assert(!item_xml(&builder, "derive=\"0x204\"", error, sizeof error) && !*error);
    assert((builder.input_mask & inputs) == inputs); /* Its anchor may be a weapon. */
    destroy(&builder);
    error[0] = 0;
    assert(!anchor_icon_xml(&builder, "derive=\"4\" icon-source=\"anchor\"", error, sizeof error) && !*error);
    assert((builder.input_mask & inputs) == inputs);
    destroy(&builder);
    static const char *const refused_icon[] = {
        "derive=\"4\"", /* Missing both sources. */
        "derive=\"4\" icon-source=\"retail\"",
        "derive=\"4\" icon-source=\"anchor\" icon-owner=\"art\" icon-key=\"art:item\" icon-revision=\"15\"",
        "derive=\"4\" icon-source=\"anchor\" icon-owner=\"art\"",
        "icon-source=\"anchor\"",
    };
    for (size_t i = 0; i < sizeof refused_icon / sizeof refused_icon[0]; ++i) {
        error[0] = 0;
        assert(anchor_icon_xml(&builder, refused_icon[i], error, sizeof error) != P4_TABLE_OK && *error);
        destroy(&builder);
    }
    static const char *const refused[] = {
        "derive=\"4\" metadata=\"00\"",            /* derive replaces metadata */
        "derive=\"1536\"",                         /* a retail anchor only */
        "derive=\"4\" stat-bonus=\"1,2,3,4\"",     /* all five bonuses */
        "derive=\"4\" stat-bonus=\"1,2,3,4,5,6\"",
        "derive=\"4\" stat-bonus=\"32768,0,0,0,0\"",
        "derive=\"4\" property=\"256\"",
        "derive=\"4\" price=\"-1\"",
        "derive=\"4\" available=\"128,1\"",
        "derive=\"4\" moon-gates=\"1\"",
        "derive=\"4\" element=\"\"",
        "derive=\"4\" color=\"1\"",
    };
    for (size_t i = 0; i < sizeof refused / sizeof refused[0]; ++i) {
        error[0] = 0;
        assert(item_xml(&builder, refused[i], error, sizeof error) != P4_TABLE_OK && *error);
        destroy(&builder);
    }
    /* Overrides belong to derived rows. */
    P4ItemRow row = {.kind = 1};
    char row_hex[137], attributes[200];
    hex(row_hex, &row, sizeof row);
    snprintf(attributes, sizeof attributes, "metadata=\"%s\" price=\"1\"", row_hex);
    assert(item_xml(&builder, attributes, error, sizeof error) != P4_TABLE_OK && *error);
    destroy(&builder);
}
int main(void)
{
    valid(); families(); enemies(); personas(); retail_rows(); derived_skills(); invalid(); derived_items();
    assert(!p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    puts("game_table_xml: strict bounded semantic decode, exact native origins, uint32 identities and unpublished-candidate refusal passed; no runtime admission claim");
    return 0;
}
