/* func_00397390 (196 bytes): binds a hierarchy record (arg0: +4 node count, +0x10 array of
 * 16-byte nodes, +0x14 the root frame) to its frame tree. Every node whose +0 equals
 * func_00397470(root) gets +0xC = root; then func_003e9680(root) runs and func_003e9af0 walks the
 * root's children with func_00396680 as the callback and arg0 as its argument. Returns arg0. */

u8 *func_00396680(u8 *arg0, u8 *arg1);

u8 *func_00397390(u8 *arg0)
{
    u8 *root;
    s32 i;

    root = *(u8 **)(arg0 + 0x14);
    i = 0;
    if (0 < *(s32 *)(arg0 + 4)) {
        s32 off = 0;

        do {
            u8 *node = *(u8 **)(arg0 + 0x10) + off;
            s32 key = *(s32 *)node;

            if (key == func_00397470(root)) {
                *(u8 **)(node + 0xC) = root;
            }
            i += 1;
            off += 0x10;
        } while (i < *(s32 *)(arg0 + 4));
    }
    func_003e9680(root);
    func_003e9af0(root, func_00396680, arg0);
    return arg0;
}
