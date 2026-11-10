#include "p4pc/game_items_save.h"
#include "p4pc/game_table_refs.h"
#include "p4pc/mod_save.h"
#include "p4pc/package_memory.h"
#include "game_items_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { ITEM_SAVE_SCHEMA = 1, ITEM_REFERENCE_BOUND = 1024 };
_Static_assert(sizeof(P4TableSourceEntry) + sizeof(P4TableSourceOrigin) + 64 < ITEM_REFERENCE_BOUND,
    "item reference wire bound includes all fixed identity strings and scalar framing");
struct P4GameItemsSave {
    P4GameItems *items;
    const P4GameTableSources *sources;
    P4ModSaveRegistration *registration;
};
typedef struct ItemSaveStaged {
    uint32_t count, equipment[10][3];
    P4GameItemState rows[];
} ItemSaveStaged;
static int fail(const P4ModSaveContext *context, const char *message)
{
    if (context->error && context->error_capacity && !context->error[0])
        snprintf(context->error, context->error_capacity, "%s", message);
    return -1;
}
static int ready(const P4GameItemsSave *save)
{
    return p4_game_tables_items_revision(save->items) && p4_game_table_sources_is_frozen(save->sources);
}
static int encode(const P4ModSaveContext *context, P4ModSaveWriter *writer, void *user)
{
    P4GameItemsSave *save = user;
    if (!ready(save)) return fail(context, "Expanded item state is not native-ready");
    uint32_t count = 0, equipment_count = 0;
    const size_t rows = p4_game_tables_items_count(save->items);
    for (size_t i = 0; i < rows; ++i) {
        const P4GameItemView *row = p4_game_tables_items_at(save->items, i);
        const P4GameItemMutable *state = p4_game_tables_items_mutable(save->items, row->id);
        if (!state) return fail(context, "Expanded item has no owned state");
        if (state->quantity || state->availability) ++count;
    }
    for (uint32_t unit = 1; unit <= 10; ++unit)
        for (uint32_t slot = 0; slot < 3; ++slot) {
            uint32_t id;
            int status = p4_game_tables_items_equipment_get(save->items, unit, slot, &id);
            if (!status) ++equipment_count;
            else if (status != P4_TABLE_NOT_FOUND) return fail(context, "Expanded equipment state is unavailable");
        }
    if (!count && !equipment_count) return 1;
    P4GameTableRefWriter *references = NULL;
    if (p4_game_table_refs_writer_begin(save->sources, context, writer, &references)) return -1;
    int result = -1;
    if (p4_mod_save_write_u32(writer, count) || p4_mod_save_write_u32(writer, equipment_count)) goto done;
    for (size_t i = 0; i < rows; ++i) {
        const P4GameItemView *row = p4_game_tables_items_at(save->items, i);
        const P4GameItemMutable *state = p4_game_tables_items_mutable(save->items, row->id);
        if (!state->quantity && !state->availability) continue;
        const uint32_t ids[5] = {row->id};
        const unsigned char bytes[2] = {state->quantity, state->availability};
        if (state->quantity > 99 || p4_game_table_refs_write(references, save->items, P4_TABLE_SOURCE_ITEM, ids) ||
            p4_mod_save_write(writer, bytes, sizeof bytes)) goto done;
    }
    for (uint32_t unit = 1; unit <= 10; ++unit)
        for (uint32_t slot = 0; slot < 3; ++slot) {
            uint32_t id;
            if (p4_game_tables_items_equipment_get(save->items, unit, slot, &id)) continue;
            const uint32_t ids[5] = {id};
            if (p4_mod_save_write_u32(writer, unit) || p4_mod_save_write_u32(writer, slot) ||
                p4_game_table_refs_write(references, save->items, P4_TABLE_SOURCE_ITEM, ids)) goto done;
        }
    result = 0;
done:
    p4_game_table_refs_writer_end(references);
    return result ? fail(context, "Cannot encode expanded item identities or state") : 0;
}
static int decode(const P4ModSaveContext *context, uint32_t schema, P4ModSaveReader *reader,
    void **out, void *user)
{
    P4GameItemsSave *save = user;
    *out = NULL;
    /* Retail Load/New Game stages the actual empty overlay, even before a
     * native view exists. No authored reference is authorized by absence. */
    if (!reader) return schema ? fail(context, "Absent expanded item block has a schema") : 0;
    if (schema != ITEM_SAVE_SCHEMA || !ready(save)) return fail(context, "Expanded item schema or native view is unavailable");
    P4GameTableRefReader *references = NULL;
    if (p4_game_table_refs_reader_begin(save->sources, context, reader, &references)) return -1;
    int result = -1;
    uint32_t count, equipment_count;
    if (p4_mod_save_read_u32(reader, &count) || p4_mod_save_read_u32(reader, &equipment_count) ||
        count > p4_game_tables_items_count(save->items) || equipment_count > 30 || (!count && !equipment_count)) goto done;
    /* Staged state is a bounded decode payload, not retained package metadata. */
    ItemSaveStaged *staged = malloc(sizeof *staged + (size_t)count * sizeof *staged->rows);
    if (!staged) goto done;
    staged->count = count;
    memset(staged->equipment, 0, sizeof staged->equipment);
    *out = staged;
    uint32_t previous = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const P4TableSourceEntry *entry = NULL;
        unsigned char bytes[2];
        if (p4_game_table_refs_read(references, save->items, P4_TABLE_SOURCE_ITEM, &entry) ||
            p4_mod_save_read(reader, bytes, sizeof bytes) || entry->ids[0] <= previous ||
            bytes[0] > 99 || (!bytes[0] && !bytes[1]) ||
            !p4_game_tables_items_mutable(save->items, entry->ids[0])) goto done;
        staged->rows[i] = (P4GameItemState){entry->ids[0], bytes[0], bytes[1]};
        previous = entry->ids[0];
    }
    previous = 0;
    for (uint32_t i = 0; i < equipment_count; ++i) {
        uint32_t unit, slot;
        const P4TableSourceEntry *entry = NULL;
        if (p4_mod_save_read_u32(reader, &unit) || p4_mod_save_read_u32(reader, &slot) ||
            !unit || unit > 10 || slot > 2 || unit * 3 + slot <= previous ||
            p4_game_table_refs_read(references, save->items, P4_TABLE_SOURCE_ITEM, &entry) ||
            p4_game_tables_items_equipment_check(save->items, unit, slot, entry->ids[0])) goto done;
        staged->equipment[unit - 1][slot] = entry->ids[0];
        previous = unit * 3 + slot;
    }
    if (p4_mod_save_remaining(reader)) goto done;
    result = 0;
done:
    p4_game_table_refs_reader_end(references);
    return result ? fail(context, "Invalid expanded item identities, order or state") : 0;
}
static void commit(void *opaque, void *user)
{
    P4GameItemsSave *save = user;
    ItemSaveStaged *staged = opaque;
    if (staged)
        p4_game_tables_items_state_apply(save->items, staged->rows, staged->count, staged->equipment);
    else p4_game_tables_items_state_clear(save->items);
    free(staged);
}
static void discard(void *staged, void *user)
{
    (void)user;
    free(staged);
}
int p4_game_tables_items_save_register(P4GameItems *items,
    const P4GameTableSources *sources, P4GameItemsSave **out)
{
    if (!items || !sources || !out) return P4_TABLE_INVALID;
    if (!p4_game_table_sources_is_sealed(sources)) return P4_TABLE_NOT_READY;
    const size_t count = p4_game_tables_items_count(items);
    if (!count) { *out = NULL; return P4_TABLE_OK; }
    const P4GameItemView *first = p4_game_tables_items_at(items, 0);
    if (!first) return P4_TABLE_NOT_READY;
    if (!p4_game_table_sources_origin_at(sources, items, first->owner, 0)) return P4_TABLE_NOT_FOUND;
    if (p4_packages_memory_reserve(sizeof(P4GameItemsSave))) return P4_TABLE_NOMEM;
    P4GameItemsSave *save = calloc(1, sizeof *save);
    if (!save) { p4_packages_memory_cancel(sizeof *save); return P4_TABLE_NOMEM; }
    p4_packages_memory_commit(sizeof *save);
    save->items = items;
    save->sources = sources;
    const P4ModSaveProvider provider = {
        .struct_size = sizeof provider, .abi_version = P4_MOD_SAVE_ABI_VERSION,
        .owner = "p4pc", .key = "expanded-items", .schema_version = ITEM_SAVE_SCHEMA,
        .flags = P4_MOD_SAVE_REQUIRED,
        .max_bytes = 12u + ((uint64_t)count + 30u) * ITEM_REFERENCE_BOUND,
        .user = save, .encode = encode, .decode = decode, .commit = commit, .discard = discard
    };
    if (p4_mod_save_register(&provider, &save->registration)) {
        free(save);
        p4_packages_memory_release(sizeof(P4GameItemsSave));
        return P4_TABLE_CONFLICT;
    }
    *out = save;
    return P4_TABLE_OK;
}
int p4_game_tables_items_save_unregister(P4GameItemsSave *save)
{
    if (!save) return P4_TABLE_OK;
    if (p4_mod_save_unregister(save->registration)) return P4_TABLE_CONFLICT;
    free(save);
    p4_packages_memory_release(sizeof(P4GameItemsSave));
    return P4_TABLE_OK;
}
