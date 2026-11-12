/* CRI @UTF, CRILAYLA, CPK and Golden container readers (include/p4pc/golden_source.h).
 * Every length read from a file is bounded before use; these parsers run on
 * the player's own install, never on shipped data. */
#include "p4pc/golden_source.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static uint16_t be16(const uint8_t *p) { return (uint16_t)(p[0] << 8 | p[1]); }
static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static uint64_t be64(const uint8_t *p) { return (uint64_t)be32(p) << 32 | be32(p + 4); }
static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

static void reason_set(char *reason, size_t capacity, const char *text, const char *detail)
{
    if (reason && capacity) {
        snprintf(reason, capacity, "%s%s%s", text, detail ? ": " : "", detail ? detail : "");
    }
}

/* ---- @UTF ---------------------------------------------------------------- */

static size_t utf_value_size(uint8_t type)
{
    static const uint8_t sizes[12] = {1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 4, 8};
    return type < 12 ? sizes[type] : 0;
}

static void utf_unmask(uint8_t *packet, size_t size)
{
    uint32_t m = 0x655F;
    for (size_t i = 0; i < size; ++i) {
        packet[i] ^= (uint8_t)m;
        m *= 0x4115;
    }
}

int p4_golden_utf_open(P4GoldenUtf *utf, uint8_t *packet, size_t size)
{
    const uint8_t *d;
    uint32_t offset;

    memset(utf, 0, sizeof *utf);
    if (!packet || size < 8 + 24) return -1;
    if (memcmp(packet, "@UTF", 4)) {
        utf_unmask(packet, size);
        if (memcmp(packet, "@UTF", 4)) return -1;
    }
    utf->size = be32(packet + 4);
    if (utf->size < 24 || utf->size > size - 8) return -1;
    d = utf->table = packet + 8;
    utf->rows_offset = be16(d + 2);
    utf->strings_offset = be32(d + 4);
    utf->data_offset = be32(d + 8);
    utf->column_count = be16(d + 16);
    utf->row_width = be16(d + 18);
    utf->row_count = be32(d + 20);
    if (utf->column_count > P4_GOLDEN_UTF_MAX_COLUMNS || utf->strings_offset > utf->size ||
        utf->data_offset > utf->size || utf->rows_offset > utf->strings_offset ||
        (uint64_t)utf->row_width * utf->row_count > utf->strings_offset - utf->rows_offset) {
        return -1;
    }
    offset = 24;
    uint32_t row_cursor = 0;
    for (uint32_t c = 0; c < utf->column_count; ++c) {
        P4GoldenUtfColumn *column = &utf->columns[c];
        size_t width;
        if (offset + 5 > utf->rows_offset) return -1;
        column->storage = d[offset] & 0xF0;
        column->type = d[offset] & 0x0F;
        column->name = be32(d + offset + 1);
        if (column->name >= utf->size - utf->strings_offset) return -1;
        offset += 5;
        width = utf_value_size(column->type);
        if (!width) return -1;
        if (column->storage & 0x20) {
            if (offset + width > utf->rows_offset) return -1;
            column->value = offset;
            offset += (uint32_t)width;
        } else if (column->storage & 0x40) {
            column->value = row_cursor;
            row_cursor += (uint32_t)width;
            if (row_cursor > utf->row_width) return -1;
        }
    }
    return 0;
}

static const char *utf_string(const P4GoldenUtf *utf, uint32_t offset)
{
    const char *s;
    if (offset >= utf->size - utf->strings_offset) return NULL;
    s = (const char *)utf->table + utf->strings_offset + offset;
    return memchr(s, 0, utf->size - utf->strings_offset - offset) ? s : NULL;
}

static const P4GoldenUtfColumn *utf_column(const P4GoldenUtf *utf, const char *name)
{
    for (uint32_t c = 0; c < utf->column_count; ++c) {
        const char *s = utf_string(utf, utf->columns[c].name);
        if (s && !strcmp(s, name)) return &utf->columns[c];
    }
    return NULL;
}

/* The cell's bytes, or NULL for a zero-stored column (*zero set). */
static const uint8_t *utf_cell(const P4GoldenUtf *utf, uint32_t row, const P4GoldenUtfColumn *column, int *zero)
{
    *zero = 0;
    if (row >= utf->row_count) return NULL;
    if (column->storage & 0x20) return utf->table + column->value;
    if (column->storage & 0x40) return utf->table + utf->rows_offset + (size_t)row * utf->row_width + column->value;
    *zero = 1;
    return NULL;
}

int p4_golden_utf_u64(const P4GoldenUtf *utf, uint32_t row, const char *name, uint64_t *out)
{
    const P4GoldenUtfColumn *column = utf_column(utf, name);
    const uint8_t *cell;
    int zero;

    if (!column || column->type > 7) return -1;
    cell = utf_cell(utf, row, column, &zero);
    if (zero) { *out = 0; return row < utf->row_count ? 0 : -1; }
    if (!cell) return -1;
    switch (utf_value_size(column->type)) {
    case 1: *out = cell[0]; break;
    case 2: *out = be16(cell); break;
    case 4: *out = be32(cell); break;
    default: *out = be64(cell); break;
    }
    return 0;
}

int p4_golden_utf_str(const P4GoldenUtf *utf, uint32_t row, const char *name, const char **out)
{
    const P4GoldenUtfColumn *column = utf_column(utf, name);
    const uint8_t *cell;
    int zero;

    if (!column || column->type != 0x0A) return -1;
    cell = utf_cell(utf, row, column, &zero);
    if (zero) { *out = ""; return row < utf->row_count ? 0 : -1; }
    if (!cell || !(*out = utf_string(utf, be32(cell)))) return -1;
    return 0;
}

/* ---- CRILAYLA ------------------------------------------------------------- */

size_t p4_golden_crilayla_size(const uint8_t *src, size_t size)
{
    uint32_t body, prefix;
    if (!src || size < 0x10 + 0x100 || memcmp(src, "CRILAYLA", 8)) return 0;
    body = le32(src + 8);
    prefix = le32(src + 12);
    if ((uint64_t)prefix + 0x110 > size || (uint64_t)body + 0x100 > (uint64_t)SIZE_MAX) return 0;
    return (size_t)body + 0x100;
}

typedef struct LaylaBits {
    const uint8_t *src;
    size_t position; /* next byte to consume, counting down; 0x10 is the first stream byte */
    unsigned pool, left;
    int bad;
} LaylaBits;

static unsigned layla_bits(LaylaBits *b, unsigned count)
{
    unsigned value = 0, got = 0;
    while (got < count) {
        unsigned take;
        if (!b->left) {
            if (b->position < 0x10) { b->bad = 1; return 0; }
            b->pool = b->src[b->position--];
            b->left = 8;
        }
        take = b->left < count - got ? b->left : count - got;
        value = value << take | ((b->pool >> (b->left - take)) & ((1u << take) - 1));
        b->left -= take;
        got += take;
    }
    return value;
}

int p4_golden_crilayla(const uint8_t *src, size_t size, uint8_t *out, size_t capacity)
{
    static const unsigned widths[4] = {2, 3, 5, 8};
    size_t total = p4_golden_crilayla_size(src, size);
    size_t body, end, done = 0;
    LaylaBits bits;

    if (!total || capacity < total) return -1;
    body = total - 0x100;
    memcpy(out, src + le32(src + 12) + 0x10, 0x100);
    if (!body) return 0;
    bits.src = src;
    bits.position = size - 0x100 - 1;
    bits.pool = bits.left = 0;
    bits.bad = 0;
    end = 0x100 + body - 1;
    while (done < body) {
        if (layla_bits(&bits, 1)) {
            size_t reference = end - done + layla_bits(&bits, 13) + 3;
            size_t length = 3;
            unsigned level;
            for (level = 0; level < 4; ++level) {
                unsigned part = layla_bits(&bits, widths[level]);
                length += part;
                if (part != (1u << widths[level]) - 1) break;
            }
            if (level == 4) {
                unsigned part;
                do {
                    part = layla_bits(&bits, 8);
                    length += part;
                } while (part == 255 && !bits.bad);
            }
            if (bits.bad || reference > end || length > body - done) return -1;
            while (length--) {
                out[end - done] = out[reference--];
                ++done;
            }
        } else {
            unsigned byte = layla_bits(&bits, 8);
            if (bits.bad) return -1;
            out[end - done] = (uint8_t)byte;
            ++done;
        }
    }
    return 0;
}

/* ---- CPK -------------------------------------------------------------------- */

static int seek64(FILE *file, uint64_t offset)
{
#ifdef _WIN32
    return _fseeki64(file, (long long)offset, SEEK_SET);
#else
    return fseeko(file, (off_t)offset, SEEK_SET);
#endif
}

static int read_at(FILE *file, uint64_t offset, void *out, size_t size)
{
    return seek64(file, offset) || fread(out, 1, size, file) != size ? -1 : 0;
}

/* A chunk ("CPK " or "TOC "): 16-byte header with the packet size at +8. */
static uint8_t *read_chunk(FILE *file, uint64_t offset, const char *magic, size_t *size)
{
    uint8_t header[16];
    uint8_t *packet;
    uint32_t length;

    if (read_at(file, offset, header, sizeof header) || memcmp(header, magic, 4)) return NULL;
    length = le32(header + 8);
    if (length < 32 || length > (64u << 20) || !(packet = malloc(length))) return NULL;
    if (fread(packet, 1, length, file) != length) { free(packet); return NULL; }
    *size = length;
    return packet;
}

/* ASCII case-insensitive compare of at most n bytes (n SIZE_MAX: whole strings). */
static int ascii_ncase(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        int x = tolower((unsigned char)a[i]), y = tolower((unsigned char)b[i]);
        if (x != y) return x - y;
        if (!x) return 0;
    }
    return 0;
}

int p4_golden_cpk_open(P4GoldenCpk *cpk, const char *path, char *reason, size_t capacity)
{
    P4GoldenUtf header;
    uint8_t *packet = NULL;
    size_t size;
    uint64_t toc = 0, content = 0;

    memset(cpk, 0, sizeof *cpk);
    if (!(cpk->file = fopen(path, "rb"))) { reason_set(reason, capacity, "cannot open", path); return -1; }
    if (!(packet = read_chunk(cpk->file, 0, "CPK ", &size)) || p4_golden_utf_open(&header, packet, size) ||
        p4_golden_utf_u64(&header, 0, "TocOffset", &toc) || !toc) {
        free(packet);
        p4_golden_cpk_close(cpk);
        reason_set(reason, capacity, "not a CPK archive", path);
        return -1;
    }
    if (p4_golden_utf_u64(&header, 0, "ContentOffset", &content) || !content) content = toc;
    cpk->base = toc < content ? toc : content;
    free(packet);
    if (!(cpk->toc = read_chunk(cpk->file, toc, "TOC ", &size)) || p4_golden_utf_open(&cpk->utf, cpk->toc, size)) {
        p4_golden_cpk_close(cpk);
        reason_set(reason, capacity, "unreadable CPK table of contents", path);
        return -1;
    }
    return 0;
}

static int path_equal(const char *dir, const char *name, const char *path)
{
    size_t length = strlen(dir);
    if (length) {
        if (ascii_ncase(path, dir, length) || path[length] != '/') return 0;
        path += length + 1;
    }
    return !ascii_ncase(path, name, SIZE_MAX);
}

int p4_golden_cpk_find(const P4GoldenCpk *cpk, const char *path, P4GoldenCpkEntry *out)
{
    for (uint32_t row = 0; row < cpk->utf.row_count; ++row) {
        const char *dir, *name;
        uint64_t size, extract, offset;
        if (p4_golden_utf_str(&cpk->utf, row, "DirName", &dir) || p4_golden_utf_str(&cpk->utf, row, "FileName", &name)) return -1;
        if (!path_equal(dir, name, path)) continue;
        if (p4_golden_utf_u64(&cpk->utf, row, "FileSize", &size) ||
            p4_golden_utf_u64(&cpk->utf, row, "ExtractSize", &extract) ||
            p4_golden_utf_u64(&cpk->utf, row, "FileOffset", &offset) ||
            size > UINT32_MAX || extract > UINT32_MAX || offset > UINT64_MAX - cpk->base) {
            return -1;
        }
        out->offset = cpk->base + offset;
        out->size = (uint32_t)size;
        out->extract = (uint32_t)extract;
        return 0;
    }
    return 1;
}

int p4_golden_cpk_read(P4GoldenCpk *cpk, const P4GoldenCpkEntry *entry, uint8_t **out, size_t *size,
                       char *reason, size_t capacity)
{
    uint8_t *stored, *plain;
    size_t total;

    *out = NULL;
    *size = 0;
    if (!(stored = malloc(entry->size ? entry->size : 1))) { reason_set(reason, capacity, "out of memory", NULL); return -1; }
    if (read_at(cpk->file, entry->offset, stored, entry->size)) {
        free(stored);
        reason_set(reason, capacity, "short read from CPK", NULL);
        return -1;
    }
    if (entry->size == entry->extract || entry->size < 8 || memcmp(stored, "CRILAYLA", 8)) {
        *out = stored;
        *size = entry->size;
        return 0;
    }
    total = p4_golden_crilayla_size(stored, entry->size);
    if (total != entry->extract || !(plain = malloc(total))) {
        free(stored);
        reason_set(reason, capacity, total != entry->extract ? "CRILAYLA size mismatch" : "out of memory", NULL);
        return -1;
    }
    if (p4_golden_crilayla(stored, entry->size, plain, total)) {
        free(stored);
        free(plain);
        reason_set(reason, capacity, "corrupt CRILAYLA stream", NULL);
        return -1;
    }
    free(stored);
    *out = plain;
    *size = total;
    return 0;
}

void p4_golden_cpk_close(P4GoldenCpk *cpk)
{
    if (cpk->file) fclose(cpk->file);
    free(cpk->toc);
    memset(cpk, 0, sizeof *cpk);
}

/* ---- .bin containers and TBL spans ------------------------------------------- */

int p4_golden_bin_find(const uint8_t *bin, size_t size, const char *name,
                       const uint8_t **payload, size_t *payload_size)
{
    size_t offset = 0;
    while (offset + 256 <= size && bin[offset]) {
        char record[253];
        uint32_t length;
        memcpy(record, bin + offset, 252);
        record[252] = 0;
        length = le32(bin + offset + 252);
        offset += 256;
        if (length > size - offset) return -1;
        if (!ascii_ncase(record, name, SIZE_MAX)) {
            *payload = bin + offset;
            *payload_size = length;
            return 0;
        }
        offset += length;
        offset = (offset + 63) & ~(size_t)63;
    }
    return 1;
}

int p4_golden_tbl_spans(const uint8_t *tbl, size_t size, P4GoldenSpan *spans, size_t max)
{
    size_t offset = 0;
    int count = 0;
    while (offset + 4 <= size) {
        uint32_t length = le32(tbl + offset);
        if (!length && offset + 16 >= size) break;
        if (length > size - offset - 4 || (size_t)count == max) return -1;
        spans[count].bytes = tbl + offset + 4;
        spans[count].size = length;
        ++count;
        offset = (offset + 4 + length + 15) & ~(size_t)15;
    }
    return count;
}
