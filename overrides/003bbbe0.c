/* func_003bbbe0 (576 bytes): builds a table object. arg0 is the entry count n, arg1 the kind (1 or 2)
 * and arg2 the source data (n 12-byte float triples). Returns the table, or 0 on an unknown kind or
 * on a failure.
 *
 * The object is 0x44 + 12*(n+1) (kind 1) or 12*(n+2) (kind 2) bytes from the allocator slot
 * 0x008873E8 with tag 0x3000C; +8 = n+2 (kind 1) or n+3 (kind 2), +0x10 = 0, byte +0x18 = 0, +0xC =
 * kind and byte +0 = 0xC. func_003b9a40(object, arg2) fills it; on a 0 result the object is freed
 * (slot 0x008873EC) and 0 returned. Then byte +0 is 0xC again, a copy of the source (12*n bytes, same
 * allocator, tag 0x3000C) becomes +0x14 (the port's memcpy, func_0043f810), and the object is
 * returned. A failed allocation reports error {0x102, func_003df590(0x80000013, size)} through
 * func_003df4d0 (the object is freed first when it is the second allocation); an unknown kind reports
 * {0x102, func_003df590(1)}. */

typedef void *(*P4AllocFn)(u32 size, u32 tag);
typedef void (*P4FreeFn)(void *ptr);
extern void *p4_memcpy(void *dst, const void *src, u32 n) __asm__("func_0043f810");

u8 *func_003bbbe0(s32 arg0, s32 arg1, u8 *arg2)
{
    u8 *obj;
    u8 *copy;
    u32 size;
    s32 err[2];

    if (arg1 == 2) {
        size = (u32)((arg0 + 2) * 12 + 0x44);
        obj = (u8 *)(*(P4AllocFn *)0x008873E8)(size, 0x3000C);
        if (obj == (u8 *)0) {
            err[0] = 0x102;
            err[1] = func_003df590(0x80000013, size);
            func_003df4d0(err);
            return (u8 *)0;
        }
        *(s32 *)(obj + 8) = arg0 + 3;
    } else if (arg1 == 1) {
        size = (u32)((arg0 + 1) * 12 + 0x44);
        obj = (u8 *)(*(P4AllocFn *)0x008873E8)(size, 0x3000C);
        if (obj == (u8 *)0) {
            err[0] = 0x102;
            err[1] = func_003df590(0x80000013, size);
            func_003df4d0(err);
            return (u8 *)0;
        }
        *(s32 *)(obj + 8) = arg0 + 2;
    } else {
        err[0] = 0x102;
        err[1] = func_003df590(1);
        func_003df4d0(err);
        return (u8 *)0;
    }
    *(u32 *)(obj + 0x10) = 0;
    *(u8 *)(obj + 0x18) = 0;
    *(s32 *)(obj + 0xC) = arg1;
    *(u8 *)obj = 0xC;
    if (func_003b9a40(obj, arg2) == 0) {
        (*(P4FreeFn *)0x008873EC)(obj);
        return (u8 *)0;
    }
    *(u8 *)obj = 0xC;
    size = (u32)(arg0 * 12);
    copy = (u8 *)(*(P4AllocFn *)0x008873E8)(size, 0x3000C);
    *(u8 **)(obj + 0x14) = copy;
    if (copy == (u8 *)0) {
        (*(P4FreeFn *)0x008873EC)(obj);
        err[0] = 0x102;
        err[1] = func_003df590(0x80000013, size);
        func_003df4d0(err);
        return (u8 *)0;
    }
    p4_memcpy(*(u8 **)(obj + 0x14), arg2, (u32)(arg0 * 12));
    return obj;
}
