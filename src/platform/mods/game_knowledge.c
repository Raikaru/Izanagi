#include "game_knowledge_runtime_internal.h"
#include <stdlib.h>
#include <string.h>

typedef struct KnowledgeEntry {
    P4KnowledgeKey key;
    size_t alias_index;
    unsigned char known, defaulted;
} KnowledgeEntry;
struct P4GameKnowledge {
    const P4GameTables *tables;
    P4KnowledgeRetail retail;
    size_t count;
    KnowledgeEntry entries[];
};
static int compare_key(const P4KnowledgeKey *a, const P4KnowledgeKey *b)
{
#define FIELD(f) if (a->f != b->f) return a->f < b->f ? -1 : 1
    FIELD(knowledge_id); FIELD(profile_id); FIELD(element_id); FIELD(channel_id);
#undef FIELD
    return 0;
}
/* Equal keys keep the lowest alias index as their canonical identity. */
static int compare_entry(const void *left, const void *right)
{
    const KnowledgeEntry *a = left, *b = right;
    int result = compare_key(&a->key, &b->key);
    if (result) return result;
    return a->alias_index < b->alias_index ? -1 : a->alias_index != b->alias_index;
}
/* Frozen affinity rows are sorted by kind/entity/profile/element/channel. */
static size_t profile_begin(const P4GameTables *tables, const P4AffinityProfileAlias *alias)
{
    size_t low = 0, high = p4_game_tables_affinity_count(tables);
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        const P4AffinityRow *row = p4_game_tables_affinity_at(tables, middle);
        int before = row->entity_kind < alias->entity_kind ||
            (row->entity_kind == alias->entity_kind && (row->entity_id < alias->entity_id ||
             (row->entity_id == alias->entity_id && row->profile_id < alias->profile_id)));
        if (before) low = middle + 1; else high = middle;
    }
    return low;
}
static int same_profile(const P4AffinityRow *row, const P4AffinityProfileAlias *alias)
{
    return row && row->entity_kind == alias->entity_kind &&
        row->entity_id == alias->entity_id && row->profile_id == alias->profile_id;
}
/* A status-channel side ailment that declares default-from: its pair exists
 * without a row (S1's side_default answers it). */
static const P4SideAilmentDef *defaulted_side(const P4GameTables *tables, uint32_t leaf, uint32_t channel)
{
    const P4SideAilmentDef *side = channel == P4_CHANNEL_STATUS ?
        p4_game_tables_find_side_ailment(tables, leaf) : NULL;
    return side && side->default_from != P4_SIDE_DEFAULT_NONE ? side : NULL;
}
/* The alias's defaulted pair for side ailment index, unless it has a row. */
static int alias_default(const P4GameTables *tables, const P4AffinityProfileAlias *alias,
    size_t index, uint32_t *leaf)
{
    const P4SideAilmentDef *side = p4_game_tables_side_ailment_at(tables, index);
    if (side->default_from == P4_SIDE_DEFAULT_NONE || p4_game_tables_find_affinity(tables, P4_ENTITY_ENEMY,
            alias->entity_id, alias->profile_id, side->leaf, P4_CHANNEL_STATUS)) return 0;
    *leaf = side->leaf;
    return 1;
}
int p4_game_knowledge_create(const P4GameTables *tables, P4GameKnowledge **out)
{
    if (!out || !p4_game_tables_revision(tables)) return P4_TABLE_INVALID;
    const size_t maximum = (SIZE_MAX - sizeof(P4GameKnowledge)) / sizeof(KnowledgeEntry);
    size_t capacity = 0, aliases = p4_game_tables_affinity_alias_count(tables);
    const size_t sides = p4_game_tables_side_ailment_count(tables);
    for (size_t a = 0; a < aliases; ++a) {
        const P4AffinityProfileAlias *alias = p4_game_tables_affinity_alias_at(tables, a);
        if (alias->entity_kind != P4_ENTITY_ENEMY) continue;
        for (size_t r = profile_begin(tables, alias);
             same_profile(p4_game_tables_affinity_at(tables, r), alias); ++r) {
            if (capacity == maximum) return P4_TABLE_NOMEM;
            ++capacity;
        }
        for (size_t s = 0; s < sides; ++s) {
            uint32_t leaf;
            if (!alias_default(tables, alias, s, &leaf)) continue;
            if (capacity == maximum) return P4_TABLE_NOMEM;
            ++capacity;
        }
    }
    P4GameKnowledge *state = calloc(1, sizeof(P4GameKnowledge) + capacity * sizeof(KnowledgeEntry));
    if (!state) return P4_TABLE_NOMEM;
    state->tables = tables;
    size_t n = 0;
    for (size_t a = 0; a < aliases; ++a) {
        const P4AffinityProfileAlias *alias = p4_game_tables_affinity_alias_at(tables, a);
        if (alias->entity_kind != P4_ENTITY_ENEMY) continue;
        for (size_t r = profile_begin(tables, alias);
             same_profile(p4_game_tables_affinity_at(tables, r), alias); ++r) {
            const P4AffinityRow *row = p4_game_tables_affinity_at(tables, r);
            state->entries[n++] = (KnowledgeEntry){
                {alias->knowledge_id, alias->profile_id, row->element_id, row->channel_id}, a, 0, 0};
        }
        for (size_t s = 0; s < sides; ++s) {
            uint32_t leaf;
            if (alias_default(tables, alias, s, &leaf))
                state->entries[n++] = (KnowledgeEntry){
                    {alias->knowledge_id, alias->profile_id, leaf, P4_CHANNEL_STATUS}, a, 0, 1};
        }
    }
    if (capacity > 1) qsort(state->entries, capacity, sizeof *state->entries, compare_entry);
    for (size_t i = 0; i < capacity; ++i) {
        if (state->count && !compare_key(&state->entries[state->count - 1].key, &state->entries[i].key)) continue;
        state->entries[state->count++] = state->entries[i];
    }
    *out = state;
    return P4_TABLE_OK;
}
void p4_game_knowledge_destroy(P4GameKnowledge *state) { free(state); }
void p4_game_knowledge_clear(P4GameKnowledge *state)
{
    if (!state) return;
    for (size_t i = 0; i < state->count; ++i) state->entries[i].known = 0;
    memset(&state->retail, 0, sizeof state->retail);
}
size_t p4_game_knowledge_count(const P4GameKnowledge *state) { return state ? state->count : 0; }
const P4GameTables *p4_game_knowledge_snapshot(const P4GameKnowledge *state) { return state ? state->tables : NULL; }
static int status_bit(uint32_t enemy_id, uint32_t slot, uint8_t *mask)
{
    if (enemy_id >= P4_KNOWLEDGE_RETAIL_ENEMIES || slot < P4_KNOWLEDGE_STATUS_FIRST ||
        slot - P4_KNOWLEDGE_STATUS_FIRST >= P4_KNOWLEDGE_STATUS_SLOTS) return P4_TABLE_INVALID;
    *mask = (uint8_t)(1u << (slot - P4_KNOWLEDGE_STATUS_FIRST));
    return P4_TABLE_OK;
}
int p4_game_knowledge_status_test(const P4GameKnowledge *state, uint32_t enemy_id, uint32_t slot, int *known)
{
    uint8_t mask;
    if (!state || !known || status_bit(enemy_id, slot, &mask)) return P4_TABLE_INVALID;
    *known = (state->retail.status[enemy_id] & mask) != 0;
    return P4_TABLE_OK;
}
int p4_game_knowledge_status_observe(P4GameKnowledge *state, uint32_t enemy_id, uint32_t slot)
{
    uint8_t mask;
    if (!state || status_bit(enemy_id, slot, &mask)) return P4_TABLE_INVALID;
    state->retail.status[enemy_id] |= mask;
    return P4_TABLE_OK;
}
/* Side-ailment indices that declare default-from; at most 16 rows exist. */
static uint32_t defaulted_mask(const P4GameTables *tables)
{
    uint32_t mask = 0;
    for (size_t i = 0; i < p4_game_tables_side_ailment_count(tables) && i < P4_SIDE_AILMENT_LIMIT; ++i)
        if (p4_game_tables_side_ailment_at(tables, i)->default_from != P4_SIDE_DEFAULT_NONE) mask |= 1u << i;
    return mask;
}
static int side_bit(const P4GameKnowledge *state, uint32_t enemy_id, uint32_t leaf, uint16_t *mask)
{
    if (!state || enemy_id >= P4_KNOWLEDGE_RETAIL_ENEMIES) return P4_TABLE_INVALID;
    for (size_t i = 0; i < p4_game_tables_side_ailment_count(state->tables) && i < P4_SIDE_AILMENT_LIMIT; ++i) {
        const P4SideAilmentDef *side = p4_game_tables_side_ailment_at(state->tables, i);
        if (side->leaf != leaf) continue;
        if (side->default_from == P4_SIDE_DEFAULT_NONE) break;
        *mask = (uint16_t)(1u << i);
        return P4_TABLE_OK;
    }
    return P4_TABLE_NOT_FOUND;
}
int p4_game_knowledge_side_test(const P4GameKnowledge *state, uint32_t enemy_id, uint32_t leaf, int *known)
{
    uint16_t mask;
    if (!known) return P4_TABLE_INVALID;
    int result = side_bit(state, enemy_id, leaf, &mask);
    if (!result) *known = (state->retail.side[enemy_id] & mask) != 0;
    return result;
}
int p4_game_knowledge_side_observe(P4GameKnowledge *state, uint32_t enemy_id, uint32_t leaf)
{
    uint16_t mask;
    int result = side_bit(state, enemy_id, leaf, &mask);
    if (!result) state->retail.side[enemy_id] |= mask;
    return result;
}
const P4KnowledgeRetail *p4_game_knowledge_retail(const P4GameKnowledge *state)
{
    return state ? &state->retail : NULL;
}
static size_t lower_bound(const P4GameKnowledge *state, const P4KnowledgeKey *key)
{
    size_t low = 0, high = p4_game_knowledge_count(state);
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (compare_key(&state->entries[middle].key, key) < 0) low = middle + 1; else high = middle;
    }
    return low;
}
static int find_index(const P4GameKnowledge *state, const P4KnowledgeKey *key, size_t *index)
{
    if (!key) return P4_TABLE_INVALID;
    size_t low = lower_bound(state, key);
    if (!state || low == state->count || compare_key(&state->entries[low].key, key)) return P4_TABLE_NOT_FOUND;
    *index = low;
    return P4_TABLE_OK;
}
int p4_game_knowledge_test(const P4GameKnowledge *state, const P4KnowledgeKey *key, int *known)
{
    if (!known) return P4_TABLE_INVALID;
    size_t index;
    int result = find_index(state, key, &index);
    if (!result) *known = state->entries[index].known != 0;
    return result;
}
int p4_game_knowledge_observe(P4GameKnowledge *state, const P4KnowledgeKey *key)
{
    size_t index;
    int result = find_index(state, key, &index);
    if (!result) state->entries[index].known = 1;
    return result;
}
int p4_game_knowledge_any(const P4GameKnowledge *state, uint32_t knowledge_id, uint32_t profile_id, int *known)
{
    if (!known) return P4_TABLE_INVALID;
    const P4KnowledgeKey first = {knowledge_id, profile_id, 0, 0};
    int value = 0;
    for (size_t i = lower_bound(state, &first); !value && i < p4_game_knowledge_count(state) &&
         state->entries[i].key.knowledge_id == knowledge_id && state->entries[i].key.profile_id == profile_id; ++i)
        value = state->entries[i].known != 0;
    *known = value;
    return P4_TABLE_OK;
}
static int resolve(const P4GameKnowledge *state, uint32_t enemy,
    uint32_t native_profile, const P4GameActionElement *element, P4KnowledgeKey *out, int runtime)
{
    if (!element || element->valid != 1u || !out) return P4_TABLE_INVALID;
    if (!state) return P4_TABLE_NOT_FOUND;
    const P4AffinityProfileAlias *alias = p4_game_tables_find_affinity_alias(state->tables,
        P4_ENTITY_ENEMY, enemy, native_profile);
    if (!alias) return P4_TABLE_NOT_FOUND;
    if (runtime) {
        int result = p4_game_tables_runtime_use_row(state->tables, P4_TABLE_SOURCE_ALIAS, alias);
        if (result) return result;
    }
    /* Sharing knowledge does not qualify another alias's missing side pair. A
     * defaulted side ailment's status pair exists without a row. */
    const P4AffinityRow *row = p4_game_tables_find_affinity(state->tables, P4_ENTITY_ENEMY, enemy,
        alias->profile_id, element->element_id, element->channel_id);
    const P4SideAilmentDef *side = row ? NULL : defaulted_side(state->tables, element->element_id, element->channel_id);
    if (!row && !side) return P4_TABLE_NOT_FOUND;
    if (runtime) {
        int result = row ? p4_game_tables_runtime_use_row(state->tables, P4_TABLE_SOURCE_AFFINITY, row) :
            p4_game_tables_runtime_use_row(state->tables, P4_TABLE_SOURCE_SIDE_AILMENT, side);
        if (result) return result;
    }
    const P4KnowledgeKey key = {alias->knowledge_id, alias->profile_id, element->element_id, element->channel_id};
    size_t index;
    int result = find_index(state, &key, &index);
    if (!result) *out = key;
    return result;
}
int p4_game_knowledge_resolve(const P4GameKnowledge *state, uint32_t enemy,
    uint32_t native_profile, const P4GameActionElement *element, P4KnowledgeKey *out)
{
    return resolve(state, enemy, native_profile, element, out, 0);
}
int p4_game_knowledge_runtime_resolve(const P4GameKnowledge *state, uint32_t enemy,
    uint32_t native_profile, const P4GameActionElement *element, P4KnowledgeKey *out)
{
    return resolve(state, enemy, native_profile, element, out, 1);
}
int p4_game_knowledge_identity_at(const P4GameKnowledge *state, size_t index,
    P4KnowledgeIdentity *identity, int *known)
{
    if (!identity || !known) return P4_TABLE_INVALID;
    if (!state || index >= state->count) return P4_TABLE_NOT_FOUND;
    const KnowledgeEntry *entry = &state->entries[index];
    const P4AffinityProfileAlias *alias = p4_game_tables_affinity_alias_at(state->tables, entry->alias_index);
    const P4ElementDesc *element = p4_game_tables_find_element(state->tables, entry->key.element_id);
    const P4AffinityChannelDesc *channel = p4_game_tables_find_channel(state->tables, entry->key.channel_id);
    *identity = (P4KnowledgeIdentity){entry->key,
        p4_game_tables_affinity_alias_owner(state->tables, entry->alias_index), alias->key,
        element->key, channel->key, alias->schema, alias->revision, element->revision, channel->revision,
        p4_game_tables_element_owner(state->tables, entry->key.element_id),
        p4_game_tables_channel_owner(state->tables, entry->key.channel_id), entry->defaulted};
    *known = entry->known != 0;
    return P4_TABLE_OK;
}
const P4AffinityProfileAlias *p4_game_knowledge_identity_alias_at(
    const P4GameKnowledge *state, size_t index)
{
    if (!state || index >= state->count) return NULL;
    return p4_game_tables_affinity_alias_at(state->tables, state->entries[index].alias_index);
}
int p4_game_knowledge_identity_find(const P4GameKnowledge *state,
    const P4KnowledgeIdentity *identity, size_t *index)
{
    if (!identity || !index || !identity->owner || !identity->alias_key ||
        !identity->element_key || !identity->channel_key) return P4_TABLE_INVALID;
    size_t found;
    int result = find_index(state, &identity->key, &found);
    if (result) return result;
    P4KnowledgeIdentity current;
    int known;
    result = p4_game_knowledge_identity_at(state, found, &current, &known);
    if (result) return result;
    if (strcmp(identity->owner, current.owner) || strcmp(identity->alias_key, current.alias_key) ||
        strcmp(identity->element_key, current.element_key) || strcmp(identity->channel_key, current.channel_key) ||
        identity->alias_schema != current.alias_schema || identity->alias_revision != current.alias_revision ||
        identity->element_revision != current.element_revision || identity->channel_revision != current.channel_revision)
        return P4_TABLE_CONFLICT;
    *index = found;
    return P4_TABLE_OK;
}
int p4_game_knowledge_replace(P4GameKnowledge *state, const size_t *indices, size_t count,
    const P4KnowledgeRetail *retail)
{
    if (count && !indices) return P4_TABLE_INVALID;
    size_t total = p4_game_knowledge_count(state);
    for (size_t i = 0; i < count; ++i)
        if (indices[i] >= total || (i && indices[i - 1] >= indices[i])) return P4_TABLE_INVALID;
    if (!state) return retail ? P4_TABLE_INVALID : P4_TABLE_OK;
    if (retail) {
        const uint32_t defaulted = defaulted_mask(state->tables);
        for (uint32_t e = 0; e < P4_KNOWLEDGE_RETAIL_ENEMIES; ++e)
            if (retail->side[e] & ~defaulted) return P4_TABLE_INVALID;
    }
    p4_game_knowledge_clear(state);
    for (size_t i = 0; i < count; ++i) state->entries[indices[i]].known = 1;
    if (retail) state->retail = *retail;
    return P4_TABLE_OK;
}
