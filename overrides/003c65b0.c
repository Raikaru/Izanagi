/* func_003c65b0 (104 bytes): finds or creates a shared entry in a list. arg1 is the list head (a
 * pointer to the first entry), arg2 and arg3 two 16-bit keys, arg4 an index into the pointer table
 * arg5, arg0 the pool passed to the allocator. An entry is 0x10 bytes: halfwords +0 and +2, pointer
 * +4 (an object), pointer +8 (the bound slot), pointer +0xC (next). The first entry with halfword
 * +2 == arg2, halfword +0 == arg3 and no bound slot (+8 == 0) gets its object's byte +0x20 and the
 * byte +0x20 of table[arg4] incremented and its +8 set to table[arg4]. If none matches a new entry is
 * taken from the freelist function at 0x008873F8 (pool arg0, tag 0x30502), with halfword +0 = arg2,
 * halfword +2 = arg3, +4 = table[arg4], +8 = 0, +0xC = the old head, and becomes the head. Returns the entry that was bound or created. */

typedef void *(*P4PoolAllocFn_003c65b0)(u8 *pool, u32 tag);

u8 *func_003c65b0(u8 *arg0, u32 **arg1, u32 arg2, u32 arg3, s32 arg4, u32 *arg5)
{
    u8 *e;

    e = (u8 *)*arg1;
    while (e != (u8 *)0) {
        if ((arg2 & 0xFFFF) == *(u16 *)(e + 2) && (arg3 & 0xFFFF) == *(u16 *)(e + 0)
            && *(u32 *)(e + 8) == 0) {
            u8 *obj = *(u8 **)(e + 4);

            *(u8 *)(obj + 0x20) = *(u8 *)(obj + 0x20) + 1;
            obj = (u8 *)arg5[arg4];
            *(u8 *)(obj + 0x20) = *(u8 *)(obj + 0x20) + 1;
            *(u32 *)(e + 8) = arg5[arg4];
            return e;
        }
        e = *(u8 **)(e + 0xC);
    }
    e = (u8 *)(*(P4PoolAllocFn_003c65b0 *)0x008873F8)(arg0, 0x30502);
    *(u16 *)(e + 0) = (u16)arg2;
    *(u16 *)(e + 2) = (u16)arg3;
    *(u32 *)(e + 4) = arg5[arg4];
    *(u32 *)(e + 8) = 0;
    *(u32 *)(e + 0xC) = *arg1;
    *arg1 = (u32 *)e;
    return e;
}
