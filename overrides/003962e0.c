/* func_003962e0 (200 bytes): writes a node table to the stream arg1. arg0: +4 count, +0x10 the nodes
 * (0x24 bytes each). Returns 1 when all writes succeed, 0 on a failed write
 * (func_003df1a0 writes a block, func_003df240 a word). For each node: its bytes +4..+0x23 (0x20 bytes), then
 * the word (node[0] - nodes), the node's first word as an offset from the table start. */

extern s32 p4c_003df1a0_003962e0(s32 stream, void *p, s32 n) __asm__("func_003df1a0");
extern s32 p4c_003df240_003962e0(s32 stream, void *p, s32 n) __asm__("func_003df240");

s32 func_003962e0(u8 *arg0, s32 arg1)
{
    u8 *base;
    u8 *node;
    s32 i;

    base = *(u8 **)(arg0 + 0x10);
    if (!(*(s32 *)(arg0 + 4) > 0)) {
        return 1;
    }
    node = base;
    i = 0;
    do {
        s32 off;

        if (p4c_003df1a0_003962e0(arg1, node + 4, 0x20) == 0) {
            return 0;
        }
        off = (s32)(*(u32 *)node - (u32)base);
        if (p4c_003df240_003962e0(arg1, &off, 4) == 0) {
            return 0;
        }
        i += 1;
        node += 0x24;
    } while (i < *(s32 *)(arg0 + 4));
    return 1;
}
