/* Paletted native texture data (retail 003fd940 readTEXFromStream, data kind 2): the PS2 pre-swizzle undone.
 * The reader that calls this is func_003fd940 in p4gl_native.c.
 *
 * Kind 2: the data chunk is a GIF stream of the uploads the retail sky texture cache replays:
 * A+D register packets (TRXPOS, TRXREG, TRXDIR) each followed by an IMAGE tag. The first IMAGE is
 * the pixels, the last the CLUT. The pixels are the texture's PSMT8 / PSMT4 memory image written
 * through PSMCT32 / PSMCT16 (the usual pre-swizzle): as a raster of (w/2 x h/2) words for 8 bit
 * and (w/2 x h/2) half words for 4 bit. They are undone as follows.
 *   PSMT8 pixel (x, y): swap = (((y + 2) >> 2) & 1) * 4, posY = (((y & ~3) >> 1) + (y & 1)) & 7;
 *   the word at column (x >> 4) * 8 + ((x + swap) & 7), row (y >> 4) * 8 + posY, byte
 *   ((y >> 1) & 1) + ((x >> 2) & 2).
 *   (Textures of 16 or 32 wide and 8 or 16 high decode too: the half words their upload does not hold read as 0.)
 *   PSMT4 pixel (x, y): page (y >> 7) * pagesAcross + (x >> 7), block table
 *   {0,2,8,10},{1,3,9,11},{4,6,12,14},{5,7,13,15},{16,18,24,26},{17,19,25,27},{20,22,28,30},
 *   {21,23,29,31} at [(y >> 4) & 7][(x >> 5) & 3], column (y >> 2) & 3 of 128 nibbles, in the
 *   column nibble 2 * ((x >> 3) & 3) + f((x & 7) + rot) + (yy >> 1) + (yy & 1) * 16 with
 *   yy = y & 3, f(b) = (b & 1) * 8 + (b >> 1) * 32 and rot = (yy >> 1) * 4, advanced by 4 in the
 *   odd columns; that nibble address is looked up in the PSMCT16 stream through the inverse of
 *   the PSMCT16 page / block / column addressing (blocks of 16 x 8, 64-bit columns of 16 x 2).
 *   The CLUT: 256 entries (PSMCT32, 16 x 16, CSM1) entry i at x = (i & 7) | ((i & 16) >> 1),
 *   y = ((i >> 5) << 1) | ((i >> 3) & 1); 16 entries in order. Checked by decoding the textures
 *   of DOBJ044.RMD to images (a cloud sheet in each). */

/* Pure C (no RenderWare), so that tests/native_tex_test.c can run it. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl_nativetex.h"

#define P4_GIF_MAX_IMAGES 16

typedef struct P4GifImage {
    const uint8_t *data;
    uint32_t bytes;
} P4GifImage;

static int gifImages(const uint8_t *d, uint32_t n, P4GifImage *imgs, int max)
{
    uint32_t o = 0;
    int count = 0;

    while (o + 16u <= n) {
        uint64_t tag;
        memcpy(&tag, d + o, sizeof tag);
        o += 16u;
        uint32_t nloop = (uint32_t)(tag & 0x7FFFu);
        uint32_t flg = (uint32_t)((tag >> 58) & 3u);
        uint32_t nreg = (uint32_t)((tag >> 60) & 15u);
        if (nreg == 0) {
            nreg = 16;
        }
        if (flg == 0) {
            o += 16u * nloop * nreg;
        } else if (flg == 1) {
            o += ((nloop * nreg + 1u) / 2u) * 16u;
        } else if (flg == 2) {
            if (o + 16u * nloop > n || count >= max) {
                return -1;
            }
            imgs[count].data = d + o;
            imgs[count].bytes = 16u * nloop;
            count++;
            o += 16u * nloop;
        } else {
            break;
        }
    }
    return count;
}

static const uint8_t kBlock16[8][4] = {{0, 2, 8, 10},   {1, 3, 9, 11},   {4, 6, 12, 14},   {5, 7, 13, 15},
                                       {16, 18, 24, 26}, {17, 19, 25, 27}, {20, 22, 28, 30}, {21, 23, 29, 31}};

/* Half word index of PSMCT16 pixel (x, y) in a GS memory image of `pagesAcross` 64-wide pages. */
static uint32_t ct16Address(uint32_t x, uint32_t y, uint32_t pagesAcross)
{
    uint32_t page = (y >> 6) * pagesAcross + (x >> 6);
    uint32_t blk = kBlock16[(y >> 3) & 7u][(x >> 4) & 3u];
    uint32_t g = 2u * (x & 1u) + 8u * ((x >> 1) & 3u) + ((x & 15u) >> 3);
    return page * 4096u + blk * 128u + ((y >> 1) & 3u) * 32u + (y & 1u) * 4u + g;
}

static uint32_t t4Nibble(uint32_t x, uint32_t y)
{
    uint32_t col = (y >> 2) & 3u;
    uint32_t yy = y & 3u;
    uint32_t rot = (yy >> 1) * 4u;
    if (col & 1u) {
        rot = (rot + 4u) & 7u;
    }
    uint32_t b = ((x & 7u) + rot) & 7u;
    uint32_t f = (b & 1u) * 8u + (b >> 1) * 32u;
    return col * 128u + 2u * ((x >> 3) & 3u) + f + (yy >> 1) + (yy & 1u) * 16u;
}

/* PSMT8 image of w x h bytes from its (w/2 x h/2)-word pre-swizzled stream. */
static int unswizzle8(const uint8_t *src, uint32_t srcBytes, int32_t w, int32_t h, uint8_t *dst)
{
    if ((w & 15) != 0 || (h & 15) != 0 || srcBytes < (uint32_t)w * (uint32_t)h) {
        return 0;
    }
    for (int32_t y = 0; y < h; y++) {
        uint32_t swap = (((uint32_t)y + 2u) >> 2 & 1u) * 4u;
        uint32_t posY = ((((uint32_t)y & ~3u) >> 1) + ((uint32_t)y & 1u)) & 7u;
        uint32_t row = ((uint32_t)y >> 4) * 8u + posY;
        for (int32_t x = 0; x < w; x++) {
            uint32_t col = ((uint32_t)x >> 4) * 8u + (((uint32_t)x + swap) & 7u);
            uint32_t byte = (((uint32_t)y >> 1) & 1u) + (((uint32_t)x >> 2) & 2u);
            dst[(size_t)y * (size_t)w + (size_t)x] = src[(row * ((uint32_t)w / 2u) + col) * 4u + byte];
        }
    }
    return 1;
}

/* PSMT4 image of w x h nibbles (one per byte) from its (w/2 x h/2)-half-word pre-swizzled stream. */
static int unswizzle4(const uint8_t *src, uint32_t srcBytes, int32_t w, int32_t h, uint8_t *dst)
{
    if ((w & 15) != 0 || (h & 7) != 0 || srcBytes < (uint32_t)w * (uint32_t)h / 2u) {
        return 0;
    }
    uint32_t cw = (uint32_t)w / 2u;
    uint32_t chh = (uint32_t)h / 2u;
    uint32_t across = (cw + 63u) / 64u;
    uint32_t down = (chh + 63u) / 64u;
    uint32_t *inv = malloc((size_t)across * down * 4096u * sizeof *inv);
    if (inv == NULL) {
        return 0;
    }
    memset(inv, 0xFF, (size_t)across * down * 4096u * sizeof *inv);   /* half words the stream does not hold: a block of a narrow or short texture is only partly written */
    for (uint32_t y = 0; y < chh; y++) {
        for (uint32_t x = 0; x < cw; x++) {
            inv[ct16Address(x, y, across)] = y * cw + x;
        }
    }
    uint32_t pagesAcross = ((uint32_t)w + 127u) / 128u;
    for (int32_t y = 0; y < h; y++) {
        for (int32_t x = 0; x < w; x++) {
            uint32_t page = ((uint32_t)y >> 7) * pagesAcross + ((uint32_t)x >> 7);
            uint32_t blk = kBlock16[((uint32_t)y >> 4) & 7u][((uint32_t)x >> 5) & 3u];
            uint32_t n = t4Nibble((uint32_t)x, (uint32_t)y);
            uint32_t hw = 2u * (page * 2048u + blk * 64u + (n >> 3)) + ((n & 7u) >> 2);
            uint16_t v = 0;
            if (hw < across * down * 4096u && inv[hw] != 0xFFFFFFFFu) {
                memcpy(&v, src + 2u * (size_t)inv[hw], sizeof v);
            }
            dst[(size_t)y * (size_t)w + (size_t)x] = (uint8_t)((v >> (4u * (n & 3u))) & 15u);
        }
    }
    free(inv);
    return 1;
}


int p4gl_nativetex_decode(const uint8_t *data, uint32_t size, int32_t w, int32_t h, int32_t depth, uint8_t *indices,
                          uint8_t *palette)
{
    P4GifImage imgs[P4_GIF_MAX_IMAGES];
    int count = gifImages(data, size, imgs, P4_GIF_MAX_IMAGES);

    if (count < 2 || (depth != 8 && depth != 4)) {
        return 0;
    }
    /* A texture narrower than the GS block (PSMT8 16 pixels, PSMT4 32) is uploaded padded to the block width (a 16x16 PSMT4 texture is 256
     * stream bytes: 32x16): unswizzle the padded image and keep the left w columns. */
    const int32_t block = depth == 8 ? 16 : 32;
    int32_t pw = (w + block - 1) & ~(block - 1);
    if (pw != w && (uint64_t)imgs[0].bytes * 2u < (uint64_t)pw * (uint64_t)h * (depth == 8 ? 2u : 1u)) {
        pw = w;   /* the stream holds only the w columns (the 16-wide uploads of the school roof): decoded directly, missing half words read 0 */
    }
    uint8_t *padded = pw != w ? malloc((size_t)pw * (size_t)h) : NULL;
    if (pw != w && padded == NULL) {
        return 0;
    }
    uint8_t *target = padded != NULL ? padded : indices;
    const int swizzled = depth == 8 ? unswizzle8(imgs[0].data, imgs[0].bytes, pw, h, target)
                                    : unswizzle4(imgs[0].data, imgs[0].bytes, pw, h, target);
    if (swizzled && padded != NULL) {
        for (int32_t y = 0; y < h; y++) {
            memcpy(indices + (size_t)y * (size_t)w, padded + (size_t)y * (size_t)pw, (size_t)w);
        }
    }
    free(padded);
    if (!swizzled) {
        return 0;
    }
    const P4GifImage *clut = &imgs[count - 1];
    const uint32_t entries = depth == 8 ? 256u : 16u;
    if (clut->bytes < entries * 4u) {
        return 0;
    }
    for (uint32_t i = 0; i < entries; i++) {
        uint32_t src = i;
        if (depth == 8) {
            src = ((i & 7u) | ((i & 16u) >> 1)) + (((i >> 5) << 1) | ((i >> 3) & 1u)) * 16u;
        }
        memcpy(palette + 4u * i, clut->data + 4u * src, 4u);
    }
    return 1;
}

int p4gl_nativetex_decode_ct32(const uint8_t *data, uint32_t size, int32_t w, int32_t h, uint8_t *rgba)
{
    P4GifImage imgs[P4_GIF_MAX_IMAGES];
    const int count = gifImages(data, size, imgs, P4_GIF_MAX_IMAGES);
    const uint32_t bytes = (uint32_t)w * (uint32_t)h * 4u;

    if (count < 1 || imgs[0].bytes < bytes) {
        return 0;
    }
    memcpy(rgba, imgs[0].data, bytes);
    return 1;
}
