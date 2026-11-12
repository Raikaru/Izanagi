/* Palette quantizer: follow depth nibbles of key down the trie (nibble i is
 * (key >> 4i) & 0xf, four levels per recursion) and return the leaf index byte at +24.
 * Retail 003f1700. depth == 0 returns the root's index byte; the retail code leaves
 * $v0 unset on that path, and no caller reaches it with depth 0. */
u8 func_003f1700(u32 *node, u32 key, s32 depth)
{
    u32 *n;

    if (depth == 0) {
        return *((u8 *)node + 24);
    }

    n = (u32 *)node[7 + (key & 0xf)];
    if (depth == 1) {
        return *((u8 *)n + 24);
    }

    n = (u32 *)n[7 + ((key >> 4) & 0xf)];
    if (depth == 2) {
        return *((u8 *)n + 24);
    }

    n = (u32 *)n[7 + ((key >> 8) & 0xf)];
    if (depth == 3) {
        return *((u8 *)n + 24);
    }

    return func_003f1700((u32 *)n[7 + ((key >> 12) & 0xf)], key >> 16, depth - 4);
}
