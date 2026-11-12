/* func_003961f0 (232 bytes): reads a node table from the stream arg0 (the mirror of func_003962e0).
 * arg1: +4 count, +0x10 the nodes (0x24 bytes each). Returns arg1, or 0 on a failed read (func_003df300
 * reads a block, func_003df360 a word). For each node the 0x20 bytes at +4 are read, then a word v, and
 * node[0] = nodes + (v / 0x24) * 0x24 (the retail code divides by 36 with a multiply and shift). */

extern s32 p4c_003df300_003961f0(s32 stream, void *p, s32 n) __asm__("func_003df300");
extern s32 p4c_003df360_003961f0(s32 stream, void *p, s32 n) __asm__("func_003df360");

u8 *func_003961f0(s32 arg0, u8 *arg1)
{
    u8 *base;
    u8 *node;
    s32 i;

    base = *(u8 **)(arg1 + 0x10);
    if (!(*(s32 *)(arg1 + 4) > 0)) {
        return arg1;
    }
    node = base;
    i = 0;
    do {
        u32 v;

        if (p4c_003df300_003961f0(arg0, node + 4, 0x20) == 0) {
            return (u8 *)0;
        }
        if (p4c_003df360_003961f0(arg0, &v, 4) == 0) {
            return (u8 *)0;
        }
        i += 1;
        *(u32 *)node = (u32)base + (v / 36u) * 36u;
        node += 0x24;
    } while (i < *(s32 *)(arg1 + 4));
    return arg1;
}
