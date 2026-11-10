/* func_00394fd0 (304 bytes): allocates a table of arg0 two-word entries. The block comes from
 * the allocator slot 0x008873E8: 8*arg0 + 8 bytes, tag 0x3001E. Header: +0 = arg0, +4 = pointer
 * to the entries (block + 8); the 8*arg0 entry bytes are zeroed. Returns the block. When the
 * allocation fails it reports error {0x122, func_003df590(0x80000013, size)} through func_003df4d0
 * and returns 0. */

typedef void *(*P4AllocFn)(u32 size, u32 tag);

u8 *func_00394fd0(u32 arg0)
{
    u8 *block;
    u32 size;
    u32 i;
    u32 *p;

    size = arg0 * 8 + 8;
    block = (u8 *)(*(P4AllocFn *)0x008873E8)(size, 0x3001E);
    if (block == (u8 *)0) {
        s32 err[2];

        err[0] = 0x122;
        err[1] = func_003df590(0x80000013, size);
        func_003df4d0(err);
        return (u8 *)0;
    }
    *(u32 *)block = arg0;
    *(u8 **)(block + 4) = block + 8;
    p = *(u32 **)(block + 4);
    for (i = 0; i < arg0; i++) {
        p[0] = 0;
        p[1] = 0;
        p += 2;
    }
    return block;
}
