#ifndef P4_GOLDEN_CPK_FIXTURE_H
#define P4_GOLDEN_CPK_FIXTURE_H
#include <stdint.h>
#include <string.h>
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
#endif
