/* func_0039c360 (960 bytes): builds the skin matrix palette of the object arg0 into the output array at the word at
 * 0x0088649C and returns it. The skin record is the pointer at arg0 + the offset word at 0x00886494; arg1 is the bone
 * source (func_003b8510(arg1) gives the bone matrix array B).
 *
 * Without a skin record the first word of arg1 gives a count and that many identity matrices are written (each
 * 0x40 bytes: words 0, 5 and 10 are 1.0, words 1, 2, 4, 6, 8, 9, 12, 13 and 14 are 0, word 3 gets 0x20003 ORed in, and
 * words 7, 11 and 15 are left alone). With a record (word 1 = count n, word 2 = inverse bind
 * matrices S, flags in word 0) the palette matrix i is, row-vector style (rows 0 to 2 use w = 0, row 3 adds the matrix' row 3), with
 * mul(X, M) the product of X and M:
 *   flags & 2:       mul(mul(B[i], func_003e9700(entries[i].word3)), F) where entries = word 4 and F is the matrix of func_003e9700(arg0[1])
 *   flags & 0x4000:  mul(B[i], S[i])
 *   otherwise:       mul(mul(B[i], S[i]), F)
 * F is copied with func_003e0960 first. The last two forms loop with the test at the end, so they write at least
 * one matrix (and never stop for n of 0). Each product is formed per component as ((M0 * x + M1 * y) + M2 * z), row 3
 * plus M3 * 1.0. */

extern u8 *p4c_003b8510_0039c360(u32 *a) __asm__("func_003b8510");
extern void p4c_003e0960_0039c360(void *m, void *src) __asm__("func_003e0960");
extern u8 *p4c_003e9700_0039c360(u32 o) __asm__("func_003e9700");


static void p4_mat_mul_039c360(f32 *out, f32 *b, f32 *m)
{
    s32 r;
    s32 c;
    f32 acc;

    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            acc = m[c] * b[4 * r];
            acc = acc + m[4 + c] * b[4 * r + 1];
            if (r < 3) {
                out[4 * r + c] = acc + m[8 + c] * b[4 * r + 2];
            } else {
                acc = acc + m[8 + c] * b[4 * r + 2];
                out[4 * r + c] = acc + m[12 + c] * 1.0f;
            }
        }
    }
}

u8 *func_0039c360(u8 *arg0, u32 *arg1)
{
    u8 *out = *(u8 **)0x0088649C;
    u8 *rec = *(u8 **)(arg0 + *(s32 *)0x00886494);
    u8 *e;
    u32 flags;
    s32 n;
    s32 i;
    u8 *S;
    u8 *B;
    f32 F[16];
    f32 T[16];
    f32 *o;
    f32 *end;
    u32 *w;
    u32 k;

    if (rec == (u8 *)0) {
        for (k = 0; k < arg1[0]; k++) {
            w = (u32 *)(out + k * 0x40);
            w[10] = 0x3F800000;
            w[5] = 0x3F800000;
            w[0] = 0x3F800000;
            w[4] = 0;
            w[2] = 0;
            w[1] = 0;
            w[9] = 0;
            w[8] = 0;
            w[6] = 0;
            w[14] = 0;
            w[13] = 0;
            w[12] = 0;
            w[3] = w[3] | 0x20003;
        }
        return out;
    }
    n = *(s32 *)(rec + 4);
    S = *(u8 **)(rec + 8);
    B = (u8 *)p4c_003b8510_0039c360(arg1);
    flags = *(u32 *)rec;
    if ((flags & 2) != 0) {
        S = *(u8 **)(rec + 0x10);
        p4c_003e0960_0039c360(F, p4c_003e9700_0039c360(*(u32 *)(arg0 + 4)));
        for (i = 0; i < n; i++) {
            p4_mat_mul_039c360(T, (f32 *)(B + i * 64), (f32 *)p4c_003e9700_0039c360(*(u32 *)(S + i * 16 + 0xC)));
            p4_mat_mul_039c360((f32 *)(out + i * 64), T, F);
        }
    } else if ((flags & 0x4000) != 0) {
        o = (f32 *)out;
        end = (f32 *)(out + (u32)n * 0x40);
        do {
            p4_mat_mul_039c360(o, (f32 *)B, (f32 *)S);
            o += 16;
            B += 0x40;
            S += 0x40;
        } while (o != end);
    } else {
        p4c_003e0960_0039c360(F, p4c_003e9700_0039c360(*(u32 *)(arg0 + 4)));
        o = (f32 *)out;
        end = (f32 *)(out + (u32)n * 0x40);
        do {
            p4_mat_mul_039c360(T, (f32 *)B, (f32 *)S);
            p4_mat_mul_039c360(o, T, F);
            o += 16;
            B += 0x40;
            S += 0x40;
        } while (o != end);
    }
    return out;
}
