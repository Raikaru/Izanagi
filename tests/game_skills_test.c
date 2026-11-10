/* Real Core/VFS original inputs, registry/text compiler and guest allocation.
 * Synthetic banks do not prove native learn/battle/description/save behavior. */
#include "p4pc/game_skills.h"
#include "p4pc/game_table_inputs.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned allocations, releases;
static int fail_allocate;
static struct Fixture {
    P4GameSkills *skills, *prefix;
    P4GameTables *tables;
    P4GameText *names, *descriptions;
    P4GameTableInput *skill, *effect, *unit, *message;
    const char *mode;
    unsigned published;
} fixture;
static void *allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user;
    ++allocations;
    if (fail_allocate) return NULL;
    void *host = p4t_galloc(size, 16);
    if (host) *guest = P4_H2G(host);
    return host;
}
static void release(void *user, uint32_t guest, void *host, uint32_t size)
{
    (void)user;
    assert(size && guest == P4_H2G(host));
    ++releases;
}
static void register_text(P4GameText *text, uint32_t id, uint32_t role)
{
    const char *bytes = role == P4_TEXT_NAME ? "Synthetic spell" : "Synthetic\nspell description";
    assert(p4_game_tables_text_register(text, "test", P4_TEXT_SKILL, id, role, 1,
        (const uint8_t *)bytes, strlen(bytes) + 1) == P4_TABLE_OK);
}
static void release_inputs(void)
{
    p4_game_table_input_destroy(fixture.skill);
    p4_game_table_input_destroy(fixture.effect);
    p4_game_table_input_destroy(fixture.unit);
    p4_game_table_input_destroy(fixture.message);
    fixture.skill = fixture.effect = fixture.unit = fixture.message = NULL;
}
static int prepare(void *user)
{
    assert(!user && !allocations);
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_SKILL, 1048576, &fixture.skill, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_EFFECT, 1048576, &fixture.effect, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_UNIT, 1048576, &fixture.unit, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_MSG, 1048576, &fixture.message, NULL));
    assert(!p4_game_table_input_seal(fixture.skill));
    assert(!p4_game_table_input_seal(fixture.effect));
    assert(!p4_game_table_input_seal(fixture.unit));
    assert(!p4_game_table_input_seal(fixture.message));
    const P4GameTableInputSpan *metadata = p4_game_table_input_span(fixture.skill, 1);
    const P4GameTableInputSpan *elements = p4_game_table_input_span(fixture.skill, 0);
    const P4GameTableInputSpan *presentation = p4_game_table_input_span(fixture.effect, 0);
    const P4GameTableInputSpan *original_names = p4_game_table_input_span(fixture.message, 1);
    if (strcmp(fixture.mode, "valid")) {
        assert(p4_game_skills_seal(fixture.skills, metadata, elements, presentation, original_names,
            fixture.tables, fixture.names, fixture.descriptions, 8) == P4_TABLE_INVALID);
        assert(!allocations && !p4_game_skills_semantic_revision(fixture.skills));
        return -1; /* The real Core transaction must roll back, never publish. */
    }
    assert(p4_game_skills_seal(fixture.skills, elements, metadata, presentation, original_names,
        fixture.tables, fixture.names, fixture.descriptions, 8) == P4_TABLE_INVALID);
    assert(p4_game_skills_seal(fixture.skills, metadata, elements, original_names, presentation,
        fixture.tables, fixture.names, fixture.descriptions, 8) == P4_TABLE_INVALID);
    /* Retail UNIT2 is a different pointer slot, even with matching bytes. */
    assert(p4_game_skills_seal(fixture.skills, metadata, elements, p4_game_table_input_span(fixture.unit, 2),
        original_names, fixture.tables, fixture.names, fixture.descriptions, 8) == P4_TABLE_INVALID);
    P4GameTableInput *other_skill = NULL;
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_SKILL, 1048576, &other_skill, NULL));
    assert(!p4_game_table_input_seal(other_skill));
    assert(p4_game_skills_seal(fixture.skills, metadata, p4_game_table_input_span(other_skill, 0),
        presentation, original_names, fixture.tables, fixture.names, fixture.descriptions, 8) == P4_TABLE_INVALID);
    p4_game_table_input_destroy(other_skill);
    P4TextProfile wrong = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *wrong_names = p4_game_tables_text_create(NULL, &wrong);
    assert(wrong_names && !p4_game_tables_text_seal(wrong_names, 1));
    assert(p4_game_skills_seal(fixture.skills, metadata, elements, presentation, original_names,
        fixture.tables, wrong_names, fixture.descriptions, 8) == P4_TABLE_INVALID);
    p4_game_tables_text_destroy(wrong_names);
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *unsealed = p4_game_tables_text_create(NULL, &raw);
    assert(unsealed);
    assert(p4_game_skills_seal(fixture.skills, metadata, elements, presentation, original_names,
        fixture.tables, unsealed, fixture.descriptions, 8) == P4_TABLE_NOT_READY);
    p4_game_tables_text_destroy(unsealed);
    P4GameTableAllocator allocator = {NULL, allocate, release};
    P4GameSkills *missing = p4_game_skills_create(&allocator);
    P4GameSkillDesc row = {.id = 600, .active = 1, .element_id = 6,
        .schema = 1, .revision = 1, .key = "test:missing-name"};
    assert(missing && !p4_game_skills_register(missing, "test", &row));
    assert(p4_game_skills_seal(missing, metadata, elements, presentation, original_names,
        fixture.tables, fixture.names, fixture.descriptions, 8) == P4_TABLE_NOT_FOUND);
    p4_game_skills_destroy(missing);
    missing = p4_game_skills_create(&allocator);
    row.id = 576; row.element_id = 65537u; /* Actually absent, not a reserved max-ID sentinel. */
    strcpy(row.key, "test:missing-element");
    assert(missing && !p4_game_skills_register(missing, "test", &row));
    assert(p4_game_skills_seal(missing, metadata, elements, presentation, original_names,
        fixture.tables, fixture.names, fixture.descriptions, 8) == P4_TABLE_NOT_FOUND);
    p4_game_skills_destroy(missing);
    /* UINT32_MAX is a real registered element in this fixture. CPU semantics
     * accept it; native admission still requires an actual allocator. */
    P4GameSkills *maximum = p4_game_skills_create(NULL);
    row.element_id = UINT32_MAX;
    strcpy(row.key, "test:registered-max-element");
    assert(maximum && !p4_game_skills_register(maximum, "test", &row));
    assert(!p4_game_skills_seal(maximum, metadata, elements, presentation, original_names,
        fixture.tables, fixture.names, fixture.descriptions, 8));
    const P4GameSkillView *maximum_row = p4_game_skills_find(maximum, row.id);
    assert(maximum_row && maximum_row->desc.element_id == UINT32_MAX);
    assert(p4_game_skills_native_admit(maximum) == P4_TABLE_NOT_FOUND);
    assert(p4_game_skills_materialize(maximum,
        p4_game_table_input_span_bytes(metadata), p4_game_table_input_span_size(metadata),
        p4_game_table_input_span_bytes(elements), p4_game_table_input_span_size(elements),
        p4_game_table_input_span_bytes(presentation), p4_game_table_input_span_size(presentation),
        p4_game_table_input_span_bytes(original_names), p4_game_table_input_span_size(original_names))
        == P4_TABLE_NOT_FOUND);
    assert(p4_game_skills_semantic_revision(maximum) == 8 && !p4_game_skills_revision(maximum));
    assert(!p4_game_skills_metadata(maximum) && !p4_game_skills_elements(maximum));
    assert(!p4_game_skills_presentation(maximum) && !p4_game_skills_name(maximum, row.id));
    assert(!p4_game_skills_description(maximum, row.id) && !allocations);
    assert(p4_game_skills_runtime_use(maximum, row.id) == P4_TABLE_NOT_READY);
    p4_game_skills_destroy(maximum);
    assert(!p4_game_skills_seal(fixture.skills, metadata, elements, presentation, original_names,
        fixture.tables, fixture.names, fixture.descriptions, 8));
    assert(!p4_game_skills_seal(fixture.skills, metadata, elements, presentation, original_names,
        fixture.tables, fixture.names, fixture.descriptions, 8));
    assert(p4_game_skills_seal(fixture.skills, metadata, elements, presentation, original_names,
        fixture.tables, fixture.names, fixture.descriptions, 9) == P4_TABLE_CONFLICT);
    assert(!allocations && p4_game_skills_semantic_revision(fixture.skills) == 8);
    /* Pure preparation cannot fabricate runtime source authority. */
    assert(p4_game_skills_validate_runtime_sources(fixture.skills) == P4_TABLE_NOT_READY);
    assert(p4_game_skills_runtime_use(fixture.skills, 576) == P4_TABLE_NOT_READY);
    assert(p4_game_skills_bind_runtime_use(fixture.skills, NULL, NULL, NULL) == P4_TABLE_FROZEN);
    assert(!p4_game_skills_revision(fixture.skills) && !p4_game_skills_metadata(fixture.skills));
    assert(!p4_game_skills_name(fixture.skills, 576) && !p4_game_skills_description(fixture.skills, 576));
    assert(p4_game_skills_text_view(fixture.skills, P4_TEXT_NAME) == fixture.names);
    assert(p4_game_skills_text_view(fixture.skills, P4_TEXT_DESCRIPTION) == fixture.descriptions);
    assert(!p4_game_skills_text_view(fixture.skills, 2));
    for (unsigned wide = 0; wide < 2; ++wide) {
        P4GameSkills *cpu = p4_game_skills_create(NULL);
        P4GameSkillDesc cpu_row = {.id = wide ? 578u : 581u, .active = 1,
            .element_id = wide ? 65536u : 6u, .schema = 1, .revision = 1,
            .key = "test:cpu-only"};
        assert(cpu && !p4_game_skills_register(cpu, "test", &cpu_row));
        assert(!p4_game_skills_seal(cpu, metadata, elements, presentation, original_names,
            fixture.tables, fixture.names, fixture.descriptions, 7));
        assert(p4_game_skills_semantic_revision(cpu) == 7 && !p4_game_skills_revision(cpu));
        const P4GameSkillView *view = p4_game_skills_find(cpu, cpu_row.id);
        assert(view && view->desc.element_id == cpu_row.element_id);
        assert(p4_game_skills_native_admit(cpu) == P4_TABLE_NOT_FOUND);
        /* A negative phase probe only: these source bytes are not presented
         * as a loaded-native proof, and no guest address can be produced. */
        assert(p4_game_skills_materialize(cpu,
            p4_game_table_input_span_bytes(metadata), p4_game_table_input_span_size(metadata),
            p4_game_table_input_span_bytes(elements), p4_game_table_input_span_size(elements),
            p4_game_table_input_span_bytes(presentation), p4_game_table_input_span_size(presentation),
            p4_game_table_input_span_bytes(original_names), p4_game_table_input_span_size(original_names))
            == P4_TABLE_NOT_FOUND);
        assert(!p4_game_skills_revision(cpu) && !p4_game_skills_metadata(cpu));
        assert(!p4_game_skills_elements(cpu) && !p4_game_skills_presentation(cpu));
        assert(!p4_game_skills_name(cpu, cpu_row.id) && !p4_game_skills_description(cpu, cpu_row.id));
        assert(p4_game_skills_runtime_use(cpu, cpu_row.id) == P4_TABLE_NOT_READY);
        assert(p4_game_skills_register(cpu, "test", &cpu_row) == P4_TABLE_FROZEN);
        assert(!allocations);
        p4_game_skills_destroy(cpu);
    }
    fixture.prefix = p4_game_skills_create(NULL);
    assert(fixture.prefix && !p4_game_skills_seal(fixture.prefix, metadata, elements, presentation,
        original_names, NULL, NULL, NULL, 2));
    assert(!p4_game_skills_native_admit(fixture.prefix));
    P4GameSkillDefinition definition;
    assert(!p4_game_skills_definition_find(fixture.prefix, 439, &definition));
    assert(definition.active && definition.metadata && definition.retail_element && !definition.authored);
    assert(!p4_game_skills_definition_find(fixture.prefix, 440, &definition));
    assert(!definition.active && !definition.metadata && definition.retail_element && !definition.authored);
    assert(!p4_game_skills_definition_find(fixture.prefix, 575, &definition));
    assert(!definition.active && !definition.metadata && definition.retail_element);
    assert(p4_game_skills_definition_find(fixture.prefix, 576, &definition) == P4_TABLE_NOT_FOUND);
    assert(!p4_game_skills_definition_find(fixture.skills, 576, &definition));
    assert(definition.active && definition.metadata && !definition.retail_element && definition.authored);
    assert(definition.authored->element_id == 256);
    assert(!p4_game_skills_definition_find(fixture.skills, 578, &definition));
    assert(definition.authored && definition.authored->element_id == 65536u);
    assert(!p4_game_skills_definition_find(fixture.skills, 579, &definition));
    assert(definition.authored && definition.authored->element_id == 0x80000000u);
    assert(!p4_game_skills_definition_find(fixture.skills, 580, &definition));
    assert(definition.authored && definition.authored->element_id == UINT32_MAX);
    assert(!p4_game_skills_definition_find(fixture.skills, 4095, &definition) && !definition.active);
    assert(p4_game_skills_definition_find(fixture.skills, 588, &definition) == P4_TABLE_NOT_FOUND);
    assert(p4_game_skills_definition_find(fixture.skills, 4096, &definition) == P4_TABLE_NOT_FOUND);
    assert(!allocations);
    return 0;
}
static void publish(void *user) { assert(!user && !allocations); ++fixture.published; }
static void rollback(void *user) { assert(!user); release_inputs(); }
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (p4t_guest_init()) return 77;
    P4GameTableAllocator allocator = {NULL, allocate, release};
    P4GameSkills *empty = p4_game_skills_create(&allocator);
    assert(empty && !p4_game_skills_native_admit(empty));
    assert(p4_game_skills_validate_runtime_sources(empty) == P4_TABLE_NOT_READY);
    assert(p4_game_skills_bind_runtime_use(empty, NULL, NULL, NULL) == P4_TABLE_INVALID);
    P4GameSkillDefinition definition;
    assert(p4_game_skills_definition_find(empty, 0, &definition) == P4_TABLE_NOT_READY);
    assert(!p4_game_skills_seal(empty, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1));
    assert(p4_game_skills_definition_find(empty, 0, &definition) == P4_TABLE_NOT_FOUND);
    assert(!p4_game_skills_materialize(empty, NULL, 0, NULL, 0, NULL, 0, NULL, 0));
    assert(p4_game_skills_runtime_use(empty, 0) == P4_TABLE_NOT_FOUND);
    assert(!allocations && !p4_game_skills_metadata(empty) && !p4_game_skills_elements(empty));
    p4_game_skills_destroy(empty);
    P4GameTables *tables = p4_game_tables_create();
    const uint32_t sealed_references[] = {256u, 65535u, 65536u, 0x80000000u, UINT32_MAX};
    P4ElementDesc wide_element = {.family = 6, .revision = 1,
        .display_name = "Wide element", .short_name = "Wide"};
    assert(tables);
    for (size_t i = 0; i < sizeof sealed_references / sizeof *sealed_references; ++i) {
        wide_element.id = wide_element.order = sealed_references[i];
        snprintf(wide_element.key, sizeof wide_element.key, "test:element-%u", (unsigned)i);
        assert(!p4_game_tables_element(tables, "test", &wide_element));
    }
    assert(!p4_game_tables_freeze(tables, 1));
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4TextProfile message = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *names = p4_game_tables_text_create(&allocator, &raw);
    P4GameText *descriptions = p4_game_tables_text_create(&allocator, &message);
    P4GameSkills *skills = p4_game_skills_create(&allocator);
    assert(names && descriptions && skills);
    P4GameSkillDesc row = {.id = 576, .active = 1, .element_id = 6,
        .schema = 1, .revision = 3, .key = "test:spell", .inheritance_rate = 4, .inheritance_group = 6};
    P4GameSkillDesc invalid = row;
    invalid.id = 575;
    assert(p4_game_skills_register(skills, "test", &invalid) == P4_TABLE_INVALID);
    invalid.id = P4_SKILL_MAX_ID + 1u;
    assert(p4_game_skills_register(skills, "test", &invalid) == P4_TABLE_INVALID);
    invalid = row; invalid.active = 2;
    assert(p4_game_skills_register(skills, "test", &invalid) == P4_TABLE_INVALID);
    invalid = row; invalid.inheritance_group = P4_SKILL_INHERITANCE_GROUP_COUNT;
    assert(p4_game_skills_register(skills, "test", &invalid) == P4_TABLE_INVALID);
    invalid.inheritance_group = UINT8_MAX;
    assert(p4_game_skills_register(skills, "test", &invalid) == P4_TABLE_INVALID);
    /* The effect source is a retail active skill: never a passive or added one. */
    invalid = row; invalid.effect_source = P4_SKILL_RETAIL_ACTIVE_COUNT;
    assert(p4_game_skills_register(skills, "test", &invalid) == P4_TABLE_INVALID);
    invalid.effect_source = P4_SKILL_RETAIL_COUNT;
    assert(p4_game_skills_register(skills, "test", &invalid) == P4_TABLE_INVALID);
    assert(p4_game_skills_register(skills, "other", &row) == P4_TABLE_INVALID);
    /* Full-width leaves are admitted with a real allocator; activating them
     * is the element owner's participant-cohort decision, not this check. */
    P4GameSkills *semantic = p4_game_skills_create(&allocator);
    const uint32_t wide_references[] = {18u, 19u, 65535u, 65536u, 0x80000000u, UINT32_MAX};
    assert(semantic);
    for (size_t i = 0; i < sizeof wide_references / sizeof *wide_references; ++i) {
        P4GameSkillDesc wide = row;
        wide.id += (uint32_t)i; wide.element_id = wide_references[i];
        snprintf(wide.key, sizeof wide.key, "test:semantic-%u", (unsigned)i);
        assert(!p4_game_skills_register(semantic, "test", &wide));
        assert(!p4_game_skills_native_admit(semantic));
    }
    p4_game_skills_destroy(semantic);
    for (uint32_t i = 0; i < 12; ++i) {
        row.id = 576 + i;
        row.element_id = i < 5 ? sealed_references[i] : (i < 6 ? 6u : 7u);
        row.inheritance_group = i < 6 ? 6 : 7;
        snprintf(row.key, sizeof row.key, "test:spell-%u", i);
        memset(row.metadata, (int)(i + 1), sizeof row.metadata);
        memset(row.presentation, (int)(i + 21), sizeof row.presentation);
        assert(!p4_game_skills_register(skills, "test", &row));
        assert(p4_game_skills_register(skills, "test", &row) == P4_TABLE_CONFLICT);
        register_text(names, row.id, P4_TEXT_NAME);
        register_text(descriptions, row.id, P4_TEXT_DESCRIPTION);
    }
    row.id = 4095; row.active = 0; row.inheritance_group = 18;
    strcpy(row.key, "test:passive-ceiling");
    assert(!p4_game_skills_register(skills, "test", &row));
    register_text(names, row.id, P4_TEXT_NAME);
    register_text(descriptions, row.id, P4_TEXT_DESCRIPTION);
    assert(!p4_game_skills_at(skills, 0) && !p4_game_skills_find(skills, row.id));
    assert(!p4_game_tables_text_seal(names, 1) && !p4_game_tables_text_seal(descriptions, 1));
    fixture.skills = skills; fixture.tables = tables; fixture.names = names;
    fixture.descriptions = descriptions; fixture.mode = argv[1];
    static const P4PackageHandler handler = {.name = "skill-input-fixture", .prepare = prepare,
        .publish = publish, .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    int result = p4_package_registry_publish();
    if (strcmp(fixture.mode, "valid")) {
        assert(result && !fixture.published && !allocations && !p4_game_skills_semantic_revision(skills));
    } else {
        assert(!result && fixture.published == 1 && !allocations);
        uint8_t metadata[440 * 40], elements[576 * 2], presentation[440 * 4], original_names[576 * 19];
        memset(metadata, 0xa5, sizeof metadata); memset(elements, 0x63, sizeof elements);
        memset(presentation, 0x39, sizeof presentation); memset(original_names, 0x4e, sizeof original_names);
        assert(p4_game_skills_materialize(skills, metadata, sizeof metadata, elements, sizeof elements,
            presentation, sizeof presentation, original_names, sizeof original_names) == P4_TABLE_NOT_READY);
        assert(!allocations);
        assert(!p4_game_skills_materialize(fixture.prefix, metadata, sizeof metadata, elements, sizeof elements,
            presentation, sizeof presentation, original_names, sizeof original_names));
        assert(p4_game_skills_revision(fixture.prefix) == 2 && !p4_game_skills_metadata(fixture.prefix));
        assert(!allocations);
        assert(!p4_game_tables_text_materialize(names) && !p4_game_tables_text_materialize(descriptions));
        unsigned before = allocations;
        original_names[0] ^= 1;
        assert(p4_game_skills_materialize(skills, metadata, sizeof metadata, elements, sizeof elements,
            presentation, sizeof presentation, original_names, sizeof original_names) == P4_TABLE_CONFLICT);
        original_names[0] ^= 1;
        assert(allocations == before && !p4_game_skills_revision(skills));
        fail_allocate = 1;
        assert(p4_game_skills_materialize(skills, metadata, sizeof metadata, elements, sizeof elements,
            presentation, sizeof presentation, original_names, sizeof original_names) == P4_TABLE_NOMEM);
        assert(p4_game_skills_semantic_revision(skills) == 8 && !p4_game_skills_revision(skills));
        assert(p4_game_skills_register(skills, "test", &row) == P4_TABLE_FROZEN);
        fail_allocate = 0;
        assert(!p4_game_skills_materialize(skills, metadata, sizeof metadata, elements, sizeof elements,
            presentation, sizeof presentation, original_names, sizeof original_names));
        assert(p4_game_skills_revision(skills) == 8 && p4_game_skills_count(skills) == 13);
        assert(p4_game_skills_runtime_use(skills, 576) == P4_TABLE_NOT_READY);
        assert(!p4_game_skills_native_admit(skills));
        assert(p4_game_skills_limit(skills) == 4096);
        uint32_t base = p4_game_skills_metadata(skills), eb = p4_game_skills_elements(skills);
        uint32_t pb = p4_game_skills_presentation(skills);
        assert(!(base & 15) && !(eb & 15) && !(pb & 15));
        assert(!memcmp(P4_G2H(base), metadata, sizeof metadata));
        assert(!memcmp(P4_G2H(eb), elements, sizeof elements));
        assert(!memcmp(P4_G2H(pb), presentation, sizeof presentation));
        before = allocations;
        assert(!p4_game_skills_materialize(skills, metadata, sizeof metadata, elements, sizeof elements,
            presentation, sizeof presentation, original_names, sizeof original_names));
        metadata[0] ^= 1;
        assert(p4_game_skills_validate_original(skills, metadata, sizeof metadata, elements, sizeof elements,
            presentation, sizeof presentation, original_names, sizeof original_names) == P4_TABLE_CONFLICT);
        metadata[0] ^= 1;
        assert(p4_game_skills_validate_original(skills, metadata, sizeof metadata, elements, sizeof elements - 2,
            presentation, sizeof presentation, original_names, sizeof original_names) == P4_TABLE_CONFLICT);
        assert(allocations == before && p4_game_skills_metadata(skills) == base);
        for (uint32_t i = 0; i < 12; ++i) {
            const P4GameSkillView *view = p4_game_skills_at(skills, i);
            assert(view && view == p4_game_skills_find(skills, 576 + i));
            assert(!strcmp(view->owner, "test") && view->desc.schema == 1 && view->desc.revision == 3);
            assert(view->desc.active && view->desc.inheritance_group == (i < 6 ? 6u : 7u));
            assert(view->desc.element_id == (i < 5 ? sealed_references[i] : (i < 6 ? 6u : 7u)));
            assert(*(const uint8_t *)P4_G2H(base + view->desc.id * 40) == i + 1);
            assert(*(const uint8_t *)P4_G2H(pb + view->desc.id * 4) == i + 21);
            uint8_t legacy = *(const uint8_t *)P4_G2H(eb + view->desc.id * 2);
            assert(legacy == (i < 5 ? P4_SKILL_LEGACY_ELEMENT_INVALID : (i < 6 ? 6u : 7u)));
            if (!i) assert(legacy != (view->desc.element_id & 255u));
            assert(!strcmp(P4_G2H(p4_game_skills_name(skills, view->desc.id)), "Synthetic spell"));
            assert(!strcmp(P4_G2H(p4_game_skills_description(skills, view->desc.id)), "Synthetic\nspell description"));
        }
        assert(!p4_game_skills_find(skills, 588) && !p4_game_skills_find(skills, 4096));
        assert(!p4_game_skills_find(skills, 4095)->desc.active);
        assert(p4_game_skills_find(skills, 4095)->desc.inheritance_group == 18);
        assert(releases + 4 == allocations); /* Three live allocations plus one failed attempt. */
    }
    p4_game_skills_destroy(fixture.prefix);
    p4_game_skills_destroy(skills);
    p4_game_tables_text_destroy(descriptions);
    p4_game_tables_text_destroy(names);
    p4_game_tables_destroy(tables);
    p4_package_registry_retire();
    release_inputs();
    assert(releases == allocations - (!strcmp(fixture.mode, "valid") ? 1u : 0u));
    puts("game_skills: real CPU source seal/row proof and late guest binding exercised; not native gameplay acceptance");
    return 0;
}
