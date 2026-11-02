/* func_0038fb50 (468 bytes): writes the object arg0 (a mesh-set / lookup record) to the stream arg1 as a chunk of type 0x2C
 * holding a chunk of type 1; returns arg0, or 0 when a write fails. The sizes are 0x30 + 16 * n bytes (n = the halfword at +0x1E), plus 2 * m (m =
 * the halfword at +0x1C) when flag 1 of word 0 is set. The type 1 payload is the 0x24-byte header {word +0, floats +0x10, +0x14,
 * +0x18, floats +4, +8, +0xC, m, n}, then the n 16-byte entries (four floats each) of the array at +0x20 and, with flag 1, the m
 * halfwords at +0x24 (func_003df270). Uses func_003deff0 for the chunk headers (version 0x37002, 0x37) and func_003e2ab0 for the data. */

extern s32 p4c_003deff0_0038fb50(s32 s, s32 t, u32 size, u32 ver, u32 x) __asm__("func_003deff0");
extern s32 p4c_003df270_0038fb50(s32 s, u32 p, u32 n) __asm__("func_003df270");
extern s32 p4c_003e2ab0_0038fb50(s32 s, void *p, s32 n) __asm__("func_003e2ab0");

u8 *func_0038fb50(u8 *arg0, s32 arg1)
{
    u32 n = *(u16 *)(arg0 + 0x1E);
    u32 m = *(u16 *)(arg0 + 0x1C);
    u32 size = 0x30 + n * 16;
    u32 hdr[9];
    u32 ent[4];
    u32 i;
    u32 *e;

    if ((*(u32 *)arg0 & 1) != 0) {
        size += m * 2;
    }
    if (p4c_003deff0_0038fb50(arg1, 0x2C, size, 0x37002, 0x37) == 0) {
        return (u8 *)0;
    }
    if (p4c_003deff0_0038fb50(arg1, 1, size - 0xC, 0x37002, 0x37) == 0) {
        return (u8 *)0;
    }
    hdr[0] = *(u32 *)arg0;
    hdr[1] = *(u32 *)(arg0 + 0x10);
    hdr[2] = *(u32 *)(arg0 + 0x14);
    hdr[3] = *(u32 *)(arg0 + 0x18);
    hdr[4] = *(u32 *)(arg0 + 4);
    hdr[5] = *(u32 *)(arg0 + 8);
    hdr[6] = *(u32 *)(arg0 + 0xC);
    hdr[7] = m;
    hdr[8] = n;
    if (p4c_003e2ab0_0038fb50(arg1, hdr, 0x24) == 0) {
        return (u8 *)0;
    }
    for (i = 0; i < *(u16 *)(arg0 + 0x1E); i++) {
        e = (u32 *)(*(u8 **)(arg0 + 0x20) + i * 16);
        ent[0] = e[0];
        ent[1] = e[1];
        ent[2] = e[2];
        ent[3] = e[3];
        if (p4c_003e2ab0_0038fb50(arg1, ent, 0x10) == 0) {
            return (u8 *)0;
        }
    }
    if ((*(u32 *)arg0 & 1) != 0) {
        if (p4c_003df270_0038fb50(arg1, *(u32 *)(arg0 + 0x24), m * 2) == 0) {
            return (u8 *)0;
        }
    }
    return arg0;
}
