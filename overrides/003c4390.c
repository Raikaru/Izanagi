/* func_003c4390 (1072 bytes): reads a light-style chunk (type 1, version in [0x35000, 0x37003), otherwise {2, func_003df590(0x80000004)} goes to
 * func_003df4d0) from the stream arg0 and returns the new node (0 on failure). The 0x1C-byte body (zeroed first) holds the colour bytes at +4, a word at +0xC (nonzero: an
 * attached object follows as a chunk of type 6, read with func_003e6430) and three floats at +0x10, +0x14, +0x18. The node comes from the pool at
 * *(u32 *)(0x008872E0 + *(s32 *)0x007647D0) (gp-0x4920; gp is 0x007690F0), allocator slot 0x008873F8, tag 0x30007: it starts with colour bytes 0xFF, reference count
 * (halfword +0x18) 1, words +0 and +8 = 0, the floats at 0x0070B008 / 0x0070B00C / 0x0070B010 and is added to the list at 0x0070AFF0 (func_003e3b70); then it takes the body's values and
 * func_003e3110(0x0070AFF0, stream, node) reads the extensions after zeroing *(u32 *)0x007647CC and *(u32 *)0x007647C8 (gp-0x4924 / gp-0x4928): on success a nonzero
 * first one is passed to func_003e32f0(0x0070AFF0, first, node, second); on failure the node is released: the last reference unlinks it (func_003e3c20), destroys +0 (func_003ef3a0) and frees
 * it (slot 0x008873FC), otherwise the count is decremented. Every failure after the allocation releases it the same way. */

extern s32 p4c_003df050_003c4390(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern void p4c_003df4d0_003c4390(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003c4390(s32 code, ...) __asm__("func_003df590");
extern s32 p4c_003e2910_003c4390(s32 stream, void *buf, s32 n) __asm__("func_003e2910");
extern s32 p4c_003e3110_003c4390(u32 list, u32 stream, u8 *obj) __asm__("func_003e3110");
extern void p4c_003e32f0_003c4390(u32 list, u32 a, u8 *obj, u32 b) __asm__("func_003e32f0");
extern void p4c_003e3b70_003c4390(u32 list, u8 *p) __asm__("func_003e3b70");
extern void p4c_003e3c20_003c4390(u32 list, u8 *obj) __asm__("func_003e3c20");
extern u32 p4c_003e6430_003c4390(u32 stream) __asm__("func_003e6430");
extern void p4c_003ef3a0_003c4390(u32 obj) __asm__("func_003ef3a0");

typedef void *(*P4PoolAllocFn_003c4390)(u32 pool, u32 tag);
typedef void (*P4PoolFreeFn_003c4390)(u32 pool, u32 obj);

static void p4_release_003c4390(u8 *node)
{
    if (*(s16 *)(node + 0x18) == 1) {
        p4c_003e3c20_003c4390(0x0070AFF0, node);
        if (*(u32 *)node != 0) {
            p4c_003ef3a0_003c4390(*(u32 *)node);
        }
        *(u32 *)node = 0;
        (*(P4PoolFreeFn_003c4390 *)0x008873FC)(*(u32 *)(0x008872E0 + *(s32 *)0x007647D0), (u32)node);
    } else {
        *(s16 *)(node + 0x18) = (s16)(*(s16 *)(node + 0x18) - 1);
    }
}

u8 *func_003c4390(u32 arg0)
{
    u32 size = 0;
    u32 version = 0;
    u8 body[0x1C];
    u8 *node;
    s32 err[2];
    u32 i;

    for (i = 0; i < 0x1C; i++) {
        body[i] = 0;
    }
    if (p4c_003df050_003c4390(arg0, 1, &size, &version) == 0) {
        return (u8 *)0;
    }
    if (version < 0x35000 || !(version < 0x37003)) {
        err[0] = 2;
        err[1] = p4c_003df590_003c4390(0x80000004);
        p4c_003df4d0_003c4390(err);
        return (u8 *)0;
    }
    if (p4c_003e2910_003c4390(arg0, body, size) != (s32)size) {
        return (u8 *)0;
    }
    node = (u8 *)(*(P4PoolAllocFn_003c4390 *)0x008873F8)(*(u32 *)(0x008872E0 + *(s32 *)0x007647D0), 0x30007);
    if (node != (u8 *)0) {
        node[4] = 0xFF;
        node[5] = 0xFF;
        node[6] = 0xFF;
        node[7] = 0xFF;
        *(u16 *)(node + 0x18) = 1;
        *(u32 *)node = 0;
        *(u32 *)(node + 8) = 0;
        *(u32 *)(node + 0xC) = *(u32 *)0x0070B008;
        *(u32 *)(node + 0x10) = *(u32 *)0x0070B00C;
        *(u32 *)(node + 0x14) = *(u32 *)0x0070B010;
        p4c_003e3b70_003c4390(0x0070AFF0, node);
    }
    if (node == (u8 *)0) {
        return (u8 *)0;
    }
    node[4] = body[4];
    node[5] = body[5];
    node[6] = body[6];
    node[7] = body[7];
    *(u32 *)(node + 0xC) = *(u32 *)(body + 0x10);
    *(u32 *)(node + 0x10) = *(u32 *)(body + 0x14);
    *(u32 *)(node + 0x14) = *(u32 *)(body + 0x18);
    *(u32 *)node = 0;
    if (*(u32 *)(body + 0xC) != 0) {
        if (p4c_003df050_003c4390(arg0, 6, 0, &version) == 0) {
            p4_release_003c4390(node);
            return (u8 *)0;
        }
        if (version < 0x35000 || !(version < 0x37003)) {
            p4_release_003c4390(node);
            err[0] = 2;
            err[1] = p4c_003df590_003c4390(0x80000004);
            p4c_003df4d0_003c4390(err);
            return (u8 *)0;
        }
        *(u32 *)node = p4c_003e6430_003c4390(arg0);
    }
    *(u32 *)0x007647CC = 0;
    *(u32 *)0x007647C8 = 0;
    if (p4c_003e3110_003c4390(0x0070AFF0, arg0, node) == 0) {
        p4_release_003c4390(node);
        return (u8 *)0;
    }
    if (*(u32 *)0x007647CC != 0) {
        p4c_003e32f0_003c4390(0x0070AFF0, *(u32 *)0x007647CC, node, *(u32 *)0x007647C8);
    }
    return node;
}
