/* _rpPTankGENInstance (0x003b42e0, 400 bytes): runs the instancing callbacks of a particle tank
 * pipeline. arg0 is the tank object; its pipeline record is *(arg0 + *(s32 *)0x00764700) (gp-0x49F0;
 * gp is 0x007690F0), and arg1, arg2, arg3 are the instance arguments. Returns 1.
 *
 * The value passed on as "data" (v) is arg2 unless (arg3 & 0x1E8000), when it is the first word of
 * the pipeline's first word (**(arg0 + slot)). If the record's callback at +0x24 is set, a 0x48-byte
 * work block is zeroed (func_0043f9c8, the port's memset) and the callback is called as cb(block, arg0, arg1, v, arg3); then each of
 * the record's callbacks at +0x28 (cb(block+0x10, block+0x30, block+0x3C, v, arg1), only when
 * block word +0x10 is non-zero), +0x2C (cb(block+0x18, v, arg1), when word +0x18 is non-zero), +0x30
 * (block+0x20) and +0x34 (block+0x28) is called, and finally the callback at +0x38 as
 * cb(block, arg0, arg1, v, arg3). The retail code leaves the block uninitialized when the +0x24
 * callback is null; here it is zeroed in every case. */

typedef void (*P4PtCb5)(u8 *a, s32 b, s32 c, s32 d, s32 e);
typedef void (*P4PtCb3)(u8 *a, s32 b, s32 c);

s32 func_003b42e0(u8 *arg0, s32 arg1, s32 arg2, s32 arg3)
{
    u8 *rec;
    s32 v;
    u32 work[18] __attribute__((aligned(8)));
    u8 *blk = (u8 *)work;
    s32 i;

    if (arg3 & 0x1E8000) {
        v = **(s32 **)(arg0 + *(s32 *)0x00764700);
    } else {
        v = arg2;
    }
    rec = *(u8 **)(arg0 + *(s32 *)0x00764700);
    if (*(u32 *)(rec + 0x24) != 0) {
        func_0043f9c8(blk, 0, 0x48);
        (*(P4PtCb5 *)(rec + 0x24))(blk, (s32)arg0, arg1, v, arg3);
    } else {
        for (i = 0; i < 18; i++) {
            work[i] = 0;
        }
    }
    if (*(u32 *)(rec + 0x28) != 0 && *(u32 *)(blk + 0x10) != 0) {
        ((void (*)(u8 *, u8 *, u8 *, s32, s32))*(u32 *)(rec + 0x28))(blk + 0x10, blk + 0x30, blk + 0x3C, v, arg1);
    }
    if (*(u32 *)(rec + 0x2C) != 0 && *(u32 *)(blk + 0x18) != 0) {
        (*(P4PtCb3 *)(rec + 0x2C))(blk + 0x18, v, arg1);
    }
    if (*(u32 *)(rec + 0x30) != 0 && *(u32 *)(blk + 0x20) != 0) {
        (*(P4PtCb3 *)(rec + 0x30))(blk + 0x20, v, arg1);
    }
    if (*(u32 *)(rec + 0x34) != 0 && *(u32 *)(blk + 0x28) != 0) {
        (*(P4PtCb3 *)(rec + 0x34))(blk + 0x28, v, arg1);
    }
    if (*(u32 *)(rec + 0x38) != 0) {
        (*(P4PtCb5 *)(rec + 0x38))(blk, (s32)arg0, arg1, v, arg3);
    }
    return 1;
}
