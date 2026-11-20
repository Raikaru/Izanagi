/* Native publication of relocated derived spans and renamed items
 * (game_table_derived.h). The loader still loads the selected original block
 * into the shared battle arena; once the battle tables have loaded, the
 * `table-derived` completion participant checks that block, copies the
 * producer's rows to the guest heap and moves the block's slot with its
 * extent. Renamed items get one guest block of strings, which
 * datGetItemNameId returns from the commit on. Nothing is staged without a
 * relocated span or a renamed item. */
#include "game_table_derived_internal.h"
#include "p4pc/game_encounters.h"
#include "p4pc/game_items_native.h"
#include "p4pc/game_personas.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/gmem.h"
#include <string.h>

/* Relocatable blocks by SLUS_217.82 005f66c0 descriptor slot, row stride and
 * the zero guard that follows the copy. */
static const struct Block {
    P4GameTableInputKind input;
    uint32_t span, slot, stride, guard;
} blocks[] = {
    /* PERSONA4 skill pairs (iGpffffb3ec). Roster fusion's pick (0x003096D0)
     * scans on after a zero half until it has seen four, so it reads up to
     * three pairs past the terminator: in retail the loader's alignment
     * padding before PERSONA5 (164 bytes padded to 176). */
    {P4_TABLE_INPUT_PERSONA, P4_PERSONA_SKILL_PAIR_SPAN, 0x007644dcu, P4_PERSONA_SKILL_PAIR_SIZE,
        3u * P4_PERSONA_SKILL_PAIR_SIZE},
    /* ENCOUNT rows are read only by ID (game_encounters.h). */
    {P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_FORMATION_SPAN, 0x00764504u, P4_ENCOUNT_FORMATION_SIZE, 0},
    {P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_SELECTOR_SPAN, 0x0076450cu, P4_ENCOUNT_SELECTOR_SIZE, 0},
    {P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_WEIGHTED_SPAN, 0x00764508u, P4_ENCOUNT_WEIGHTED_SIZE, 0},
    {P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_RECORD_SPAN, 0x00764514u, P4_ENCOUNT_RECORD_SIZE, 0},
};
enum { BLOCKS = sizeof blocks / sizeof blocks[0] };
typedef struct Relocation {
    const P4GameTableInput *loaded; /* the selected input the loader copies */
    const uint8_t *bytes;           /* the producer's span, borrowed until boot reset */
    size_t size;
    void *host;                     /* the guest copy */
    uint32_t guest, allocated;
    uint32_t original, original_size; /* the block the loader placed */
} Relocation;
static struct {
    P4GameTableAllocator allocator;
    Relocation relocations[BLOCKS];
    size_t names;     /* renamed items at stage */
    void *names_host; /* their guest copy */
    uint32_t names_guest, names_allocated;
    uint32_t name_streams[P4_ITEM_RETAIL_COUNT];
    int staged, active;
} state;

static int prepare_complete(void *user);
static void commit_complete(void *user);

int p4_game_tables_derived_boot_stage(const P4GameTableAllocator *allocator)
{
    if (state.active) return P4_TABLE_FROZEN;
    Relocation found[BLOCKS];
    memset(found, 0, sizeof found);
    const size_t names = p4_game_tables_derived_item_name_count();
    int any = names != 0;
    for (unsigned i = 0; i < BLOCKS; ++i)
        any |= p4_game_tables_derived_relocated(blocks[i].input, blocks[i].span,
            &found[i].loaded, &found[i].bytes, &found[i].size);
    if (state.staged) {
        if (names != state.names) return P4_TABLE_CONFLICT;
        for (unsigned i = 0; i < BLOCKS; ++i)
            if (found[i].loaded != state.relocations[i].loaded || found[i].bytes != state.relocations[i].bytes ||
                found[i].size != state.relocations[i].size) return P4_TABLE_CONFLICT;
        return P4_TABLE_OK;
    }
    if (!any) return P4_TABLE_OK;
    if (!allocator || !allocator->allocate || !allocator->release) return P4_TABLE_NOT_FOUND;
    int result = p4_game_tables_battle_complete_register("table-derived", prepare_complete, commit_complete, NULL);
    if (result) return result;
    state.allocator = *allocator;
    memcpy(state.relocations, found, sizeof found);
    state.names = names;
    state.staged = 1;
    return P4_TABLE_OK;
}
int p4_game_tables_derived_boot_unstage(void)
{
    if (!state.staged) return P4_TABLE_OK;
    for (unsigned i = 0; i < BLOCKS; ++i)
        if (state.relocations[i].host) return P4_TABLE_FROZEN;
    if (state.active || state.names_host) return P4_TABLE_FROZEN;
    int result = p4_game_tables_battle_complete_unregister("table-derived", prepare_complete, commit_complete, NULL);
    if (result) return result;
    memset(&state, 0, sizeof state);
    return P4_TABLE_OK;
}
static int prepare_complete(void *user)
{
    (void)user;
    if (!state.staged) return P4_TABLE_NOT_READY;
    for (unsigned i = 0; i < BLOCKS; ++i) {
        Relocation *relocation = &state.relocations[i];
        const struct Block *block = &blocks[i];
        if (!relocation->bytes) continue;
        /* The block the loader actually placed must be the selected original. */
        const P4GameTableInputSpan *span = p4_game_table_input_span(relocation->loaded, block->span);
        uint32_t base, rows;
        if (!span || !p4_game_table_extent_rows(block->slot, block->stride, &base, &rows)) return P4_TABLE_NOT_READY;
        const size_t loaded = (size_t)rows * block->stride;
        if (loaded != p4_game_table_input_span_size(span) ||
            memcmp(P4_GPTR(const uint8_t, base), p4_game_table_input_span_bytes(span), loaded)) return P4_TABLE_INVALID;
        if (!relocation->host) {
            const uint32_t size = (uint32_t)relocation->size + block->guard;
            uint32_t guest = 0;
            void *host = state.allocator.allocate(state.allocator.user, size, &guest);
            if (!host) return P4_TABLE_NOMEM;
            memcpy(host, relocation->bytes, relocation->size);
            memset((uint8_t *)host + relocation->size, 0, block->guard);
            relocation->host = host;
            relocation->guest = guest;
            relocation->allocated = size;
        }
        relocation->original = base;
        relocation->original_size = (uint32_t)loaded;
    }
    if (state.names && !state.names_host) {
        size_t size = 0;
        for (uint32_t id = 0; id < P4_ITEM_RETAIL_COUNT; ++id) {
            const char *name = p4_game_tables_derived_item_name(id);
            if (name) size += strlen(name) + 1u;
        }
        uint32_t guest = 0;
        uint8_t *host = state.allocator.allocate(state.allocator.user, (uint32_t)size, &guest);
        if (!host) return P4_TABLE_NOMEM;
        size_t at = 0;
        for (uint32_t id = 0; id < P4_ITEM_RETAIL_COUNT; ++id) {
            const char *name = p4_game_tables_derived_item_name(id);
            if (!name) { state.name_streams[id] = 0; continue; }
            const size_t length = strlen(name) + 1u;
            memcpy(host + at, name, length);
            state.name_streams[id] = guest + (uint32_t)at;
            at += length;
        }
        state.names_host = host;
        state.names_guest = guest;
        state.names_allocated = (uint32_t)size;
    }
    return P4_TABLE_OK;
}
static void commit_complete(void *user)
{
    (void)user;
    /* One non-yielding game-thread boundary: each store publishes its extent. */
    for (unsigned i = 0; i < BLOCKS; ++i) {
        const Relocation *relocation = &state.relocations[i];
        if (relocation->host)
            p4_game_table_slot_publish(blocks[i].slot, relocation->guest, (uint32_t)relocation->size);
    }
    state.active = 1;
}
int p4_game_tables_derived_session_end(int image_live)
{
    if (state.active && image_live) {
        for (unsigned i = 0; i < BLOCKS; ++i)
            if (state.relocations[i].host && P4_G32(blocks[i].slot) != state.relocations[i].guest)
                return P4_TABLE_CONFLICT;
        for (unsigned i = 0; i < BLOCKS; ++i)
            if (state.relocations[i].host)
                p4_game_table_slot_publish(blocks[i].slot, state.relocations[i].original, state.relocations[i].original_size);
    }
    for (unsigned i = 0; i < BLOCKS; ++i) {
        const Relocation *relocation = &state.relocations[i];
        if (relocation->host)
            state.allocator.release(state.allocator.user, relocation->guest, relocation->host, relocation->allocated);
    }
    if (state.names_host)
        state.allocator.release(state.allocator.user, state.names_guest, state.names_host, state.names_allocated);
    memset(&state, 0, sizeof state);
    return P4_TABLE_OK;
}
uint32_t p4_game_tables_derived_item_name_stream(uint32_t id)
{
    return state.active && id < P4_ITEM_RETAIL_COUNT ? state.name_streams[id] : 0;
}
