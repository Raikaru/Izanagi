/* Real native enemy lifecycle over actual prepared synthetic source inputs:
 * staging, the shared packed observer with selected-input receipts, completion
 * relocation of the five banks, reload and session restore in guest memory.
 * No native image, battle or gameplay admission is claimed. */
#include "p4pc/game_enemies.h"
#include "p4pc/game_enemies_native.h"
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

#define RETAIL P4_ENEMY_RETAIL_ROWS
#define ARENA 0x01000000u
#define ARENA_SIZE 0xa0000u
/* Loads use the first 0x60000 bytes; relocated banks come from the rest, so
 * every guest address stays in the 32 MiB RAM the extent readers accept. */
#define HEAP (ARENA + 0x60000u)
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected enemy lifecycle runtime recovery: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ (void)provider; fprintf(stderr, "Unexpected enemy lifecycle source use: %s\n", reason); abort(); }
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
    P4GameEnemies *enemies, *empty, *unbound;
    P4GameText *names;
    P4GameTableInput *inputs[4];
} fixture;
static const P4GameTableInputKind kinds[4] = {
    P4_TABLE_INPUT_UNIT, P4_TABLE_INPUT_MODEL, P4_TABLE_INPUT_AICALC, P4_TABLE_INPUT_MSG
};
/* SLUS_217.82 005f66c0 descriptor slots of the four files. */
static const uint32_t file_slots[4][11] = {
    {0x7644b4, 0x7644b8, 0x7644c8},
    {0x7644b0, 0x7644bc, 0x7644d0},
    {0x7644e4, 0x7644e8, 0x7644ec, 0x7644f0, 0x7644f4, 0x7644f8, 0x7644fc, 0x764500, 0x7644c0, 0x764518, 0x76451c},
    {0x76453c, 0x764538, 0x764534, 0x764530, 0x76452c}
};
static const unsigned file_blocks[4] = {3, 3, 11, 5};
static const uint32_t bank_slots[5] = {0x007644b4u, 0x007644b8u, 0x007644bcu, 0x007644c0u, 0x00764534u};
static const uint32_t bank_strides[5] = {P4_ENEMY_ROW_SIZE, P4_ENEMY_AFFINITY_SIZE, P4_ENEMY_MODEL_SIZE,
    P4_ENEMY_AI_SIZE, P4_ENEMY_NAME_SIZE};
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
static int authored_use(const P4GameEnemies *view, uint32_t id, void *context)
{
    /* Only an added enemy's presentation lookup attributes its source. */
    assert(view == fixture.enemies && context == &fixture && id >= RETAIL);
    ++authored_uses;
    return 0;
}
static int text_use(const P4GameText *view, uint32_t table, uint32_t id, uint32_t role, void *context)
{
    (void)view; (void)table; (void)id; (void)role; (void)context;
    fputs("Lifecycle consumed an enemy name\n", stderr); abort();
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
    assert(!p4_game_tables_text_register(fixture.names, "test", P4_TEXT_ENEMY, id, P4_TEXT_NAME, 1,
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
    const P4GameTableInputSpan *unit0 = p4_game_table_input_span(fixture.inputs[0], 0);
    const P4GameTableInputSpan *unit1 = p4_game_table_input_span(fixture.inputs[0], 1);
    const P4GameTableInputSpan *model1 = p4_game_table_input_span(fixture.inputs[1], 1);
    const P4GameTableInputSpan *ai8 = p4_game_table_input_span(fixture.inputs[2], 8);
    const P4GameTableInputSpan *msg2 = p4_game_table_input_span(fixture.inputs[3], 2);
    assert(!p4_game_enemies_seal(fixture.enemies, unit0, unit1, model1, ai8, msg2, fixture.names, 1));
    assert(!p4_game_enemies_seal(fixture.empty, unit0, unit1, model1, ai8, msg2, NULL, 1));
    assert(!p4_game_enemies_seal(fixture.unbound, unit0, unit1, model1, ai8, msg2, NULL, 1));
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
static void loaded_slots(uint32_t out[5])
{
    for (unsigned bank = 0; bank < 5; ++bank) out[bank] = P4_G32(bank_slots[bank]);
}
static void check_relocated(void)
{
    P4GameEnemies *enemies = fixture.enemies;
    const uint32_t bases[5] = {p4_game_enemies_metadata(enemies), p4_game_enemies_affinity(enemies),
        p4_game_enemies_model(enemies), p4_game_enemies_ai(enemies), p4_game_enemies_names(enemies)};
    for (unsigned bank = 0; bank < 5; ++bank) {
        uint32_t base, rows;
        assert(bases[bank] && P4_G32(bank_slots[bank]) == bases[bank]);
        /* The relocating commit published its extent with the pointer store. */
        assert(p4_game_table_extent_rows(bank_slots[bank], bank_strides[bank], &base, &rows));
        assert(base == bases[bank] && rows == RETAIL + 2u);
    }
    assert(p4_game_enemies_active() == enemies && p4_game_enemies_expanded());
    assert(p4_game_enemies_native_limit() == RETAIL + 2u);
    static const uint8_t fills[5] = {0x55, 0x41, 0x4d, 0xa1, 0x4e};
    for (unsigned bank = 0; bank < 5; ++bank) {
        const uint8_t *bank_bytes = P4_G2H(bases[bank]);
        for (uint32_t i = 0; i < RETAIL * bank_strides[bank]; ++i) assert(bank_bytes[i] == fills[bank]);
    }
    for (uint32_t i = 0; i < 2; ++i) {
        const P4GameEnemyDesc *row = &p4_game_enemies_find(enemies, RETAIL + i)->desc;
        assert(!memcmp((const uint8_t *)P4_G2H(bases[0]) + row->id * P4_ENEMY_ROW_SIZE, row->metadata, P4_ENEMY_ROW_SIZE));
        assert(!memcmp((const uint8_t *)P4_G2H(bases[3]) + row->id * P4_ENEMY_AI_SIZE, row->ai, P4_ENEMY_AI_SIZE));
    }
    assert(!strcmp((const char *)P4_G2H(bases[4]) + RETAIL * P4_ENEMY_NAME_SIZE, "Shadow"));
    assert(!strcmp((const char *)P4_G2H(bases[4]) + (RETAIL + 1u) * P4_ENEMY_NAME_SIZE, "Mimic"));
}
static void detach(int image_live)
{
    assert(!p4_game_enemies_session_end(image_live));
    assert(!p4_game_tables_battle_complete_clear());
    assert(!p4_game_enemies_active() && p4_game_enemies_native_limit() == RETAIL && !p4_game_enemies_expanded());
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (strcmp(argv[1], "valid")) return 0; /* game_enemies_test owns the refused seals. */
    if (p4t_guest_map(0x00764000u, 0x2000u) || p4t_guest_map(ARENA, ARENA_SIZE)) return 77;
    P4GameTableAllocator allocator = {NULL, allocate, release};
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    fixture.names = p4_game_tables_text_create(NULL, &raw);
    assert(fixture.names && !p4_game_tables_text_bind_runtime_use(fixture.names, text_use, &fixture));
    register_name(RETAIL, "Shadow");
    register_name(RETAIL + 1u, "Mimic");
    assert(!p4_game_tables_text_seal(fixture.names, 1));
    fixture.enemies = p4_game_enemies_create(&allocator);
    fixture.empty = p4_game_enemies_create(&allocator);
    fixture.unbound = p4_game_enemies_create(NULL);
    assert(fixture.enemies && fixture.empty && fixture.unbound);
    assert(!p4_game_enemies_bind_runtime_use(fixture.enemies, p4_game_table_input_semantic_use, authored_use, &fixture));
    assert(!p4_game_enemies_bind_runtime_use(fixture.empty, p4_game_table_input_semantic_use, NULL, NULL));
    for (uint32_t i = 0; i < 2; ++i) {
        P4GameEnemyDesc row = {.id = RETAIL + i, .model_source = 3 + i, .schema = 1, .revision = 1};
        snprintf(row.key, sizeof row.key, "test:enemy-%u", (unsigned)i);
        memset(row.metadata, (int)(i + 1), sizeof row.metadata);
        memset(row.ai, (int)(i + 31), sizeof row.ai);
        assert(!p4_game_enemies_register(fixture.enemies, "test", &row));
    }
    static const P4PackageHandler handler = {.name = "enemy-native-fixture", .prepare = prepare,
        .publish = publish, .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    assert(!p4_package_registry_publish() && !allocations);
    assert(p4_game_enemies_boot_stage(NULL) == P4_TABLE_INVALID);
    assert(p4_game_enemies_boot_unstage(NULL) == P4_TABLE_INVALID);
    assert(!p4_game_enemies_boot_unstage(fixture.enemies));
    /* Runtime-source transport is a staging prerequisite. */
    assert(p4_game_enemies_boot_stage(fixture.unbound) == P4_TABLE_NOT_READY);
    P4GameTableInput *owned[4] = {fixture.inputs[0], fixture.inputs[1], fixture.inputs[2], fixture.inputs[3]};
    assert(!p4_game_table_admission_stage(owned, 4));

    /* Retail identity: an empty snapshot registers no participant, observes
     * nothing and leaves every loaded slot in place. */
    assert(!p4_game_enemies_boot_stage(fixture.empty) && !p4_game_enemies_packed_pending());
    load_files(4, ARENA);
    uint32_t loaded[5], now[5];
    loaded_slots(loaded);
    assert(!p4_game_tables_battle_complete());
    loaded_slots(now);
    assert(!memcmp(loaded, now, sizeof loaded) && !allocations && p4_game_enemies_native_limit() == RETAIL);
    detach(1);

    /* Exact early rollback and ownership conflicts. */
    assert(!p4_game_enemies_boot_stage(fixture.enemies) && p4_game_enemies_packed_pending());
    assert(!p4_game_enemies_boot_stage(fixture.enemies));
    assert(p4_game_enemies_boot_stage(fixture.empty) == P4_TABLE_CONFLICT);
    assert(p4_game_enemies_boot_unstage(fixture.empty) == P4_TABLE_CONFLICT);
    assert(!p4_game_enemies_boot_unstage(fixture.enemies) && !p4_game_enemies_packed_pending());
    assert(!p4_game_tables_battle_complete_clear());

    /* A load that did not deliver every bank cannot complete. */
    assert(!p4_game_enemies_boot_stage(fixture.enemies));
    load_files(3, ARENA);
    assert(p4_game_enemies_boot_unstage(fixture.enemies) == P4_TABLE_FROZEN);
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY);
    assert(!allocations && !p4_game_enemies_active());
    detach(1);

    /* Loaded bytes that differ from the sealed source refuse before allocation. */
    assert(!p4_game_enemies_boot_stage(fixture.enemies));
    load_files(4, ARENA);
    loaded_slots(loaded);
    ((uint8_t *)P4_G2H(loaded[3]))[7] ^= 1;
    assert(p4_game_tables_battle_complete() == P4_TABLE_CONFLICT);
    loaded_slots(now);
    assert(!memcmp(loaded, now, sizeof loaded) && !allocations && !p4_game_enemies_revision(fixture.enemies));
    detach(1);

    /* Verified banks relocate together; retail prefix plus added rows. */
    assert(!p4_game_enemies_boot_stage(fixture.enemies));
    load_files(4, ARENA);
    assert(!p4_game_tables_battle_complete() && allocations == 1);
    check_relocated();
    /* A later load writes fresh retail bases; completion re-verifies the same
     * bytes, rebinds the relocated banks and allocates nothing. */
    load_files(4, ARENA + 0x30000u);
    loaded_slots(loaded);
    assert(!p4_game_tables_battle_complete() && allocations == 1);
    check_relocated();
    assert(p4_game_enemies_boot_stage(fixture.enemies) == P4_TABLE_FROZEN);
    /* Presentation follows the declared model source; retail IDs are their own. */
    assert(p4_game_enemies_native_model(10) == 10 && !authored_uses);
    assert(p4_game_enemies_native_model(RETAIL + 1u) == 4 && authored_uses == 1);
    /* A live image gets the latest loaded bases back. */
    detach(1);
    loaded_slots(now);
    assert(!memcmp(loaded, now, sizeof loaded));
    assert(p4_game_enemies_native_model(RETAIL + 1u) == RETAIL + 1u && authored_uses == 1);
    for (unsigned bank = 0; bank < 5; ++bank) {
        uint32_t base, rows;
        assert(p4_game_table_extent_rows(bank_slots[bank], bank_strides[bank], &base, &rows));
        assert(base == loaded[bank] && rows == RETAIL);
    }

    assert(!p4_game_table_admission_unstage(owned, 4));
    assert(!p4_game_tables_prepare_clear());
    p4_game_enemies_destroy(fixture.unbound);
    p4_game_enemies_destroy(fixture.empty);
    p4_game_enemies_destroy(fixture.enemies);
    p4_game_tables_text_destroy(fixture.names);
    p4_package_registry_retire();
    release_inputs();
    assert(releases == 1);
    puts("game_enemies_native: retail identity, staged rollback, observed-bank, byte-verified relocation, reload and restore passed; no native image/gameplay admission");
    return 0;
}
