/* Palette quantizer: give every leaf whose cell overlaps the bounds box the palette
 * index in `index` (stored at leaf+24). Retail 003f0c10. box[0..3] is the cell origin,
 * cell size is 1 << depth. bounds[0..3] is L and bounds[4..7] is H, as in 003f0de0. */
void func_003f0c10(u32 *node, const s32 *box, s32 depth, const s32 *bounds, s32 index)
{
    s32 size;
    s32 k;
    s32 i;

    if (node == 0) {
        return;
    }

    size = 1 << depth;
    for (k = 0; k < 4; k++) {
        if (box[k] + size <= bounds[k]) {
            return;
        }
    }
    for (k = 0; k < 4; k++) {
        if (box[k] >= bounds[4 + k]) {
            return;
        }
    }

    if (depth == 0) {
        *((u8 *)node + 24) = (u8)index;
        return;
    }

    for (i = 0; i < 16; i++) {
        s32 cb[4];

        cb[0] = box[0] + ((((u32)i >> 3) & 1) << (depth - 1));
        cb[1] = box[1] + ((((u32)i >> 2) & 1) << (depth - 1));
        cb[2] = box[2] + ((((u32)i >> 1) & 1) << (depth - 1));
        cb[3] = box[3] + (((u32)i & 1) << (depth - 1));
        func_003f0c10((u32 *)node[7 + i], cb, depth - 1, bounds, index);
    }
}
