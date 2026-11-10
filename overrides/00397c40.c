/* func_00397c40 (1696 bytes): updates the node matrices of an animation hierarchy (RpHAnimHierarchyUpdateMatrices).
 * arg0 is the hierarchy: +0 flags, +4 node count n, +8 matrix array (64 bytes each), +0x10 node info array (16 bytes
 * each: +8 flags, +0xC frame), +0x14 the root object chain, +0x18 the parent hierarchy and +0x1C the parent node index
 * (-1 none), +0x20 the interpolator (+0x24 key frame size, +0x3C the key frame to matrix callback, +0x4C the first key
 * frame). Returns 1, or the result of func_00397480(arg0) when flag 2 is set.
 *
 * The matrix the nodes are placed in (parent) starts as the identity (flag 0x4000), as the matrix of the parent node in
 * the parent hierarchy (flag 1, a parent index and the two flags below), or as the root matrix `root`. root is built from the
 * object chain (flag 0x2000 together with 0x4000 on the first two forms, always on the third): the identity when the
 * chain is empty, func_003e9700(first) when func_003e9240(first) is 0, otherwise first's matrix at +0x10 times each linked
 * child's matrix at +0x10 (func_003e05f0(d, a, b)). With flag 0x2000 the root frame's record at +0xA0 is linked into the
 * list at 0x0088739C once (byte +3 bits 0 and 1 clear, then bit 1 set). For every node the callback builds a local matrix
 * (retail inlines it for func_003954b0, which is the same result), func_003e05f0(node matrix, local, parent) places it,
 * and a node with a frame gets: with flag 0x1000 the local matrix at frame + 0x10 (and func_003e9680(frame) unless
 * 0x2000), with 0x2000 the node matrix times root (0x4000) or plain at frame + 0x50 and its byte +3 set to (b & ~4) | 8.
 * The low two bits of the node info flags steer the parent: 0 the node's matrix, 1 pop the saved parent, 2 push the parent
 * and use the node's matrix, 3 unchanged. Words 3 (|= 0x20003 in the identity), 7, 11 and 15 of locally built matrices
 * that retail never writes are stack garbage there and 0 here. */

extern s32 p4c_00397480_00397c40(u8 *h) __asm__("func_00397480");
extern void p4c_003e05f0_00397c40(void *d, void *a, void *b) __asm__("func_003e05f0");
extern s32 p4c_003e9240_00397c40(u8 *o) __asm__("func_003e9240");
extern void p4c_003e9680_00397c40(u8 *o) __asm__("func_003e9680");
extern u8 *p4c_003e9700_00397c40(u8 *o) __asm__("func_003e9700");

typedef void (*P4KeyFn_00397c40)(void *out, void *key);

static void p4_ident_00397c40(u32 *m)
{
    u32 w3 = m[3];
    s32 i;

    for (i = 0; i < 16; i++) {
        m[i] = 0;
    }
    m[0] = 0x3F800000;
    m[5] = 0x3F800000;
    m[10] = 0x3F800000;
    m[3] = w3 | 0x20003;
}

static void p4_copy16_00397c40(u32 *d, u32 *s)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        d[i] = s[i];
    }
}

s32 func_00397c40(u8 *arg0)
{
    u32 flags = *(u32 *)arg0;
    u32 root[16] __attribute__((aligned(16)));
    u32 ident[16] __attribute__((aligned(16)));
    u32 tmp[16] __attribute__((aligned(16)));
    u32 local[16] __attribute__((aligned(16)));
    u32 *saved[64];
    u32 **sp = saved;
    u32 *parent;
    u8 *chain = (u8 *)0;
    s32 haveChain = 0;
    s32 i;
    s32 n;
    u8 *interp;
    P4KeyFn_00397c40 fn;
    u32 keySize;
    u8 *key;
    u8 *info;
    u32 *mat;
    u8 *frame;
    u8 *rec;
    u8 *p;
    u32 mode;

    for (i = 0; i < 16; i++) {
        root[i] = 0;
        ident[i] = 0;
        tmp[i] = 0;
        local[i] = 0;
    }
    if ((flags & 2) != 0) {
        return p4c_00397480_00397c40(arg0);
    }
    parent = root;
    if ((flags & 1) != 0 && *(s32 *)(arg0 + 0x1C) != -1) {
        u8 *ph = *(u8 **)(arg0 + 0x18);

        parent = (u32 *)(*(u8 **)(ph + 8) + (*(s32 *)(arg0 + 0x1C) << 6));
        if ((flags & 0x2000) != 0 && (flags & 0x4000) != 0) {
            chain = *(u8 **)(ph + 0x14);
            haveChain = 1;
        }
    } else if ((flags & 0x4000) != 0) {
        ident[3] = 0;
        p4_ident_00397c40(ident);
        parent = ident;
        if ((flags & 0x2000) != 0) {
            chain = *(u8 **)(arg0 + 0x14);
            haveChain = 1;
        }
    } else {
        chain = *(u8 **)(arg0 + 0x14);
        haveChain = 1;
        parent = root;
    }

    if (haveChain) {
        u8 *first = (chain != (u8 *)0) ? *(u8 **)(chain + 4) : (u8 *)0;

        if (first == (u8 *)0) {
            p4_ident_00397c40(root);
        } else if (p4c_003e9240_00397c40(first) == 0) {
            p4_copy16_00397c40(root, (u32 *)p4c_003e9700_00397c40(first));
        } else {
            u8 *child;

            p4_copy16_00397c40(root, (u32 *)(first + 0x10));
            for (child = *(u8 **)(first + 4); child != (u8 *)0; child = *(u8 **)(child + 4)) {
                p4_copy16_00397c40(tmp, root);
                p4c_003e05f0_00397c40(root, tmp, child + 0x10);
            }
        }
    }
    if ((flags & 0x2000) != 0) {
        rec = *(u8 **)(*(u8 **)(arg0 + 0x14) + 0xA0);
        if ((rec[3] & 3) == 0) {
            u32 *head = (u32 *)0x0088739C;

            *(u32 *)(rec + 8) = *head;
            *(u32 **)(rec + 0xC) = head;
            *(u32 *)(*head + 4) = (u32)(rec + 8);
            *head = (u32)(rec + 8);
            rec = *(u8 **)(*(u8 **)(arg0 + 0x14) + 0xA0);
            rec[3] = rec[3] | 2;
        }
    }

    interp = *(u8 **)(arg0 + 0x20);
    n = *(s32 *)(arg0 + 4);
    info = *(u8 **)(arg0 + 0x10);
    mat = *(u32 **)(arg0 + 8);
    fn = *(P4KeyFn_00397c40 *)(interp + 0x3C);
    keySize = *(u32 *)(interp + 0x24);
    key = interp + 0x4C;
    if (n <= 0) {
        return 1;
    }
    for (i = 0; i < n; i++) {
        fn(local, key);
        p4c_003e05f0_00397c40(mat, local, parent);
        frame = *(u8 **)(info + 0xC);
        if (frame != (u8 *)0) {
            if ((flags & 0x1000) != 0) {
                p4_copy16_00397c40((u32 *)(frame + 0x10), local);
                if ((flags & 0x2000) == 0) {
                    p4c_003e9680_00397c40(frame);
                }
            }
            if ((flags & 0x2000) != 0) {
                if ((flags & 0x4000) != 0) {
                    p4c_003e05f0_00397c40((u32 *)(frame + 0x50), mat, root);
                } else {
                    p4_copy16_00397c40((u32 *)(frame + 0x50), mat);
                }
                frame[3] = (u8)((frame[3] & ~4) | 8);
            }
        }
        mode = *(u32 *)(info + 8) & 3;
        if (mode == 2) {
            *sp = parent;
            sp++;
            parent = mat;
        } else if (mode == 1) {
            sp--;
            parent = *sp;
        } else if (mode == 0) {
            parent = mat;
        }
        mat += 16;
        key += keySize;
        info += 0x10;
    }
    return 1;
}
