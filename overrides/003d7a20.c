/* func_003d7a20 (160 bytes): reads a 0x40-byte block from the stream arg0 (func_003e2910 and func_003df360
 * return 0 on failure). The block (0x30135) comes from the freelist slot 0x008873F8 with the pool *(u32 *)0x00764838
 * (gp-0x48B8). Its first 0x20 bytes are read with func_003e2910 and the next 0x20 with func_003df360;
 * +0x40 is set to 1 and the block is returned. A failed allocation or read returns 0. */

extern s32 p4c_003df360_003d7a20(s32 stream, void *p, s32 n) __asm__("func_003df360");
extern s32 p4c_003e2910_003d7a20(s32 stream, void *buf, s32 n) __asm__("func_003e2910");

typedef void *(*P4PoolAllocFn_003d7a20)(u32 pool, u32 tag);

u8 *func_003d7a20(s32 arg0)
{
    u8 *blk;

    blk = (u8 *)(*(P4PoolAllocFn_003d7a20 *)0x008873F8)(*(u32 *)0x00764838, 0x30135);
    if (blk == (u8 *)0) {
        return (u8 *)0;
    }
    if (p4c_003e2910_003d7a20(arg0, blk, 0x20) == 0) {
        return (u8 *)0;
    }
    if (p4c_003df360_003d7a20(arg0, blk + 0x20, 0x20) == 0) {
        return (u8 *)0;
    }
    *(u32 *)(blk + 0x40) = 1;
    return blk;
}
