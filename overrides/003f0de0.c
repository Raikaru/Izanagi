/* Palette quantizer: accumulate the trie node into out when its cell lies wholly
 * inside the bounds box, else recurse into children that overlap it. Retail 003f0de0.
 * box[0..3] is the cell origin, cell size is 1 << depth. bounds[0..3] is the lower
 * corner (L) and bounds[4..7] the upper corner (H), both exclusive as in retail. */
extern void func_003f02d0(float *dst, const float *a, const float *b);

void func_003f0de0(u32 *node, const s32 *box, s32 depth, const s32 *bounds, float *out)
{
    s32 size;
    s32 x[4];
    s32 contained = 1;
    s32 i;
    s32 k;

    if (node == 0) {
        return;
    }

    size = 1 << depth;
    for (k = 0; k < 4; k++) {
        x[k] = box[k];
    }

    /* Reject cells that miss the box: need x + size > L and x < H in every channel. */
    for (k = 0; k < 4; k++) {
        if (x[k] + size <= bounds[k] || x[k] >= bounds[4 + k]) {
            return;
        }
    }

    for (k = 0; k < 4; k++) {
        if (x[k] < bounds[k] || x[k] + size > bounds[4 + k]) {
            contained = 0;
        }
    }
    if (contained) {
        func_003f02d0(out, out, (const float *)node);
        return;
    }
    if (depth <= 0) {
        return;
    }

    for (i = 0; i < 16; i++) {
        s32 cb[4];

        cb[0] = x[0] + ((((u32)i >> 3) & 1) << (depth - 1));
        cb[1] = x[1] + ((((u32)i >> 2) & 1) << (depth - 1));
        cb[2] = x[2] + ((((u32)i >> 1) & 1) << (depth - 1));
        cb[3] = x[3] + (((u32)i & 1) << (depth - 1));
        func_003f0de0((u32 *)node[7 + i], cb, depth - 1, bounds, out);
    }
}
