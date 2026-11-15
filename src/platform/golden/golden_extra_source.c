/* Pinned English battle and camp inputs from the owner's Golden install. */
#include "p4pc/golden_extra_source.h"
#include "p4pc/content_digest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const P4GoldenExtraIdentity sources[P4_GOLDEN_EXTRA_COUNT] = {
    {"battle/EFFECT.TBL", "3437b24206d799b61f936a0c61977f05020e4ca5e0710c283aa5bfce4e56dcf7"},
    {"battle/MODEL.TBL", "1267c2068f376f275f06f8d013449a287b2852919e8c3b7b3de9263c87c630c4"},
    {"battle/AICALC.TBL", "4c246790ffdd94caa631048f04062a36aa0c96fa47b110a78e56dcf3b9547fc7"},
    {"init/cmpTable.bin", "0fa99d455bbf394b669f69f92805e979dad2e232d449978e98cb09ff77525273"},
};
const P4GoldenExtraIdentity *p4_golden_extra_identities(size_t *count)
{
    if (count) *count = P4_GOLDEN_EXTRA_COUNT;
    return sources;
}
void p4_golden_extra_free(P4GoldenExtraSource *source)
{
    if (!source) return;
    for (unsigned i = 0; i < P4_GOLDEN_EXTRA_COUNT; ++i) free(source->tables[i].bytes);
    memset(source, 0, sizeof *source);
}
int p4_golden_extra_load(P4GoldenExtraSource *out, const char *directory, char *reason, size_t capacity)
{
    static const int expected_spans[] = {1, 3, 11};
    P4GoldenExtraSource built = {0};
    P4GoldenCpk cpk = {0};
    P4GoldenCpkEntry entry;
    uint8_t *bin = NULL;
    size_t bin_size;
    char path[1100], digest[65], scratch[160];
    char *why = reason && capacity ? reason : scratch;
    size_t room = reason && capacity ? capacity : sizeof scratch;
    int result = -1;
    if (!out || !directory || (size_t)snprintf(path, sizeof path, "%s/data_e.cpk", directory) >= sizeof path) {
        snprintf(why, room, "invalid extra table source path or output"); return -1;
    }
    if (p4_golden_cpk_open(&cpk, path, why, room)) return -1;
    if (p4_golden_cpk_find(&cpk, "init_free.bin", &entry) ||
        p4_golden_cpk_read(&cpk, &entry, &bin, &bin_size, why, room)) {
        snprintf(why, room, "cannot read data_e.cpk::init_free.bin"); goto done;
    }
    for (unsigned i = 0; i < P4_GOLDEN_EXTRA_COUNT; ++i) {
        const uint8_t *data;
        size_t size;
        P4GoldenExtraTable *table = &built.tables[i];
        if (p4_golden_bin_find(bin, bin_size, sources[i].member, &data, &size)) {
            snprintf(why, room, "missing Golden source %s", sources[i].member); goto done;
        }
        p4_content_digest(data, size, digest);
        if (strcmp(digest, sources[i].sha256)) {
            snprintf(why, room, "unsupported Golden source %s", sources[i].member); goto done;
        }
        table->bytes = malloc(size ? size : 1);
        if (!table->bytes) { snprintf(why, room, "out of memory"); goto done; }
        memcpy(table->bytes, data, size); table->size = size;
        if (i != P4_GOLDEN_EXTRA_CMP) {
            table->span_count = p4_golden_tbl_spans(table->bytes, size, table->spans, 12);
            if (table->span_count != expected_spans[i]) {
                snprintf(why, room, "malformed Golden source %s", sources[i].member); goto done;
            }
        }
    }
    *out = built;
    memset(&built, 0, sizeof built);
    if (reason && capacity) reason[0] = 0;
    result = 0;
done:
    free(bin);
    p4_golden_cpk_close(&cpk);
    p4_golden_extra_free(&built);
    return result;
}
