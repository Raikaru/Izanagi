extern s32 func_004118c0(u8 *arg0, u32 arg1);
extern s32 func_00411a70(u8 *arg0, u8 *arg1);
extern s32 func_003df590();
extern u8 *func_003df4d0();

/* func_00412ca0: opens a registry once (word 0 is the opened flag; an opened registry is
 * returned as is). The entry table (word 2, 40-byte entries, capacity word 9, storage word 8)
 * must hold the engine's per-instance count (*(0x00764AA8 + 0x887318)) * 180 bytes: it is
 * allocated through the engine callback at retail slot 0x008873E8 (flags 0x1030409), or grown
 * through func_004118c0 when too small, and then func_00411a70 is run on it. Allocation failure
 * reports error {1, func_003df590(0x80000013, size)} and returns 0. Once open, every entry's
 * record (word 0 of the entry) gets its +0x14 callback run with the entry slot, its use count at
 * +0x3c decremented, its +0xc callback run with the record when the count reaches 0, and the
 * entry's word 3 cleared. */
u8 *func_00412ca0(u8 *reg)
{
    u32 size;
    s32 err[2];
    u32 i;
    s32 off;
    u8 *rec;
    s32 count;
    void (*cb)(void *);

    if (*(s32 *)reg != 0) {
        return reg;
    }
    size = *(s32 *)(*(s32 *)0x00764AA8 + 0x887318) * 0xB4;
    if (*(s32 *)(reg + 8) != 0) {
        if (*(u32 *)(reg + 0x24) < size && func_004118c0(reg, size) == 0) {
            return 0;
        }
        if (func_00411a70(reg, reg) == 0) {
            return 0;
        }
    } else {
        *(s32 *)(reg + 0x20) = (*(s32 (**)(u32, u32))0x008873E8)(size, 0x1030409);
        if (*(s32 *)(reg + 0x20) == 0) {
            err[0] = 1;
            err[1] = func_003df590(0x80000013, size);
            func_003df4d0(err);
            return 0;
        }
        *(u32 *)(reg + 0x24) = size;
        *(s32 *)(reg + 8) = *(s32 *)(reg + 0x20);
    }
    *(s32 *)reg = 1;
    if (*(s32 *)(reg + 8) != 0 && *(u32 *)(reg + 4) != 0) {
        off = 0;
        for (i = 0; i < *(u32 *)(reg + 4); i++) {
            rec = *(u8 **)(*(s32 *)(reg + 8) + off);
            cb = *(void (**)(void *))(rec + 0x14);
            if (cb != 0) {
                cb((void *)(*(s32 *)(reg + 8) + off));
            }
            rec = *(u8 **)(*(s32 *)(reg + 8) + off);
            count = *(s32 *)(rec + 0x3C) - 1;
            *(s32 *)(rec + 0x3C) = count;
            if (count == 0) {
                cb = *(void (**)(void *))(rec + 0xC);
                if (cb != 0) {
                    cb(*(void **)(*(s32 *)(reg + 8) + off));
                }
            }
            *(s32 *)(*(s32 *)(reg + 8) + off + 0xC) = 0;
            off += 0x28;
        }
    }
    return reg;
}
