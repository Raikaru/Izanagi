#include "game_tables_runtime_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OWNER_SIZE 64

typedef struct Element { P4ElementDesc desc; char owner[OWNER_SIZE]; } Element;
typedef struct Channel { P4AffinityChannelDesc desc; char owner[OWNER_SIZE]; } Channel;
/* retail_pending: the value is the entity's own retail entry (row, column),
 * read from the sealed UNIT bank before freeze; never an authored value. */
typedef struct Affinity {
    P4AffinityRow row; char owner[OWNER_SIZE];
    uint32_t retail_row, retail_element, retail_pending;
} Affinity;
typedef struct ActionElement { P4ActionElementBinding binding; char owner[OWNER_SIZE]; } ActionElement;
typedef struct AffinityAlias { P4AffinityProfileAlias alias; char owner[OWNER_SIZE]; } AffinityAlias;
typedef struct AffinityEffect { P4AffinityEffectBinding effect; char owner[OWNER_SIZE]; } AffinityEffect;
typedef struct DamageCover { P4DamageCoverBinding cover; char owner[OWNER_SIZE]; } DamageCover;
typedef struct SideAilment { P4SideAilmentDef def; char owner[OWNER_SIZE]; } SideAilment;
typedef struct EffectPair { uint32_t element, channel; } EffectPair;
typedef struct DisplayOrder { uint32_t order, id; size_t index; } DisplayOrder;
struct P4GameTables {
    Element *elements;
    Channel *channels;
    Affinity *affinities;
    ActionElement *actions;
    AffinityAlias *aliases;
    AffinityEffect *effects;
    DamageCover *covers;
    SideAilment *sides;
    size_t elements_count, channels_count, affinities_count;
    DisplayOrder *display_order;
    size_t elements_capacity, channels_capacity, affinities_capacity;
    size_t actions_count, aliases_count, actions_capacity, aliases_capacity;
    size_t effects_count, effects_capacity;
    size_t covers_count, covers_capacity;
    size_t sides_count, sides_capacity;
    EffectPair *effect_pairs;
    size_t effect_pairs_count;
    uint32_t revision;
    int frozen;
    P4GameTableRegistryUseFn runtime_use;
    void *source_context;
};
int p4_game_elements_requested(const P4GameTables *tables)
{
    if (!tables) return 0;
    if (tables->elements_count != P4_ELEMENT_RETAIL_COUNT || tables->channels_count != 3u ||
        tables->affinities_count || tables->actions_count || tables->aliases_count || tables->effects_count ||
        tables->covers_count || tables->sides_count) return 1;
    for (size_t i = 0; i < tables->elements_count; ++i) if (tables->elements[i].owner[0]) return 1;
    return 0;
}
int p4_game_tables_bind_runtime_use(P4GameTables *t, P4GameTableRegistryUseFn use, void *context)
{
    if (!t) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    if (!use || !context) return P4_TABLE_INVALID;
    if (t->runtime_use) return t->runtime_use == use && t->source_context == context ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    t->runtime_use = use; t->source_context = context;
    return P4_TABLE_OK;
}
int p4_game_tables_validate_runtime_sources(const P4GameTables *t)
{
    if (!t) return P4_TABLE_INVALID;
    if (!t->frozen) return P4_TABLE_NOT_READY;
    return p4_game_elements_requested(t) && (!t->runtime_use || !t->source_context) ? P4_TABLE_NOT_READY : P4_TABLE_OK;
}
static int issued_row(const void *row, const void *base, size_t count, size_t stride)
{
    if (!row || !base || !count) return 0;
    const uintptr_t first = (uintptr_t)base, at = (uintptr_t)row;
    return at >= first && (at - first) % stride == 0 && (at - first) / stride < count;
}
int p4_game_tables_runtime_use_row(const P4GameTables *t, uint32_t domain, const void *row)
{
    if (!t || !row) return P4_TABLE_INVALID;
    if (!t->frozen) return P4_TABLE_NOT_READY;
    uint32_t ids[5] = {0};
    const char *owner;
    switch (domain) {
    case P4_TABLE_SOURCE_ELEMENT: {
        if (!issued_row(row, t->elements, t->elements_count, sizeof *t->elements)) return P4_TABLE_INVALID;
        const Element *entry = row;
        ids[0] = entry->desc.id; owner = entry->owner;
        break;
    }
    case P4_TABLE_SOURCE_CHANNEL: {
        if (!issued_row(row, t->channels, t->channels_count, sizeof *t->channels)) return P4_TABLE_INVALID;
        const Channel *entry = row;
        ids[0] = entry->desc.id; owner = entry->owner;
        break;
    }
    case P4_TABLE_SOURCE_AFFINITY: {
        if (!issued_row(row, t->affinities, t->affinities_count, sizeof *t->affinities)) return P4_TABLE_INVALID;
        const Affinity *entry = row;
        ids[0] = entry->row.entity_kind; ids[1] = entry->row.entity_id; ids[2] = entry->row.profile_id;
        ids[3] = entry->row.element_id; ids[4] = entry->row.channel_id; owner = entry->owner;
        break;
    }
    case P4_TABLE_SOURCE_ACTION: {
        if (!issued_row(row, t->actions, t->actions_count, sizeof *t->actions)) return P4_TABLE_INVALID;
        const ActionElement *entry = row;
        ids[0] = entry->binding.source_kind; ids[1] = entry->binding.source_id;
        ids[2] = entry->binding.component; owner = entry->owner;
        break;
    }
    case P4_TABLE_SOURCE_ALIAS: {
        if (!issued_row(row, t->aliases, t->aliases_count, sizeof *t->aliases)) return P4_TABLE_INVALID;
        const AffinityAlias *entry = row;
        ids[0] = entry->alias.entity_kind; ids[1] = entry->alias.entity_id;
        ids[2] = entry->alias.native_profile; owner = entry->owner;
        break;
    }
    case P4_TABLE_SOURCE_EFFECT: {
        if (!issued_row(row, t->effects, t->effects_count, sizeof *t->effects)) return P4_TABLE_INVALID;
        const AffinityEffect *entry = row;
        ids[0] = entry->effect.source_kind; ids[1] = entry->effect.source_id;
        ids[2] = entry->effect.element_id; ids[3] = entry->effect.channel_id; owner = entry->owner;
        break;
    }
    case P4_TABLE_SOURCE_COVER: {
        if (!issued_row(row, t->covers, t->covers_count, sizeof *t->covers)) return P4_TABLE_INVALID;
        const DamageCover *entry = row;
        ids[0] = entry->cover.source_kind; ids[1] = entry->cover.source_id;
        ids[2] = entry->cover.element_id; ids[3] = entry->cover.channel_id; owner = entry->owner;
        break;
    }
    case P4_TABLE_SOURCE_SIDE_AILMENT: {
        if (!issued_row(row, t->sides, t->sides_count, sizeof *t->sides)) return P4_TABLE_INVALID;
        const SideAilment *entry = row;
        ids[0] = entry->def.leaf; owner = entry->owner;
        break;
    }
    default: return P4_TABLE_INVALID;
    }
    if (!owner[0]) return domain == P4_TABLE_SOURCE_ELEMENT || domain == P4_TABLE_SOURCE_CHANNEL ?
        P4_TABLE_OK : P4_TABLE_INVALID;
    if (!t->runtime_use || !t->source_context) return P4_TABLE_NOT_READY;
    int result = t->runtime_use(t, domain, ids, t->source_context);
    return result > 0 ? P4_TABLE_INVALID : result;
}
static int terminated(const char *s, size_t cap)
{
    for (size_t i = 0; i < cap; ++i) if (!s[i]) return 1;
    return 0;
}
static int identifier(const char *s, size_t cap)
{
    if (!s || !terminated(s, cap) || !s[0]) return 0;
    for (; *s; ++s)
        if (!((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '_' || *s == '-' || *s == '.')) return 0;
    return 1;
}
static int owned_key(const char *owner, const char *key)
{
    size_t n = strlen(owner);
    return !strncmp(owner, key, n) && key[n] == ':' && identifier(key + n + 1, P4_TABLE_KEY_SIZE - n - 1);
}
static int key_valid(const char *key)
{
    if (!terminated(key, P4_TABLE_KEY_SIZE)) return 0;
    const char *colon = strchr(key, ':');
    if (!colon || colon == key || (size_t)(colon - key) >= OWNER_SIZE) return 0;
    char owner[OWNER_SIZE];
    memcpy(owner, key, (size_t)(colon - key));
    owner[colon - key] = 0;
    return identifier(owner, sizeof owner) && identifier(colon + 1, P4_TABLE_KEY_SIZE - (size_t)(colon + 1 - key));
}
static int resource_ref_valid(const P4ResourceRef *reference)
{
    if (!reference->revision) return !reference->owner[0] && !reference->key[0];
    if (!identifier(reference->owner, sizeof reference->owner) || reference->owner[0] == '.' ||
        !key_valid(reference->key) || !owned_key(reference->owner, reference->key)) return 0;
    const size_t owner_size = strlen(reference->owner), key_size = strlen(reference->key);
    return reference->owner[owner_size - 1] != '.' && reference->key[owner_size + 1] != '.' &&
        reference->key[key_size - 1] != '.';
}
static int label_valid(const char *s)
{
    if (!terminated(s, P4_TABLE_NAME_SIZE) || !s[0]) return 0;
    for (; *s; ++s) if ((unsigned char)*s < 32 || (unsigned char)*s == 127) return 0;
    return 1;
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
static int element_id_compare(const void *a, const void *b)
{
    uint32_t x = ((const Element *)a)->desc.id, y = ((const Element *)b)->desc.id;
    return (x > y) - (x < y);
}
static int channel_id_compare(const void *a, const void *b)
{
    uint32_t x = ((const Channel *)a)->desc.id, y = ((const Channel *)b)->desc.id;
    return (x > y) - (x < y);
}
static int display_compare(const void *a, const void *b)
{
    const DisplayOrder *x = a, *y = b;
    if (x->order != y->order) return (x->order > y->order) - (x->order < y->order);
    return (x->id > y->id) - (x->id < y->id);
}
static int affinity_compare(const void *a, const void *b)
{
    const P4AffinityRow *x = &((const Affinity *)a)->row, *y = &((const Affinity *)b)->row;
#define CMP(field) if (x->field != y->field) return (x->field > y->field) - (x->field < y->field)
    CMP(entity_kind); CMP(entity_id); CMP(profile_id); CMP(element_id); CMP(channel_id);
#undef CMP
    return 0;
}
static int component_channel_valid(uint32_t component, uint32_t kind)
{
    if (component == P4_ACTION_COMPONENT_INSTANT_KO) return kind == P4_CHANNEL_KO;
    if (component == P4_ACTION_COMPONENT_DAMAGE || component == P4_ACTION_COMPONENT_STATUS)
        return kind == P4_CHANNEL_PERCENT;
    return 1; /* Other semantic roles require their own native capability. */
}
static int action_compare(const void *a, const void *b)
{
    const P4ActionElementBinding *x = &((const ActionElement *)a)->binding, *y = &((const ActionElement *)b)->binding;
    if (x->source_kind != y->source_kind) return (x->source_kind > y->source_kind) - (x->source_kind < y->source_kind);
    if (x->source_id != y->source_id) return (x->source_id > y->source_id) - (x->source_id < y->source_id);
    return (x->component > y->component) - (x->component < y->component);
}
static int alias_compare(const void *a, const void *b)
{
    const P4AffinityProfileAlias *x = &((const AffinityAlias *)a)->alias, *y = &((const AffinityAlias *)b)->alias;
#define CMP(field) if (x->field != y->field) return (x->field > y->field) - (x->field < y->field)
    CMP(entity_kind); CMP(entity_id); CMP(native_profile);
#undef CMP
    return 0;
}
static int effect_pair_compare(const void *a, const void *b)
{
    const EffectPair *x = a, *y = b;
    if (x->element != y->element) return (x->element > y->element) - (x->element < y->element);
    return (x->channel > y->channel) - (x->channel < y->channel);
}
static int effect_compare(const void *a, const void *b)
{
    const P4AffinityEffectBinding *x = &((const AffinityEffect *)a)->effect, *y = &((const AffinityEffect *)b)->effect;
#define CMP(field) if (x->field != y->field) return (x->field > y->field) - (x->field < y->field)
    CMP(source_kind); CMP(source_id); CMP(element_id); CMP(channel_id);
#undef CMP
    return 0;
}
static const AffinityEffect *effect_find(const P4GameTables *t,
    uint32_t kind, uint32_t id, uint32_t element, uint32_t channel)
{
    if (!t || !t->effects_count) return NULL;
    AffinityEffect key = {0};
    key.effect.source_kind = kind; key.effect.source_id = id; key.effect.element_id = element; key.effect.channel_id = channel;
    if (t->frozen) return bsearch(&key, t->effects, t->effects_count, sizeof *t->effects, effect_compare);
    for (size_t i = 0; i < t->effects_count; ++i)
        if (!effect_compare(&key, &t->effects[i])) return &t->effects[i];
    return NULL;
}
static int cover_compare(const void *a, const void *b)
{
    const P4DamageCoverBinding *x = &((const DamageCover *)a)->cover, *y = &((const DamageCover *)b)->cover;
#define CMP(field) if (x->field != y->field) return (x->field > y->field) - (x->field < y->field)
    CMP(source_kind); CMP(source_id); CMP(element_id); CMP(channel_id);
#undef CMP
    return 0;
}
static int side_compare(const void *a, const void *b)
{
    const uint32_t x = ((const SideAilment *)a)->def.leaf, y = ((const SideAilment *)b)->def.leaf;
    return (x > y) - (x < y);
}
static const ActionElement *action_find(const P4GameTables *t, uint32_t kind, uint32_t id, uint32_t component)
{
    if (!t || !t->actions_count) return NULL;
    ActionElement key = {0}; key.binding.source_kind = kind; key.binding.source_id = id;
    key.binding.component = component;
    if (t->frozen) return bsearch(&key, t->actions, t->actions_count, sizeof *t->actions, action_compare);
    for (size_t i = 0; i < t->actions_count; ++i)
        if (!action_compare(&key, &t->actions[i])) return &t->actions[i];
    return NULL;
}
static const AffinityAlias *alias_find(const P4GameTables *t, uint32_t kind, uint32_t id, uint32_t native_profile)
{
    if (!t || !t->aliases_count) return NULL;
    AffinityAlias key = {0};
    key.alias.entity_kind = kind; key.alias.entity_id = id; key.alias.native_profile = native_profile;
    if (t->frozen) return bsearch(&key, t->aliases, t->aliases_count, sizeof *t->aliases, alias_compare);
    for (size_t i = 0; i < t->aliases_count; ++i)
        if (!alias_compare(&key, &t->aliases[i])) return &t->aliases[i];
    return NULL;
}
/* Affinity rows are sorted before resolving alias profiles. Look for the
 * first possible row of the exact profile, never another entity or profile. */
static int profile_present(const P4GameTables *t, const P4AffinityProfileAlias *alias)
{
    Affinity key = {0};
    key.row.entity_kind = alias->entity_kind; key.row.entity_id = alias->entity_id; key.row.profile_id = alias->profile_id;
    size_t first = 0, end = t->affinities_count;
    while (first < end) {
        size_t mid = first + (end - first) / 2;
        if (affinity_compare(&t->affinities[mid], &key) < 0) first = mid + 1;
        else end = mid;
    }
    if (first == t->affinities_count) return 0;
    const P4AffinityRow *row = &t->affinities[first].row;
    return row->entity_kind == alias->entity_kind && row->entity_id == alias->entity_id && row->profile_id == alias->profile_id;
}
static const Element *element_find(const P4GameTables *t, uint32_t id)
{
    if (!t) return NULL;
    if (!t->frozen) {
        for (size_t i = 0; i < t->elements_count; ++i) if (t->elements[i].desc.id == id) return &t->elements[i];
        return NULL;
    }
    Element key = {0}; key.desc.id = id;
    return bsearch(&key, t->elements, t->elements_count, sizeof *t->elements, element_id_compare);
}
static const Channel *channel_find(const P4GameTables *t, uint32_t id)
{
    if (!t) return NULL;
    if (!t->frozen) {
        for (size_t i = 0; i < t->channels_count; ++i) if (t->channels[i].desc.id == id) return &t->channels[i];
        return NULL;
    }
    Channel key = {0}; key.desc.id = id;
    return bsearch(&key, t->channels, t->channels_count, sizeof *t->channels, channel_id_compare);
}

P4GameTables *p4_game_tables_create(void)
{
    static const char *const keys[] = {"physical", "fire", "ice", "electric", "wind", "almighty", "light", "dark",
        "status8", "status9", "status10", "status11", "status12", "status13", "status14", "status15", "special16", "special17", "special18"};
    static const char *const names[] = {"Physical", "Fire", "Ice", "Electric", "Wind", "Almighty", "Light", "Dark",
        "Status 8", "Status 9", "Status 10", "Status 11", "Status 12", "Status 13", "Status 14", "Status 15", "Special 16", "Special 17", "Special 18"};
    P4GameTables *t = calloc(1, sizeof *t);
    if (!t) return NULL;
    if (reserve((void **)&t->elements, &t->elements_capacity, P4_ELEMENT_RETAIL_COUNT, sizeof *t->elements) ||
        reserve((void **)&t->channels, &t->channels_capacity, 3, sizeof *t->channels)) { p4_game_tables_destroy(t); return NULL; }
    for (uint32_t i = 0; i < P4_ELEMENT_RETAIL_COUNT; ++i) {
        Element *e = &t->elements[t->elements_count++];
        memset(e, 0, sizeof *e);
        e->desc.id = e->desc.family = e->desc.order = i;
        e->desc.flags = (i < 8 ? P4_ELEMENT_OFFENSIVE : 0) | (i < 16 ? P4_ELEMENT_AFFINITY : 0) |
                        (i < 8 && i != 5 ? P4_ELEMENT_VISIBLE : 0) | (i == 0 ? P4_ELEMENT_PHYSICAL : 0);
        strcpy(e->desc.key, "p4:"); strcat(e->desc.key, keys[i]);
        strcpy(e->desc.display_name, names[i]); strcpy(e->desc.short_name, names[i]);
    }
    static const char *const channel_keys[] = {"p4:damage", "p4:instant_ko", "p4:status"};
    static const char *const channel_names[] = {"Damage", "Instant KO", "Status"};
    for (uint32_t i = 0; i < 3; ++i) {
        Channel *c = &t->channels[t->channels_count++];
        memset(c, 0, sizeof *c); c->desc.id = i; c->desc.kind = i == P4_CHANNEL_INSTANT_KO ? P4_CHANNEL_KO : P4_CHANNEL_PERCENT;
        strcpy(c->desc.key, channel_keys[i]); strcpy(c->desc.display_name, channel_names[i]);
    }
    return t;
}
void p4_game_tables_destroy(P4GameTables *t)
{
    if (!t) return;
    free(t->elements); free(t->channels); free(t->affinities); free(t->actions); free(t->aliases);
    free(t->effects); free(t->covers); free(t->sides);
    free(t->effect_pairs);
    free(t->display_order); free(t);
}
int p4_game_tables_element(P4GameTables *t, const char *owner, const P4ElementDesc *d)
{
    if (!t || !d || !identifier(owner, OWNER_SIZE) || !strcmp(owner, "p4")) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    if (d->flags & ~15u || !key_valid(d->key) || !label_valid(d->display_name) || !label_valid(d->short_name) ||
        !resource_ref_valid(&d->icon) || !resource_ref_valid(&d->header_icon)) return P4_TABLE_INVALID;
    Element *existing = (Element *)element_find(t, d->id);
    if (existing) {
        if (strcmp(d->key, existing->desc.key) || existing->owner[0] || d->id >= P4_ELEMENT_RETAIL_COUNT) return P4_TABLE_CONFLICT;
        existing->desc = *d; strcpy(existing->owner, owner); return P4_TABLE_OK;
    }
    if (d->id < P4_ELEMENT_RETAIL_COUNT || !owned_key(owner, d->key)) return P4_TABLE_INVALID;
    if (p4_game_tables_element_key(t, d->key)) return P4_TABLE_CONFLICT;
    int rc = reserve((void **)&t->elements, &t->elements_capacity, t->elements_count + 1, sizeof *t->elements);
    if (rc) return rc;
    Element *e = &t->elements[t->elements_count++]; e->desc = *d; strcpy(e->owner, owner); return P4_TABLE_OK;
}
int p4_game_tables_channel(P4GameTables *t, const char *owner, const P4AffinityChannelDesc *d)
{
    if (!t || !d || !identifier(owner, OWNER_SIZE) || !strcmp(owner, "p4")) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    if (d->kind > P4_CHANNEL_KO || !key_valid(d->key) || !owned_key(owner, d->key) || !label_valid(d->display_name)) return P4_TABLE_INVALID;
    if (channel_find(t, d->id) || p4_game_tables_channel_key(t, d->key)) return P4_TABLE_CONFLICT;
    int rc = reserve((void **)&t->channels, &t->channels_capacity, t->channels_count + 1, sizeof *t->channels);
    if (rc) return rc;
    Channel *c = &t->channels[t->channels_count++]; c->desc = *d; strcpy(c->owner, owner); return P4_TABLE_OK;
}
static int add_affinity(P4GameTables *t, const char *owner, const P4AffinityRow *row,
    uint32_t pending, uint32_t retail_row, uint32_t retail_element)
{
    if (!t || !row || !identifier(owner, OWNER_SIZE) || !strcmp(owner, "p4")) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    if (row->entity_kind > P4_ENTITY_ENEMY || (!pending && row->scalar > 65535u) ||
        (pending && retail_element >= 16u)) return P4_TABLE_INVALID;
    Affinity key = {0}; key.row = *row;
    for (size_t i = 0; i < t->affinities_count; ++i)
        if (!affinity_compare(&key, &t->affinities[i])) return P4_TABLE_CONFLICT;
    int rc = reserve((void **)&t->affinities, &t->affinities_capacity, t->affinities_count + 1, sizeof *t->affinities);
    if (rc) return rc;
    Affinity *a = &t->affinities[t->affinities_count++];
    a->row = *row; strcpy(a->owner, owner);
    if (pending) a->row.value = a->row.scalar = 0;
    a->retail_row = retail_row; a->retail_element = retail_element; a->retail_pending = pending;
    return P4_TABLE_OK;
}
int p4_game_tables_affinity(P4GameTables *t, const char *owner, const P4AffinityRow *row)
{
    return add_affinity(t, owner, row, 0, 0, 0);
}
int p4_game_tables_affinity_retail(P4GameTables *t, const char *owner, const P4AffinityRow *row,
    uint32_t retail_row, uint32_t retail_element)
{
    return add_affinity(t, owner, row, 1, retail_row, retail_element);
}
size_t p4_game_tables_affinity_retail_pending(const P4GameTables *t)
{
    size_t count = 0;
    for (size_t i = 0; t && i < t->affinities_count; ++i) count += t->affinities[i].retail_pending != 0;
    return count;
}
int p4_game_tables_affinity_resolve_retail(P4GameTables *t,
    const uint8_t *enemy_bank, size_t enemy_size, const uint8_t *persona_bank, size_t persona_size)
{
    if (!t) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    /* Validate every pending row first: a refusal resolves nothing. */
    for (size_t i = 0; i < t->affinities_count; ++i) {
        const Affinity *a = &t->affinities[i];
        if (!a->retail_pending) continue;
        const uint8_t *bank = a->row.entity_kind == P4_ENTITY_ENEMY ? enemy_bank : persona_bank;
        const size_t size = a->row.entity_kind == P4_ENTITY_ENEMY ? enemy_size : persona_size;
        const char *kind = a->row.entity_kind == P4_ENTITY_ENEMY ? "enemy" : "persona";
        if (!bank || size % 32u || a->retail_row >= size / 32u) {
            fprintf(stderr, "p4pc: %s retail affinity row %u is outside the selected UNIT input (%zu rows)\n",
                kind, (unsigned)a->retail_row, bank && !(size % 32u) ? size / 32u : (size_t)0);
            return P4_TABLE_NOT_FOUND;
        }
        const Channel *channel = channel_find(t, a->row.channel_id);
        if (!channel) return P4_TABLE_NOT_FOUND;
        const uint8_t *entry = bank + (size_t)a->retail_row * 32u + (size_t)a->retail_element * 2u;
        const uint16_t packed = (uint16_t)(entry[0] | entry[1] << 8);
        const uint32_t decoded = p4_affinity_decode(packed), flags = decoded & P4_AFF_FLAGS;
        /* No channel models a flag outside P4_AFF_FLAGS, notably retail's
         * ailment no-effect flag (0x20, read only by datCalcInflictStatus
         * 00238940's 0x27000000 mask); projecting it as KO would invert it.
         * A percentage row also keeps freeze's one-flag rule. */
        if ((decoded & ~(P4_AFF_FLAGS | 65535u)) ||
            (channel->desc.kind != P4_CHANNEL_KO && (flags & (flags - 1u)))) {
            fprintf(stderr, "p4pc: %s retail affinity row %u element %u word 0x%04x has no %s form\n",
                kind, (unsigned)a->retail_row, (unsigned)a->retail_element, (unsigned)packed,
                channel->desc.kind == P4_CHANNEL_KO ? "KO" : "percentage");
            return P4_TABLE_INVALID;
        }
    }
    for (size_t i = 0; i < t->affinities_count; ++i) {
        Affinity *a = &t->affinities[i];
        if (!a->retail_pending) continue;
        const uint8_t *bank = a->row.entity_kind == P4_ENTITY_ENEMY ? enemy_bank : persona_bank;
        const uint8_t *entry = bank + (size_t)a->retail_row * 32u + (size_t)a->retail_element * 2u;
        const uint32_t decoded = p4_affinity_decode((uint16_t)(entry[0] | entry[1] << 8));
        if (channel_find(t, a->row.channel_id)->desc.kind == P4_CHANNEL_KO) {
            p4_affinity_project_ko(decoded, &a->row.value, &a->row.scalar);
        } else {
            a->row.value = decoded; a->row.scalar = decoded & 65535u;
        }
        a->retail_pending = 0;
    }
    return P4_TABLE_OK;
}
int p4_game_tables_action_element(P4GameTables *t, const char *owner, const P4ActionElementBinding *binding)
{
    if (!t || !binding || !identifier(owner, OWNER_SIZE) || !strcmp(owner, "p4")) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    if (binding->source_kind > P4_ACTION_ELEMENT_ENEMY_ATTACK || !binding->schema || !binding->revision ||
        !key_valid(binding->key) || !owned_key(owner, binding->key)) return P4_TABLE_INVALID;
    if (action_find(t, binding->source_kind, binding->source_id, binding->component)) return P4_TABLE_CONFLICT;
    for (size_t i = 0; i < t->actions_count; ++i)
        if (!strcmp(t->actions[i].binding.key, binding->key)) return P4_TABLE_CONFLICT;
    int result = reserve((void **)&t->actions, &t->actions_capacity, t->actions_count + 1, sizeof *t->actions);
    if (result) return result;
    ActionElement *row = &t->actions[t->actions_count++]; row->binding = *binding; strcpy(row->owner, owner);
    return P4_TABLE_OK;
}
int p4_game_tables_affinity_alias(P4GameTables *t, const char *owner, const P4AffinityProfileAlias *alias)
{
    if (!t || !alias || !identifier(owner, OWNER_SIZE) || !strcmp(owner, "p4")) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    if (alias->entity_kind > P4_ENTITY_ENEMY || !alias->schema || !alias->revision ||
        !key_valid(alias->key) || !owned_key(owner, alias->key)) return P4_TABLE_INVALID;
    if (alias_find(t, alias->entity_kind, alias->entity_id, alias->native_profile)) return P4_TABLE_CONFLICT;
    for (size_t i = 0; i < t->aliases_count; ++i)
        if (!strcmp(t->aliases[i].alias.key, alias->key)) return P4_TABLE_CONFLICT;
    int result = reserve((void **)&t->aliases, &t->aliases_capacity, t->aliases_count + 1, sizeof *t->aliases);
    if (result) return result;
    AffinityAlias *row = &t->aliases[t->aliases_count++]; row->alias = *alias; strcpy(row->owner, owner);
    return P4_TABLE_OK;
}
int p4_game_tables_affinity_effect(P4GameTables *t, const char *owner, const P4AffinityEffectBinding *effect)
{
    if (!t || !effect || !identifier(owner, OWNER_SIZE) || !strcmp(owner, "p4")) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    const uint32_t allowed = P4_AFF_RESIST | P4_AFF_NULL | P4_AFF_REPEL | P4_AFF_DRAIN;
    if (effect->source_kind > P4_AFFINITY_EFFECT_EQUIPPED_ITEM || !effect->schema || !effect->revision ||
        !effect->flags || (effect->flags & ~allowed) || (effect->flags & (effect->flags - 1u)) ||
        !key_valid(effect->key) || !owned_key(owner, effect->key)) return P4_TABLE_INVALID;
    for (size_t i = 0; i < t->effects_count; ++i) {
        const AffinityEffect *row = &t->effects[i];
        if (!strcmp(row->effect.key, effect->key)) return P4_TABLE_CONFLICT;
        if (row->effect.source_kind == effect->source_kind && row->effect.source_id == effect->source_id &&
            (strcmp(row->owner, owner) || (row->effect.element_id == effect->element_id && row->effect.channel_id == effect->channel_id)))
            return P4_TABLE_CONFLICT;
    }
    int result = reserve((void **)&t->effects, &t->effects_capacity, t->effects_count + 1, sizeof *t->effects);
    if (result) return result;
    AffinityEffect *row = &t->effects[t->effects_count++]; row->effect = *effect; strcpy(row->owner, owner);
    return P4_TABLE_OK;
}
int p4_game_tables_damage_cover(P4GameTables *t, const char *owner, const P4DamageCoverBinding *cover)
{
    if (!t || !cover || !identifier(owner, OWNER_SIZE) || !strcmp(owner, "p4")) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    if (cover->source_kind > P4_AFFINITY_EFFECT_EQUIPPED_ITEM || cover->channel_id != P4_CHANNEL_DAMAGE ||
        !cover->percent || cover->percent > 99u || !cover->schema || !cover->revision ||
        !key_valid(cover->key) || !owned_key(owner, cover->key) ||
        !key_valid(cover->family) || !owned_key(owner, cover->family)) return P4_TABLE_INVALID;
    for (size_t i = 0; i < t->covers_count; ++i) {
        const DamageCover *row = &t->covers[i];
        if (!strcmp(row->cover.key, cover->key)) return P4_TABLE_CONFLICT;
        if (row->cover.source_kind == cover->source_kind && row->cover.source_id == cover->source_id &&
            (strcmp(row->owner, owner) || row->cover.element_id == cover->element_id))
            return P4_TABLE_CONFLICT;
    }
    int result = reserve((void **)&t->covers, &t->covers_capacity, t->covers_count + 1, sizeof *t->covers);
    if (result) return result;
    DamageCover *row = &t->covers[t->covers_count++]; row->cover = *cover; strcpy(row->owner, owner);
    return P4_TABLE_OK;
}
int p4_game_tables_side_ailment(P4GameTables *t, const char *owner, const P4SideAilmentDef *def)
{
    if (!t || !def || !identifier(owner, OWNER_SIZE) || !strcmp(owner, "p4")) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    if ((def->flags & ~P4_SIDE_FLAGS) || !def->cure_mask || def->crit_bonus > 100u || !def->schema ||
        !def->revision || !key_valid(def->key) || !owned_key(owner, def->key)) return P4_TABLE_INVALID;
    for (size_t i = 0; i < t->sides_count; ++i)
        if (!strcmp(t->sides[i].def.key, def->key) || t->sides[i].def.leaf == def->leaf) return P4_TABLE_CONFLICT;
    if (t->sides_count == P4_SIDE_AILMENT_LIMIT) return P4_TABLE_INVALID;
    int result = reserve((void **)&t->sides, &t->sides_capacity, t->sides_count + 1, sizeof *t->sides);
    if (result) return result;
    SideAilment *row = &t->sides[t->sides_count++]; row->def = *def; strcpy(row->owner, owner);
    return P4_TABLE_OK;
}
int p4_game_tables_freeze(P4GameTables *t, uint32_t revision)
{
    if (!t || !revision) return P4_TABLE_INVALID;
    if (t->frozen) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < t->elements_count; ++i)
        if (!element_find(t, t->elements[i].desc.family)) return P4_TABLE_NOT_FOUND;
    for (size_t i = 0; i < t->affinities_count; ++i) {
        if (t->affinities[i].retail_pending) return P4_TABLE_NOT_READY;
        const P4AffinityRow *a = &t->affinities[i].row;
        const Element *e = element_find(t, a->element_id);
        const Channel *c = channel_find(t, a->channel_id);
        if (!e || !c) return P4_TABLE_NOT_FOUND;
        if (!(e->desc.flags & P4_ELEMENT_AFFINITY)) return P4_TABLE_INVALID;
        if (c->desc.kind == P4_CHANNEL_KO) {
            if (a->value > P4_KO_NULL) return P4_TABLE_INVALID;
        } else {
            uint32_t flags = a->value & P4_AFF_FLAGS;
            if ((a->value & ~(P4_AFF_FLAGS | 65535u)) || (flags && (flags & (flags - 1u)))) return P4_TABLE_INVALID;
        }
    }
    for (size_t i = 0; i < t->actions_count; ++i) {
        const P4ActionElementBinding *binding = &t->actions[i].binding;
        const Channel *channel = channel_find(t, binding->channel_id);
        if (!element_find(t, binding->element_id) || !channel) return P4_TABLE_NOT_FOUND;
        if (!component_channel_valid(binding->component, channel->desc.kind)) return P4_TABLE_INVALID;
    }
    for (size_t i = 0; i < t->effects_count; ++i) {
        const P4AffinityEffectBinding *effect = &t->effects[i].effect;
        const Element *element = element_find(t, effect->element_id);
        const Channel *channel = channel_find(t, effect->channel_id);
        if (!element || !channel) return P4_TABLE_NOT_FOUND;
        if (!(element->desc.flags & P4_ELEMENT_AFFINITY) ||
            (channel->desc.kind == P4_CHANNEL_KO && (effect->flags & (P4_AFF_REPEL | P4_AFF_DRAIN))))
            return P4_TABLE_INVALID;
    }
    for (size_t i = 0; i < t->covers_count; ++i) {
        const Element *element = element_find(t, t->covers[i].cover.element_id);
        if (!element) return P4_TABLE_NOT_FOUND;
        if (!(element->desc.flags & P4_ELEMENT_AFFINITY)) return P4_TABLE_INVALID;
    }
    /* A side ailment's leaf is its owner's own status leaf (affinity-bearing,
     * neither offensive nor physical); its default comes from a damage leaf. */
    for (size_t i = 0; i < t->sides_count; ++i) {
        const SideAilment *side = &t->sides[i];
        const Element *element = element_find(t, side->def.leaf);
        if (!element) return P4_TABLE_NOT_FOUND;
        if (side->def.leaf < P4_ELEMENT_RETAIL_COUNT || strcmp(element->owner, side->owner) ||
            (element->desc.flags & (P4_ELEMENT_AFFINITY | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_PHYSICAL)) !=
                P4_ELEMENT_AFFINITY) return P4_TABLE_INVALID;
        if (side->def.default_from == P4_SIDE_DEFAULT_NONE) continue;
        const Element *from = element_find(t, side->def.default_from);
        if (!from) return P4_TABLE_NOT_FOUND;
        if ((from->desc.flags & (P4_ELEMENT_AFFINITY | P4_ELEMENT_OFFENSIVE)) !=
            (P4_ELEMENT_AFFINITY | P4_ELEMENT_OFFENSIVE)) return P4_TABLE_INVALID;
    }
    if (t->affinities_count) qsort(t->affinities, t->affinities_count, sizeof *t->affinities, affinity_compare);
    for (size_t i = 0; i < t->aliases_count; ++i)
        if (!profile_present(t, &t->aliases[i].alias)) return P4_TABLE_NOT_FOUND;
    DisplayOrder *order = malloc(t->elements_count * sizeof *order);
    if (!order) return P4_TABLE_NOMEM;
    EffectPair *pairs = t->effects_count ? malloc(t->effects_count * sizeof *pairs) : NULL;
    if (t->effects_count && !pairs) { free(order); return P4_TABLE_NOMEM; }
    for (size_t i = 0; i < t->effects_count; ++i)
        pairs[i] = (EffectPair){t->effects[i].effect.element_id, t->effects[i].effect.channel_id};
    if (t->effects_count) qsort(pairs, t->effects_count, sizeof *pairs, effect_pair_compare);
    size_t pairs_count = 0;
    for (size_t i = 0; i < t->effects_count; ++i)
        if (!pairs_count || effect_pair_compare(&pairs[i], &pairs[pairs_count - 1])) {
            if (pairs_count != i) pairs[pairs_count] = pairs[i];
            ++pairs_count;
        }
    qsort(t->elements, t->elements_count, sizeof *t->elements, element_id_compare);
    qsort(t->channels, t->channels_count, sizeof *t->channels, channel_id_compare);
    for (size_t i = 0; i < t->elements_count; ++i)
        order[i] = (DisplayOrder){t->elements[i].desc.order, t->elements[i].desc.id, i};
    qsort(order, t->elements_count, sizeof *order, display_compare);
    if (t->actions_count) qsort(t->actions, t->actions_count, sizeof *t->actions, action_compare);
    if (t->aliases_count) qsort(t->aliases, t->aliases_count, sizeof *t->aliases, alias_compare);
    if (t->effects_count) qsort(t->effects, t->effects_count, sizeof *t->effects, effect_compare);
    if (t->covers_count) qsort(t->covers, t->covers_count, sizeof *t->covers, cover_compare);
    if (t->sides_count) qsort(t->sides, t->sides_count, sizeof *t->sides, side_compare);
    t->effect_pairs = pairs; t->effect_pairs_count = pairs_count;
    t->display_order = order; t->revision = revision; t->frozen = 1; return P4_TABLE_OK;
}
uint32_t p4_game_tables_revision(const P4GameTables *t) { return t && t->frozen ? t->revision : 0; }
const P4ElementDesc *p4_game_tables_find_element(const P4GameTables *t, uint32_t id)
{ const Element *e = element_find(t, id); return e ? &e->desc : NULL; }
const P4AffinityChannelDesc *p4_game_tables_find_channel(const P4GameTables *t, uint32_t id)
{ const Channel *c = channel_find(t, id); return c ? &c->desc : NULL; }
const P4ElementDesc *p4_game_tables_element_key(const P4GameTables *t, const char *key)
{
    if (t && key) for (size_t i = 0; i < t->elements_count; ++i) if (!strcmp(t->elements[i].desc.key, key)) return &t->elements[i].desc;
    return NULL;
}
const P4AffinityChannelDesc *p4_game_tables_channel_key(const P4GameTables *t, const char *key)
{
    if (t && key) for (size_t i = 0; i < t->channels_count; ++i) if (!strcmp(t->channels[i].desc.key, key)) return &t->channels[i].desc;
    return NULL;
}
size_t p4_game_tables_element_count(const P4GameTables *t) { return t ? t->elements_count : 0; }
const P4ElementDesc *p4_game_tables_element_at(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->elements_count ? &t->elements[t->display_order[index].index].desc : NULL; }
const P4AffinityRow *p4_game_tables_find_affinity(const P4GameTables *t, uint32_t kind, uint32_t entity, uint32_t profile, uint32_t element, uint32_t channel)
{
    if (!t || !t->frozen || !t->affinities_count) return NULL;
    Affinity key = {0}; key.row = (P4AffinityRow){kind, entity, profile, element, channel, 0, 0};
    const Affinity *a = bsearch(&key, t->affinities, t->affinities_count, sizeof *t->affinities, affinity_compare);
    return a ? &a->row : NULL;
}
size_t p4_game_tables_channel_count(const P4GameTables *t) { return t ? t->channels_count : 0; }
size_t p4_game_tables_affinity_count(const P4GameTables *t) { return t ? t->affinities_count : 0; }
const P4AffinityChannelDesc *p4_game_tables_channel_at(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->channels_count ? &t->channels[index].desc : NULL; }
const P4AffinityRow *p4_game_tables_affinity_at(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->affinities_count ? &t->affinities[index].row : NULL; }
const char *p4_game_tables_element_owner(const P4GameTables *t, uint32_t id)
{
    const Element *e = element_find(t, id);
    return e && e->owner[0] ? e->owner : NULL;
}
const char *p4_game_tables_channel_owner(const P4GameTables *t, uint32_t id)
{
    const Channel *c = channel_find(t, id);
    return c && c->owner[0] ? c->owner : NULL;
}
const char *p4_game_tables_affinity_owner(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->affinities_count ? t->affinities[index].owner : NULL; }
const P4ActionElementBinding *p4_game_tables_find_action_element(const P4GameTables *t, uint32_t kind, uint32_t id, uint32_t component)
{
    const ActionElement *row = action_find(t, kind, id, component);
    return row ? &row->binding : NULL;
}
const P4AffinityProfileAlias *p4_game_tables_find_affinity_alias(const P4GameTables *t,
    uint32_t kind, uint32_t id, uint32_t native_profile)
{
    const AffinityAlias *row = alias_find(t, kind, id, native_profile);
    return row ? &row->alias : NULL;
}
size_t p4_game_tables_action_element_count(const P4GameTables *t) { return t ? t->actions_count : 0; }
size_t p4_game_tables_affinity_alias_count(const P4GameTables *t) { return t ? t->aliases_count : 0; }
const P4ActionElementBinding *p4_game_tables_action_element_at(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->actions_count ? &t->actions[index].binding : NULL; }
const P4AffinityProfileAlias *p4_game_tables_affinity_alias_at(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->aliases_count ? &t->aliases[index].alias : NULL; }
const char *p4_game_tables_action_element_owner(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->actions_count ? t->actions[index].owner : NULL; }
const char *p4_game_tables_affinity_alias_owner(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->aliases_count ? t->aliases[index].owner : NULL; }
const P4AffinityEffectBinding *p4_game_tables_find_affinity_effect(const P4GameTables *t,
    uint32_t kind, uint32_t id, uint32_t element, uint32_t channel)
{
    const AffinityEffect *row = effect_find(t, kind, id, element, channel);
    return row ? &row->effect : NULL;
}
size_t p4_game_tables_affinity_effect_count(const P4GameTables *t) { return t ? t->effects_count : 0; }
const P4AffinityEffectBinding *p4_game_tables_affinity_effect_at(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->effects_count ? &t->effects[index].effect : NULL; }
const char *p4_game_tables_affinity_effect_owner(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->effects_count ? t->effects[index].owner : NULL; }
int p4_game_tables_has_affinity_effect_pair(const P4GameTables *t, uint32_t element, uint32_t channel)
{
    if (!t || !t->frozen || !t->effect_pairs_count) return 0;
    const EffectPair key = {element, channel};
    return bsearch(&key, t->effect_pairs, t->effect_pairs_count, sizeof *t->effect_pairs, effect_pair_compare) != NULL;
}
const P4DamageCoverBinding *p4_game_tables_find_damage_cover(const P4GameTables *t,
    uint32_t kind, uint32_t id, uint32_t element, uint32_t channel)
{
    if (!t || !t->frozen || !t->covers_count) return NULL;
    DamageCover key = {0};
    key.cover.source_kind = kind; key.cover.source_id = id; key.cover.element_id = element; key.cover.channel_id = channel;
    const DamageCover *row = bsearch(&key, t->covers, t->covers_count, sizeof *t->covers, cover_compare);
    return row ? &row->cover : NULL;
}
size_t p4_game_tables_damage_cover_count(const P4GameTables *t) { return t ? t->covers_count : 0; }
const P4DamageCoverBinding *p4_game_tables_damage_cover_at(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->covers_count ? &t->covers[index].cover : NULL; }
const char *p4_game_tables_damage_cover_owner(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->covers_count ? t->covers[index].owner : NULL; }
const char *p4_game_tables_damage_cover_row_owner(const P4GameTables *t, const P4DamageCoverBinding *row)
{
    return t && t->frozen && issued_row(row, t->covers, t->covers_count, sizeof *t->covers) ?
        ((const DamageCover *)row)->owner : NULL;
}
const P4SideAilmentDef *p4_game_tables_find_side_ailment(const P4GameTables *t, uint32_t leaf)
{
    if (!t || !t->frozen || !t->sides_count) return NULL;
    SideAilment key = {0};
    key.def.leaf = leaf;
    const SideAilment *row = bsearch(&key, t->sides, t->sides_count, sizeof *t->sides, side_compare);
    return row ? &row->def : NULL;
}
size_t p4_game_tables_side_ailment_count(const P4GameTables *t) { return t ? t->sides_count : 0; }
const P4SideAilmentDef *p4_game_tables_side_ailment_at(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->sides_count ? &t->sides[index].def : NULL; }
const char *p4_game_tables_side_ailment_owner(const P4GameTables *t, size_t index)
{ return t && t->frozen && index < t->sides_count ? t->sides[index].owner : NULL; }
static int classify_action_element(const P4GameTables *t, const P4GameActionElement *declared,
    P4GameActionElement *resolved, int runtime)
{
    if (!t || !declared || !resolved || declared->valid > 1u) return P4_TABLE_INVALID;
    if (!t->frozen) return P4_TABLE_NOT_READY;
    if (!declared->valid) {
        *resolved = (P4GameActionElement){.component = declared->component};
        return P4_TABLE_OK;
    }
    const Element *descriptor = element_find(t, declared->element_id);
    const Channel *channel_descriptor = channel_find(t, declared->channel_id);
    if (!descriptor || !channel_descriptor) return P4_TABLE_NOT_FOUND;
    if (runtime) {
        int result = p4_game_tables_runtime_use_row(t, P4_TABLE_SOURCE_ELEMENT, &descriptor->desc);
        if (result) return result;
        result = p4_game_tables_runtime_use_row(t, P4_TABLE_SOURCE_CHANNEL, &channel_descriptor->desc);
        if (result) return result;
    }
    if (!component_channel_valid(declared->component, channel_descriptor->desc.kind)) return P4_TABLE_INVALID;
    *resolved = (P4GameActionElement){1u, declared->element_id, declared->channel_id,
        descriptor->desc.flags, descriptor->desc.family, declared->component};
    return P4_TABLE_OK;
}
int p4_game_tables_classify_action_element(const P4GameTables *t,
    const P4GameActionElement *declared, P4GameActionElement *resolved)
{ return classify_action_element(t, declared, resolved, 0); }
int p4_game_tables_classify_runtime_action_element(const P4GameTables *t,
    const P4GameActionElement *declared, P4GameActionElement *resolved)
{ return classify_action_element(t, declared, resolved, 1); }
int p4_game_tables_resolve_action_element(const P4GameTables *t,
    uint32_t kind, uint32_t id, uint32_t component, const P4GameActionElement *declared, P4GameActionElement *resolved)
{
    if (!t || !resolved || kind > P4_ACTION_ELEMENT_ENEMY_ATTACK || (declared && declared->valid > 1u))
        return P4_TABLE_INVALID;
    if (!t->frozen) return P4_TABLE_NOT_READY;
    const ActionElement *action = action_find(t, kind, id, component);
    P4GameActionElement value = {.component = component};
    if (action) {
        value.valid = 1u; value.element_id = action->binding.element_id; value.channel_id = action->binding.channel_id;
    } else if (declared && declared->valid) {
        if (declared->component != component) return P4_TABLE_INVALID;
        value = *declared;
    }
    return p4_game_tables_classify_action_element(t, &value, resolved);
}
const char *p4_game_tables_error(int result)
{
    switch (result) {
    case P4_TABLE_OK: return "success";
    case P4_TABLE_INVALID: return "invalid table descriptor or affinity value";
    case P4_TABLE_CONFLICT: return "table id, key or row already claimed";
    case P4_TABLE_NOMEM: return "table allocation failed";
    case P4_TABLE_FROZEN: return "immutable table snapshot";
    case P4_TABLE_NOT_FOUND: return "unresolved table reference";
    case P4_TABLE_PREPARE_REQUIRED: return "native table preparation callback is not registered";
    case P4_TABLE_NOT_READY: return "native table publication prerequisites are not ready";
    case P4_TABLE_RENDER_ERROR: return "required authored resource render failed";
    default: return "unknown table result";
    }
}
uint32_t p4_affinity_decode(uint16_t packed)
{
    uint32_t value = ((uint32_t)(packed & 0xff00u) << 16) | ((packed & 0xffu) * 5u);
    if (!(value & 65535u)) {
        if (value & P4_AFF_WEAK) value |= 125u;
        else if (value & 0x27000000u) value |= 100u;
        else if (value & P4_AFF_RESIST) value |= 50u;
    }
    return value;
}
void p4_affinity_project_ko(uint32_t decoded, uint32_t *kind, uint32_t *scalar)
{
    uint32_t k = decoded & (P4_AFF_NULL | P4_AFF_REPEL | P4_AFF_DRAIN) ? P4_KO_NULL :
                 decoded & P4_AFF_WEAK ? P4_KO_VULNERABLE : decoded & P4_AFF_RESIST ? P4_KO_RESIST : P4_KO_NORMAL;
    *kind = k; *scalar = k == P4_KO_NULL ? 0 : decoded & 65535u;
}
