/* func_003b90b0 (spline plugin, module 0x102 open: natural-spline matrix of order n, 4..10):
 * returns a newly allocated n x n float matrix, or NULL after reporting error 0x80000013 for
 * module 0x102 when an allocation fails (flags 0x30102).
 *
 * The matrix is the inverse of the tridiagonal natural-spline system, built in closed form:
 *   A[0] = 1, A[1] = 4, A[i] = 4 A[i-1] - A[i-2]          (n-1 entries; d = A[n-2])
 *   B[0] = 1, B[1] = 2, B[i] = 4 B[i-1] - B[i-2]          (n entries), then B[n-2], B[n-4], ...
 *                                                          are negated
 *   s = 2 for odd n, -2 for even n
 *   M[r][0] = B[n-1-r] / d
 *   M[r][c] = s * B[c] * B[n-1-r] / d                     for 1 <= c <= r
 *   M[n-1][n-1] = M[0][0]
 *   M[r][c] = M[n-1-r][n-1-c]                             for c > r
 * A and B are freed before returning (through the RenderWare free slot at 0x008873EC).
 *
 * Direct transliteration of the retail code; the retail loops are unrolled eight-fold and the
 * unrolled bodies compute exactly the recurrences above. The retail memset of the matrix is
 * dead (every entry is written) and is kept as a clear loop. */

typedef void *(*P4SplineAllocFn)(u32 size, u32 flags);
typedef void (*P4SplineFreeFn)(void *p);
typedef s32 (*P4SplineErrFn)(s32 code, ...);

static void p4_spline_alloc_error(s32 size)
{
    s32 err[2];

    err[0] = 258;
    err[1] = ((P4SplineErrFn)func_003df590)((s32)0x80000013, size);
    func_003df4d0(err);
}

f32 *func_003b90b0(s32 n)
{
    P4SplineAllocFn alloc = *(P4SplineAllocFn *)0x008873E8;
    P4SplineFreeFn release = *(P4SplineFreeFn *)0x008873EC;
    f32 *m;
    s32 *a;
    s32 *b;
    s32 d;
    s32 sign;
    s32 i;
    s32 r;
    s32 c;
    f32 fd;

    m = (f32 *)alloc(n * n * 4, 0x30102);
    if (m == NULL) {
        p4_spline_alloc_error(n * n * 4);
        return NULL;
    }
    /* retail zeroes the block with memset(m, 0, n * n * 4) (the byte count wraps like the allocation size) */
    __builtin_memset(m, 0, (u32)n * (u32)n * 4u);

    a = (s32 *)alloc((n - 1) * 4, 0x30102);
    if (a == NULL) {
        p4_spline_alloc_error((n - 1) * 4);
        release(m);
        return NULL;
    }
    a[0] = 1;
    a[1] = 4;
    for (i = 2; i < n - 1; i++)
        a[i] = a[i - 1] * 4 - a[i - 2];
    d = a[n - 2];
    release(a);

    b = (s32 *)alloc(n * 4, 0x30102);
    if (b == NULL) {
        p4_spline_alloc_error(n * 4);
        release(m);
        return NULL;
    }
    b[0] = 1;
    b[1] = 2;
    for (i = 2; i < n; i++)
        b[i] = b[i - 1] * 4 - b[i - 2];
    for (i = n - 2; i >= 0; i -= 2)
        b[i] = -b[i];

    sign = (n & 1) ? 2 : -2;
    fd = (f32)d;
    for (r = 0; r < n; r++) {
        s32 br = b[n - 1 - r];

        m[r * n] = (f32)br / fd;
        for (c = 1; c <= r; c++)
            m[r * n + c] = (f32)(sign * b[c] * br) / fd;
    }
    m[n * n - 1] = m[0];
    for (r = 0; r < n; r++) {
        for (c = n - 1; c > r; c--)
            m[r * n + c] = m[n * (n - r) - 1 - c];
    }
    release(b);
    return m;
}
