/* p4.data edit layers over real package boot and synthetic disc inputs
 * (game_data_edit_test.py): data/edits.xml rows resolved by name and key,
 * typed field writes landing in the composed inputs in load order, the later
 * layer winning a shared field with the conflict recorded, learnset edits of
 * a fusion and a party Persona, and refusals that refuse the package with a
 * readable reason. No native stage, guest heap or gameplay claim. */
#include "game_table_derived_internal.h"
#include "../src/platform/golden/golden_mode_cache_publish.h"
#include "p4pc/os.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/game_data_edit.h"
#include "p4pc/game_damage_cover.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/game_mode.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_enemies.h"
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_items.h"
#include "p4pc/game_personas.h"
#include "p4pc/game_personas_native.h"
#include "p4pc/game_personas_save.h"
#include "p4pc/game_skills.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/packages.h"
#include "mod_save_internal.h"
#include "guest_test.h"
#include "package_boot_internal.h"
#include "p4pc/package_memory.h"
#include "p4pc/mod_setting_store.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const P4PackageHandler p4_game_tables_sources_boot_provider;
/* No fixture source declares damage cover; publication finds no plan. */
int p4_game_damage_cover_prepare(const P4GameTables *tables, P4DamageCoverLoadOrderFn load_order)
{ (void)tables; (void)load_order; fputs("Unrequested damage cover plan\n", stderr); abort(); }
void p4_game_damage_cover_publish(void) { }
void p4_game_damage_cover_session_end(void) { }
/* No fixture source declares a side-ailment plan. */
int p4_game_side_ailment_prepare(const P4GameTables *tables)
{ (void)tables; fputs("Unrequested side-ailment plan\n", stderr); abort(); }
void p4_game_side_ailment_publish(void) { }
void p4_game_side_ailment_session_end(void) { }
static const char *mode;
static const char *fixture_root;
static unsigned prepares;
static P4GameMode fixture_game_mode = P4_GAME_MODE_PS2;
P4GameMode p4_game_mode(void) { return fixture_game_mode; }

_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected data-edit boot recovery: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ (void)provider; fprintf(stderr, "Unexpected data-edit origin recovery: %s\n", reason); abort(); }
void *p4pc_heap_malloc(size_t size) { (void)size; fputs("CPU data edits touched guest heap\n", stderr); abort(); }
void p4pc_heap_free(void *host) { (void)host; fputs("CPU data edits released guest heap\n", stderr); abort(); }
int p4_game_tables_items_source_stage(const P4GameItems *items)
{ (void)items; fputs("Data edits staged items\n", stderr); abort(); }
int p4_game_tables_items_boot_stage(P4GameItems *items)
{ (void)items; fputs("Data edits staged items\n", stderr); abort(); }
int p4_game_tables_items_boot_unstage(P4GameItems *items)
{ (void)items; fputs("Unowned item raw-hook rollback\n", stderr); abort(); }
int p4_game_skills_boot_stage(P4GameSkills *skills)
{ (void)skills; fputs("Data edits staged skills\n", stderr); abort(); }
int p4_game_skills_boot_unstage(P4GameSkills *skills)
{ (void)skills; fputs("Unowned skill raw-hook rollback\n", stderr); abort(); }
int p4_game_skills_save_register(const P4GameSkills *skills, const P4GameTableSources *sources)
{ (void)skills; (void)sources; fputs("Data edits registered a skill save block\n", stderr); abort(); }
int p4_game_skills_save_unregister(void) { return P4_TABLE_OK; }
int p4_game_enemies_boot_stage(P4GameEnemies *enemies)
{ (void)enemies; fputs("Data edits staged enemies\n", stderr); abort(); }
int p4_game_enemies_boot_unstage(P4GameEnemies *enemies)
{ (void)enemies; fputs("Unowned enemy raw-hook rollback\n", stderr); abort(); }
int p4_game_personas_boot_stage(P4GamePersonas *personas)
{ (void)personas; fputs("Data edits staged personas\n", stderr); abort(); }
int p4_game_personas_boot_unstage(P4GamePersonas *personas)
{ (void)personas; fputs("Unowned persona raw-hook rollback\n", stderr); abort(); }
int p4_game_personas_save_register(const P4GamePersonas *personas, const P4GameTableSources *sources)
{ (void)personas; (void)sources; fputs("Data edits registered a persona save block\n", stderr); abort(); }
int p4_game_personas_save_unregister(void) { return P4_TABLE_OK; }
int p4_game_elements_boot_stage(const P4GameTables *tables, const P4GameSkills *skills, const P4GameItems *items)
{ (void)tables; (void)skills; (void)items; fputs("Data edits staged elements\n", stderr); abort(); }

/* No family is staged, so no added-row snapshot is active. */
P4GameSkills *p4_game_skills_active(void) { return NULL; }
P4GameEnemies *p4_game_enemies_active(void) { return NULL; }
P4GamePersonas *p4_game_personas_active(void) { return NULL; }
P4GameItems *p4_game_tables_items_active(void) { return NULL; }
uint32_t p4_game_enemies_native_limit(void) { return P4_ENEMY_RETAIL_ROWS; }
uint32_t p4_game_personas_native_limit(void) { return P4_PERSONA_RETAIL_ROWS; }
static int is(const char *name) { return !strcmp(mode, name); }
static const P4GameTablesBootProduct *product;
static const P4GameTableInput *input_of(P4GameTableInputKind kind)
{
    for (size_t i = 0; product && i < product->input_count; ++i)
        if (p4_game_table_input_kind(product->inputs[i]) == kind) return product->inputs[i];
    return NULL;
}
static const uint8_t *row(P4GameTableInputKind kind, uint32_t span, uint32_t header, uint32_t stride, uint32_t id)
{
    const P4GameTableInput *input = input_of(kind);
    assert(input);
    const P4GameTableInputSpan *s = p4_game_table_input_span(input, span);
    assert(s && p4_game_table_input_span_size(s) >= header + (size_t)(id + 1) * stride);
    return p4_game_table_input_span_bytes(s) + header + (size_t)id * stride;
}
static unsigned half(const uint8_t *at) { return at[0] | (unsigned)at[1] << 8; }
static void original_level(void)
{
    uint8_t level = 0;
    const char *source = NULL, *sha256 = NULL, *reason = NULL;
    assert(!p4_data_original_enemy_level(3, &level, &source, &sha256, &reason));
    assert(level == 5 && source && strstr(source, "UNIT.TBL") && sha256);
    assert(getenv("P4_TEST_ORIGINAL_UNIT_SHA") && !strcmp(sha256, getenv("P4_TEST_ORIGINAL_UNIT_SHA")));
    assert(!reason);
    assert(p4_data_original_enemy_level(P4_ENEMY_RETAIL_ROWS, &level, NULL, NULL, &reason));
}
static void events(const uint8_t *at, const uint8_t expected[][3], unsigned count, unsigned capacity)
{
    for (unsigned i = 0; i < capacity; ++i, at += 4) {
        if (i < count) assert(at[0] == expected[i][0] && at[1] == 1 && half(at + 2) == expected[i][2]);
        else assert(!at[1] && !half(at + 2));
    }
}
static void good(void)
{
    const uint8_t *vexaro = row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 2);
    assert(half(vexaro + 0x12) == 140 && vexaro[0x03] == 2 && half(vexaro + 0x04) == 10 && vexaro[0x0E] == 95);
    const uint8_t *poxkin = row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 5);
    assert(poxkin[0x18] == 1 && poxkin[0x19] == 60 && half(poxkin + 0x1C) == 0x04 && !half(poxkin + 0x1E));
    const uint8_t *zorb = row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 10);
    assert(zorb[0x03] == 1 && half(zorb + 0x04) == 9 && zorb[0x0F] == 2 && zorb[0x10] == 3);
    const uint8_t *vex = row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 1);
    assert(half(vex + 0x12) == 50 && half(vex + 0x04) == 4); /* untouched */
    const uint8_t *fakelore = row(P4_TABLE_INPUT_UNIT, 0, 0, 60, 3);
    original_level(); /* Original 5, despite the composed edit's level 20. */
    assert(fakelore[0x03] == 20 && half(fakelore + 0x04) == 300 && fakelore[0x08] == 12);
    assert(half(fakelore + 0x22) == 0x302 && fakelore[0x24] == 100 && half(fakelore + 0x10) == 3);
    const uint8_t *affinity = row(P4_TABLE_INPUT_UNIT, 1, 0, 32, 3);
    assert(half(affinity + 1 * 2) == 0x0800 && half(affinity + 2 * 2) == 0x0100 && half(affinity + 9 * 2) == 0x2000);
    assert(half(affinity + 0 * 2) == 0x0014);
    const uint8_t *nominal = row(P4_TABLE_INPUT_PERSONA, 0, 0, 14, 1);
    assert(nominal[0x03] == 3 && nominal[0x05] == 5);
    /* Fusion Personas store levels after the base (now 3): Vex innate, Nubo
     * moved to 4, Vexaro added at 7, kept in level order. */
    static const uint8_t fusion[][3] = {{0, 1, 1}, {1, 1, 3}, {4, 1, 2}};
    events(row(P4_TABLE_INPUT_PERSONA, 1, 0, 70, 1) + 6, fusion, 3, 16);
    /* Party Personas store absolute levels, sorted: Vexaro inserted at 3,
     * Nubo's event at 5 now teaches Vexdrix. */
    static const uint8_t party[][3] = {{0, 1, 1}, {3, 1, 2}, {5, 1, 4}};
    events(row(P4_TABLE_INPUT_PERSONA, 2, 0, 622, 0) + 4, party, 3, 32);
    /* Renames land in the fixed-width MSG rows; later rows still found Nubo
     * by its disc name. */
    static const char frost[19] = "Frost", big[21] = "Big Fakelore";
    assert(!memcmp(row(P4_TABLE_INPUT_MSG, 1, 0, 19, 3), frost, sizeof frost));
    assert(!memcmp(row(P4_TABLE_INPUT_MSG, 2, 0, 21, 3), big, sizeof big));
    const uint8_t *medixine = row(P4_TABLE_INPUT_ITEM, 0, 16, 68, 0x302);
    assert(medixine[0x34] == 100 && !medixine[0x35] && medixine[0x24] == 1);
    /* A retail item's name goes to the item-name channel, not ITEMTBL. */
    assert(!strcmp(p4_game_tables_derived_item_name(0x302), "Herbal Tonic") && p4_game_tables_derived_item_name_count() == 1);
    /* Shops: Medixine sells from 4/20; Test Club lost the Blank gate; 5-Widget
     * waits for 3 Test Material. */
    assert(medixine[0x38] == 4 && medixine[0x39] == 20);
    const uint8_t *testclub = row(P4_TABLE_INPUT_ITEM, 0, 16, 68, 1), *widget = row(P4_TABLE_INPUT_ITEM, 0, 16, 68, 3);
    assert(testclub[0x3C] == 0 && testclub[0x3D] == 0x00 && testclub[0x3E] == 0x05);
    assert(widget[0x3C] == 3 && widget[0x3D] == 0x0D && widget[0x3E] == 0x05 && !widget[0x40]);
    /* Dungeons: floor 8's spawn and chests, encounter group 1's rare rate and
     * normal slot 2, formation 17's second enemy, treasure group 1's record 2. */
    const uint8_t *floor = row(P4_TABLE_INPUT_ENCOUNT, 1, 0, 10, 8), *group = row(P4_TABLE_INPUT_ENCOUNT, 3, 0, 248, 1);
    assert(floor[2] == 2 && floor[3] == 6 && floor[4] == 4 && floor[0] == 1);
    assert(group[0] == 97 && group[4] == 5 && group[8 + 4] == 17 && group[8 + 6] == 40);
    const uint8_t *formation = row(P4_TABLE_INPUT_ENCOUNT, 0, 0, 24, 17), *treasure = row(P4_TABLE_INPUT_ENCOUNT, 4, 0, 348, 1);
    assert(formation[8] == 1 && formation[10] == 3);
    assert(treasure[12] == 5 && treasure[14] == 0x02 && treasure[15] == 0x03 && (treasure[12 + 7] & 1));
    /* edits.two assigned Vexaro's power after edits.one: the later layer won
     * and the shared field is reported with both owners. */
    assert(p4_data_edits_count() == 2 && p4_data_edits_conflict_count() == 1);
    P4DataConflict conflict;
    assert(!p4_data_edits_conflict_at(0, &conflict) && conflict.kind == P4_DATA_SKILL && conflict.id == 2);
    assert(!strcmp(conflict.field, "power") && conflict.owner_count == 2);
    assert(!strcmp(conflict.owners[0], "edits.one") && !strcmp(conflict.owners[1], "edits.two"));
    assert(p4_data_edits_conflict_at(1, &conflict));
}
static int prepare(void *user)
{
    (void)user; ++prepares;
    const void *found = (const void *)1;
    assert(!p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &found));
    product = found;
    if (is("none")) { assert(!product && !p4_data_edits_count()); return 0; }
    if (is("original")) {
        assert(row(P4_TABLE_INPUT_UNIT, 0, 0, 60, 2)[3] == 37); /* developer replacement */
        assert(row(P4_TABLE_INPUT_UNIT, 0, 0, 60, 3)[3] == 20); /* semantic edit */
        original_level();
        return 0;
    }
    if (is("session")) return 0;
    good();
    return 0;
}
static void publish(void *user) { (void)user; }

/* Session edits run over the loaded banks: here the composed inputs copied
 * into guest memory behind their slots, as the native loader leaves them. */
enum { ARENA = 0x01400000u, ARENA_SIZE = 0x100000u };
static uint32_t arena_next = ARENA;
static uint32_t live(P4GameTableInputKind kind, uint32_t span, uint32_t header, uint32_t slot)
{
    const P4GameTableInputSpan *s = p4_game_table_input_span(input_of(kind), span);
    assert(s);
    const uint32_t size = (uint32_t)p4_game_table_input_span_size(s) - header, base = arena_next;
    memcpy(P4_G2H(base), p4_game_table_input_span_bytes(s) + header, size);
    if (slot) p4_game_table_slot_publish(slot, base, size);
    arena_next = (base + size + 15u) & ~15u;
    assert(arena_next <= ARENA + ARENA_SIZE);
    return base;
}
/* The seal exposes both original PS2 widths and Golden's wider rows. */
static void name_views(void)
{
    static const uint8_t ps2_skill[2][19] = {"First", "PS2 skill name max"};
    static const uint8_t ps2_persona[2][17] = {"First", "PS2 persona max!"};
    static const uint8_t golden_skill[2][23] = {"First", "Golden Skill Name Wide"};
    static const uint8_t golden_persona[2][21] = {"First", "Golden Persona Wide"};
    P4DataBanks view = {0};
    char name[32];
    view.skill_name = &ps2_skill[0][0]; view.skill_names = 2; view.skill_name_stride = 19;
    view.persona_name = &ps2_persona[0][0]; view.persona_names = 2; view.persona_name_stride = 17;
    p4_data_catalog_seal_view(&view);
    assert(!strcmp(p4_data_catalog_name(P4_DATA_SKILL, 1, name, sizeof name), "PS2 skill name max"));
    assert(!strcmp(p4_data_catalog_name(P4_DATA_PERSONA, 1, name, sizeof name), "PS2 persona max!"));
    fixture_game_mode = P4_GAME_MODE_GOLDEN;
    view.skill_name = &golden_skill[0][0]; view.skill_name_stride = 23;
    view.persona_name = &golden_persona[0][0]; view.persona_name_stride = 21;
    assert(!strcmp(p4_data_catalog_name(P4_DATA_SKILL, 1, name, sizeof name), "Golden Skill Name Wide"));
    assert(!strcmp(p4_data_catalog_name(P4_DATA_PERSONA, 1, name, sizeof name), "Golden Persona Wide"));
    fixture_game_mode = P4_GAME_MODE_PS2;
    p4_data_catalog_seal_view(NULL);
}
static void session(void)
{
    if (p4t_guest_map(0x00764000u, 0x2000u) || p4t_guest_map(ARENA, ARENA_SIZE)) exit(77);
    char error[768];
    assert(p4_data_session_begin("edits.one", 0, error, sizeof error)); /* nothing loaded yet */
    name_views();
    live(P4_TABLE_INPUT_SKILL, 0, 0, 0x007644A4); live(P4_TABLE_INPUT_SKILL, 1, 0, 0x007644A8);
    live(P4_TABLE_INPUT_MSG, 1, 0, 0x00764538); live(P4_TABLE_INPUT_MSG, 2, 0, 0x00764534);
    live(P4_TABLE_INPUT_MSG, 3, 0, 0x00764530);
    const uint32_t enemies = live(P4_TABLE_INPUT_UNIT, 0, 0, 0x007644B4);
    live(P4_TABLE_INPUT_UNIT, 1, 0, 0x007644B8); live(P4_TABLE_INPUT_UNIT, 2, 0, 0x007644C8);
    live(P4_TABLE_INPUT_PERSONA, 0, 0, 0x007644C4); live(P4_TABLE_INPUT_PERSONA, 1, 0, 0x007644CC);
    live(P4_TABLE_INPUT_PERSONA, 2, 0, 0x007644D4);
    const uint32_t skills = P4_G32(0x007644A8);
    P4_G32(0x007642A0) = live(P4_TABLE_INPUT_ITEM, 0, P4_ITEM_FILE_HEADER_SIZE, 0);
    for (uint32_t span = 0; span < 5; ++span)
        if (span != 2) live(P4_TABLE_INPUT_ENCOUNT, span, 0, (const uint32_t[]){0x00764504, 0x0076450C, 0, 0x00764508,
            0x00764514}[span]);
    P4_GPDATA(uint32_t, 0xb438) = 8; /* the native boot catalog completed */
    uint8_t *fakelore = P4_G2H(enemies + 3u * 60u + 4u), *vexaro = P4_G2H(skills + 2u * 40u + 0x12u);
    const unsigned hp = half(fakelore), power = half(vexaro);
    size_t enemy = 3, skill = 2;
    const P4DataEditValue seven = {0, 777, NULL}, two = {0, 222, NULL}, huge = {0, 100000, NULL};

    /* This run only, then persisted; both write the loaded banks. */
    assert(!p4_data_session_begin("edits.one", 0, error, sizeof error));
    assert(!p4_data_edit_set(P4_DATA_ENEMY, enemy, "hp", &seven, error, sizeof error));
    assert(!p4_data_session_end(1, error, sizeof error) && half(fakelore) == 777);
    const P4DataEditValue changed_level = {0, 44, NULL};
    assert(!p4_data_session_begin("edits.one", 0, error, sizeof error));
    assert(!p4_data_edit_set(P4_DATA_ENEMY, enemy, "level", &changed_level, error, sizeof error));
    assert(!p4_data_session_end(1, error, sizeof error));
    assert(P4_G8(enemies + 3u * 60u + 3u) == 44);
    original_level(); /* Session edits cannot alter the captured disc value. */
    assert(!p4_data_session_begin("edits.one", 1, error, sizeof error));
    assert(!p4_data_edit_set(P4_DATA_SKILL, skill, "power", &two, error, sizeof error));
    assert(!p4_data_session_end(1, error, sizeof error) && half(vexaro) == 222 && p4_data_session_persisted() == 1);
    /* A refused write makes the caller undo the whole call. */
    const P4DataEditValue five = {0, 555, NULL};
    assert(!p4_data_session_begin("edits.one", 1, error, sizeof error));
    assert(!p4_data_edit_set(P4_DATA_ENEMY, enemy, "hp", &five, error, sizeof error) && half(fakelore) == 555);
    assert(p4_data_edit_set(P4_DATA_SKILL, skill, "power", &huge, error, sizeof error) && strstr(error, "power"));
    assert(!p4_data_session_end(0, NULL, 0) && half(fakelore) == 777 && p4_data_session_persisted() == 1);
    /* Item names change only at boot. */
    const P4DataEditValue tonic = {1, 0, "Tonic"};
    assert(!p4_data_session_begin("edits.one", 0, error, sizeof error));
    assert(p4_data_edit_set(P4_DATA_ITEM, 0x302, "name", &tonic, error, sizeof error) && strstr(error, "data.luau"));
    assert(!p4_data_session_end(0, NULL, 0));

    /* The save carries the persisted edit by key: a load undoes every
     * session edit, then re-applies it; a new game re-applies nothing. */
    assert(!p4_data_session_save_register() && !p4_data_session_save_register());
    uint8_t image_bytes[4100] = {0};
    image_bytes[56] = 0x00; image_bytes[57] = 0x20; image_bytes[60] = 1;
    memset(image_bytes + 65, 0xFF, 4);
    const P4ModSaveImage image = {image_bytes, sizeof image_bytes, "synthetic"};
    P4ModSaveCapture *capture = NULL;
    assert(!p4_mod_save_capture_image(&image, &capture, error, sizeof error) && capture);
    uint8_t *sidecar = malloc(capture->sidecar_size);
    assert(sidecar);
    memcpy(sidecar, capture->sidecar, capture->sidecar_size);
    P4ModSaveLoad *load = NULL;
    assert(!p4_mod_save_stage_image(&image, sidecar, capture->sidecar_size, &load, error, sizeof error) && load);
    p4_mod_save_load_commit(load);
    assert(half(fakelore) == hp && half(vexaro) == power); /* committed: everything undone */
    p4_mod_save_load_notify();
    assert(half(fakelore) == hp && half(vexaro) == 222 && p4_data_session_persisted() == 1);
    p4_mod_save_capture_free(capture);
    assert(!p4_mod_save_reset_session(error, sizeof error));
    assert(half(fakelore) == hp && half(vexaro) == power && !p4_data_session_persisted() && !p4_data_session_count());
}
/* The loaded executable's name banks: datGetItemNameId's at 0x005E2200 (the
 * items the fixture uses; zero, no row, for the rest) and the floor names at
 * 0x005F4090 (27 bytes by step index; only floor 8 named). */
static int item_names(void)
{
    if (p4t_guest_map(0x005E2000u, 0x4000u) || p4t_guest_map(0x005F4000u, 0x2000u)) return -1;
    static const struct { uint32_t id; const char *name; } named[] = {
        {0x302, "Medixine"}, {1, "Test Club"}, {3, "5-Widget"}, {0x50D, "Test Material"},
    };
    for (uint32_t id = 0; id < 1536; ++id) P4_G32(0x005E2200u + id * 4u) = 0;
    uint32_t strings = 0x005E2200u + 1536u * 4u;
    for (size_t i = 0; i < sizeof named / sizeof named[0]; ++i) {
        memcpy(P4_G2H(strings), named[i].name, strlen(named[i].name) + 1);
        P4_G32(0x005E2200u + named[i].id * 4u) = strings;
        strings += 16;
    }
    memset(P4_G2H(0x005F4090u), 0, 160u * 27u);
    memcpy(P4_G2H(0x005F4090u + 8u * 27u), "Example Castle 33F", 19);
    return 0;
}
static int cache_table_order(const void *a, const void *b)
{ return strcmp(((const P4GoldenCacheTable *)a)->name, ((const P4GoldenCacheTable *)b)->name); }
static int accept_cached_declarations(void *context, P4GameTablesDefinitionBuilder *builder,
    const P4GoldenCacheTable *extra, size_t count, char *reason, size_t capacity)
{ (void)context; (void)builder; (void)extra; (void)count; (void)reason; (void)capacity; return 0; }
/* Run after the real catalog provider requested Golden capture, before the
 * table builder. Inputs are synthetic and released immediately after capture. */
static int golden_catalog_fixture(void *unused)
{
    (void)unused;
    static uint8_t msg[32768], items[1536 * 24], original_msg[32768], original_items[1536 * 24];
    const size_t widths[] = {21, 23, 21, 21}, counts[] = {32, 576, 336, 256};
    size_t offset = 0, starts[4];
    for (unsigned block = 0; block < 4; ++block) {
        size_t n = widths[block] * counts[block];
        msg[offset] = (uint8_t)n; msg[offset + 1] = (uint8_t)(n >> 8);
        msg[offset + 2] = (uint8_t)(n >> 16); msg[offset + 3] = (uint8_t)(n >> 24);
        starts[block] = offset + 4;
        offset = (offset + 4 + n + 15) & ~(size_t)15;
    }
    memcpy(msg + starts[0] + 21, "Golden Arcana", 14);
    memcpy(msg + starts[1] + 23, "Golden Skill Name Wide", 23);
    memcpy(msg + starts[3] + 21, "Golden Persona Wide", 20);
    memcpy(items + 24, "Golden Item Name Wide", 22);
    assert(p4_data_catalog_capture_golden(msg, offset - 1, items, sizeof items));
    /* MSG4 is a fifth empty packed span. All native banks and item names pass
     * through a real cache store and verified load before catalog capture. */
    offset += 16;
    static uint8_t banks[P4_TABLE_INPUT_COUNT][176];
    static const unsigned blocks[] = {1,2,3,5,6,5,1,3,11};
    P4GoldenCacheTable tables[P4_TABLE_INPUT_COUNT + 1], extra[] = {{"decl/item-names", NULL, 0}};
    for (unsigned i = 0; i < P4_TABLE_INPUT_COUNT; ++i)
        tables[i] = (P4GoldenCacheTable){p4_game_table_input_label((P4GameTableInputKind)i), banks[i], blocks[i] * 16u};
    tables[P4_TABLE_INPUT_MSG].data = msg; tables[P4_TABLE_INPUT_MSG].size = offset;
    tables[P4_TABLE_INPUT_COUNT] = (P4GoldenCacheTable){"decl/item-names", items, sizeof items};
    qsort(tables, P4_TABLE_INPUT_COUNT + 1, sizeof tables[0], cache_table_order);
    char pref[1024], install[1024], program[1024], reason[192];
    snprintf(pref, sizeof pref, "%s/cache-pref", fixture_root);
    snprintf(install, sizeof install, "%s/cache-install", fixture_root);
    snprintf(program, sizeof program, "%s/cache-program", fixture_root);
    assert(!p4_os_mkdir(pref) && !p4_os_mkdir(install) && !p4_os_mkdir(program));
    P4GoldenCachePaths paths = {pref, install, program, NULL};
    P4GoldenCacheIdentityEntry sources[] = {{"fixture", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}};
    P4GoldenCacheIdentityEntry versions[] = {{"schema", "1"}};
    P4GoldenCacheIdentity identity = {sources, 1, versions, 1};
    char cache_key[65];
    memcpy(original_msg, msg, offset); memcpy(original_items, items, sizeof items);
    if (p4_golden_cache_store(&paths, &identity, tables, P4_TABLE_INPUT_COUNT + 1, cache_key, reason, sizeof reason)) {
        fprintf(stderr, "synthetic cache store: %s\n", reason);
        abort();
    }
    msg[starts[1] + 23] = 'X'; items[24] = 'X';
    P4GoldenModeCachePlan plan = {&paths, &identity, tables, P4_TABLE_INPUT_COUNT + 1, extra, 1,
                                  accept_cached_declarations, NULL};
    P4GameTablesDefinitionBuilder *builder = p4_game_tables_boot_builder();
    assert(builder && builder->input_mask);
    assert(!p4_golden_mode_cache_publish(&plan, builder, reason, sizeof reason));
    /* Keys must come from the verified cache, not the mutated source arrays. */
    memset(msg, 0, sizeof msg); memset(items, 0, sizeof items);
    assert(!strcmp(p4_data_catalog_arcana(1), "Golden Arcana"));
    assert(!strcmp(p4_data_catalog_key(P4_DATA_SKILL, 1), "p4:skill:golden-skill-name-wide"));
    assert(!strcmp(p4_data_catalog_key(P4_DATA_PERSONA, 1), "p4:persona:golden-persona-wide"));
    assert(!strcmp(p4_data_catalog_key(P4_DATA_ITEM, 1), "p4:item:golden-item-name-wide"));
    char name[32];
    assert(!strcmp(p4_data_catalog_name(P4_DATA_ITEM, 1, name, sizeof name), "Golden Item Name Wide"));
    p4_data_catalog_golden_rollback();
    assert(!p4_data_catalog_arcana(1));
    assert(!p4_data_catalog_capture_golden(original_msg, offset, original_items, sizeof original_items));
    assert(!strcmp(p4_data_catalog_arcana(1), "Golden Arcana"));
    return -1; /* exercise package-provider rollback as well */
}
int main(int argc, char **argv)
{
    if (argc != 4) return 2;
    mode = argv[1];
    fixture_root = argv[2];
    if (is("golden-capture")) fixture_game_mode = P4_GAME_MODE_GOLDEN;
    if (item_names()) return 77;
    const int success = is("good") || is("none") || is("session") || is("original");
    static const P4PackageClient client = {.diagnostic_name = "data edit fixture", .manifest_name = "mod.ini",
        .section = "[mod]", .kind = P4_PACKAGE_MOD};
    static const char *const dependencies[] = {P4_GAME_TABLES_BOOT_PROVIDER, NULL};
    static const P4PackageHandler observer = {.name = "data-edit-fixture", .prepare = prepare, .publish = publish,
        .boot_phase = P4_PACKAGE_BOOT_UI, .dependencies = dependencies};
    static const char *const catalog_dependency[] = {"game-data.catalog", NULL};
    static const P4PackageHandler capture = {.name = "game-data.catalog-test", .prepare = golden_catalog_fixture,
        .boot_phase = P4_PACKAGE_BOOT_TABLES, .dependencies = catalog_dependency};
    if (is("golden-capture")) assert(!p4_package_handler_register(&capture));
    assert(!p4_package_handler_register(&observer));
    assert(!p4_package_handler_register(&p4_game_tables_sources_boot_provider));
    assert(!p4_package_handler_register(&p4_game_data_boot_provider));
    assert(!p4_package_handler_register(&p4_mod_settings_boot_provider));
    P4PackageRegistry *registry = NULL;
    assert(!p4_package_registry_load(&registry, &client, argv[2], argv[3]));
    const int result = p4_package_registry_publish();
    if (success) assert(!result && prepares == 1);
    else assert(result && !prepares && !p4_data_edits_count());
    if (is("golden-capture")) {
        const char *why = NULL;
        assert(p4_data_catalog_ready(&why) && why && strstr(why, "not captured"));
    }
    if (is("session")) session();
    assert(!p4_game_tables_boot_session_end());
    assert(!p4_data_edits_count() && !p4_data_edits_conflict_count() && !p4_game_tables_derived_layer_count());
    p4_package_registry_retire();
    assert(!registry && !p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    printf("game_data_edit: %s passed\n", mode);
    return 0;
}
