#include "p4pc/game_enemies.h"
#include "p4pc/game_table_inputs.h"
#include <stdlib.h>
#include <string.h>

enum { BANK_METADATA, BANK_AFFINITY, BANK_MODEL, BANK_AI, BANK_NAMES, BANK_COUNT };
static const uint32_t bank_stride[BANK_COUNT] = {
    P4_ENEMY_ROW_SIZE, P4_ENEMY_AFFINITY_SIZE, P4_ENEMY_MODEL_SIZE, P4_ENEMY_AI_SIZE, P4_ENEMY_NAME_SIZE
};
typedef struct EnemyRow {
    P4GameEnemyView view;
} EnemyRow;
struct P4GameEnemies {
    EnemyRow *rows;
    size_t count, capacity;
    P4GameTableAllocator allocator;
    uint8_t *arena;
    uint32_t guest, bytes, limit, revision, semantic_revision;
    const P4GameTableInputSpan *spans[BANK_COUNT];
    const P4GameText *names;
    const P4GameTableInput *original_input;
    P4GameTableSourceUseFn original_use;
    P4GameEnemyRowUseFn authored_use;
    void *source_context;
};
int p4_game_enemies_bind_runtime_use(P4GameEnemies *enemies,
    P4GameTableSourceUseFn original_use, P4GameEnemyRowUseFn authored_use, void *source_context)
{
    if (!enemies) return P4_TABLE_INVALID;
    if (enemies->semantic_revision) return P4_TABLE_FROZEN;
    if (!original_use || (!!authored_use != !!source_context)) return P4_TABLE_INVALID;
    if (enemies->original_use)
        return enemies->original_use == original_use && enemies->authored_use == authored_use &&
            enemies->source_context == source_context ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    enemies->original_use = original_use;
    enemies->authored_use = authored_use;
    enemies->source_context = source_context;
    return P4_TABLE_OK;
}
int p4_game_enemies_validate_runtime_sources(const P4GameEnemies *enemies)
{
    if (!enemies) return P4_TABLE_INVALID;
    if (!enemies->semantic_revision) return P4_TABLE_NOT_READY;
    if (!enemies->spans[BANK_METADATA]) return enemies->count ? P4_TABLE_INVALID : P4_TABLE_OK;
    if (!enemies->original_input || !enemies->original_use ||
        (enemies->count && (!enemies->authored_use || !enemies->source_context))) return P4_TABLE_NOT_READY;
    return enemies->count ? p4_game_tables_text_validate_runtime_sources(enemies->names) : P4_TABLE_OK;
}
int p4_game_enemies_runtime_use(const P4GameEnemies *enemies, uint32_t id)
{
    if (!enemies || !enemies->revision) return P4_TABLE_NOT_READY;
    int result = p4_game_enemies_validate_runtime_sources(enemies);
    if (result) return result;
    if (id < P4_ENEMY_RETAIL_ROWS) {
        if (!enemies->spans[BANK_METADATA]) return P4_TABLE_NOT_FOUND;
        result = enemies->original_use(enemies->spans[BANK_METADATA], (void *)enemies->original_input);
    } else {
        if (id >= enemies->limit) return P4_TABLE_NOT_FOUND;
        /* Root proves the exact row and its captured origin; no provider inference here. */
        result = enemies->authored_use(enemies, id, enemies->source_context);
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
    uint32_t x = ((const EnemyRow *)a)->view.desc.id, y = ((const EnemyRow *)b)->view.desc.id;
    return (x > y) - (x < y);
}
P4GameEnemies *p4_game_enemies_create(const P4GameTableAllocator *allocator)
{
    if (allocator && (!allocator->allocate || !allocator->release)) return NULL;
    P4GameEnemies *enemies = calloc(1, sizeof *enemies);
    if (enemies) {
        if (allocator) enemies->allocator = *allocator;
        enemies->limit = P4_ENEMY_RETAIL_ROWS;
    }
    return enemies;
}
void p4_game_enemies_destroy(P4GameEnemies *enemies)
{
    if (!enemies) return;
    if (enemies->arena) enemies->allocator.release(enemies->allocator.user,
        enemies->guest, enemies->arena, enemies->bytes);
    free(enemies->rows);
    free(enemies);
}
int p4_game_enemies_register(P4GameEnemies *enemies, const char *owner, const P4GameEnemyDesc *desc)
{
    if (!enemies || !desc || !owned_key(owner, desc->key) || !desc->schema || !desc->revision ||
        desc->id < P4_ENEMY_RETAIL_ROWS || desc->id > P4_ENEMY_MAX_ID ||
        desc->model_source >= P4_ENEMY_RETAIL_ROWS)
        return P4_TABLE_INVALID;
    if (enemies->semantic_revision) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < enemies->count; ++i)
        if (enemies->rows[i].view.desc.id == desc->id || !strcmp(enemies->rows[i].view.desc.key, desc->key))
            return P4_TABLE_CONFLICT;
    if (enemies->count == enemies->capacity) {
        size_t capacity = enemies->capacity ? enemies->capacity * 2 : 16;
        if (capacity > SIZE_MAX / sizeof *enemies->rows) return P4_TABLE_NOMEM;
        EnemyRow *rows = realloc(enemies->rows, capacity * sizeof *rows);
        if (!rows) return P4_TABLE_NOMEM;
        enemies->rows = rows;
        enemies->capacity = capacity;
    }
    EnemyRow *row = &enemies->rows[enemies->count++];
    memset(row, 0, sizeof *row);
    row->view.desc = *desc;
    strcpy(row->view.owner, owner);
    if (enemies->limit <= desc->id) enemies->limit = desc->id + 1;
    return P4_TABLE_OK;
}
int p4_game_enemies_native_admit(const P4GameEnemies *enemies)
{
    if (!enemies) return P4_TABLE_INVALID;
    if (enemies->count && !enemies->allocator.allocate) return P4_TABLE_NOT_FOUND;
    return P4_TABLE_OK;
}
static uint32_t bank_offset(const P4GameEnemies *enemies, unsigned bank)
{
    uint32_t offset = 0;
    for (unsigned i = 0; i < bank; ++i) offset = (offset + enemies->limit * bank_stride[i] + 15u) & ~15u;
    return offset;
}
static int span_role(const P4GameTableInputSpan *span, P4GameTableInputKind kind, size_t index)
{
    const P4GameTableInput *owner = p4_game_table_input_span_owner(span);
    return owner && p4_game_table_input_kind(owner) == kind &&
        p4_game_table_input_span_index(span) == index &&
        p4_game_table_input_span(owner, index) == span && p4_game_table_input_span_bytes(span);
}
int p4_game_enemies_seal(P4GameEnemies *enemies,
    const P4GameTableInputSpan *metadata, const P4GameTableInputSpan *affinity,
    const P4GameTableInputSpan *model, const P4GameTableInputSpan *ai,
    const P4GameTableInputSpan *original_names, const P4GameText *names, uint32_t revision)
{
    const P4GameTableInputSpan *spans[BANK_COUNT] = {metadata, affinity, model, ai, original_names};
    if (!enemies || !revision) return P4_TABLE_INVALID;
    if (enemies->semantic_revision)
        return enemies->semantic_revision == revision && enemies->names == names &&
            !memcmp(enemies->spans, spans, sizeof spans) ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    if (enemies->count || metadata || affinity || model || ai || original_names) {
        if (!span_role(metadata, P4_TABLE_INPUT_UNIT, 0) || !span_role(affinity, P4_TABLE_INPUT_UNIT, 1) ||
            p4_game_table_input_span_owner(metadata) != p4_game_table_input_span_owner(affinity) ||
            !span_role(model, P4_TABLE_INPUT_MODEL, 1) || !span_role(ai, P4_TABLE_INPUT_AICALC, 8) ||
            !span_role(original_names, P4_TABLE_INPUT_MSG, 2))
            return P4_TABLE_INVALID;
        for (unsigned bank = 0; bank < BANK_COUNT; ++bank)
            if (p4_game_table_input_span_size(spans[bank]) != (size_t)P4_ENEMY_RETAIL_ROWS * bank_stride[bank])
                return P4_TABLE_INVALID;
    }
    if (enemies->count) {
        if (!p4_game_tables_text_semantic_revision(names)) return P4_TABLE_NOT_READY;
        const P4TextProfile *profile = p4_game_tables_text_profile(names);
        if (!profile || profile->encoding != P4_TEXT_ENCODING_US_RAW_NATIVE) return P4_TABLE_INVALID;
        qsort(enemies->rows, enemies->count, sizeof *enemies->rows, compare);
        for (size_t i = 0; i < enemies->count; ++i) {
            /* Dense IDs: no zero row is ever invented between retail and added enemies. */
            if (enemies->rows[i].view.desc.id != P4_ENEMY_RETAIL_ROWS + i) return P4_TABLE_INVALID;
            P4GameTextDefinition definition;
            int result = p4_game_tables_text_definition_find(names, P4_TEXT_ENEMY,
                enemies->rows[i].view.desc.id, P4_TEXT_NAME, &definition);
            if (result) return result;
            if (!definition.size || definition.size > P4_ENEMY_NAME_SIZE) return P4_TABLE_INVALID;
        }
        enemies->bytes = bank_offset(enemies, BANK_COUNT);
    }
    memcpy(enemies->spans, spans, sizeof spans);
    enemies->original_input = metadata ? p4_game_table_input_span_owner(metadata) : NULL;
    enemies->names = names;
    enemies->semantic_revision = revision;
    return P4_TABLE_OK;
}
static int same_bytes(const P4GameTableInputSpan *span, const uint8_t *bytes, size_t size)
{
    return bytes && size == p4_game_table_input_span_size(span) &&
        !memcmp(p4_game_table_input_span_bytes(span), bytes, size);
}
int p4_game_enemies_validate_original(const P4GameEnemies *enemies,
    const uint8_t *metadata, size_t metadata_size, const uint8_t *affinity, size_t affinity_size,
    const uint8_t *model, size_t model_size, const uint8_t *ai, size_t ai_size,
    const uint8_t *original_names, size_t original_names_size)
{
    if (!enemies || !enemies->semantic_revision) return P4_TABLE_NOT_READY;
    if (!enemies->spans[BANK_METADATA]) return P4_TABLE_OK;
    return same_bytes(enemies->spans[BANK_METADATA], metadata, metadata_size) &&
        same_bytes(enemies->spans[BANK_AFFINITY], affinity, affinity_size) &&
        same_bytes(enemies->spans[BANK_MODEL], model, model_size) &&
        same_bytes(enemies->spans[BANK_AI], ai, ai_size) &&
        same_bytes(enemies->spans[BANK_NAMES], original_names, original_names_size) ?
        P4_TABLE_OK : P4_TABLE_CONFLICT;
}
int p4_game_enemies_materialize(P4GameEnemies *enemies,
    const uint8_t *metadata, size_t metadata_size, const uint8_t *affinity, size_t affinity_size,
    const uint8_t *model, size_t model_size, const uint8_t *ai, size_t ai_size,
    const uint8_t *original_names, size_t original_names_size)
{
    int result = p4_game_enemies_validate_original(enemies, metadata, metadata_size, affinity, affinity_size,
        model, model_size, ai, ai_size, original_names, original_names_size);
    if (result || enemies->revision) return result;
    if (!enemies->count) { enemies->revision = enemies->semantic_revision; return P4_TABLE_OK; }
    if (!enemies->allocator.allocate) return P4_TABLE_NOT_FOUND;
    uint32_t guest = 0;
    uint8_t *arena = enemies->allocator.allocate(enemies->allocator.user, enemies->bytes, &guest);
    if (!arena) return P4_TABLE_NOMEM;
    if (!guest || (guest & 15u) || guest > UINT32_MAX - enemies->bytes) {
        enemies->allocator.release(enemies->allocator.user, guest, arena, enemies->bytes);
        return P4_TABLE_INVALID;
    }
    memset(arena, 0, enemies->bytes);
    const uint8_t *original[BANK_COUNT] = {metadata, affinity, model, ai, original_names};
    for (unsigned bank = 0; bank < BANK_COUNT; ++bank)
        memcpy(arena + bank_offset(enemies, bank), original[bank], (size_t)P4_ENEMY_RETAIL_ROWS * bank_stride[bank]);
    for (size_t i = 0; i < enemies->count; ++i) {
        const P4GameEnemyDesc *row = &enemies->rows[i].view.desc;
        P4GameTextDefinition name;
        if (p4_game_tables_text_definition_find(enemies->names, P4_TEXT_ENEMY, row->id, P4_TEXT_NAME, &name) ||
            !name.size || name.size > P4_ENEMY_NAME_SIZE) {
            enemies->allocator.release(enemies->allocator.user, guest, arena, enemies->bytes);
            return P4_TABLE_NOT_READY;
        }
        memcpy(arena + bank_offset(enemies, BANK_METADATA) + row->id * P4_ENEMY_ROW_SIZE, row->metadata, P4_ENEMY_ROW_SIZE);
        memcpy(arena + bank_offset(enemies, BANK_AFFINITY) + row->id * P4_ENEMY_AFFINITY_SIZE, row->affinity, P4_ENEMY_AFFINITY_SIZE);
        memcpy(arena + bank_offset(enemies, BANK_MODEL) + row->id * P4_ENEMY_MODEL_SIZE, row->model, P4_ENEMY_MODEL_SIZE);
        memcpy(arena + bank_offset(enemies, BANK_AI) + row->id * P4_ENEMY_AI_SIZE, row->ai, P4_ENEMY_AI_SIZE);
        memcpy(arena + bank_offset(enemies, BANK_NAMES) + row->id * P4_ENEMY_NAME_SIZE, name.bytes, name.size);
    }
    enemies->arena = arena;
    enemies->guest = guest;
    enemies->revision = enemies->semantic_revision;
    return P4_TABLE_OK;
}
uint32_t p4_game_enemies_semantic_revision(const P4GameEnemies *enemies) { return enemies ? enemies->semantic_revision : 0; }
uint32_t p4_game_enemies_revision(const P4GameEnemies *enemies) { return enemies ? enemies->revision : 0; }
size_t p4_game_enemies_count(const P4GameEnemies *enemies) { return enemies ? enemies->count : 0; }
uint32_t p4_game_enemies_limit(const P4GameEnemies *enemies) { return enemies ? enemies->limit : P4_ENEMY_RETAIL_ROWS; }
const P4GameEnemyView *p4_game_enemies_at(const P4GameEnemies *enemies, size_t index)
{
    return enemies && enemies->semantic_revision && index < enemies->count ? &enemies->rows[index].view : NULL;
}
const P4GameEnemyView *p4_game_enemies_find(const P4GameEnemies *enemies, uint32_t id)
{
    if (!enemies || !enemies->semantic_revision || id < P4_ENEMY_RETAIL_ROWS || id >= enemies->limit) return NULL;
    return &enemies->rows[id - P4_ENEMY_RETAIL_ROWS].view;
}
int p4_game_enemies_model_source(const P4GameEnemies *enemies, uint32_t id, uint32_t *model_id)
{
    if (!enemies || !model_id) return P4_TABLE_INVALID;
    if (!enemies->semantic_revision) return P4_TABLE_NOT_READY;
    if (id < P4_ENEMY_RETAIL_ROWS) { *model_id = id; return P4_TABLE_OK; }
    const P4GameEnemyView *row = p4_game_enemies_find(enemies, id);
    if (!row) return P4_TABLE_NOT_FOUND;
    *model_id = row->desc.model_source;
    return P4_TABLE_OK;
}
static uint32_t bank_guest(const P4GameEnemies *enemies, unsigned bank)
{
    return enemies && enemies->revision && enemies->guest ? enemies->guest + bank_offset(enemies, bank) : 0;
}
uint32_t p4_game_enemies_metadata(const P4GameEnemies *enemies) { return bank_guest(enemies, BANK_METADATA); }
uint32_t p4_game_enemies_affinity(const P4GameEnemies *enemies) { return bank_guest(enemies, BANK_AFFINITY); }
uint32_t p4_game_enemies_model(const P4GameEnemies *enemies) { return bank_guest(enemies, BANK_MODEL); }
uint32_t p4_game_enemies_ai(const P4GameEnemies *enemies) { return bank_guest(enemies, BANK_AI); }
uint32_t p4_game_enemies_names(const P4GameEnemies *enemies) { return bank_guest(enemies, BANK_NAMES); }
