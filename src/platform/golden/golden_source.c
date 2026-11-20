/* Locates and reads the player's own Persona 4 Golden install (include/p4pc/golden_source.h).
 * Only data_e.cpk is opened: its init_free.bin holds the English battle
 * tables (SKILL/UNIT/PERSONA there are byte-identical to data.cpk's), the
 * item table and cmpTable.bin; its init.bin holds the community tables. */
#include "p4pc/golden_source.h"
#include "p4pc/content_digest.h"
#include <stdlib.h>
#include <string.h>

static const char *const table_paths[P4_GOLDEN_TABLE_COUNT] = {
    "battle/SKILL.TBL", "battle/UNIT.TBL", "battle/PERSONA.TBL",
    "battle/MSG.TBL", "battle/ENCOUNT.TBL", "init/itemtbl.bin", "init/cmpTable.bin", "event/cmm.bin",
};
static const char *const table_names[P4_GOLDEN_TABLE_COUNT] = {
    "SKILL.TBL", "UNIT.TBL", "PERSONA.TBL", "MSG.TBL", "ENCOUNT.TBL", "ITEMTBL.BIN", "CMPTABLE.BIN", "CMM.BIN",
};

/* The supported source: Steam app 1113000, build 10836033 (depot 1113001
 * manifest 4737529339202475845), English tables from data_e.cpk. Digests of
 * the extracted files, not their contents; any other build is refused until
 * its tables are measured and the translation is checked against them. */
static const char *const supported_digests[P4_GOLDEN_TABLE_COUNT] = {
    "2e4e770a61e1537441479e3a79f54d4aff1d091e92b6f0122cf0b79ed2bc9897",
    "f9cf91f9e40c2677e3be12d86f22a854adbdcf8b42a3d0d3dcc1845d8f8ff1a1",
    "0d4e105a9bdee6a868106a703c164ed98f92d601c7f887e606f4bced78286f13",
    "b59ad00f323140810533dddc61a9b3d75070607b0aa70e998c0c732bc5ab7909",
    "6391a03228554a7d40cba108a7ba24cf813d2a719e0bba2f2ef06dc6c726910e",
    "1bf1b589cb754d64099774ab158bb3d039806e8e9b41bc645d3ec97b23bcf533",
    "0fa99d455bbf394b669f69f92805e979dad2e232d449978e98cb09ff77525273",
    "b71dbfede9263ea54bb6a2fa7d9761c0fe03613a3f66d3abc6a2b35b7e07ab93",
};

const char *p4_golden_table_name(int table)
{
    return table >= 0 && table < P4_GOLDEN_TABLE_COUNT ? table_names[table] : "?";
}

static int file_exists(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    fclose(file);
    return 1;
}

static int install_at(const char *directory, char *out, size_t capacity)
{
    char probe[1100];
    if (!directory || !directory[0]) return 0;
    if ((size_t)snprintf(probe, sizeof probe, "%s/data_e.cpk", directory) >= sizeof probe || !file_exists(probe)) return 0;
    if ((size_t)snprintf(out, capacity, "%s", directory) >= capacity) return 0;
    return 1;
}

/* Every "path" value in a Steam libraryfolders.vdf, unescaped. */
static int scan_libraries(const char *vdf_path, char *out, size_t capacity)
{
    FILE *file = fopen(vdf_path, "rb");
    char line[1200];
    if (!file) return 0;
    while (fgets(line, sizeof line, file)) {
        char *key = strstr(line, "\"path\"");
        char *value, *end, library[1100], candidate[1200];
        size_t length = 0;
        if (!key || !(value = strchr(key + 6, '"'))) continue;
        for (end = value + 1; *end && *end != '"' && length + 1 < sizeof library; ++end) {
            if (*end == '\\' && end[1]) ++end;
            library[length++] = *end == '\\' ? '/' : *end;
        }
        library[length] = 0;
        snprintf(candidate, sizeof candidate, "%s/steamapps/common/Persona 4 Golden", library);
        if (install_at(candidate, out, capacity)) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

int p4_golden_locate(const char *configured, char *out, size_t capacity)
{
    static const char *const fixed[] = {
        "C:/Program Files (x86)/Steam", "C:/Program Files/Steam",
    };
    static const char *const home_relative[] = {
        ".steam/steam", ".local/share/Steam", ".var/app/com.valvesoftware.Steam/.local/share/Steam",
        "Library/Application Support/Steam",
    };
    char vdf[1200];
    const char *home = getenv("HOME");
    const char *x86 = getenv("ProgramFiles(x86)");

    if (configured && configured[0]) return install_at(configured, out, capacity) ? 0 : 1;
    if (x86) {
        snprintf(vdf, sizeof vdf, "%s/Steam/steamapps/libraryfolders.vdf", x86);
        if (scan_libraries(vdf, out, capacity)) return 0;
    }
    for (size_t i = 0; i < sizeof fixed / sizeof fixed[0]; ++i) {
        snprintf(vdf, sizeof vdf, "%s/steamapps/libraryfolders.vdf", fixed[i]);
        if (scan_libraries(vdf, out, capacity)) return 0;
    }
    for (size_t i = 0; home && i < sizeof home_relative / sizeof home_relative[0]; ++i) {
        snprintf(vdf, sizeof vdf, "%s/%s/steamapps/libraryfolders.vdf", home, home_relative[i]);
        if (scan_libraries(vdf, out, capacity)) return 0;
    }
    return 1;
}

void p4_golden_free(P4GoldenSource *source)
{
    for (int t = 0; t < P4_GOLDEN_TABLE_COUNT; ++t) free(source->tables[t].bytes);
    memset(source, 0, sizeof *source);
}

static void hex_digest(const uint8_t *bytes, size_t size, uint8_t digest[32], char hex[65])
{
    p4_content_digest(bytes, size, hex);
    for (int i = 0; i < 32; ++i) {
        unsigned value;
        sscanf(hex + 2 * i, "%2x", &value);
        digest[i] = (uint8_t)value;
    }
}

int p4_golden_load(P4GoldenSource *source, const char *directory, char *reason, size_t capacity)
{
    static const char *const containers[2] = {"init_free.bin", "init.bin"};
    char path[1100];
    P4GoldenCpk cpk;
    P4GoldenCpkEntry entry;
    uint8_t *bins[2] = {NULL, NULL};
    size_t bin_sizes[2] = {0, 0};
    int result = -1;

    memset(source, 0, sizeof *source);
    if ((size_t)snprintf(path, sizeof path, "%s/data_e.cpk", directory) >= sizeof path) {
        snprintf(reason, capacity, "install path too long");
        return -1;
    }
    if (!file_exists(path)) {
        snprintf(reason, capacity, "no data_e.cpk in %s", directory);
        return 1;
    }
    if (p4_golden_cpk_open(&cpk, path, reason, capacity)) return -1;
    for (int c = 0; c < 2; ++c) {
        if (p4_golden_cpk_find(&cpk, containers[c], &entry)) {
            snprintf(reason, capacity, "data_e.cpk has no %s", containers[c]);
            goto done;
        }
        if (p4_golden_cpk_read(&cpk, &entry, &bins[c], &bin_sizes[c], reason, capacity)) goto done;
    }
    snprintf(source->directory, sizeof source->directory, "%s", directory);
    for (int t = 0; t < P4_GOLDEN_TABLE_COUNT; ++t) {
        P4GoldenTable *table = &source->tables[t];
        const int c = t == P4_GOLDEN_CMM;
        const uint8_t *payload;
        size_t size;
        char hex[65];
        if (p4_golden_bin_find(bins[c], bin_sizes[c], table_paths[t], &payload, &size)) {
            snprintf(reason, capacity, "%s has no %s", containers[c], table_paths[t]);
            goto done;
        }
        if (!(table->bytes = malloc(size ? size : 1))) {
            snprintf(reason, capacity, "out of memory");
            goto done;
        }
        memcpy(table->bytes, payload, size);
        table->size = size;
        hex_digest(table->bytes, size, table->digest, hex);
        if (strcmp(hex, supported_digests[t])) {
            snprintf(reason, capacity, "unsupported Persona 4 Golden build: %s differs from Steam build 10836033", table_names[t]);
            goto done;
        }
        if (t == P4_GOLDEN_ITEM || t == P4_GOLDEN_CMPTABLE || t == P4_GOLDEN_CMM) {
            /* Not packed banks: the raw file is span 0. */
            table->spans[0].bytes = table->bytes;
            table->spans[0].size = size;
            table->span_count = 1;
        } else if ((table->span_count = p4_golden_tbl_spans(table->bytes, size, table->spans, 8)) < 1) {
            snprintf(reason, capacity, "%s has malformed spans", table_names[t]);
            goto done;
        }
    }
    result = 0;
done:
    free(bins[0]);
    free(bins[1]);
    p4_golden_cpk_close(&cpk);
    if (result) p4_golden_free(source);
    return result;
}
