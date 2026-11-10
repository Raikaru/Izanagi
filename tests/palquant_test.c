/* Palette quantizer host test. The palquant closure is compiled from its overrides as separate
 * translation units (tests/CMakeLists.txt): func_003f07d0 accumulates pixels into the colour
 * trie, func_003f17e0 builds the palette, func_003f1ce0 remaps pixels to palette indices.
 *
 * The pool allocator (func_003e1210 and D_008873F8) is a test double. The mask table at
 * 0x0088B400 is filled with the rows func_003f24a0 writes (its C body in the palquant mirror),
 * and the bit count at 0x007690F0-21552 is set in memory mapped at the retail address. Checks:
 * every index is below the palette size, and when the image has no more
 * colours than the palette holds, every pixel maps back to its exact colour. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "p4pc/os.h"
#include "palquant_prelude.h"
#include "guest_test.h"

typedef struct PalqImage {
    s32 unused0;
    s32 width;
    s32 height;
    s32 bpp;
    s32 stride;
    u32 rows;       /* guest addresses: the overrides read these fields as 4-byte pointers */
    u32 palette;
} PalqImage;

void func_003f07d0(u8 *ctx, u8 *img, float w);
s32 func_003f17e0(u8 *pal, s32 nout, u8 *ctx);
void func_003f1ce0(u8 *dst, s32 dst_stride, s32 dst_bpp, s32 pack, u8 *ctx, PalqImage *img);

#define PALQ_COUNT ((s32 *)P4_G2H(0x007690F0u - 21552u))
#define PALQ_MASKS ((s64 *)P4_G2H(0x0088B400u))

/* What the overrides reach through a guest address (the trie, its node pool, the image record) is guest storage on a 64-bit host (tests/guest_test.h). */
#ifdef P4_GUEST64
#define T_ALLOC(n) ((u8 *)p4t_galloc((n), 16))
#define T_FREE(p) ((void)(p))
#else
#define T_ALLOC(n) ((u8 *)calloc(1, (n)))
#define T_FREE(p) free(p)
#endif

static int checks;
static int failures;

#define CHECK(cond, ...)                                 \
    do {                                                 \
        checks++;                                        \
        if (!(cond)) {                                   \
            failures++;                                  \
            fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
            fprintf(stderr, __VA_ARGS__);                \
            fputc('\n', stderr);                         \
        }                                                \
    } while (0)

/* Test double for the trie node pool: each node is 92 bytes, zeroed. */
static u8 *test_node_alloc(u8 *pool, s32 flags)
{
    (void)pool;
    (void)flags;
    return T_ALLOC(128);
}

#ifndef P4_GUEST64
u8 *(*D_008873F8[])(u8 *arg0, s32 arg1) = { test_node_alloc };
#endif

u8 *func_003e1210(s32 arg0, s32 arg1, s32 arg2, s32 arg3)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    return T_ALLOC(64);
}

/* Same rows as func_003f24a0 (palquant mirror): masks[t2] ORs, for each set bit t1 of t2,
 * the sign-extended 32-bit value 1 << (4 * (count - 1 - t1)). */
static void palq_mask_fill(s32 count)
{
    s32 rows = (s32)((u32)1 << (count & 31));
    s32 t1;
    s32 t2;

    for (t2 = 0; t2 < rows; t2++) {
        s64 acc = 0;
        for (t1 = 0; t1 < count; t1++) {
            if ((t2 & ((s32)1 << (t1 & 31))) != 0) {
                s32 sh = ((count - 1 - t1) * 4) & 31;
                acc |= (s64)(s32)((u32)1 << sh);
            }
        }
        PALQ_MASKS[t2] = acc;
    }
}

/* Same setup as func_003f24a0: the trie root at ctx+16384 and its pool at ctx+16388. */
static u8 *palq_new_ctx(void)
{
    u8 *ctx = T_ALLOC(32768);
    u8 *pool = func_003e1210(92, 1024, 4, 0x00030411);

    *(u32 *)(ctx + 16388) = (u32)P4_H2G(pool);
    *(u32 *)(ctx + 16384) = (u32)P4_H2G(test_node_alloc(pool, 0x00030411));
    return ctx;
}

static void palq_quantize(const u8 *rgba, s32 w, s32 h, s32 nout, u8 *pal, u8 *out,
                          s32 out_stride, s32 dst_bpp, s32 pack, s32 *leaves)
{
    PalqImage img;
    u8 *ctx = palq_new_ctx();

    img.unused0 = 0;
    img.width = w;
    img.height = h;
    img.bpp = 32;
    img.stride = w * 4;
    img.rows = (u32)P4_H2G(rgba);
    img.palette = 0;

    func_003f07d0(ctx, (u8 *)&img, 1.0f);
    *leaves = func_003f17e0(pal, nout, ctx);
    func_003f1ce0(out, out_stride, dst_bpp, pack, ctx, &img);
    T_FREE(ctx);
}

/* Reads the index of pixel (x, y): one byte per pixel, or a nibble pair when packed (even pixel
 * in the low nibble, odd pixel in the high nibble). */
static u8 palq_index_at(const u8 *out, s32 out_stride, s32 nibble, s32 x, s32 y)
{
    if (!nibble) {
        return out[y * out_stride + x];
    }
    if ((x & 1) != 0) {
        return (u8)(out[y * out_stride + x / 2] >> 4);
    }
    return (u8)(out[y * out_stride + x / 2] & 0x0f);
}

static void palq_check_remap(const char *name, const u8 *rgba, s32 w, s32 h, const u8 *pal,
                             s32 nout, const u8 *out, s32 out_stride, s32 nibble, s32 exact)
{
    s32 x;
    s32 y;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            u8 idx = palq_index_at(out, out_stride, nibble, x, y);
            const u8 *px = rgba + 4 * (y * w + x);
            CHECK(idx < nout, "%s: pixel (%d,%d) index %d >= palette size %d",
                  name, x, y, idx, nout);
            if (exact && idx < nout) {
                CHECK(memcmp(px, pal + 4 * idx, 4) == 0,
                      "%s: pixel (%d,%d) colour %02x%02x%02x%02x, palette[%d] %02x%02x%02x%02x",
                      name, x, y, px[0], px[1], px[2], px[3], idx,
                      pal[4 * idx], pal[4 * idx + 1], pal[4 * idx + 2], pal[4 * idx + 3]);
            }
        }
    }
}

static void palq_build_gradient(u8 *gradient)
{
    s32 x;
    s32 y;

    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            u8 *px = gradient + 4 * (y * 16 + x);
            px[0] = (u8)(x * 16);
            px[1] = (u8)(y * 16);
            px[2] = 0;
            px[3] = 255;
        }
    }
}

static int run_test(int argc, char **argv)
{
    /* Four colours in a 4x4 image: fits a palette of 16 or more entries. */
    _Alignas(16) const u8 four_colour[4 * 4 * 4] = {
        255, 0, 0, 255,   0, 255, 0, 255,   0, 0, 255, 255,   16, 32, 48, 255,
        0, 255, 0, 255,   0, 0, 255, 255,   16, 32, 48, 255,  255, 0, 0, 255,
        0, 0, 255, 255,   16, 32, 48, 255,  255, 0, 0, 255,   0, 255, 0, 255,
        16, 32, 48, 255,  255, 0, 0, 255,   0, 255, 0, 255,   0, 0, 255, 255,
    };
    /* 256 colours: red and green vary with x and y, blue is zero. */
    _Alignas(16) u8 gradient[16 * 16 * 4];
    _Alignas(16) u8 pal[4 * 256];
    _Alignas(16) u8 out[16 * 16];
    s32 leaves;

    (void)argc;
    (void)argv;

    if (p4t_guest_map(0x0088B000u, 0x4000u) != 0) {
        fprintf(stderr, "palquant_test: cannot map the mask table\n");
        return 1;
    }
    if (p4t_guest_map(0x00763000u, 0x2000u) != 0) {
        fprintf(stderr, "palquant_test: cannot map the bit count\n");
        return 1;
    }
#ifdef P4_GUEST64
    /* the node allocator slot the overrides call through (the i386 build defines it as a symbol) */
    if (p4t_guest_map(0x00887000u, 0x1000u) != 0) {
        return 1;
    }
    *(uint32_t *)P4_G2H(0x008873F8u) = p4_fn_h2g_sig(test_node_alloc, "ppi");
#endif
    *PALQ_COUNT = 8;
    palq_mask_fill(8);
    palq_build_gradient(gradient);

    /* 4 colours, 16-entry palette, byte indices: exact. */
    memset(pal, 0, sizeof(pal));
    memset(out, 0, sizeof(out));
    palq_quantize(four_colour, 4, 4, 16, pal, out, 4, 8, 0, &leaves);
    CHECK(leaves >= 1 && leaves <= 16, "4-colour/16: leaf count %d", leaves);
    palq_check_remap("4-colour/16 byte", four_colour, 4, 4, pal, 16, out, 4, 0, 1);

    /* 4 colours, 256-entry palette, byte indices: exact. */
    memset(pal, 0, sizeof(pal));
    memset(out, 0, sizeof(out));
    palq_quantize(four_colour, 4, 4, 256, pal, out, 4, 8, 0, &leaves);
    CHECK(leaves >= 1 && leaves <= 256, "4-colour/256: leaf count %d", leaves);
    palq_check_remap("4-colour/256 byte", four_colour, 4, 4, pal, 256, out, 4, 0, 1);

    /* 4 colours, 16-entry palette, packed 4-bit indices (two pixels per byte): exact. */
    memset(pal, 0, sizeof(pal));
    memset(out, 0, sizeof(out));
    palq_quantize(four_colour, 4, 4, 16, pal, out, 2, 4, 1, &leaves);
    CHECK(leaves >= 1 && leaves <= 16, "4-colour/16 nibble: leaf count %d", leaves);
    palq_check_remap("4-colour/16 nibble", four_colour, 4, 4, pal, 16, out, 2, 1, 1);

    /* 256 colours, 256-entry palette: colour count fits, so exact. */
    memset(pal, 0, sizeof(pal));
    memset(out, 0, sizeof(out));
    palq_quantize(gradient, 16, 16, 256, pal, out, 16, 8, 0, &leaves);
    CHECK(leaves >= 1 && leaves <= 256, "gradient/256: leaf count %d", leaves);
    palq_check_remap("gradient/256 byte", gradient, 16, 16, pal, 256, out, 16, 0, 1);

    /* 256 colours, 16-entry palette: colours do not fit, so only the index range is checked. */
    memset(pal, 0, sizeof(pal));
    memset(out, 0, sizeof(out));
    palq_quantize(gradient, 16, 16, 16, pal, out, 16, 8, 0, &leaves);
    CHECK(leaves >= 1 && leaves <= 16, "gradient/16: leaf count %d", leaves);
    palq_check_remap("gradient/16 byte", gradient, 16, 16, pal, 16, out, 16, 0, 0);

    if (failures) {
        fprintf(stderr, "palquant_test: %d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("palquant_test: %d checks passed\n", checks);
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(run_test, argc, argv);
}
