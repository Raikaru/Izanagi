#include "compendium.h"
#include "p4pc/os.h"
#include "../../../third_party/yxml/yxml.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define XML_MAX 65536u
static unsigned char upper(unsigned char c) { return c >= 'a' && c <= 'z' ? (unsigned char)(c - 'a' + 'A') : c; }
static int equal(const char *a, const char *b)
{
    while (*a && upper((unsigned char)*a) == upper((unsigned char)*b)) { a++; b++; }
    return !*a && !*b;
}
static int join(char *out, size_t capacity, const char *directory, const char *name)
{
    int n = snprintf(out, capacity, "%s/%s", directory, name);
    return n < 0 || (size_t)n >= capacity ? -1 : 0;
}
static int label(const char *name)
{
    static const char *const names[] = {"DATA", "BTL", "BGM", "ENV", "ZZZZZ"};
    for (int i = 0; i < 5; i++) if (equal(name, names[i])) return i;
    return -1;
}

typedef struct Probe {
    const char *directory;
    unsigned labels;
    int failed;
    char xml[256], data[256];
} Probe;
static void probe_entry(const char *name, void *user)
{
    Probe *probe = user;
    char path[1200];
    if (!strcmp(name, ".") || !strcmp(name, "..") || probe->failed) return;
    if (join(path, sizeof path, probe->directory, name)) { probe->failed = 1; return; }
    int which = label(name);
    if (which >= 0) {
        if (!p4_os_is_dir_nofollow(path) || (probe->labels & (1u << which))) { probe->failed = 1; return; }
        probe->labels |= 1u << which;
        if (!which) snprintf(probe->data, sizeof probe->data, "%s", name);
    }
    if (equal(name, "Mod.xml")) {
        if (probe->xml[0] || !p4_os_is_file_nofollow(path)) { probe->failed = 1; return; }
        snprintf(probe->xml, sizeof probe->xml, "%s", name);
    }
}
static int probe(const char *directory, Probe *out)
{
    memset(out, 0, sizeof *out);
    out->directory = directory;
    return p4_os_list_dir(directory, probe_entry, out) || out->failed ? -1 : 0;
}
static const char *local_name(const char *name)
{
    const char *colon = strrchr(name, ':');
    return colon ? colon + 1 : name;
}
static int xml_fields(const char *path, char values[4][256], unsigned *seen)
{
    static const char *const fields[] = {"Id", "Title", "Version", "Game"};
    unsigned char stack[4096];
    size_t lengths[4] = {0}, bytes = 0;
    int depth = 0, field = -1, failed = 0, ch;
    yxml_t xml;
    yxml_init(&xml, stack, sizeof stack);
    FILE *file = p4_os_fopen_rb(path);
    if (!file) return -1;
    memset(values, 0, 4 * 256);
    *seen = 0;
    while ((ch = fgetc(file)) != EOF) {
        if (++bytes > XML_MAX) { failed = 1; break; }
        yxml_ret_t token = yxml_parse(&xml, ch);
        if (token < 0) { failed = 1; break; }
        if (token == YXML_ELEMSTART) {
            if (++depth > 32 || (depth == 1 && strcmp(local_name(xml.elem), "Mod"))) { failed = 1; break; }
            if (depth == 2) {
                field = -1;
                for (int i = 0; i < 4; i++) if (!strcmp(local_name(xml.elem), fields[i])) field = i;
                if (field >= 0) {
                    if (*seen & (1u << field)) { failed = 1; break; }
                    *seen |= 1u << field;
                }
            }
        } else if (token == YXML_CONTENT && field >= 0) {
            size_t n = strlen(xml.data);
            if (n >= sizeof values[0] - lengths[field]) { failed = 1; break; }
            for (size_t i = 0; i < n; i++) {
                unsigned char c = (unsigned char)xml.data[i];
                if (c == '\n' || c == '\r' || c == '\t') c = ' ';
                if (c < 32 || c == 127) { failed = 1; break; }
                values[field][lengths[field]++] = (char)c;
            }
            if (failed) break;
        } else if (token == YXML_ELEMEND) {
            if (depth == 2) field = -1;
            depth--;
        }
    }
    if (ferror(file) || yxml_eof(&xml) != YXML_OK || depth) failed = 1;
    fclose(file);
    if (failed) return -1;
    for (int i = 0; i < 4; i++) {
        size_t first = 0, last = lengths[i];
        while (first < last && values[i][first] == ' ') first++;
        while (last > first && values[i][last - 1] == ' ') last--;
        memmove(values[i], values[i] + first, last - first);
        values[i][last - first] = 0;
    }
    return 0;
}
static int guid(const char *text, char *out)
{
    if (strlen(text) != 36) return -1;
    int nonzero = 0;
    for (int i = 0; i < 36; i++) {
        unsigned char c = upper((unsigned char)text[i]);
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != '-') return -1;
        } else {
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) return -1;
            nonzero |= c != '0';
        }
        out[i] = c >= 'A' && c <= 'F' ? (char)(c + 'a' - 'A') : (char)c;
    }
    out[36] = 0;
    return nonzero ? 0 : 1;
}
static int copy(char *out, size_t capacity, const char *text)
{
    size_t n = strlen(text);
    if (!n || n >= capacity) return -1;
    for (size_t i = 0; i < n; i++) if ((unsigned char)text[i] < 32 || (unsigned char)text[i] == 127) return -1;
    memcpy(out, text, n + 1);
    return 0;
}
int p4_compendium_manifest(const char *directory, const char *folder,
    P4PackageInfo *info, char *files, size_t capacity)
{
    Probe root, wrapped;
    char child[1200], values[4][256] = {{0}};
    unsigned seen = 0;
    if (probe(directory, &root)) goto fail;
    const char *payload = directory;
    unsigned labels = root.labels;
    if (root.data[0]) {
        if (join(child, sizeof child, directory, root.data) || probe(child, &wrapped)) goto fail;
        if (wrapped.labels && root.labels == 1u) { payload = child; labels = wrapped.labels; }
    }
    if (!labels && !root.xml[0]) return 1;
    if (root.xml[0]) {
        char path[1200];
        if (join(path, sizeof path, directory, root.xml) || xml_fields(path, values, &seen)) goto fail;
        if ((seen & 8u) && values[3][0] && !equal(values[3], "Persona4")) {
            fprintf(stderr, "p4pc: mods: %s targets %s, not Persona 4 PS2\n", directory, values[3]);
            return -1;
        }
    }
    memset(info, 0, sizeof *info);
    uint64_t hash = UINT64_C(14695981039346656037);
    for (const unsigned char *p = (const unsigned char *)folder; *p; p++) { hash ^= upper(*p); hash *= UINT64_C(1099511628211); }
    snprintf(info->id, sizeof info->id, "compendium.%016llx", (unsigned long long)hash);
    if (values[0][0]) {
        char canonical[37];
        int result = guid(values[0], canonical);
        if (result < 0) goto fail;
        if (!result) snprintf(info->id, sizeof info->id, "compendium.%s", canonical);
    }
    if (copy(info->name, sizeof info->name, values[1][0] ? values[1] : folder) ||
        copy(info->version, sizeof info->version, values[2][0] ? values[2] : "unversioned") ||
        copy(info->directory, sizeof info->directory, directory) || copy(files, capacity, payload)) goto fail;
    return 0;
fail:
    fprintf(stderr, "p4pc: mods: invalid Compendium metadata/layout: %s\n", directory);
    return -1;
}

int p4_mod_code_name(const char *name)
{
    if (equal(name, "HOSTFS") || equal(name, "SLUS_217.82") || equal(name, "SYSTEM.CNF")) return 1;
    const char *extension = strrchr(name, '.');
    static const char *const code[] = {".pnach", ".elf", ".irx", ".exe", ".dll", ".so", ".dylib", ".bat", ".cmd", ".ps1", ".sh"};
    for (size_t i = 0; extension && i < sizeof code / sizeof code[0]; i++) if (equal(extension, code[i])) return 1;
    return 0;
}
int p4_mod_code_file(const char *path)
{
    unsigned char header[64];
    FILE *file = p4_os_fopen_rb(path);
    if (!file) return -1;
    size_t n = fread(header, 1, sizeof header, file);
    int result = ferror(file) ? -1 : n >= 4 && !memcmp(header, "\177ELF", 4);
    if (!result && n == sizeof header && header[0] == 'M' && header[1] == 'Z') {
        uint32_t offset = (uint32_t)header[60] | (uint32_t)header[61] << 8 | (uint32_t)header[62] << 16 | (uint32_t)header[63] << 24;
        if (offset >= 64 && fseeko(file, offset, SEEK_SET) == 0 && fread(header, 1, 4, file) == 4)
            result = !memcmp(header, "PE\0\0", 4);
    }
    fclose(file);
    return result;
}

typedef struct Safety { const char *directory, *skip; unsigned depth; int failed; } Safety;
static void safe_entry(const char *name, void *user)
{
    Safety *s = user;
    char path[1200];
    if (!strcmp(name, ".") || !strcmp(name, "..") || s->failed) return;
    if (join(path, sizeof path, s->directory, name)) { s->failed = 1; return; }
    if (s->skip && !strcmp(path, s->skip)) return;
    if (p4_mod_code_name(name)) {
        /* Compendium emits an empty prebuild.bat even for pure data packages. */
        int empty = 0;
        if (!s->depth && equal(name, "prebuild.bat") && p4_os_is_file_nofollow(path)) {
            FILE *file = p4_os_fopen_rb(path);
            if (file) { empty = fgetc(file) == EOF && !ferror(file); fclose(file); }
        }
        if (!empty) goto code;
        return;
    }
    if (p4_os_is_dir_nofollow(path)) {
        if (s->depth == 32) { s->failed = 1; return; }
        Safety child = {path, s->skip, s->depth + 1, 0};
        if (p4_os_list_dir(path, safe_entry, &child) || child.failed) s->failed = 1;
    } else if (!p4_os_is_file_nofollow(path)) {
        s->failed = 1;
    } else if (p4_mod_code_file(path) != 0) {
        goto code;
    }
    return;
code:
    fprintf(stderr, "p4pc: mods: unsupported executable/HostFS/cheat input %s; rewrite this feature as a Luau mod\n", path);
    s->failed = 1;
}
int p4_compendium_root_safe(const char *directory, const char *validated_payload)
{
    if (validated_payload && !strcmp(directory, validated_payload)) return 0;
    Safety safety = {directory, validated_payload, 0, 0};
    return p4_os_list_dir(directory, safe_entry, &safety) || safety.failed ? -1 : 0;
}
