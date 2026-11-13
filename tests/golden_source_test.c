/* Golden install reader and translation (include/p4pc/golden_source.h,
 * golden_tables.h) on synthetic fixtures only: no Golden or retail bytes. */
#include "p4pc/golden_source.h"
#include "p4pc/golden_tables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "golden_source:%d: %s\n", __LINE__, #x); return 1; \
} } while (0)

static void be16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void be32(uint8_t *p, uint32_t v) { be16(p, v >> 16); be16(p + 2, v); }
static void be64(uint8_t *p, uint64_t v) { be32(p, (uint32_t)(v >> 32)); be32(p + 4, (uint32_t)v); }
static void le16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void le32(uint8_t *p, uint32_t v) { le16(p, v); le16(p + 2, v >> 16); }

/* ---- @UTF builder: columns of u32/u64/string per row, plus one u16 constant ---- */

typedef struct UtfCol { const char *name; uint8_t type; } UtfCol;

/* rows x cols cells; strings give their text in strs[row][col], numbers in nums. */
static size_t utf_build(uint8_t *out, const UtfCol *cols, int ncols, int nrows,
                        const char *const *strs, const uint64_t *nums, uint16_t constant)
{
    uint8_t *d = out + 8;
    char pool[2048] = "<NULL>";
    size_t pool_len = 7, row_width = 0, schema = 24, rows_offset, strings_offset;

    memset(out, 0, 8192);
    for (int c = 0; c < ncols; ++c) {
        row_width += cols[c].type == 0x0A ? 4 : cols[c].type == 6 ? 8 : 4;
        schema += 5;
    }
    schema += 5 + 2; /* the constant column and its value */
    rows_offset = schema;
    strings_offset = rows_offset + row_width * (size_t)nrows;
#define ADD_STRING(s, at) do { size_t n_ = strlen(s) + 1; memcpy(pool + pool_len, s, n_); be32(at, (uint32_t)pool_len); pool_len += n_; } while (0)
    size_t offset = 24;
    for (int c = 0; c < ncols; ++c) {
        d[offset] = (uint8_t)(0x50 | cols[c].type);
        ADD_STRING(cols[c].name, d + offset + 1);
        offset += 5;
    }
    d[offset] = 0x30 | 2;
    ADD_STRING("Const", d + offset + 1);
    be16(d + offset + 5, constant);
    for (int r = 0; r < nrows; ++r) {
        uint8_t *row = d + rows_offset + (size_t)r * row_width;
        for (int c = 0; c < ncols; ++c) {
            if (cols[c].type == 0x0A) { ADD_STRING(strs[r * ncols + c], row); row += 4; }
            else if (cols[c].type == 6) { be64(row, nums[r * ncols + c]); row += 8; }
            else { be32(row, (uint32_t)nums[r * ncols + c]); row += 4; }
        }
    }
#undef ADD_STRING
    memcpy(d + strings_offset, pool, pool_len);
    be16(d + 2, (uint32_t)rows_offset);
    be32(d + 4, (uint32_t)strings_offset);
    be32(d + 8, (uint32_t)(strings_offset + pool_len));
    be32(d + 12, 0);
    be16(d + 16, (uint32_t)ncols + 1);
    be16(d + 18, (uint32_t)row_width);
    be32(d + 20, (uint32_t)nrows);
    memcpy(out, "@UTF", 4);
    be32(out + 4, (uint32_t)(strings_offset + pool_len));
    return 8 + strings_offset + pool_len;
}

static int test_utf(void)
{
    static uint8_t packet[8192];
    static const UtfCol cols[] = {{"Name", 0x0A}, {"Size", 4}};
    const char *strs[] = {"alpha", NULL, "beta", NULL};
    const uint64_t nums[] = {0, 11, 0, 22};
    P4GoldenUtf utf;
    uint64_t value;
    const char *text;
    size_t size = utf_build(packet, cols, 2, 2, strs, nums, 0x1234);
    uint32_t m = 0x655F;

    CHECK(!p4_golden_utf_open(&utf, packet, size));
    CHECK(utf.row_count == 2);
    CHECK(!p4_golden_utf_str(&utf, 1, "Name", &text) && !strcmp(text, "beta"));
    CHECK(!p4_golden_utf_u64(&utf, 0, "Size", &value) && value == 11);
    CHECK(!p4_golden_utf_u64(&utf, 1, "Const", &value) && value == 0x1234);
    CHECK(p4_golden_utf_u64(&utf, 2, "Size", &value) == -1);
    CHECK(p4_golden_utf_u64(&utf, 0, "Missing", &value) == -1);
    CHECK(p4_golden_utf_u64(&utf, 0, "Name", &value) == -1);
    /* An obfuscated packet is unmasked in place. */
    utf_build(packet, cols, 2, 2, strs, nums, 0x1234);
    for (size_t i = 0; i < size; ++i) { packet[i] ^= (uint8_t)m; m *= 0x4115; }
    CHECK(!p4_golden_utf_open(&utf, packet, size));
    CHECK(!p4_golden_utf_str(&utf, 0, "Name", &text) && !strcmp(text, "alpha"));
    /* A size word beyond the buffer is refused. */
    be32(packet + 4, 1u << 30);
    CHECK(p4_golden_utf_open(&utf, packet, size) == -1);
    return 0;
}

/* ---- CRILAYLA encoder: literals plus optional back references -------------- */

typedef struct Bits { uint8_t bytes[4096]; size_t count; unsigned used; } Bits;

static void put_bits(Bits *b, unsigned value, unsigned width)
{
    for (unsigned i = width; i-- > 0;) {
        if (!b->used) b->bytes[b->count++] = 0;
        if (value >> i & 1) b->bytes[b->count - 1] |= (uint8_t)(0x80 >> b->used);
        b->used = (b->used + 1) & 7;
    }
}

/* Encodes plain (decoded from its end) as literals, except that "copy" bytes
 * starting at plain[copy_at] (counting down) form one back reference of
 * distance 3 when copy_len > 0. Returns the stream size. */
static size_t layla_encode(uint8_t *out, const uint8_t *plain, size_t n, const uint8_t prefix[256], size_t copy_from, size_t copy_len)
{
    Bits bits;
    size_t i = n;
    memset(&bits, 0, sizeof bits);
    while (i > 0) {
        if (copy_len && i == copy_from) {
            size_t extra = copy_len - 3;
            put_bits(&bits, 1, 1);
            put_bits(&bits, 0, 13);
            /* VLE: 2, 3, 5, 8 bits; this fixture keeps extra below 3. */
            put_bits(&bits, (unsigned)extra, 2);
            i -= copy_len;
            continue;
        }
        put_bits(&bits, 0, 1);
        put_bits(&bits, plain[--i], 8);
    }
    memcpy(out, "CRILAYLA", 8);
    le32(out + 8, (uint32_t)n);
    le32(out + 12, (uint32_t)bits.count);
    for (size_t k = 0; k < bits.count; ++k) out[0x10 + bits.count - 1 - k] = bits.bytes[k];
    memcpy(out + 0x10 + bits.count, prefix, 256);
    return 0x10 + bits.count + 256;
}

static int test_crilayla(void)
{
    static uint8_t stream[8192], out[8192], prefix[256];
    static const uint8_t plain[] = "XYZABCABC";
    size_t n = sizeof plain - 1, size;

    for (int i = 0; i < 256; ++i) prefix[i] = (uint8_t)(255 - i);
    size = layla_encode(stream, plain, n, prefix, 0, 0);
    CHECK(p4_golden_crilayla_size(stream, size) == 256 + n);
    CHECK(!p4_golden_crilayla(stream, size, out, sizeof out));
    CHECK(!memcmp(out, prefix, 256) && !memcmp(out + 256, plain, n));
    /* Bytes 2..4 (counting down from i = 5) copy the three bytes after them. */
    size = layla_encode(stream, plain, n, prefix, 6, 3);
    CHECK(!p4_golden_crilayla(stream, size, out, sizeof out));
    CHECK(!memcmp(out + 256, plain, n));
    /* A too-small output, a truncated stream and a bad magic are refused. */
    CHECK(p4_golden_crilayla(stream, size, out, 256 + n - 1) == -1);
    le32(stream + 8, (uint32_t)(n + 50));
    CHECK(p4_golden_crilayla(stream, size, out, sizeof out) == -1);
    stream[0] = 'X';
    CHECK(p4_golden_crilayla_size(stream, size) == 0);
    return 0;
}

/* ---- .bin container and TBL spans ------------------------------------------ */

static size_t bin_add(uint8_t *bin, size_t at, const char *name, const void *payload, uint32_t size)
{
    memset(bin + at, 0, 256);
    strcpy((char *)bin + at, name);
    le32(bin + at + 252, size);
    memcpy(bin + at + 256, payload, size);
    return (at + 256 + size + 63) & ~(size_t)63;
}

static int test_containers(void)
{
    static uint8_t bin[4096], tbl[256];
    const uint8_t *payload;
    size_t size, at = 0;
    P4GoldenSpan spans[4];

    memset(bin, 0, sizeof bin);
    at = bin_add(bin, at, "battle/A.TBL", "aaaa", 4);
    at = bin_add(bin, at, "init/b.bin", "bbbbbb", 6);
    CHECK(!p4_golden_bin_find(bin, at, "INIT/B.BIN", &payload, &size) && size == 6 && !memcmp(payload, "bbbbbb", 6));
    CHECK(p4_golden_bin_find(bin, at, "missing", &payload, &size) == 1);
    le32(bin + 252, 1u << 20);
    CHECK(p4_golden_bin_find(bin, at, "init/b.bin", &payload, &size) == -1);

    memset(tbl, 0, sizeof tbl);
    le32(tbl, 3); memcpy(tbl + 4, "xyz", 3);
    le32(tbl + 16, 20); memset(tbl + 20, 7, 20);
    CHECK(p4_golden_tbl_spans(tbl, 48, spans, 4) == 2);
    CHECK(spans[0].size == 3 && spans[1].size == 20 && spans[1].bytes == tbl + 20);
    CHECK(p4_golden_tbl_spans(tbl, 30, spans, 4) == -1);
    return 0;
}

/* ---- CPK on disk -------------------------------------------------------------- */

static int test_cpk(void)
{
    static uint8_t file[65536], packet[8192], plain_entry[300], prefix[256];
    static const UtfCol header_cols[] = {{"TocOffset", 6}, {"ContentOffset", 6}};
    static const UtfCol toc_cols[] = {{"DirName", 0x0A}, {"FileName", 0x0A}, {"FileSize", 4}, {"ExtractSize", 4}, {"FileOffset", 6}};
    const char *toc_strs[] = {"init", "plain.bin", NULL, NULL, NULL, "", "packed.bin", NULL, NULL, NULL};
    uint64_t header_nums[2] = {0x800, 0x2000};
    uint64_t toc_nums[10];
    const char *path = "golden_source_test.cpk";
    uint8_t stream[2048];
    size_t packet_size, stream_size;
    P4GoldenCpk cpk;
    P4GoldenCpkEntry entry;
    uint8_t *bytes;
    size_t size;
    char reason[128];
    FILE *out;

    for (int i = 0; i < 300; ++i) plain_entry[i] = (uint8_t)(i * 7);
    memset(prefix, 0x5A, sizeof prefix);
    stream_size = layla_encode(stream, (const uint8_t *)"golden!", 7, prefix, 0, 0);
    memset(file, 0, sizeof file);
    packet_size = utf_build(packet, header_cols, 2, 1, NULL, header_nums, 0);
    memcpy(file, "CPK ", 4); le32(file + 8, (uint32_t)packet_size); memcpy(file + 0x10, packet, packet_size);
    /* Entries live at base(0x800) + FileOffset. */
    memset(toc_nums, 0, sizeof toc_nums);
    toc_nums[2] = 300; toc_nums[3] = 300; toc_nums[4] = 0x1800;
    toc_nums[7] = stream_size; toc_nums[8] = 256 + 7; toc_nums[9] = 0x1A00;
    packet_size = utf_build(packet, toc_cols, 5, 2, toc_strs, toc_nums, 0);
    memcpy(file + 0x800, "TOC ", 4); le32(file + 0x808, (uint32_t)packet_size); memcpy(file + 0x810, packet, packet_size);
    memcpy(file + 0x2000, plain_entry, 300);
    memcpy(file + 0x2200, stream, stream_size);
    CHECK((out = fopen(path, "wb")) != NULL);
    CHECK(fwrite(file, 1, 0x2200 + stream_size, out) == 0x2200 + stream_size);
    fclose(out);

    CHECK(!p4_golden_cpk_open(&cpk, path, reason, sizeof reason));
    CHECK(!p4_golden_cpk_find(&cpk, "INIT/plain.bin", &entry) && entry.offset == 0x2000 && entry.size == 300);
    CHECK(!p4_golden_cpk_read(&cpk, &entry, &bytes, &size, reason, sizeof reason) && size == 300 && !memcmp(bytes, plain_entry, 300));
    free(bytes);
    CHECK(!p4_golden_cpk_find(&cpk, "packed.bin", &entry));
    CHECK(!p4_golden_cpk_read(&cpk, &entry, &bytes, &size, reason, sizeof reason) && size == 263);
    CHECK(!memcmp(bytes, prefix, 256) && !memcmp(bytes + 256, "golden!", 7));
    free(bytes);
    CHECK(p4_golden_cpk_find(&cpk, "init/absent.bin", &entry) == 1);
    p4_golden_cpk_close(&cpk);
    remove(path);
    CHECK(p4_golden_cpk_open(&cpk, path, reason, sizeof reason) == -1);
    return 0;
}

/* ---- Translation on a synthetic Golden source and PS2 base ----------------------- */

enum { PS2_SKILLS = 576, PS2_ACTIVE = 440, PS2_ENEMIES = 336, PS2_ITEMS = 1536 };

static uint8_t g_skill_el[624 * 2], g_skill_rows[472 * 44];
static uint8_t g_unit0[368 * 60], g_unit1[368 * 32], g_unit2[256 * 32];
static uint8_t g_p0[256 * 14], g_p1[256 * 70], g_p2[24 * 0x26E], g_p3[9 * 392], g_p4[432], g_p5[256 * 4];
static uint8_t g_item[16 + 2560 * 68];
static uint8_t g_e0[944 * 24], g_e1[180 * 10], g_e2[200], g_e3[68 * 252], g_e4[84 * 348];
static uint8_t b_skill_el[PS2_SKILLS * 2], b_skill_rows[PS2_ACTIVE * 40], b_skill_names[PS2_SKILLS * 19];
static uint8_t b_enemy_names[PS2_ENEMIES * 21];
static uint8_t b_unit0[PS2_ENEMIES * 60], b_unit1[PS2_ENEMIES * 32], b_unit2[256 * 32];
static uint8_t b_p0[256 * 14], b_p1[256 * 70], b_p2[24 * 0x26E], b_p3[9 * 392];
static uint8_t b_item[16 + PS2_ITEMS * 68];
static uint8_t b_e0[912 * 24], b_e1[160 * 10], b_e3[56 * 248], b_e4[75 * 348];
static int base_shrunk;

static int base(void *user, P4GameTableInputKind input, uint32_t span, const uint8_t **bytes, size_t *size)
{
    (void)user;
#define SPAN(k, s, a) if (input == k && span == s) { *bytes = a; *size = sizeof a; if (base_shrunk && (const void *)a == (const void *)b_unit0) --*size; return 0; }
    SPAN(P4_TABLE_INPUT_SKILL, 0, b_skill_el) SPAN(P4_TABLE_INPUT_SKILL, 1, b_skill_rows)
    SPAN(P4_TABLE_INPUT_MSG, 1, b_skill_names) SPAN(P4_TABLE_INPUT_MSG, 2, b_enemy_names)
    SPAN(P4_TABLE_INPUT_UNIT, 0, b_unit0) SPAN(P4_TABLE_INPUT_UNIT, 1, b_unit1) SPAN(P4_TABLE_INPUT_UNIT, 2, b_unit2)
    SPAN(P4_TABLE_INPUT_PERSONA, 0, b_p0) SPAN(P4_TABLE_INPUT_PERSONA, 1, b_p1)
    SPAN(P4_TABLE_INPUT_PERSONA, 2, b_p2) SPAN(P4_TABLE_INPUT_PERSONA, 3, b_p3)
    SPAN(P4_TABLE_INPUT_ITEM, 0, b_item)
    SPAN(P4_TABLE_INPUT_ENCOUNT, 0, b_e0) SPAN(P4_TABLE_INPUT_ENCOUNT, 1, b_e1)
    SPAN(P4_TABLE_INPUT_ENCOUNT, 3, b_e3) SPAN(P4_TABLE_INPUT_ENCOUNT, 4, b_e4)
#undef SPAN
    return -1;
}

static void name(uint8_t *bank, size_t width, uint32_t id, const char *text) { strcpy((char *)bank + id * width, text); }
static void event(uint8_t *at, uint8_t level, int8_t kind, uint16_t skill) { at[0] = level; at[1] = (uint8_t)kind; le16(at + 2, skill); }

static const P4GoldenRows *set(const P4GoldenBuild *build, P4GameTableInputKind input, uint32_t span)
{
    for (size_t i = 0; i < build->count; ++i) {
        if (build->sets[i].input == input && build->sets[i].span == span) return &build->sets[i];
    }
    return NULL;
}

static void source_init(P4GoldenSource *source)
{
    P4GoldenTable *t = source->tables;
    memset(source, 0, sizeof *source);
    t[P4_GOLDEN_SKILL].spans[0] = (P4GoldenSpan){g_skill_el, sizeof g_skill_el};
    t[P4_GOLDEN_SKILL].spans[1] = (P4GoldenSpan){g_skill_rows, sizeof g_skill_rows};
    t[P4_GOLDEN_SKILL].span_count = 2;
    t[P4_GOLDEN_UNIT].spans[0] = (P4GoldenSpan){g_unit0, sizeof g_unit0};
    t[P4_GOLDEN_UNIT].spans[1] = (P4GoldenSpan){g_unit1, sizeof g_unit1};
    t[P4_GOLDEN_UNIT].spans[2] = (P4GoldenSpan){g_unit2, sizeof g_unit2};
    t[P4_GOLDEN_UNIT].span_count = 3;
    t[P4_GOLDEN_PERSONA].spans[0] = (P4GoldenSpan){g_p0, sizeof g_p0};
    t[P4_GOLDEN_PERSONA].spans[1] = (P4GoldenSpan){g_p1, sizeof g_p1};
    t[P4_GOLDEN_PERSONA].spans[2] = (P4GoldenSpan){g_p2, sizeof g_p2};
    t[P4_GOLDEN_PERSONA].spans[3] = (P4GoldenSpan){g_p3, sizeof g_p3};
    t[P4_GOLDEN_PERSONA].spans[4] = (P4GoldenSpan){g_p4, sizeof g_p4};
    t[P4_GOLDEN_PERSONA].spans[5] = (P4GoldenSpan){g_p5, sizeof g_p5};
    t[P4_GOLDEN_PERSONA].span_count = 6;
    t[P4_GOLDEN_ITEM].bytes = g_item;
    t[P4_GOLDEN_ITEM].size = sizeof g_item;
    t[P4_GOLDEN_ENCOUNT].spans[0] = (P4GoldenSpan){g_e0, sizeof g_e0};
    t[P4_GOLDEN_ENCOUNT].spans[1] = (P4GoldenSpan){g_e1, sizeof g_e1};
    t[P4_GOLDEN_ENCOUNT].spans[2] = (P4GoldenSpan){g_e2, sizeof g_e2};
    t[P4_GOLDEN_ENCOUNT].spans[3] = (P4GoldenSpan){g_e3, sizeof g_e3};
    t[P4_GOLDEN_ENCOUNT].spans[4] = (P4GoldenSpan){g_e4, sizeof g_e4};
    t[P4_GOLDEN_ENCOUNT].span_count = 5;
}

static int test_translation(void)
{
    P4GoldenSource source;
    P4GoldenBuild build;
    const P4GoldenRows *rows;
    char reason[160] = "";
    uint8_t *row;

    source_init(&source);
    le32(g_item, 2560);
    /* Names: 1 Agi, 2 Blank, 0x11F placeholder, 0x1FC Invigorate 1 (Golden 0x21C), 0x10F Relaxing Wave. */
    name(b_skill_names, 19, 1, "Agi");
    name(b_skill_names, 19, 2, "Blank");
    name(b_skill_names, 19, 0x11F, "11F");
    name(b_skill_names, 19, 0x10F, "Relaxing Wave");
    name(b_skill_names, 19, 0x1FC, "Invigorate 1");
    name(b_enemy_names, 21, 1, "Lying Hablerie");
    name(b_enemy_names, 21, 2, "blank");

    /* Skill 1: Golden cost 4 -> 3 and a changed byte 1 (kept from PS2). Skill 2: Golden-only. */
    b_skill_rows[1 * 40 + 1] = 2; b_skill_rows[1 * 40 + 4] = 4;
    g_skill_rows[1 * 44 + 4] = 9; g_skill_rows[1 * 44 + 8] = 3;
    g_skill_rows[2 * 44 + 1] = 4; g_skill_rows[2 * 44 + 8] = 7;

    /* Party row 8: a learn of the placeholder 0x11F at 79 with a forget of Relaxing
     * Wave at 79 (both dropped), and a passive learned through the shifted id. */
    event(g_p2 + 8 * 0x26E + 4, 61, 1, 0x10F);
    event(g_p2 + 8 * 0x26E + 8, 79, 2, 0x10F);
    event(g_p2 + 8 * 0x26E + 12, 79, 1, 0x11F);
    event(g_p2 + 8 * 0x26E + 16, 80, 1, 0x21C);
    event(b_p2 + 8 * 0x26E + 4, 61, 1, 0x10F);

    /* Enemy 1: new HP and skills (skills stay PS2's), drops of a Golden-only
     * item bank, of a PS2 dummy item and of a real item. Enemy 2 is blank. */
    le16(b_unit0 + 60 + 4, 100); le16(b_unit0 + 60 + 0x0E, 1);
    le16(g_unit0 + 60 + 4, 160); le16(g_unit0 + 60 + 0x0E, 2);
    le16(g_unit0 + 60 + 0x22, 0x850); g_unit0[60 + 0x24] = 10;
    le16(g_unit0 + 60 + 0x26, 0x1A5); g_unit0[60 + 0x28] = 10;
    le16(g_unit0 + 60 + 0x2A, 0x501); g_unit0[60 + 0x2C] = 10;
    le16(g_unit0 + 60 + 0x32, 123); le16(g_unit0 + 60 + 0x34, 0x501); g_unit0[60 + 0x36] = 40;
    le32(b_item + 16 + 0x1A5 * 68 + 0x34, 1);
    le32(b_item + 16 + 0x501 * 68 + 0x34, 300);
    le16(g_unit0 + 120 + 4, 999);

    /* Item 0x30C: price cut, Golden flags and availability ignored. Item 0x3A0: a PS2 dummy. */
    le32(b_item + 16 + 0x30C * 68 + 0x34, 10000); le32(b_item + 16 + 0x30C * 68 + 0x04, 0x10000);
    le32(g_item + 16 + 0x30C * 68 + 0x34, 2500); le32(g_item + 16 + 0x30C * 68 + 0x04, 0x400000);
    g_item[16 + 0x30C * 68 + 0x38] = 9;
    le32(b_item + 16 + 0x3A0 * 68 + 0x34, 1); le32(g_item + 16 + 0x3A0 * 68 + 0x34, 900);
    /* Accessory 0x26D: Golden grants Golden-only 0x25F; PS2 Invigorate 1 stays. */
    le32(b_item + 16 + 0x26D * 68 + 0x34, 5000); le16(b_item + 16 + 0x26D * 68 + 0x24, 0x1FC);
    le32(g_item + 16 + 0x26D * 68 + 0x34, 6000); le16(g_item + 16 + 0x26D * 68 + 0x24, 0x25F);

    /* Curve 0 grows. */
    le32(b_p3, 100); le32(g_p3, 140);

    CHECK(!p4_golden_build(&source, P4_GOLDEN_GROUP_NUMBERS | P4_GOLDEN_GROUP_ENEMIES | P4_GOLDEN_GROUP_PARTY | P4_GOLDEN_GROUP_EXP,
                           base, NULL, &build, reason, sizeof reason));

    CHECK((rows = set(&build, P4_TABLE_INPUT_SKILL, 1)) && rows->count == 1 && rows->ids[0] == 1 && rows->row_size == 40);
    CHECK(rows->bytes[1] == 2 && rows->bytes[4] == 3);
    CHECK(!set(&build, P4_TABLE_INPUT_SKILL, 0));

    CHECK((rows = set(&build, P4_TABLE_INPUT_PERSONA, 2)) && rows->count == 1 && rows->ids[0] == 8);
    row = rows->bytes;
    CHECK(row[4] == 61 && row[5] == 1 && row[6] == 0x0F && row[7] == 0x01);
    CHECK(row[8] == 80 && row[9] == 1 && row[10] == 0xFC && row[11] == 0x01);
    CHECK(!row[12] && !row[13] && !row[14] && !row[15]);

    CHECK((rows = set(&build, P4_TABLE_INPUT_UNIT, 0)) && rows->count == 1 && rows->ids[0] == 1);
    row = rows->bytes;
    CHECK(row[4] == 160 && row[0x0E] == 1);
    CHECK(!row[0x22] && !row[0x26] && row[0x2A] == 0x01 && row[0x2B] == 0x05 && row[0x2C] == 10);
    CHECK(row[0x32] == 123 && row[0x34] == 0x01 && row[0x35] == 0x05 && row[0x36] == 40);

    CHECK((rows = set(&build, P4_TABLE_INPUT_ITEM, 0)) && rows->count == 1 && rows->ids[0] == 0x30C);
    row = rows->bytes;
    CHECK(row[0x34] == (2500 & 0xFF) && row[0x06] == 0x01 && row[0x38] == 0);

    CHECK((rows = set(&build, P4_TABLE_INPUT_PERSONA, 3)) && rows->count == 1 && rows->ids[0] == 0 && rows->row_size == 392);
    p4_golden_build_free(&build);

    /* Unsupported special items fall through to ordinary drops. */
    le16(b_unit0 + 60 + 0x34, 0x501);
    le16(g_unit0 + 60 + 0x34, 0x850);
    CHECK(!p4_golden_build(&source, P4_GOLDEN_GROUP_ENEMIES, base, NULL, &build, reason, sizeof reason));
    CHECK((rows = set(&build, P4_TABLE_INPUT_UNIT, 0)) && rows->count == 1);
    CHECK(!rows->bytes[0x34] && !rows->bytes[0x35]);
    p4_golden_build_free(&build);
    le16(g_unit0 + 60 + 0x34, 0x1A5);
    CHECK(!p4_golden_build(&source, P4_GOLDEN_GROUP_ENEMIES, base, NULL, &build, reason, sizeof reason));
    CHECK((rows = set(&build, P4_TABLE_INPUT_UNIT, 0)) && rows->count == 1);
    CHECK(!rows->bytes[0x34] && !rows->bytes[0x35]);
    p4_golden_build_free(&build);

    /* Group selection: nothing for no groups, only curves for EXP. */
    CHECK(!p4_golden_build(&source, 0, base, NULL, &build, reason, sizeof reason) && build.count == 0);
    CHECK(!p4_golden_build(&source, P4_GOLDEN_GROUP_EXP, base, NULL, &build, reason, sizeof reason) && build.count == 1);
    p4_golden_build_free(&build);

    /* Encounters. Golden formation 5 lands on PS2's empty row 5, 6 moves past
     * the base (PS2's row 6 is another formation), 7 names a blank enemy. */
    le16(g_e0 + 5 * 24 + 8, 1); le16(g_e0 + 6 * 24 + 8, 1); le16(g_e0 + 6 * 24 + 10, 1);
    le16(g_e0 + 7 * 24 + 8, 2); le16(b_e0 + 6 * 24 + 8, 1);
    /* Group 1: rates (80, 70, x) (10, 10, x) (10, 15, x); day 0 normal 5 and
     * 6, rare 7 only; day 1 normal 5. */
    g_e3[252] = 80; g_e3[253] = 70; g_e3[254] = 0xFF; g_e3[255] = 10; g_e3[256] = 10; g_e3[257] = 1;
    g_e3[258] = 10; g_e3[259] = 15; g_e3[260] = 1;
    le16(g_e3 + 252 + 12, 5); le16(g_e3 + 252 + 14, 20);
    le16(g_e3 + 252 + 16, 6); le16(g_e3 + 252 + 18, 30);
    le16(g_e3 + 252 + 12 + 25 * 4, 7); le16(g_e3 + 252 + 14 + 25 * 4, 100);
    le16(g_e3 + 252 + 12 + 30 * 4, 5); le16(g_e3 + 252 + 14 + 30 * 4, 40);
    /* Selector 0 differs but stays PS2's; selector 3 sends group 1 with treasure group 2. */
    g_e1[2] = 1;
    le16(g_e1 + 30, 1); g_e1[32] = 3; g_e1[33] = 6; g_e1[34] = 4; le16(g_e1 + 36, 2);
    /* Treasure group 2 changes real item 0x501's weight; group 3 names a
     * Golden-only item; group 5 drops PS2's only locked-chest record (byte 7
     * bit 0) and group 6 adds one PS2's lacks. */
    le16(b_e4 + 2 * 348, 50); le16(b_e4 + 2 * 348 + 2, 0x501);
    le16(g_e4 + 2 * 348, 100); le16(g_e4 + 2 * 348 + 2, 0x501);
    le16(g_e4 + 3 * 348, 100); le16(g_e4 + 3 * 348 + 2, 0x850);
    le16(g_e4 + 5 * 348, 100); le16(g_e4 + 5 * 348 + 2, 0x501);
    le16(b_e4 + 5 * 348 + 12, 30); le16(b_e4 + 5 * 348 + 14, 0x501); b_e4[5 * 348 + 19] = 1;
    le16(b_e4 + 6 * 348, 100); le16(b_e4 + 6 * 348 + 2, 0x501);
    le16(g_e4 + 6 * 348, 100); le16(g_e4 + 6 * 348 + 2, 0x501);
    le16(g_e4 + 6 * 348 + 12, 30); le16(g_e4 + 6 * 348 + 14, 0x501); g_e4[6 * 348 + 19] = 1;

    CHECK(!p4_golden_build(&source, P4_GOLDEN_GROUP_ENCOUNTERS, base, NULL, &build, reason, sizeof reason));
    CHECK((rows = set(&build, P4_TABLE_INPUT_ENCOUNT, 3)) && rows->count == 1 && rows->ids[0] == 1 && rows->row_size == 248);
    row = rows->bytes;
    CHECK(row[0] == 80 && row[1] == 70 && row[2] == 10 && row[3] == 10 && row[4] == 0 && row[5] == 15 && !row[6] && !row[7]);
    CHECK(row[8] == 5 && row[10] == 20 && row[12] == 0x90 && row[13] == 0x03 && row[14] == 30);
    CHECK(!row[8 + 25 * 4] && !row[10 + 25 * 4] && row[8 + 30 * 4] == 5 && row[10 + 30 * 4] == 40);
    CHECK((rows = set(&build, P4_TABLE_INPUT_ENCOUNT, 0)) && rows->count == 2 && rows->row_size == 24);
    CHECK(rows->ids[0] == 5 && rows->ids[1] == 912 && rows->bytes[24 + 10] == 1);
    CHECK((rows = set(&build, P4_TABLE_INPUT_ENCOUNT, 1)) && rows->count == 1 && rows->ids[0] == 3);
    CHECK((rows = set(&build, P4_TABLE_INPUT_ENCOUNT, 4)) && rows->count == 1 && rows->ids[0] == 2);
    p4_golden_build_free(&build);

    /* A selector naming a group past PS2's is refused. */
    le16(g_e1 + 30, 60);
    CHECK(p4_golden_build(&source, P4_GOLDEN_GROUP_ENCOUNTERS, base, NULL, &build, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "selectors") != NULL && build.count == 0);
    le16(g_e1 + 30, 1);

    /* A base that is not the retail layout is refused with a reason. */
    base_shrunk = 1;
    CHECK(p4_golden_build(&source, P4_GOLDEN_GROUP_ENEMIES, base, NULL, &build, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "UNIT.TBL") != NULL && build.count == 0);
    base_shrunk = 0;
    return 0;
}

int main(void)
{
    if (test_utf() || test_crilayla() || test_containers() || test_cpk() || test_translation()) return 1;
    puts("golden_source: ok");
    return 0;
}
