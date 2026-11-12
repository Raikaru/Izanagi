/* func_00396fb0 (368 bytes): creates a node-index (HAnim hierarchy) record. Returns it.
 * arg0 is the node count, arg1 and arg2 are optional arrays of arg0 words, arg3 the flags, arg4
 * is passed on to func_003d5790.
 *
 * The record (0x1C+ bytes) comes from the freelist function at 0x008873F8 with the pool at
 * 0x007646CC (gp-0x4A24) and tag 0x3011E. Fields: +0 flags, +4 count, +0x20 func_003d5790(arg0,
 * arg4), +0x14 = 0, +0x18 = the record itself. With flags bit 1 clear +8/+0xC hold a 16-aligned
 * block of arg0*64 bytes (+0xC is the raw allocation from 0x008873E8 with tag 0x3011E and size
 * arg0*64+15); with bit 1 set both are 0. +0x10 is an array of arg0 16-byte nodes from the same
 * allocator (size arg0*16): node i has +0xC = 0, +4 = i, +0 = arg2[i] when arg2 is non-null,
 * +8 = arg1[i] when arg1 is non-null. */

typedef void *(*P4AllocFn)(u32 size, u32 tag);
typedef void *(*P4PoolAllocFn)(u32 pool, u32 tag);

void *func_00396fb0(s32 arg0, s32 *arg1, s32 *arg2, u32 arg3, s32 arg4)
{
    u8 *rec;
    s32 i;
    u8 *nodes;

    rec = (u8 *)(*(P4PoolAllocFn *)0x008873F8)(*(u32 *)0x007646CC, 0x3011E);
    *(void **)(rec + 0x20) = func_003d5790(arg0, arg4);
    *(u32 *)(rec + 0) = arg3;
    *(s32 *)(rec + 4) = arg0;
    *(u32 *)(rec + 0x14) = 0;
    if (arg3 & 2) {
        *(u32 *)(rec + 0x8) = 0;
        *(u32 *)(rec + 0xC) = 0;
    } else {
        u8 *raw;

        raw = (u8 *)(*(P4AllocFn *)0x008873E8)((u32)(arg0 * 64 + 15), 0x3011E);
        *(u32 *)(rec + 0x8) = (u32)(raw + 0xF) & 0xFFFFFFF0u;
        *(u8 **)(rec + 0xC) = raw;
    }
    nodes = (u8 *)(*(P4AllocFn *)0x008873E8)((u32)(arg0 << 4), 0x3011E);
    *(u8 **)(rec + 0x10) = nodes;
    for (i = 0; i < arg0; i++) {
        *(u32 *)(*(u8 **)(rec + 0x10) + 16 * i + 0xC) = 0;
        if (arg2 != 0) {
            *(s32 *)(*(u8 **)(rec + 0x10) + 16 * i) = arg2[i];
        }
        *(s32 *)(*(u8 **)(rec + 0x10) + 16 * i + 4) = i;
        if (arg1 != 0) {
            *(s32 *)(*(u8 **)(rec + 0x10) + 16 * i + 8) = arg1[i];
        }
    }
    *(u8 **)(rec + 0x18) = rec;
    return rec;
}
