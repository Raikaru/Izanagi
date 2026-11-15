/* p4.data session edits in the save (include/p4pc/game_data_edit.h).
 *
 * The persisted records travel in the "p4.data"/"session" block by key, so a
 * save keeps naming the same rows whatever IDs another build gives them. A
 * load or a new game first undoes every session edit (commit, infallible),
 * then re-applies the records the loaded save carries (notify, after every
 * native commit and before the mods' own load callbacks can matter). */
#include "p4pc/game_data_edit.h"
#include "game_data_edit_internal.h"
#include "../memcard/mod_save_internal.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/mod_save.h"
#include "p4pc/packages.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SCHEMA = 1, RECORDS_MAX = 4096, TEXT_SIZE = 96 };

/* One persisted edit by key. learn: 0 a field write, otherwise the learnset
 * change + 1 with the skill's key and a level (or the replacement's key in
 * string). */
typedef struct Saved {
    char owner[64], key[TEXT_SIZE], path[96], string[TEXT_SIZE], skill[TEXT_SIZE];
    uint32_t kind, learn, text, argument;
    double number;
} Saved;
typedef struct Staged {
    size_t count;
    Saved records[];
} Staged;
static struct {
    P4ModSaveRegistration *registration;
    Staged *pending;
} save;

static const char *key_of(P4DataKind kind, uint32_t id)
{
    size_t index;
    return !p4_data_catalog_index(kind, id, &index) ? p4_data_catalog_key(kind, index) : NULL;
}
static int write_text(P4ModSaveWriter *writer, const char *text)
{
    const size_t size = strlen(text);
    return p4_mod_save_write_u32(writer, (uint32_t)size) || (size && p4_mod_save_write(writer, text, size));
}
static int read_text(P4ModSaveReader *reader, char *out, size_t capacity)
{
    uint32_t size;
    if (p4_mod_save_read_u32(reader, &size) || size >= capacity) return -1;
    if (size && p4_mod_save_read(reader, out, size)) return -1;
    out[size] = 0;
    return 0;
}
static int write_number(P4ModSaveWriter *writer, double number)
{
    uint64_t bits;
    memcpy(&bits, &number, sizeof bits);
    return p4_mod_save_write_u64(writer, bits);
}

static int encode(const P4ModSaveContext *context, P4ModSaveWriter *writer, void *user)
{
    (void)context; (void)user;
    const size_t count = p4_data_session_record_count();
    if (!count) return 1; /* nothing persisted: no block */
    for (size_t i = 0; i < count; ++i) {
        const P4DataSessionRecord *record = p4_data_session_record_at(i);
        int seen = 0;
        for (size_t j = 0; j < i && !seen; ++j) seen = !strcmp(p4_data_session_record_at(j)->owner, record->owner);
        if (!seen && p4_mod_save_require_mod(writer, record->owner, NULL, 0)) return -1;
    }
    if (p4_mod_save_write_u32(writer, (uint32_t)count)) return -1;
    for (size_t i = 0; i < count; ++i) {
        const P4DataSessionRecord *record = p4_data_session_record_at(i);
        const char *key = key_of(record->kind, record->id);
        const char *skill = record->learn ? key_of(P4_DATA_SKILL, record->skill) : "";
        const char *other = record->learn == P4_DATA_LEARN_REPLACE + 1u ? key_of(P4_DATA_SKILL, record->argument) : NULL;
        if (!key || !skill || (record->learn == P4_DATA_LEARN_REPLACE + 1u && !other)) return -1;
        if (write_text(writer, record->owner) || p4_mod_save_write_u32(writer, (uint32_t)record->kind) ||
            write_text(writer, key) || p4_mod_save_write_u32(writer, record->learn) || write_text(writer, record->path) ||
            p4_mod_save_write_u32(writer, (uint32_t)record->text) || write_number(writer, record->number) ||
            write_text(writer, other ? other : record->string) || write_text(writer, skill) ||
            p4_mod_save_write_u32(writer, other ? 0 : record->argument)) return -1;
    }
    return 0;
}
static int decode(const P4ModSaveContext *context, uint32_t schema, P4ModSaveReader *reader, void **staged, void *user)
{
    (void)user;
    *staged = NULL;
    if (!reader) return 0; /* a new game, or a save without session edits */
    uint32_t count;
    if (schema != SCHEMA || p4_mod_save_read_u32(reader, &count) || count > RECORDS_MAX) {
        if (context->error) snprintf(context->error, context->error_capacity, "p4.data session edits: unreadable block");
        return -1;
    }
    Staged *s = calloc(1, sizeof *s + (size_t)count * sizeof s->records[0]);
    if (!s) return -1;
    *staged = s;
    for (uint32_t i = 0; i < count; ++i) {
        Saved *r = s->records + i;
        uint64_t bits;
        if (read_text(reader, r->owner, sizeof r->owner) || p4_mod_save_read_u32(reader, &r->kind) ||
            read_text(reader, r->key, sizeof r->key) || p4_mod_save_read_u32(reader, &r->learn) ||
            read_text(reader, r->path, sizeof r->path) || p4_mod_save_read_u32(reader, &r->text) ||
            p4_mod_save_read_u64(reader, &bits) || read_text(reader, r->string, sizeof r->string) ||
            read_text(reader, r->skill, sizeof r->skill) || p4_mod_save_read_u32(reader, &r->argument) ||
            r->kind >= P4_DATA_KIND_COUNT || r->learn > P4_DATA_LEARN_REPLACE + 1u) {
            if (context->error) snprintf(context->error, context->error_capacity, "p4.data session edits: unreadable record");
            return -1;
        }
        memcpy(&r->number, &bits, sizeof bits);
        s->count = i + 1u;
    }
    return 0;
}
static void commit(void *staged, void *user)
{
    (void)user;
    p4_data_session_revert();
    p4_data_session_records_clear();
    free(save.pending);
    save.pending = staged;
}
static void discard(void *staged, void *user) { (void)user; free(staged); }

static int installed(const char *owner)
{
    for (size_t i = 0, count = p4_packages_count(); i < count; ++i) {
        const P4PackageInfo *info = p4_package_provider_info(p4_packages_provider(i));
        if (info && !strcmp(info->id, owner)) return 1;
    }
    return 0;
}
static int find(P4DataKind kind, const char *key, size_t *index, char *error, size_t capacity)
{
    P4DataRows rows;
    if (p4_data_catalog_rows(kind, &rows)) { snprintf(error, capacity, "the catalog is unavailable"); return -1; }
    return p4_data_lookup(&rows, key, index, error, capacity) ? -1 : 0;
}
static int reapply(const Saved *r, char *error, size_t capacity)
{
    size_t index;
    if (find((P4DataKind)r->kind, r->key, &index, error, capacity)) return -1;
    if (p4_data_session_begin(r->owner, 1, error, capacity)) return -1;
    int result;
    if (r->learn) {
        size_t skill, other = 0;
        const P4DataLearnOp op = (P4DataLearnOp)(r->learn - 1u);
        result = find(P4_DATA_SKILL, r->skill, &skill, error, capacity);
        if (!result && op == P4_DATA_LEARN_REPLACE) result = find(P4_DATA_SKILL, r->string, &other, error, capacity);
        if (!result)
            result = p4_data_edit_learn(index, op, p4_data_catalog_id(P4_DATA_SKILL, skill),
                op == P4_DATA_LEARN_REPLACE ? p4_data_catalog_id(P4_DATA_SKILL, other) : r->argument, error, capacity);
    } else {
        const P4DataEditValue value = {(int)r->text, r->number, r->string};
        result = p4_data_edit_set((P4DataKind)r->kind, index, r->path, &value, error, capacity);
    }
    return p4_data_session_end(!result, result ? NULL : error, result ? 0 : capacity) || result ? -1 : 0;
}
static void notify(void *user)
{
    (void)user;
    Staged *s = save.pending;
    save.pending = NULL;
    for (size_t i = 0; s && i < s->count; ++i) {
        const Saved *r = s->records + i;
        char error[768] = "";
        if (!installed(r->owner))
            fprintf(stderr, "p4pc: p4.data: the save's session edit of %s stays off: the mod is not enabled\n", r->owner);
        else if (reapply(r, error, sizeof error))
            fprintf(stderr, "p4pc: p4.data: the save's session edit of %s no longer applies: %s\n", r->owner, error);
    }
    free(s);
}

int p4_data_session_save_register(void)
{
    if (save.registration) return 0;
    const P4ModSaveProvider provider = {
        .struct_size = sizeof provider, .abi_version = P4_MOD_SAVE_ABI_VERSION, .owner = "p4.data", .key = "session",
        .schema_version = SCHEMA, .max_bytes = 1u << 20, .encode = encode, .decode = decode, .commit = commit,
        .discard = discard
    };
    if (p4_mod_save_register(&provider, &save.registration)) return -1;
    if (p4_mod_save_set_notify(save.registration, notify)) {
        p4_mod_save_unregister(save.registration);
        save.registration = NULL;
        return -1;
    }
    return 0;
}
