/* func_003c4a80 (308 bytes): appends an item to a growable pointer array. arg0: +0 data, +4 count,
 * +8 capacity; arg1 is the item, whose halfword +0x18 is a reference count. Returns the new index of the
 * item, or -1 when growing fails.
 *
 * When count >= capacity the array grows by 0x14 entries: realloc through the slot at 0x008873F0
 * (data, bytes, tag 0x1030008) when data is set, else allocate through 0x008873E8 (bytes, tag
 * 0x1030008); bytes = 4 * new capacity. A failed allocation reports error
 * {2, func_003df590(0x80000013, bytes)} through func_003df4d0. Then data[count] = item, the item's
 * reference count is incremented and count grows by one. */

extern void p4c_003df4d0_003c4a80(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003c4a80(s32 code, ...) __asm__("func_003df590");

typedef void *(*P4ReallocFn_003c4a80)(void *p, u32 size, u32 tag);
typedef void *(*P4AllocFn_003c4a80)(u32 size, u32 tag);

s32 func_003c4a80(u8 *arg0, u8 *arg1)
{
    s32 count;
    s32 cap;
    u32 bytes;
    u8 *data;

    count = *(s32 *)(arg0 + 4);
    cap = *(s32 *)(arg0 + 8);
    if (!(count < cap)) {
        bytes = (u32)((cap + 0x14) << 2);
        if (*(u32 *)arg0 != 0) {
            data = (u8 *)(*(P4ReallocFn_003c4a80 *)0x008873F0)(*(void **)arg0, bytes, 0x1030008);
        } else {
            data = (u8 *)(*(P4AllocFn_003c4a80 *)0x008873E8)(bytes, 0x1030008);
        }
        if (data == (u8 *)0) {
            s32 err[2];

            err[0] = 2;
            err[1] = p4c_003df590_003c4a80(0x80000013, bytes);
            p4c_003df4d0_003c4a80(err);
            return -1;
        }
        *(u8 **)arg0 = data;
        *(s32 *)(arg0 + 8) = *(s32 *)(arg0 + 8) + 0x14;
    }
    *(u8 **)(*(u8 **)arg0 + (*(s32 *)(arg0 + 4) << 2)) = arg1;
    *(s16 *)(arg1 + 0x18) = (s16)(*(s16 *)(arg1 + 0x18) + 1);
    *(s32 *)(arg0 + 4) = *(s32 *)(arg0 + 4) + 1;
    return *(s32 *)(arg0 + 4) - 1;
}
