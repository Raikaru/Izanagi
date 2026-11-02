/* func_003ceeb0 (436 bytes): size of the native mesh data of a world sector (_rpWorldSectorNativeSize). 0 unless the owner
 * object func_003c9c20(arg0) has bit 0x1000000 in its flags word (+8); otherwise 0x10 plus, for each block b of the mesh cache at arg0 + the offset word
 * at gp - 0x48D8 (0x00764818), the payload length b[2] - (b[8] - b) plus 8. */

extern u8 *p4c_003c9c20_003ceeb0(u8 *o) __asm__("func_003c9c20");

u32 func_003ceeb0(u8 *arg0)
{
    u32 size = 0;
    u32 *cache;
    u32 i;
    u32 *b;

    if ((*(u32 *)((u8 *)p4c_003c9c20_003ceeb0(arg0) + 8) & 0x1000000) == 0) {
        return 0;
    }
    size += 0x10;
    cache = *(u32 **)(arg0 + *(s32 *)0x00764818);
    for (i = 0; i < cache[0]; i++) {
        b = (u32 *)cache[1 + i];
        size += (b[2] - (b[8] - (u32)b)) + 8;
    }
    return size;
}
