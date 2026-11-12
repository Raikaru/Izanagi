/* Palette quantizer: give each leaf below node the next palette index and write
 * its colour to pal[index] (003f0510), storing the index at leaf+24. Retail 003f1600.
 * Returns the next free index. 003f0510 leaves $v0 untouched, so idx survives it. */
extern void func_003f0510(u8 *out, const float *leaf);

s32 func_003f1600(u32 *node, u8 *pal, s32 idx, s32 depth)
{
    s32 i;
    s32 j;

    if (node == 0) {
        return idx;
    }
    if (depth <= 0) {
        func_003f0510(pal + idx * 4, (const float *)node);
        *((u8 *)node + 24) = (u8)idx;
        return idx + 1;
    }

    for (i = 0; i < 16; i++) {
        u32 *child = (u32 *)node[7 + i];

        if (child == 0) {
            continue;
        }
        if (depth - 1 > 0) {
            for (j = 0; j < 16; j++) {
                idx = func_003f1600((u32 *)child[7 + j], pal, idx, depth - 2);
            }
        } else {
            func_003f0510(pal + idx * 4, (const float *)child);
            *((u8 *)child + 24) = (u8)idx;
            idx++;
        }
    }
    return idx;
}
