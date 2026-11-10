/* Palette quantizer: accumulate every pixel of an image into the octree.
 * img: +4 pixels per row, +8 rows, +12 bpp (32 raw RGBA, 8/4 palette index),
 *      +16 row stride, +20 row base, +24 palette (4-byte entries).
 * w is the sample weight (retail $f12, kept in $f20 across the loops). */
extern u8 *func_003f0610(u8 *ctx, u32 *node, u64 key, s32 depth);
extern void func_003eff30(float *leaf, const u8 *px, float w);
u32 *palquant_new_node(u8 *ctx, s32 leaf);
static u64 palquant_key(const u8 *px, s32 count);
static void palquant_add(u8 *ctx, const u8 *px, s32 count, float w);

void func_003f07d0(u8 *ctx, u8 *img, float w)
{
    s32 count = *(s32 *)(0x007690F0 - 21552);
    u32 pixels = *(u32 *)(img + 4);
    u32 rows = *(u32 *)(img + 8);
    u32 bpp = *(u32 *)(img + 12);
    u32 stride = *(u32 *)(img + 16);
    u8 *row = *(u8 **)(img + 20);
    u8 *pal = *(u8 **)(img + 24);
    u32 r;
    u32 p;

    if (bpp == 32) {
        for (r = 0; r < rows; r++) {
            u8 *px = row;

            for (p = 0; p < pixels; p++) {
                palquant_add(ctx, px, count, w);
                px += 4;
            }
            row += stride;
        }
    } else if (bpp == 8 || bpp == 4) {
        for (r = 0; r < rows; r++) {
            u8 *idx = row;

            for (p = 0; p < pixels; p++) {
                palquant_add(ctx, pal + (u32)*idx * 4, count, w);
                idx++;
            }
            row += stride;
        }
    }
}

/* Node allocator shared with 003f0610 (retail D_008873F8[0] with flags 0x30411).
 * Defined here once; 003f0610 takes it as an extern. */
u32 *palquant_new_node(u8 *ctx, s32 leaf)
{
    u32 *node = (u32 *)D_008873F8[0](*(u8 **)(ctx + 16388), 0x30411);
    s32 i;

    for (i = 7; i < 23; i++) {
        node[i] = 0;
    }
    if (leaf) {
        node[0] = 0;
        node[1] = 0;
        node[2] = 0;
        node[3] = 0;
        node[4] = 0;
        node[5] = 0;
        *((u8 *)node + 24) = 0;
    }
    return node;
}

/* Trie key: 4 channels, each shifted to count significant bits, looked up in the
 * 8-byte mask table at 0x0088B400; channel 0 supplies the most significant bits. */
static u64 palquant_key(const u8 *px, s32 count)
{
    const u64 *tbl = (const u64 *)0x0088B400;
    s32 sh = 8 - count;
    u64 key;

    key = (tbl[px[0] >> sh] << 3) | (tbl[px[1] >> sh] << 2);
    key |= (tbl[px[2] >> sh] << 1) | tbl[px[3] >> sh];
    return key;
}

static void palquant_add(u8 *ctx, const u8 *px, s32 count, float w)
{
    u32 *root = *(u32 **)(ctx + 16384);
    u8 *leaf;

    if (count != 0) {
        u64 key = palquant_key(px, count);
        s32 slot = (s32)(key & 0xf);

        if (root[7 + slot] == 0) {
            root[7 + slot] = (u32)palquant_new_node(ctx, count == 1);
        }
        leaf = func_003f0610(ctx, (u32 *)root[7 + slot], key >> 4, count - 1);
    } else {
        leaf = (u8 *)root;
    }
    func_003eff30((float *)leaf, px, w);
}
