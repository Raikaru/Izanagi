#include "p4pc/game_table_boot.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/game_elements.h"
#include "p4pc/packages.h"
#include "package_boot_internal.h"
#include "p4pc/package_memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const P4PackageHandler p4_game_tables_sources_boot_provider;
static const P4GameTablesBootProduct *cpu_product, *runtime_product;
static unsigned prepares, publishes, runtime_prepares, reads;
static uint64_t bytes_read;
static int expected_active, expected_families, expected_category, expected_item_only, expected_skill_only;
static unsigned native_stage_requests, action_slot, action_prepares;
static P4GameItemActionRef copied_reference;
static int declared_action(void *context, uint32_t item, uint32_t category)
{
    (void)context; (void)item; (void)category;
    fputs("CPU preparation executed a category action\n", stderr); abort();
}
static int prepare_action(P4GameItems *items, const char *owner, uint32_t category,
    const P4GameItemActionRef *reference, void *context)
{
    assert(context == &action_slot && !strcmp(owner, "xml") && category == 6 && reference &&
        reference->revision == 7 && !strcmp(reference->owner, "cards") && !strcmp(reference->key, "cards:apply"));
    copied_reference = *reference;
    ++action_prepares;
    return p4_game_tables_items_action_register(items, owner, category, &copied_reference, declared_action, &action_slot);
}
/* These CPU-only cases must never enter a native recovery frontend. The
 * runtime-use fixture exercises that contract with explicit interception. */
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected source boot recovery: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ (void)provider; fprintf(stderr, "Unexpected source origin recovery: %s\n", reason); abort(); }
/* Explicit CPU-test boundaries, NOT production native stage/admission. There
 * is no authenticated loaded-range receiver or guest heap in this fixture. */
void *p4pc_heap_malloc(size_t size)
{ (void)size; fputs("CPU source preparation touched guest heap\n", stderr); abort(); }
void p4pc_heap_free(void *host)
{ (void)host; fputs("CPU source preparation released guest heap\n", stderr); abort(); }
int p4_game_tables_items_source_stage(const P4GameItems *items)
{
    assert(cpu_product && items == cpu_product->items && p4_game_tables_items_semantic_revision(items));
    assert(!p4_game_tables_items_revision(items));
    ++native_stage_requests;
    return P4_TABLE_NOT_READY; /* Deliberate missing native dependency fact. */
}
int p4_game_tables_items_boot_stage(P4GameItems *items)
{ (void)items; fputs("Native item stage continued after missing source authority\n", stderr); abort(); }
int p4_game_tables_items_boot_unstage(P4GameItems *items)
{ (void)items; fputs("Unowned item raw-hook rollback\n", stderr); abort(); }
int p4_game_skills_boot_unstage(P4GameSkills *skills)
{ (void)skills; fputs("Unowned skill raw-hook rollback\n", stderr); abort(); }
int p4_game_skills_boot_stage(P4GameSkills *skills)
{
    assert(expected_skill_only && cpu_product && skills == cpu_product->skills);
    ++native_stage_requests;
    return P4_TABLE_NOT_READY; /* Deliberate missing native dependency fact. */
}
int p4_game_enemies_boot_stage(P4GameEnemies *enemies)
{ (void)enemies; fputs("Unrequested enemy raw-hook stage\n", stderr); abort(); }
int p4_game_enemies_boot_unstage(P4GameEnemies *enemies)
{ (void)enemies; fputs("Unowned enemy raw-hook rollback\n", stderr); abort(); }
int p4_game_personas_boot_stage(P4GamePersonas *personas)
{ (void)personas; fputs("Unrequested persona raw-hook stage\n", stderr); abort(); }
int p4_game_personas_boot_unstage(P4GamePersonas *personas)
{ (void)personas; fputs("Unowned persona raw-hook rollback\n", stderr); abort(); }
int p4_game_elements_boot_stage(const P4GameTables *tables, const P4GameSkills *skills, const P4GameItems *items)
{ (void)tables; (void)skills; (void)items; fputs("Element stage continued after required family refusal\n", stderr); abort(); }
static void observed(const P4PackageFileOrigin *origin, const void *bytes, size_t size, uint64_t offset, void *user)
{
    assert(!user && origin && origin->package && bytes && size && offset == bytes_read);
    ++reads; bytes_read += size;
}
static int prepare(void *user)
{
    (void)user; ++prepares;
    const void *product = (const void *)1;
    assert(!p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &product));
    cpu_product = product;
    assert(!!cpu_product == expected_active);
    assert(!!p4_packages_source_tracking_required() == expected_active);
    assert(!p4_game_tables_boot_builder()); /* The CPU seal includes inactive input. */
    if (!cpu_product) return 0;
    assert(p4_game_tables_revision(cpu_product->tables));
    assert(cpu_product->native_uses); /* Actual owned provider/lease, not a readiness bit. */
    assert(p4_game_table_sources_is_sealed(cpu_product->sources) && !p4_game_table_sources_is_frozen(cpu_product->sources));
    assert(p4_game_table_sources_validate_view(cpu_product->sources, cpu_product->tables) == P4_TABLE_OK);
    if (expected_item_only) {
        assert(!cpu_product->skills && cpu_product->items && cpu_product->input_count == 1);
        assert(p4_game_table_input_kind(cpu_product->inputs[0]) == P4_TABLE_INPUT_ITEM);
        assert(p4_game_tables_items_count(cpu_product->items) == 1);
        return 0;
    }
    if (expected_skill_only) {
        assert(cpu_product->skills && !cpu_product->items && cpu_product->input_count == P4_TABLE_INPUT_COUNT - 1u);
        unsigned kinds = 0;
        for (size_t i = 0; i < cpu_product->input_count; ++i)
            kinds |= 1u << p4_game_table_input_kind(cpu_product->inputs[i]);
        assert(kinds == (((1u << P4_TABLE_INPUT_COUNT) - 1u) & ~(1u << P4_TABLE_INPUT_ITEM)));
        assert(p4_game_skills_count(cpu_product->skills) == 1);
        return 0;
    }
    assert(cpu_product->skills && cpu_product->items && cpu_product->input_count == P4_TABLE_INPUT_COUNT);
    assert(p4_game_skills_semantic_revision(cpu_product->skills) && !p4_game_skills_revision(cpu_product->skills));
    assert(p4_game_tables_items_semantic_revision(cpu_product->items) && !p4_game_tables_items_revision(cpu_product->items));
    assert(p4_game_skills_count(cpu_product->skills) == (size_t)expected_families &&
        p4_game_tables_items_count(cpu_product->items) == (size_t)expected_families);
    if (expected_families || expected_category) {
        assert(cpu_product->names && cpu_product->descriptions &&
            p4_game_tables_text_count(cpu_product->names) == (expected_families ? 2u : 1u) &&
            p4_game_tables_text_count(cpu_product->descriptions) == (expected_families ? 2u : 1u));
        assert(!p4_game_table_sources_validate_view(cpu_product->sources, cpu_product->skills) &&
            !p4_game_table_sources_validate_view(cpu_product->sources, cpu_product->items) &&
            !p4_game_table_sources_validate_view(cpu_product->sources, cpu_product->names) &&
            !p4_game_table_sources_validate_view(cpu_product->sources, cpu_product->descriptions));
        if (expected_families) {
            const P4GameSkillView *skill = p4_game_skills_at(cpu_product->skills, 0);
            const P4GameItemView *item = p4_game_tables_items_at(cpu_product->items, 0);
            const uint8_t metadata[40] = {2}, presentation[4] = {1, 0, 2, 0};
            const P4ItemRow expected = {.kind = 1, .flags = 0x12345678u, .stat_bonus = {0, 0, -3, 0, 0}};
            assert(skill && skill->desc.id == 1024 && skill->desc.element_id == UINT32_MAX &&
                skill->desc.element_aux == UINT8_MAX && skill->desc.inheritance_group == 18 &&
                skill->desc.inheritance_rate == INT8_MIN && !memcmp(skill->desc.metadata, metadata, sizeof metadata) &&
                !memcmp(skill->desc.presentation, presentation, sizeof presentation));
            assert(item && item->id == 1536 && !memcmp(&item->metadata, &expected, sizeof expected) &&
                item->requirements_mask == 1 && item->requirements[0].item_id == 7 && item->requirements[0].quantity == 2);
        }
        const P4TableSourceOrigin *source = p4_game_table_sources_origin_at(cpu_product->sources, cpu_product->items, "xml", 0);
        assert(source && !strcmp(source->package_id, "source.owner") && source->revision == 37);
    } else assert(!cpu_product->names && !cpu_product->descriptions);
    assert(p4_game_tables_items_category_count(cpu_product->items) == (size_t)expected_category);
    if (expected_category) {
        const P4GameItemCategoryView *category = p4_game_tables_items_category_at(cpu_product->items, 0);
        assert(category && category->id == 6 && category->action.revision == copied_reference.revision && action_prepares == 1);
        assert(p4_game_tables_boot_action_preparer_register(prepare_action, &action_slot) == P4_TABLE_FROZEN);
    }
    const P4GameTableInput *ai = NULL;
    unsigned kinds = 0;
    for (size_t i = 0; i < cpu_product->input_count; ++i) {
        assert(cpu_product->inputs[i] && p4_game_table_input_span_count(cpu_product->inputs[i]));
        kinds |= 1u << p4_game_table_input_kind(cpu_product->inputs[i]);
        assert(!p4_game_table_input_provider(cpu_product->inputs[i])); /* Actual disc, not XML provider inference. */
        if (p4_game_table_input_kind(cpu_product->inputs[i]) == P4_TABLE_INPUT_AICALC) ai = cpu_product->inputs[i];
    }
    assert(kinds == (1u << P4_TABLE_INPUT_COUNT) - 1u);
    assert(ai && p4_game_table_input_span_count(ai) == 11 &&
        p4_game_table_input_span_size(p4_game_table_input_span(ai, 8)) == 164);
    for (size_t bank = 9; bank <= 10; ++bank)
        assert(p4_game_table_input_span_owner(p4_game_table_input_span(ai, bank)) == ai &&
            p4_game_table_input_span_size(p4_game_table_input_span(ai, bank)) == 16);
    const P4ElementDesc *element = p4_game_tables_find_element(cpu_product->tables, 19);
    assert(element && !strcmp(element->key, "xml:leaf") && element->revision == 37);
    const P4TableSourceOrigin *origin = p4_game_table_sources_origin_at(cpu_product->sources, cpu_product->tables, "xml", 0);
    assert(origin && !strcmp(origin->source_key, "xml:combat") && origin->schema == 1 && origin->revision == 37);
    assert(!strcmp(origin->package_id, "source.owner") && !strcmp(origin->package_version, "1.2.3"));
    return 0;
}
static void publish(void *user) { (void)user; ++publishes; }
static int runtime_prepare(void *user)
{
    (void)user; ++runtime_prepares;
    const void *product = (const void *)1;
    assert(!p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &product));
    runtime_product = product;
    assert(!!runtime_product == expected_active);
    if (runtime_product) {
        assert(runtime_product != cpu_product && runtime_product->tables == cpu_product->tables && runtime_product->sources == cpu_product->sources);
        assert(runtime_product->native_uses == cpu_product->native_uses);
        assert(!p4_game_table_sources_active());
    }
    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    const char *mode = argv[1];
    expected_families = !strcmp(mode, "families");
    expected_category = !strcmp(mode, "bound-category");
    expected_item_only = !strcmp(mode, "item-only");
    expected_skill_only = !strcmp(mode, "skill-only");
    expected_active = !strcmp(mode, "good") || expected_families || expected_category || expected_item_only || expected_skill_only;
    int success = expected_active || !strcmp(mode, "empty");
    if (expected_category) assert(!p4_game_tables_boot_action_preparer_register(prepare_action, &action_slot));
    static const P4PackageClient client = {.diagnostic_name = "table source fixture", .manifest_name = "mod.ini",
        .section = "[mod]", .kind = P4_PACKAGE_MOD};
    static const char *const dependencies[] = {P4_GAME_TABLES_BOOT_PROVIDER, NULL};
    static const P4PackageHandler observer = {.name = "source-product-fixture", .prepare = prepare, .publish = publish,
        .boot_phase = P4_PACKAGE_BOOT_UI, .dependencies = dependencies, .runtime_prepare = runtime_prepare};
    assert(!p4_package_handler_register(&observer)); /* Registration order is deliberately reversed. */
    assert(!p4_package_handler_register(&p4_game_tables_sources_boot_provider));
    assert(!p4_package_reads_observe(observed, NULL));
    P4PackageRegistry *registry = NULL;
    if (strcmp(mode, "empty")) assert(!p4_package_registry_load(&registry, &client, argv[2], "source.owner"));
    int result = p4_package_registry_publish();
    if (!success) {
        assert(result && !prepares && !publishes && !runtime_prepares && !p4_game_table_sources_active());
    } else if (expected_active) {
        /* prepare exposed the real sealed CPU product to its dependent before
         * finalize refused the deliberately unavailable native source hook. */
        assert(result && prepares == 1 && !publishes && !runtime_prepares && native_stage_requests == 1);
        assert(reads > 1 && bytes_read > 4096 && !p4_game_table_sources_active());
    } else {
        assert(!result && prepares == 1 && publishes == 1 && !runtime_prepares && !reads && !bytes_read);
        const P4PackageProvider *failed = (const P4PackageProvider *)1; const char *reason = (const char *)1;
        assert(!p4_package_registry_runtime_materialize(&failed, &reason) && !failed && !reason &&
            runtime_prepares == 1 && !native_stage_requests);
        assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    }
    /* No table-source completion committed a catalog here: every mode keeps
     * the stable retail identity, including sealed-but-refused candidates. */
    static const uint8_t retail[32] = {0xe6, 0x47, 0x4d, 0xe9, 0xed, 0x71, 0x9a, 0x17, 0x77, 0x21, 0x49,
        0x8a, 0x3c, 0xa0, 0xfb, 0xc6, 0x3f, 0x0c, 0xc1, 0x16, 0x43, 0x91, 0xfe, 0x9a, 0xa8, 0x56, 0xa5, 0x13,
        0xbb, 0x28, 0xa5, 0x81};
    uint8_t identity[32];
    assert(p4_game_table_catalog_identity(NULL) == P4_TABLE_INVALID);
    assert(!p4_game_table_catalog_identity(identity) && !memcmp(identity, retail, sizeof retail));
    assert(action_prepares == (unsigned)expected_category);
    assert(!p4_game_tables_boot_session_end());
    assert(!p4_game_tables_battle_complete_clear());
    assert(!p4_package_reads_unobserve(observed, NULL));
    p4_package_registry_retire();
    assert(!registry && !p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    puts("game_table_boot: real contained source plans, early CPU seal, exact captured family/input products and pre-raw dependency refusal passed; no native loaded-range/admission claim");
    return 0;
}
