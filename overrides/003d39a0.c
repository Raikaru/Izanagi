/* func_003d39a0 (PS2AllMat node: set a callback slot): stores `fn` in word `slot - 1`
 * of the node's private data (node + 0x14), slots 1..5 (offsets 0, 4, 8, 0xC, 0x10).
 * Slots 2, 3 and 4 refuse a NULL `fn` and return NULL, as does a NULL node or private data
 * or a slot outside 1..5. Slots 1 and 5 accept NULL. Returns the node.
 *
 * Direct transliteration of the retail code (the decomp's NON_MATCHING body has the same
 * behaviour). */

u8 *func_003d39a0(u8 *node, s32 slot, s32 fn)
{
    u8 *priv;

    if (node == NULL)
        return NULL;
    priv = *(u8 **)(node + 0x14);
    if (priv == NULL)
        return NULL;

    switch (slot) {
    case 1:
        *(s32 *)(priv + 0x0) = fn;
        break;
    case 2:
        if (fn == 0)
            return NULL;
        *(s32 *)(priv + 0x4) = fn;
        break;
    case 3:
        if (fn == 0)
            return NULL;
        *(s32 *)(priv + 0x8) = fn;
        break;
    case 4:
        if (fn == 0)
            return NULL;
        *(s32 *)(priv + 0xC) = fn;
        break;
    case 5:
        *(s32 *)(priv + 0x10) = fn;
        break;
    default:
        return NULL;
    }
    return node;
}
