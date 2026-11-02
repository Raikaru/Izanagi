/* func_003bee80 (796 bytes): reads a counted block of material references from the stream arg0 into arg1, a
 * {items, count} pair. The block is a chunk of type 1 with the version in [0x35000, 0x37003) (anything else
 * reports error 0x80000004 through func_003df590/func_003df4d0 and returns 0), holding a word n followed by n
 * chunks of type 0xF, each read by func_003c30b0. items is an array of n words allocated through the allocator slot
 * 0x008873E8 (tag 0x3000F; failure reports 0x80000013 with the byte size); count counts the references read so far. Returns arg1, with
 * count == n, or 0 on any failure after releasing what was read (func_003c2a80 for each reference, the array freed
 * through 0x008873EC and items cleared). n <= 0 gives an empty block (items 0, count 0). */

extern s32 p4c_003c2a80_003bee80(u32 m) __asm__("func_003c2a80");
extern u32 p4c_003c30b0_003bee80(s32 stream) __asm__("func_003c30b0");
extern s32 p4c_003df050_003bee80(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern void p4c_003df4d0_003bee80(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003bee80(s32 code, ...) __asm__("func_003df590");
extern s32 p4c_003e2910_003bee80(s32 stream, void *buf, s32 n) __asm__("func_003e2910");

typedef struct P4MatList_003bee80 {
    u32 *items;
    s32 count;
} P4MatList_003bee80;

typedef void *(*P4AllocFn_003bee80)(u32 size, u32 tag);
typedef void (*P4FreeFn_003bee80)(void *p);

static void p4_ee80_release(P4MatList_003bee80 *out)
{
    s32 i;

    if (out->count > 0) {
        for (i = 0; i < out->count; i++) {
            p4c_003c2a80_003bee80(out->items[i]);
        }
    }
    if (out->items != (u32 *)0) {
        (*(P4FreeFn_003bee80 *)0x008873EC)(out->items);
        out->items = (u32 *)0;
    }
}

P4MatList_003bee80 *func_003bee80(s32 arg0, P4MatList_003bee80 *arg1)
{
    u32 size;
    u32 version;
    s32 n;
    s32 err[2];
    s32 i;

    if (p4c_003df050_003bee80(arg0, 1, &size, &version) == 0) {
        return (P4MatList_003bee80 *)0;
    }
    if (version < 0x35000 || !(version < 0x37003)) {
        err[0] = 2;
        err[1] = p4c_003df590_003bee80(0x80000004);
        p4c_003df4d0_003bee80(err);
        return (P4MatList_003bee80 *)0;
    }
    if (p4c_003e2910_003bee80(arg0, &n, 4) != 4) {
        return (P4MatList_003bee80 *)0;
    }
    arg1->count = 0;
    if (n <= 0) {
        arg1->items = (u32 *)0;
        return arg1;
    }
    arg1->items = (u32 *)(*(P4AllocFn_003bee80 *)0x008873E8)(n * 4, 0x3000F);
    if (arg1->items == (u32 *)0) {
        err[0] = 2;
        err[1] = p4c_003df590_003bee80(0x80000013, n * 4);
        p4c_003df4d0_003bee80(err);
        return (P4MatList_003bee80 *)0;
    }
    for (i = 0; i < n; i++) {
        u32 m;

        if (p4c_003df050_003bee80(arg0, 0xF, 0, &version) == 0) {
            p4_ee80_release(arg1);
            return (P4MatList_003bee80 *)0;
        }
        if (version < 0x35000 || !(version < 0x37003)) {
            p4_ee80_release(arg1);
            return (P4MatList_003bee80 *)0;
        }
        m = p4c_003c30b0_003bee80(arg0);
        arg1->items[i] = m;
        if (m == 0) {
            p4_ee80_release(arg1);
            return (P4MatList_003bee80 *)0;
        }
        arg1->count++;
    }
    return arg1;
}
