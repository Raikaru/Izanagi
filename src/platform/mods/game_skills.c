#include "p4pc/game_skills.h"
#include "p4pc/game_table_inputs.h"
#include <stdlib.h>
#include <string.h>

typedef struct SkillRow {
    P4GameSkillView view;
    uint32_t name, description;
} SkillRow;
struct P4GameSkills {
    SkillRow *rows;
    size_t count, capacity;
    P4GameTableAllocator allocator;
    uint8_t *arena;
    uint32_t guest, bytes, limit, revision, semantic_revision;
    const P4GameTableInputSpan *metadata, *elements, *presentation, *original_names;
    const P4GameTables *tables;
    const P4GameText *names, *descriptions;
    const P4GameTableInput *original_input;
    P4GameTableSourceUseFn original_use;
    P4GameSkillRowUseFn authored_use;
    void *source_context;
};
int p4_game_skills_bind_runtime_use(P4GameSkills *skills,
    P4GameTableSourceUseFn original_use, P4GameSkillRowUseFn authored_use, void *source_context)
{
    if (!skills) return P4_TABLE_INVALID;
    if (skills->semantic_revision) return P4_TABLE_FROZEN;
    if (!original_use || (!!authored_use != !!source_context)) return P4_TABLE_INVALID;
    if (skills->original_use)
        return skills->original_use == original_use && skills->authored_use == authored_use &&
            skills->source_context == source_context ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    skills->original_use = original_use;
    skills->authored_use = authored_use;
    skills->source_context = source_context;
    return P4_TABLE_OK;
}
int p4_game_skills_validate_runtime_sources(const P4GameSkills *skills)
{
    if (!skills) return P4_TABLE_INVALID;
    if (!skills->semantic_revision) return P4_TABLE_NOT_READY;
    if (!skills->elements) return skills->count ? P4_TABLE_INVALID : P4_TABLE_OK;
    if (!skills->original_input || !skills->original_use ||
        (skills->count && (!skills->authored_use || !skills->source_context))) return P4_TABLE_NOT_READY;
    if (!skills->count) return P4_TABLE_OK;
    int result = p4_game_tables_text_validate_runtime_sources(skills->names);
    return result ? result : p4_game_tables_text_validate_runtime_sources(skills->descriptions);
}
int p4_game_skills_runtime_use(const P4GameSkills *skills, uint32_t id)
{
    if (!skills || !skills->revision) return P4_TABLE_NOT_READY;
    int result = p4_game_skills_validate_runtime_sources(skills);
    if (result) return result;
    if (id < P4_SKILL_RETAIL_COUNT) {
        P4GameSkillDefinition definition;
        result = p4_game_skills_definition_find(skills, id, &definition);
        if (result) return result;
        if (definition.active && !definition.metadata) return P4_TABLE_NOT_FOUND;
        const P4GameTableInputSpan *span = definition.active ? skills->metadata : skills->elements;
        result = skills->original_use(span, (void *)skills->original_input);
    } else {
        if (id > P4_SKILL_MAX_ID || !skills->count) return P4_TABLE_NOT_FOUND;
        /* Root must prove actual row existence AND its exact captured origin.
         * Do not repeat its authoritative lookup or infer a provider here. */
        result = skills->authored_use(skills, id, skills->source_context);
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
    uint32_t x = ((const SkillRow *)a)->view.desc.id;
    uint32_t y = ((const SkillRow *)b)->view.desc.id;
    return (x > y) - (x < y);
}
P4GameSkills *p4_game_skills_create(const P4GameTableAllocator *allocator)
{
    if (allocator && (!allocator->allocate || !allocator->release)) return NULL;
    P4GameSkills *skills = calloc(1, sizeof *skills);
    if (skills) {
        if (allocator) skills->allocator = *allocator;
        skills->limit = P4_SKILL_RETAIL_COUNT;
    }
    return skills;
}
void p4_game_skills_destroy(P4GameSkills *skills)
{
    if (!skills) return;
    if (skills->arena) skills->allocator.release(skills->allocator.user,
        skills->guest, skills->arena, skills->bytes);
    free(skills->rows);
    free(skills);
}
int p4_game_skills_register(P4GameSkills *skills, const char *owner, const P4GameSkillDesc *desc)
{
    if (!skills || !desc || !owned_key(owner, desc->key) || !desc->schema || !desc->revision ||
        desc->id < P4_SKILL_RETAIL_COUNT || desc->id > P4_SKILL_MAX_ID || desc->active > 1 ||
        desc->inheritance_group >= P4_SKILL_INHERITANCE_GROUP_COUNT)
        return P4_TABLE_INVALID;
    if (skills->semantic_revision) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < skills->count; ++i) {
        if (skills->rows[i].view.desc.id == desc->id || !strcmp(skills->rows[i].view.desc.key, desc->key))
            return P4_TABLE_CONFLICT;
    }
    if (skills->count == skills->capacity) {
        size_t capacity = skills->capacity ? skills->capacity * 2 : 16;
        if (capacity > SIZE_MAX / sizeof *skills->rows) return P4_TABLE_NOMEM;
        SkillRow *rows = realloc(skills->rows, capacity * sizeof *rows);
        if (!rows) return P4_TABLE_NOMEM;
        skills->rows = rows;
        skills->capacity = capacity;
    }
    SkillRow *row = &skills->rows[skills->count++];
    memset(row, 0, sizeof *row);
    row->view.desc = *desc;
    strcpy(row->view.owner, owner);
    if (skills->limit <= desc->id) skills->limit = desc->id + 1;
    return P4_TABLE_OK;
}
int p4_game_skills_native_admit(const P4GameSkills *skills)
{
    if (!skills) return P4_TABLE_INVALID;
    if (skills->count && !skills->allocator.allocate) return P4_TABLE_NOT_FOUND;
    return P4_TABLE_OK;
}
static uint32_t element_offset(const P4GameSkills *skills)
{
    return (skills->limit * P4_SKILL_ROW_SIZE + 15u) & ~15u;
}
static uint32_t presentation_offset(const P4GameSkills *skills)
{
    return (element_offset(skills) + skills->limit * P4_SKILL_ELEMENT_SIZE + 15u) & ~15u;
}
static int span_role(const P4GameTableInputSpan *span, P4GameTableInputKind kind, size_t index)
{
    const P4GameTableInput *owner = p4_game_table_input_span_owner(span);
    return owner && p4_game_table_input_kind(owner) == kind &&
        p4_game_table_input_span_index(span) == index &&
        p4_game_table_input_span(owner, index) == span && p4_game_table_input_span_bytes(span);
}
int p4_game_skills_seal(P4GameSkills *skills,
    const P4GameTableInputSpan *metadata, const P4GameTableInputSpan *elements,
    const P4GameTableInputSpan *presentation, const P4GameTableInputSpan *original_names,
    const P4GameTables *tables, const P4GameText *names,
    const P4GameText *descriptions, uint32_t revision)
{
    if (!skills || !revision) return P4_TABLE_INVALID;
    if (skills->semantic_revision)
        return skills->semantic_revision == revision && skills->metadata == metadata &&
            skills->elements == elements && skills->presentation == presentation &&
            skills->original_names == original_names && skills->tables == tables &&
            skills->names == names && skills->descriptions == descriptions ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    if (skills->count || metadata || elements || presentation || original_names) {
        if (!span_role(metadata, P4_TABLE_INPUT_SKILL, 1) ||
            !span_role(elements, P4_TABLE_INPUT_SKILL, 0) ||
            p4_game_table_input_span_owner(metadata) != p4_game_table_input_span_owner(elements) ||
            !span_role(presentation, P4_TABLE_INPUT_EFFECT, 0) ||
            !span_role(original_names, P4_TABLE_INPUT_MSG, 1)) return P4_TABLE_INVALID;
        size_t metadata_size = p4_game_table_input_span_size(metadata);
        size_t elements_size = p4_game_table_input_span_size(elements);
        size_t presentation_size = p4_game_table_input_span_size(presentation);
        if (metadata_size % P4_SKILL_ROW_SIZE || elements_size % P4_SKILL_ELEMENT_SIZE ||
            presentation_size % 4u ||
            metadata_size < P4_SKILL_RETAIL_ACTIVE_COUNT * P4_SKILL_ROW_SIZE ||
            metadata_size > P4_SKILL_RETAIL_COUNT * P4_SKILL_ROW_SIZE ||
            elements_size < P4_SKILL_RETAIL_ACTIVE_COUNT * P4_SKILL_ELEMENT_SIZE ||
            elements_size > P4_SKILL_RETAIL_COUNT * P4_SKILL_ELEMENT_SIZE ||
            presentation_size < P4_SKILL_RETAIL_ACTIVE_COUNT * 4u ||
            presentation_size > P4_SKILL_RETAIL_COUNT * 4u ||
            p4_game_table_input_span_size(original_names) != P4_SKILL_RETAIL_COUNT * 19u)
            return P4_TABLE_INVALID;
    }
    if (skills->count) {
        if (!p4_game_tables_revision(tables) || !p4_game_tables_text_semantic_revision(names) ||
            !p4_game_tables_text_semantic_revision(descriptions)) return P4_TABLE_NOT_READY;
        const P4TextProfile *np = p4_game_tables_text_profile(names);
        const P4TextProfile *dp = p4_game_tables_text_profile(descriptions);
        if (!np || !dp || np->encoding != P4_TEXT_ENCODING_US_RAW_NATIVE ||
            dp->encoding != P4_TEXT_ENCODING_US_NATIVE) return P4_TABLE_INVALID;
        for (size_t i = 0; i < skills->count; ++i) {
            const P4GameSkillDesc *row = &skills->rows[i].view.desc;
            P4GameTextDefinition definition;
            int result = p4_game_tables_text_definition_find(names, P4_TEXT_SKILL,
                row->id, P4_TEXT_NAME, &definition);
            if (result) return result;
            result = p4_game_tables_text_definition_find(descriptions, P4_TEXT_SKILL,
                row->id, P4_TEXT_DESCRIPTION, &definition);
            if (result) return result;
            if (!p4_game_tables_find_element(tables, row->element_id)) return P4_TABLE_NOT_FOUND;
        }
        qsort(skills->rows, skills->count, sizeof *skills->rows, compare);
        skills->bytes = presentation_offset(skills) + skills->limit * 4u;
    }
    skills->metadata = metadata;
    skills->elements = elements;
    skills->presentation = presentation;
    skills->original_names = original_names;
    skills->original_input = p4_game_table_input_span_owner(metadata);
    skills->tables = tables;
    skills->names = names;
    skills->descriptions = descriptions;
    skills->semantic_revision = revision;
    return P4_TABLE_OK;
}
static int same_bytes(const P4GameTableInputSpan *span, const uint8_t *bytes, size_t size)
{
    return bytes && size == p4_game_table_input_span_size(span) &&
        !memcmp(p4_game_table_input_span_bytes(span), bytes, size);
}
int p4_game_skills_validate_original(const P4GameSkills *skills,
    const uint8_t *metadata, size_t metadata_size, const uint8_t *elements, size_t elements_size,
    const uint8_t *presentation, size_t presentation_size,
    const uint8_t *original_names, size_t original_names_size)
{
    if (!skills || !skills->semantic_revision) return P4_TABLE_NOT_READY;
    if (!skills->elements) return P4_TABLE_OK;
    return same_bytes(skills->metadata, metadata, metadata_size) &&
        same_bytes(skills->elements, elements, elements_size) &&
        same_bytes(skills->presentation, presentation, presentation_size) &&
        same_bytes(skills->original_names, original_names, original_names_size) ?
        P4_TABLE_OK : P4_TABLE_CONFLICT;
}
int p4_game_skills_materialize(P4GameSkills *skills,
    const uint8_t *metadata, size_t metadata_size, const uint8_t *elements, size_t elements_size,
    const uint8_t *presentation, size_t presentation_size,
    const uint8_t *original_names, size_t original_names_size)
{
    int result = p4_game_skills_validate_original(skills, metadata, metadata_size, elements, elements_size,
        presentation, presentation_size, original_names, original_names_size);
    if (result || skills->revision) return result;
    if (!skills->count) { skills->revision = skills->semantic_revision; return P4_TABLE_OK; }
    if (!skills->allocator.allocate) return P4_TABLE_NOT_FOUND;
    if (!p4_game_tables_text_revision(skills->names) || !p4_game_tables_text_revision(skills->descriptions))
        return P4_TABLE_NOT_READY;
    uint32_t guest = 0;
    uint8_t *arena = skills->allocator.allocate(skills->allocator.user, skills->bytes, &guest);
    if (!arena) return P4_TABLE_NOMEM;
    if (!guest || (guest & 15u) || guest > UINT32_MAX - skills->bytes) {
        skills->allocator.release(skills->allocator.user, guest, arena, skills->bytes);
        return P4_TABLE_INVALID;
    }
    memset(arena, 0, skills->bytes);
    uint8_t *element_rows = arena + element_offset(skills);
    uint8_t *presentation_rows = arena + presentation_offset(skills);
    memcpy(arena, metadata, metadata_size);
    memcpy(element_rows, elements, elements_size);
    memcpy(presentation_rows, presentation, presentation_size);
    for (size_t i = 0; i < skills->count; ++i) {
        SkillRow *binding = &skills->rows[i];
        const P4GameSkillDesc *row = &binding->view.desc;
        const P4GameTextRow *name = p4_game_tables_text_find(skills->names, P4_TEXT_SKILL, row->id, P4_TEXT_NAME);
        const P4GameTextRow *description = p4_game_tables_text_find(skills->descriptions, P4_TEXT_SKILL,
            row->id, P4_TEXT_DESCRIPTION);
        if (!name || !description || !name->stream || !description->stream) {
            skills->allocator.release(skills->allocator.user, guest, arena, skills->bytes);
            return P4_TABLE_NOT_READY;
        }
        binding->name = name->stream;
        binding->description = description->stream;
        memcpy(arena + row->id * P4_SKILL_ROW_SIZE, row->metadata, P4_SKILL_ROW_SIZE);
        memcpy(presentation_rows + row->id * 4u, row->presentation, 4);
        /* Full semantic IDs never alias a different legacy byte-sized leaf. */
        element_rows[row->id * P4_SKILL_ELEMENT_SIZE] = row->element_id < P4_ELEMENT_RETAIL_COUNT ?
            (uint8_t)row->element_id : P4_SKILL_LEGACY_ELEMENT_INVALID;
        element_rows[row->id * P4_SKILL_ELEMENT_SIZE + 1] = row->element_aux;
    }
    skills->arena = arena;
    skills->guest = guest;
    skills->revision = skills->semantic_revision;
    return P4_TABLE_OK;
}
uint32_t p4_game_skills_semantic_revision(const P4GameSkills *skills) { return skills ? skills->semantic_revision : 0; }
uint32_t p4_game_skills_revision(const P4GameSkills *skills) { return skills ? skills->revision : 0; }
size_t p4_game_skills_count(const P4GameSkills *skills) { return skills ? skills->count : 0; }
uint32_t p4_game_skills_limit(const P4GameSkills *skills) { return skills ? skills->limit : P4_SKILL_RETAIL_COUNT; }
const P4GameSkillView *p4_game_skills_at(const P4GameSkills *skills, size_t index)
{
    return skills && skills->semantic_revision && index < skills->count ? &skills->rows[index].view : NULL;
}
static const SkillRow *find(const P4GameSkills *skills, uint32_t id)
{
    if (!skills || !skills->semantic_revision) return NULL;
    size_t lower = 0, upper = skills->count;
    while (lower < upper) {
        size_t middle = lower + (upper - lower) / 2;
        uint32_t current = skills->rows[middle].view.desc.id;
        if (current < id) lower = middle + 1;
        else if (current > id) upper = middle;
        else return &skills->rows[middle];
    }
    return NULL;
}
const P4GameSkillView *p4_game_skills_find(const P4GameSkills *skills, uint32_t id)
{
    const SkillRow *row = find(skills, id);
    return row ? &row->view : NULL;
}
int p4_game_skills_definition_find(const P4GameSkills *skills, uint32_t id, P4GameSkillDefinition *definition)
{
    if (!skills || !definition) return P4_TABLE_INVALID;
    if (!skills->semantic_revision) return P4_TABLE_NOT_READY;
    P4GameSkillDefinition result = {.id = id};
    if (id < P4_SKILL_RETAIL_COUNT) {
        if (id >= p4_game_table_input_span_size(skills->elements) / P4_SKILL_ELEMENT_SIZE)
            return P4_TABLE_NOT_FOUND;
        result.retail_element = p4_game_table_input_span_bytes(skills->elements) + id * P4_SKILL_ELEMENT_SIZE;
        result.active = id < P4_SKILL_RETAIL_ACTIVE_COUNT;
        if (id < p4_game_table_input_span_size(skills->metadata) / P4_SKILL_ROW_SIZE)
            result.metadata = p4_game_table_input_span_bytes(skills->metadata) + id * P4_SKILL_ROW_SIZE;
    } else {
        const SkillRow *row = find(skills, id);
        if (!row) return P4_TABLE_NOT_FOUND;
        result.authored = &row->view.desc;
        result.metadata = row->view.desc.metadata;
        result.active = row->view.desc.active;
    }
    *definition = result;
    return P4_TABLE_OK;
}
uint32_t p4_game_skills_metadata(const P4GameSkills *skills) { return skills && skills->revision ? skills->guest : 0; }
uint32_t p4_game_skills_elements(const P4GameSkills *skills)
{
    return skills && skills->revision && skills->guest ? skills->guest + element_offset(skills) : 0;
}
uint32_t p4_game_skills_presentation(const P4GameSkills *skills)
{
    return skills && skills->revision && skills->guest ? skills->guest + presentation_offset(skills) : 0;
}
uint32_t p4_game_skills_name(const P4GameSkills *skills, uint32_t id)
{
    const SkillRow *row = skills && skills->revision ? find(skills, id) : NULL;
    return row ? row->name : 0;
}
uint32_t p4_game_skills_description(const P4GameSkills *skills, uint32_t id)
{
    const SkillRow *row = skills && skills->revision ? find(skills, id) : NULL;
    return row ? row->description : 0;
}
const P4GameText *p4_game_skills_text_view(const P4GameSkills *skills, uint32_t role)
{
    if (!skills || !skills->semantic_revision) return NULL;
    return role == P4_TEXT_NAME ? skills->names :
        role == P4_TEXT_DESCRIPTION ? skills->descriptions : NULL;
}
