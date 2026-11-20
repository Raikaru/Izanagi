/* The REQUIRED p4pc/expanded-skills block over the real mod-save core, source
 * ledger and a skill view materialized from real Core/VFS synthetic banks.
 * Persona records are synthetic guest words; no gameplay claim. */
#include "p4pc/game_skills.h"
#include "p4pc/game_skills_save.h"
#include "p4pc/game_personas_native.h"
#include "p4pc/game_table_inputs.h"
#include "p4pc/game_table_sources.h"
#include "p4pc/packages.h"
#include "p4pc/gmem.h"
#include "mod_save_internal.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { ADDED = 576u, SECOND = 650u, UNREGISTERED = 700u, RETAIL_PERSONAS = 256u };
#define STOCK 0x00797F8Cu
#define STAGE 0x0079B6A8u
#define PARTY 0x00796E50u
#define COMPENDIUM 0x007981CCu
static struct Fixture {
    P4GameSkills *skills, *moved;
    P4GameTables *tables;
    P4GameText *names, *descriptions;
    P4GameTableInput *skill, *effect, *unit, *message;
} fixture;
/* The persona owner is not linked: its compendium contract is retail rows at
 * 0x007981CC plus, when a case adds one, a port-owned row for persona 256. */
static uint32_t persona_limit = RETAIL_PERSONAS, added_row;
uint32_t p4_game_personas_native_limit(void) { return persona_limit; }
uint32_t p4_game_personas_native_compendium(uint32_t id)
{
    assert(id < persona_limit);
    return id < RETAIL_PERSONAS ? COMPENDIUM + id * 0x30u : added_row;
}
static void *allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user;
    void *host = p4t_galloc(size, 16);
    if (host) *guest = P4_H2G(host);
    return host;
}
static void release(void *user, uint32_t guest, void *host, uint32_t size)
{ (void)user; assert(size && guest == P4_H2G(host)); }
static void release_inputs(void)
{
    p4_game_table_input_destroy(fixture.skill);
    p4_game_table_input_destroy(fixture.effect);
    p4_game_table_input_destroy(fixture.unit);
    p4_game_table_input_destroy(fixture.message);
    fixture.skill = fixture.effect = fixture.unit = fixture.message = NULL;
}
static void seal(P4GameSkills *skills)
{
    assert(!p4_game_skills_seal(skills, p4_game_table_input_span(fixture.skill, 1),
        p4_game_table_input_span(fixture.skill, 0), p4_game_table_input_span(fixture.effect, 0),
        p4_game_table_input_span(fixture.message, 1), fixture.tables, fixture.names, fixture.descriptions, 1));
}
static int prepare(void *user)
{
    (void)user;
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_SKILL, 1048576, &fixture.skill, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_EFFECT, 1048576, &fixture.effect, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_UNIT, 1048576, &fixture.unit, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_MSG, 1048576, &fixture.message, NULL));
    assert(!p4_game_table_input_seal(fixture.skill) && !p4_game_table_input_seal(fixture.effect) &&
        !p4_game_table_input_seal(fixture.unit) && !p4_game_table_input_seal(fixture.message));
    seal(fixture.skills);
    seal(fixture.moved);
    return 0;
}
static void publish(void *user) { (void)user; }
static void rollback(void *user) { (void)user; release_inputs(); }
static void materialize(P4GameSkills *skills)
{
    const P4GameTableInputSpan *metadata = p4_game_table_input_span(fixture.skill, 1);
    const P4GameTableInputSpan *elements = p4_game_table_input_span(fixture.skill, 0);
    const P4GameTableInputSpan *presentation = p4_game_table_input_span(fixture.effect, 0);
    const P4GameTableInputSpan *names = p4_game_table_input_span(fixture.message, 1);
    assert(!p4_game_skills_materialize(skills,
        p4_game_table_input_span_bytes(metadata), p4_game_table_input_span_size(metadata),
        p4_game_table_input_span_bytes(elements), p4_game_table_input_span_size(elements),
        p4_game_table_input_span_bytes(presentation), p4_game_table_input_span_size(presentation),
        p4_game_table_input_span_bytes(names), p4_game_table_input_span_size(names)));
}
static P4GameTableSources *ledger(const P4GameSkills *skills)
{
    P4GameTableSources *sources = p4_game_table_sources_create(); assert(sources);
    uint32_t native_source;
    assert(!p4_game_table_sources_native(sources, "fixture:skills", 1, 1, &native_source));
    assert(!p4_game_table_sources_view(sources, P4_TABLE_SOURCE_SKILLS, skills));
    P4TableSourceCursor cursor = {0}; P4TableSourceEntry entry;
    while (!p4_game_table_sources_next(P4_TABLE_SOURCE_SKILLS, skills, &cursor, &entry))
        assert(!p4_game_table_sources_bind(sources, skills, &entry, native_source));
    assert(!p4_game_table_sources_seal(sources) && !p4_game_table_sources_freeze(sources));
    return sources;
}
static void register_row(P4GameSkills *skills, uint32_t id, const char *key)
{
    P4GameSkillDesc row = {.id = id, .active = 1, .element_id = 6, .schema = 1, .revision = 3,
        .inheritance_rate = 4, .inheritance_group = 6};
    snprintf(row.key, sizeof row.key, "%s", key);
    memset(row.metadata, 0x11, sizeof row.metadata);
    assert(!p4_game_skills_register(skills, "test", &row));
}
static void register_text(uint32_t id)
{
    static const char name[] = "Added spell", description[] = "Added\nspell";
    assert(!p4_game_tables_text_register(fixture.names, "test", P4_TEXT_SKILL, id, P4_TEXT_NAME, 1,
        (const uint8_t *)name, sizeof name));
    assert(!p4_game_tables_text_register(fixture.descriptions, "test", P4_TEXT_SKILL, id, P4_TEXT_DESCRIPTION, 1,
        (const uint8_t *)description, sizeof description));
}
static void put_record(uint32_t record, uint32_t persona, uint32_t skill)
{
    uint8_t *bytes = P4_G2H(record);
    memset(bytes, 0, 0x30);
    bytes[0] = 1; bytes[2] = (uint8_t)persona; bytes[3] = (uint8_t)(persona >> 8);
    P4_G16(record + 0x0Cu) = 0x0010u; /* A retail skill in slot 0. */
    P4_G16(record + 0x0Cu + 5u * 2u) = (uint16_t)skill;
}
static uint8_t *sidecar_copy(const P4ModSaveCapture *capture)
{
    uint8_t *copy = malloc(capture->sidecar_size); assert(copy);
    memcpy(copy, capture->sidecar, capture->sidecar_size);
    return copy;
}
static int carries_block(const P4ModSaveImage *saved)
{
    char error[512] = {0};
    P4ModSaveCapture *capture = NULL;
    assert(!p4_mod_save_capture_image(saved, &capture, error, sizeof error));
    const int carried = capture != NULL;
    p4_mod_save_capture_free(capture);
    return carried;
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (strcmp(argv[1], "valid")) return 0; /* game_skills_test owns the refused seals. */
    if (p4t_guest_init() || p4t_guest_map(0x00796000u, 0x6000u)) return 77;
    P4GameTableAllocator allocator = {NULL, allocate, release};
    fixture.tables = p4_game_tables_create(); assert(fixture.tables && !p4_game_tables_freeze(fixture.tables, 1));
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4TextProfile message = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    fixture.names = p4_game_tables_text_create(&allocator, &raw);
    fixture.descriptions = p4_game_tables_text_create(&allocator, &message);
    fixture.skills = p4_game_skills_create(&allocator);
    fixture.moved = p4_game_skills_create(&allocator);
    assert(fixture.names && fixture.descriptions && fixture.skills && fixture.moved);
    register_row(fixture.skills, ADDED, "test:spell");
    register_row(fixture.skills, SECOND, "test:second");
    /* The same package after an edit that moved test:spell to another ID and
     * gave its old ID to a new skill. */
    register_row(fixture.moved, ADDED, "test:other");
    register_row(fixture.moved, ADDED + 1u, "test:spell");
    register_row(fixture.moved, SECOND, "test:second");
    register_text(ADDED); register_text(ADDED + 1u); register_text(SECOND);
    assert(!p4_game_tables_text_seal(fixture.names, 1) && !p4_game_tables_text_seal(fixture.descriptions, 1));
    static const P4PackageHandler handler = {.name = "skill-save-fixture", .prepare = prepare,
        .publish = publish, .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    assert(!p4_package_registry_publish());
    assert(!p4_game_tables_text_materialize(fixture.names) && !p4_game_tables_text_materialize(fixture.descriptions));
    materialize(fixture.skills);
    materialize(fixture.moved);
    P4GameTableSources *sources = ledger(fixture.skills), *moved_sources = ledger(fixture.moved);
    P4GameSkills *empty = p4_game_skills_create(NULL); assert(empty);
    assert(!p4_game_skills_seal(empty, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1));
    const int providers = p4_mod_save_has_providers();
    assert(p4_game_skills_save_register(NULL, sources) == P4_TABLE_INVALID);
    assert(!p4_game_skills_save_register(empty, sources) && p4_mod_save_has_providers() == providers);
    assert(!p4_game_skills_save_register(fixture.skills, sources) && p4_mod_save_has_providers());
    assert(!p4_game_skills_save_register(fixture.skills, sources));
    assert(p4_game_skills_save_register(fixture.moved, moved_sources) == P4_TABLE_CONFLICT);

    uint8_t image[4100] = {0};
    image[57] = 0x20; image[60] = 1; memset(image + 65, 0xff, 4); /* The persona fixture's header words. */
    const P4ModSaveImage saved = {.data = image, .size = sizeof image, .slot = "skill-fixture"};
    /* Retail skills, inactive stock slots and an unregistered high ID pin nothing. */
    put_record(STOCK + 2u * 0x30u, 1, 0x0020u);
    assert(!carries_block(&saved));
    put_record(STOCK + 4u * 0x30u, 1, ADDED);
    P4_G16(STOCK + 4u * 0x30u) = 0;
    assert(!carries_block(&saved));
    put_record(STOCK + 4u * 0x30u, 1, UNREGISTERED);
    assert(!carries_block(&saved));
    memset(P4_G2H(STOCK + 4u * 0x30u), 0, 0x30);
    /* Each saved record kind alone carries an added skill. */
    const uint32_t records[] = {STOCK + 11u * 0x30u, STAGE, PARTY + 8u * 0x88u + 0x54u, COMPENDIUM + 255u * 0x30u};
    for (size_t i = 0; i < sizeof records / sizeof *records; ++i) {
        put_record(records[i], 7, SECOND);
        assert(carries_block(&saved));
        memset(P4_G2H(records[i]), 0, 0x30);
        assert(!carries_block(&saved));
    }
    void *row = p4t_galloc(0x30, 16); assert(row);
    added_row = P4_H2G(row); persona_limit = RETAIL_PERSONAS + 1u;
    put_record(added_row, RETAIL_PERSONAS, ADDED);
    assert(carries_block(&saved));
    memset(row, 0, 0x30); persona_limit = RETAIL_PERSONAS;

    /* Both added skills in use, in two records. */
    put_record(STOCK + 3u * 0x30u, 9, ADDED);
    put_record(PARTY + 0x54u, 2, SECOND);
    char error[512] = {0};
    P4ModSaveCapture *capture = NULL;
    assert(!p4_mod_save_capture_image(&saved, &capture, error, sizeof error) && capture);
    P4ModSaveLoad *load = NULL;
    /* Without the provider the REQUIRED block refuses the load. */
    assert(!p4_game_skills_save_unregister());
    assert(p4_mod_save_stage_image(&saved, sidecar_copy(capture), capture->sidecar_size, &load, error, sizeof error) &&
        !load && strstr(error, "expanded-skills"));
    /* That edited package refuses it: ID 576 is now another skill. */
    error[0] = 0;
    assert(!p4_game_skills_save_register(fixture.moved, moved_sources));
    assert(p4_mod_save_stage_image(&saved, sidecar_copy(capture), capture->sidecar_size, &load, error, sizeof error) &&
        !load && strstr(error, "Required table entry identity/revision differs"));
    assert(!p4_game_skills_save_unregister());
    /* The same package loads it; the records come with the image. */
    assert(!p4_game_skills_save_register(fixture.skills, sources));
    assert(!p4_mod_save_stage_image(&saved, sidecar_copy(capture), capture->sidecar_size, &load, error, sizeof error) && load);
    p4_mod_save_load_commit(load); load = NULL;
    /* A save without the block is a save without added skills. */
    assert(!p4_mod_save_stage_image(&saved, NULL, 0, &load, error, sizeof error) && load);
    p4_mod_save_load_commit(load); load = NULL;
    p4_mod_save_capture_free(capture);
    assert(!p4_game_skills_save_unregister() && p4_mod_save_has_providers() == providers);
    assert(!p4_game_skills_save_unregister());
    assert(!p4_mod_save_reset_session(error, sizeof error));
    p4_game_table_sources_destroy(sources); p4_game_table_sources_destroy(moved_sources);
    p4_game_skills_destroy(empty); p4_game_skills_destroy(fixture.skills); p4_game_skills_destroy(fixture.moved);
    p4_game_tables_text_destroy(fixture.names); p4_game_tables_text_destroy(fixture.descriptions);
    p4_game_tables_destroy(fixture.tables);
    p4_package_registry_retire();
    puts("game_skills_save: pins every saved record's added skill by ledger identity, refuses a missing provider or a reassigned ID; no gameplay claim");
    return 0;
}
