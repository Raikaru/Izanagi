#include "p4pc/game_table_inputs_use.h"
#include "p4pc/game_table_sources.h"
#include "game_table_inputs_internal.h"
#include "p4pc/mod_save.h"
#include "p4pc/package_boot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NATIVE_HISTORY_MAX 64u
#define NATIVE_KEY_MAX 511u
#define NATIVE_HISTORY_BYTES (4u + NATIVE_HISTORY_MAX * (52u + NATIVE_KEY_MAX))
enum { NATIVE_DEV_FILE = 1, NATIVE_COMPILED_MODE = 2 };
typedef struct NativeRecord {
    uint32_t kind, schema, revision;
    uint64_t size;
    uint8_t digest[32];
    char key[NATIVE_KEY_MAX + 1u];
} NativeRecord;
typedef struct NativeHistory { size_t count; NativeRecord records[]; } NativeHistory;
typedef struct NativeRecordView {
    const char *key;
    const uint8_t *digest;
    uint64_t size;
    uint32_t kind, schema, revision;
} NativeRecordView;
typedef struct NativeModeUse { const P4TableSourceOrigin *origin; unsigned used; } NativeModeUse;
struct P4GameTableNativeUses {
    P4GameTableInput *const *inputs;
    const P4GameTableSources *sources;
    size_t count, modes_count;
    P4ModSaveRegistration *registration;
    NativeHistory *history;
    NativeModeUse modes[]; /* Use bits, not a second origin/authority registry. */
};
static int failure(const P4ModSaveContext *context, const char *reason)
{
    if (context && context->error && context->error_capacity)
        snprintf(context->error, context->error_capacity, "%s", reason);
    return -1;
}
static void *allocate(size_t size)
{
    if (p4_packages_memory_reserve(size)) return NULL;
    void *pointer = malloc(size);
    if (!pointer) { p4_packages_memory_cancel(size); return NULL; }
    p4_packages_memory_commit(size);
    return pointer;
}
static void history_destroy(NativeHistory *history)
{
    if (!history) return;
    size_t size = sizeof *history + history->count * sizeof *history->records;
    free(history);
    p4_packages_memory_release(size);
}
static const uint8_t *content_digest(P4GameTableInput *input)
{
    if (!input->digest_valid) {
        /* The sealed content fingerprint is already SHA-256 of these exact
         * retained bytes. Decode its canonical lowercase hex instead of
         * hashing the input again when it is first used. */
        for (unsigned i = 0; i < 32; ++i) {
            unsigned hi = (unsigned char)input->fingerprint.hex[2u * i];
            unsigned lo = (unsigned char)input->fingerprint.hex[2u * i + 1u];
            hi = hi <= '9' ? hi - '0' : hi - 'a' + 10u;
            lo = lo <= '9' ? lo - '0' : lo - 'a' + 10u;
            input->digest[i] = (uint8_t)(hi << 4 | lo);
        }
        input->digest_valid = 1;
    }
    return input->digest;
}
static NativeRecordView record_view(const NativeRecord *record)
{
    return (NativeRecordView){record->key, record->kind == NATIVE_DEV_FILE ? record->digest : NULL,
        record->size, record->kind, record->schema, record->revision};
}
static int compare_views(const void *left, const void *right)
{
    const NativeRecordView *a = left, *b = right;
    if (a->kind != b->kind) return a->kind < b->kind ? -1 : 1;
    int order = strcmp(a->key, b->key);
    if (order) return order;
    if (a->schema != b->schema) return a->schema < b->schema ? -1 : 1;
    if (a->revision != b->revision) return a->revision < b->revision ? -1 : 1;
    if (a->kind == NATIVE_COMPILED_MODE) return 0;
    if (a->size != b->size) return a->size < b->size ? -1 : 1;
    return memcmp(a->digest, b->digest, 32);
}
static int encode(const P4ModSaveContext *context, P4ModSaveWriter *writer, void *user)
{
    const P4GameTableNativeUses *uses = user;
    NativeRecordView views[NATIVE_HISTORY_MAX * 2u + P4_TABLE_INPUT_COUNT];
    size_t count = 0;
    if (uses->history)
        for (size_t i = 0; i < uses->history->count; ++i) views[count++] = record_view(&uses->history->records[i]);
    for (size_t i = 0; i < uses->count; ++i) {
        P4GameTableInput *input = uses->inputs[i];
        if (!input->native_used) continue;
        const P4VfsSourceInfo *source = p4_game_table_input_native(input);
        if (!source || source->kind != P4_VFS_DEVELOPER || !input->digest_valid)
            return failure(context, "Native table use lost its authenticated developer origin.");
        views[count++] = (NativeRecordView){source->key, input->digest, input->size, NATIVE_DEV_FILE, 1, 0};
    }
    for (size_t i = 0; i < uses->modes_count; ++i) {
        if (!uses->modes[i].used) continue;
        const P4TableSourceOrigin *origin = uses->modes[i].origin;
        views[count++] = (NativeRecordView){origin->source_key, NULL, 0, NATIVE_COMPILED_MODE, origin->schema, origin->revision};
    }
    if (!count) return 1; /* Neither registration nor an unused source emits anything. */
    if (count > 1u) qsort(views, count, sizeof *views, compare_views);
    size_t unique = 0;
    for (size_t i = 0; i < count; ++i)
        if (!unique || compare_views(&views[i], &views[unique - 1u])) views[unique++] = views[i];
    if (unique > NATIVE_HISTORY_MAX)
        return failure(context, "Native table source history exceeds its bounded save block.");
    if (p4_mod_save_write_u32(writer, (uint32_t)unique)) return -1;
    for (size_t i = 0; i < unique; ++i) {
        const NativeRecordView *view = &views[i];
        size_t length = strlen(view->key);
        if (!length || length > NATIVE_KEY_MAX || p4_mod_save_write_u32(writer, view->kind) ||
            p4_mod_save_write_u32(writer, view->schema)) return -1;
        if (view->kind == NATIVE_COMPILED_MODE) {
            if (p4_mod_save_write_u32(writer, view->revision)) return -1;
        }
        if (p4_mod_save_write_u32(writer, (uint32_t)length)) return -1;
        if (view->kind == NATIVE_DEV_FILE &&
            (p4_mod_save_write_u64(writer, view->size) || p4_mod_save_write(writer, view->digest, 32))) return -1;
        if (p4_mod_save_write(writer, view->key, length)) return -1;
    }
    return 0;
}
static int matches_current(P4GameTableNativeUses *uses, const NativeRecord *record)
{
    if (record->kind == NATIVE_COMPILED_MODE) {
        for (size_t i = 0; i < uses->modes_count; ++i) {
            const P4TableSourceOrigin *origin = uses->modes[i].origin;
            if (!strcmp(origin->source_key, record->key) && origin->schema == record->schema &&
                origin->revision == record->revision) return 1;
        }
        return 0;
    }
    if (record->schema != 1) return 0;
    for (size_t i = 0; i < uses->count; ++i) {
        P4GameTableInput *input = uses->inputs[i];
        const P4VfsSourceInfo *source = p4_game_table_input_native(input);
        if (source && source->kind == P4_VFS_DEVELOPER && !strcmp(source->key, record->key) &&
            input->size == record->size && !memcmp(content_digest(input), record->digest, 32)) return 1;
    }
    return 0;
}
static int decode(const P4ModSaveContext *context, uint32_t schema, P4ModSaveReader *reader,
    void **staged, void *user)
{
    *staged = NULL;
    if (!reader) return schema ? -1 : 0; /* New Game clears history, never current process use. */
    uint32_t count;
    if (schema != 1 || p4_mod_save_read_u32(reader, &count) || !count || count > NATIVE_HISTORY_MAX ||
        count > p4_mod_save_remaining(reader) / 17u)
        return failure(context, "Invalid native table source history header.");
    NativeHistory *history = allocate(sizeof *history + count * sizeof *history->records);
    if (!history) return failure(context, "Cannot allocate bounded native table source history.");
    history->count = count;
    *staged = history; /* The codec owns partial staged data on failure too. */
    for (uint32_t i = 0; i < count; ++i) {
        NativeRecord *record = &history->records[i];
        uint32_t length;
        record->revision = 0; record->size = 0;
        if (p4_mod_save_read_u32(reader, &record->kind) ||
            (record->kind != NATIVE_DEV_FILE && record->kind != NATIVE_COMPILED_MODE) ||
            p4_mod_save_read_u32(reader, &record->schema) || !record->schema)
            return failure(context, "Invalid native table source history variant.");
        if (record->kind == NATIVE_COMPILED_MODE &&
            (p4_mod_save_read_u32(reader, &record->revision) || !record->revision))
            return failure(context, "Invalid compiled native table mode revision.");
        if (p4_mod_save_read_u32(reader, &length) || !length || length > NATIVE_KEY_MAX)
            return failure(context, "Invalid native table source history key length.");
        if (record->kind == NATIVE_DEV_FILE && (p4_mod_save_read_u64(reader, &record->size) || !record->size ||
            p4_mod_save_read(reader, record->digest, 32))) return failure(context, "Invalid native table content identity.");
        if (p4_mod_save_read(reader, record->key, length)) return failure(context, "Truncated native table source key.");
        if (memchr(record->key, 0, length))
            return failure(context, "Embedded NUL in native table source history key.");
        record->key[length] = 0;
        if (i) {
            NativeRecordView previous = record_view(&history->records[i - 1u]), current = record_view(record);
            if (compare_views(&previous, &current) >= 0) return failure(context, "Noncanonical native table source history.");
        }
    }
    if (p4_mod_save_remaining(reader)) return failure(context, "Trailing native table source history bytes.");
    P4GameTableNativeUses *uses = user;
    for (uint32_t i = 0; i < count; ++i) {
        const NativeRecord *record = &history->records[i];
        if (matches_current(uses, record)) continue;
        char message[768];
        if (record->kind == NATIVE_COMPILED_MODE)
            snprintf(message, sizeof message,
                "Historical compiled-native table mode is unavailable or differs: %s (schema %u, revision %u). This history does not authorize expanded state references.",
                record->key, (unsigned)record->schema, (unsigned)record->revision);
        else
            snprintf(message, sizeof message,
                "Historical developer table source is unavailable or differs: %s (content schema %u, %llu bytes, SHA256 %02x%02x%02x%02x...). This history does not authorize expanded state references.",
                record->key, (unsigned)record->schema, (unsigned long long)record->size,
                (unsigned)record->digest[0], (unsigned)record->digest[1], (unsigned)record->digest[2], (unsigned)record->digest[3]);
        if (p4_mod_save_warn(context, message)) return -1;
    }
    return 0;
}
static void commit(void *staged, void *user)
{
    P4GameTableNativeUses *uses = user;
    history_destroy(uses->history); uses->history = staged;
}
static void discard(void *staged, void *user)
{ (void)user; history_destroy(staged); }
int p4_game_table_native_uses_prepare(P4GameTableInput *const *inputs, size_t count,
    const P4GameTableSources *sources, P4GameTableNativeUses **out)
{
    if (!out || *out || (!count && !sources) || (count && !inputs) || count > P4_TABLE_INPUT_COUNT ||
        (sources && !p4_game_table_sources_is_sealed(sources))) return -1;
    for (size_t i = 0; i < count; ++i) {
        if (!inputs[i] || !inputs[i]->sealed || inputs[i]->native_uses) return -1;
        for (size_t previous = 0; previous < i; ++previous) if (inputs[previous] == inputs[i]) return -1;
    }
    size_t modes = p4_game_table_sources_native_mode_count(sources);
    if (modes > NATIVE_HISTORY_MAX) return -1;
    size_t bytes = sizeof(P4GameTableNativeUses) + modes * sizeof(NativeModeUse);
    P4GameTableNativeUses *uses = allocate(bytes);
    if (!uses) return -1;
    uses->inputs = inputs; uses->sources = sources; uses->count = count; uses->modes_count = modes;
    uses->registration = NULL; uses->history = NULL;
    size_t cursor = 0;
    for (size_t i = 0; i < modes; ++i)
        uses->modes[i] = (NativeModeUse){p4_game_table_sources_native_mode_next(sources, &cursor), 0};
    P4ModSaveProvider provider = {
        sizeof provider, P4_MOD_SAVE_ABI_VERSION, "p4pc", "native-table-use", 1, 0,
        NATIVE_HISTORY_BYTES, uses, encode, decode, NULL, commit, discard
    };
    if (p4_mod_save_register(&provider, &uses->registration)) {
        free(uses); p4_packages_memory_release(bytes); return -1;
    }
    for (size_t i = 0; i < count; ++i) inputs[i]->native_uses = uses;
    *out = uses;
    return 0;
}
int p4_game_table_native_uses_destroy(P4GameTableNativeUses *uses)
{
    if (!uses) return 0;
    if (p4_mod_save_unregister(uses->registration)) return -1;
    for (size_t i = 0; i < uses->count; ++i) uses->inputs[i]->native_uses = NULL;
    history_destroy(uses->history);
    size_t bytes = sizeof *uses + uses->modes_count * sizeof *uses->modes;
    free(uses); p4_packages_memory_release(bytes);
    return 0;
}
static void package_use(const P4PackageProvider *provider)
{
    if (!p4_package_provider_selected(provider))
        p4_package_boot_runtime_failed("Native table semantic use has a retired or unselected source binding.");
    if (p4_mod_save_touch(provider))
        p4_package_boot_source_failed(provider, "Native table semantic use cannot record its actual selected provider.");
}
int p4_game_table_sources_semantic_use(const void *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    P4GameTableNativeUses *uses = context;
    P4TableSourceResolved resolved;
    if (!uses || !uses->sources || p4_game_table_sources_resolve_entry(uses->sources, view, domain, ids, &resolved))
        p4_package_boot_runtime_failed("Native table semantic use has no matching immutable entry-origin binding.");
    if (resolved.origin->kind == P4_TABLE_SOURCE_PACKAGE) package_use(resolved.provider);
    else {
        size_t index = 0;
        while (index < uses->modes_count && uses->modes[index].origin != resolved.origin) ++index;
        if (index == uses->modes_count)
            p4_package_boot_runtime_failed("Native table use has no issued compiled-mode origin in its owning source context.");
        uses->modes[index].used = 1;
    }
    return 0;
}
int p4_game_table_input_semantic_use(const void *identity, void *immutable_context)
{
    P4GameTableInput *input = immutable_context;
    if (!p4_game_table_input_owns_span(input, identity))
        p4_package_boot_runtime_failed("Native table use has no matching immutable prepared source span.");
    const P4PackageProvider *provider = p4_game_table_input_provider(input);
    if (provider) package_use(provider);
    else {
        const P4VfsSourceInfo *native = p4_game_table_input_native(input);
        if (!native || (native->kind != P4_VFS_DISC && native->kind != P4_VFS_DEVELOPER) || !native->file.size)
            p4_package_boot_runtime_failed("Native table use has no authenticated disc/developer source origin.");
        if (native->kind == P4_VFS_DEVELOPER) {
            if (!input->native_uses)
                p4_package_boot_runtime_failed("Developer table use has no registered owning native provenance context.");
            content_digest(input); input->native_used = 1;
        }
    }
    return 0;
}
