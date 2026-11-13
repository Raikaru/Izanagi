#include "game_knowledge_runtime_internal.h"
#include "p4pc/game_table_refs.h"
#include <stdlib.h>
#include <string.h>

/* Schema 2 appends the retail status bits and schema 3 then the defaulted
 * side-ailment bits of retail enemies without an alias; an older block loads
 * with its missing sections clear. */
#define KNOWLEDGE_SCHEMA 3u
#define KNOWLEDGE_SCHEMA_SIDE_ONLY 1u
#define KNOWLEDGE_SCHEMA_STATUS 2u
#define KNOWLEDGE_MAGIC 0x4e4b3450u /* P4KN, little endian */
#define KNOWLEDGE_MAX_BYTES (16u * 1024u * 1024u)
/* Eight words, four non-empty strings and four reference variants. */
#define MINIMUM_RECORD_BYTES 68u
/* Retail enemy id and its nonzero status bits. */
#define STATUS_RECORD_BYTES 8u
/* Retail enemy id and its side-ailment reference variant (at least). */
#define SIDE_RECORD_BYTES 8u

typedef struct KnowledgeLoad {
    P4GameKnowledge *state;
    P4KnowledgeRetail retail;
    size_t count;
    size_t indices[];
} KnowledgeLoad;
typedef struct KnowledgeSaveContext {
    const P4GameTables *tables;
    const P4GameTableSources *sources;
} KnowledgeSaveContext;
static P4ModSaveRegistration *registration;
static KnowledgeSaveContext save_context;

static int snapshot_ready(const KnowledgeSaveContext *source)
{
    if (!source || !p4_game_tables_revision(source->tables)) return P4_TABLE_NOT_READY;
    const P4GameTables *published = p4_game_knowledge_native_snapshot();
    if ((published && published != source->tables) ||
        (!published && p4_game_tables_affinity_alias_count(source->tables))) return P4_TABLE_NOT_READY;
    return P4_TABLE_OK;
}
static int write_string(P4ModSaveWriter *writer, const char *text)
{
    size_t length = strlen(text);
    if (!length || length >= P4_TABLE_KEY_SIZE) return P4_TABLE_INVALID;
    int result = p4_mod_save_write_u32(writer, (uint32_t)length);
    return result ? result : p4_mod_save_write(writer, text, length);
}
static int read_string(P4ModSaveReader *reader, char *text)
{
    uint32_t length;
    int result = p4_mod_save_read_u32(reader, &length);
    if (result) return result;
    if (!length || length >= P4_TABLE_KEY_SIZE || length > p4_mod_save_remaining(reader)) return P4_TABLE_INVALID;
    result = p4_mod_save_read(reader, text, length);
    if (result) return result;
    if (memchr(text, 0, length)) return P4_TABLE_INVALID;
    text[length] = 0;
    return P4_TABLE_OK;
}
/* Builtin references have an explicit component-owned variant. They may only
 * denote the actual unowned retail descriptors; a missing authored ledger
 * entry never switches to this format. The record already contains the exact
 * descriptor key/revision, checked by identity_find on restore. */
static int write_reference(P4ModSaveWriter *writer, P4GameTableRefWriter *refs,
    const P4GameTables *tables, uint32_t domain, const uint32_t ids[5], int owned)
{
    if (!owned && !((domain == P4_TABLE_SOURCE_ELEMENT && ids[0] < P4_ELEMENT_RETAIL_COUNT) ||
        (domain == P4_TABLE_SOURCE_CHANNEL && ids[0] <= P4_CHANNEL_STATUS))) return P4_TABLE_INVALID;
    int result = p4_mod_save_write_u32(writer, owned ? 1u : 0u);
    return result || !owned ? result : p4_game_table_refs_write(refs, tables, domain, ids);
}
static int read_reference(P4ModSaveReader *reader, P4GameTableRefReader *refs,
    const P4GameTables *tables, uint32_t domain, const uint32_t ids[5], int owned)
{
    uint32_t variant;
    int result = p4_mod_save_read_u32(reader, &variant);
    if (result) return result;
    if (variant != (owned ? 1u : 0u)) return P4_TABLE_CONFLICT;
    if (!owned) return ((domain == P4_TABLE_SOURCE_ELEMENT && ids[0] < P4_ELEMENT_RETAIL_COUNT) ||
        (domain == P4_TABLE_SOURCE_CHANNEL && ids[0] <= P4_CHANNEL_STATUS)) ? P4_TABLE_OK : P4_TABLE_INVALID;
    const P4TableSourceEntry *entry;
    result = p4_game_table_refs_read(refs, tables, domain, &entry);
    if (result) return result;
    return memcmp(entry->ids, ids, sizeof entry->ids) ? P4_TABLE_CONFLICT : P4_TABLE_OK;
}
/* Alias, element, channel and side-row references, in that order; a defaulted
 * pair has no row, so its fourth reference is its side ailment. */
static void reference_ids(const P4AffinityProfileAlias *alias, const P4KnowledgeIdentity *identity,
    uint32_t ids[4][5], uint32_t domains[4])
{
    const P4KnowledgeKey *key = &identity->key;
    const uint32_t values[4][5] = {
        {alias->entity_kind, alias->entity_id, alias->native_profile, 0, 0},
        {key->element_id, 0, 0, 0, 0},
        {key->channel_id, 0, 0, 0, 0},
        {alias->entity_kind, alias->entity_id, alias->profile_id, key->element_id, key->channel_id}};
    const uint32_t kinds[4] = {P4_TABLE_SOURCE_ALIAS, P4_TABLE_SOURCE_ELEMENT, P4_TABLE_SOURCE_CHANNEL,
        identity->defaulted ? P4_TABLE_SOURCE_SIDE_AILMENT : P4_TABLE_SOURCE_AFFINITY};
    memcpy(ids, values, sizeof values);
    memcpy(domains, kinds, sizeof kinds);
    if (identity->defaulted) memcpy(ids[3], (const uint32_t[5]){key->element_id, 0, 0, 0, 0}, sizeof ids[3]);
}
/* The side-ailment index of a defaulted leaf (UINT32_MAX for none). */
static uint32_t defaulted_index(const P4GameTables *tables, uint32_t leaf)
{
    for (size_t i = 0; i < p4_game_tables_side_ailment_count(tables) && i < P4_SIDE_AILMENT_LIMIT; ++i) {
        const P4SideAilmentDef *side = p4_game_tables_side_ailment_at(tables, i);
        if (side->leaf == leaf) return side->default_from == P4_SIDE_DEFAULT_NONE ? UINT32_MAX : (uint32_t)i;
    }
    return UINT32_MAX;
}
static int write_record(const P4GameKnowledge *state, size_t index,
    const P4KnowledgeIdentity *identity, P4ModSaveWriter *writer, P4GameTableRefWriter *refs)
{
    const P4GameTables *tables = p4_game_knowledge_snapshot(state);
    const P4AffinityProfileAlias *alias = p4_game_knowledge_identity_alias_at(state, index);
    if (!alias) return P4_TABLE_INVALID;
    const uint32_t fields[] = {identity->key.knowledge_id, identity->key.profile_id,
        identity->key.element_id, identity->key.channel_id, identity->alias_schema,
        identity->alias_revision, identity->element_revision, identity->channel_revision};
    for (unsigned f = 0; f < sizeof fields / sizeof fields[0]; ++f) {
        int result = p4_mod_save_write_u32(writer, fields[f]);
        if (result) return result;
    }
    const char *strings[] = {identity->owner, identity->alias_key, identity->element_key, identity->channel_key};
    for (unsigned s = 0; s < sizeof strings / sizeof strings[0]; ++s) {
        int result = write_string(writer, strings[s]);
        if (result) return result;
    }
    uint32_t ids[4][5], domains[4];
    reference_ids(alias, identity, ids, domains);
    const int owned[4] = {1, identity->element_owner != NULL, identity->channel_owner != NULL, 1};
    for (unsigned r = 0; r < 4; ++r) {
        int result = write_reference(writer, refs, tables, domains[r], ids[r], owned[r]);
        if (result) return result;
    }
    return P4_TABLE_OK;
}
static int encode(const P4ModSaveContext *context, P4ModSaveWriter *writer, void *user)
{
    const KnowledgeSaveContext *source = user;
    int result = snapshot_ready(source);
    if (result) return result;
    const P4GameKnowledge *state = p4_game_knowledge_native_active();
    uint32_t count = 0;
    for (size_t i = 0; i < p4_game_knowledge_count(state); ++i) {
        P4KnowledgeIdentity identity; int known;
        result = p4_game_knowledge_identity_at(state, i, &identity, &known);
        if (result) return result;
        if (known) {
            if (count == UINT32_MAX) return P4_TABLE_INVALID;
            ++count;
        }
    }
    const P4KnowledgeRetail *retail = p4_game_knowledge_retail(state);
    uint32_t statuses = 0, sides = 0;
    for (uint32_t enemy = 0; retail && enemy < P4_KNOWLEDGE_RETAIL_ENEMIES; ++enemy) {
        statuses += retail->status[enemy] != 0;
        for (uint32_t bits = retail->side[enemy]; bits; bits &= bits - 1u) ++sides;
    }
    if (!count && !statuses && !sides) return 1; /* Omit: the absent block decodes to the cleared default. */
    if (p4_game_knowledge_snapshot(state) != source->tables) return P4_TABLE_NOT_READY;
    result = p4_mod_save_write_u32(writer, KNOWLEDGE_MAGIC);
    if (!result) result = p4_mod_save_write_u32(writer, KNOWLEDGE_SCHEMA);
    if (!result) result = p4_mod_save_write_u32(writer, count);
    if (result) return result;
    P4GameTableRefWriter *refs = NULL;
    result = p4_game_table_refs_writer_begin(source->sources, context, writer, &refs);
    if (result) return result;
    for (size_t i = 0; i < p4_game_knowledge_count(state); ++i) {
        P4KnowledgeIdentity identity; int known;
        result = p4_game_knowledge_identity_at(state, i, &identity, &known);
        if (!result && known) result = write_record(state, i, &identity, writer, refs);
        if (result) break;
    }
    /* Retail enemies and slots are the game's own fixed ids: no ledger reference. */
    if (!result) result = p4_mod_save_write_u32(writer, statuses);
    for (uint32_t enemy = 0; !result && enemy < P4_KNOWLEDGE_RETAIL_ENEMIES; ++enemy) {
        if (!retail->status[enemy]) continue;
        result = p4_mod_save_write_u32(writer, enemy);
        if (!result) result = p4_mod_save_write_u32(writer, retail->status[enemy]);
    }
    /* A side ailment is authored: its ledger reference names it. */
    if (!result) result = p4_mod_save_write_u32(writer, sides);
    for (uint32_t enemy = 0; !result && enemy < P4_KNOWLEDGE_RETAIL_ENEMIES; ++enemy) {
        for (uint32_t i = 0; !result && i < P4_SIDE_AILMENT_LIMIT; ++i) {
            if (!(retail->side[enemy] & (1u << i))) continue;
            const uint32_t ids[5] = {p4_game_tables_side_ailment_at(source->tables, i)->leaf, 0, 0, 0, 0};
            result = p4_mod_save_write_u32(writer, enemy);
            if (!result) result = write_reference(writer, refs, source->tables,
                P4_TABLE_SOURCE_SIDE_AILMENT, ids, 1);
        }
    }
    p4_game_table_refs_writer_end(refs);
    /* Retail enemies and slots are the game's own fixed ids: no ledger reference. */
    if (!result) result = p4_mod_save_write_u32(writer, statuses);
    for (uint32_t enemy = 0; !result && enemy < P4_KNOWLEDGE_RETAIL_ENEMIES; ++enemy) {
        uint32_t bits = retail ? retail->status[enemy] : 0;
        if (!bits) continue;
        result = p4_mod_save_write_u32(writer, enemy);
        if (!result) result = p4_mod_save_write_u32(writer, bits);
    }
    return result;
}
static int read_record(P4GameKnowledge *state, P4ModSaveReader *reader,
    P4GameTableRefReader *refs, size_t *index)
{
    uint32_t fields[8];
    for (unsigned f = 0; f < sizeof fields / sizeof fields[0]; ++f) {
        int result = p4_mod_save_read_u32(reader, &fields[f]);
        if (result) return result;
    }
    char strings[4][P4_TABLE_KEY_SIZE];
    for (unsigned s = 0; s < sizeof strings / sizeof strings[0]; ++s) {
        int result = read_string(reader, strings[s]);
        if (result) return result;
    }
    P4KnowledgeIdentity identity = {{fields[0], fields[1], fields[2], fields[3]},
        strings[0], strings[1], strings[2], strings[3], fields[4], fields[5], fields[6], fields[7],
        NULL, NULL, 0};
    int result = p4_game_knowledge_identity_find(state, &identity, index);
    if (result) return result;
    int known;
    result = p4_game_knowledge_identity_at(state, *index, &identity, &known);
    if (result) return result;
    const P4AffinityProfileAlias *alias = p4_game_knowledge_identity_alias_at(state, *index);
    if (!alias) return P4_TABLE_INVALID;
    uint32_t ids[4][5], domains[4];
    reference_ids(alias, &identity, ids, domains);
    const int owned[4] = {1, identity.element_owner != NULL, identity.channel_owner != NULL, 1};
    for (unsigned r = 0; r < 4; ++r) {
        result = read_reference(reader, refs, p4_game_knowledge_snapshot(state), domains[r], ids[r], owned[r]);
        if (result) return result;
    }
    return P4_TABLE_OK;
}
static int read_status(P4ModSaveReader *reader, KnowledgeLoad *load)
{
    uint32_t statuses;
    int result = p4_mod_save_read_u32(reader, &statuses);
    if (result) return result;
    if (statuses > P4_KNOWLEDGE_RETAIL_ENEMIES ||
        statuses > p4_mod_save_remaining(reader) / STATUS_RECORD_BYTES) return P4_TABLE_INVALID;
    if (statuses && !load->state) return P4_TABLE_NOT_READY;
    for (uint32_t i = 0, previous = 0; i < statuses; ++i) {
        uint32_t enemy, bits;
        result = p4_mod_save_read_u32(reader, &enemy);
        if (!result) result = p4_mod_save_read_u32(reader, &bits);
        if (result) return result;
        if (enemy >= P4_KNOWLEDGE_RETAIL_ENEMIES || (i && enemy <= previous) || !bits || bits > 0xffu)
            return P4_TABLE_INVALID;
        load->retail.status[enemy] = (uint8_t)bits;
        previous = enemy;
    }
    return P4_TABLE_OK;
}
static int read_side(P4ModSaveReader *reader, P4GameTableRefReader *refs, KnowledgeLoad *load)
{
    uint32_t sides;
    int result = p4_mod_save_read_u32(reader, &sides);
    if (result) return result;
    if (sides > P4_KNOWLEDGE_RETAIL_ENEMIES * P4_SIDE_AILMENT_LIMIT ||
        sides > p4_mod_save_remaining(reader) / SIDE_RECORD_BYTES) return P4_TABLE_INVALID;
    if (sides && !load->state) return P4_TABLE_NOT_READY;
    const P4GameTables *tables = p4_game_knowledge_snapshot(load->state);
    for (uint32_t i = 0, previous = 0; i < sides; ++i) {
        uint32_t enemy, variant;
        result = p4_mod_save_read_u32(reader, &enemy);
        if (!result) result = p4_mod_save_read_u32(reader, &variant);
        if (result) return result;
        if (variant != 1u) return P4_TABLE_CONFLICT;
        const P4TableSourceEntry *entry;
        result = p4_game_table_refs_read(refs, tables, P4_TABLE_SOURCE_SIDE_AILMENT, &entry);
        if (result) return result;
        const uint32_t index = defaulted_index(tables, entry->ids[0]);
        /* Strictly increasing (enemy, index): no duplicate or reordered bit. */
        const uint32_t order = enemy * P4_SIDE_AILMENT_LIMIT + index;
        if (enemy >= P4_KNOWLEDGE_RETAIL_ENEMIES || index == UINT32_MAX || (i && order <= previous))
            return P4_TABLE_INVALID;
        load->retail.side[enemy] |= (uint16_t)(1u << index);
        previous = order;
    }
    return P4_TABLE_OK;
}
static int decode(const P4ModSaveContext *context, uint32_t schema,
    P4ModSaveReader *reader, void **staged, void *user)
{
    const KnowledgeSaveContext *source = user;
    if (context && context->image.data) {
        int result = snapshot_ready(source);
        if (result) return result;
    }
    if (!reader) return schema == 0 ? P4_TABLE_OK : P4_TABLE_INVALID;
    if (schema != KNOWLEDGE_SCHEMA && schema != KNOWLEDGE_SCHEMA_STATUS && schema != KNOWLEDGE_SCHEMA_SIDE_ONLY)
        return P4_TABLE_INVALID;
    uint32_t magic, version, count;
    int result = p4_mod_save_read_u32(reader, &magic);
    if (!result) result = p4_mod_save_read_u32(reader, &version);
    if (!result) result = p4_mod_save_read_u32(reader, &count);
    if (result) return result;
    if (magic != KNOWLEDGE_MAGIC || version != schema || (schema == KNOWLEDGE_SCHEMA_SIDE_ONLY && !count))
        return P4_TABLE_INVALID;
    P4GameKnowledge *state = p4_game_knowledge_native_active();
    if (count > p4_game_knowledge_count(state) ||
        count > p4_mod_save_remaining(reader) / MINIMUM_RECORD_BYTES) return P4_TABLE_INVALID;
#if SIZE_MAX == UINT32_MAX
    /* Every u32 count fits the size_t allocation on 64-bit hosts, not on 32-bit. */
    if (count > (SIZE_MAX - sizeof(KnowledgeLoad)) / sizeof(size_t)) return P4_TABLE_INVALID;
#endif
    KnowledgeLoad *load = malloc(sizeof *load + (size_t)count * sizeof *load->indices);
    if (!load) return P4_TABLE_NOMEM;
    load->state = state; load->count = count;
    memset(&load->retail, 0, sizeof load->retail);
    *staged = load;
    P4GameTableRefReader *refs = NULL;
    result = p4_game_table_refs_reader_begin(source->sources, context, reader, &refs);
    if (result) return result;
    for (uint32_t i = 0; i < count; ++i) {
        result = read_record(state, reader, refs, &load->indices[i]);
        if (!result && i && load->indices[i - 1] >= load->indices[i]) result = P4_TABLE_INVALID;
        if (result) break;
    }
    if (!result && schema >= KNOWLEDGE_SCHEMA_STATUS) result = read_status(reader, load);
    if (!result && schema >= KNOWLEDGE_SCHEMA) result = read_side(reader, refs, load);
    p4_game_table_refs_reader_end(refs);
    if (!result && schema == KNOWLEDGE_SCHEMA) result = read_status(reader, load);
    if (!result && p4_mod_save_remaining(reader)) result = P4_TABLE_INVALID;
    /* A written block holds at least one known key or retail bit. */
    if (!result && schema != KNOWLEDGE_SCHEMA_SIDE_ONLY && !count) {
        int any = 0;
        for (uint32_t e = 0; !any && e < P4_KNOWLEDGE_RETAIL_ENEMIES; ++e)
            any = load->retail.status[e] || load->retail.side[e];
        if (!any) result = P4_TABLE_INVALID;
    }
    return result;
}
static int validate(const P4ModSaveContext *context, void *staged, void *user)
{
    (void)context;
    const KnowledgeSaveContext *source = user;
    const KnowledgeLoad *load = staged;
    if (!load) return P4_TABLE_OK;
    return load->state == p4_game_knowledge_native_active() &&
        p4_game_knowledge_snapshot(load->state) == source->tables &&
        p4_game_table_sources_validate_view(source->sources, source->tables) == P4_TABLE_OK ?
        P4_TABLE_OK : P4_TABLE_CONFLICT;
}
static void commit(void *staged, void *user)
{
    (void)user;
    KnowledgeLoad *load = staged;
    if (!load) p4_game_knowledge_clear(p4_game_knowledge_native_active());
    else {
        if (p4_game_knowledge_replace(load->state, load->indices, load->count, &load->retail)) abort();
        free(load);
    }
}
static void discard(void *staged, void *user) { (void)user; free(staged); }
int p4_game_knowledge_save_register(const P4GameTables *tables, const P4GameTableSources *sources)
{
    if (!tables || !sources || !p4_game_table_sources_is_sealed(sources)) return P4_TABLE_INVALID;
    int readiness = p4_game_table_sources_validate_view(sources, tables);
    if (readiness) return readiness;
    if (registration)
        return save_context.tables == tables && save_context.sources == sources ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    save_context = (KnowledgeSaveContext){tables, sources};
    const P4ModSaveProvider provider = {
        .struct_size = sizeof provider, .abi_version = P4_MOD_SAVE_ABI_VERSION,
        .owner = "p4pc", .key = "enemy-knowledge", .schema_version = KNOWLEDGE_SCHEMA,
        .flags = P4_MOD_SAVE_REQUIRED, .max_bytes = KNOWLEDGE_MAX_BYTES, .user = &save_context,
        .encode = encode, .decode = decode, .validate = validate, .commit = commit, .discard = discard
    };
    if (p4_mod_save_register(&provider, &registration)) {
        save_context = (KnowledgeSaveContext){0};
        return P4_TABLE_CONFLICT;
    }
    return P4_TABLE_OK;
}
int p4_game_knowledge_save_ready(const P4GameTables *tables)
{
    return registration && save_context.tables == tables ? P4_TABLE_OK : P4_TABLE_NOT_READY;
}
int p4_game_knowledge_save_unregister(void)
{
    if (!registration) return P4_TABLE_OK;
    if (p4_mod_save_unregister(registration)) return P4_TABLE_CONFLICT;
    registration = NULL;
    save_context = (KnowledgeSaveContext){0};
    return P4_TABLE_OK;
}
