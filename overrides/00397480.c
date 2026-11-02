/* func_00397480 (1972 bytes): evaluates an animation hierarchy arg0 and returns 1. Fields: arg0[0] = flags (0x1000: write the local matrices into the frames, 0x2000: write the world
 * matrices and queue the frame record), arg0[1] = node count, arg0[4] = node table (16 bytes per node: +8 = stack op in bits 0 to 1, +0xC = frame or null), arg0[5] = the owner (word +4 = its frame,
 * word +0xA0 = the record that is linked into the list headed at 0x0088739C once, byte +3 bits 0 and 1 clear), arg0[8] = the animation (+0x3C = key evaluator, +0x24 = key stride, keys start at +0x4C).
 * The starting world matrix W is the frame's matrix (+0x10, 16 words) when func_003e9240(frame) is nonzero, then multiplied by every ancestor's (+4 chain) matrix
 * with func_003e05f0(W, copy of W, ancestor + 0x10); otherwise the matrix func_003e9700(frame) returns; an owner without frame gets the identity with flag word | 0x20003. For each node
 * the evaluator makes the local matrix L (func_003954b0 is expanded inline: the quaternion at key + 8 (x, y, z, w) gives the rotation, +0x18 the translation, flag word 3), P = L * W (func_003e05f0(P, L, W)),
 * L / P are stored into the node's frame (+0x10 / +0x50, with byte +3 = (byte & ~4) | 8; func_003e9680(frame) when only the local matrix is stored), and the op of the node updates W:
 * 0 -> W = P, 1 -> pop W from the matrix stack, 2 -> push W and W = P, 3 -> unchanged. */

extern void p4c_003e05f0_00397480(u32 *d, u32 *a, u32 *b) __asm__("func_003e05f0");
extern s32 p4c_003e9240_00397480(u32 f) __asm__("func_003e9240");
extern void p4c_003e9680_00397480(u32 f) __asm__("func_003e9680");
extern u8 *p4c_003e9700_00397480(u32 f) __asm__("func_003e9700");

typedef void (*P4KeyFn_00397480)(u32 *m, u32 key);

extern void p4fn_003954b0_00397480(void) __asm__("func_003954b0");

static void p4_mcopy_00397480(u32 *d, u32 *s)
{
    u32 i;

    for (i = 0; i < 16; i++) {
        d[i] = s[i];
    }
}

s32 func_00397480(u32 *arg0)
{
    u32 W[16] __attribute__((aligned(16)));
    u32 C[16] __attribute__((aligned(16)));
    u32 L[16] __attribute__((aligned(16)));
    u32 P[16] __attribute__((aligned(16)));
    u32 stack[32][16];
    s32 sp = 0;
    u32 flags = arg0[0];
    u32 owner = arg0[5];
    u32 frame = *(u32 *)(owner + 4);
    u32 *anim;
    u32 stride;
    u32 key;
    u32 fn;
    u32 i;
    u32 *node;
    u32 k;

    for (i = 0; i < 16; i++) {
        W[i] = 0;
        C[i] = 0;
        L[i] = 0;
        P[i] = 0;
    }
    if (frame == 0) {
        *(f32 *)&W[0] = 1.0f;
        *(f32 *)&W[5] = 1.0f;
        *(f32 *)&W[10] = 1.0f;
        W[3] = W[3] | 0x20003;
    } else if (p4c_003e9240_00397480(frame) == 0) {
        p4_mcopy_00397480(W, (u32 *)p4c_003e9700_00397480(frame));
    } else {
        u32 p;

        p4_mcopy_00397480(W, (u32 *)(frame + 0x10));
        for (p = *(u32 *)(frame + 4); p != 0; p = *(u32 *)(p + 4)) {
            p4_mcopy_00397480(C, W);
            p4c_003e05f0_00397480(W, C, (u32 *)(p + 0x10));
        }
    }
    if ((flags & 0x2000) != 0) {
        u32 X = *(u32 *)(owner + 0xA0);

        if ((*(u8 *)(X + 3) & 3) == 0) {
            *(u32 *)(X + 8) = *(u32 *)0x0088739C;
            *(u32 *)(X + 0xC) = 0x0088739C;
            *(u32 *)(*(u32 *)0x0088739C + 4) = X + 8;
            *(u32 *)0x0088739C = X + 8;
            *(u8 *)(X + 3) = *(u8 *)(X + 3) | 2;
        }
    }
    anim = (u32 *)arg0[8];
    node = (u32 *)arg0[4];
    fn = anim[0x3C / 4];
    key = (u32)anim + 0x4C;
    stride = anim[0x24 / 4];
    for (k = 0; (s32)k < (s32)arg0[1]; k++, node += 4, key += stride) {
        u32 nframe;

        if (fn == (u32)p4fn_003954b0_00397480) {
            f32 *kq = (f32 *)key;
            f32 two = 2.0f;
            f32 one = 1.0f;
            f32 x = kq[2];
            f32 y = kq[3];
            f32 z = kq[4];
            f32 w = kq[5];
            f32 xx = x * x;
            f32 zz = z * z;
            f32 yz = y * z;
            f32 zx = z * x;
            f32 xy = x * y;
            f32 wx = w * x;
            f32 yy = y * y;
            f32 wy = w * y;
            f32 wz = w * z;
            f32 a1 = yy + zz;
            f32 a2 = xx + zz;
            f32 a3 = xx + yy;
            f32 b1 = xy + wz;
            f32 b2 = xy - wz;
            f32 *m = (f32 *)L;

            L[3] = 3;
            m[12] = 0.0f;
            m[13] = 0.0f;
            m[14] = 0.0f;
            m[1] = two * b1;
            m[4] = two * b2;
            m[0] = (0.0f + one) - two * a1;
            m[5] = (0.0f + one) - two * a2;
            m[10] = (0.0f + one) - two * a3;
            m[2] = two * (zx - wy);
            m[8] = two * (zx + wy);
            m[6] = two * (yz + wx);
            m[9] = two * (yz - wx);
            m[12] = kq[6];
            m[13] = kq[7];
            m[14] = kq[8];
        } else {
            ((P4KeyFn_00397480)fn)(L, key);
        }
        p4c_003e05f0_00397480(P, L, W);
        nframe = node[3];
        if (nframe != 0) {
            if ((flags & 0x1000) != 0) {
                p4_mcopy_00397480((u32 *)(nframe + 0x10), L);
                if ((flags & 0x2000) == 0) {
                    p4c_003e9680_00397480(nframe);
                }
            }
            if ((flags & 0x2000) != 0) {
                p4_mcopy_00397480((u32 *)(nframe + 0x50), P);
                *(u8 *)(nframe + 3) = (u8)((*(u8 *)(nframe + 3) & 0xFB) | 8);
            }
        }
        switch (node[2] & 3) {
        case 3:
            break;
        case 2:
            p4_mcopy_00397480(stack[sp], W);
            sp++;
            p4_mcopy_00397480(W, P);
            break;
        case 1:
            sp--;
            p4_mcopy_00397480(W, stack[sp]);
            break;
        default:
            p4_mcopy_00397480(W, P);
            break;
        }
    }
    return 1;
}
