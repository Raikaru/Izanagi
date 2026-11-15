/* Real native persona lifecycle over actual prepared synthetic source inputs:
 * staging, the shared packed observer with selected-input receipts, completion
 * relocation of the six banks, reload and session restore in guest memory.
 * No native image, battle or gameplay admission is claimed. */
#include "p4pc/game_personas.h"
#include "p4pc/game_personas_native.h"
#include "p4pc/game_personas_save.h"
#include "mod_save_internal.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/game_table_inputs.h"
#include "p4pc/game_table_inputs_use.h"
#include "p4pc/game_table_admission.h"
#include "p4pc/game_table_admission_native.h"
#include "p4pc/game_table_extents.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RETAIL P4_PERSONA_RETAIL_ROWS
#define ARENA 0x01000000u
#define ARENA_SIZE 0xa0000u
/* Loads use the first 0x60000 bytes; relocated banks come from the rest, so
 * every guest address stays in the 32 MiB RAM the extent readers accept. */
#define HEAP (ARENA + 0x60000u)
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected persona lifecycle runtime recovery: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ (void)provider; fprintf(stderr, "Unexpected persona lifecycle source use: %s\n", reason); abort(); }
/* The lifecycle witness must never enter unrelated game/renderer code. */
uint32_t RpRandom(void) { abort(); }
void p4_game_tables_text_draw(uint32_t stream, float x, float y, float depth,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline)
{
    (void)stream; (void)x; (void)y; (void)depth; (void)color; (void)style; (void)effect; (void)flags; (void)multiline;
    abort();
}

static unsigned allocations, releases, authored_uses;
static struct Fixture {
    P4GamePersonas *personas, *empty, *unbound;
    P4GameText *names;
    P4GameTableInput *inputs[4];
} fixture;
static const P4GameTableInputKind kinds[4] = {
    P4_TABLE_INPUT_PERSONA, P4_TABLE_INPUT_MODEL, P4_TABLE_INPUT_MSG, P4_TABLE_INPUT_UNIT
};
/* SLUS_217.82 005f66c0 descriptor slots of the four files. */
static const uint32_t file_slots[4][6] = {
    {0x7644c4, 0x7644cc, 0x7644d4, 0x7644d8, 0x7644dc, 0x7644e0},
    {0x7644b0, 0x7644bc, 0x7644d0},
    {0x76453c, 0x764538, 0x764534, 0x764530, 0x76452c},
    {0x7644b4, 0x7644b8, 0x7644c8}
};
static const unsigned file_blocks[4] = {6, 3, 5, 3};
static const uint32_t bank_slots[6] = {0x007644c4u, 0x007644ccu, 0x007644d0u, 0x007644e0u, 0x00764530u, 0x007644c8u};
static const uint32_t bank_strides[6] = {P4_PERSONA_METADATA_SIZE, P4_PERSONA_GROWTH_SIZE, P4_PERSONA_MODEL_SIZE,
    P4_PERSONA_CANDIDATES_SIZE, P4_PERSONA_NAME_SIZE, P4_PERSONA_AFFINITY_SIZE};
static uint32_t heap_next = HEAP;
static void *allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user;
    ++allocations;
    *guest = heap_next;
    heap_next += (size + 15u) & ~15u;
    assert(heap_next <= ARENA + ARENA_SIZE);
    return P4_G2H(*guest);
}
static void release(void *user, uint32_t guest, void *host, uint32_t size)
{
    (void)user;
    assert(size && guest == P4_H2G(host));
    ++releases;
}
static int authored_use(const P4GamePersonas *view, uint32_t id, void *context)
{
    /* Only an added persona's presentation lookup attributes its source. */
    assert(view == fixture.personas && context == &fixture && id >= RETAIL);
    ++authored_uses;
    return 0;
}
static int text_use(const P4GameText *view, uint32_t table, uint32_t id, uint32_t role, void *context)
{
    (void)view; (void)table; (void)id; (void)role; (void)context;
    fputs("Lifecycle consumed an persona name\n", stderr); abort();
}
static uint32_t read_word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void write_word(uint8_t *out, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) out[i] = (uint8_t)(value >> (i * 8u));
}
static void register_name(uint32_t id, const char *name)
{
    assert(!p4_game_tables_text_register(fixture.names, "test", P4_TEXT_PERSONA, id, P4_TEXT_NAME, 1,
        (const uint8_t *)name, strlen(name) + 1));
}
static void release_inputs(void)
{
    for (unsigned i = 0; i < 4; ++i) { p4_game_table_input_destroy(fixture.inputs[i]); fixture.inputs[i] = NULL; }
}
static int prepare(void *user)
{
    assert(!user);
    for (unsigned i = 0; i < 4; ++i) {
        assert(!p4_game_table_input_prepare(kinds[i], 1048576, &fixture.inputs[i], NULL));
        assert(!p4_game_table_input_seal(fixture.inputs[i]));
    }
    const P4GameTableInputSpan *persona0 = p4_game_table_input_span(fixture.inputs[0], 0);
    const P4GameTableInputSpan *persona1 = p4_game_table_input_span(fixture.inputs[0], 1);
    const P4GameTableInputSpan *model2 = p4_game_table_input_span(fixture.inputs[1], 2);
    const P4GameTableInputSpan *persona5 = p4_game_table_input_span(fixture.inputs[0], 5);
    const P4GameTableInputSpan *msg3 = p4_game_table_input_span(fixture.inputs[2], 3);
    const P4GameTableInputSpan *unit2 = p4_game_table_input_span(fixture.inputs[3], 2);
    assert(!p4_game_personas_seal(fixture.personas, persona0, persona1, model2, persona5, msg3, unit2, fixture.names, 1));
    assert(!p4_game_personas_seal(fixture.empty, persona0, persona1, model2, persona5, msg3, unit2, NULL, 1));
    assert(!p4_game_personas_seal(fixture.unbound, persona0, persona1, model2, persona5, msg3, unit2, NULL, 1));
    return 0;
}
static void publish(void *user) { (void)user; }
static void rollback(void *user) { (void)user; release_inputs(); }
/* The unchanged loader: shared admission and observer, then the GP stores. */
static uint32_t load(unsigned file, uint32_t arena)
{
    uint8_t descriptor[0xa4] = {0};
    for (unsigned block = 0; block < file_blocks[file]; ++block)
        write_word(descriptor + block * 8u + 4u, file_slots[file][block]);
    const char *path = p4_game_table_input_label(kinds[file]);
    const uint8_t *bytes = p4_game_table_input_bytes(fixture.inputs[file]);
    const uint32_t size = (uint32_t)p4_game_table_input_size(fixture.inputs[file]);
    /* The fixture's sealed input stands in for the loader's writable buffer. */
    p4_game_table_admission_packed(descriptor, path, (uint8_t *)bytes, size, p4_game_tables_observe_packed);
    uint32_t offset = 0;
    for (unsigned block = 0; block < file_blocks[file]; ++block) {
        const uint32_t block_size = read_word(bytes + offset), slot = file_slots[file][block];
        memcpy(P4_G2H(arena), bytes + offset + 4, block_size);
        P4_G32(slot) = arena;
        p4_game_table_extent_note(slot, arena, block_size);
        arena += (block_size + 15u) & ~15u;
        offset += (block_size + 4u + 15u) & ~15u;
    }
    return arena;
}
static void load_files(unsigned files, uint32_t arena)
{
    for (unsigned file = 0; file < files; ++file) arena = load(file, arena);
    assert(arena <= ARENA + ARENA_SIZE);
    P4_GPDATA(uint32_t, 0xb438) = 8;
}
static void loaded_slots(uint32_t out[6])
{
    for (unsigned bank = 0; bank < 6; ++bank) out[bank] = P4_G32(bank_slots[bank]);
}
static void check_relocated(void)
{
    P4GamePersonas *personas = fixture.personas;
    const uint32_t bases[6] = {p4_game_personas_metadata(personas), p4_game_personas_growth(personas),
        p4_game_personas_model(personas), p4_game_personas_candidates(personas), p4_game_personas_names(personas),
        p4_game_personas_affinity(personas)};
    for (unsigned bank = 0; bank < 6; ++bank) {
        uint32_t base, rows;
        assert(bases[bank] && P4_G32(bank_slots[bank]) == bases[bank]);
        /* The relocating commit published its extent with the pointer store. */
        assert(p4_game_table_extent_rows(bank_slots[bank], bank_strides[bank], &base, &rows));
        assert(base == bases[bank] && rows == RETAIL + 2u);
    }
    assert(p4_game_personas_active() == personas && p4_game_personas_expanded());
    assert(p4_game_personas_native_limit() == RETAIL + 2u);
    static const uint8_t fills[6] = {0x55, 0x41, 0x4d, 0xa1, 0x4e, 0xa7};
    for (unsigned bank = 0; bank < 6; ++bank) {
        const uint8_t *bank_bytes = P4_G2H(bases[bank]);
        for (uint32_t i = 0; i < RETAIL * bank_strides[bank]; ++i) assert(bank_bytes[i] == fills[bank]);
    }
    for (uint32_t i = 0; i < 2; ++i) {
        const P4GamePersonaDesc *row = &p4_game_personas_find(personas, RETAIL + i)->desc;
        assert(!memcmp((const uint8_t *)P4_G2H(bases[0]) + row->id * P4_PERSONA_METADATA_SIZE, row->metadata, P4_PERSONA_METADATA_SIZE));
        assert(!memcmp((const uint8_t *)P4_G2H(bases[3]) + row->id * P4_PERSONA_CANDIDATES_SIZE, row->candidates, P4_PERSONA_CANDIDATES_SIZE));
        assert(!memcmp((const uint8_t *)P4_G2H(bases[5]) + row->id * P4_PERSONA_AFFINITY_SIZE, row->affinity, P4_PERSONA_AFFINITY_SIZE));
    }
    assert(!strcmp((const char *)P4_G2H(bases[4]) + RETAIL * P4_PERSONA_NAME_SIZE, "Shadow"));
    assert(!strcmp((const char *)P4_G2H(bases[4]) + (RETAIL + 1u) * P4_PERSONA_NAME_SIZE, "Mimic"));
}
static void detach(int image_live)
{
    assert(!p4_game_personas_session_end(image_live));
    assert(!p4_game_tables_battle_complete_clear());
    assert(!p4_game_personas_active() && p4_game_personas_native_limit() == RETAIL && !p4_game_personas_expanded());
}
static uint8_t *row_of(uint32_t id) { return P4_G2H(p4_game_personas_native_compendium(id)); }
static void put_record(uint8_t *record, uint32_t id, uint8_t level)
{
    memset(record, 0, 0x30);
    record[0] = 1; record[2] = (uint8_t)id; record[3] = (uint8_t)(id >> 8); record[4] = level;
}
static uint8_t *sidecar_copy(const P4ModSaveCapture *capture)
{
    uint8_t *copy = malloc(capture->sidecar_size); assert(copy);
    memcpy(copy, capture->sidecar, capture->sidecar_size);
    return copy;
}
/* Added compendium rows and the REQUIRED persona block over the real mod-save
 * core and source ledger; the retail image is a synthetic record stream. */
static void compendium_case(void)
{
    assert(p4_game_personas_native_compendium(5) == 0x007981CCu + 5u * 0x30u);
    assert(p4_game_personas_native_compendium(RETAIL + 1u) == p4_game_personas_native_compendium(RETAIL) + 0x30u);
    for (unsigned i = 0; i < 0x30u; ++i) assert(!row_of(RETAIL)[i] && !row_of(RETAIL + 1u)[i]);
    put_record(row_of(RETAIL + 1u), RETAIL + 1u, 9);
    p4_game_personas_native_compendium_clear();
    assert(!row_of(RETAIL + 1u)[0] && !row_of(RETAIL + 1u)[4]);

    P4GameTableSources *sources = p4_game_table_sources_create(); assert(sources);
    uint32_t native_source;
    assert(!p4_game_table_sources_native(sources, "fixture:personas", 1, 1, &native_source));
    assert(!p4_game_table_sources_view(sources, P4_TABLE_SOURCE_PERSONAS, fixture.personas));
    P4TableSourceCursor cursor = {0}; P4TableSourceEntry entry;
    while (!p4_game_table_sources_next(P4_TABLE_SOURCE_PERSONAS, fixture.personas, &cursor, &entry))
        assert(!p4_game_table_sources_bind(sources, fixture.personas, &entry, native_source));
    assert(!p4_game_table_sources_seal(sources) && !p4_game_table_sources_freeze(sources));
    const int providers = p4_mod_save_has_providers();
    assert(!p4_game_personas_save_register(fixture.empty, sources) && p4_mod_save_has_providers() == providers);
    assert(!p4_game_personas_save_register(fixture.personas, sources) && p4_mod_save_has_providers());

    uint8_t image[4100] = {0};
    write_word(image + 56, 0x2000); write_word(image + 60, 1); write_word(image + 65, UINT32_MAX);
    const P4ModSaveImage saved = {.data = image, .size = sizeof image, .slot = "persona-fixture"};
    char error[512] = {0};
    P4ModSaveCapture *capture = NULL;
    /* Nothing uses an added persona: the block is omitted. */
    assert(!p4_mod_save_capture_image(&saved, &capture, error, sizeof error) && !capture);
    /* A registered added row and an added persona in an active stock slot. */
    put_record(row_of(RETAIL + 1u), RETAIL + 1u, 9);
    uint8_t *stock = P4_G2H(0x00797F8Cu + 3u * 0x30u);
    put_record(stock, RETAIL, 4);
    assert(!p4_mod_save_capture_image(&saved, &capture, error, sizeof error) && capture);
    p4_game_personas_native_compendium_clear();
    P4ModSaveLoad *load = NULL;
    /* Without the provider the REQUIRED block refuses the load (re-registration
     * is possible only while no committed load holds the block). */
    assert(!p4_game_personas_save_unregister());
    assert(p4_mod_save_stage_image(&saved, sidecar_copy(capture), capture->sidecar_size, &load, error, sizeof error) && !load);
    assert(!p4_game_personas_save_register(fixture.personas, sources));
    assert(!p4_mod_save_stage_image(&saved, sidecar_copy(capture), capture->sidecar_size, &load, error, sizeof error) && load);
    assert(!row_of(RETAIL + 1u)[0]); /* Nothing changes before commit. */
    p4_mod_save_load_abort(load); load = NULL;
    assert(!row_of(RETAIL + 1u)[0]);
    assert(!p4_mod_save_stage_image(&saved, sidecar_copy(capture), capture->sidecar_size, &load, error, sizeof error) && load);
    p4_mod_save_load_commit(load); load = NULL;
    assert(row_of(RETAIL + 1u)[0] == 1 && row_of(RETAIL + 1u)[4] == 9 && !row_of(RETAIL)[0]);
    /* A save without the block loads an empty added compendium. */
    assert(!p4_mod_save_stage_image(&saved, NULL, 0, &load, error, sizeof error) && load);
    p4_mod_save_load_commit(load); load = NULL;
    assert(!row_of(RETAIL + 1u)[0]);
    p4_mod_save_capture_free(capture); capture = NULL;
    memset(stock, 0, 0x30);
    /* The saved staged record (datStageRecord) alone also carries an added ID. */
    P4_G16(0x0079B6A4u) = (uint16_t)RETAIL;
    assert(!p4_mod_save_capture_image(&saved, &capture, error, sizeof error) && capture);
    p4_mod_save_capture_free(capture); capture = NULL;
    P4_G16(0x0079B6A4u) = 0;
    assert(!p4_mod_save_capture_image(&saved, &capture, error, sizeof error) && !capture);
    assert(!p4_game_personas_save_unregister() && p4_mod_save_has_providers() == providers);
    assert(!p4_mod_save_reset_session(error, sizeof error));
    p4_game_table_sources_destroy(sources);
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (strcmp(argv[1], "valid")) return 0; /* game_personas_test owns the refused seals. */
    if (p4t_guest_map(0x00764000u, 0x2000u) || p4t_guest_map(0x00797000u, 0x5000u) || p4t_guest_map(ARENA, ARENA_SIZE))
        return 77;
    P4GameTableAllocator allocator = {NULL, allocate, release};
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    fixture.names = p4_game_tables_text_create(NULL, &raw);
    assert(fixture.names && !p4_game_tables_text_bind_runtime_use(fixture.names, text_use, &fixture));
    register_name(RETAIL, "Shadow");
    register_name(RETAIL + 1u, "Mimic");
    assert(!p4_game_tables_text_seal(fixture.names, 1));
    fixture.personas = p4_game_personas_create(&allocator);
    fixture.empty = p4_game_personas_create(&allocator);
    fixture.unbound = p4_game_personas_create(NULL);
    assert(fixture.personas && fixture.empty && fixture.unbound);
    assert(!p4_game_personas_bind_runtime_use(fixture.personas, p4_game_table_input_semantic_use, authored_use, &fixture));
    assert(!p4_game_personas_bind_runtime_use(fixture.empty, p4_game_table_input_semantic_use, NULL, NULL));
    for (uint32_t i = 0; i < 2; ++i) {
        P4GamePersonaDesc row = {.id = RETAIL + i, .model_source = 3 + i, .schema = 1, .revision = 1};
        snprintf(row.key, sizeof row.key, "test:persona-%u", (unsigned)i);
        memset(row.metadata, (int)(i + 1), sizeof row.metadata);
        memset(row.candidates, (int)(i + 31), sizeof row.candidates);
        memset(row.affinity, (int)(i + 41), sizeof row.affinity);
        assert(!p4_game_personas_register(fixture.personas, "test", &row));
    }
    static const P4PackageHandler handler = {.name = "persona-native-fixture", .prepare = prepare,
        .publish = publish, .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    assert(!p4_package_registry_publish() && !allocations);
    assert(p4_game_personas_boot_stage(NULL) == P4_TABLE_INVALID);
    assert(p4_game_personas_boot_unstage(NULL) == P4_TABLE_INVALID);
    assert(!p4_game_personas_boot_unstage(fixture.personas));
    /* Runtime-source transport is a staging prerequisite. */
    assert(p4_game_personas_boot_stage(fixture.unbound) == P4_TABLE_NOT_READY);
    P4GameTableInput *owned[4] = {fixture.inputs[0], fixture.inputs[1], fixture.inputs[2], fixture.inputs[3]};
    assert(!p4_game_table_admission_stage(owned, 4));

    /* Retail identity: an empty snapshot registers no participant, observes
     * nothing and leaves every loaded slot in place. */
    assert(!p4_game_personas_boot_stage(fixture.empty) && !p4_game_personas_packed_pending());
    load_files(4, ARENA);
    uint32_t loaded[6], now[6];
    loaded_slots(loaded);
    assert(!p4_game_tables_battle_complete());
    loaded_slots(now);
    assert(!memcmp(loaded, now, sizeof loaded) && !allocations && p4_game_personas_native_limit() == RETAIL);
    detach(1);

    /* Exact early rollback and ownership conflicts. */
    assert(!p4_game_personas_boot_stage(fixture.personas) && p4_game_personas_packed_pending());
    assert(!p4_game_personas_boot_stage(fixture.personas));
    assert(p4_game_personas_boot_stage(fixture.empty) == P4_TABLE_CONFLICT);
    assert(p4_game_personas_boot_unstage(fixture.empty) == P4_TABLE_CONFLICT);
    assert(!p4_game_personas_boot_unstage(fixture.personas) && !p4_game_personas_packed_pending());
    assert(!p4_game_tables_battle_complete_clear());

    /* A load that did not deliver every bank cannot complete. */
    assert(!p4_game_personas_boot_stage(fixture.personas));
    load_files(2, ARENA);
    assert(p4_game_personas_boot_unstage(fixture.personas) == P4_TABLE_FROZEN);
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY);
    assert(!allocations && !p4_game_personas_active());
    detach(1);

    /* Loaded bytes that differ from the sealed source refuse before allocation. */
    assert(!p4_game_personas_boot_stage(fixture.personas));
    load_files(4, ARENA);
    loaded_slots(loaded);
    ((uint8_t *)P4_G2H(loaded[3]))[7] ^= 1;
    assert(p4_game_tables_battle_complete() == P4_TABLE_CONFLICT);
    loaded_slots(now);
    assert(!memcmp(loaded, now, sizeof loaded) && !allocations && !p4_game_personas_revision(fixture.personas));
    detach(1);

    /* Verified banks relocate together; retail prefix plus added rows. */
    assert(!p4_game_personas_boot_stage(fixture.personas));
    load_files(4, ARENA);
    assert(!p4_game_tables_battle_complete() && allocations == 1);
    check_relocated();
    /* A later load writes fresh retail bases; completion re-verifies the same
     * bytes, rebinds the relocated banks and allocates nothing. */
    load_files(4, ARENA + 0x30000u);
    loaded_slots(loaded);
    assert(!p4_game_tables_battle_complete() && allocations == 1);
    check_relocated();
    assert(p4_game_personas_boot_stage(fixture.personas) == P4_TABLE_FROZEN);
    /* Presentation follows the declared model source; retail IDs are their own. */
    assert(p4_game_personas_native_model(10) == 10 && !authored_uses);
    assert(p4_game_personas_native_model(RETAIL + 1u) == 4 && authored_uses == 1);
    compendium_case();
    /* Fusion result scans: retail IDs below 0xC0, then the added IDs. */
    assert(p4_game_personas_native_fusion_candidate(0xBFu) && !p4_game_personas_native_fusion_candidate(0xC0u));
    assert(p4_game_personas_native_fusion_next(0xBFu) == RETAIL && p4_game_personas_native_fusion_next(5u) == 6u);
    assert(p4_game_personas_native_fusion_candidate(RETAIL + 1u) && !p4_game_personas_native_fusion_candidate(RETAIL + 2u));
    /* A live image gets the latest loaded bases back. */
    detach(1);
    loaded_slots(now);
    assert(!memcmp(loaded, now, sizeof loaded));
    assert(p4_game_personas_native_model(RETAIL + 1u) == RETAIL + 1u && authored_uses == 1);
    assert(p4_game_personas_native_compendium(RETAIL + 1u) == 0x007981CCu + (RETAIL + 1u) * 0x30u);
    assert(!p4_game_personas_native_fusion_candidate(RETAIL)); /* Retail scan ends after 0xBF. */
    for (unsigned bank = 0; bank < 6; ++bank) {
        uint32_t base, rows;
        assert(p4_game_table_extent_rows(bank_slots[bank], bank_strides[bank], &base, &rows));
        assert(base == loaded[bank] && rows == RETAIL);
    }

    assert(!p4_game_table_admission_unstage(owned, 4));
    assert(!p4_game_tables_prepare_clear());
    p4_game_personas_destroy(fixture.unbound);
    p4_game_personas_destroy(fixture.empty);
    p4_game_personas_destroy(fixture.personas);
    p4_game_tables_text_destroy(fixture.names);
    p4_package_registry_retire();
    release_inputs();
    assert(releases == 1);
    puts("game_personas_native: retail identity, staged rollback, observed-bank, byte-verified relocation, reload and restore passed; no native image/gameplay admission");
    return 0;
}
