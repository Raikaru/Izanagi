/* func_003caa50 (468 bytes): links an object into every leaf of a k-d tree its bounding box touches.
 * arg0 is the tree owner (+0x1C the root), arg1 the object (+0x64 the head of its leaf-link list).
 *
 * func_003bfae0(arg1) returns the box as a centre (x, y, z) and a radius r; lo = centre - r and
 * hi = centre + r per axis. A node's first word is the axis as a byte offset into those triples (0, 4 or
 * 8; negative marks a leaf). An inner node's split values are the floats at +0x10 and +0x14 and its
 * children are +8 (low side) and +0xC (high side). If lo[axis] < node[0x10] the walk goes to
 * +8 and also queues +0xC when node[0x14] < hi[axis]; otherwise it goes to +0xC when node[0x14] <
 * hi[axis] and pops the queue when not. A leaf gets a new
 * link entry (0x14 bytes from the freelist function at 0x008873F8, pool *(0x008872E0 + *(s32 *)0x00764808),
 * tag 0x30507): +0x14 the leaf, +8 the object, +0 and +4 its place at the head of the circular list at
 * leaf+0x38, +0xC and +0x10 its place at the head of the list at object+0x64. The walk ends when the
 * queue is exhausted. */

extern u8 *p4c_003bfae0_003caa50(u8 *obj) __asm__("func_003bfae0");

typedef void *(*P4PoolAllocFn_003caa50)(u32 pool, u32 tag);

void func_003caa50(u8 *arg0, u8 *arg1)
{
    f32 *box;
    f32 lo[3];
    f32 hi[3];
    u8 *stack[64];
    s32 sp;
    u8 *node;
    f32 cx;
    f32 cy;
    f32 cz;
    f32 r;

    sp = 0;
    stack[0] = (u8 *)0;
    box = (f32 *)p4c_003bfae0_003caa50(arg1);
    node = *(u8 **)(arg0 + 0x1C);
    cx = box[0];
    cy = box[1];
    cz = box[2];
    r = box[3];
    lo[0] = cx - r;
    hi[0] = cx + r;
    lo[1] = cy - r;
    hi[1] = cy + r;
    lo[2] = cz - r;
    hi[2] = cz + r;

    do {
        s32 axis = *(s32 *)node;

        if (axis < 0) {
            u8 *e;
            u8 *first;

            e = (u8 *)(*(P4PoolAllocFn_003caa50 *)0x008873F8)(*(u32 *)(0x008872E0 + *(s32 *)0x00764808), 0x30507);
            *(u8 **)(e + 0x14) = node;
            *(u8 **)(e + 8) = arg1;
            first = *(u8 **)(node + 0x38);
            *(u8 **)(e + 0) = first;
            *(u8 **)(e + 4) = node + 0x38;
            *(u8 **)(*(u8 **)(node + 0x38) + 4) = e;
            *(u8 **)(node + 0x38) = e;
            node = stack[sp];
            sp -= 1;
            first = *(u8 **)(arg1 + 0x64);
            *(u8 **)(e + 0xC) = first;
            *(u8 **)(e + 0x10) = arg1 + 0x64;
            *(u8 **)(*(u8 **)(arg1 + 0x64) + 4) = e + 0xC;
            *(u8 **)(arg1 + 0x64) = e + 0xC;
            continue;
        }
        if (lo[axis >> 2] < *(f32 *)(node + 0x10)) {
            u8 *right = *(u8 **)(node + 0xC);
            u8 *left = *(u8 **)(node + 8);

            if (*(f32 *)(node + 0x14) < hi[axis >> 2]) {
                sp += 1;
                stack[sp] = right;
            }
            node = left;
        } else if (*(f32 *)(node + 0x14) < hi[axis >> 2]) {
            node = *(u8 **)(node + 0xC);
        } else {
            node = stack[sp];
            sp -= 1;
        }
    } while (sp >= 0);
}
