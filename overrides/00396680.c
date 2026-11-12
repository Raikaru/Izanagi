/* func_00396680 (208 bytes): node-list callback. arg0 is a frame, arg1 a hierarchy record (+4 node
 * count, +0x10 array of 16-byte nodes). The first node whose +0 equals func_00397470(arg0) gets
 * its +0xC set to arg0 (a node after the first match is checked too: the scan continues, so every
 * matching node is set). Then func_003e9680(arg0) runs and func_003e9af0 walks the frame's
 * children with this function as the callback and arg1 as its argument. Returns arg0. */

u8 *func_00396680(u8 *arg0, u8 *arg1)
{
    s32 i;

    i = 0;
    if (0 < *(s32 *)(arg1 + 4)) {
        s32 off = 0;

        do {
            u8 *node = *(u8 **)(arg1 + 0x10) + off;
            s32 key = *(s32 *)node;

            if (key == func_00397470(arg0)) {
                *(u8 **)(node + 0xC) = arg0;
            }
            i += 1;
            off += 0x10;
        } while (i < *(s32 *)(arg1 + 4));
    }
    func_003e9680(arg0);
    func_003e9af0(arg0, func_00396680, arg1);
    return arg0;
}
