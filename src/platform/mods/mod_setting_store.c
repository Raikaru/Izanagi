#include "p4pc/mod_setting_store.h"
#include "p4pc/os.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_LIMIT (256u * 1024u)
#define PATH_CAP 1300

typedef struct StoreValue { char key[P4_MOD_SETTING_KEY_MAX], value[P4_MOD_SETTING_VALUE_MAX]; } StoreValue;
typedef struct StoreFile {
    struct StoreFile *next;
    char id[64];
    StoreValue *values;
    size_t count;
} StoreFile;
typedef struct BootEntry {
    char id[64];
    P4ModSettingDecl *decls;
    size_t count;
    char (*values)[P4_MOD_SETTING_CHOICE_MAX];
} BootEntry;

static StoreFile *files;
static char *root;
static BootEntry *active, *staged;
static size_t active_count, staged_count;

static int fail(char *reason, size_t capacity, const char *format, ...)
{
    if (reason && capacity) {
        va_list args;
        va_start(args, format);
        vsnprintf(reason, capacity, format, args);
        va_end(args);
    }
    return -1;
}
static int text_valid(const char *text, size_t capacity)
{
    size_t size = text ? strlen(text) : 0;
    if (!size || size >= capacity || text[0] == ' ' || text[size - 1] == ' ') return 0;
    for (size_t i = 0; i < size; ++i)
        if ((unsigned char)text[i] < 32 || (unsigned char)text[i] > 126) return 0;
    return 1;
}
int p4_mod_setting_key_valid(const char *key)
{
    size_t size = key ? strlen(key) : 0;
    if (!size || size >= P4_MOD_SETTING_KEY_MAX) return 0;
    for (size_t i = 0; i < size; ++i) {
        char c = key[i];
        int alnum = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (!alnum && (!i || (c != '_' && c != '.' && c != '-'))) return 0;
    }
    return 1;
}
int p4_mod_setting_value_valid(const char *value) { return text_valid(value, P4_MOD_SETTING_VALUE_MAX); }
/* The registry's package id rule: lowercase, no leading or trailing dot. */
static int id_valid(const char *id)
{
    size_t size = id ? strlen(id) : 0;
    if (!size || size >= 64 || id[0] == '.' || id[size - 1] == '.') return 0;
    for (size_t i = 0; i < size; ++i) {
        char c = id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-')) return 0;
    }
    return 1;
}

static int decl_line_compare(const void *left, const void *right)
{
    const P4ModSettingDecl *a = left, *b = right;
    return (a->line > b->line) - (a->line < b->line);
}
static int copy_field(char *out, size_t capacity, const char *value)
{
    if (!text_valid(value, capacity)) return -1;
    strcpy(out, value);
    return 0;
}
static int parse_choices(P4ModSettingDecl *decl, const char *text)
{
    const char *cursor = text;
    decl->choice_count = 0;
    for (;;) {
        const char *end = strchr(cursor, '|');
        size_t size = end ? (size_t)(end - cursor) : strlen(cursor);
        while (size && cursor[0] == ' ') { ++cursor; --size; }
        while (size && cursor[size - 1] == ' ') --size;
        if (!size || size >= P4_MOD_SETTING_CHOICE_MAX || decl->choice_count == P4_MOD_SETTING_CHOICES) return -1;
        char *choice = decl->choices[decl->choice_count];
        memcpy(choice, cursor, size);
        choice[size] = 0;
        if (!text_valid(choice, P4_MOD_SETTING_CHOICE_MAX)) return -1;
        for (unsigned i = 0; i < decl->choice_count; ++i)
            if (!strcmp(decl->choices[i], choice)) return -1;
        ++decl->choice_count;
        if (!end) break;
        cursor = end + 1;
    }
    return decl->choice_count >= 2 ? 0 : -1;
}
static int parse_setting(const P4PackageField *fields, size_t count, P4ModSettingDecl *decl,
                         char *reason, size_t capacity)
{
    const char *kind = NULL, *def = NULL, *choices = NULL, *apply = NULL;
    const char *key = fields[0].argument;
    unsigned line = fields[0].line;
    memset(decl, 0, sizeof *decl);
    if (!p4_mod_setting_key_valid(key))
        return fail(reason, capacity, "Line%u: invalid setting key [setting %.60s]", line, key);
    strcpy(decl->key, key);
    for (size_t i = 0; i < count; ++i) {
        const P4PackageField *field = &fields[i];
        if (field->line < line) line = field->line;
        int bad = 0;
        if (!strcmp(field->key, "kind")) kind = field->value;
        else if (!strcmp(field->key, "default")) def = field->value;
        else if (!strcmp(field->key, "choices")) choices = field->value;
        else if (!strcmp(field->key, "apply")) apply = field->value;
        else if (!strcmp(field->key, "label")) bad = copy_field(decl->label, sizeof decl->label, field->value);
        else if (!strcmp(field->key, "help")) bad = copy_field(decl->help, sizeof decl->help, field->value);
        else if (!strcmp(field->key, "group")) bad = copy_field(decl->group, sizeof decl->group, field->value);
        else return fail(reason, capacity, "Line%u: setting %s has unknown field %.40s", field->line, key, field->key);
        if (bad)
            return fail(reason, capacity, "Line%u: setting %s %s must be bounded printable ASCII",
                        field->line, key, field->key);
    }
    decl->line = line;
    if (!decl->label[0]) return fail(reason, capacity, "Line%u: setting %s needs a label", line, key);
    if (kind && !strcmp(kind, "toggle")) {
        if (choices) return fail(reason, capacity, "Line%u: toggle %s cannot list choices", line, key);
        decl->kind = P4_MOD_SETTING_TOGGLE;
        decl->choice_count = 2;
        strcpy(decl->choices[0], "off");
        strcpy(decl->choices[1], "on");
    } else if (kind && !strcmp(kind, "choice")) {
        decl->kind = P4_MOD_SETTING_CHOICE;
        if (!choices || parse_choices(decl, choices))
            return fail(reason, capacity, "Line%u: choice %s needs 2..%u distinct choices of 1..%u bytes",
                        line, key, P4_MOD_SETTING_CHOICES, P4_MOD_SETTING_CHOICE_MAX - 1);
    } else {
        return fail(reason, capacity, "Line%u: setting %s kind must be toggle or choice", line, key);
    }
    decl->default_choice = 0;
    if (def) {
        unsigned i = 0;
        while (i < decl->choice_count && strcmp(decl->choices[i], def)) ++i;
        if (i == decl->choice_count)
            return fail(reason, capacity, "Line%u: setting %s default is not one of its choices", line, key);
        decl->default_choice = i;
    }
    if (apply && strcmp(apply, "live") && strcmp(apply, "restart"))
        return fail(reason, capacity, "Line%u: setting %s apply must be live or restart", line, key);
    decl->restart = apply && !strcmp(apply, "restart");
    return 0;
}
int p4_mod_setting_declarations(const P4PackageField *fields, size_t count,
                                P4ModSettingDecl **out, size_t *found, char *reason, size_t capacity)
{
    if (!out || !found || (count && !fields)) return fail(reason, capacity, "Invalid setting query");
    *out = NULL;
    *found = 0;
    size_t settings = 0;
    for (size_t i = 0; i < count; ++i)
        if (!strcmp(fields[i].section, "setting") &&
            (!settings || strcmp(fields[i - 1].section, "setting") || strcmp(fields[i - 1].argument, fields[i].argument)))
            ++settings;
    if (!settings) return 0;
    if (settings > P4_MOD_SETTINGS_PER_MOD)
        return fail(reason, capacity, "More than %u settings", P4_MOD_SETTINGS_PER_MOD);
    P4ModSettingDecl *decls = calloc(settings, sizeof *decls);
    if (!decls) return fail(reason, capacity, "Setting allocation failed");
    size_t made = 0;
    for (size_t first = 0; first < count;) {
        size_t last = first + 1;
        while (last < count && !strcmp(fields[first].section, fields[last].section) &&
               !strcmp(fields[first].argument, fields[last].argument)) ++last;
        if (!strcmp(fields[first].section, "setting") &&
            parse_setting(fields + first, last - first, &decls[made++], reason, capacity)) {
            free(decls);
            return -1;
        }
        first = last;
    }
    qsort(decls, made, sizeof *decls, decl_line_compare);
    *out = decls;
    *found = made;
    return 0;
}
const P4ModSettingDecl *p4_mod_setting_find(const P4ModSettingDecl *decls, size_t count, const char *key)
{
    for (size_t i = 0; key && i < count; ++i)
        if (!strcmp(decls[i].key, key)) return &decls[i];
    return NULL;
}
unsigned p4_mod_setting_choice(const P4ModSettingDecl *decl, const char *value)
{
    for (unsigned i = 0; value && i < decl->choice_count; ++i)
        if (!strcmp(decl->choices[i], value)) return i;
    return decl->default_choice;
}

static void files_clear(void)
{
    while (files) {
        StoreFile *next = files->next;
        free(files->values);
        free(files);
        files = next;
    }
}
int p4_mod_setting_store_root(const char *directory)
{
    char *copy = NULL;
    if ((!directory && !root) || (directory && root && !strcmp(directory, root))) return 0;
    if (directory) {
        size_t size = strlen(directory);
        if (!size || size >= PATH_CAP - 96 || !(copy = malloc(size + 1))) return -1;
        memcpy(copy, directory, size + 1);
    }
    files_clear();
    free(root);
    root = copy;
    return 0;
}
static int file_path(const char *id, const char *suffix, char out[PATH_CAP])
{
    int n = snprintf(out, PATH_CAP, "%s/mod-settings/%s.ini%s", root, id, suffix);
    return n > 0 && n < PATH_CAP ? 0 : -1;
}
static StoreValue *value_find(StoreFile *file, const char *key)
{
    for (size_t i = 0; i < file->count; ++i)
        if (!strcmp(file->values[i].key, key)) return &file->values[i];
    return NULL;
}
static char *trim(char *text)
{
    while (*text == ' ' || *text == '\t') ++text;
    size_t size = strlen(text);
    while (size && (text[size - 1] == ' ' || text[size - 1] == '\t' || text[size - 1] == '\r')) text[--size] = 0;
    return text;
}
static int parse_file(StoreFile *file, char *text)
{
    file->values = calloc(P4_MOD_SETTINGS_PER_MOD, sizeof *file->values);
    if (!file->values) return -1;
    unsigned line_number = 0;
    for (char *line = text; line && *line;) {
        char *next = strchr(line, '\n');
        if (next) *next++ = 0;
        ++line_number;
        char *s = trim(line), *eq = strchr(s, '=');
        if (*s && *s != '#' && *s != ';') {
            if (!eq) goto skip;
            *eq = 0;
            char *key = trim(s), *value = trim(eq + 1);
            if (!p4_mod_setting_key_valid(key) || !p4_mod_setting_value_valid(value) ||
                value_find(file, key) || file->count == P4_MOD_SETTINGS_PER_MOD) goto skip;
            strcpy(file->values[file->count].key, key);
            strcpy(file->values[file->count].value, value);
            ++file->count;
        }
        line = next;
        continue;
skip:
        fprintf(stderr, "p4pc: mod settings %s line%u ignored\n", file->id, line_number);
        line = next;
    }
    return 0;
}
static StoreFile *load(const char *id, char *reason, size_t capacity)
{
    if (!id_valid(id)) { fail(reason, capacity, "Invalid mod id"); return NULL; }
    for (StoreFile *file = files; file; file = file->next)
        if (!strcmp(file->id, id)) return file;
    StoreFile *file = calloc(1, sizeof *file);
    if (!file) { fail(reason, capacity, "Setting allocation failed"); return NULL; }
    strcpy(file->id, id);
    char *text = NULL;
    if (root) {
        char path[PATH_CAP];
        FILE *stream = NULL;
        if (file_path(id, "", path) || (!(stream = fopen(path, "rb")) && errno != ENOENT)) {
            free(file);
            fail(reason, capacity, "Cannot read the settings of %s", id);
            return NULL;
        }
        if (stream) {
            text = malloc(FILE_LIMIT + 1);
            size_t size = text ? fread(text, 1, FILE_LIMIT + 1, stream) : 0;
            int bad = !text || ferror(stream) || size > FILE_LIMIT || memchr(text, 0, size);
            fclose(stream);
            if (bad) {
                free(text);
                free(file);
                fail(reason, capacity, "Settings file of %s is unreadable or over 256 KiB", id);
                return NULL;
            }
            text[size] = 0;
        }
    }
    if (parse_file(file, text ? text : (char[]){0})) {
        free(text);
        free(file);
        fail(reason, capacity, "Setting allocation failed");
        return NULL;
    }
    free(text);
    file->next = files;
    files = file;
    return file;
}
int p4_mod_setting_stored(const char *mod_id, const char *key, char *out, size_t capacity,
                          char *reason, size_t reason_capacity)
{
    if (!p4_mod_setting_key_valid(key) || !out || !capacity)
        return fail(reason, reason_capacity, "Invalid setting key");
    StoreFile *file = load(mod_id, reason, reason_capacity);
    if (!file) return -1;
    StoreValue *value = value_find(file, key);
    if (!value) return 0;
    if (strlen(value->value) >= capacity) return fail(reason, reason_capacity, "Setting value exceeds buffer");
    strcpy(out, value->value);
    return 1;
}
static int write_file(const StoreFile *file, const StoreValue *values, size_t count, char *reason, size_t capacity)
{
    char directory[PATH_CAP], path[PATH_CAP], temporary[PATH_CAP];
    int n = snprintf(directory, sizeof directory, "%s/mod-settings", root);
    if (n <= 0 || n >= PATH_CAP || file_path(file->id, "", path) || file_path(file->id, ".tmp", temporary))
        return fail(reason, capacity, "Settings path is too long");
    if (p4_os_mkdir(directory) && errno != EEXIST)
        return fail(reason, capacity, "Cannot create the mod-settings folder");
    FILE *stream = fopen(temporary, "wb");
    if (!stream) return fail(reason, capacity, "Cannot write the settings of %s", file->id);
    int bad = fprintf(stream, "# Settings of mod %s, written by the game. Edit only while it is closed.\n",
                      file->id) < 0;
    for (size_t i = 0; i < count && !bad; ++i)
        bad = fprintf(stream, "%s = %s\n", values[i].key, values[i].value) < 0;
    if (fflush(stream)) bad = 1;
    if (fclose(stream)) bad = 1;
    if (bad || p4_os_replace_file(temporary, path)) {
        remove(temporary);
        return fail(reason, capacity, "Cannot write the settings of %s", file->id);
    }
    return 0;
}
int p4_mod_setting_store(const char *mod_id, const char *key, const char *value,
                         char *reason, size_t reason_capacity)
{
    if (!p4_mod_setting_key_valid(key)) return fail(reason, reason_capacity, "Invalid setting key");
    if (value && !p4_mod_setting_value_valid(value))
        return fail(reason, reason_capacity, "Setting values are 1..255 printable ASCII bytes");
    if (!root) return fail(reason, reason_capacity, "Settings are not persistent in this run");
    StoreFile *file = load(mod_id, reason, reason_capacity);
    if (!file) return -1;
    StoreValue *existing = value_find(file, key);
    if (!value && !existing) return 0;
    if (value && existing && !strcmp(existing->value, value)) return 0;
    if (value && !existing && file->count == P4_MOD_SETTINGS_PER_MOD)
        return fail(reason, reason_capacity, "A mod stores at most %u settings", P4_MOD_SETTINGS_PER_MOD);
    /* Build the complete next generation, publish it on disk, then in memory. */
    StoreValue *next = calloc(P4_MOD_SETTINGS_PER_MOD, sizeof *next);
    if (!next) return fail(reason, reason_capacity, "Setting allocation failed");
    size_t count = 0;
    for (size_t i = 0; i < file->count; ++i) {
        if (&file->values[i] == existing && !value) continue;
        next[count] = file->values[i];
        if (&file->values[i] == existing) strcpy(next[count].value, value);
        ++count;
    }
    if (value && !existing) {
        strcpy(next[count].key, key);
        strcpy(next[count].value, value);
        ++count;
    }
    if (write_file(file, next, count, reason, reason_capacity)) {
        free(next);
        return -1;
    }
    free(file->values);
    file->values = next;
    file->count = count;
    return 0;
}

static void boot_free(BootEntry *entries, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        free(entries[i].decls);
        free(entries[i].values);
    }
    free(entries);
}
int p4_mod_setting_boot(const P4PackageProvider *provider, const char *key, char *out, size_t capacity)
{
    const P4PackageInfo *info = provider ? p4_package_provider_info(provider) : NULL;
    if (!info || !key || !out || !capacity) return -1;
    /* Later handlers' prepare reads this admission's staged snapshot. */
    const BootEntry *entries = staged ? staged : active;
    size_t count = staged ? staged_count : active_count;
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(entries[i].id, info->id)) continue;
        for (size_t j = 0; j < entries[i].count; ++j) {
            if (strcmp(entries[i].decls[j].key, key)) continue;
            if (strlen(entries[i].values[j]) >= capacity) return -1;
            strcpy(out, entries[i].values[j]);
            return 1;
        }
        return 0;
    }
    return 0;
}
static void rollback(void *unused)
{
    (void)unused;
    boot_free(staged, staged_count);
    staged = NULL;
    staged_count = 0;
}
static int prepare(void *unused)
{
    (void)unused;
    rollback(NULL);
    size_t packages = p4_packages_count();
    if (!packages) return 0;
    staged = calloc(packages, sizeof *staged);
    if (!staged) return -1;
    for (size_t i = 0; i < packages; ++i) {
        const P4PackageProvider *provider = p4_packages_provider(i);
        const P4PackageInfo *info = p4_package_provider_info(provider);
        size_t field_count = 0, count = 0;
        const P4PackageField *fields = p4_package_provider_fields(provider, &field_count);
        P4ModSettingDecl *decls = NULL;
        char reason[256];
        if (!info) return -1;
        if (p4_mod_setting_declarations(fields, field_count, &decls, &count, reason, sizeof reason)) {
            p4_package_provider_refuse(provider, reason);
            return -1;
        }
        if (!count) continue;
        /* A pack may share a mod's id; settings files and boot values are mod-owned. */
        if (info->kind != P4_PACKAGE_MOD) {
            free(decls);
            p4_package_provider_refuse(provider, "[setting] sections are for mods, not engine packs");
            return -1;
        }
        if (info->api_major == 1 && info->api_minor < 2) {
            free(decls);
            p4_package_provider_refuse(provider, "[setting] sections require api=1.2");
            return -1;
        }
        BootEntry *entry = &staged[staged_count++];
        strcpy(entry->id, info->id);
        entry->decls = decls;
        entry->count = count;
        entry->values = calloc(count, sizeof *entry->values);
        if (!entry->values) return -1;
        for (size_t j = 0; j < count; ++j) {
            char stored[P4_MOD_SETTING_VALUE_MAX];
            int found = p4_mod_setting_stored(info->id, decls[j].key, stored, sizeof stored, reason, sizeof reason);
            if (found < 0) fprintf(stderr, "p4pc: mod settings %s: %s; using defaults\n", info->id, reason);
            strcpy(entry->values[j], decls[j].choices[p4_mod_setting_choice(&decls[j], found > 0 ? stored : NULL)]);
        }
    }
    return 0;
}
static void publish(void *unused)
{
    (void)unused;
    boot_free(active, active_count);
    active = staged;
    active_count = staged_count;
    staged = NULL;
    staged_count = 0;
}
static const char *const sections[] = {"setting", NULL};
const P4PackageHandler p4_mod_settings_boot_provider = {
    .name = "mod-settings", .sections = sections,
    .prepare = prepare, .publish = publish, .rollback = rollback,
    .boot_phase = P4_PACKAGE_BOOT_SCHEMA
};
