/* func_003ce3a0 (448 bytes): writes the native (PS2) mesh data of the geometry arg1 to the stream arg0 and returns arg0 (0 on a
 * failed write); _rpGeometryNativeWrite. Geometry without bit 0x1000000 in its flags word (+8) writes nothing. Otherwise the mesh cache (slot
 * at arg1 + the offset word at gp - 0x48D0, 0x00764820: a count and one block pointer per mesh) has func_003cc6e0(block)
 * called for each block, a chunk of type 1 is opened (func_003deff0(stream, 1, func_003ce840(arg1) - 0xC, 0x37002, 0x37)) and the platform word
 * 4 (the word at 0x00763BA8) is written; then, per block b: the payload length b[2] - (b[8] - b), func_003ce170(b + 0x18) and the payload
 * (b[8], that length) are written with func_003e2ab0, followed by func_003ce230(b + 0x18) and the cache flush func_004220d8(b, b + b[2] + 0x18). */

extern void p4c_003cc6e0_003ce3a0(u32 p) __asm__("func_003cc6e0");
extern u32 p4c_003ce170_003ce3a0(u8 *p) __asm__("func_003ce170");
extern void p4c_003ce230_003ce3a0(u8 *p) __asm__("func_003ce230");
extern u32 p4c_003ce840_003ce3a0(u8 *o) __asm__("func_003ce840");
extern s32 p4c_003deff0_003ce3a0(s32 s, s32 t, u32 size, u32 ver, u32 x) __asm__("func_003deff0");
extern s32 p4c_003e2ab0_003ce3a0(s32 s, void *p, s32 n) __asm__("func_003e2ab0");
extern void p4c_004220d8_003ce3a0(u32 a, u32 b) __asm__("func_004220d8");

s32 func_003ce3a0(s32 arg0, u8 *arg1)
{
    u32 *cache;
    u32 count;
    u32 i;
    u32 *b;
    u32 len;
    u32 tag;

    if ((*(u32 *)(arg1 + 8) & 0x1000000) == 0) {
        return arg0;
    }
    cache = *(u32 **)(arg1 + *(s32 *)0x00764820);
    count = cache[0];
    for (i = 0; i < count; i++) {
        p4c_003cc6e0_003ce3a0(cache[1 + i]);
    }
    if (p4c_003deff0_003ce3a0(arg0, 1, p4c_003ce840_003ce3a0(arg1) - 0xC, 0x37002, 0x37) == 0) {
        return 0;
    }
    if (p4c_003e2ab0_003ce3a0(arg0, (void *)0x00763BA8, 4) == 0) {
        return 0;
    }
    for (i = 0; i < cache[0]; i++) {
        b = (u32 *)cache[1 + i];
        len = b[2] - (b[8] - (u32)b);
        if (p4c_003e2ab0_003ce3a0(arg0, &len, 4) == 0) {
            return 0;
        }
        tag = p4c_003ce170_003ce3a0((u8 *)(b + 6));
        if (p4c_003e2ab0_003ce3a0(arg0, &tag, 4) == 0) {
            return 0;
        }
        if (p4c_003e2ab0_003ce3a0(arg0, (void *)b[8], (s32)len) == 0) {
            return 0;
        }
        p4c_003ce230_003ce3a0((u8 *)(b + 6));
        p4c_004220d8_003ce3a0((u32)b, (u32)b + b[2] + 0x18);
    }
    return arg0;
}
