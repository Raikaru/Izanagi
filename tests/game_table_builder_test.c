/* Synthetic selected VFS banks and actual XML/source/skill/item CPU builder.
 * This does not bind native guest tables or claim gameplay qualification. */
#include "p4pc/game_table_builder.h"
#include "p4pc/game_table_xml.h"
#include "p4pc/game_items_ui.h"
#include "p4pc/packages.h"
#include "p4pc/package_memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static P4GameTablesDefinitionBuilder builder;
static unsigned prepared, published;
static const char *fusion_mode, *fusion_xml, *fusion_catalog;
/* This fixture requests no authored images. Abort if the UI tries to resolve,
 * pin or release one rather than retaining native anchor sources. */
uint32_t p4_resources_revision(const P4Resources *resources)
{ (void)resources; abort(); }
const P4ResourceImage *p4_resources_find(const P4Resources *resources, const char *key)
{ (void)resources; (void)key; abort(); }
int p4_resources_pin(uint32_t token) { (void)token; abort(); }
int p4_resources_release(uint32_t token) { (void)token; abort(); }

static uint8_t *read_file(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb"); assert(file);
    assert(!fseek(file, 0, SEEK_END));
    long length = ftell(file); assert(length > 0);
    assert(!fseek(file, 0, SEEK_SET));
    uint8_t *bytes = malloc((size_t)length); assert(bytes);
    assert(fread(bytes, 1, (size_t)length, file) == (size_t)length);
    assert(!fclose(file));
    *size = (size_t)length;
    return bytes;
}
static uint32_t family_flag(unsigned actor)
{
    switch (actor) {
    case 1: return 1u;
    case 2: return 2u;
    case 3: return 4u;
    case 4: return 8u;
    case 6: return 16u;
    case 7: return 32u;
    case 8: return 64u;
    default: abort();
    }
}
static int prepare_fusion(void)
{
    assert(!p4_game_tables_builder_init(&builder, NULL, 9));
    uint32_t source = 0;
    assert(!p4_game_table_sources_native(builder.sources, "fixture:fusion", 1, 7, &source));
    P4GameTableXmlTarget target = {&builder, "p4pc.fusion-arts", source, 7};
    size_t size;
    uint8_t *xml = read_file(fusion_xml, &size);
    char error[256];
    int result = p4_game_tables_source_xml(&target, xml, size, error, sizeof error);
    free(xml);
    if (!strcmp(fusion_mode, "missing-icon") || !strcmp(fusion_mode, "conflicting-icon")) {
        assert(result && error[0] && !builder.sealed);
        return 0;
    }
    if (result) fprintf(stderr, "Fusion XML refused: %s (%d)\n", error, result);
    assert(!result);
    const P4PackageProvider *failed = (const P4PackageProvider *)1;
    result = p4_game_tables_builder_seal(&builder, &failed);
    if (!strcmp(fusion_mode, "bad-kind") || !strcmp(fusion_mode, "bad-flags")) {
        assert(result == P4_TABLE_INVALID && !builder.sealed);
        return 0;
    }
    if (!strcmp(fusion_mode, "missing-name")) {
        assert(result == P4_TABLE_NOT_FOUND && !builder.sealed);
        return 0;
    }
    if (result) fprintf(stderr, "Fusion builder seal refused: %s (%d)\n", p4_game_tables_error(result), result);
    assert(!result && !failed && builder.sealed);
    assert(p4_game_tables_items_count(builder.items) == 910);
    assert(p4_game_tables_text_count(builder.names) == 910);
    assert(p4_game_tables_text_count(builder.descriptions) == 910);
    assert(p4_game_table_sources_validate_view(builder.sources, builder.items) == P4_TABLE_OK);
    const P4TableSourceOrigin *origin = p4_game_table_sources_origin_at(builder.sources, builder.items, "p4pc.fusion-arts", 0);
    assert(origin && origin->kind == P4_TABLE_SOURCE_NATIVE && !strcmp(origin->source_key, "fixture:fusion"));
    assert(!p4_game_table_sources_origin_at(builder.sources, builder.items, "p4pc.fusion-arts", 1));
    const P4GameTableInput *input = p4_game_tables_builder_input(&builder, P4_TABLE_INPUT_ITEM);
    assert(input && p4_game_table_input_size(input) == P4_ITEM_RETAIL_FILE_SIZE);
    const uint8_t *retail = p4_game_table_input_bytes(input);
    P4GameItemsUi *ui = NULL;
    assert(p4_game_tables_items_ui_prepare(builder.items, NULL, &ui) == P4_TABLE_OK && ui);
    FILE *catalog = fopen(fusion_catalog, "rb"); assert(catalog);
    char line[160], name[64], key[96];
    unsigned id, actor, anchor, value, element, hit, count = 0, mismatches = 0;
    while (fgets(line, sizeof line, catalog)) {
        assert(sscanf(line, "%u %u %u %u %u %u %63[^\r\n]", &id, &actor, &anchor, &value, &element, &hit, name) == 7);
        assert(id == 2048u + count && anchor < 256 && hit <= 1);
        const P4GameItemView *row = p4_game_tables_items_find(builder.items, id);
        assert(row && row->id == id && row->revision == 7 && !strcmp(row->owner, "p4pc.fusion-arts"));
        snprintf(key, sizeof key, "p4pc.fusion-arts:forge-%u", id);
        assert(!strcmp(row->key, key));
        assert(!row->icon.owner[0] && !row->icon.key[0] && !row->icon.revision);
        assert(p4_game_tables_items_ui_item(ui, id) == 0);
        P4ItemRow expected;
        memcpy(&expected, retail + P4_ITEM_FILE_HEADER_SIZE + anchor * P4_ITEM_ROW_SIZE, sizeof expected);
        int mismatch = row->icon_anchor_plus_one != anchor + 1 || expected.flags != family_flag(actor) ||
            expected.category_mask != (1u << (actor + 4)) || !row->models_registered ||
            row->right_model_id != anchor ||
            row->left_model_id != (actor == 2 ? anchor + 1000u : P4_ITEM_MODEL_NONE);
        if (value) expected.value = (uint16_t)value;
        if (element) expected.element = (uint16_t)element;
        if (hit) expected.secondary_value = 99;
        if (memcmp(&row->metadata, &expected, sizeof expected)) mismatch = 1;
        if (mismatch) ++mismatches;
        P4GameTextDefinition text;
        assert(p4_game_tables_text_definition_find(builder.names, P4_TEXT_ITEM, id, P4_TEXT_NAME, &text) == P4_TABLE_OK);
        assert(!strcmp(text.owner, "p4pc.fusion-arts") && text.size == strlen(name) + 1 && !memcmp(text.bytes, name, text.size));
        assert(p4_game_tables_text_definition_find(builder.descriptions, P4_TEXT_ITEM, id, P4_TEXT_DESCRIPTION, &text) == P4_TABLE_OK);
        assert(!strcmp(text.owner, "p4pc.fusion-arts") && text.size > 1);
        ++count;
    }
    assert(!ferror(catalog) && !fclose(catalog) && count == 910);
    if (!strcmp(fusion_mode, "wrong-anchor") || !strcmp(fusion_mode, "wrong-family")) assert(mismatches);
    else assert(!strcmp(fusion_mode, "good") && !mismatches);
    p4_game_tables_items_ui_destroy(ui);
    return 0;
}
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected builder runtime failure: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ (void)provider; fprintf(stderr, "Unexpected builder source failure: %s\n", reason); abort(); }
static int prepare(void *user)
{
    assert(!user && !prepared++);
    if (fusion_mode) return prepare_fusion();
    assert(!p4_game_tables_builder_init(&builder, NULL, 9));
    uint32_t source = 0;
    assert(!p4_game_table_sources_native(builder.sources, "fixture:builder", 1, 7, &source));
    P4GameTableXmlTarget target = {&builder, "test", source, 7};
    char metadata[81], presentation[9], item[137], xml[2048], error[256];
    memset(metadata, '0', sizeof metadata - 1); metadata[sizeof metadata - 1] = 0;
    memset(presentation, '0', sizeof presentation - 1); presentation[sizeof presentation - 1] = 0;
    memset(item, '0', sizeof item - 1); item[sizeof item - 1] = 0;
    item[1] = '3'; /* Retail consumable kind: no invented weapon model. */
    int length = snprintf(xml, sizeof xml,
        "<p4-tables schema=\"1\">"
        "<skill id=\"576\" active=\"1\" element=\"0\" element-aux=\"0\" inheritance-group=\"0\" inheritance-rate=\"1\" key=\"test:skill\" metadata=\"%s\" presentation=\"%s\"/>"
        "<item id=\"1536\" key=\"test:item\" metadata=\"%s\" icon-owner=\"art\" icon-key=\"art:item\" icon-revision=\"1\"/>"
        "<string table=\"1\" id=\"576\" role=\"0\" bytes=\"4100\"/>"
        "<string table=\"1\" id=\"576\" role=\"1\" bytes=\"4200\"/>"
        "<string table=\"0\" id=\"1536\" role=\"0\" bytes=\"4300\"/>"
        "<string table=\"0\" id=\"1536\" role=\"1\" bytes=\"4400\"/>"
        "</p4-tables>", metadata, presentation, item);
    assert(length > 0 && (size_t)length < sizeof xml);
    int result = p4_game_tables_source_xml(&target, (const uint8_t *)xml, (size_t)length, error, sizeof error);
    if (result) fprintf(stderr, "Builder XML refused: %s (%d)\n", error, result);
    assert(!result);
    const P4PackageProvider *failed = (const P4PackageProvider *)1;
    result = p4_game_tables_builder_seal(&builder, &failed);
    if (result) fprintf(stderr, "Builder seal refused: %s (%d), inputs=%zu tables=%u skill=%u item=%u names=%u descriptions=%u\n",
        p4_game_tables_error(result), result, builder.input_count, p4_game_tables_revision(builder.tables),
        p4_game_skills_semantic_revision(builder.skills), p4_game_tables_items_semantic_revision(builder.items),
        p4_game_tables_text_semantic_revision(builder.names), p4_game_tables_text_semantic_revision(builder.descriptions));
    assert(!result && !failed && builder.sealed && builder.input_count == 4);
    assert(p4_game_skills_semantic_revision(builder.skills) == 9);
    assert(p4_game_tables_items_semantic_revision(builder.items) == 9);
    assert(!p4_game_skills_revision(builder.skills) && !p4_game_tables_items_revision(builder.items));
    assert(p4_game_skills_find(builder.skills, 576) && p4_game_tables_items_find(builder.items, 1536));
    assert(p4_game_table_input_kind(p4_game_table_input_span_owner(p4_game_table_input_span(
        p4_game_tables_builder_input(&builder, P4_TABLE_INPUT_EFFECT), 0))) == P4_TABLE_INPUT_EFFECT);
    return 0;
}
static void publish(void *user) { assert(!user); ++published; }
int main(int argc, char **argv)
{
    if (argc == 5 && !strcmp(argv[1], "--fusion")) {
        fusion_mode = argv[2]; fusion_xml = argv[3]; fusion_catalog = argv[4];
    } else assert(argc == 1);
    static const P4PackageHandler handler = {.name = "builder-fixture", .prepare = prepare,
        .publish = publish, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    assert(!p4_package_registry_publish() && prepared == 1 && published == 1);
    assert(!p4_game_tables_builder_destroy(&builder));
    p4_package_registry_retire();
    assert(!p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    puts(fusion_mode ? "game_table_builder: Fusion CPU fixture passed; no native gameplay claim" :
        "game_table_builder: real selected synthetic item/skill input seal and exact source bindings passed; no native gameplay claim");
    return 0;
}
