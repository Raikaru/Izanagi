/* Palette quantizer: push one colour down the trie, one bit-plane per level.
 * depth <= 0: the node is a leaf, so accumulate px into it (003f0110) and return the node: retail keeps
 * the node in 168(sp) (stored in the delay slot of the depth test) and reloads it after the call, so the
 * parent merges every non-null child, leaves included. Returns 0 only for a null node.
 * depth > 0: clear the node header, then for each of the 16 child slots build the
 * sub-colour (channel k gets bit (3-k) of the slot index, shifted to this plane),
 * recurse, and merge a non-null result into this node (003f02d0). Returns node. */
extern void func_003f0110(float *leaf, const u8 *px);
extern void func_003f02d0(float *dst, const float *a, const float *b);

u32 *func_003f13b0(u32 *node, const u8 *px, s32 depth)
{
    s32 count = *(s32 *)(0x007690F0 - 21552);
    s32 i;

    if (node == 0) {
        return 0;
    }
    if (depth <= 0) {
        func_003f0110((float *)node, px);
        return node;
    }

    node[0] = 0;
    node[1] = 0;
    node[2] = 0;
    node[3] = 0;
    node[4] = 0;
    node[5] = 0;
    *((u8 *)node + 24) = 0;

    for (i = 0; i < 16; i++) {
        s32 sh = (8 - count) + (depth - 1);
        u8 sub[4];
        u32 *child;

        sub[0] = (u8)(px[0] + ((((u32)i >> 3) & 1) << sh));
        sub[1] = (u8)(px[1] + ((((u32)i >> 2) & 1) << sh));
        sub[2] = (u8)(px[2] + ((((u32)i >> 1) & 1) << sh));
        sub[3] = (u8)(px[3] + (((u32)i & 1) << sh));

        child = func_003f13b0((u32 *)node[7 + i], sub, depth - 1);
        if (child != 0) {
            func_003f02d0((float *)node, (float *)node, (float *)child);
        }
    }
    return node;
}
