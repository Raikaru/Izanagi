/* func_003c67b0 (432 bytes): builds the triangle table of a mesh and returns it (an array of arg0 record pointers; allocator slot 0x008873E8,
 * tag 0x30502). Two pools are made into arg1 (arg1[0] = func_003e1210(0x24, arg0, 4, 0x30502) for the records and arg1[1] =
 * func_003e1210(0x10, arg0 / 2 + 1, 4, 0x30502) for the edges). Every record (pool slot 0x008873F8, tag 0x30502) gets +0x20 = 0, +0 = its index, +0x10, +0x14 and
 * +0x18 = 0 and the three edges +4, +8 and +0xC made by func_003c65b0(arg1[1], arg2, vertex a, vertex b, index, table) from the halfword
 * triples of arg3 (20 bytes per triangle): (h0, h1), (h1, h2) and (h2, h0). */

extern u32 p4c_003c65b0_003c67b0(u32 pool, u32 key, u32 a, u32 b, u32 i, u32 t) __asm__("func_003c65b0");
extern u32 p4c_003e1210_003c67b0(u32 a, u32 b, u32 c, u32 d) __asm__("func_003e1210");

typedef void *(*P4AllocFn_003c67b0)(u32 size, u32 tag);
typedef void *(*P4PoolAllocFn_003c67b0)(u32 pool, u32 tag);

u32 **func_003c67b0(u32 arg0, u32 *arg1, u32 arg2, u16 *arg3)
{
    u32 **table = (u32 **)(*(P4AllocFn_003c67b0 *)0x008873E8)(arg0 << 2, 0x30502);
    u32 i;
    u32 *o;

    arg1[0] = p4c_003e1210_003c67b0(0x24, arg0, 4, 0x30502);
    arg1[1] = p4c_003e1210_003c67b0(0x10, (arg0 >> 1) + 1, 4, 0x30502);
    for (i = 0; i < arg0; i++, arg3 += 10) {
        o = (u32 *)(*(P4PoolAllocFn_003c67b0 *)0x008873F8)(arg1[0], 0x30502);
        table[i] = o;
        *(u8 *)((u8 *)o + 0x20) = 0;
        o[0] = i;
        o[5] = 0;
        o[4] = 0;
        o[6] = 0;
        o[1] = p4c_003c65b0_003c67b0(arg1[1], arg2, arg3[0], arg3[1], i, (u32)table);
        o[2] = p4c_003c65b0_003c67b0(arg1[1], arg2, arg3[1], arg3[2], i, (u32)table);
        o[3] = p4c_003c65b0_003c67b0(arg1[1], arg2, arg3[2], arg3[0], i, (u32)table);
    }
    return table;
}
