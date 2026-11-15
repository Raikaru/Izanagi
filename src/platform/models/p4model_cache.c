/* Cooked-model cache: <directory>/<source hash>.p4mc, written atomically, trusted only after
 * p4_model_cooked_check and a matching hash. A missing, stale or damaged file is simply a miss. */
#include "p4model.h"
#include "p4pc/os.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CACHE_FILE_MAX (256u * 1024u * 1024u)

static int cache_path(const char *directory, const char *source, const char *suffix, char *out, size_t capacity)
{
    if (!directory || !source || strlen(source) != 64) return -1;
    for (const char *p = source; *p; ++p)
        if (!((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f'))) return -1;
    int n = snprintf(out, capacity, "%s/%s%s", directory, source, suffix);
    return n > 0 && (size_t)n < capacity ? 0 : -1;
}

int p4_model_cache_load(const char *directory, const char *source, uint8_t **out, size_t *out_size)
{
    char path[1200];
    *out = NULL;
    *out_size = 0;
    if (cache_path(directory, source, ".p4mc", path, sizeof path)) return -1;
    FILE *stream = p4_os_fopen_rb(path);
    if (!stream) return -1;
    long size = -1;
    if (fseek(stream, 0, SEEK_END) == 0) size = ftell(stream);
    if (size <= 0 || (unsigned long)size > CACHE_FILE_MAX || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return -1;
    }
    uint8_t *blob = malloc((size_t)size);
    if (!blob || fread(blob, 1, (size_t)size, stream) != (size_t)size) {
        free(blob);
        fclose(stream);
        return -1;
    }
    fclose(stream);
    if (p4_model_cooked_check(blob, (size_t)size) || strcmp(p4_model_header(blob)->source, source)) {
        free(blob);
        return -1;
    }
    *out = blob;
    *out_size = (size_t)size;
    return 0;
}

int p4_model_cache_store(const char *directory, const uint8_t *blob, size_t size)
{
    char path[1200], temporary[1200];
    if (p4_model_cooked_check(blob, size)) return -1;
    const char *source = p4_model_header(blob)->source;
    if (cache_path(directory, source, ".p4mc", path, sizeof path) ||
        cache_path(directory, source, ".p4mc.tmp", temporary, sizeof temporary))
        return -1;
    if (p4_os_mkdir(directory) != 0 && errno != EEXIST) return -1;
    FILE *stream = fopen(temporary, "wb");
    if (!stream) return -1;
    int failed = fwrite(blob, 1, size, stream) != size;
    if (fclose(stream) != 0) failed = 1;
    if (failed || p4_os_replace_file(temporary, path) != 0) {
        remove(temporary);
        return -1;
    }
    return 0;
}
