#include "p4pc/game_table_sources.h"
#include "p4pc/game_skills.h"
#include "p4pc/game_items.h"
#include "p4pc/game_enemies.h"
#include "p4pc/game_personas.h"
#include "p4pc/game_text.h"
#include "p4pc/packages.h"
#include "p4pc/package_memory.h"
#include <stdlib.h>
#include <string.h>

typedef struct SourceView { const void *view; uint32_t kind, revision; } SourceView;
typedef struct SourceOrigin { P4TableSourceOrigin desc; const P4PackageProvider *provider; } SourceOrigin;
typedef struct SourceBinding { P4TableSourceEntry entry; uint32_t view, source; } SourceBinding;
typedef struct OwnerSource { const char *owner; uint32_t view, source; } OwnerSource;
struct P4GameTableSources {
    SourceView *views;
    SourceOrigin *origins;
    SourceBinding *bindings;
    OwnerSource *owners;
    size_t views_count, views_capacity, origins_count, origins_capacity;
    size_t bindings_count, bindings_capacity, owners_count, owners_bytes;
    int sealed, frozen;
};
static P4GameTableSources *staged_sources;
static const P4GameTableSources *active_sources;
static int current_package(const SourceOrigin *origin);
static int bounded_string(const char *text, size_t capacity, int empty)
{
    if (!text || (!empty && !*text)) return 0;
    for (size_t i = 0; i < capacity; ++i) if (!text[i]) return 1;
    return 0;
}
static int grow(void **rows, size_t *capacity, size_t count, size_t stride)
{
    if (count < *capacity) return P4_TABLE_OK;
    size_t next = *capacity ? *capacity : 8;
    if (next <= count) {
        if (next > SIZE_MAX / 2) return P4_TABLE_NOMEM;
        next *= 2;
    }
    if (next > SIZE_MAX / stride) return P4_TABLE_NOMEM;
    const size_t bytes = next * stride, old_bytes = *capacity * stride;
    if (p4_packages_memory_reserve(bytes)) return P4_TABLE_NOMEM;
    void *replacement = malloc(bytes);
    if (!replacement) { p4_packages_memory_cancel(bytes); return P4_TABLE_NOMEM; }
    if (count) memcpy(replacement, *rows, count * stride);
    p4_packages_memory_commit(bytes);
    free(*rows);
    if (old_bytes) p4_packages_memory_release(old_bytes);
    *rows = replacement; *capacity = next;
    return P4_TABLE_OK;
}
static uint32_t view_revision(uint32_t kind, const void *view)
{
    switch (kind) {
    case P4_TABLE_SOURCE_REGISTRY: return p4_game_tables_revision(view);
    case P4_TABLE_SOURCE_SKILLS: return p4_game_skills_revision(view);
    case P4_TABLE_SOURCE_ITEMS: return p4_game_tables_items_revision(view);
    case P4_TABLE_SOURCE_TEXT: return p4_game_tables_text_revision(view);
    case P4_TABLE_SOURCE_ENEMIES: return p4_game_enemies_revision(view);
    case P4_TABLE_SOURCE_PERSONAS: return p4_game_personas_revision(view);
    default: return 0;
    }
}
static uint32_t view_semantic_revision(uint32_t kind, const void *view)
{
    switch (kind) {
    case P4_TABLE_SOURCE_REGISTRY: return p4_game_tables_revision(view);
    case P4_TABLE_SOURCE_SKILLS: return p4_game_skills_semantic_revision(view);
    case P4_TABLE_SOURCE_ITEMS: return p4_game_tables_items_semantic_revision(view);
    case P4_TABLE_SOURCE_TEXT: return p4_game_tables_text_semantic_revision(view);
    case P4_TABLE_SOURCE_ENEMIES: return p4_game_enemies_semantic_revision(view);
    case P4_TABLE_SOURCE_PERSONAS: return p4_game_personas_semantic_revision(view);
    default: return 0;
    }
}
static int entry_finish(P4TableSourceEntry *entry, const char *owner, const char *key,
    uint32_t schema, uint32_t revision)
{
    if (!bounded_string(owner, sizeof entry->owner, 0) ||
        !bounded_string(key, sizeof entry->key, 1) || !schema || !revision) return P4_TABLE_INVALID;
    strcpy(entry->owner, owner); strcpy(entry->key, key);
    entry->schema = schema; entry->revision = revision;
    return P4_TABLE_OK;
}
static size_t registry_count(const P4GameTables *tables, uint32_t domain)
{
    switch (domain) {
    case 0: return p4_game_tables_element_count(tables);
    case 1: return p4_game_tables_channel_count(tables);
    case 2: return p4_game_tables_affinity_count(tables);
    case 3: return p4_game_tables_action_element_count(tables);
    case 4: return p4_game_tables_affinity_alias_count(tables);
    case 5: return p4_game_tables_affinity_effect_count(tables);
    case 6: return p4_game_tables_damage_cover_count(tables);
    case 7: return p4_game_tables_side_ailment_count(tables);
    case 8: return p4_game_tables_side_rider_count(tables);
    default: return p4_game_tables_offensive_effect_count(tables);
    }
}
/* Registry cursor domains 0..5 are ELEMENT..EFFECT; 6, 7, 8 and 9 are the later
 * COVER, SIDE_AILMENT, SIDE_RIDER and OFFENSE. */
#define REGISTRY_DOMAINS 10u
static int registry_next(const P4GameTables *tables, P4TableSourceCursor *cursor, P4TableSourceEntry *out)
{
    while (cursor->domain < REGISTRY_DOMAINS) {
        if (cursor->index >= registry_count(tables, cursor->domain)) {
            ++cursor->domain; cursor->index = 0; continue;
        }
        const size_t index = cursor->index++;
        P4TableSourceEntry entry = {.domain = cursor->domain == 6u ? P4_TABLE_SOURCE_COVER :
            cursor->domain == 7u ? P4_TABLE_SOURCE_SIDE_AILMENT :
            cursor->domain == 8u ? P4_TABLE_SOURCE_SIDE_RIDER :
            cursor->domain == 9u ? P4_TABLE_SOURCE_OFFENSE : cursor->domain + 1u};
        const char *owner, *key = "";
        uint32_t schema = 1, revision = p4_game_tables_revision(tables);
        switch (entry.domain) {
        case P4_TABLE_SOURCE_ELEMENT: {
            const P4ElementDesc *row = p4_game_tables_element_at(tables, index);
            owner = p4_game_tables_element_owner(tables, row->id);
            entry.ids[0] = row->id; key = row->key; revision = row->revision;
            break;
        }
        case P4_TABLE_SOURCE_CHANNEL: {
            const P4AffinityChannelDesc *row = p4_game_tables_channel_at(tables, index);
            owner = p4_game_tables_channel_owner(tables, row->id);
            entry.ids[0] = row->id; key = row->key; revision = row->revision;
            break;
        }
        case P4_TABLE_SOURCE_AFFINITY: {
            const P4AffinityRow *row = p4_game_tables_affinity_at(tables, index);
            owner = p4_game_tables_affinity_owner(tables, index);
            entry.ids[0] = row->entity_kind; entry.ids[1] = row->entity_id; entry.ids[2] = row->profile_id;
            entry.ids[3] = row->element_id; entry.ids[4] = row->channel_id;
            break;
        }
        case P4_TABLE_SOURCE_ACTION: {
            const P4ActionElementBinding *row = p4_game_tables_action_element_at(tables, index);
            owner = p4_game_tables_action_element_owner(tables, index);
            entry.ids[0] = row->source_kind; entry.ids[1] = row->source_id; entry.ids[2] = row->component;
            key = row->key; schema = row->schema; revision = row->revision;
            break;
        }
        case P4_TABLE_SOURCE_ALIAS: {
            const P4AffinityProfileAlias *row = p4_game_tables_affinity_alias_at(tables, index);
            owner = p4_game_tables_affinity_alias_owner(tables, index);
            entry.ids[0] = row->entity_kind; entry.ids[1] = row->entity_id; entry.ids[2] = row->native_profile;
            key = row->key; schema = row->schema; revision = row->revision;
            break;
        }
        case P4_TABLE_SOURCE_EFFECT: {
            const P4AffinityEffectBinding *row = p4_game_tables_affinity_effect_at(tables, index);
            owner = p4_game_tables_affinity_effect_owner(tables, index);
            entry.ids[0] = row->source_kind; entry.ids[1] = row->source_id;
            entry.ids[2] = row->element_id; entry.ids[3] = row->channel_id;
            key = row->key; schema = row->schema; revision = row->revision;
            break;
        }
        case P4_TABLE_SOURCE_COVER: {
            const P4DamageCoverBinding *row = p4_game_tables_damage_cover_at(tables, index);
            owner = p4_game_tables_damage_cover_owner(tables, index);
            entry.ids[0] = row->source_kind; entry.ids[1] = row->source_id;
            entry.ids[2] = row->element_id; entry.ids[3] = row->channel_id;
            key = row->key; schema = row->schema; revision = row->revision;
            break;
        }
        case P4_TABLE_SOURCE_OFFENSE: {
            const P4OffensiveEffectBinding *row = p4_game_tables_offensive_effect_at(tables, index);
            owner = p4_game_tables_offensive_effect_owner(tables, index);
            entry.ids[0] = row->source_kind; entry.ids[1] = row->source_id;
            entry.ids[2] = row->element_id;
            key = row->key; schema = row->schema; revision = row->revision;
            break;
        }
        case P4_TABLE_SOURCE_SIDE_AILMENT: {
            const P4SideAilmentDef *row = p4_game_tables_side_ailment_at(tables, index);
            owner = p4_game_tables_side_ailment_owner(tables, index);
            entry.ids[0] = row->leaf;
            key = row->key; schema = row->schema; revision = row->revision;
            break;
        }
        default: {
            const P4SideRiderDesc *row = p4_game_tables_side_rider_at(tables, index);
            owner = p4_game_tables_side_rider_owner(tables, index);
            entry.ids[0] = row->skill_id; entry.ids[1] = row->leaf;
            key = row->key; schema = row->schema; revision = row->revision;
            break;
        }
        }
        /* Only the registry's explicit built-in element/channel definitions
         * are unowned. An authored missing owner is never a native fallback. */
        if (!owner) {
            if ((entry.domain == P4_TABLE_SOURCE_ELEMENT && entry.ids[0] < P4_ELEMENT_RETAIL_COUNT) ||
                (entry.domain == P4_TABLE_SOURCE_CHANNEL && entry.ids[0] < 3u)) continue;
            return P4_TABLE_INVALID;
        }
        int result = entry_finish(&entry, owner, key, schema, revision);
        if (result) return result;
        *out = entry;
        return P4_TABLE_OK;
    }
    return cursor->domain == REGISTRY_DOMAINS ? P4_TABLE_NOT_FOUND : P4_TABLE_INVALID;
}
int p4_game_table_sources_next(P4TableSourceViewKind kind, const void *view,
    P4TableSourceCursor *cursor, P4TableSourceEntry *out)
{
    if (!view || !cursor || !out || (uint32_t)kind > P4_TABLE_SOURCE_PERSONAS) return P4_TABLE_INVALID;
    if (!view_semantic_revision(kind, view)) return P4_TABLE_NOT_READY;
    if (kind == P4_TABLE_SOURCE_REGISTRY) return registry_next(view, cursor, out);
    const uint32_t domains = kind == P4_TABLE_SOURCE_ITEMS ? 2u : 1u;
    if (cursor->domain > domains) return P4_TABLE_INVALID;
    while (cursor->domain < domains) {
        const size_t count = kind == P4_TABLE_SOURCE_SKILLS ? p4_game_skills_count(view) :
            kind == P4_TABLE_SOURCE_TEXT ? p4_game_tables_text_count(view) :
            kind == P4_TABLE_SOURCE_ENEMIES ? p4_game_enemies_count(view) :
            kind == P4_TABLE_SOURCE_PERSONAS ? p4_game_personas_count(view) :
            cursor->domain ? p4_game_tables_items_category_count(view) : p4_game_tables_items_count(view);
        if (cursor->index >= count) { ++cursor->domain; cursor->index = 0; continue; }
        const size_t index = cursor->index++;
        P4TableSourceEntry entry = {0};
        const char *owner, *key = "";
        uint32_t schema = 1, revision;
        if (kind == P4_TABLE_SOURCE_SKILLS) {
            const P4GameSkillView *row = p4_game_skills_at(view, index);
            entry.domain = P4_TABLE_SOURCE_SKILL; entry.ids[0] = row->desc.id;
            owner = row->owner; key = row->desc.key; schema = row->desc.schema; revision = row->desc.revision;
        } else if (kind == P4_TABLE_SOURCE_ENEMIES) {
            const P4GameEnemyView *row = p4_game_enemies_at(view, index);
            entry.domain = P4_TABLE_SOURCE_ENEMY; entry.ids[0] = row->desc.id;
            owner = row->owner; key = row->desc.key; schema = row->desc.schema; revision = row->desc.revision;
        } else if (kind == P4_TABLE_SOURCE_PERSONAS) {
            const P4GamePersonaView *row = p4_game_personas_at(view, index);
            entry.domain = P4_TABLE_SOURCE_PERSONA; entry.ids[0] = row->desc.id;
            owner = row->owner; key = row->desc.key; schema = row->desc.schema; revision = row->desc.revision;
        } else if (kind == P4_TABLE_SOURCE_TEXT) {
            P4GameTextDefinition row;
            int result = p4_game_tables_text_definition_at(view, index, &row);
            if (result) return result;
            entry.domain = P4_TABLE_SOURCE_STRING; entry.ids[0] = row.table;
            entry.ids[1] = row.id; entry.ids[2] = row.role;
            owner = row.owner; revision = row.revision;
        } else if (!cursor->domain) {
            const P4GameItemView *row = p4_game_tables_items_at(view, index);
            entry.domain = P4_TABLE_SOURCE_ITEM; entry.ids[0] = row->id;
            owner = row->owner; key = row->key; revision = row->revision;
        } else {
            const P4GameItemCategoryView *row = p4_game_tables_items_category_at(view, index);
            entry.domain = P4_TABLE_SOURCE_CATEGORY; entry.ids[0] = row->id;
            owner = row->owner; key = row->key; revision = row->revision;
        }
        int result = entry_finish(&entry, owner, key, schema, revision);
        if (result) return result;
        *out = entry;
        return P4_TABLE_OK;
    }
    return P4_TABLE_NOT_FOUND;
}
P4GameTableSources *p4_game_table_sources_create(void)
{
    if (p4_packages_memory_reserve(sizeof(P4GameTableSources))) return NULL;
    P4GameTableSources *sources = calloc(1, sizeof *sources);
    if (!sources) { p4_packages_memory_cancel(sizeof *sources); return NULL; }
    p4_packages_memory_commit(sizeof *sources);
    return sources;
}
static void release_rows(void *rows, size_t bytes)
{
    free(rows);
    if (bytes) p4_packages_memory_release(bytes);
}
void p4_game_table_sources_destroy(P4GameTableSources *sources)
{
    if (!sources) return;
    for (size_t i = 0; i < sources->origins_count; ++i)
        if (sources->origins[i].provider) p4_package_provider_release(sources->origins[i].provider);
    release_rows(sources->views, sources->views_capacity * sizeof *sources->views);
    release_rows(sources->origins, sources->origins_capacity * sizeof *sources->origins);
    release_rows(sources->bindings, sources->bindings_capacity * sizeof *sources->bindings);
    release_rows(sources->owners, sources->owners_bytes);
    free(sources); p4_packages_memory_release(sizeof *sources);
}
static size_t view_index(const P4GameTableSources *sources, const void *view)
{
    size_t index = 0;
    while (index < sources->views_count && sources->views[index].view != view) ++index;
    return index;
}
int p4_game_table_sources_view(P4GameTableSources *sources, P4TableSourceViewKind kind, const void *view)
{
    if (!sources || !view || (uint32_t)kind > P4_TABLE_SOURCE_PERSONAS) return P4_TABLE_INVALID;
    if (sources->sealed) return P4_TABLE_FROZEN;
    const size_t existing = view_index(sources, view);
    if (existing < sources->views_count) return sources->views[existing].kind == (uint32_t)kind ?
        P4_TABLE_OK : P4_TABLE_CONFLICT;
    if (sources->views_count == UINT32_MAX) return P4_TABLE_NOMEM;
    int result = grow((void **)&sources->views, &sources->views_capacity, sources->views_count, sizeof *sources->views);
    if (result) return result;
    sources->views[sources->views_count++] = (SourceView){view, kind, 0};
    return P4_TABLE_OK;
}
int p4_game_table_sources_native(P4GameTableSources *sources, const char *mode,
    uint32_t schema, uint32_t revision, uint32_t *source)
{
    if (!sources || !source || !schema || !revision || !bounded_string(mode, P4_TABLE_KEY_SIZE, 0)) return P4_TABLE_INVALID;
    if (sources->sealed) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < sources->origins_count; ++i) {
        const P4TableSourceOrigin *origin = &sources->origins[i].desc;
        if (origin->kind != P4_TABLE_SOURCE_NATIVE || strcmp(origin->source_key, mode)) continue;
        if (origin->schema != schema || origin->revision != revision) return P4_TABLE_CONFLICT;
        *source = (uint32_t)i; return P4_TABLE_OK;
    }
    if (sources->origins_count == UINT32_MAX) return P4_TABLE_NOMEM;
    int result = grow((void **)&sources->origins, &sources->origins_capacity, sources->origins_count, sizeof *sources->origins);
    if (result) return result;
    SourceOrigin origin = {.desc = {.kind = P4_TABLE_SOURCE_NATIVE, .schema = schema, .revision = revision}};
    strcpy(origin.desc.source_key, mode);
    *source = (uint32_t)sources->origins_count;
    sources->origins[sources->origins_count++] = origin;
    return P4_TABLE_OK;
}
int p4_game_table_sources_package(P4GameTableSources *sources, const P4PackageProvider *provider,
    const char *source_key, uint32_t schema, uint32_t revision, uint32_t *source)
{
    if (!sources || !provider || !source || !schema || !revision ||
        !bounded_string(source_key, P4_TABLE_KEY_SIZE, 0)) return P4_TABLE_INVALID;
    if (sources->sealed) return P4_TABLE_FROZEN;
    if (!p4_package_provider_selected(provider)) return P4_TABLE_NOT_READY;
    const P4PackageInfo *info = p4_package_provider_info(provider);
    if (!info || (info->kind != P4_PACKAGE_MOD && info->kind != P4_PACKAGE_ENGINE_PACK) ||
        !bounded_string(info->id, sizeof info->id, 0) || !bounded_string(info->version, sizeof info->version, 0))
        return P4_TABLE_INVALID;
    for (size_t i = 0; i < sources->origins_count; ++i) {
        const SourceOrigin *origin = sources->origins + i;
        if (!origin->provider || origin->desc.package_kind != (uint32_t)info->kind ||
            strcmp(origin->desc.package_id, info->id) || strcmp(origin->desc.source_key, source_key)) continue;
        if (origin->desc.schema != schema || origin->desc.revision != revision ||
            strcmp(origin->desc.package_version, info->version) || !p4_package_provider_same_source(origin->provider, provider))
            return P4_TABLE_CONFLICT;
        *source = (uint32_t)i; return P4_TABLE_OK;
    }
    if (sources->origins_count == UINT32_MAX) return P4_TABLE_NOMEM;
    int result = grow((void **)&sources->origins, &sources->origins_capacity, sources->origins_count, sizeof *sources->origins);
    if (result) return result;
    if (p4_package_provider_retain(provider)) return P4_TABLE_NOT_READY;
    SourceOrigin origin = {.desc = {.kind = P4_TABLE_SOURCE_PACKAGE, .schema = schema,
        .revision = revision, .package_kind = info->kind}, .provider = provider};
    strcpy(origin.desc.source_key, source_key);
    strcpy(origin.desc.package_id, info->id); strcpy(origin.desc.package_version, info->version);
    *source = (uint32_t)sources->origins_count;
    sources->origins[sources->origins_count++] = origin;
    return P4_TABLE_OK;
}
int p4_game_table_sources_bind(P4GameTableSources *sources, const void *view,
    const P4TableSourceEntry *entry, uint32_t source)
{
    if (!sources || !view || !entry || source >= sources->origins_count ||
        entry->domain < P4_TABLE_SOURCE_ELEMENT || entry->domain > P4_TABLE_SOURCE_OFFENSE ||
        !entry->schema || !entry->revision || !bounded_string(entry->owner, sizeof entry->owner, 0) ||
        !bounded_string(entry->key, sizeof entry->key, 1)) return P4_TABLE_INVALID;
    if (sources->sealed) return P4_TABLE_FROZEN;
    const size_t index = view_index(sources, view);
    if (index == sources->views_count) return P4_TABLE_NOT_FOUND;
    int result = grow((void **)&sources->bindings, &sources->bindings_capacity, sources->bindings_count, sizeof *sources->bindings);
    if (result) return result;
    sources->bindings[sources->bindings_count++] = (SourceBinding){*entry, (uint32_t)index, source};
    return P4_TABLE_OK;
}
static int word_compare(uint32_t left, uint32_t right) { return left == right ? 0 : left < right ? -1 : 1; }
static int binding_compare(const void *left, const void *right)
{
    const SourceBinding *a = left, *b = right;
    int result = word_compare(a->view, b->view);
    if (!result) result = word_compare(a->entry.domain, b->entry.domain);
    for (size_t i = 0; !result && i < 5; ++i) result = word_compare(a->entry.ids[i], b->entry.ids[i]);
    return result;
}
static int owner_compare(const void *left, const void *right)
{
    const OwnerSource *a = left, *b = right;
    int result = word_compare(a->view, b->view);
    if (!result) result = strcmp(a->owner, b->owner);
    return result ? result : word_compare(a->source, b->source);
}
static size_t binding_view_first(const P4GameTableSources *sources, uint32_t view)
{
    size_t low = 0, high = sources->bindings_count;
    while (low < high) {
        const size_t middle = low + (high - low) / 2;
        if (sources->bindings[middle].view < view) low = middle + 1; else high = middle;
    }
    return low;
}
static int validate_view(const P4GameTableSources *sources, uint32_t view)
{
    const SourceView *current = sources->views + view;
    if (!view_semantic_revision(current->kind, current->view)) return P4_TABLE_NOT_READY;
    P4TableSourceCursor cursor = {0};
    P4TableSourceEntry entry;
    size_t expected = 0;
    int result;
    while ((result = p4_game_table_sources_next(current->kind, current->view, &cursor, &entry)) == P4_TABLE_OK) {
        SourceBinding key = {.entry = entry, .view = view};
        const SourceBinding *binding = sources->bindings_count ? bsearch(&key, sources->bindings,
            sources->bindings_count, sizeof *sources->bindings, binding_compare) : NULL;
        if (!binding) return P4_TABLE_NOT_READY;
        if (strcmp(binding->entry.owner, entry.owner) || strcmp(binding->entry.key, entry.key) ||
            binding->entry.schema != entry.schema || binding->entry.revision != entry.revision) return P4_TABLE_CONFLICT;
        ++expected;
    }
    if (result != P4_TABLE_NOT_FOUND) return result;
    const size_t first = binding_view_first(sources, view);
    const size_t last = view + 1u == sources->views_count ? sources->bindings_count : binding_view_first(sources, view + 1u);
    return expected == last - first ? P4_TABLE_OK : P4_TABLE_CONFLICT;
}
int p4_game_table_sources_seal(P4GameTableSources *sources)
{
    if (!sources) return P4_TABLE_INVALID;
    if (sources->sealed) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < sources->origins_count; ++i)
        if (sources->origins[i].provider) {
            int result = current_package(sources->origins + i);
            if (result) return result;
        }
    if (sources->bindings_count) qsort(sources->bindings, sources->bindings_count, sizeof *sources->bindings, binding_compare);
    for (size_t i = 1; i < sources->bindings_count; ++i)
        if (!binding_compare(sources->bindings + i - 1, sources->bindings + i)) return P4_TABLE_CONFLICT;
    for (size_t view = 0; view < sources->views_count; ++view) {
        SourceView *current = sources->views + view;
        const uint32_t revision = view_semantic_revision(current->kind, current->view);
        if (!revision) return P4_TABLE_NOT_READY; /* Every semantic view must seal before exposure. */
        int result = validate_view(sources, (uint32_t)view);
        if (result) return result;
        current->revision = revision;
    }
    const size_t expected = sources->bindings_count;
    if (expected > SIZE_MAX / sizeof(OwnerSource)) return P4_TABLE_NOMEM;
    const size_t bytes = expected * sizeof(OwnerSource);
    OwnerSource *owners = NULL;
    if (bytes) {
        if (p4_packages_memory_reserve(bytes)) return P4_TABLE_NOMEM;
        owners = malloc(bytes);
        if (!owners) { p4_packages_memory_cancel(bytes); return P4_TABLE_NOMEM; }
        p4_packages_memory_commit(bytes);
    }
    for (size_t i = 0; i < expected; ++i) owners[i] = (OwnerSource){sources->bindings[i].entry.owner,
        sources->bindings[i].view, sources->bindings[i].source};
    if (expected) qsort(owners, expected, sizeof *owners, owner_compare);
    size_t unique = 0;
    for (size_t i = 0; i < expected; ++i)
        if (!unique || owner_compare(owners + i, owners + unique - 1)) {
            if (unique != i) owners[unique] = owners[i];
            ++unique;
        }
    sources->owners = owners; sources->owners_count = unique; sources->owners_bytes = bytes;
    sources->sealed = 1;
    return P4_TABLE_OK;
}
int p4_game_table_sources_validate_view(const P4GameTableSources *sources, const void *view)
{
    if (!sources || !view) return P4_TABLE_INVALID;
    if (!sources->sealed) return P4_TABLE_NOT_READY;
    const size_t index = view_index(sources, view);
    if (index == sources->views_count) return P4_TABLE_NOT_FOUND;
    return validate_view(sources, (uint32_t)index);
}
int p4_game_table_sources_freeze(P4GameTableSources *sources)
{
    if (!sources) return P4_TABLE_INVALID;
    if (sources->frozen) return P4_TABLE_FROZEN;
    if (!sources->sealed) {
        int result = p4_game_table_sources_seal(sources);
        if (result) return result;
    } else {
        for (size_t i = 0; i < sources->origins_count; ++i)
            if (sources->origins[i].provider) {
                int result = current_package(sources->origins + i);
                if (result) return result;
            }
    }
    for (size_t i = 0; i < sources->views_count; ++i) {
        SourceView *view = sources->views + i;
        const uint32_t revision = view_revision(view->kind, view->view);
        if (!revision) return P4_TABLE_NOT_READY;
        if (revision != view->revision || view_semantic_revision(view->kind, view->view) != view->revision)
            return P4_TABLE_CONFLICT;
    }
    sources->frozen = 1;
    return P4_TABLE_OK;
}
int p4_game_table_sources_is_sealed(const P4GameTableSources *sources) { return sources && sources->sealed; }
int p4_game_table_sources_is_frozen(const P4GameTableSources *sources) { return sources && sources->frozen; }
static size_t owner_first(const P4GameTableSources *sources, uint32_t view, const char *owner)
{
    size_t low = 0, high = sources->owners_count;
    while (low < high) {
        const size_t middle = low + (high - low) / 2;
        const OwnerSource *row = sources->owners + middle;
        int result = word_compare(row->view, view);
        if (!result) result = strcmp(row->owner, owner);
        if (result < 0) low = middle + 1; else high = middle;
    }
    return low;
}
static int current_package(const SourceOrigin *origin)
{
    if (!p4_package_provider_selected(origin->provider)) return P4_TABLE_NOT_READY;
    const P4PackageInfo *info = p4_package_provider_info(origin->provider);
    return info && (uint32_t)info->kind == origin->desc.package_kind &&
        !strcmp(info->id, origin->desc.package_id) && !strcmp(info->version, origin->desc.package_version) ?
        P4_TABLE_OK : P4_TABLE_CONFLICT;
}
int p4_game_table_sources_require_owner(const void *view, P4ModSaveWriter *writer,
    const char *owner, void *immutable_sources)
{
    const P4GameTableSources *sources = immutable_sources;
    if (!sources || !view || !owner || !*owner) return P4_TABLE_INVALID;
    if (!sources->frozen) return P4_TABLE_NOT_READY;
    const size_t index = view_index(sources, view);
    if (index == sources->views_count) return P4_TABLE_NOT_FOUND;
    if (view_revision(sources->views[index].kind, view) != sources->views[index].revision) return P4_TABLE_NOT_READY;
    size_t position = owner_first(sources, (uint32_t)index, owner);
    if (position == sources->owners_count || sources->owners[position].view != index || strcmp(sources->owners[position].owner, owner))
        return P4_TABLE_NOT_FOUND;
    do {
        const SourceOrigin *origin = sources->origins + sources->owners[position].source;
        int result = P4_TABLE_OK;
        if (origin->desc.kind == P4_TABLE_SOURCE_PACKAGE)
            result = writer ? p4_mod_save_require_provider(writer, origin->provider, 1) : current_package(origin);
        else if (origin->desc.kind != P4_TABLE_SOURCE_NATIVE) result = P4_TABLE_INVALID;
        if (result) return result < 0 ? result : P4_TABLE_INVALID;
        ++position;
    } while (position < sources->owners_count && sources->owners[position].view == index && !strcmp(sources->owners[position].owner, owner));
    return P4_TABLE_OK;
}
const P4TableSourceOrigin *p4_game_table_sources_origin_at(const P4GameTableSources *sources,
    const void *view, const char *owner, size_t index)
{
    if (!sources || !sources->sealed || !view || !owner || !*owner) return NULL;
    const size_t id = view_index(sources, view);
    if (id == sources->views_count || !sources->views[id].revision ||
        view_semantic_revision(sources->views[id].kind, view) != sources->views[id].revision) return NULL;
    const size_t first = owner_first(sources, (uint32_t)id, owner);
    if (index > SIZE_MAX - first || first + index >= sources->owners_count) return NULL;
    const OwnerSource *row = sources->owners + first + index;
    return row->view == id && !strcmp(row->owner, owner) ? &sources->origins[row->source].desc : NULL;
}
size_t p4_game_table_sources_native_mode_count(const P4GameTableSources *sources)
{
    if (!sources || !sources->sealed) return 0;
    size_t count = 0;
    for (size_t i = 0; i < sources->origins_count; ++i)
        if (sources->origins[i].desc.kind == P4_TABLE_SOURCE_NATIVE) ++count;
    return count;
}
const P4TableSourceOrigin *p4_game_table_sources_native_mode_next(const P4GameTableSources *sources, size_t *cursor)
{
    if (!sources || !sources->sealed || !cursor) return NULL;
    while (*cursor < sources->origins_count) {
        const P4TableSourceOrigin *origin = &sources->origins[(*cursor)++].desc;
        if (origin->kind == P4_TABLE_SOURCE_NATIVE) return origin;
    }
    return NULL;
}
int p4_game_table_sources_resolve_entry(const P4GameTableSources *sources, const void *view,
    uint32_t domain, const uint32_t ids[5], P4TableSourceResolved *out)
{
    if (!sources || !view || !ids || !out || domain < P4_TABLE_SOURCE_ELEMENT || domain > P4_TABLE_SOURCE_OFFENSE)
        return P4_TABLE_INVALID;
    if (!sources->frozen) return P4_TABLE_NOT_READY;
    size_t index = view_index(sources, view);
    if (index == sources->views_count) return P4_TABLE_NOT_FOUND;
    const SourceView *snapshot = &sources->views[index];
    if (view_revision(snapshot->kind, view) != snapshot->revision ||
        view_semantic_revision(snapshot->kind, view) != snapshot->revision) return P4_TABLE_NOT_READY;
    SourceBinding key = {.view = (uint32_t)index, .entry = {.domain = domain,
        .ids = {ids[0], ids[1], ids[2], ids[3], ids[4]}}};
    const SourceBinding *binding = sources->bindings_count ?
        bsearch(&key, sources->bindings, sources->bindings_count, sizeof *sources->bindings, binding_compare) : NULL;
    if (!binding) return P4_TABLE_NOT_FOUND;
    const SourceOrigin *origin = &sources->origins[binding->source];
    if ((origin->desc.kind == P4_TABLE_SOURCE_NATIVE && origin->provider) ||
        (origin->desc.kind == P4_TABLE_SOURCE_PACKAGE && !origin->provider) ||
        (origin->desc.kind != P4_TABLE_SOURCE_NATIVE && origin->desc.kind != P4_TABLE_SOURCE_PACKAGE)) return P4_TABLE_INVALID;
    *out = (P4TableSourceResolved){.origin = &origin->desc, .provider = origin->provider,
        .entry = &binding->entry, .origin_index = binding->source};
    return P4_TABLE_OK;
}
size_t p4_game_table_sources_origin_count(const P4GameTableSources *sources)
{ return sources && sources->frozen ? sources->origins_count : 0; }
static int source_prepare_complete(void *user)
{
    P4GameTableSources *sources = user;
    if (!sources->frozen) return p4_game_table_sources_freeze(sources);
    for (size_t i = 0; i < sources->views_count; ++i)
        if (view_revision(sources->views[i].kind, sources->views[i].view) != sources->views[i].revision)
            return P4_TABLE_NOT_READY;
    for (size_t i = 0; i < sources->origins_count; ++i)
        if (sources->origins[i].provider) {
            int result = current_package(sources->origins + i);
            if (result) return result;
        }
    return P4_TABLE_OK;
}
static void source_commit_complete(void *user)
{
    active_sources = user;
}
int p4_game_table_sources_boot_stage(P4GameTableSources *sources)
{
    if (!sources) return P4_TABLE_INVALID;
    if (active_sources) return P4_TABLE_FROZEN;
    if (staged_sources) return staged_sources == sources ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    int result = p4_game_tables_battle_complete_register("table-source",
        source_prepare_complete, source_commit_complete, sources);
    if (result) return result;
    staged_sources = sources;
    return P4_TABLE_OK;
}
const P4GameTableSources *p4_game_table_sources_active(void) { return active_sources; }
int p4_game_table_sources_session_end(void)
{
    staged_sources = NULL; active_sources = NULL;
    return P4_TABLE_OK;
}
