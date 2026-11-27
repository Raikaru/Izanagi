/* Golden AWB voice adapter. ADX implementation remains src/movie/adx_dec.c. */
#include "p4pc/golden_audio.h"
#include "../../movie/adx_dec.h"
#include <limits.h>
#include <stdlib.h>

struct P4GoldenAdx {
    P4GoldenAdxInfo info;
    P4Adx *decoder;
    const uint8_t *bytes;
    size_t position, end;
    uint32_t played;
    int failed;
};
static uint16_t be16(const uint8_t *p) { return (uint16_t)(p[0] << 8 | p[1]); }
static uint32_t be32(const uint8_t *p) { return (uint32_t)be16(p) << 16 | be16(p + 2); }
int p4_golden_adx_open(P4GoldenAdx **out, const uint8_t *bytes, size_t size)
{
    if (!out) return -1;
    *out = NULL;
    if (!bytes || size < 20 || be16(bytes) != 0x8000 || bytes[4] != 3 ||
        bytes[5] != 18 || bytes[6] != 4 || bytes[7] < 1 || bytes[7] > 2 || bytes[19]) return -1;
    uint32_t header = (uint32_t)be16(bytes + 2) + 4;
    P4GoldenAdxInfo info = {be32(bytes + 8), bytes[7], be32(bytes + 12)};
    uint64_t body = (((uint64_t)info.frames + 31) / 32) * 18 * info.channels;
    if (header < 20 || header > size || !info.rate || info.rate > 384000 || !info.frames || body > size - header) return -1;
    P4GoldenAdx *a = calloc(1, sizeof *a);
    if (!a) return -1;
    a->decoder = p4adx_open();
    if (!a->decoder) { free(a); return -1; }
    a->info = info;
    a->bytes = bytes;
    a->position = header;
    a->end = header + (size_t)body;
    p4adx_set_limit(a->decoder, 2048);
    if (p4adx_feed(a->decoder, bytes, (int)header) ||
        p4adx_sample_rate(a->decoder) != (int)info.rate || p4adx_channels(a->decoder) != (int)info.channels) {
        p4_golden_adx_close(a); return -1;
    }
    *out = a;
    return 0;
}
const P4GoldenAdxInfo *p4_golden_adx_format(const P4GoldenAdx *a) { return a ? &a->info : NULL; }
int p4_golden_adx_pull(P4GoldenAdx *a, int16_t *pcm, uint32_t capacity)
{
    if (!a || !pcm || capacity > INT_MAX || a->failed) return -1;
    uint32_t done = 0;
    while (done < capacity && a->played < a->info.frames) {
        if (!p4adx_available(a->decoder)) {
            size_t take = a->end - a->position;
            if (take > 1024) take = 1024;
            if (!take || p4adx_feed(a->decoder, a->bytes + a->position, (int)take)) { a->failed = 1; return -1; }
            a->position += take;
            if (!p4adx_available(a->decoder)) continue;
        }
        uint32_t take = capacity - done;
        if (take > a->info.frames - a->played) take = a->info.frames - a->played;
        int got = p4adx_pull(a->decoder, pcm + (size_t)done * a->info.channels, (int)take);
        if (got <= 0) { a->failed = 1; return -1; }
        done += (uint32_t)got;
        a->played += (uint32_t)got;
    }
    return (int)done;
}
void p4_golden_adx_close(P4GoldenAdx *a)
{
    if (a) { p4adx_close(a->decoder); free(a); }
}
