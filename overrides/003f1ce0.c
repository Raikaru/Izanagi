/* Palette quantizer: remap an image to palette indices (retail 003f1ce0).
 * Each pixel's colour is the 4 RGBA bytes of a 32-bit image, or the palette entry
 * named by the pixel byte of an 8- or 4-bit image (retail reads one byte per pixel
 * for 4-bit sources too). Channel bits are looked up in the 64-bit mask table at
 * 0x0088B400 to form the trie key, and the leaf index comes from palq_remap_leaf.
 * dst_bpp == 4 with pack == 1 packs two indices per byte (even pixel low nibble,
 * odd pixel high nibble); otherwise one index per byte. Other source bpps write
 * nothing. Retail has four copies of this loop (nibble/byte x RGBA/palette); they
 * differ only in output packing and colour source, so one loop covers them. */

typedef struct PalqImage {
    s32 unused0;
    s32 width;   /* +4 */
    s32 height;  /* +8 */
    s32 bpp;     /* +12 */
    s32 stride;  /* +16 */
    u8 *rows;    /* +20 */
    u8 *palette; /* +24 */
} PalqImage;

extern u8 func_003f1700(u32 *node, u32 key, s32 depth);

/* Walk the trie by key nibbles down to the leaf index. depth is the number of
 * channel levels (count); depth >= 3 hands the rest of the key to func_003f1700. */
static u8 palq_remap_leaf(u32 *root, u32 key, s32 depth)
{
    u32 *n1;
    u32 *n2;
    u32 *n3;

    if (depth == 0) {
        return *((u8 *)root + 24);
    }
    n1 = (u32 *)root[7 + (key & 0xf)];
    if (depth == 1) {
        return *((u8 *)n1 + 24);
    }
    n2 = (u32 *)n1[7 + ((key >> 4) & 0xf)];
    if (depth == 2) {
        return *((u8 *)n2 + 24);
    }
    n3 = (u32 *)n2[7 + ((key >> 8) & 0xf)];
    return func_003f1700(n3, key >> 12, depth - 3);
}

/* Build the trie key from the colour's 4 channel bytes, then walk it. */
static u8 palq_remap_colour(u8 *ctx, const u8 *c, s32 depth)
{
    const s64 *mask = (const s64 *)0x0088B400;
    s32 sh = 8 - depth;
    u32 key;
    u32 *root;

    key = ((u32)mask[c[0] >> sh] << 3) | ((u32)mask[c[1] >> sh] << 2) |
          ((u32)mask[c[2] >> sh] << 1) | (u32)mask[c[3] >> sh];
    root = *(u32 **)(ctx + 16384);
    return palq_remap_leaf(root, key, depth);
}

void func_003f1ce0(u8 *dst, s32 dst_stride, s32 dst_bpp, s32 pack, u8 *ctx, PalqImage *img)
{
    s32 depth = *(s32 *)(0x007690F0 - 21552);
    s32 nibble = (dst_bpp == 4 && pack == 1);
    s32 x;
    s32 y;

    if (img->bpp != 32 && img->bpp != 8 && img->bpp != 4) {
        return;
    }
    for (y = 0; y < img->height; y++) {
        const u8 *srow = img->rows + y * img->stride;
        u8 *drow = dst + y * dst_stride;
        u8 *o = drow;

        for (x = 0; x < img->width; x++) {
            const u8 *c = (img->bpp == 32) ? srow + 4 * x : img->palette + 4 * srow[x];
            u8 idx = palq_remap_colour(ctx, c, depth);

            if (!nibble) {
                drow[x] = idx;
            } else if ((x & 1) == 0) {
                *o = (u8)((*o & 0xf0) | (idx & 0xf));
            } else {
                *o = (u8)((*o & 0x0f) | ((idx & 0xf) << 4));
                o++;
            }
        }
    }
}
