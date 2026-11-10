#include "p4pc/game_personas.h"
#include "p4pc/game_table_inputs.h"
#include <stdlib.h>
#include <string.h>

enum { BANK_METADATA, BANK_GROWTH, BANK_MODEL, BANK_CANDIDATES, BANK_NAMES, BANK_AFFINITY, BANK_COUNT };
static const uint32_t bank_stride[BANK_COUNT] = {
    P4_PERSONA_METADATA_SIZE, P4_PERSONA_GROWTH_SIZE, P4_PERSONA_MODEL_SIZE, P4_PERSONA_CANDIDATES_SIZE, P4_PERSONA_NAME_SIZE,
    P4_PERSONA_AFFINITY_SIZE
};
typedef struct PersonaRow {
    P4GamePersonaView view;
} PersonaRow;
struct P4GamePersonas {
    PersonaRow *rows;
    size_t count, capacity;
    P4GameTableAllocator allocator;
    uint8_t *arena;
    uint32_t guest, bytes, limit, revision, semantic_revision;
    const P4GameTableInputSpan *spans[BANK_COUNT];
    const P4GameText *names;
    const P4GameTableInput *original_input;
    P4GameTableSourceUseFn original_use;
    P4GamePersonaRowUseFn authored_use;
    void *source_context;
};
int p4_game_personas_bind_runtime_use(P4GamePersonas *personas,
    P4GameTableSourceUseFn original_use, P4GamePersonaRowUseFn authored_use, void *source_context)
{
    if (!personas) return P4_TABLE_INVALID;
    if (personas->semantic_revision) return P4_TABLE_FROZEN;
    if (!original_use || (!!authored_use != !!source_context)) return P4_TABLE_INVALID;
    if (personas->original_use)
        return personas->original_use == original_use && personas->authored_use == authored_use &&
            personas->source_context == source_context ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    personas->original_use = original_use;
    personas->authored_use = authored_use;
    personas->source_context = source_context;
    return P4_TABLE_OK;
}
int p4_game_personas_validate_runtime_sources(const P4GamePersonas *personas)
{
    if (!personas) return P4_TABLE_INVALID;
    if (!personas->semantic_revision) return P4_TABLE_NOT_READY;
    if (!personas->spans[BANK_METADATA]) return personas->count ? P4_TABLE_INVALID : P4_TABLE_OK;
    if (!personas->original_input || !personas->original_use ||
        (personas->count && (!personas->authored_use || !personas->source_context))) return P4_TABLE_NOT_READY;
    return personas->count ? p4_game_tables_text_validate_runtime_sources(personas->names) : P4_TABLE_OK;
}
int p4_game_personas_runtime_use(const P4GamePersonas *personas, uint32_t id)
{
    if (!personas || !personas->revision) return P4_TABLE_NOT_READY;
    int result = p4_game_personas_validate_runtime_sources(personas);
    if (result) return result;
    if (id < P4_PERSONA_RETAIL_ROWS) {
        if (!personas->spans[BANK_METADATA]) return P4_TABLE_NOT_FOUND;
        result = personas->original_use(personas->spans[BANK_METADATA], (void *)personas->original_input);
    } else {
        if (id >= personas->limit) return P4_TABLE_NOT_FOUND;
        /* Root proves the exact row and its captured origin; no provider inference here. */
        result = personas->authored_use(personas, id, personas->source_context);
    }
    return result > 0 ? P4_TABLE_INVALID : result;
}
static int identifier(const char *s, size_t capacity)
{
    if (!s || !s[0]) return 0;
    for (size_t i = 0; i < capacity; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (!c) return 1;
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return 0;
    }
    return 0;
}
static int owned_key(const char *owner, const char *key)
{
    if (!identifier(owner, 64) || !key) return 0;
    size_t length = strlen(owner);
    if (length + 2 > P4_TABLE_KEY_SIZE || strncmp(owner, key, length) || key[length] != ':') return 0;
    return identifier(key + length + 1, P4_TABLE_KEY_SIZE - length - 1);
}
static int compare(const void *a, const void *b)
{
    uint32_t x = ((const PersonaRow *)a)->view.desc.id, y = ((const PersonaRow *)b)->view.desc.id;
    return (x > y) - (x < y);
}
P4GamePersonas *p4_game_personas_create(const P4GameTableAllocator *allocator)
{
    if (allocator && (!allocator->allocate || !allocator->release)) return NULL;
    P4GamePersonas *personas = calloc(1, sizeof *personas);
    if (personas) {
        if (allocator) personas->allocator = *allocator;
        personas->limit = P4_PERSONA_RETAIL_ROWS;
    }
    return personas;
}
void p4_game_personas_destroy(P4GamePersonas *personas)
{
    if (!personas) return;
    if (personas->arena) personas->allocator.release(personas->allocator.user,
        personas->guest, personas->arena, personas->bytes);
    free(personas->rows);
    free(personas);
}
int p4_game_personas_register(P4GamePersonas *personas, const char *owner, const P4GamePersonaDesc *desc)
{
    if (!personas || !desc || !owned_key(owner, desc->key) || !desc->schema || !desc->revision ||
        desc->id < P4_PERSONA_RETAIL_ROWS || desc->id > P4_PERSONA_MAX_ID ||
        desc->model_source >= P4_PERSONA_RETAIL_ROWS)
        return P4_TABLE_INVALID;
    if (personas->semantic_revision) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < personas->count; ++i)
        if (personas->rows[i].view.desc.id == desc->id || !strcmp(personas->rows[i].view.desc.key, desc->key))
            return P4_TABLE_CONFLICT;
    if (personas->count == personas->capacity) {
        size_t capacity = personas->capacity ? personas->capacity * 2 : 16;
        if (capacity > SIZE_MAX / sizeof *personas->rows) return P4_TABLE_NOMEM;
        PersonaRow *rows = realloc(personas->rows, capacity * sizeof *rows);
        if (!rows) return P4_TABLE_NOMEM;
        personas->rows = rows;
        personas->capacity = capacity;
    }
    PersonaRow *row = &personas->rows[personas->count++];
    memset(row, 0, sizeof *row);
    row->view.desc = *desc;
    strcpy(row->view.owner, owner);
    if (personas->limit <= desc->id) personas->limit = desc->id + 1;
    return P4_TABLE_OK;
}
int p4_game_personas_native_admit(const P4GamePersonas *personas)
{
    if (!personas) return P4_TABLE_INVALID;
    if (personas->count && !personas->allocator.allocate) return P4_TABLE_NOT_FOUND;
    return P4_TABLE_OK;
}
static uint32_t bank_offset(const P4GamePersonas *personas, unsigned bank)
{
    uint32_t offset = 0;
    for (unsigned i = 0; i < bank; ++i) offset = (offset + personas->limit * bank_stride[i] + 15u) & ~15u;
    return offset;
}
static int span_role(const P4GameTableInputSpan *span, P4GameTableInputKind kind, size_t index)
{
    const P4GameTableInput *owner = p4_game_table_input_span_owner(span);
    return owner && p4_game_table_input_kind(owner) == kind &&
        p4_game_table_input_span_index(span) == index &&
        p4_game_table_input_span(owner, index) == span && p4_game_table_input_span_bytes(span);
}
int p4_game_personas_seal(P4GamePersonas *personas,
    const P4GameTableInputSpan *metadata, const P4GameTableInputSpan *growth,
    const P4GameTableInputSpan *model, const P4GameTableInputSpan *candidates,
    const P4GameTableInputSpan *original_names, const P4GameTableInputSpan *affinity,
    const P4GameText *names, uint32_t revision)
{
    const P4GameTableInputSpan *spans[BANK_COUNT] = {metadata, growth, model, candidates, original_names, affinity};
    if (!personas || !revision) return P4_TABLE_INVALID;
    if (personas->semantic_revision)
        return personas->semantic_revision == revision && personas->names == names &&
            !memcmp(personas->spans, spans, sizeof spans) ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    if (personas->count || metadata || growth || model || candidates || original_names || affinity) {
        if (!span_role(metadata, P4_TABLE_INPUT_PERSONA, 0) || !span_role(growth, P4_TABLE_INPUT_PERSONA, 1) ||
            !span_role(candidates, P4_TABLE_INPUT_PERSONA, 5) ||
            p4_game_table_input_span_owner(metadata) != p4_game_table_input_span_owner(growth) ||
            p4_game_table_input_span_owner(metadata) != p4_game_table_input_span_owner(candidates) ||
            !span_role(model, P4_TABLE_INPUT_MODEL, 2) || !span_role(original_names, P4_TABLE_INPUT_MSG, 3) ||
            !span_role(affinity, P4_TABLE_INPUT_UNIT, 2))
            return P4_TABLE_INVALID;
        for (unsigned bank = 0; bank < BANK_COUNT; ++bank)
            if (p4_game_table_input_span_size(spans[bank]) != (size_t)P4_PERSONA_RETAIL_ROWS * bank_stride[bank])
                return P4_TABLE_INVALID;
    }
    if (personas->count) {
        if (!p4_game_tables_text_semantic_revision(names)) return P4_TABLE_NOT_READY;
        const P4TextProfile *profile = p4_game_tables_text_profile(names);
        if (!profile || profile->encoding != P4_TEXT_ENCODING_US_RAW_NATIVE) return P4_TABLE_INVALID;
        qsort(personas->rows, personas->count, sizeof *personas->rows, compare);
        for (size_t i = 0; i < personas->count; ++i) {
            /* Dense IDs: no zero row is ever invented between retail and added personas. */
            if (personas->rows[i].view.desc.id != P4_PERSONA_RETAIL_ROWS + i) return P4_TABLE_INVALID;
            P4GameTextDefinition definition;
            int result = p4_game_tables_text_definition_find(names, P4_TEXT_PERSONA,
                personas->rows[i].view.desc.id, P4_TEXT_NAME, &definition);
            if (result) return result;
            if (!definition.size || definition.size > P4_PERSONA_NAME_SIZE) return P4_TABLE_INVALID;
        }
        personas->bytes = bank_offset(personas, BANK_COUNT);
    }
    memcpy(personas->spans, spans, sizeof spans);
    personas->original_input = metadata ? p4_game_table_input_span_owner(metadata) : NULL;
    personas->names = names;
    personas->semantic_revision = revision;
    return P4_TABLE_OK;
}
static int same_bytes(const P4GameTableInputSpan *span, const uint8_t *bytes, size_t size)
{
    return bytes && size == p4_game_table_input_span_size(span) &&
        !memcmp(p4_game_table_input_span_bytes(span), bytes, size);
}
int p4_game_personas_validate_original(const P4GamePersonas *personas,
    const uint8_t *metadata, size_t metadata_size, const uint8_t *growth, size_t growth_size,
    const uint8_t *model, size_t model_size, const uint8_t *candidates, size_t candidates_size,
    const uint8_t *original_names, size_t original_names_size, const uint8_t *affinity, size_t affinity_size)
{
    if (!personas || !personas->semantic_revision) return P4_TABLE_NOT_READY;
    if (!personas->spans[BANK_METADATA]) return P4_TABLE_OK;
    return same_bytes(personas->spans[BANK_METADATA], metadata, metadata_size) &&
        same_bytes(personas->spans[BANK_GROWTH], growth, growth_size) &&
        same_bytes(personas->spans[BANK_MODEL], model, model_size) &&
        same_bytes(personas->spans[BANK_CANDIDATES], candidates, candidates_size) &&
        same_bytes(personas->spans[BANK_NAMES], original_names, original_names_size) &&
        same_bytes(personas->spans[BANK_AFFINITY], affinity, affinity_size) ?
        P4_TABLE_OK : P4_TABLE_CONFLICT;
}
int p4_game_personas_materialize(P4GamePersonas *personas,
    const uint8_t *metadata, size_t metadata_size, const uint8_t *growth, size_t growth_size,
    const uint8_t *model, size_t model_size, const uint8_t *candidates, size_t candidates_size,
    const uint8_t *original_names, size_t original_names_size, const uint8_t *affinity, size_t affinity_size)
{
    int result = p4_game_personas_validate_original(personas, metadata, metadata_size, growth, growth_size,
        model, model_size, candidates, candidates_size, original_names, original_names_size, affinity, affinity_size);
    if (result || personas->revision) return result;
    if (!personas->count) { personas->revision = personas->semantic_revision; return P4_TABLE_OK; }
    if (!personas->allocator.allocate) return P4_TABLE_NOT_FOUND;
    uint32_t guest = 0;
    uint8_t *arena = personas->allocator.allocate(personas->allocator.user, personas->bytes, &guest);
    if (!arena) return P4_TABLE_NOMEM;
    if (!guest || (guest & 15u) || guest > UINT32_MAX - personas->bytes) {
        personas->allocator.release(personas->allocator.user, guest, arena, personas->bytes);
        return P4_TABLE_INVALID;
    }
    memset(arena, 0, personas->bytes);
    const uint8_t *original[BANK_COUNT] = {metadata, growth, model, candidates, original_names, affinity};
    for (unsigned bank = 0; bank < BANK_COUNT; ++bank)
        memcpy(arena + bank_offset(personas, bank), original[bank], (size_t)P4_PERSONA_RETAIL_ROWS * bank_stride[bank]);
    for (size_t i = 0; i < personas->count; ++i) {
        const P4GamePersonaDesc *row = &personas->rows[i].view.desc;
        P4GameTextDefinition name;
        if (p4_game_tables_text_definition_find(personas->names, P4_TEXT_PERSONA, row->id, P4_TEXT_NAME, &name) ||
            !name.size || name.size > P4_PERSONA_NAME_SIZE) {
            personas->allocator.release(personas->allocator.user, guest, arena, personas->bytes);
            return P4_TABLE_NOT_READY;
        }
        memcpy(arena + bank_offset(personas, BANK_METADATA) + row->id * P4_PERSONA_METADATA_SIZE, row->metadata, P4_PERSONA_METADATA_SIZE);
        memcpy(arena + bank_offset(personas, BANK_GROWTH) + row->id * P4_PERSONA_GROWTH_SIZE, row->growth, P4_PERSONA_GROWTH_SIZE);
        memcpy(arena + bank_offset(personas, BANK_MODEL) + row->id * P4_PERSONA_MODEL_SIZE, row->model, P4_PERSONA_MODEL_SIZE);
        memcpy(arena + bank_offset(personas, BANK_CANDIDATES) + row->id * P4_PERSONA_CANDIDATES_SIZE, row->candidates, P4_PERSONA_CANDIDATES_SIZE);
        memcpy(arena + bank_offset(personas, BANK_NAMES) + row->id * P4_PERSONA_NAME_SIZE, name.bytes, name.size);
        memcpy(arena + bank_offset(personas, BANK_AFFINITY) + row->id * P4_PERSONA_AFFINITY_SIZE, row->affinity, P4_PERSONA_AFFINITY_SIZE);
    }
    personas->arena = arena;
    personas->guest = guest;
    personas->revision = personas->semantic_revision;
    return P4_TABLE_OK;
}
uint32_t p4_game_personas_semantic_revision(const P4GamePersonas *personas) { return personas ? personas->semantic_revision : 0; }
uint32_t p4_game_personas_revision(const P4GamePersonas *personas) { return personas ? personas->revision : 0; }
size_t p4_game_personas_count(const P4GamePersonas *personas) { return personas ? personas->count : 0; }
uint32_t p4_game_personas_limit(const P4GamePersonas *personas) { return personas ? personas->limit : P4_PERSONA_RETAIL_ROWS; }
const P4GamePersonaView *p4_game_personas_at(const P4GamePersonas *personas, size_t index)
{
    return personas && personas->semantic_revision && index < personas->count ? &personas->rows[index].view : NULL;
}
const P4GamePersonaView *p4_game_personas_find(const P4GamePersonas *personas, uint32_t id)
{
    if (!personas || !personas->semantic_revision || id < P4_PERSONA_RETAIL_ROWS || id >= personas->limit) return NULL;
    return &personas->rows[id - P4_PERSONA_RETAIL_ROWS].view;
}
int p4_game_personas_model_source(const P4GamePersonas *personas, uint32_t id, uint32_t *model_id)
{
    if (!personas || !model_id) return P4_TABLE_INVALID;
    if (!personas->semantic_revision) return P4_TABLE_NOT_READY;
    if (id < P4_PERSONA_RETAIL_ROWS) { *model_id = id; return P4_TABLE_OK; }
    const P4GamePersonaView *row = p4_game_personas_find(personas, id);
    if (!row) return P4_TABLE_NOT_FOUND;
    *model_id = row->desc.model_source;
    return P4_TABLE_OK;
}
static uint32_t bank_guest(const P4GamePersonas *personas, unsigned bank)
{
    return personas && personas->revision && personas->guest ? personas->guest + bank_offset(personas, bank) : 0;
}
uint32_t p4_game_personas_metadata(const P4GamePersonas *personas) { return bank_guest(personas, BANK_METADATA); }
uint32_t p4_game_personas_growth(const P4GamePersonas *personas) { return bank_guest(personas, BANK_GROWTH); }
uint32_t p4_game_personas_model(const P4GamePersonas *personas) { return bank_guest(personas, BANK_MODEL); }
uint32_t p4_game_personas_candidates(const P4GamePersonas *personas) { return bank_guest(personas, BANK_CANDIDATES); }
uint32_t p4_game_personas_names(const P4GamePersonas *personas) { return bank_guest(personas, BANK_NAMES); }
uint32_t p4_game_personas_affinity(const P4GamePersonas *personas) { return bank_guest(personas, BANK_AFFINITY); }
