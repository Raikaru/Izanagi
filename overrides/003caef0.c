/* func_003caef0 (752 bytes): (re)links the object arg0 into the leaves of a world's BSP tree and returns arg0. The record at
 * arg0 + *(s32 *)0x00764804 (gp-0x48EC; gp is 0x007690F0) holds {+0 = world, +4 = validity callback}; a callback result of 0 reports the error
 * {2, func_003df590(0x80000016)} to func_003df4d0 and ends. Nothing else happens when the byte at arg0 + 1 is below 0x80, the world is 0 or the frame arg0[1] is 0. Otherwise every link of
 * the object's list at arg0 + 0x2C (circular, sentinel arg0 + 0x2C, 20-byte-aligned link nodes whose containing node starts 0xC earlier) is unlinked from both its lists and freed
 * (pool slot 0x008873FC, pool = *(u32 *)(0x008872E4 + *(s32 *)0x00764808)). Then the bounding box (the frame's position func_003e9700(frame)[0x30..0x38] +- the
 * radius at arg0 + 0x14) is pushed down the tree from world[7]: a node's +0 is the axis (a byte offset 0, 4 or 8 into the box, negative for a leaf),
 * +0x10 / +0x14 the plane bounds, +8 / +0xC the children; the box goes into the near child when it is below +0x10 and also into the far one when
 * it reaches above +0x14 (pushed on a stack); for each leaf a node of 0x18 bytes (pool slot 0x008873F8, tag 0x30507) is made that is
 * put at the head of the leaf's list (sentinel at leaf + 0x40) and of the object's list, with +8 = the object and +0x14 = the leaf. */

extern void p4c_003df4d0_003caef0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003caef0(s32 code, ...) __asm__("func_003df590");
extern u8 *p4c_003e9700_003caef0(u32 frame) __asm__("func_003e9700");

typedef void *(*P4PoolAllocFn_003caef0)(u32 pool, u32 tag);
typedef void (*P4PoolFreeFn_003caef0)(u32 pool, u32 obj);
typedef s32 (*P4CallbackFn_003caef0)(u32 obj);

u32 func_003caef0(u32 *arg0)
{
    u8 *rec = (u8 *)arg0 + *(s32 *)0x00764804;
    u32 world;
    u32 frame;
    u32 *sentinel;
    u32 *link;
    f32 radius;
    f32 *m;
    f32 hi[3];
    f32 lo[3];
    u32 stack[70];
    s32 sp;
    u32 node;
    s32 err[2];
    s32 i;

    if ((*(P4CallbackFn_003caef0 *)(rec + 4))((u32)arg0) == 0) {
        err[0] = 2;
        err[1] = p4c_003df590_003caef0(0x80000016);
        p4c_003df4d0_003caef0(err);
        return (u32)arg0;
    }
    if (*(u8 *)((u8 *)arg0 + 1) < 0x80) {
        return (u32)arg0;
    }
    world = *(u32 *)rec;
    if (world == 0) {
        return (u32)arg0;
    }
    frame = arg0[1];
    if (frame == 0) {
        return (u32)arg0;
    }
    link = (u32 *)arg0[11];
    radius = *(f32 *)((u8 *)arg0 + 0x14);
    sentinel = arg0 + 11;
    sp = 0;
    for (i = 0; i < 70; i++) {
        stack[i] = 0;
    }
    while (link != sentinel) {
        u32 *p = link - 3;

        link = (u32 *)link[0];
        *(u32 *)p[4] = p[3];
        ((u32 *)p[3])[1] = p[4];
        *(u32 *)p[1] = p[0];
        ((u32 *)p[0])[1] = p[1];
        (*(P4PoolFreeFn_003caef0 *)0x008873FC)(*(u32 *)(0x008872E4 + *(s32 *)0x00764808), (u32)p);
    }
    m = (f32 *)p4c_003e9700_003caef0(frame);
    hi[0] = m[12] + radius;
    lo[0] = m[12] - radius;
    hi[1] = m[13] + radius;
    lo[1] = m[13] - radius;
    hi[2] = m[14] + radius;
    lo[2] = m[14] - radius;
    node = ((u32 *)world)[7];
    while (sp >= 0) {
        s32 axis = *(s32 *)node;

        if (axis < 0) {
            u32 *n = (u32 *)(*(P4PoolAllocFn_003caef0 *)0x008873F8)(*(u32 *)(0x008872E4 + *(s32 *)0x00764808), 0x30507);
            u32 *old;

            n[5] = node;
            n[2] = (u32)arg0;
            n[0] = ((u32 *)node)[16];
            n[1] = node + 0x40;
            old = (u32 *)((u32 *)node)[16];
            old[1] = (u32)n;
            ((u32 *)node)[16] = (u32)n;
            node = stack[sp];
            sp--;
            n[3] = arg0[11];
            n[4] = (u32)(arg0 + 11);
            ((u32 *)arg0[11])[1] = (u32)(n + 3);
            arg0[11] = (u32)(n + 3);
        } else if (*(f32 *)((u8 *)lo + axis) < *(f32 *)(node + 0x10)) {
            u32 cur = node;

            node = ((u32 *)cur)[2];
            if (*(f32 *)(cur + 0x14) < *(f32 *)((u8 *)hi + axis)) {
                sp++;
                stack[sp] = ((u32 *)cur)[3];
            }
        } else if (*(f32 *)(node + 0x14) < *(f32 *)((u8 *)hi + axis)) {
            node = ((u32 *)node)[3];
        } else {
            node = stack[sp];
            sp--;
        }
    }
    return (u32)arg0;
}
