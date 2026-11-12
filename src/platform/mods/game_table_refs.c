#include "p4pc/game_table_refs.h"
#include "p4pc/packages.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct P4GameTableRefWriter {
    const P4GameTableSources *sources;
    const P4ModSaveContext *transaction;
    P4ModSaveWriter *writer;
    size_t capacity;
    uint32_t count;
    uint32_t indices[]; /* Current origin handle -> this stream's dictionary ID. */
};
struct P4GameTableRefReader {
    const P4GameTableSources *sources;
    const P4ModSaveContext *transaction;
    P4ModSaveReader *reader;
    size_t capacity;
    uint32_t count;
    uint32_t indices[]; /* Wire dictionary ID -> CURRENT local origin handle. */
};
static int failure(const P4ModSaveContext *transaction, const char *reason)
{
    if (transaction && transaction->error && transaction->error_capacity)
        snprintf(transaction->error, transaction->error_capacity, "%s", reason);
    return P4_TABLE_INVALID;
}
static void *allocate(size_t size)
{
    if (p4_packages_memory_reserve(size)) return NULL;
    void *memory = malloc(size);
    if (!memory) { p4_packages_memory_cancel(size); return NULL; }
    p4_packages_memory_commit(size);
    return memory;
}
static int origin_current(const P4TableSourceResolved *resolved)
{
    const P4TableSourceOrigin *origin = resolved->origin;
    if (origin->kind == P4_TABLE_SOURCE_NATIVE) return !resolved->provider;
    if (origin->kind != P4_TABLE_SOURCE_PACKAGE || !resolved->provider || !p4_package_provider_selected(resolved->provider)) return 0;
    const P4PackageInfo *info = p4_package_provider_info(resolved->provider);
    return info && (uint32_t)info->kind == origin->package_kind && !strcmp(info->id, origin->package_id) &&
        !strcmp(info->version, origin->package_version);
}
static int write_string(P4ModSaveWriter *writer, const char *text)
{
    size_t length = strlen(text);
    return p4_mod_save_write_u32(writer, (uint32_t)length) || p4_mod_save_write(writer, text, length) ? -1 : 0;
}
static int read_string_equal(P4ModSaveReader *reader, const char *current, size_t capacity)
{
    uint32_t length;
    unsigned char bytes[P4_TABLE_KEY_SIZE];
    if (capacity > sizeof bytes || p4_mod_save_read_u32(reader, &length) || length >= capacity ||
        length != strlen(current) || p4_mod_save_read(reader, bytes, length)) return -1;
    return memcmp(bytes, current, length) ? -1 : 0;
}
static int write_origin(P4ModSaveWriter *writer, const P4TableSourceOrigin *origin)
{
    if (p4_mod_save_write_u32(writer, origin->kind) || p4_mod_save_write_u32(writer, origin->schema) ||
        p4_mod_save_write_u32(writer, origin->revision) || write_string(writer, origin->source_key)) return -1;
    return origin->kind == P4_TABLE_SOURCE_PACKAGE ?
        (p4_mod_save_write_u32(writer, origin->package_kind) || write_string(writer, origin->package_id) ||
            write_string(writer, origin->package_version) ? -1 : 0) : 0;
}
static int read_origin_equal(P4ModSaveReader *reader, const P4TableSourceOrigin *origin)
{
    uint32_t kind, schema, revision, package_kind;
    if (p4_mod_save_read_u32(reader, &kind) || kind != origin->kind ||
        p4_mod_save_read_u32(reader, &schema) || schema != origin->schema ||
        p4_mod_save_read_u32(reader, &revision) || revision != origin->revision ||
        read_string_equal(reader, origin->source_key, sizeof origin->source_key)) return -1;
    if (kind != P4_TABLE_SOURCE_PACKAGE) return kind == P4_TABLE_SOURCE_NATIVE ? 0 : -1;
    return p4_mod_save_read_u32(reader, &package_kind) || package_kind != origin->package_kind ||
        read_string_equal(reader, origin->package_id, sizeof origin->package_id) ||
        read_string_equal(reader, origin->package_version, sizeof origin->package_version) ? -1 : 0;
}
int p4_game_table_refs_writer_begin(const P4GameTableSources *sources,
    const P4ModSaveContext *transaction, P4ModSaveWriter *writer, P4GameTableRefWriter **out)
{
    if (!out || *out || !transaction || !writer || !p4_game_table_sources_is_frozen(sources))
        return failure(transaction, "Required table reference writer has no exact native source context.");
    size_t capacity = p4_game_table_sources_origin_count(sources);
    if (capacity > UINT32_MAX || capacity > (SIZE_MAX - sizeof(P4GameTableRefWriter)) / sizeof(uint32_t))
        return failure(transaction, "Required table source dictionary exceeds its bounded context.");
    P4GameTableRefWriter *context = allocate(sizeof *context + capacity * sizeof *context->indices);
    if (!context) return failure(transaction, "Cannot allocate required table reference dictionary.");
    context->sources = sources; context->transaction = transaction; context->writer = writer;
    context->capacity = capacity; context->count = 0;
    if (capacity) memset(context->indices, 0xff, capacity * sizeof *context->indices);
    if (p4_mod_save_write_u32(writer, P4_GAME_TABLE_REFS_WIRE_VERSION)) {
        p4_game_table_refs_writer_end(context); return P4_TABLE_INVALID;
    }
    *out = context;
    return P4_TABLE_OK;
}
void p4_game_table_refs_writer_end(P4GameTableRefWriter *context)
{
    if (!context) return;
    size_t bytes = sizeof *context + context->capacity * sizeof *context->indices;
    free(context); p4_packages_memory_release(bytes);
}
int p4_game_table_refs_write(P4GameTableRefWriter *context, const void *view, uint32_t domain, const uint32_t ids[5])
{
    if (!context) return P4_TABLE_INVALID;
    P4TableSourceResolved resolved;
    if (p4_game_table_sources_resolve_entry(context->sources, view, domain, ids, &resolved) ||
        resolved.origin_index >= context->capacity || !origin_current(&resolved))
        return failure(context->transaction, "Required table reference has no exact current entry/source.");
    uint32_t index = context->indices[resolved.origin_index], define = index == UINT32_MAX;
    if (define) {
        if (context->count >= context->capacity) return failure(context->transaction, "Required table origin dictionary is full.");
        if (resolved.provider && p4_mod_save_require_provider(context->writer, resolved.provider, 1)) return P4_TABLE_INVALID;
        index = context->count++;
        context->indices[resolved.origin_index] = index;
    }
    const P4TableSourceEntry *entry = resolved.entry;
    if (p4_mod_save_write_u32(context->writer, domain)) return P4_TABLE_INVALID;
    for (size_t i = 0; i < 5; ++i) if (p4_mod_save_write_u32(context->writer, ids[i])) return P4_TABLE_INVALID;
    if (p4_mod_save_write_u32(context->writer, entry->schema) || p4_mod_save_write_u32(context->writer, entry->revision) ||
        write_string(context->writer, entry->owner) || write_string(context->writer, entry->key) ||
        p4_mod_save_write_u32(context->writer, index) || p4_mod_save_write_u32(context->writer, define) ||
        (define && write_origin(context->writer, resolved.origin))) return P4_TABLE_INVALID;
    return P4_TABLE_OK;
}
int p4_game_table_refs_reader_begin(const P4GameTableSources *sources,
    const P4ModSaveContext *transaction, P4ModSaveReader *reader, P4GameTableRefReader **out)
{
    if (!out || *out || !transaction || !reader || !p4_game_table_sources_is_frozen(sources))
        return failure(transaction, "Required table reference reader has no exact native source context.");
    uint32_t version;
    if (p4_mod_save_read_u32(reader, &version) || version != P4_GAME_TABLE_REFS_WIRE_VERSION)
        return failure(transaction, "Unsupported required table reference wire version.");
    size_t capacity = p4_game_table_sources_origin_count(sources);
    if (capacity > UINT32_MAX || capacity > (SIZE_MAX - sizeof(P4GameTableRefReader)) / sizeof(uint32_t))
        return failure(transaction, "Required table source dictionary exceeds its bounded context.");
    size_t seen_bytes = capacity / 8u + (capacity % 8u != 0);
    size_t bytes = sizeof(P4GameTableRefReader) + capacity * sizeof(uint32_t);
    if (seen_bytes > SIZE_MAX - bytes)
        return failure(transaction, "Required table source dictionary exceeds its bounded context.");
    P4GameTableRefReader *context = allocate(bytes + seen_bytes);
    if (!context) return failure(transaction, "Cannot allocate required table reference dictionary.");
    context->sources = sources; context->transaction = transaction; context->reader = reader;
    context->capacity = capacity; context->count = 0;
    if (seen_bytes) memset(context->indices + capacity, 0, seen_bytes);
    *out = context;
    return P4_TABLE_OK;
}
void p4_game_table_refs_reader_end(P4GameTableRefReader *context)
{
    if (!context) return;
    size_t count = context->capacity;
    size_t bytes = sizeof *context + count * sizeof *context->indices + count / 8u + (count % 8u != 0);
    free(context); p4_packages_memory_release(bytes);
}
int p4_game_table_refs_read(P4GameTableRefReader *context, const void *view,
    uint32_t expected_domain, const P4TableSourceEntry **out)
{
    if (!context || !out) return P4_TABLE_INVALID;
    P4ModSaveReader *reader = context->reader;
    uint32_t domain, ids[5], schema, revision, index, define;
    if (p4_mod_save_read_u32(reader, &domain) || domain != expected_domain)
        return failure(context->transaction, "Required table reference domain differs.");
    for (size_t i = 0; i < 5; ++i) if (p4_mod_save_read_u32(reader, &ids[i])) return P4_TABLE_INVALID;
    P4TableSourceResolved resolved;
    if (p4_game_table_sources_resolve_entry(context->sources, view, domain, ids, &resolved) ||
        resolved.origin_index >= context->capacity || !origin_current(&resolved))
        return failure(context->transaction, "Required table reference is missing or its source is unselected.");
    const P4TableSourceEntry *entry = resolved.entry;
    if (p4_mod_save_read_u32(reader, &schema) || schema != entry->schema ||
        p4_mod_save_read_u32(reader, &revision) || revision != entry->revision ||
        read_string_equal(reader, entry->owner, sizeof entry->owner) || read_string_equal(reader, entry->key, sizeof entry->key) ||
        p4_mod_save_read_u32(reader, &index) || p4_mod_save_read_u32(reader, &define) || define > 1)
        return failure(context->transaction, "Required table entry identity/revision differs.");
    if (define) {
        unsigned char *seen = (unsigned char *)(context->indices + context->capacity);
        uint32_t current = resolved.origin_index;
        if (index != context->count || context->count >= context->capacity || (seen[current / 8u] & (1u << (current % 8u))))
            return failure(context->transaction, "Noncanonical required table origin definition.");
        if (read_origin_equal(reader, resolved.origin))
            return failure(context->transaction, "Required table native mode or package source identity/revision differs.");
        seen[current / 8u] |= (unsigned char)(1u << (current % 8u));
        context->indices[context->count++] = current;
    } else if (index >= context->count || context->indices[index] != resolved.origin_index)
        return failure(context->transaction, "Required table reference reused a different/undefined source origin.");
    *out = entry;
    return P4_TABLE_OK;
}
