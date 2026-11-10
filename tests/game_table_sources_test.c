/* Real frozen table/text views, package providers and sidecar codec. Synthetic
 * data only: no gameplay, retail-data or native consumer qualification claim. */
#include "p4pc/game_table_sources.h"
#include "p4pc/game_table_refs.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/game_text.h"
#include "p4pc/game_items_save.h"
#include "p4pc/gmem.h"
#include "p4pc/packages.h"
#include "guest_test.h"
#include "mod_save_internal.h"
#include "mod_save_hash.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user; void *bytes = p4t_galloc(size, 16); if (bytes) *guest = P4_H2G(bytes); return bytes;
}
static void release(void *user, uint32_t guest, void *bytes, uint32_t size)
{
    (void)user; assert(size && bytes && guest == P4_H2G(bytes));
}
static P4GameTables *table(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    P4ElementDesc native = {.id = 19, .flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
        .revision = 1, .key = "native.owner:leaf", .display_name = "Native", .short_name = "Native"};
    P4ElementDesc package = {.id = 20, .family = 1, .flags = P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
        .revision = 2, .key = "shared.owner:leaf", .display_name = "Package", .short_name = "Package"};
    P4AffinityChannelDesc channel = {.id = 19, .kind = P4_CHANNEL_PERCENT, .revision = 3,
        .key = "shared.owner:channel", .display_name = "Authored channel"};
    P4AffinityRow affinity = {P4_ENTITY_ENEMY, 1, 0x80000000u, 19, 19, P4_AFF_WEAK | 125u, 125};
    P4ActionElementBinding action = {.source_kind = P4_ACTION_ELEMENT_SKILL, .source_id = 120,
        .element_id = 19, .channel_id = 19, .schema = 1, .revision = 4, .key = "native.owner:action"};
    P4AffinityProfileAlias alias = {.entity_kind = P4_ENTITY_ENEMY, .entity_id = 1, .native_profile = 5,
        .profile_id = 0x80000000u, .knowledge_id = UINT32_MAX, .schema = 1, .revision = 5, .key = "shared.owner:alias"};
    P4AffinityEffectBinding effect = {.source_kind = P4_AFFINITY_EFFECT_SKILL, .source_id = 440,
        .element_id = 19, .channel_id = 19, .flags = P4_AFF_RESIST, .schema = 1, .revision = 6, .key = "shared.owner:effect"};
    assert(p4_game_tables_element(tables, "native.owner", &native) == P4_TABLE_OK);
    assert(p4_game_tables_element(tables, "shared.owner", &package) == P4_TABLE_OK);
    assert(p4_game_tables_channel(tables, "shared.owner", &channel) == P4_TABLE_OK);
    assert(p4_game_tables_affinity(tables, "shared.owner", &affinity) == P4_TABLE_OK);
    assert(p4_game_tables_action_element(tables, "native.owner", &action) == P4_TABLE_OK);
    assert(p4_game_tables_affinity_alias(tables, "shared.owner", &alias) == P4_TABLE_OK);
    assert(p4_game_tables_affinity_effect(tables, "shared.owner", &effect) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 9) == P4_TABLE_OK);
    return tables;
}
static P4GameText *text(void)
{
    P4GameTableAllocator allocator = {NULL, allocate, release};
    P4TextProfile profile = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *strings = p4_game_tables_text_create(&allocator, &profile); assert(strings);
    const uint8_t name[] = "Authored source fixture";
    assert(p4_game_tables_text_register(strings, "shared.owner", P4_TEXT_ITEM, 1536, P4_TEXT_NAME, 4, name, sizeof name) == P4_TABLE_OK);
    assert(p4_game_tables_text_freeze(strings, 9) == P4_TABLE_OK);
    return strings;
}
static P4GameTableSources *sources_for(P4GameTables *tables, P4GameText *strings,
    const P4PackageProvider *mod, const P4PackageProvider *pack, uint32_t mode_schema,
    int omitted, int extra, int duplicate, int wrong_revision)
{
    P4GameTableSources *sources = p4_game_table_sources_create(); assert(sources);
    uint32_t native_source, mod_source, pack_source;
    char mode[] = "fixture:native-mode";
    assert(p4_game_table_sources_native(sources, mode, mode_schema, 7, &native_source) == P4_TABLE_OK);
    mode[0] = 'x'; /* The prepared identity must own its source string. */
    uint32_t same;
    assert(p4_game_table_sources_native(sources, "fixture:native-mode", mode_schema, 7, &same) == P4_TABLE_OK && same == native_source);
    assert(p4_game_table_sources_native(sources, "fixture:native-mode", mode_schema + 1, 7, &same) == P4_TABLE_CONFLICT);
    assert(p4_game_table_sources_package(sources, mod, "fixture:mod-rows", 1, 11, &mod_source) == P4_TABLE_OK);
    assert(p4_game_table_sources_package(sources, pack, "fixture:pack-rows", 1, 13, &pack_source) == P4_TABLE_OK && mod_source != pack_source);
    assert(p4_game_table_sources_package(sources, mod, "fixture:mod-rows", 1, 11, &same) == P4_TABLE_OK && same == mod_source);
    assert(p4_game_table_sources_package(sources, mod, "fixture:mod-rows", 1, 12, &same) == P4_TABLE_CONFLICT);
    same = UINT32_MAX;
    assert(p4_game_table_sources_package(sources, mod, "fixture:mod-rows", 0, 11, &same) == P4_TABLE_INVALID && same == UINT32_MAX);
    assert(p4_game_table_sources_package(sources, mod, "fixture:mod-rows", 1, 0, &same) == P4_TABLE_INVALID && same == UINT32_MAX);
    assert(p4_game_table_sources_view(sources, P4_TABLE_SOURCE_REGISTRY, tables) == P4_TABLE_OK);
    assert(p4_game_table_sources_view(sources, P4_TABLE_SOURCE_REGISTRY, tables) == P4_TABLE_OK);
    assert(p4_game_table_sources_view(sources, P4_TABLE_SOURCE_TEXT, tables) == P4_TABLE_CONFLICT);
    assert(p4_game_table_sources_view(sources, P4_TABLE_SOURCE_TEXT, strings) == P4_TABLE_OK);
    P4TableSourceCursor cursor = {0}; P4TableSourceEntry entry; unsigned count = 0;
    while (p4_game_table_sources_next(P4_TABLE_SOURCE_REGISTRY, tables, &cursor, &entry) == P4_TABLE_OK) {
        ++count;
        if (omitted && entry.domain == P4_TABLE_SOURCE_ACTION) continue;
        uint32_t origin = entry.domain == P4_TABLE_SOURCE_ACTION ||
            (entry.domain == P4_TABLE_SOURCE_ELEMENT && entry.ids[0] == 19) ? native_source :
            entry.domain == P4_TABLE_SOURCE_CHANNEL || entry.domain == P4_TABLE_SOURCE_ALIAS ||
            entry.domain == P4_TABLE_SOURCE_AFFINITY ? pack_source : mod_source;
        if (wrong_revision && entry.domain == P4_TABLE_SOURCE_ACTION) ++entry.revision;
        assert(p4_game_table_sources_bind(sources, tables, &entry, origin) == P4_TABLE_OK);
        if (duplicate && entry.domain == P4_TABLE_SOURCE_ACTION)
            assert(p4_game_table_sources_bind(sources, tables, &entry, origin) == P4_TABLE_OK);
    }
    assert(count == 7);
    cursor = (P4TableSourceCursor){0};
    assert(p4_game_table_sources_next(P4_TABLE_SOURCE_TEXT, strings, &cursor, &entry) == P4_TABLE_OK);
    assert(p4_game_table_sources_bind(sources, strings, &entry, mod_source) == P4_TABLE_OK);
    if (extra) { ++entry.ids[1]; assert(p4_game_table_sources_bind(sources, strings, &entry, mod_source) == P4_TABLE_OK); }
    assert(p4_game_table_sources_next(P4_TABLE_SOURCE_TEXT, strings, &cursor, &entry) == P4_TABLE_NOT_FOUND);
    return sources;
}
typedef struct SaveState {
    const P4GameTables *tables;
    const P4GameText *strings;
    P4GameTableSources *sources;
    uint32_t value;
    int bad_reference;
} SaveState;
typedef struct Saved { uint32_t value; } Saved;
static const struct RefCase { uint32_t domain, ids[5]; } references[] = {
    {P4_TABLE_SOURCE_ELEMENT, {19, 0, 0, 0, 0}},
    {P4_TABLE_SOURCE_ELEMENT, {20, 0, 0, 0, 0}},
    {P4_TABLE_SOURCE_CHANNEL, {19, 0, 0, 0, 0}},
    {P4_TABLE_SOURCE_ACTION, {P4_ACTION_ELEMENT_SKILL, 120, P4_ACTION_COMPONENT_DAMAGE, 0, 0}},
    {P4_TABLE_SOURCE_ALIAS, {P4_ENTITY_ENEMY, 1, 5, 0, 0}},
    {P4_TABLE_SOURCE_AFFINITY, {P4_ENTITY_ENEMY, 1, 0x80000000u, 19, 19}},
    {P4_TABLE_SOURCE_EFFECT, {P4_AFFINITY_EFFECT_SKILL, 440, 19, 19, 0}},
    {P4_TABLE_SOURCE_STRING, {P4_TEXT_ITEM, 1536, P4_TEXT_NAME, 0, 0}}
};
static int encode(const P4ModSaveContext *context, P4ModSaveWriter *writer, void *user)
{
    SaveState *state = user;
    if (!state->value) return 1;
    if (p4_mod_save_write_u32(writer, state->value) ||
        p4_mod_save_write_u32(writer, sizeof references / sizeof references[0])) return P4_TABLE_INVALID;
    P4GameTableRefWriter *refs = NULL;
    int result = p4_game_table_refs_writer_begin(state->sources, context, writer, &refs);
    if (result) return result;
    if (state->bad_reference) {
        const uint32_t builtin[5] = {0};
        result = p4_game_table_refs_write(refs, state->tables, P4_TABLE_SOURCE_ELEMENT, builtin);
    } else {
        for (size_t i = 0; !result && i < sizeof references / sizeof references[0]; ++i)
            result = p4_game_table_refs_write(refs,
                references[i].domain == P4_TABLE_SOURCE_STRING ? (const void *)state->strings : state->tables,
                references[i].domain, references[i].ids);
    }
    p4_game_table_refs_writer_end(refs);
    return result;
}
static int decode(const P4ModSaveContext *context, uint32_t schema, P4ModSaveReader *reader, void **staged, void *user)
{
    SaveState *state = user;
    if (!reader) return P4_TABLE_OK;
    if (schema != 2) return P4_TABLE_INVALID;
    Saved *saved = malloc(sizeof *saved); if (!saved) return P4_TABLE_NOMEM;
    *staged = saved;
    uint32_t count;
    if (p4_mod_save_read_u32(reader, &saved->value) || p4_mod_save_read_u32(reader, &count) ||
        count != sizeof references / sizeof references[0]) return P4_TABLE_INVALID;
    P4GameTableRefReader *refs = NULL;
    int result = p4_game_table_refs_reader_begin(state->sources, context, reader, &refs);
    if (result) return result;
    for (size_t i = 0; !result && i < count; ++i) {
        const P4TableSourceEntry *entry = NULL;
        result = p4_game_table_refs_read(refs,
            references[i].domain == P4_TABLE_SOURCE_STRING ? (const void *)state->strings : state->tables,
            references[i].domain, &entry);
        if (!result && memcmp(entry->ids, references[i].ids, sizeof entry->ids)) result = P4_TABLE_CONFLICT;
    }
    p4_game_table_refs_reader_end(refs);
    return result ? result : p4_mod_save_remaining(reader) ? P4_TABLE_INVALID : P4_TABLE_OK;
}
static int validate(const P4ModSaveContext *context, void *staged, void *user)
{
    (void)context; SaveState *state = user;
    return !staged || (p4_game_table_sources_is_frozen(state->sources) &&
        p4_game_table_sources_validate_view(state->sources, state->tables) == P4_TABLE_OK &&
        p4_game_table_sources_validate_view(state->sources, state->strings) == P4_TABLE_OK) ? P4_TABLE_OK : P4_TABLE_NOT_READY;
}
static void commit(void *staged, void *user)
{
    SaveState *state = user; state->value = staged ? ((Saved *)staged)->value : 0; free(staged);
}
static void discard(void *staged, void *user) { (void)user; free(staged); }
static void word(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
}
static uint8_t *copy_sidecar(const P4ModSaveCapture *capture)
{
    uint8_t *copy = malloc(capture->sidecar_size); assert(copy); memcpy(copy, capture->sidecar, capture->sidecar_size); return copy;
}
static void semantic_view_case(P4GameText *frozen_strings)
{
    P4GameTableAllocator allocator = {NULL, allocate, release};
    P4TextProfile profile = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *pending = p4_game_tables_text_create(&allocator, &profile); assert(pending);
    const uint8_t name[] = "Authored source fixture";
    assert(p4_game_tables_text_register(pending, "shared.owner", P4_TEXT_ITEM, 1536, P4_TEXT_NAME, 4, name, sizeof name) == P4_TABLE_OK);
    P4TableSourceCursor cursor = {0}; P4TableSourceEntry entry;
    assert(p4_game_table_sources_next(P4_TABLE_SOURCE_TEXT, frozen_strings, &cursor, &entry) == P4_TABLE_OK);
    P4GameTableSources *sources = p4_game_table_sources_create(); assert(sources);
    uint32_t source;
    assert(p4_game_table_sources_native(sources, "fixture:native-strings", 1, 1, &source) == P4_TABLE_OK);
    assert(p4_game_table_sources_view(sources, P4_TABLE_SOURCE_TEXT, pending) == P4_TABLE_OK);
    assert(p4_game_table_sources_bind(sources, pending, &entry, source) == P4_TABLE_OK);
    assert(!p4_game_table_sources_is_sealed(sources) && !p4_game_table_sources_is_frozen(sources));
    assert(p4_game_table_sources_validate_view(sources, pending) == P4_TABLE_NOT_READY);
    assert(p4_game_table_sources_seal(sources) == P4_TABLE_NOT_READY);
    assert(!p4_game_table_sources_is_sealed(sources));
    assert(p4_game_tables_text_seal(pending, 9) == P4_TABLE_OK && !p4_game_tables_text_revision(pending));
    assert(p4_game_table_sources_seal(sources) == P4_TABLE_OK);
    assert(p4_game_table_sources_is_sealed(sources) && !p4_game_table_sources_is_frozen(sources));
    assert(p4_game_table_sources_seal(sources) == P4_TABLE_FROZEN);
    assert(p4_game_table_sources_bind(sources, pending, &entry, source) == P4_TABLE_FROZEN);
    assert(p4_game_table_sources_validate_view(sources, pending) == P4_TABLE_OK);
    const P4TableSourceOrigin *early = p4_game_table_sources_origin_at(sources, pending, "shared.owner", 0);
    assert(early && early->kind == P4_TABLE_SOURCE_NATIVE && !strcmp(early->source_key, "fixture:native-strings"));
    size_t mode_cursor = 0;
    assert(p4_game_table_sources_native_mode_count(sources) == 1);
    assert(p4_game_table_sources_native_mode_next(sources, &mode_cursor) == early);
    assert(!p4_game_table_sources_native_mode_next(sources, &mode_cursor));
    uint32_t ids[5] = {P4_TEXT_ITEM, 1536, P4_TEXT_NAME, 0, 0};
    P4TableSourceResolved resolved = {0};
    assert(p4_game_table_sources_resolve_entry(sources, pending, P4_TABLE_SOURCE_STRING, ids, &resolved) == P4_TABLE_NOT_READY);
    assert(p4_game_table_sources_require_owner(pending, NULL, "shared.owner", sources) == P4_TABLE_NOT_READY);
    assert(p4_game_table_sources_freeze(sources) == P4_TABLE_NOT_READY);
    assert(p4_game_tables_text_materialize(pending) == P4_TABLE_OK);
    assert(p4_game_table_sources_validate_view(sources, pending) == P4_TABLE_OK);
    assert(p4_game_table_sources_validate_view(sources, frozen_strings) == P4_TABLE_NOT_FOUND);
    assert(p4_game_table_sources_freeze(sources) == P4_TABLE_OK && p4_game_table_sources_is_frozen(sources));
    assert(p4_game_table_sources_require_owner(pending, NULL, "shared.owner", sources) == P4_TABLE_OK);
    const P4TableSourceOrigin *origin = p4_game_table_sources_origin_at(sources, pending, "shared.owner", 0);
    assert(origin == early && origin->kind == P4_TABLE_SOURCE_NATIVE && !strcmp(origin->source_key, "fixture:native-strings"));
    assert(p4_game_table_sources_resolve_entry(sources, pending, P4_TABLE_SOURCE_STRING, ids, &resolved) == P4_TABLE_OK &&
        resolved.origin == early && !resolved.provider);
    p4_game_table_sources_destroy(sources); p4_game_tables_text_destroy(pending);
}
static P4GameTableSources *item_sources(P4GameItems *items, P4GameText *names, P4GameText *descriptions,
    const P4PackageProvider *mod, const P4PackageProvider *pack, uint32_t mode_revision)
{
    P4GameTableSources *sources = p4_game_table_sources_create(); assert(sources);
    uint32_t native_source, mod_source, pack_source;
    assert(!p4_game_table_sources_native(sources, "fixture:item-mode", 1, mode_revision, &native_source));
    assert(!p4_game_table_sources_package(sources, mod, "fixture:item-mod", 1, 17, &mod_source));
    assert(!p4_game_table_sources_package(sources, pack, "fixture:item-pack", 1, 23, &pack_source));
    const struct { P4TableSourceViewKind kind; const void *view; } views[] = {
        {P4_TABLE_SOURCE_ITEMS, items}, {P4_TABLE_SOURCE_TEXT, names}, {P4_TABLE_SOURCE_TEXT, descriptions}
    };
    unsigned count = 0;
    for (size_t i = 0; i < sizeof views / sizeof views[0]; ++i) {
        assert(!p4_game_table_sources_view(sources, views[i].kind, views[i].view));
        P4TableSourceCursor cursor = {0}; P4TableSourceEntry entry;
        int status;
        while (!(status = p4_game_table_sources_next(views[i].kind, views[i].view, &cursor, &entry))) {
            uint32_t id = entry.ids[views[i].kind == P4_TABLE_SOURCE_ITEMS ? 0 : 1];
            uint32_t source = id == 1536 ? native_source : id == 1600 ? mod_source : pack_source;
            assert(!p4_game_table_sources_bind(sources, views[i].view, &entry, source));
            ++count;
        }
        assert(status == P4_TABLE_NOT_FOUND);
    }
    assert(count == 9 && !p4_game_table_sources_seal(sources));
    return sources;
}
static void item_state_is(const P4GameItems *items, uint32_t id, unsigned quantity, unsigned availability)
{
    P4GameItemState state;
    assert(!p4_game_tables_items_state_get(items, id, &state));
    assert(state.id == id && state.quantity == quantity && state.availability == availability);
}
static void item_sidecar_case(const P4PackageProvider *mod, const P4PackageProvider *pack)
{
    const P4GameTableAllocator allocator = {NULL, allocate, release};
    const P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    const P4TextProfile message = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *names = p4_game_tables_text_create(&allocator, &raw);
    P4GameText *descriptions = p4_game_tables_text_create(&allocator, &message);
    P4GameItems *items = p4_game_tables_items_create(&allocator);
    assert(items && names && descriptions);
    const uint32_t ids[] = {1536, 1600, P4_ITEM_MAX_ID};
    const uint8_t label[] = "Synthetic item";
    for (unsigned i = 0; i < 3; ++i) {
        char key[P4_TABLE_KEY_SIZE];
        snprintf(key, sizeof key, "shared.owner:item%u", i);
        const P4GameItemDesc item = {.id = ids[i], .revision = 3, .key = key,
            .icon = {"shared.owner", "shared.owner:icon", 1},
            .metadata = {.kind = i, .category_mask = 0x7fe0}};
        assert(!p4_game_tables_items_register(items, "shared.owner", &item));
        assert(!p4_game_tables_text_register(names, "shared.owner", P4_TEXT_ITEM, ids[i], P4_TEXT_NAME, 3, label, sizeof label));
        assert(!p4_game_tables_text_register(descriptions, "shared.owner", P4_TEXT_ITEM, ids[i], P4_TEXT_DESCRIPTION, 3, label, sizeof label));
    }
    assert(!p4_game_tables_items_models_register(items, "shared.owner", ids[0], 0x70, 1000));
    uint8_t *retail = calloc(1, P4_ITEM_RETAIL_FILE_SIZE); assert(retail);
    word(retail, P4_ITEM_RETAIL_COUNT);
    for (size_t i = P4_ITEM_FILE_HEADER_SIZE; i < P4_ITEM_RETAIL_FILE_SIZE; ++i) retail[i] = (uint8_t)(i * 37u + 19u);
    assert(!p4_game_tables_text_seal(names, 9) && !p4_game_tables_text_seal(descriptions, 9));
    assert(!p4_game_tables_items_seal(items, retail, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9));
    P4GameTableSources *sources = item_sources(items, names, descriptions, mod, pack, 1);
    P4GameItemsSave *save = NULL;
    uint8_t image[4100] = {0}, original[sizeof image];
    word(image + 56, 0x2000); word(image + 60, 1); word(image + 65, UINT32_MAX);
    memcpy(original, image, sizeof image);
    const P4ModSaveImage saved = {.data = image, .size = sizeof image, .slot = "item-sidecar-fixture"};
    char error[1024] = {0};
    P4ModSaveCapture *capture = NULL;
    P4GameItems *empty_items = p4_game_tables_items_create(NULL); assert(empty_items);
    P4GameTableSources *empty_sources = p4_game_table_sources_create(); assert(empty_sources);
    assert(!p4_game_tables_items_seal(empty_items, retail, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9));
    assert(!p4_game_table_sources_view(empty_sources, P4_TABLE_SOURCE_ITEMS, empty_items));
    assert(!p4_game_table_sources_seal(empty_sources));
    P4GameItemsSave *none = (P4GameItemsSave *)(uintptr_t)0xdeadbeef;
    P4PackageMemoryUsage before = p4_packages_memory_usage();
    assert(!p4_game_tables_items_save_register(empty_items, empty_sources, &none) && !none);
    assert(p4_game_tables_items_save_register(items, empty_sources, &none) == P4_TABLE_NOT_FOUND && !none);
    P4PackageMemoryUsage after = p4_packages_memory_usage();
    assert(before.resident == after.resident && before.reserved == after.reserved && !p4_mod_save_has_providers());
    assert(!p4_mod_save_capture_image(&saved, &capture, error, sizeof error) && !capture);
    p4_game_table_sources_destroy(empty_sources); p4_game_tables_items_destroy(empty_items);
    assert(!p4_game_tables_items_save_register(items, sources, &save) && save);
    assert(p4_mod_save_capture_image(&saved, &capture, error, sizeof error) < 0 && !capture);
    const P4ModSaveImage new_game = {0};
    P4ModSaveLoad *load = NULL;
    assert(!p4_mod_save_stage_image(&new_game, NULL, 0, &load, error, sizeof error) && load);
    p4_mod_save_load_commit(load); load = NULL; /* Actual cold NewGame default, no guest view yet. */
    assert(!p4_game_tables_text_materialize(names) && !p4_game_tables_text_materialize(descriptions));
    assert(!p4_game_tables_items_materialize(items) && !p4_game_table_sources_freeze(sources));
    assert(!p4_mod_save_capture_image(&saved, &capture, error, sizeof error) && !capture);
    for (unsigned i = 0; i < 3; ++i)
        assert(!p4_game_tables_items_state_set(items, ids[i], i == 2 ? 99 : i + 3, i == 1 ? 0xa5 : i));
    assert(!p4_game_tables_items_equipment_set(items, 1, 0, ids[0]));
    assert(!p4_game_tables_items_equipment_set(items, 4, 1, ids[1]));
    assert(!p4_game_tables_items_equipment_set(items, 10, 2, ids[2]));
    assert(!p4_mod_save_capture_image(&saved, &capture, error, sizeof error) && capture);
    const size_t payload = P4_MOD_SAVE_HEADER_BYTES + 32u + sizeof("p4pc") - 1u + sizeof("expanded-items") - 1u;
    const uint32_t bad_counts[][2] = {{UINT32_MAX, 3}, {3, 31}, {0, 0}};
    for (size_t i = 0; i < sizeof bad_counts / sizeof bad_counts[0]; ++i) {
        uint8_t *bad = copy_sidecar(capture);
        word(bad + payload + 4, bad_counts[i][0]);
        word(bad + payload + 8, bad_counts[i][1]);
        p4_save_hash(bad, capture->sidecar_size - 32, bad + capture->sidecar_size - 32);
        assert(p4_mod_save_stage_image(&saved, bad, capture->sidecar_size, &load, error, sizeof error) < 0 && !load);
        assert(strstr(error, "Invalid expanded item identities"));
        item_state_is(items, ids[0], 3, 0);
    }
    /* Same IDs/views but a different explicit compiled-mode revision cannot
     * authorize the old reference. No owner-string/package-miss inference. */
    assert(!p4_game_tables_items_save_unregister(save)); save = NULL;
    P4GameTableSources *changed = item_sources(items, names, descriptions, mod, pack, 2);
    assert(!p4_game_table_sources_freeze(changed));
    assert(!p4_game_tables_items_save_register(items, changed, &save));
    assert(p4_mod_save_stage_image(&saved, copy_sidecar(capture), capture->sidecar_size, &load, error, sizeof error) < 0 && !load);
    item_state_is(items, ids[0], 3, 0);
    assert(!p4_game_tables_items_save_unregister(save)); save = NULL;
    p4_game_table_sources_destroy(changed);
    assert(!p4_game_tables_items_save_register(items, sources, &save));
    p4_game_tables_items_state_clear(items);
    assert(!p4_game_tables_items_state_set(items, ids[0], 1, 0));
    assert(!p4_mod_save_stage_image(&saved, copy_sidecar(capture), capture->sidecar_size, &load, error, sizeof error) && load);
    item_state_is(items, ids[0], 1, 0);
    assert(p4_game_tables_items_save_unregister(save) == P4_TABLE_CONFLICT);
    p4_mod_save_load_abort(load); load = NULL;
    item_state_is(items, ids[0], 1, 0);
    assert(!p4_mod_save_stage_image(&saved, copy_sidecar(capture), capture->sidecar_size, &load, error, sizeof error) && load);
    p4_mod_save_load_commit(load); load = NULL;
    for (unsigned i = 0; i < 3; ++i) {
        item_state_is(items, ids[i], i == 2 ? 99 : i + 3, i == 1 ? 0xa5 : i);
        uint32_t equipped;
        assert(!p4_game_tables_items_equipment_get(items, i == 0 ? 1 : i == 1 ? 4 : 10, i, &equipped) && equipped == ids[i]);
    }
    assert(!memcmp(image, original, sizeof image));
    assert(!memcmp(P4_G2H(p4_game_tables_items_base(items)), retail + P4_ITEM_FILE_HEADER_SIZE,
        P4_ITEM_RETAIL_COUNT * P4_ITEM_ROW_SIZE));
    assert(!p4_mod_save_stage_image(&saved, NULL, 0, &load, error, sizeof error) && load);
    item_state_is(items, ids[2], 99, 2);
    p4_mod_save_load_commit(load); load = NULL;
    item_state_is(items, ids[2], 0, 0); /* Real retail Load clears the overlay only on commit. */
    assert(!p4_game_tables_items_state_set(items, ids[0], 7, 1));
    assert(!p4_mod_save_stage_image(&new_game, NULL, 0, &load, error, sizeof error) && load);
    item_state_is(items, ids[0], 7, 1);
    p4_mod_save_load_commit(load); load = NULL;
    item_state_is(items, ids[0], 0, 0);
    P4ModSaveCapture *empty_capture = NULL;
    assert(!p4_mod_save_capture_image(&saved, &empty_capture, error, sizeof error) && !empty_capture);
    assert(!p4_game_tables_items_save_unregister(save));
    p4_mod_save_capture_free(capture);
    assert(!p4_mod_save_reset_session(error, sizeof error));
    p4_game_table_sources_destroy(sources);
    p4_game_tables_items_destroy(items);
    p4_game_tables_text_destroy(names); p4_game_tables_text_destroy(descriptions); free(retail);
}
int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    if (p4t_guest_map(0x00764000u, 0x2000u)) return 77;
    static const P4PackageClient client = {.diagnostic_name = "source fixture packs", .manifest_name = "pack.ini",
        .section = "[pack]", .kind = P4_PACKAGE_ENGINE_PACK};
    static const P4PackageClient mod_client = {.diagnostic_name = "source fixture mods", .manifest_name = "mod.ini",
        .section = "[mod]", .kind = P4_PACKAGE_MOD};
    P4PackageRegistry *mods = NULL, *packs = NULL;
    assert(!p4_package_registry_load(&mods, &mod_client, argv[1], "source.mod"));
    assert(!p4_package_registry_load(&packs, &client, argv[2], "source.pack"));
    assert(!p4_package_registry_publish() && p4_packages_count() == 2);
    const P4PackageProvider *mod = NULL, *pack = NULL;
    for (size_t i = 0; i < p4_packages_count(); ++i) {
        const P4PackageProvider *provider = p4_packages_provider(i);
        if (p4_package_provider_info(provider)->kind == P4_PACKAGE_MOD) mod = provider; else pack = provider;
    }
    assert(mod && pack && !strcmp(p4_package_provider_info(mod)->id, "source.mod") &&
        !strcmp(p4_package_provider_info(pack)->id, "source.pack"));
    item_sidecar_case(mod, pack);
    P4GameTables *tables = table(); P4GameText *strings = text();
    semantic_view_case(strings);
    P4GameTableSources *sources = sources_for(tables, strings, mod, pack, 1, 1, 0, 0, 0);
    assert(p4_game_table_sources_freeze(sources) == P4_TABLE_NOT_READY);
    assert(p4_game_table_sources_require_owner(tables, NULL, "native.owner", sources) == P4_TABLE_NOT_READY);
    p4_game_table_sources_destroy(sources);
    sources = sources_for(tables, strings, mod, pack, 1, 0, 1, 0, 0);
    assert(p4_game_table_sources_freeze(sources) == P4_TABLE_CONFLICT); p4_game_table_sources_destroy(sources);
    sources = sources_for(tables, strings, mod, pack, 1, 0, 0, 1, 0);
    assert(p4_game_table_sources_freeze(sources) == P4_TABLE_CONFLICT); p4_game_table_sources_destroy(sources);
    sources = sources_for(tables, strings, mod, pack, 1, 0, 0, 0, 1);
    assert(p4_game_table_sources_freeze(sources) == P4_TABLE_CONFLICT); p4_game_table_sources_destroy(sources);
    sources = sources_for(tables, strings, mod, pack, 1, 0, 0, 0, 0);
    assert(p4_game_table_sources_freeze(sources) == P4_TABLE_OK);
    P4ModSaveRequireOwnerFn require_owner = p4_game_table_sources_require_owner;
    assert(require_owner(tables, NULL, "shared.owner", sources) == P4_TABLE_OK);
    assert(require_owner(tables, NULL, "native.owner", sources) == P4_TABLE_OK);
    assert(require_owner(tables, NULL, "missing.owner", sources) == P4_TABLE_NOT_FOUND);
    assert(require_owner(strings, NULL, "native.owner", sources) == P4_TABLE_NOT_FOUND);
    const P4TableSourceOrigin *native = p4_game_table_sources_origin_at(sources, tables, "native.owner", 0);
    assert(native && !strcmp(native->source_key, "fixture:native-mode") && native->schema == 1 && native->revision == 7);
    assert(!p4_game_table_sources_origin_at(sources, tables, "native.owner", 1));
    const P4TableSourceOrigin *first = p4_game_table_sources_origin_at(sources, tables, "shared.owner", 0);
    const P4TableSourceOrigin *second = p4_game_table_sources_origin_at(sources, tables, "shared.owner", 1);
    assert(first && second && first->kind == P4_TABLE_SOURCE_PACKAGE && second->kind == P4_TABLE_SOURCE_PACKAGE);
    assert(first->package_kind == P4_PACKAGE_MOD && second->package_kind == P4_PACKAGE_ENGINE_PACK);
    assert(!strcmp(first->package_id, "source.mod") && !strcmp(first->package_version, "1.2.3") &&
        !strcmp(second->package_id, "source.pack") && !strcmp(second->package_version, "9.8.7"));
    assert(!strcmp(first->source_key, "fixture:mod-rows") && first->schema == 1 && first->revision == 11 &&
        !strcmp(second->source_key, "fixture:pack-rows") && second->schema == 1 && second->revision == 13);
    size_t mode_cursor = 0;
    assert(p4_game_table_sources_native_mode_count(sources) == 1 &&
        p4_game_table_sources_native_mode_next(sources, &mode_cursor) == native &&
        !p4_game_table_sources_native_mode_next(sources, &mode_cursor));
    uint32_t ids[5] = {19, 0, 0, 0, 0};
    P4TableSourceResolved resolved = {0};
    assert(p4_game_table_sources_resolve_entry(sources, tables, P4_TABLE_SOURCE_ELEMENT, ids, &resolved) == P4_TABLE_OK &&
        resolved.origin == native && !resolved.provider);
    ids[0] = 20;
    assert(p4_game_table_sources_resolve_entry(sources, tables, P4_TABLE_SOURCE_ELEMENT, ids, &resolved) == P4_TABLE_OK &&
        resolved.origin == first && resolved.provider == mod);
    ids[0] = 19;
    assert(p4_game_table_sources_resolve_entry(sources, tables, P4_TABLE_SOURCE_CHANNEL, ids, &resolved) == P4_TABLE_OK &&
        resolved.origin == second && resolved.provider == pack);
    ids[0] = 0;
    assert(p4_game_table_sources_resolve_entry(sources, tables, P4_TABLE_SOURCE_ELEMENT, ids, &resolved) == P4_TABLE_NOT_FOUND &&
        resolved.origin == second && resolved.provider == pack); /* Unowned builtin is not a fabricated binding. */
    assert(p4_game_table_sources_boot_stage(sources) == P4_TABLE_OK && !p4_game_table_sources_active());
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK && p4_game_table_sources_active() == sources);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    uint8_t image[4100] = {0}, original[sizeof image]; word(image + 56, 0x2000); word(image + 60, 1); word(image + 65, UINT32_MAX);
    memcpy(original, image, sizeof image);
    P4ModSaveImage saved = {.data = image, .size = sizeof image, .slot = "source-fixture"};
    SaveState state = {tables, strings, sources, 0, 0};
    P4ModSaveProvider descriptor = {.struct_size = sizeof descriptor, .abi_version = P4_MOD_SAVE_ABI_VERSION,
        .owner = "table-source.fixture", .key = "state", .schema_version = 2, .flags = P4_MOD_SAVE_REQUIRED,
        .max_bytes = 8192, .user = &state, .encode = encode, .decode = decode,
        .validate = validate, .commit = commit, .discard = discard};
    P4ModSaveRegistration *registration = NULL;
    assert(!p4_mod_save_register(&descriptor, &registration));
    char error[1024] = {0}; P4ModSaveCapture *capture = NULL;
    assert(!p4_mod_save_capture_image(&saved, &capture, error, sizeof error) && !capture);
    state.value = UINT32_MAX;
    assert(!p4_mod_save_capture_image(&saved, &capture, error, sizeof error) && capture);
    state.bad_reference = 1;
    P4ModSaveCapture *invalid_reference = NULL;
    assert(p4_mod_save_capture_image(&saved, &invalid_reference, error, sizeof error) < 0 && !invalid_reference);
    state.bad_reference = 0;
    assert(!memcmp(image, original, sizeof image));
    state.value = 17; P4ModSaveLoad *load = NULL;
    assert(!p4_mod_save_stage_image(&saved, copy_sidecar(capture), capture->sidecar_size, &load, error, sizeof error));
    assert(state.value == 17); p4_mod_save_load_commit(load); load = NULL; assert(state.value == UINT32_MAX);
    P4GameTableSources *changed = sources_for(tables, strings, mod, pack, 2, 0, 0, 0, 0);
    assert(p4_game_table_sources_freeze(changed) == P4_TABLE_OK); state.sources = changed; state.value = 23;
    assert(p4_mod_save_stage_image(&saved, copy_sidecar(capture), capture->sidecar_size, &load, error, sizeof error) < 0 && !load && state.value == 23);
    state.sources = sources; p4_game_table_sources_destroy(changed);
    p4_package_registry_retire();
    assert(require_owner(tables, NULL, "shared.owner", sources) == P4_TABLE_NOT_READY);
    assert(!strcmp(p4_package_provider_info(mod)->version, "1.2.3")); /* Actual context pin survives retirement. */
    assert(p4_mod_save_stage_image(&saved, copy_sidecar(capture), capture->sidecar_size, &load, error, sizeof error) < 0 && !load && state.value == 23);
    assert(!p4_mod_save_unregister(registration)); p4_mod_save_capture_free(capture);
    assert(!p4_mod_save_reset_session(error, sizeof error));
    assert(p4_game_table_sources_session_end() == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    p4_game_table_sources_destroy(sources); p4_game_tables_text_destroy(strings); p4_game_tables_destroy(tables);
    assert(!p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    puts("game_table_sources: exact frozen entries, explicit native modes, pinned actual cross-kind providers and atomic synthetic sidecar round trip passed; no gameplay claim");
    return 0;
}

