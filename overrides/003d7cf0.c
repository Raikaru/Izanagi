/* func_003d7cf0 (880 bytes): evaluates the UV animation channels of a material set and returns arg0. The table at arg0 + *(s32 *)0x0076483C (gp-0x48B4; gp is
 * 0x007690F0) holds pointers to matrices: entries 0 and 1 are reset to identity (translation 0, the flag word +0xC or'ed with 0x20003),
 * entries 2 to 9 are channel records. A record r has the key array at (*(u32 **)((u32 *)r[0])[5]) + 0x20 (r[11] words, an index below 2 selects the matrix to
 * transform and anything else skips the key), a stride r[9] and the per-key data starting at r + 0x4C. The evaluator r[15] is called as fn(matrix, data); the two
 * known evaluators func_003d6900 (scroll: the data floats +8, +0xC, +0x10, +0x14, +0x18, +0x1C become the rows right = (d2, d3, 0), up = (d4, d5, 0), pos = (d6, d7, 0)) and func_003d6c50
 * (rotation: right = (d3, d5, 0), up = (0, d4, 0), pos = (d6, d7, 0), the rotation about the axis D_0070B6B0 by d[2] / pi * 180 degrees between two translations by D_00752F88 / D_00752F98)
 * are expanded inline. The 4 x 4 matrix is updated with func_003e05d0 and applied to the selected matrix with func_003e0e20(m, local, 1). The data pointer advances
 * by the stride per key. Finally func_0039a700(arg0, table[0], table[1]) is called. The local matrix starts as zero (the interpreter's stack was) and keeps the words an evaluator leaves between keys. */

extern void p4c_0039a700_003d7cf0(u32 a, u32 b, u32 c) __asm__("func_0039a700");
extern void p4c_003e05d0_003d7cf0(f32 *m) __asm__("func_003e05d0");
extern void p4c_003e0870_003d7cf0(f32 *m, f32 *axis, s32 o, f32 deg) __asm__("func_003e0870");
extern void p4c_003e0c90_003d7cf0(f32 *m, u32 p, s32 o) __asm__("func_003e0c90");
extern void p4c_003e0e20_003d7cf0(u32 m, f32 *l, s32 o) __asm__("func_003e0e20");

extern void p4fn_003d6900_003d7cf0(void) __asm__("func_003d6900");
extern void p4fn_003d6c50_003d7cf0(void) __asm__("func_003d6c50");

u32 func_003d7cf0(u32 arg0)
{
    u32 *table = (u32 *)(arg0 + *(s32 *)0x0076483C);
    u32 i;
    u32 k;
    u32 j;
    /* The matrix code (RwMatrixRotate and friends) loads a matrix with movaps: the local matrix and the axis the
     * rotate takes are 16-byte aligned on the PS2 stack, the host stack only guarantees 4. */
    f32 local[16] __attribute__((aligned(16)));
    f32 axis[4] __attribute__((aligned(16)));
    u32 *r;
    u32 *keys;
    u32 data;
    u32 stride;
    u32 fn;
    u32 *m;

    for (i = 0; i < 16; i++) {
        local[i] = 0.0f;
    }
    for (i = 0; i < 2; i++) {
        u32 *p = (u32 *)table[i];

        if (p != (u32 *)0) {
            ((f32 *)p)[10] = 1.0f;
            ((f32 *)p)[5] = 1.0f;
            ((f32 *)p)[0] = 1.0f;
            p[4] = 0;
            p[2] = 0;
            p[1] = 0;
            p[9] = 0;
            p[8] = 0;
            p[6] = 0;
            p[14] = 0;
            p[13] = 0;
            p[12] = 0;
            p[3] = p[3] | 0x20003;
        }
    }
    for (k = 0; k < 8; k++) {
        r = (u32 *)table[2 + k];
        if (r == (u32 *)0) {
            continue;
        }
        stride = r[9];
        data = (u32)r + 0x4C;
        fn = r[15];
        keys = (u32 *)(((u32 *)((u32 *)r[0])[5]) + 8);
        for (j = 0; (s32)j < (s32)r[11]; j++, keys++) {
            if (*keys >= 2) {
                continue;
            }
            m = (u32 *)table[*keys];
            if (m == (u32 *)0) {
                continue;
            }
            if (fn == (u32)p4fn_003d6900_003d7cf0) {
                local[0] = *(f32 *)(data + 8);
                local[1] = *(f32 *)(data + 0xC);
                local[2] = 0.0f;
                local[4] = *(f32 *)(data + 0x10);
                local[5] = *(f32 *)(data + 0x14);
                local[6] = 0.0f;
                local[8] = 0.0f;
                local[9] = 0.0f;
                local[10] = 0.0f;
                local[12] = *(f32 *)(data + 0x18);
                local[13] = *(f32 *)(data + 0x1C);
                local[14] = 0.0f;
                p4c_003e05d0_003d7cf0(local);
            } else if (fn == (u32)p4fn_003d6c50_003d7cf0) {
                axis[0] = *(f32 *)0x0070B6B0;
                axis[1] = *(f32 *)0x0070B6B4;
                axis[2] = *(f32 *)0x0070B6B8;
                local[0] = *(f32 *)(data + 0xC);
                local[1] = *(f32 *)(data + 0x14);
                local[2] = 0.0f;
                local[4] = 0.0f;
                local[5] = *(f32 *)(data + 0x10);
                local[6] = 0.0f;
                local[8] = 0.0f;
                local[9] = 0.0f;
                local[10] = 0.0f;
                local[12] = *(f32 *)(data + 0x18);
                local[13] = *(f32 *)(data + 0x1C);
                local[14] = 0.0f;
                p4c_003e05d0_003d7cf0(local);
                p4c_003e0c90_003d7cf0(local, 0x00752F88, 2);
                p4c_003e0870_003d7cf0(local, axis, 2, 180.0f * (*(f32 *)(data + 8) / 3.1415927f));
                p4c_003e0c90_003d7cf0(local, 0x00752F98, 2);
            } else {
                ((void (*)(f32 *, u32))fn)(local, data);
            }
            p4c_003e0e20_003d7cf0(m, local, 1);
            data += stride;
        }
    }
    p4c_0039a700_003d7cf0(arg0, table[0], table[1]);
    return arg0;
}
