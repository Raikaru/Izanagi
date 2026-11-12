/* func_00395100 (400 bytes): allocates the arg2-entry array of slot arg1 of a table (arg0: +4 the
 * slots, 8 bytes each: +0 entry count, +4 entry array). The array is 20*arg2 bytes from the
 * allocator at 0x008873E8 with tag 0x30122, zero-filled (func_0043f9c8, the port's memset), and
 * word +0x10 of every 20-byte entry is set to -1. The slot's count becomes arg2 and arg0 is
 * returned. When the allocation fails it reports error {0x122, func_003df590(0x80000013, size)}
 * through func_003df4d0 and returns 0. */

typedef void *(*P4AllocFn)(u32 size, u32 tag);

u8 *func_00395100(u8 *arg0, s32 arg1, u32 arg2)
{
    u8 *slot;
    u32 bytes;
    u32 i;

    slot = *(u8 **)(arg0 + 4) + (arg1 << 3);
    bytes = arg2 * 20;
    *(void **)(slot + 4) = (*(P4AllocFn *)0x008873E8)(bytes, 0x30122);
    if (*(u8 **)(slot + 4) == (u8 *)0) {
        s32 err[2];

        err[0] = 0x122;
        err[1] = func_003df590(0x80000013, bytes);
        func_003df4d0(err);
        return (u8 *)0;
    }
    func_0043f9c8(*(u8 **)(slot + 4), 0, bytes);
    for (i = 0; i < arg2; i++) {
        *(s32 *)(*(u8 **)(slot + 4) + 20 * i + 0x10) = -1;
    }
    *(u32 *)slot = arg2;
    return arg0;
}
