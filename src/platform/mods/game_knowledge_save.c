#include "game_knowledge_runtime_internal.h"
#include "p4pc/game_table_refs.h"
#include <stdlib.h>
#include <string.h>

#define KNOWLEDGE_SCHEMA 1u
#define KNOWLEDGE_MAGIC 0x4e4b3450u /* P4KN, little endian */
#define KNOWLEDGE_MAX_BYTES (16u * 1024u * 1024u)
/* Eight words, four non-empty strings and four reference variants. */
#define MINIMUM_RECORD_BYTES 68u

typedef struct KnowledgeLoad {
    P4GameKnowledge *state;
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
/* Alias, element, channel and side-row references, in that order. */
static void reference_ids(const P4AffinityProfileAlias *alias, const P4KnowledgeKey *key,
    uint32_t ids[4][5])
{
    const uint32_t values[4][5] = {
        {alias->entity_kind, alias->entity_id, alias->native_profile, 0, 0},
        {key->element_id, 0, 0, 0, 0},
        {key->channel_id, 0, 0, 0, 0},
        {alias->entity_kind, alias->entity_id, alias->profile_id, key->element_id, key->channel_id}};
    memcpy(ids, values, sizeof values);
}
static const uint32_t reference_domains[4] = {
    P4_TABLE_SOURCE_ALIAS, P4_TABLE_SOURCE_ELEMENT, P4_TABLE_SOURCE_CHANNEL, P4_TABLE_SOURCE_AFFINITY};
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
    uint32_t ids[4][5];
    reference_ids(alias, &identity->key, ids);
    const int owned[4] = {1, identity->element_owner != NULL, identity->channel_owner != NULL, 1};
    for (unsigned r = 0; r < 4; ++r) {
        int result = write_reference(writer, refs, tables, reference_domains[r], ids[r], owned[r]);
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
    if (!count) return 1; /* Omit: the absent block decodes to the cleared default. */
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
    p4_game_table_refs_writer_end(refs);
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
        NULL, NULL};
    int result = p4_game_knowledge_identity_find(state, &identity, index);
    if (result) return result;
    int known;
    result = p4_game_knowledge_identity_at(state, *index, &identity, &known);
    if (result) return result;
    const P4AffinityProfileAlias *alias = p4_game_knowledge_identity_alias_at(state, *index);
    if (!alias) return P4_TABLE_INVALID;
    uint32_t ids[4][5];
    reference_ids(alias, &identity.key, ids);
    const int owned[4] = {1, identity.element_owner != NULL, identity.channel_owner != NULL, 1};
    for (unsigned r = 0; r < 4; ++r) {
        result = read_reference(reader, refs, p4_game_knowledge_snapshot(state),
            reference_domains[r], ids[r], owned[r]);
        if (result) return result;
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
    if (schema != KNOWLEDGE_SCHEMA) return P4_TABLE_INVALID;
    uint32_t magic, version, count;
    int result = p4_mod_save_read_u32(reader, &magic);
    if (!result) result = p4_mod_save_read_u32(reader, &version);
    if (!result) result = p4_mod_save_read_u32(reader, &count);
    if (result) return result;
    if (magic != KNOWLEDGE_MAGIC || version != KNOWLEDGE_SCHEMA || !count) return P4_TABLE_INVALID;
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
    *staged = load;
    P4GameTableRefReader *refs = NULL;
    result = p4_game_table_refs_reader_begin(source->sources, context, reader, &refs);
    if (result) return result;
    for (uint32_t i = 0; i < count; ++i) {
        result = read_record(state, reader, refs, &load->indices[i]);
        if (!result && i && load->indices[i - 1] >= load->indices[i]) result = P4_TABLE_INVALID;
        if (result) break;
    }
    p4_game_table_refs_reader_end(refs);
    if (!result && p4_mod_save_remaining(reader)) result = P4_TABLE_INVALID;
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
        if (p4_game_knowledge_replace(load->state, load->indices, load->count)) abort();
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
