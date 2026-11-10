#include "p4pc/game_text.h"
#include <stdlib.h>
#include <string.h>

#define TEXT_OWNER_SIZE 64u
#define TEXT_TOTAL_MAX 0x01000000u

typedef struct Text {
    P4GameTextRow row;
    char owner[TEXT_OWNER_SIZE];
    uint8_t *staged;
    uint32_t offset;
} Text;
struct P4GameText {
    Text *rows;
    size_t count, capacity;
    uint32_t total, revision, semantic_revision, guest;
    uint8_t *arena;
    P4GameTableAllocator allocator;
    P4TextProfile profile;
    P4GameTextUseFn runtime_use;
    void *runtime_context;
};

static int owner_valid(const char *s)
{
    if (!s || !s[0] || s[0] == '.') return 0;
    for (size_t n = 0; n < TEXT_OWNER_SIZE; ++n) {
        unsigned char c = (unsigned char)s[n];
        if (!c) return n && s[n - 1] != '.';
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')) return 0;
    }
    return 0;
}
int p4_game_tables_text_validate(const P4TextProfile *profile, uint32_t role, const uint8_t *bytes, size_t size)
{
    if (!profile || profile->font != P4_TEXT_FONT_US_NATIVE || role > P4_TEXT_DESCRIPTION ||
        (profile->encoding != P4_TEXT_ENCODING_US_NATIVE && profile->encoding != P4_TEXT_ENCODING_US_RAW_NATIVE) ||
        !size || size > (role == P4_TEXT_NAME ? 512u : 8192u) || (role == P4_TEXT_NAME && size == 1) ||
        p4_native_glyphs_validate(bytes, size, (P4NativeGlyphFlavor)profile->encoding,
            role == P4_TEXT_DESCRIPTION)) return P4_TABLE_INVALID;
    return P4_TABLE_OK;
}
static int compare(const void *left, const void *right)
{
    const Text *a = left, *b = right;
#define CMP(field) if (a->row.field != b->row.field) return (a->row.field > b->row.field) - (a->row.field < b->row.field)
    CMP(table); CMP(id); CMP(role);
#undef CMP
    return 0;
}
P4GameText *p4_game_tables_text_create(const P4GameTableAllocator *allocator, const P4TextProfile *profile)
{
    if (allocator && (!allocator->allocate || !allocator->release)) return NULL;
    if (!profile || profile->font != P4_TEXT_FONT_US_NATIVE ||
        (profile->encoding != P4_TEXT_ENCODING_US_NATIVE && profile->encoding != P4_TEXT_ENCODING_US_RAW_NATIVE)) return NULL;
    P4GameText *t = calloc(1, sizeof *t);
    if (t && allocator) t->allocator = *allocator;
    if (t) t->profile = *profile;
    return t;
}
void p4_game_tables_text_destroy(P4GameText *t)
{
    if (!t) return;
    for (size_t i = 0; i < t->count; ++i) free(t->rows[i].staged);
    if (t->arena) t->allocator.release(t->allocator.user, t->guest, t->arena, t->total);
    free(t->rows); free(t);
}
int p4_game_tables_text_bind_runtime_use(P4GameText *t, P4GameTextUseFn use, void *context)
{
    if (!t || !use || !context) return P4_TABLE_INVALID;
    if (t->semantic_revision) return P4_TABLE_FROZEN;
    if (t->runtime_use)
        return t->runtime_use == use && t->runtime_context == context ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    t->runtime_use = use; t->runtime_context = context;
    return P4_TABLE_OK;
}
int p4_game_tables_text_validate_runtime_sources(const P4GameText *t)
{
    if (!t) return P4_TABLE_INVALID;
    return !t->count || (t->runtime_use && t->runtime_context) ? P4_TABLE_OK : P4_TABLE_NOT_READY;
}
int p4_game_tables_text_register(P4GameText *t, const char *owner, uint32_t table, uint32_t id,
                          uint32_t role, uint32_t revision, const uint8_t *bytes, size_t size)
{
    if (!t || !owner_valid(owner) || table > P4_TEXT_ITEM_CATEGORY || role > P4_TEXT_DESCRIPTION || !revision) return P4_TABLE_INVALID;
    if (t->semantic_revision) return P4_TABLE_FROZEN;
    if (p4_game_tables_text_validate(&t->profile, role, bytes, size) || size > TEXT_TOTAL_MAX - t->total) return P4_TABLE_INVALID;
    Text key = {0}; key.row.table = table; key.row.id = id; key.row.role = role;
    for (size_t i = 0; i < t->count; ++i) if (!compare(&key, &t->rows[i])) return P4_TABLE_CONFLICT;
    uint8_t *copy = malloc(size);
    if (!copy) return P4_TABLE_NOMEM;
    if (t->count == t->capacity) {
        size_t capacity = t->capacity ? t->capacity * 2 : 16;
        if (capacity > SIZE_MAX / sizeof *t->rows) { free(copy); return P4_TABLE_NOMEM; }
        Text *rows = realloc(t->rows, capacity * sizeof *rows);
        if (!rows) { free(copy); return P4_TABLE_NOMEM; }
        t->rows = rows; t->capacity = capacity;
    }
    Text *row = &t->rows[t->count++]; memset(row, 0, sizeof *row);
    row->row = (P4GameTextRow){table, id, role, revision, (uint32_t)size, 0};
    strcpy(row->owner, owner); memcpy(copy, bytes, size); row->staged = copy;
    t->total += (uint32_t)size;
    return P4_TABLE_OK;
}
int p4_game_tables_text_seal(P4GameText *t, uint32_t revision)
{
    if (!t || !revision) return P4_TABLE_INVALID;
    if (t->semantic_revision) return t->semantic_revision == revision ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    if (t->count) qsort(t->rows, t->count, sizeof *t->rows, compare);
    uint32_t offset = 0;
    for (size_t i = 0; i < t->count; ++i) { t->rows[i].offset = offset; offset += t->rows[i].row.size; }
    t->semantic_revision = revision;
    return P4_TABLE_OK;
}
uint32_t p4_game_tables_text_semantic_revision(const P4GameText *t) { return t ? t->semantic_revision : 0; }
static void definition(const P4GameText *t, const Text *row, P4GameTextDefinition *out)
{
    *out = (P4GameTextDefinition){row->row.table, row->row.id, row->row.role, row->row.revision, row->row.size,
        row->owner, t->revision ? t->arena + row->offset : row->staged};
}
int p4_game_tables_text_definition_find(const P4GameText *t, uint32_t table,
    uint32_t id, uint32_t role, P4GameTextDefinition *out)
{
    if (!t || !out || table > P4_TEXT_ITEM_CATEGORY || role > P4_TEXT_DESCRIPTION) return P4_TABLE_INVALID;
    if (!t->semantic_revision) return P4_TABLE_NOT_READY;
    Text key = {0}; key.row.table = table; key.row.id = id; key.row.role = role;
    const Text *row = t->count ? bsearch(&key, t->rows, t->count, sizeof *t->rows, compare) : NULL;
    if (!row) return P4_TABLE_NOT_FOUND;
    definition(t, row, out);
    return P4_TABLE_OK;
}
int p4_game_tables_text_definition_at(const P4GameText *t, size_t index, P4GameTextDefinition *out)
{
    if (!t || !out) return P4_TABLE_INVALID;
    if (!t->semantic_revision) return P4_TABLE_NOT_READY;
    if (index >= t->count) return P4_TABLE_NOT_FOUND;
    definition(t, t->rows + index, out);
    return P4_TABLE_OK;
}
int p4_game_tables_text_materialize(P4GameText *t)
{
    if (!t) return P4_TABLE_INVALID;
    if (t->revision) return P4_TABLE_FROZEN;
    if (!t->semantic_revision) return P4_TABLE_NOT_READY;
    if (t->total) {
        if (!t->allocator.allocate) return P4_TABLE_NOT_FOUND;
        uint32_t guest = 0;
        uint8_t *arena = t->allocator.allocate(t->allocator.user, t->total, &guest);
        if (!arena) return P4_TABLE_NOMEM;
        if (!guest || guest > UINT32_MAX - t->total) {
            t->allocator.release(t->allocator.user, guest, arena, t->total);
            return P4_TABLE_INVALID;
        }
        t->arena = arena; t->guest = guest;
    }
    for (size_t i = 0; i < t->count; ++i) {
        Text *row = &t->rows[i]; row->row.stream = t->guest + row->offset;
        memcpy(t->arena + row->offset, row->staged, row->row.size);
        free(row->staged); row->staged = NULL;
    }
    t->revision = t->semantic_revision;
    return P4_TABLE_OK;
}
int p4_game_tables_text_freeze(P4GameText *t, uint32_t revision)
{
    if (!t || !revision) return P4_TABLE_INVALID;
    if (t->revision) return P4_TABLE_FROZEN;
    int result = p4_game_tables_text_seal(t, revision);
    return result ? result : p4_game_tables_text_materialize(t);
}
uint32_t p4_game_tables_text_revision(const P4GameText *t) { return t ? t->revision : 0; }
const P4TextProfile *p4_game_tables_text_profile(const P4GameText *t) { return t ? &t->profile : NULL; }
size_t p4_game_tables_text_count(const P4GameText *t) { return t ? t->count : 0; }
const P4GameTextRow *p4_game_tables_text_find(const P4GameText *t, uint32_t table, uint32_t id, uint32_t role)
{
    if (!t || !t->revision || !t->count) return NULL;
    Text key = {0}; key.row.table = table; key.row.id = id; key.row.role = role;
    const Text *row = bsearch(&key, t->rows, t->count, sizeof *t->rows, compare);
    return row ? &row->row : NULL;
}
int p4_game_tables_text_runtime_use(const P4GameText *t, uint32_t table, uint32_t id, uint32_t role)
{
    if (!t || table > P4_TEXT_ITEM_CATEGORY || role > P4_TEXT_DESCRIPTION) return P4_TABLE_INVALID;
    if (!t->revision) return P4_TABLE_NOT_READY;
    if (!p4_game_tables_text_find(t, table, id, role)) return P4_TABLE_NOT_FOUND;
    int result = p4_game_tables_text_validate_runtime_sources(t);
    if (result) return result;
    result = t->runtime_use(t, table, id, role, t->runtime_context);
    return result <= 0 ? result : P4_TABLE_INVALID;
}
const P4GameTextRow *p4_game_tables_text_at(const P4GameText *t, size_t index)
{ return t && t->revision && index < t->count ? &t->rows[index].row : NULL; }
const uint8_t *p4_game_tables_text_bytes(const P4GameText *t, size_t index)
{ return t && t->revision && index < t->count ? t->arena + t->rows[index].offset : NULL; }
const char *p4_game_tables_text_owner(const P4GameText *t, size_t index)
{ return t && t->revision && index < t->count ? t->rows[index].owner : NULL; }
