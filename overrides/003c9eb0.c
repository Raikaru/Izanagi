/* func_003c9eb0: builds a 0x3000B-byte object (obj) from the 24-byte float
 * record src (src[0..2] and src[3..5]), builds a second 0x3000B object
 * (ent) from the same record, and registers both. A registration block is
 * allocated with the per-id allocator (retail slot 0x008873F8) for the
 * table entry chosen by the byte offset at retail 0x007647F0. On success
 * returns obj. On failure, reports error 2 and returns 0, or frees obj and
 * returns 0 after the registration calls.
 *
 * Written as a direct transliteration of the retail code: every offset
 * and global is kept as the asm has it. */

/* Retail slot 0x008873E8 is the RenderWare malloc: (size, flags). The sizes of the two objects
 * built here are kept in the retail words 0x0070B060 and 0x0070B040. */
typedef void *(*P4ObjAllocFn)(u32 size, u32 flags);
typedef void *(*P4ObjAllocByIdFn)(u32 size, u32 flags);
typedef void (*P4ObjFreeFn)(void *p);
typedef void (*P4ObjInsertFn)(u8 *list, u8 *item);

typedef struct P4ObjError {
    u32 pluginID;
    s32 errorCode;
} P4ObjError;

u8 *func_003c9eb0(u8 *src)
{
    extern s32 func_003df590(s32 arg0, ...);
    extern u8 *func_003df4d0(s32 *arg0);
    P4ObjAllocFn alloc = *(P4ObjAllocFn *)0x008873E8;
    P4ObjAllocByIdFn alloc_by_id = *(P4ObjAllocByIdFn *)0x008873F8;
    P4ObjFreeFn free_fn = *(P4ObjFreeFn *)0x008873EC;
    P4ObjError err;
    u8 *obj;
    u8 *ent;
    u8 *t2;
    u8 *t1;
    u8 *t0;
    u8 *blk;
    u8 *old;
    u32 pool;
    s32 idx;

    pool = *(u32 *)0x0070B060;
    obj = (u8 *)alloc(pool, 0x3000B);
    if (obj == 0) {
        goto err_a;
    }

    obj[0] = 7;
    obj[1] = 0;
    obj[2] = 0;
    obj[3] = 0;
    *(u32 *)(obj + 4) = 0;
    func_003c4a40(obj + 16);
    *(u32 *)(obj + 12) = 2;
    *(u32 *)(obj + 8) = 0;

    ent = (u8 *)alloc(*(u32 *)0x0070B040, 0x3000B);
    if (ent == 0) {
        goto err_b;
    }

    *(s32 *)(ent + 0) = -1;
    *(u8 **)(ent + 56) = ent + 56;
    *(u8 **)(ent + 60) = ent + 56;
    *(u8 **)(ent + 64) = ent + 64;
    *(u8 **)(ent + 68) = ent + 64;
    *(u32 *)(ent + 52) = 0;
    *(u32 *)(ent + 120) = 0;
    *(u16 *)(ent + 130) = 0;
    *(u16 *)(ent + 132) = 0;
    *(u32 *)(ent + 8) = 0;
    *(u32 *)(ent + 4) = 0;
    *(u32 *)(ent + 12) = 0;
    func_0043f9c8(ent + 16, 0, 32);
    *(u32 *)(ent + 48) = 0;

    *(float *)(ent + 84) = *(float *)(src + 12);
    *(float *)(ent + 88) = *(float *)(src + 16);
    *(float *)(ent + 92) = *(float *)(src + 20);
    *(float *)(ent + 72) = *(float *)(src + 0);
    *(float *)(ent + 76) = *(float *)(src + 4);
    *(float *)(ent + 80) = *(float *)(src + 8);
    *(float *)(ent + 108) = *(float *)(src + 12);
    *(float *)(ent + 112) = *(float *)(src + 16);
    *(float *)(ent + 116) = *(float *)(src + 20);
    *(float *)(ent + 96) = *(float *)(src + 0);
    *(float *)(ent + 100) = *(float *)(src + 4);
    *(float *)(ent + 104) = *(float *)(src + 8);
    *(u32 *)(ent + 124) = 0;

    *(u8 **)(obj + 28) = ent;
    *(u32 *)(obj + 32) = 0;
    *(u32 *)(obj + 76) = 0;
    *(u32 *)(obj + 72) = 0;
    *(u32 *)(obj + 68) = 0;
    *(float *)(obj + 92) = *(float *)(src + 12);
    *(float *)(obj + 96) = *(float *)(src + 16);
    *(float *)(obj + 100) = *(float *)(src + 20);
    *(float *)(obj + 80) = *(float *)(src + 0);
    *(float *)(obj + 84) = *(float *)(src + 4);
    *(float *)(obj + 88) = *(float *)(src + 8);

    t2 = obj + 44;
    t1 = obj + 52;
    t0 = obj + 60;
    *(u8 **)(obj + 44) = t2;
    *(u8 **)(obj + 48) = t2;
    *(u32 *)(obj + 36) = 0;
    *(u8 **)(obj + 40) = t2;
    *(u8 **)(obj + 52) = t1;
    *(u8 **)(obj + 56) = t1;
    *(u8 **)(obj + 60) = t0;
    *(u8 **)(obj + 64) = t0;
    *(u32 *)(obj + 104) = (u32)(unsigned long)func_003c9640;
    *(u32 *)(obj + 108) = 0;

    idx = *(s32 *)0x007647F0;
    blk = (u8 *)alloc_by_id(*(u32 *)(0x008872E0 + idx), 0x40507);
    if (blk != 0) {
        *(u8 **)(blk + 0) = obj;
        *(void **)(blk + 4) = pool;
        *(u32 *)(blk + 8) = *(u32 *)(0x008872E0 + idx + 4);
        *(u8 **)(blk + 12) = (u8 *)(0x008872E0 + idx + 4);
        old = *(u8 **)(0x008872E0 + idx + 4);
        *(u8 **)(old + 4) = blk + 8;
        *(u8 **)(0x008872E0 + idx + 4) = blk + 8;
    }

    ((P4ObjInsertFn)func_003e3b70)((u8 *)0x0070B060, obj);
    ((P4ObjInsertFn)func_003e3b70)((u8 *)0x0070B040, ent);

    if (func_003c9b30(obj) != 0) {
        return obj;
    }

    func_003c99f0(obj);
    func_003c9a80(obj);
    func_003c49a0(obj + 16);
    if ((obj[3] & 1) != 0) {
        if (*(u8 **)(obj + 28) != 0) {
            func_003c91d0(*(u8 **)(obj + 28));
        }
        func_003e3c20((u8 *)0x0070B060, obj);
        free_fn(obj);
        return 0;
    }
    if (*(u8 **)(obj + 28) != 0) {
        func_003c9300(*(u8 **)(obj + 28));
    }
    func_003e3c20((u8 *)0x0070B060, obj);
    free_fn(obj);
    return 0;

err_a:
    err.pluginID = 2;
    err.errorCode = func_003df590((s32)0x80000013, *(void **)0x0070B060);
    func_003df4d0((void *)&err);
    return 0;

err_b:
    err.pluginID = 2;
    err.errorCode = func_003df590((s32)0x80000013, 4);
    func_003df4d0((void *)&err);
    free_fn(obj);
    return 0;
}
