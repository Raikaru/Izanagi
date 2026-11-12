/* func_00414ef0 (RxHeap init): clears the work count, walks the free-node list
 * at heap->data[2] carving each node through func_00414930, then carves the
 * whole data block. On success it sets work[6] = 0 and returns 1.
 *
 * Heap work object: w[1] = data block {base, size, list head}. Direct
 * transliteration of the retail control flow. */

s32 func_00414930(u32 *blk, u32 *prev, u32 *w);

s32 func_00414ef0(u32 *w)
{
    u32 *prev = NULL;
    u32 *node;
    u32 *blk;

    w[5] = 0;
    node = (u32 *)((u32 *)w[1])[2];
    while (node != NULL) {
        if (func_00414930(node, prev, w) == 0)
            return 0;
        if (prev == NULL)
            w[2] = node[0];
        prev = node;
        node = (u32 *)node[2];
    }

    blk = (u32 *)w[1];
    if (func_00414930(blk, prev, w) == 0)
        return 0;
    if (prev == NULL)
        w[2] = blk[0];
    w[6] = 0;
    return 1;
}
