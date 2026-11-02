#include "p4pc/game_personas_save.h"
#include "p4pc/game_personas_native.h"
#include "p4pc/game_table_refs.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_save.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PERSONA_SAVE_SCHEMA 1u
#define PERSONA_SAVE_MAGIC 0x50503450u /* P4PP, little endian */
#define PERSONA_RECORD 0x30u
/* Retail-image places that carry a persona ID (save tags of D_00797F8C and
 * D_0079B6A4): the twelve 48-byte stock slots (DatPersonaRecord: bit 0 of flags
 * active, ID at +2; datPersonaRosterSlot 0x0010ACE0) and datStageRecord's
 * (0x00110270) selection ID with its staged record. Party members' personas are
 * fixed retail rows. */
#define STOCK_BASE 0x00797F8Cu
#define STOCK_SLOTS 12u
#define STAGE_ID 0x0079B6A4u
#define STAGE_RECORD 0x0079B6A8u
enum { USE_ROW = 1u, USE_IMAGE = 2u };

typedef struct PersonaSaveContext {
    const P4GamePersonas *personas;
    const P4GameTableSources *sources;
} PersonaSaveContext;
typedef struct PersonaSaveEntry {
    uint32_t id, uses;
    uint8_t row[PERSONA_RECORD];
} PersonaSaveEntry;
typedef struct PersonaSaveLoad {
    size_t count;
    PersonaSaveEntry entries[];
} PersonaSaveLoad;
static P4ModSaveRegistration *registration;
static PersonaSaveContext save_context;

static int fail(const P4ModSaveContext *context, const char *message)
{
    if (context && context->error && context->error_capacity && !context->error[0])
        snprintf(context->error, context->error_capacity, "%s", message);
    return -1;
}
static int ready(const PersonaSaveContext *source)
{
    return p4_game_personas_revision(source->personas) && p4_game_personas_active() == source->personas &&
        p4_game_table_sources_validate_view(source->sources, source->personas) == P4_TABLE_OK;
}
static uint32_t uses_of(uint32_t id)
{
    uint32_t uses = (P4_G16(p4_game_personas_native_compendium(id)) & 1u) ? USE_ROW : 0u;
    for (uint32_t slot = 0; slot < STOCK_SLOTS; ++slot) {
        const uint32_t record = STOCK_BASE + slot * PERSONA_RECORD;
        if ((P4_G16(record) & 1u) && P4_G16(record + 2u) == id) uses |= USE_IMAGE;
    }
    if (P4_G16(STAGE_ID) == id || P4_G16(STAGE_RECORD + 2u) == id) uses |= USE_IMAGE;
    return uses;
}
static int encode(const P4ModSaveContext *context, P4ModSaveWriter *writer, void *user)
{
    const PersonaSaveContext *source = user;
    if (!ready(source)) return fail(context, "Added persona state is not native-ready");
    uint32_t count = 0;
    const size_t rows = p4_game_personas_count(source->personas);
    for (size_t i = 0; i < rows; ++i)
        if (uses_of(p4_game_personas_at(source->personas, i)->desc.id)) ++count;
    if (!count) return 1; /* Omit: the absent block decodes to an empty added compendium. */
    int result = p4_mod_save_write_u32(writer, PERSONA_SAVE_MAGIC);
    if (!result) result = p4_mod_save_write_u32(writer, PERSONA_SAVE_SCHEMA);
    if (!result) result = p4_mod_save_write_u32(writer, count);
    if (result) return result;
    P4GameTableRefWriter *refs = NULL;
    result = p4_game_table_refs_writer_begin(source->sources, context, writer, &refs);
    if (result) return result;
    for (size_t i = 0; !result && i < rows; ++i) {
        const uint32_t id = p4_game_personas_at(source->personas, i)->desc.id, uses = uses_of(id);
        if (!uses) continue;
        const uint32_t ids[5] = {id};
        result = p4_game_table_refs_write(refs, source->personas, P4_TABLE_SOURCE_PERSONA, ids);
        if (!result) result = p4_mod_save_write_u32(writer, id);
        if (!result) result = p4_mod_save_write_u32(writer, uses);
        if (!result && (uses & USE_ROW))
            result = p4_mod_save_write(writer, P4_G2H(p4_game_personas_native_compendium(id)), PERSONA_RECORD);
    }
    p4_game_table_refs_writer_end(refs);
    return result;
}
static int read_entry(const PersonaSaveContext *source, const P4ModSaveContext *context,
    P4ModSaveReader *reader, P4GameTableRefReader *refs, PersonaSaveEntry *entry)
{
    const P4TableSourceEntry *current;
    int result = p4_game_table_refs_read(refs, source->personas, P4_TABLE_SOURCE_PERSONA, &current);
    if (!result) result = p4_mod_save_read_u32(reader, &entry->id);
    if (!result) result = p4_mod_save_read_u32(reader, &entry->uses);
    if (result) return result;
    if (current->ids[0] != entry->id) {
        char message[P4_TABLE_KEY_SIZE + 96];
        snprintf(message, sizeof message, "Added persona %s was ID %u when saved and is now ID %u",
            current->key, (unsigned)entry->id, (unsigned)current->ids[0]);
        return fail(context, message);
    }
    if (entry->id < P4_PERSONA_RETAIL_ROWS || entry->id >= p4_game_personas_limit(source->personas) ||
        !entry->uses || (entry->uses & ~(uint32_t)(USE_ROW | USE_IMAGE))) return P4_TABLE_INVALID;
    if (!(entry->uses & USE_ROW)) return P4_TABLE_OK;
    result = p4_mod_save_read(reader, entry->row, PERSONA_RECORD);
    if (result) return result;
    /* A registered row of exactly this persona. */
    return (entry->row[0] & 1u) && (uint32_t)(entry->row[2] | entry->row[3] << 8) == entry->id ?
        P4_TABLE_OK : P4_TABLE_INVALID;
}
static int decode(const P4ModSaveContext *context, uint32_t schema,
    P4ModSaveReader *reader, void **staged, void *user)
{
    const PersonaSaveContext *source = user;
    if (context && context->image.data && !ready(source))
        return fail(context, "Added persona state is not native-ready");
    if (!reader) return schema == 0 ? P4_TABLE_OK : P4_TABLE_INVALID;
    if (schema != PERSONA_SAVE_SCHEMA) return P4_TABLE_INVALID;
    uint32_t magic, version, count;
    int result = p4_mod_save_read_u32(reader, &magic);
    if (!result) result = p4_mod_save_read_u32(reader, &version);
    if (!result) result = p4_mod_save_read_u32(reader, &count);
    if (result) return result;
    if (magic != PERSONA_SAVE_MAGIC || version != PERSONA_SAVE_SCHEMA || !count ||
        count > p4_game_personas_count(source->personas)) return P4_TABLE_INVALID;
    PersonaSaveLoad *load = calloc(1, sizeof *load + (size_t)count * sizeof *load->entries);
    if (!load) return P4_TABLE_NOMEM;
    load->count = count;
    *staged = load;
    P4GameTableRefReader *refs = NULL;
    result = p4_game_table_refs_reader_begin(source->sources, context, reader, &refs);
    if (result) return result;
    for (uint32_t i = 0; !result && i < count; ++i) {
        result = read_entry(source, context, reader, refs, &load->entries[i]);
        if (!result && i && load->entries[i - 1].id >= load->entries[i].id) result = P4_TABLE_INVALID;
    }
    p4_game_table_refs_reader_end(refs);
    if (!result && p4_mod_save_remaining(reader)) result = P4_TABLE_INVALID;
    return result;
}
static int validate(const P4ModSaveContext *context, void *staged, void *user)
{
    (void)context; (void)staged;
    return ready(user) ? P4_TABLE_OK : P4_TABLE_CONFLICT;
}
static void commit(void *staged, void *user)
{
    (void)user;
    PersonaSaveLoad *load = staged;
    p4_game_personas_native_compendium_clear();
    if (!load) return;
    for (size_t i = 0; i < load->count; ++i)
        if (load->entries[i].uses & USE_ROW)
            memcpy(P4_G2H(p4_game_personas_native_compendium(load->entries[i].id)), load->entries[i].row, PERSONA_RECORD);
    free(load);
}
static void discard(void *staged, void *user) { (void)user; free(staged); }
int p4_game_personas_save_register(const P4GamePersonas *personas, const P4GameTableSources *sources)
{
    if (!personas || !sources) return P4_TABLE_INVALID;
    if (!p4_game_personas_count(personas)) return P4_TABLE_OK;
    if (registration)
        return save_context.personas == personas && save_context.sources == sources ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    save_context = (PersonaSaveContext){personas, sources};
    const P4ModSaveProvider provider = {
        .struct_size = sizeof provider, .abi_version = P4_MOD_SAVE_ABI_VERSION,
        .owner = "p4pc", .key = "persona-compendium", .schema_version = PERSONA_SAVE_SCHEMA,
        .flags = P4_MOD_SAVE_REQUIRED, .max_bytes = 1024u * 1024u, .user = &save_context,
        .encode = encode, .decode = decode, .validate = validate, .commit = commit, .discard = discard
    };
    if (p4_mod_save_register(&provider, &registration)) {
        save_context = (PersonaSaveContext){0};
        return P4_TABLE_CONFLICT;
    }
    return P4_TABLE_OK;
}
int p4_game_personas_save_unregister(void)
{
    if (!registration) return P4_TABLE_OK;
    if (p4_mod_save_unregister(registration)) return P4_TABLE_CONFLICT;
    registration = NULL;
    save_context = (PersonaSaveContext){0};
    return P4_TABLE_OK;
}
