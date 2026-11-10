/* Synthetic selected VFS banks and actual XML/source/skill/item CPU builder.
 * This does not bind native guest tables or claim gameplay qualification. */
#include "p4pc/game_table_builder.h"
#include "p4pc/game_table_xml.h"
#include "p4pc/packages.h"
#include "p4pc/package_memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static P4GameTablesDefinitionBuilder builder;
static unsigned prepared, published;
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected builder runtime failure: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ (void)provider; fprintf(stderr, "Unexpected builder source failure: %s\n", reason); abort(); }
static int prepare(void *user)
{
    assert(!user && !prepared++);
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
int main(void)
{
    static const P4PackageHandler handler = {.name = "builder-fixture", .prepare = prepare,
        .publish = publish, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    assert(!p4_package_registry_publish() && prepared == 1 && published == 1);
    assert(!p4_game_tables_builder_destroy(&builder));
    p4_package_registry_retire();
    assert(!p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    puts("game_table_builder: real selected synthetic item/skill input seal and exact source bindings passed; no native gameplay claim");
    return 0;
}
