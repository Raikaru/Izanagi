/* _rpWorldSectorNativeRead (003cebb0, 768 bytes): reads the native (PS2) mesh data of the world sector arg1 from the stream arg0 and returns arg1, or 0 on
 * failure (_rpWorldSectorNativeRead; the same format as func_003ce560 reads for a geometry). func_003c9c20(arg1) is called first and its
 * result is not used. A chunk of type 1 with the version in [0x35000, 0x37003) is required (anything else reports {2, func_003df590(0x80000004)}
 * through func_003df4d0) followed by the platform word 4. The mesh cache of the sector (the slot at arg1 + the offset word at gp - 0x48D8,
 * 0x00764818) is reused when its first word equals the 16 bit word at world + 4 (world = arg1[0x78 / 4]), else func_003ce050(slot, that
 * 16 bit word) makes one (it takes the slot address); it holds a count and one block pointer per mesh. For each mesh: a length word len, then a
 * block of len + 0x247 bytes (allocator slot 0x008873E8, tag 0x3050D) with block[0] = block[1] = 0, block[2] = len + 0x22F, block[3] = arg1,
 * block[4] = the address of the mesh slot and block[5] = func_003cc6e0; a flag word; the 0x1B0 bytes at block + 0x18 are cleared (with words
 * 0 and 1 first), its word +8 is the payload address: (block + 0x18 + 0x1B0 + 0x3F) & ~0x3F, rounded up to 128 bytes with the flag; the len
 * payload bytes are read into it, func_003ce230(block + 0x18) post-processes it and func_004220d8(block, block + block[2] + 0x18) flushes
 * the cache range. */

extern void p4c_003cc6e0_003cebb0(u32 p) __asm__("func_003cc6e0");
extern void p4c_003c9c20_003cebb0(u8 *sector) __asm__("func_003c9c20");
extern u32 p4c_003ce050_003cebb0(u8 *o, u32 n) __asm__("func_003ce050");
extern void p4c_003ce230_003cebb0(u8 *p) __asm__("func_003ce230");
extern s32 p4c_003df050_003cebb0(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern void p4c_003df4d0_003cebb0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003cebb0(s32 code, ...) __asm__("func_003df590");
extern s32 p4c_003e2910_003cebb0(s32 s, void *b, s32 n) __asm__("func_003e2910");
extern void p4c_004220d8_003cebb0(u32 a, u32 b) __asm__("func_004220d8");
extern void p4c_0043f9c8_003cebb0(void *p, s32 v, s32 n) __asm__("func_0043f9c8");

typedef void *(*P4AllocFn_003cebb0)(u32 size, u32 tag);

s32 _rpWorldSectorNativeRead(s32 arg0, u8 *arg1)
{
    u8 *world;
    u32 version;
    u32 platform;
    u32 len;
    u32 flag;
    s32 err[2];
    u32 **slot;
    u32 *cache;
    u32 count;
    u32 i;
    u32 *block;
    u32 *entry;
    u8 *data;

    p4c_003c9c20_003cebb0(arg1);
    if (p4c_003df050_003cebb0(arg0, 1, 0, &version) == 0) {
        return 0;
    }
    if (version < 0x35000 || !(version < 0x37003)) {
        err[0] = 2;
        err[1] = p4c_003df590_003cebb0(0x80000004);
        p4c_003df4d0_003cebb0(err);
        return 0;
    }
    if (p4c_003e2910_003cebb0(arg0, &platform, 4) != 4) {
        return 0;
    }
    if (platform != 4) {
        return 0;
    }
    slot = (u32 **)(arg1 + *(s32 *)0x00764818);
    if (*slot != (u32 *)0 && *(u16 *)(*(u8 **)(arg1 + 0x78) + 4) == **slot) {
        cache = *slot;
    } else {
        world = *(u8 **)(arg1 + 0x78);
        slot = (u32 **)p4c_003ce050_003cebb0((u8 *)slot, *(u16 *)(world + 4));
        cache = *slot;
    }
    count = cache[0];
    if ((s32)count <= 0) {
        return (s32)arg1;
    }
    for (i = 0; i < count; i++, cache++) {
        if (p4c_003e2910_003cebb0(arg0, &len, 4) != 4) {
            return 0;
        }
        entry = cache + 1;
        block = (u32 *)(*(P4AllocFn_003cebb0 *)0x008873E8)(len + 0x247, 0x3050D);
        *entry = (u32)block;
        if (block == (u32 *)0) {
            return 0;
        }
        block[0] = 0;
        block[1] = 0;
        block[3] = (u32)arg1;
        block[2] = len + 0x22F;
        block[4] = (u32)entry;
        block[5] = (u32)p4c_003cc6e0_003cebb0;
        if (p4c_003e2910_003cebb0(arg0, &flag, 4) != 4) {
            return 0;
        }
        block[6] = 0;
        block[7] = 0;
        p4c_0043f9c8_003cebb0(block + 6, 0, 0x1B0);
        block[8] = ((u32)block + 0x18 + 0x1B0 + 0x3F) & ~0x3Fu;
        if (flag != 0) {
            block[8] = (block[8] + 0x7F) & ~0x7Fu;
        }
        data = (u8 *)block[8];
        if ((u32)p4c_003e2910_003cebb0(arg0, data, len) != len) {
            return 0;
        }
        p4c_003ce230_003cebb0((u8 *)(block + 6));
        p4c_004220d8_003cebb0(*entry, *entry + ((u32 *)*entry)[2] + 0x18);
    }
    return (s32)arg1;
}
