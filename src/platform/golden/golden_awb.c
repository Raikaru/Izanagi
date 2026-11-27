/* Original AFS2 and memory-CPK bank framing; public facts in audio.md. */
#include "p4pc/golden_audio.h"
#include <stdlib.h>
#include <string.h>

static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t le32(const uint8_t *p) { return (uint32_t)le16(p) | (uint32_t)le16(p + 2) << 16; }
static uint64_t le64(const uint8_t *p) { return (uint64_t)le32(p) | (uint64_t)le32(p + 4) << 32; }
static uint64_t little(const uint8_t *p, unsigned width)
{
    return width == 2 ? le16(p) : width == 4 ? le32(p) : le64(p);
}
static int by_id(const void *a, const void *b)
{
    uint32_t x = ((const P4GoldenWave *)a)->id, y = ((const P4GoldenWave *)b)->id;
    return (x > y) - (x < y);
}
static int index_alloc(P4GoldenAwb *awb, uint64_t count)
{
    if (count > (1u << 20) || count > SIZE_MAX / sizeof *awb->waves) return -1;
    awb->count = (uint32_t)count;
    if (count && !(awb->waves = calloc((size_t)count, sizeof *awb->waves))) return -1;
    return 0;
}
static int afs2(P4GoldenAwb *awb, uint8_t *bytes, size_t size)
{
    unsigned ow = bytes[5], iw = le16(bytes + 6);
    uint32_t count = le32(bytes + 8), align = le16(bytes + 12);
    size_t offsets, tables;
    if ((bytes[4] != 1 && bytes[4] != 2) || (ow != 2 && ow != 4 && ow != 8) ||
        (iw != 2 && iw != 4) || !align || count > (1u << 20)) return -1;
    offsets = 16 + (size_t)count * iw;
    tables = offsets + ((size_t)count + 1) * ow;
    if (tables > size || index_alloc(awb, count)) return -1;
    awb->subkey = le16(bytes + 14);
    for (uint32_t i = 0; i < count; ++i) {
        uint64_t start = little(bytes + offsets + (size_t)i * ow, ow);
        uint64_t end = little(bytes + offsets + ((size_t)i + 1) * ow, ow);
        if (start < tables || start > size || end > size) return -1;
        start += (align - start % align) % align;
        if (start > end) return -1;
        awb->waves[i].id = (uint32_t)little(bytes + 16 + (size_t)i * iw, iw);
        awb->waves[i].payload = (P4GoldenSpan){bytes + (size_t)start, (size_t)(end - start)};
    }
    return 0;
}
static int chunk(uint8_t *bytes, size_t size, uint64_t offset, const char *magic, P4GoldenUtf *utf)
{
    uint64_t length;
    if (offset > size || size - (size_t)offset < 16) return -1;
    uint8_t *p = bytes + (size_t)offset;
    if (memcmp(p, magic, 4)) return -1;
    length = le64(p + 8);
    if (length > size - (size_t)offset - 16) return -1;
    return p4_golden_utf_open(utf, p + 16, (size_t)length);
}
static int cpk(P4GoldenAwb *awb, uint8_t *bytes, size_t size)
{
    P4GoldenUtf header, itoc, parts[2];
    uint64_t offset, content, align, counts[2], cursor;
    const char *const names[] = {"DataL", "DataH"};
    const char *const numbers[] = {"FilesL", "FilesH"};
    if (chunk(bytes, size, 0, "CPK ", &header) || header.row_count != 1 ||
        p4_golden_utf_u64(&header, 0, "ItocOffset", &offset) ||
        p4_golden_utf_u64(&header, 0, "ContentOffset", &content) ||
        p4_golden_utf_u64(&header, 0, "Align", &align) || !align || content > size ||
        chunk(bytes, size, offset, "ITOC", &itoc) || itoc.row_count != 1) return -1;
    memset(parts, 0, sizeof parts);
    for (unsigned part = 0; part < 2; ++part) {
        uint8_t *data;
        size_t length;
        if (p4_golden_utf_u64(&itoc, 0, numbers[part], &counts[part]) ||
            p4_golden_utf_blob(&itoc, 0, names[part], &data, &length)) return -1;
        if (!length) { if (counts[part]) return -1; continue; }
        if (p4_golden_utf_open(&parts[part], data, length) || parts[part].row_count != counts[part]) return -1;
    }
    if (counts[0] > (1u << 20) || counts[1] > (1u << 20) || index_alloc(awb, counts[0] + counts[1])) return -1;
    uint32_t next = 0;
    for (unsigned part = 0; part < 2; ++part) {
        for (uint32_t row = 0; row < parts[part].row_count; ++row) {
            uint64_t id, stored, extracted;
            if (p4_golden_utf_u64(&parts[part], row, "ID", &id) || id > UINT32_MAX ||
                p4_golden_utf_u64(&parts[part], row, "FileSize", &stored)) return -1;
            if (p4_golden_utf_u64(&parts[part], row, "ExtractSize", &extracted)) extracted = stored;
            /* Measured memory banks are uncompressed; never pass compressed bytes to a codec. */
            if (stored != extracted || stored > size) return -1;
            awb->waves[next].id = (uint32_t)id;
            awb->waves[next++].payload.size = (size_t)stored;
        }
    }
    qsort(awb->waves, awb->count, sizeof *awb->waves, by_id);
    cursor = content;
    for (uint32_t i = 0; i < awb->count; ++i) {
        size_t length = awb->waves[i].payload.size;
        if (cursor > size || length > size - (size_t)cursor) return -1;
        awb->waves[i].payload.bytes = bytes + (size_t)cursor;
        cursor += length;
        uint64_t pad = (align - cursor % align) % align;
        if (pad > UINT64_MAX - cursor) return -1;
        cursor += pad;
    }
    return 0;
}
int p4_golden_awb_open(P4GoldenAwb *awb, uint8_t *bytes, size_t size)
{
    int result;
    if (!awb) return -1;
    memset(awb, 0, sizeof *awb);
    if (!bytes || size < 16) return -1;
    if (!memcmp(bytes, "AFS2", 4)) result = afs2(awb, bytes, size);
    else if (!memcmp(bytes, "CPK ", 4)) result = cpk(awb, bytes, size);
    else return -1;
    if (!result) {
        if (!memcmp(bytes, "AFS2", 4) && awb->count > 1)
            qsort(awb->waves, awb->count, sizeof *awb->waves, by_id);
        for (uint32_t i = 1; i < awb->count; ++i)
            if (awb->waves[i - 1].id == awb->waves[i].id) result = -1;
    }
    if (result) p4_golden_awb_close(awb);
    return result;
}
void p4_golden_awb_close(P4GoldenAwb *awb)
{
    if (awb) { free(awb->waves); memset(awb, 0, sizeof *awb); }
}
const P4GoldenWave *p4_golden_awb_find(const P4GoldenAwb *awb, uint32_t id)
{
    size_t lo = 0, hi = awb->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (awb->waves[mid].id < id) lo = mid + 1;
        else hi = mid;
    }
    return lo < awb->count && awb->waves[lo].id == id ? &awb->waves[lo] : NULL;
}
