#include "game_items_internal.h"
#include <stdlib.h>
#include <string.h>

typedef P4GameItemMutable ItemState;
typedef struct ItemTextBinding { uint32_t name, description; } ItemTextBinding;
struct P4GameItems {
    P4GameItemView *rows;
    P4GameItemCategoryView *categories;
    ItemState *state;
    ItemTextBinding *texts;
    const uint8_t *retail_file;
    const P4GameText *names, *descriptions;
    size_t count, capacity, categories_count, categories_capacity;
    P4GameTableAllocator allocator;
    uint8_t *arena;
    uint32_t guest, allocation_size, limit, revision, semantic_revision;
    uint32_t names_revision, descriptions_revision;
    uint16_t equipment[10][3];
    struct { P4GameItemActionFn fn; void *user; } actions[32];
    P4GameItemUseFn runtime_use;
    void *runtime_context;
};
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
static int key_valid(const char *key)
{
    if (!key) return 0;
    size_t size = 0;
    while (size < P4_TABLE_KEY_SIZE && key[size]) ++size;
    if (size == P4_TABLE_KEY_SIZE) return 0;
    const char *colon = strchr(key, ':');
    if (!colon || colon == key || (size_t)(colon - key) >= 64) return 0;
    char owner[64]; size_t length = (size_t)(colon - key);
    memcpy(owner, key, length); owner[length] = 0;
    return identifier(owner, sizeof owner) && identifier(colon + 1, P4_TABLE_KEY_SIZE - length - 1);
}
static int owned_key(const char *owner, const char *key)
{
    if (!identifier(owner, 64) || !key_valid(key)) return 0;
    size_t length = strlen(owner);
    return !strncmp(owner, key, length) && key[length] == ':';
}
static int image_ref_valid(const P4ResourceRef *icon)
{
    return icon && icon->revision && owned_key(icon->owner, icon->key);
}
static const P4GameItemActionRef empty_action;
static int action_ref_valid(const P4GameItemActionRef *reference)
{
    return !memcmp(reference, &empty_action, sizeof *reference) ||
        (reference->revision && owned_key(reference->owner, reference->key));
}
static int action_ref_equal(const P4GameItemActionRef *left, const P4GameItemActionRef *right)
{
    return left->revision == right->revision && !strcmp(left->owner, right->owner) && !strcmp(left->key, right->key);
}
static int reserve(void **data, size_t *capacity, size_t count, size_t stride)
{
    if (count <= *capacity) return P4_TABLE_OK;
    if (count > SIZE_MAX / stride) return P4_TABLE_NOMEM;
    size_t next = *capacity ? *capacity * 2 : 32;
    if (next < count || next > SIZE_MAX / stride) next = count;
    void *grown = realloc(*data, next * stride);
    if (!grown) return P4_TABLE_NOMEM;
    *data = grown; *capacity = next;
    return P4_TABLE_OK;
}
static int item_compare(const void *a, const void *b)
{
    uint32_t x = ((const P4GameItemView *)a)->id, y = ((const P4GameItemView *)b)->id;
    return (x > y) - (x < y);
}
static int category_compare(const void *a, const void *b)
{
    uint32_t x = ((const P4GameItemCategoryView *)a)->id, y = ((const P4GameItemCategoryView *)b)->id;
    return (x > y) - (x < y);
}
ItemState *p4_game_tables_items_mutable(const P4GameItems *t, uint32_t id)
{
    if (!t || !t->revision || id < P4_ITEM_RETAIL_COUNT || id >= t->limit) return NULL;
    ItemState *state = &t->state[id - P4_ITEM_RETAIL_COUNT];
    return state->row ? state : NULL;
}
P4GameItems *p4_game_tables_items_create(const P4GameTableAllocator *allocator)
{
    if (allocator && (!allocator->allocate || !allocator->release)) return NULL;
    P4GameItems *t = calloc(1, sizeof *t);
    if (t) { t->limit = P4_ITEM_RETAIL_COUNT; if (allocator) t->allocator = *allocator; }
    return t;
}
void p4_game_tables_items_destroy(P4GameItems *t)
{
    if (!t) return;
    if (t->arena) t->allocator.release(t->allocator.user, t->guest, t->arena, t->allocation_size);
    free(t->state); free(t->texts); free(t->categories); free(t->rows); free(t);
}
int p4_game_tables_items_bind_runtime_use(P4GameItems *t, P4GameItemUseFn use, void *context)
{
    if (!t || !use || !context) return P4_TABLE_INVALID;
    if (t->semantic_revision) return P4_TABLE_FROZEN;
    if (t->runtime_use)
        return t->runtime_use == use && t->runtime_context == context ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    t->runtime_use = use; t->runtime_context = context;
    return P4_TABLE_OK;
}
int p4_game_tables_items_validate_runtime_sources(const P4GameItems *t)
{
    if (!t) return P4_TABLE_INVALID;
    if (!t->count && !t->categories_count)
        return !t->retail_file || (t->runtime_use && t->runtime_context) ? P4_TABLE_OK : P4_TABLE_NOT_READY;
    if (!t->runtime_use || !t->runtime_context) return P4_TABLE_NOT_READY;
    int result = p4_game_tables_text_validate_runtime_sources(t->names);
    return result ? result : p4_game_tables_text_validate_runtime_sources(t->descriptions);
}
int p4_game_tables_items_runtime_use(const P4GameItems *t, uint32_t id)
{
    if (!t || !t->revision) return P4_TABLE_NOT_READY;
    if (id >= t->limit || (id < P4_ITEM_RETAIL_COUNT ? !t->retail_file : !p4_game_tables_items_mutable(t, id)))
        return P4_TABLE_NOT_FOUND;
    if (!t->runtime_use || !t->runtime_context) return P4_TABLE_NOT_READY;
    int result = t->runtime_use(t, P4_ITEM_USE_ROW, id, t->runtime_context);
    return result <= 0 ? result : P4_TABLE_INVALID;
}
int p4_game_tables_items_category_runtime_use(const P4GameItems *t, uint32_t category)
{
    if (!t || !t->revision) return P4_TABLE_NOT_READY;
    if (!p4_game_tables_items_category_find(t, category)) return P4_TABLE_NOT_FOUND;
    if (!t->runtime_use || !t->runtime_context) return P4_TABLE_NOT_READY;
    int result = t->runtime_use(t, P4_ITEM_USE_CATEGORY, category, t->runtime_context);
    return result <= 0 ? result : P4_TABLE_INVALID;
}
int p4_game_tables_items_text_runtime_use(const P4GameItems *t, uint32_t table, uint32_t id, uint32_t role)
{
    if (!t || !t->revision) return P4_TABLE_NOT_READY;
    if (role > P4_TEXT_DESCRIPTION) return P4_TABLE_INVALID;
    return p4_game_tables_text_runtime_use(role == P4_TEXT_NAME ? t->names : t->descriptions, table, id, role);
}
int p4_game_tables_items_register(P4GameItems *t, const char *owner, const P4GameItemDesc *row)
{
    if (!t || !row || !owned_key(owner, row->key) || !image_ref_valid(&row->icon) || !row->revision ||
        row->id < P4_ITEM_RETAIL_COUNT || row->id > P4_ITEM_MAX_ID || row->metadata.kind > 31) return P4_TABLE_INVALID;
    if (t->semantic_revision) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < t->count; ++i)
        if (t->rows[i].id == row->id || !strcmp(t->rows[i].key, row->key)) return P4_TABLE_CONFLICT;
    int result = reserve((void **)&t->rows, &t->capacity, t->count + 1, sizeof *t->rows);
    if (result) return result;
    P4GameItemView *view = &t->rows[t->count]; memset(view, 0, sizeof *view);
    view->id = row->id; view->revision = row->revision;
    strcpy(view->owner, owner); strcpy(view->key, row->key); view->icon = row->icon;
    view->metadata = row->metadata;
    if (t->limit <= row->id) t->limit = row->id + 1;
    ++t->count;
    return P4_TABLE_OK;
}
int p4_game_tables_items_requirement_register(P4GameItems *t, const char *owner,
    uint32_t id, uint32_t gate, uint32_t required_id, uint32_t quantity)
{
    if (!t || !identifier(owner, 64) || id < P4_ITEM_RETAIL_COUNT || id > P4_ITEM_MAX_ID ||
        gate > 1 || required_id > P4_ITEM_MAX_ID || quantity > 255) return P4_TABLE_INVALID;
    if (t->semantic_revision) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < t->count; ++i) {
        P4GameItemView *row = &t->rows[i];
        if (row->id != id) continue;
        if (strcmp(row->owner, owner) || (row->requirements_mask & (1u << gate))) return P4_TABLE_CONFLICT;
        row->requirements[gate] = (P4GameItemRequirement){required_id, quantity};
        row->requirements_mask |= 1u << gate;
        return P4_TABLE_OK;
    }
    return P4_TABLE_NOT_FOUND;
}
int p4_game_tables_items_models_register(P4GameItems *t, const char *owner,
    uint32_t id, uint32_t right_model_id, uint32_t left_model_id)
{
    if (!t || !identifier(owner, 64) || id < P4_ITEM_RETAIL_COUNT || id > P4_ITEM_MAX_ID ||
        right_model_id > 255 || (left_model_id != P4_ITEM_MODEL_NONE &&
            (left_model_id < 1000 || left_model_id > 1255))) return P4_TABLE_INVALID;
    if (t->semantic_revision) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < t->count; ++i) {
        P4GameItemView *row = &t->rows[i];
        if (row->id != id) continue;
        if (strcmp(row->owner, owner) || row->models_registered) return P4_TABLE_CONFLICT;
        const P4ItemRow *metadata = &row->metadata;
        if (metadata->kind || ((metadata->category_mask & 0x40u) != 0) != (left_model_id != P4_ITEM_MODEL_NONE) ||
            ((metadata->category_mask & 0x80u) && (right_model_id < 0x70u || right_model_id >= 0x96u)))
            return P4_TABLE_INVALID;
        row->models_registered = 1;
        row->right_model_id = right_model_id; row->left_model_id = left_model_id;
        return P4_TABLE_OK;
    }
    return P4_TABLE_NOT_FOUND;
}
int p4_game_tables_items_check_models(const P4GameItems *t, P4GameItemModelCheck check, void *user)
{
    if (!t || !check) return P4_TABLE_INVALID;
    for (size_t i = 0; i < t->count; ++i) {
        const P4GameItemView *row = &t->rows[i];
        const P4ItemRow *metadata = &row->metadata;
        if (metadata->kind) continue;
        if (!row->models_registered) return P4_TABLE_NOT_FOUND;
        int result = check(user, row->right_model_id, row->left_model_id, metadata->category_mask);
        if (result) return result;
    }
    return P4_TABLE_OK;
}
int p4_game_tables_items_action_register(P4GameItems *t, const char *owner,
    uint32_t category, const P4GameItemActionRef *reference, P4GameItemActionFn action, void *user)
{
    if (!t || !identifier(owner, 64) || category < 6 || category > 31 || !action) return P4_TABLE_INVALID;
    if (t->semantic_revision) return P4_TABLE_FROZEN;
    if (!reference) reference = &empty_action;
    if (!action_ref_valid(reference)) return P4_TABLE_INVALID;
    for (size_t i = 0; i < t->categories_count; ++i) {
        const P4GameItemCategoryView *row = &t->categories[i];
        if (row->id != category) continue;
        if (strcmp(row->owner, owner) || !action_ref_equal(&row->action, reference)) return P4_TABLE_CONFLICT;
        if (t->actions[category].fn)
            return t->actions[category].fn == action && t->actions[category].user == user ?
                P4_TABLE_OK : P4_TABLE_CONFLICT;
        t->actions[category].fn = action; t->actions[category].user = user;
        return P4_TABLE_OK;
    }
    return P4_TABLE_NOT_FOUND;
}
int p4_game_tables_items_prepare_actions(P4GameItems *t, P4GameItemActionPrepareFn prepare, void *context)
{
    if (!t) return P4_TABLE_INVALID;
    if (t->semantic_revision) return P4_TABLE_FROZEN;
    const size_t count = t->categories_count;
    for (size_t i = 0; i < count; ++i) {
        const P4GameItemCategoryView *category = &t->categories[i];
        if (!category->action.key[0]) continue;
        if (!prepare) return P4_TABLE_NOT_READY;
        int result = prepare(t, category->owner, category->id, &category->action, context);
        if (result) return result < 0 ? result : P4_TABLE_INVALID;
    }
    return P4_TABLE_OK;
}
int p4_game_tables_items_action_dispatch(P4GameItems *t, uint32_t id)
{
    const P4ItemRow *row = p4_game_tables_items_metadata(t, id);
    if (!row || row->kind < 6 || row->kind > 31 || !t->actions[row->kind].fn) return P4_ITEM_ACTION_ERROR;
    int result = t->actions[row->kind].fn(t->actions[row->kind].user, id, row->kind);
    return result == P4_ITEM_ACTION_HANDLED || result == P4_ITEM_ACTION_DEFERRED ? result : P4_ITEM_ACTION_ERROR;
}
int p4_game_tables_items_action_bound(const P4GameItems *t, uint32_t category)
{ return t && t->revision && category < 32 && t->actions[category].fn != NULL; }
static int registered_sorted(const P4GameItems *t, uint32_t id)
{
    size_t lower = 0, upper = t->count;
    while (lower < upper) {
        size_t middle = lower + (upper - lower) / 2;
        if (t->rows[middle].id < id) lower = middle + 1;
        else if (t->rows[middle].id > id) upper = middle;
        else return 1;
    }
    return 0;
}
int p4_game_tables_items_category_register(P4GameItems *t, const char *owner, const P4GameItemCategoryDesc *category)
{
    if (!t || !category || !owned_key(owner, category->key) || !image_ref_valid(&category->icon) || !action_ref_valid(&category->action) ||
        category->id < 6 || category->id > 31 || !category->revision ||
        category->stock_policy < P4_ITEM_STOCK_NONE || category->stock_policy > P4_ITEM_STOCK_REQUIREMENTS) return P4_TABLE_INVALID;
    if (t->semantic_revision) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < t->categories_count; ++i)
        if (t->categories[i].id == category->id || !strcmp(t->categories[i].key, category->key)) return P4_TABLE_CONFLICT;
    int result = reserve((void **)&t->categories, &t->categories_capacity, t->categories_count + 1, sizeof *t->categories);
    if (result) return result;
    P4GameItemCategoryView *view = &t->categories[t->categories_count++]; memset(view, 0, sizeof *view);
    view->id = category->id; view->revision = category->revision;
    view->filter_a = category->filter_a; view->filter_b = category->filter_b;
    view->stock_policy = category->stock_policy;
    strcpy(view->owner, owner); strcpy(view->key, category->key); view->icon = category->icon;
    view->action = category->action;
    return P4_TABLE_OK;
}
static int definitions(const P4GameText *names, const P4GameText *descriptions, uint32_t table, uint32_t id)
{
    P4GameTextDefinition definition;
    int result = p4_game_tables_text_definition_find(names, table, id, P4_TEXT_NAME, &definition);
    if (result) return result;
    return p4_game_tables_text_definition_find(descriptions, table, id, P4_TEXT_DESCRIPTION, &definition);
}
static int resolve_binding(const P4GameItems *t, uint32_t table, uint32_t id, ItemTextBinding *binding)
{
    const P4GameTextRow *name = p4_game_tables_text_find(t->names, table, id, P4_TEXT_NAME);
    const P4GameTextRow *description = p4_game_tables_text_find(t->descriptions, table, id, P4_TEXT_DESCRIPTION);
    if (!name || !description || !name->stream || !description->stream) return P4_TABLE_NOT_READY;
    *binding = (ItemTextBinding){name->stream, description->stream};
    return P4_TABLE_OK;
}
static uint32_t little_word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
int p4_game_tables_items_seal(P4GameItems *t, const uint8_t *file, size_t size,
    const P4GameText *names, const P4GameText *descriptions, uint32_t revision)
{
    if (!t || !revision) return P4_TABLE_INVALID;
    if (t->semantic_revision) return t->semantic_revision == revision ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    if ((file || size || t->count) &&
        (!file || size != P4_ITEM_RETAIL_FILE_SIZE || little_word(file) != P4_ITEM_RETAIL_COUNT)) return P4_TABLE_INVALID;
    if (t->count || t->categories_count) {
        const P4TextProfile *np = p4_game_tables_text_profile(names), *dp = p4_game_tables_text_profile(descriptions);
        if (!np || !dp || np->encoding != P4_TEXT_ENCODING_US_RAW_NATIVE || dp->encoding != P4_TEXT_ENCODING_US_NATIVE ||
            !p4_game_tables_text_semantic_revision(names) || !p4_game_tables_text_semantic_revision(descriptions)) return P4_TABLE_INVALID;
    }
    if (t->count > 1) qsort(t->rows, t->count, sizeof *t->rows, item_compare);
    for (size_t i = 0; i < t->count; ++i) {
        const P4GameItemView *view = &t->rows[i];
        uint32_t category = view->metadata.kind;
        uint32_t stock_policy = category < 3 ? P4_ITEM_STOCK_REQUIREMENTS :
            category == 3 ? P4_ITEM_STOCK_CALENDAR : P4_ITEM_STOCK_NONE;
        if (!category && !view->models_registered) return P4_TABLE_NOT_FOUND;
        if (category >= 6) {
            size_t index = 0;
            while (index < t->categories_count && t->categories[index].id != category) ++index;
            if (index == t->categories_count || !t->actions[category].fn) return P4_TABLE_NOT_FOUND;
            stock_policy = t->categories[index].stock_policy;
        }
        if (view->requirements_mask && stock_policy != P4_ITEM_STOCK_REQUIREMENTS) return P4_TABLE_INVALID;
        for (uint32_t gate = 0; gate < 2; ++gate)
            if ((view->requirements_mask & (1u << gate)) && view->requirements[gate].item_id >= P4_ITEM_RETAIL_COUNT &&
                !registered_sorted(t, view->requirements[gate].item_id)) return P4_TABLE_NOT_FOUND;
        int result = definitions(names, descriptions, P4_TEXT_ITEM, view->id);
        if (result) return result;
    }
    for (size_t i = 0; i < t->categories_count; ++i) {
        const P4GameItemCategoryView *view = &t->categories[i];
        if (!t->actions[view->id].fn) return P4_TABLE_NOT_FOUND;
        int result = definitions(names, descriptions, P4_TEXT_ITEM_CATEGORY, view->id);
        if (result) return result;
    }
    if (t->categories_count > 1) qsort(t->categories, t->categories_count, sizeof *t->categories, category_compare);
    t->retail_file = file; t->names = names; t->descriptions = descriptions;
    t->names_revision = p4_game_tables_text_semantic_revision(names);
    t->descriptions_revision = p4_game_tables_text_semantic_revision(descriptions);
    t->semantic_revision = revision;
    return P4_TABLE_OK;
}
int p4_game_tables_items_native_admit(const P4GameItems *t)
{
    if (!t) return P4_TABLE_INVALID;
    if (!t->semantic_revision) return P4_TABLE_NOT_READY;
    return t->count && !t->allocator.allocate ? P4_TABLE_NOT_FOUND : P4_TABLE_OK;
}
int p4_game_tables_items_materialize(P4GameItems *t)
{
    if (!t) return P4_TABLE_INVALID;
    if (!t->semantic_revision) return P4_TABLE_NOT_READY;
    if (t->revision) return P4_TABLE_FROZEN;
    if (!t->count && !t->categories_count) { t->revision = t->semantic_revision; return P4_TABLE_OK; }
    if (t->count && !t->allocator.allocate) return P4_TABLE_NOT_FOUND;
    if (p4_game_tables_text_revision(t->names) != t->names_revision ||
        p4_game_tables_text_revision(t->descriptions) != t->descriptions_revision) return P4_TABLE_NOT_READY;
    ItemTextBinding *texts = malloc((t->count + t->categories_count) * sizeof *texts);
    if (!texts) return P4_TABLE_NOMEM;
    int result = P4_TABLE_OK;
    for (size_t i = 0; !result && i < t->count; ++i)
        result = resolve_binding(t, P4_TEXT_ITEM, t->rows[i].id, &texts[i]);
    for (size_t i = 0; !result && i < t->categories_count; ++i)
        result = resolve_binding(t, P4_TEXT_ITEM_CATEGORY, t->categories[i].id, &texts[t->count + i]);
    if (result) { free(texts); return result; }
    ItemState *state = NULL; uint8_t *arena = NULL; uint32_t guest = 0, bytes = 0;
    if (t->count) {
        state = calloc(t->limit - P4_ITEM_RETAIL_COUNT, sizeof *state);
        if (!state) { free(texts); return P4_TABLE_NOMEM; }
        bytes = P4_ITEM_FILE_HEADER_SIZE + t->limit * P4_ITEM_ROW_SIZE;
        arena = t->allocator.allocate(t->allocator.user, bytes, &guest);
        if (!arena) { free(state); free(texts); return P4_TABLE_NOMEM; }
        if (!guest || (guest & 3u) || guest > UINT32_MAX - bytes) {
            t->allocator.release(t->allocator.user, guest, arena, bytes); free(state); free(texts); return P4_TABLE_INVALID;
        }
        memcpy(arena, t->retail_file, P4_ITEM_RETAIL_FILE_SIZE);
        uint32_t next = P4_ITEM_RETAIL_COUNT;
        for (size_t i = 0; i < t->count; ++i) {
            const P4GameItemView *view = &t->rows[i];
            if (view->id > next)
                memset(arena + P4_ITEM_FILE_HEADER_SIZE + next * P4_ITEM_ROW_SIZE, 0, (view->id - next) * P4_ITEM_ROW_SIZE);
            memcpy(arena + P4_ITEM_FILE_HEADER_SIZE + view->id * P4_ITEM_ROW_SIZE, &view->metadata, P4_ITEM_ROW_SIZE);
            next = view->id + 1u;
            state[view->id - P4_ITEM_RETAIL_COUNT].row = (uint16_t)(i + 1u);
        }
        for (unsigned i = 0; i < 4; ++i) arena[i] = (uint8_t)(t->limit >> (i * 8));
    }
    t->texts = texts; t->state = state; t->arena = arena; t->guest = guest;
    t->allocation_size = bytes; t->revision = t->semantic_revision;
    return P4_TABLE_OK;
}
uint32_t p4_game_tables_items_semantic_revision(const P4GameItems *t) { return t ? t->semantic_revision : 0; }
int p4_game_tables_items_definition_find(const P4GameItems *t, uint32_t id, P4GameItemDefinition *out)
{
    if (!t || !out) return P4_TABLE_INVALID;
    if (!t->semantic_revision) return P4_TABLE_NOT_READY;
    if (id < P4_ITEM_RETAIL_COUNT) {
        if (!t->retail_file) return P4_TABLE_NOT_FOUND;
        *out = (P4GameItemDefinition){id, 0, NULL,
            (const P4ItemRow *)(t->retail_file + P4_ITEM_FILE_HEADER_SIZE + id * P4_ITEM_ROW_SIZE)};
    } else {
        const P4GameItemView *view = p4_game_tables_items_find(t, id);
        if (!view) return P4_TABLE_NOT_FOUND;
        *out = (P4GameItemDefinition){id, view->revision, view->owner, &view->metadata};
    }
    return P4_TABLE_OK;
}
int p4_game_tables_items_check_prefix(const P4GameItems *t, const uint8_t *bytes, size_t size)
{
    if (!t || !t->semantic_revision) return P4_TABLE_NOT_READY;
    if (!t->retail_file) return t->count ? P4_TABLE_NOT_READY : P4_TABLE_OK;
    return bytes && size == P4_ITEM_RETAIL_FILE_SIZE && !memcmp(bytes, t->retail_file, size) ?
        P4_TABLE_OK : P4_TABLE_CONFLICT;
}
uint32_t p4_game_tables_items_revision(const P4GameItems *t) { return t ? t->revision : 0; }
size_t p4_game_tables_items_count(const P4GameItems *t) { return t ? t->count : 0; }
const P4GameItemView *p4_game_tables_items_at(const P4GameItems *t, size_t index)
{ return t && t->semantic_revision && index < t->count ? &t->rows[index] : NULL; }
const P4GameItemView *p4_game_tables_items_find(const P4GameItems *t, uint32_t id)
{
    if (!t || !t->semantic_revision || id < P4_ITEM_RETAIL_COUNT || id >= t->limit) return NULL;
    if (t->revision) {
        ItemState *state = p4_game_tables_items_mutable(t, id);
        return state ? &t->rows[state->row - 1u] : NULL;
    }
    size_t lower = 0, upper = t->count;
    while (lower < upper) {
        size_t middle = lower + (upper - lower) / 2;
        if (t->rows[middle].id < id) lower = middle + 1u;
        else if (t->rows[middle].id > id) upper = middle;
        else return &t->rows[middle];
    }
    return NULL;
}
size_t p4_game_tables_items_category_count(const P4GameItems *t) { return t ? t->categories_count : 0; }
const P4GameItemCategoryView *p4_game_tables_items_category_at(const P4GameItems *t, size_t index)
{ return t && t->semantic_revision && index < t->categories_count ? &t->categories[index] : NULL; }
const P4GameItemCategoryView *p4_game_tables_items_category_find(const P4GameItems *t, uint32_t id)
{
    if (!t || !t->semantic_revision) return NULL;
    size_t lower = 0, upper = t->categories_count;
    while (lower < upper) {
        size_t middle = lower + (upper - lower) / 2;
        const P4GameItemCategoryView *row = &t->categories[middle];
        if (row->id < id) lower = middle + 1;
        else if (row->id > id) upper = middle;
        else return row;
    }
    return NULL;
}
uint32_t p4_game_tables_items_text_binding(const P4GameItems *t, uint32_t table, uint32_t id, uint32_t role)
{
    if (!t || !t->revision || !t->texts || (role != P4_TEXT_NAME && role != P4_TEXT_DESCRIPTION)) return 0;
    size_t index;
    if (table == P4_TEXT_ITEM) {
        const P4GameItemView *view = p4_game_tables_items_find(t, id);
        if (!view) return 0;
        index = (size_t)(view - t->rows);
    } else if (table == P4_TEXT_ITEM_CATEGORY) {
        const P4GameItemCategoryView *view = p4_game_tables_items_category_find(t, id);
        if (!view) return 0;
        index = t->count + (size_t)(view - t->categories);
    } else return 0;
    return role == P4_TEXT_NAME ? t->texts[index].name : t->texts[index].description;
}
const P4ItemRow *p4_game_tables_items_metadata(const P4GameItems *t, uint32_t id)
{
    if (!t || !t->revision || !t->arena || id >= t->limit || (id >= P4_ITEM_RETAIL_COUNT && !p4_game_tables_items_mutable(t, id))) return NULL;
    return (const P4ItemRow *)(t->arena + P4_ITEM_FILE_HEADER_SIZE + id * P4_ITEM_ROW_SIZE);
}
uint32_t p4_game_tables_items_limit(const P4GameItems *t) { return t && t->semantic_revision ? t->limit : 0; }
uint32_t p4_game_tables_items_base(const P4GameItems *t) { return t && t->revision && t->guest ? t->guest + P4_ITEM_FILE_HEADER_SIZE : 0; }
int p4_game_tables_items_state_get(const P4GameItems *t, uint32_t id, P4GameItemState *out)
{
    ItemState *state = p4_game_tables_items_mutable(t, id);
    if (!state || !out) return P4_TABLE_NOT_FOUND;
    *out = (P4GameItemState){id, state->quantity, state->availability}; return P4_TABLE_OK;
}
int p4_game_tables_items_state_set(P4GameItems *t, uint32_t id, uint32_t quantity, uint32_t availability)
{
    ItemState *state = p4_game_tables_items_mutable(t, id);
    if (!state) return P4_TABLE_NOT_FOUND;
    if (quantity > 99 || availability > 255) return P4_TABLE_INVALID;
    state->quantity = (uint8_t)quantity; state->availability = (uint8_t)availability; return P4_TABLE_OK;
}
int p4_game_tables_items_equipment_get(const P4GameItems *t, uint32_t unit, uint32_t slot, uint32_t *id)
{
    if (!t || !t->revision || !id || !unit || unit > 10 || slot > 2) return P4_TABLE_INVALID;
    uint32_t item = t->equipment[unit - 1][slot];
    if (!item) return P4_TABLE_NOT_FOUND;
    *id = item; return P4_TABLE_OK;
}
int p4_game_tables_items_equipment_check(const P4GameItems *t, uint32_t unit, uint32_t slot, uint32_t id)
{
    if (!t || !t->revision || !unit || unit > 10 || slot > 2 || id < P4_ITEM_RETAIL_COUNT) return P4_TABLE_INVALID;
    const P4ItemRow *row = p4_game_tables_items_metadata(t, id);
    if (!row) return P4_TABLE_NOT_FOUND;
    return row->kind == slot && (row->category_mask & (1u << (unit + 4))) ? P4_TABLE_OK : P4_TABLE_INVALID;
}
int p4_game_tables_items_equipment_set(P4GameItems *t, uint32_t unit, uint32_t slot, uint32_t id)
{
    int result = p4_game_tables_items_equipment_check(t, unit, slot, id);
    if (result) return result;
    t->equipment[unit - 1][slot] = (uint16_t)id; return P4_TABLE_OK;
}
int p4_game_tables_items_equipment_clear(P4GameItems *t, uint32_t unit, uint32_t slot)
{
    if (!t || !t->revision || !unit || unit > 10 || slot > 2) return P4_TABLE_INVALID;
    t->equipment[unit - 1][slot] = 0; return P4_TABLE_OK;
}
void p4_game_tables_items_state_clear(P4GameItems *t)
{
    if (!t || !t->revision) return;
    for (size_t i = 0; i < t->count; ++i) {
        ItemState *state = &t->state[t->rows[i].id - P4_ITEM_RETAIL_COUNT];
        state->quantity = state->availability = 0;
    }
    memset(t->equipment, 0, sizeof t->equipment);
}
void p4_game_tables_items_state_apply(P4GameItems *t,
    const P4GameItemState *rows, size_t count, const uint32_t equipment[10][3])
{
    p4_game_tables_items_state_clear(t);
    for (size_t i = 0; i < count; ++i) {
        ItemState *state = &t->state[rows[i].id - P4_ITEM_RETAIL_COUNT];
        state->quantity = rows[i].quantity;
        state->availability = rows[i].availability;
    }
    for (unsigned unit = 0; unit < 10; ++unit)
        for (unsigned slot = 0; slot < 3; ++slot)
            t->equipment[unit][slot] = (uint16_t)equipment[unit][slot];
}