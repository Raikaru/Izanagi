/* func_003ce9e0 (456 bytes): writes the native mesh data of a world sector (_rpWorldSectorNativeWrite); as func_003ce3a0 with
 * the owner object func_003c9c20(arg1) whose flags word (+8) carries bit 0x1000000, the mesh cache slot at arg1 + the offset word at
 * gp - 0x48D8 (0x00764818), the chunk size func_003ceeb0(arg1) - 0xC and the platform word at 0x00763BAC. Returns arg0, or 0 on a failed
 * write. */

extern u8 *p4c_003c9c20_003ce9e0(u8 *o) __asm__("func_003c9c20");
extern void p4c_003cc6e0_003ce9e0(u32 p) __asm__("func_003cc6e0");
extern u32 p4c_003ce170_003ce9e0(u8 *p) __asm__("func_003ce170");
extern void p4c_003ce230_003ce9e0(u8 *p) __asm__("func_003ce230");
extern u32 p4c_003ceeb0_003ce9e0(u8 *o) __asm__("func_003ceeb0");
extern s32 p4c_003deff0_003ce9e0(s32 s, s32 t, u32 size, u32 ver, u32 x) __asm__("func_003deff0");
extern s32 p4c_003e2ab0_003ce9e0(s32 s, void *p, s32 n) __asm__("func_003e2ab0");
extern void p4c_004220d8_003ce9e0(u32 a, u32 b) __asm__("func_004220d8");

s32 func_003ce9e0(s32 arg0, u8 *arg1)
{
    u32 *cache;
    u32 count;
    u32 i;
    u32 *b;
    u32 len;
    u32 tag;

    if ((*(u32 *)((u8 *)p4c_003c9c20_003ce9e0(arg1) + 8) & 0x1000000) == 0) {
        return arg0;
    }
    cache = *(u32 **)(arg1 + *(s32 *)0x00764818);
    count = cache[0];
    for (i = 0; i < count; i++) {
        p4c_003cc6e0_003ce9e0(cache[1 + i]);
    }
    if (p4c_003deff0_003ce9e0(arg0, 1, p4c_003ceeb0_003ce9e0(arg1) - 0xC, 0x37002, 0x37) == 0) {
        return 0;
    }
    if (p4c_003e2ab0_003ce9e0(arg0, (void *)0x00763BAC, 4) == 0) {
        return 0;
    }
    for (i = 0; i < cache[0]; i++) {
        b = (u32 *)cache[1 + i];
        len = b[2] - (b[8] - (u32)b);
        if (p4c_003e2ab0_003ce9e0(arg0, &len, 4) == 0) {
            return 0;
        }
        tag = p4c_003ce170_003ce9e0((u8 *)(b + 6));
        if (p4c_003e2ab0_003ce9e0(arg0, &tag, 4) == 0) {
            return 0;
        }
        if (p4c_003e2ab0_003ce9e0(arg0, (void *)b[8], (s32)len) == 0) {
            return 0;
        }
        p4c_003ce230_003ce9e0((u8 *)(b + 6));
        p4c_004220d8_003ce9e0((u32)b, (u32)b + b[2] + 0x18);
    }
    return arg0;
}
