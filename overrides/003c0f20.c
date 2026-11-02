/* func_003c0f20 (2948 bytes): reads a clump from the stream arg0. Returns the clump, or 0 after reporting
 * the error {2, func_003df590(code)} through func_003df4d0 (0x8000001A for read failures, 0x80000004 for an
 * unsupported version).
 *
 * Layout of the stream: chunk type 1 holding a 12-byte header (three counts: nA, nB, nC); chunk 0xE; the
 * shared table (func_003e5df0 fills `tab`, a {items, count} pair like the material block, freed again with func_003e59e0); chunk
 * 0x1A; the material block (func_003bee80 fills `mats` and its count `nMats`); then nA objects of chunk 0x14
 * (func_003bf5f0(stream, &tab, &mats), linked at the head of the clump's list at +8, the item's link at +0x40
 * and its owner at +0x3C), nB chunks (type 1) each holding an index chunk 0x12: a light read by func_003c3ae0
 * (func_003efd20(light, tab[index]), linked into the list at clump + 0x10 through the plugin offset at 0x007647A4)
 * and nC chunks (type 1) holding an index chunk 5 whose object is read by func_003e53b0 (func_003efd20(obj,
 * tab[index]), linked into the list at clump + 0x18 through the plugin offset at 0x007647A0). The version must be at least
 * 0x35000 and below 0x37003.
 *
 * The clump itself is created by func_003c0640 and its +4 is set to the first word of the table. On any failure
 * the references taken in the material block are released (func_003c2a80 each, the block freed), the
 * table is freed, and the clump is destroyed: func_003e3c20(0x0070AF90, clump), RpClumpForAllAtomics, func_003c0050 and
 * func_003bffc0 with the callbacks func_003bf330, func_003bf200 and func_003bf240, func_003e95c0(clump[4]) when
 * set, and the clump is returned to the freelist through the slot 0x008873FC (pool = *(0x008872E0 +
 * *(s32 *)0x007647B0) + 4). On success the material references are released the same way and
 * func_003e3110(0x0070AF90, stream, clump) registers the clump; a failure there destroys it as above. */

extern u8 *p4c_003c3ae0_003c0f20(s32 stream) __asm__("func_003c3ae0");
extern void p4c_003e95c0_003c0f20(u32 a) __asm__("func_003e95c0");
extern s32 p4c_003bee80_003c0f20(s32 stream, void *mats) __asm__("func_003bee80");
extern u8 *p4c_003bf5f0_003c0f20(s32 stream, void *tab, void *mats) __asm__("func_003bf5f0");
extern void p4c_003bff30_003c0f20(u8 *c, void *cb, s32 z) __asm__("RpClumpForAllAtomics");
extern void p4c_003bffc0_003c0f20(u8 *c, void *cb, s32 z) __asm__("func_003bffc0");
extern void p4c_003c0050_003c0f20(u8 *c, void *cb, s32 z) __asm__("func_003c0050");
extern u8 *p4c_003c0640_003c0f20(u8 *o) __asm__("func_003c0640");
extern s32 p4c_003c2a80_003c0f20(u32 m) __asm__("func_003c2a80");
extern s32 p4c_003df050_003c0f20(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern s32 p4c_003df360_003c0f20(s32 stream, void *p, s32 n) __asm__("func_003df360");
extern void p4c_003df4d0_003c0f20(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003c0f20(s32 code, ...) __asm__("func_003df590");
extern s32 p4c_003e2910_003c0f20(s32 stream, void *buf, s32 n) __asm__("func_003e2910");
extern s32 p4c_003e3110_003c0f20(u32 list, s32 stream, void *obj) __asm__("func_003e3110");
extern void p4c_003e3c20_003c0f20(u32 list, void *obj) __asm__("func_003e3c20");
extern u8 *p4c_003e53b0_003c0f20(s32 stream) __asm__("func_003e53b0");
extern void p4c_003e59e0_003c0f20(void *tab) __asm__("func_003e59e0");
extern s32 p4c_003e5df0_003c0f20(s32 stream, void *tab) __asm__("func_003e5df0");
extern void p4c_003efd20_003c0f20(u8 *o, u32 t) __asm__("func_003efd20");

typedef void (*P4FreeListFn_003c0f20)(u32 pool, u8 *obj);
typedef void (*P4FreeFn_003c0f20)(void *p);

extern void p4c_003bf330_003c0f20(void) __asm__("func_003bf330");
extern void p4c_003bf200_003c0f20(void) __asm__("func_003bf200");
extern void p4c_003bf240_003c0f20(void) __asm__("func_003bf240");

static void p4_0f20_error(u32 code)
{
    s32 err[2];

    err[0] = 2;
    err[1] = p4c_003df590_003c0f20(code);
    p4c_003df4d0_003c0f20(err);
}

static void p4_0f20_destroy(u8 *clump)
{
    p4c_003e3c20_003c0f20(0x0070AF90, clump);
    p4c_003bff30_003c0f20(clump, p4c_003bf330_003c0f20, 0);
    p4c_003c0050_003c0f20(clump, p4c_003bf200_003c0f20, 0);
    p4c_003bffc0_003c0f20(clump, p4c_003bf240_003c0f20, 0);
    if (*(u32 *)(clump + 4) != 0) {
        p4c_003e95c0_003c0f20(*(u32 *)(clump + 4));
    }
    (*(P4FreeListFn_003c0f20 *)0x008873FC)(*(u32 *)(0x008872E0 + *(s32 *)0x007647B0 + 4), clump);
}

typedef struct P4MatList_003c0f20 {
    u32 *items;
    s32 count;
} P4MatList_003c0f20;

static void p4_0f20_free_mats(P4MatList_003c0f20 *mats)
{
    s32 i;

    if (mats->count > 0) {
        for (i = 0; i < mats->count; i++) {
            p4c_003c2a80_003c0f20(mats->items[i]);
        }
    }
    if (mats->items != (u32 *)0) {
        (*(P4FreeFn_003c0f20 *)0x008873EC)(mats->items);
        mats->items = (u32 *)0;
    }
}

u8 *func_003c0f20(s32 arg0)
{
    u32 size;
    u32 version;
    u32 hdr[3];
    u8 *clump;
    P4MatList_003c0f20 tab;
    P4MatList_003c0f20 mats;
    u32 chunkVer;
    u32 idx;
    s32 i;
    u8 *item;

    if (p4c_003df050_003c0f20(arg0, 1, &size, &version) == 0) {
        p4_0f20_error(0x8000001A);
        return (u8 *)0;
    }
    if (version < 0x35000 || !(version < 0x37003)) {
        p4_0f20_error(0x80000004);
        return (u8 *)0;
    }
    if (p4c_003e2910_003c0f20(arg0, hdr, 12) != 12) {
        p4_0f20_error(0x8000001A);
        return (u8 *)0;
    }
    clump = (u8 *)p4c_003c0640_003c0f20(0);
    if (clump == (u8 *)0) {
        return (u8 *)0;
    }
    if (p4c_003df050_003c0f20(arg0, 0xE, 0, &chunkVer) == 0) {
        p4_0f20_destroy(clump);
        p4_0f20_error(0x8000001A);
        return (u8 *)0;
    }
    if (p4c_003e5df0_003c0f20(arg0, &tab) == 0) {
        p4_0f20_destroy(clump);
        p4_0f20_error(0x8000001A);
        return (u8 *)0;
    }
    *(u32 *)(clump + 4) = *tab.items;
    if (p4c_003df050_003c0f20(arg0, 0x1A, 0, &chunkVer) == 0) {
        p4c_003e59e0_003c0f20(&tab);
        p4_0f20_destroy(clump);
        p4_0f20_error(0x8000001A);
        return (u8 *)0;
    }
    mats.items = (u32 *)0;
    mats.count = 0;
    if (p4c_003bee80_003c0f20(arg0, &mats) == 0) {
        p4c_003e59e0_003c0f20(&tab);
        p4_0f20_destroy(clump);
        p4_0f20_error(0x8000001A);
        return (u8 *)0;
    }

    for (i = 0; i < (s32)hdr[0]; i++) {
        item = (u8 *)0;
        if (p4c_003df050_003c0f20(arg0, 0x14, 0, &version) != 0) {
            item = (u8 *)p4c_003bf5f0_003c0f20(arg0, &tab, &mats);
        }
        if (item == (u8 *)0) {
            goto fail;
        }
        *(u32 *)(item + 0x40) = *(u32 *)(clump + 8);
        *(u8 **)(item + 0x44) = clump + 8;
        *(u32 *)(*(u8 **)(clump + 8) + 4) = (u32)(item + 0x40);
        *(u32 *)(clump + 8) = (u32)(item + 0x40);
        *(u8 **)(item + 0x3C) = clump;
    }
    for (i = 0; i < (s32)hdr[1]; i++) {
        u8 *light = (u8 *)0;

        if (p4c_003df050_003c0f20(arg0, 1, 0, 0) != 0 && p4c_003df360_003c0f20(arg0, &idx, 4) != 0) {
            if (p4c_003df050_003c0f20(arg0, 0x12, 0, 0) != 0) {
                light = (u8 *)p4c_003c3ae0_003c0f20(arg0);
            }
        }
        if (light == (u8 *)0) {
            goto fail;
        }
        p4c_003efd20_003c0f20(light, tab.items[idx]);
        {
            u8 *e = light + *(s32 *)0x007647A4;

            *(u32 *)(e + 4) = *(u32 *)(clump + 0x10);
            *(u8 **)(e + 8) = clump + 0x10;
            *(u32 *)(*(u8 **)(clump + 0x10) + 4) = (u32)(e + 4);
            *(u32 *)(clump + 0x10) = (u32)(e + 4);
            *(u8 **)e = clump;
        }
    }
    for (i = 0; i < (s32)hdr[2]; i++) {
        u8 *obj = (u8 *)0;

        if (p4c_003df050_003c0f20(arg0, 1, 0, 0) != 0 && p4c_003df360_003c0f20(arg0, &idx, 4) != 0) {
            if (p4c_003df050_003c0f20(arg0, 5, 0, 0) != 0) {
                obj = (u8 *)p4c_003e53b0_003c0f20(arg0);
            }
        }
        if (obj == (u8 *)0) {
            goto fail;
        }
        p4c_003efd20_003c0f20(obj, tab.items[idx]);
        {
            u8 *e = obj + *(s32 *)0x007647A0;

            *(u32 *)(e + 4) = *(u32 *)(clump + 0x18);
            *(u8 **)(e + 8) = clump + 0x18;
            *(u32 *)(*(u8 **)(clump + 0x18) + 4) = (u32)(e + 4);
            *(u32 *)(clump + 0x18) = (u32)(e + 4);
            *(u8 **)e = clump;
        }
    }

    p4_0f20_free_mats(&mats);
    p4c_003e59e0_003c0f20(&tab);
    if (p4c_003e3110_003c0f20(0x0070AF90, arg0, clump) == 0) {
        p4_0f20_destroy(clump);
        p4_0f20_error(0x8000001A);
        return (u8 *)0;
    }
    return clump;

fail:
    p4_0f20_free_mats(&mats);
    p4c_003e59e0_003c0f20(&tab);
    p4_0f20_destroy(clump);
    p4_0f20_error(0x8000001A);
    return (u8 *)0;
}
