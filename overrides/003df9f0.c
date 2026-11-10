/* func_003df9f0 (RwMatrixOrthoNormalize): writes the orthonormalised right/up/at rows of
 * `src` and its position into `dst`, and returns `dst`. The three row vectors are copied to a
 * stack frame (row 0 = A, row 1 = B, row 2 = C, each normalised in place; a row of length <= 0
 * stays unscaled). The row that is most orthogonal to the other two is kept as the cross-product
 * source: the pair with the smallest |dot| is picked, P1 = P2 x P3 is normalised, then
 * P2 = P1 x P3 and normalised. The sums follow the retail mula/madda/madd order (y, x, z for a
 * row length; the dots add 0.0f between steps, which does not change the value). Flags at +0xC
 * get bits 0 and 1 set and bit 17 cleared. */
u8 *func_003df9f0(u8 *arg0, u8 *arg1)
{
    f32 *in = (f32 *)arg1;
    f32 *out = (f32 *)arg0;
    f32 a[3];
    f32 b[3];
    f32 c[3];
    f32 pos[3];
    f32 *p1;
    f32 *p2;
    f32 *p3;
    f32 invA;
    f32 invB;
    f32 invC;
    f32 inv;
    f32 dotBC;
    f32 dotAC;
    f32 dotAB;

    a[1] = in[1];
    a[0] = in[0];
    a[2] = in[2];
    b[0] = in[4];
    b[1] = in[5];
    b[2] = in[6];
    c[0] = in[8];
    c[1] = in[9];
    c[2] = in[10];
    pos[0] = in[12];
    pos[1] = in[13];
    pos[2] = in[14];

    invA = __builtin_sqrtf(a[1] * a[1] + a[0] * a[0] + a[2] * a[2]);
    if (!(invA <= 0.0f)) {
        invA = 1.0f / invA;
    }
    invB = __builtin_sqrtf(b[1] * b[1] + b[0] * b[0] + b[2] * b[2]);
    a[1] = a[1] * invA;
    a[0] = a[0] * invA;
    a[2] = a[2] * invA;
    if (!(invB <= 0.0f)) {
        invB = 1.0f / invB;
    }
    invC = __builtin_sqrtf(c[1] * c[1] + c[0] * c[0] + c[2] * c[2]);
    b[1] = b[1] * invB;
    b[2] = b[2] * invB;
    b[0] = b[0] * invB;
    if (!(invC <= 0.0f)) {
        invC = 1.0f / invC;
    }
    c[0] = c[0] * invC;
    c[1] = c[1] * invC;
    c[2] = c[2] * invC;

    if (invA <= 0.0f) {
        p3 = b;
        p2 = c;
        p1 = a;
    } else if (invB <= 0.0f) {
        p3 = c;
        p2 = a;
        p1 = b;
    } else if (invC <= 0.0f) {
        p3 = a;
        p2 = b;
        p1 = c;
    } else {
        dotBC = b[1] * c[1] + b[0] * c[0] + b[2] * c[2];
        if (dotBC < 0.0f) {
            dotBC = -dotBC;
        }
        dotAC = c[1] * a[1] + c[0] * a[0] + c[2] * a[2];
        if (dotAC < 0.0f) {
            dotAC = -dotAC;
        }
        dotAB = a[1] * b[1] + a[0] * b[0] + a[2] * b[2];
        if (dotAB < 0.0f) {
            dotAB = -dotAB;
        }
        if (dotBC < dotAC) {
            if (dotBC < dotAB) {
                p3 = b;
                p2 = c;
                p1 = a;
            } else {
                p3 = a;
                p2 = b;
                p1 = c;
            }
        } else if (dotAC < dotAB) {
            p3 = c;
            p2 = a;
            p1 = b;
        } else {
            p3 = a;
            p2 = b;
            p1 = c;
        }
    }

    p1[0] = p3[1] * p2[2] - p3[2] * p2[1];
    p1[1] = p3[2] * p2[0] - p3[0] * p2[2];
    p1[2] = p3[0] * p2[1] - p3[1] * p2[0];
    inv = __builtin_sqrtf(p1[1] * p1[1] + p1[0] * p1[0] + p1[2] * p1[2]);
    if (!(inv <= 0.0f)) {
        inv = 1.0f / inv;
    }
    p1[0] = p1[0] * inv;
    p1[1] = p1[1] * inv;
    p1[2] = p1[2] * inv;

    p2[0] = p1[1] * p3[2] - p1[2] * p3[1];
    p2[1] = p1[2] * p3[0] - p1[0] * p3[2];
    p2[2] = p1[0] * p3[1] - p1[1] * p3[0];
    inv = __builtin_sqrtf(p2[1] * p2[1] + p2[0] * p2[0] + p2[2] * p2[2]);
    if (!(inv <= 0.0f)) {
        inv = 1.0f / inv;
    }
    p2[0] = p2[0] * inv;
    p2[1] = p2[1] * inv;
    p2[2] = p2[2] * inv;

    out[0] = a[0];
    out[1] = a[1];
    out[2] = a[2];
    out[4] = b[0];
    out[5] = b[1];
    out[6] = b[2];
    out[8] = c[0];
    out[9] = c[1];
    out[10] = c[2];
    out[12] = pos[0];
    out[13] = pos[1];
    out[14] = pos[2];
    *(u32 *)(arg0 + 0xC) = (*(u32 *)(arg0 + 0xC) | 3) & 0xFFFDFFFF;
    return arg0;
}
