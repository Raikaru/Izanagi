/* Palette quantizer: count palette leaves below a trie node. Retail 003f1520.
 * A null node counts 0; depth <= 0 counts 1. Otherwise each non-null child
 * counts 1 when depth == 1, else the sum of its non-null grandchildren's counts
 * at depth - 2. */
s32 func_003f1520(u32 *node, s32 depth)
{
    s32 total = 0;
    s32 i;
    s32 j;

    if (node == 0) {
        return 0;
    }
    if (depth <= 0) {
        return 1;
    }

    for (i = 0; i < 16; i++) {
        u32 *child = (u32 *)node[7 + i];
        s32 n = 0;

        if (child != 0) {
            if (depth - 1 > 0) {
                for (j = 0; j < 16; j++) {
                    n += func_003f1520((u32 *)child[7 + j], depth - 2);
                }
            } else {
                n = 1;
            }
        }
        total += n;
    }
    return total;
}
