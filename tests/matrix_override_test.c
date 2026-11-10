/* Host checks for the RwMatrix overrides (overrides/003df9f0.c OrthoNormalize, 003e0680.c
 * Rotate, 003e0e20.c Transform). The override bodies are included as they are spliced into the
 * decomp mirror; the decomp's integer types and the RwMatrixMultiply and error callees are
 * defined here. Expected values are worked by hand from the retail asm semantics. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "guest_test.h"

typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
typedef float f32;

static int failures;
static int error_calls;
static s32 last_error_code;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                   \
        }                                                                 \
    } while (0)

/* out = A * B with the retail affine layout: rows 0-2 are 3x3 products, row 3 adds B's row 3. */
u8 *RwMatrixMultiply(u8 *out, u8 *a, u8 *b)
{
    f32 *o = (f32 *)out;
    const f32 *x = (const f32 *)a;
    const f32 *y = (const f32 *)b;
    int r, c, k;

    for (r = 0; r < 4; r++) {
        for (c = 0; c < 3; c++) {
            f32 s = 0.0f;

            for (k = 0; k < 3; k++) {
                s += x[r * 4 + k] * y[k * 4 + c];
            }
            if (r == 3) {
                s += y[12 + c];
            }
            o[r * 4 + c] = s;
        }
        o[r * 4 + 3] = 0.0f;
    }
    return out;
}

s32 func_003df590(s32 code, ...)
{
    error_calls++;
    last_error_code = code;
    return 7;
}

void func_003df4d0(void *err)
{
    (void)err;
}

#ifdef P4_GUEST64
/* On a 64-bit host the overrides are their own rewritten objects (unit64.py build --src), not text included here. */
u8 *func_003df9f0(u8 *arg0, u8 *arg1);
u8 *func_003e0e20(u8 *matrix, u8 *transform, s32 mode);
u8 *func_003e0680(u8 *matrix, const u8 *axis, f32 oneMinusCosine, f32 sine, s32 combineOp);
#else
#include "003df9f0.c"
#include "003e0e20.c"
#include "003e0680.c"
#endif

static int near(f32 a, f32 b)
{
    return fabsf(a - b) < 1e-5f;
}

static int run_test(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    /* OrthoNormalize: rows come out unit length, mutually orthogonal, position copied, and the
     * flags word is (flags | 3) & ~0x20000. */
    {
        f32 in[16] = {1.0f, 0.1f, 0.0f, 0.0f, 0.2f, 2.0f, 0.1f, 0.0f, 0.0f, 0.3f, 3.0f, 0.0f, 5.0f, 6.0f, 7.0f, 0.0f};
        f32 out[16];
        int a, b, c;

        memset(out, 0, sizeof out);
        ((u32 *)out)[3] = 0xFFFFFFFFu;
        CHECK(func_003df9f0((u8 *)out, (u8 *)in) == (u8 *)out);
        for (a = 0; a < 3; a++) {
            f32 len = 0.0f;

            for (c = 0; c < 3; c++) {
                len += out[a * 4 + c] * out[a * 4 + c];
            }
            CHECK(near(len, 1.0f));
            for (b = a + 1; b < 3; b++) {
                f32 dot = 0.0f;

                for (c = 0; c < 3; c++) {
                    dot += out[a * 4 + c] * out[b * 4 + c];
                }
                CHECK(near(dot, 0.0f));
            }
        }
        CHECK(out[12] == 5.0f && out[13] == 6.0f && out[14] == 7.0f);
        CHECK(((u32 *)out)[3] == 0xFFFDFFFFu);
    }
    /* An already orthonormal matrix keeps its rows; a zero-length row stays unscaled and the
     * remaining two rows are still orthogonalised. */
    {
        f32 in[16] = {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0};
        f32 out[16];

        memset(out, 0, sizeof out);
        func_003df9f0((u8 *)out, (u8 *)in);
        CHECK(near(out[1], 1.0f) && near(out[4], -1.0f) && near(out[10], 1.0f));
    }
    {
        f32 in[16] = {0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0};
        f32 out[16];
        f32 dot = 0.0f;
        int c;

        memset(out, 0, sizeof out);
        func_003df9f0((u8 *)out, (u8 *)in);
        for (c = 0; c < 3; c++) {
            dot += out[4 + c] * out[8 + c];
        }
        CHECK(near(dot, 0.0f));
    }

    /* Rotate about z with 1 - cos = 1 and sin = 1 (a quarter turn): m[0] = 0, m[1] = z*s = 1,
     * m[4] = -z*s = -1, m[5] = 0, m[10] = 1, flags word 3, position 0. */
    {
        f32 m[16];
        f32 axis[3] = {0.0f, 0.0f, 1.0f};
        u8 *r;

        memset(m, 0, sizeof m);
        r = func_003e0680((u8 *)m, (u8 *)axis, 1.0f, 1.0f, 0);
        CHECK(r == (u8 *)m);
        CHECK(near(m[0], 0.0f) && near(m[1], 1.0f) && near(m[4], -1.0f) && near(m[5], 0.0f));
        CHECK(near(m[10], 1.0f) && near(m[2], 0.0f) && near(m[6], 0.0f) && near(m[8], 0.0f));
        CHECK(((u32 *)m)[3] == 3u);
        CHECK(m[12] == 0.0f && m[13] == 0.0f && m[14] == 0.0f);
    }
    /* Rotate about x, same angle: m[5] = 1 - (1 - 0)*0... = 1, m[6] = x*s = 1, m[9] = -x*s. */
    {
        f32 m[16];
        f32 axis[3] = {1.0f, 0.0f, 0.0f};

        func_003e0680((u8 *)m, (u8 *)axis, 1.0f, 1.0f, 0);
        CHECK(near(m[0], 1.0f) && near(m[6], 1.0f) && near(m[9], -1.0f) && near(m[5], 0.0f));
    }

    /* Transform: mode 0 replaces, 1 stores transform * matrix, 2 stores matrix * transform, and
     * any other mode reports error code 0x80000003 and returns 0 without touching the matrix. */
    {
        f32 t[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 2, 3, 0};
        f32 base[16] = {2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 10, 20, 30, 0};
        f32 m[16];

        memcpy(m, base, sizeof m);
        CHECK(func_003e0e20((u8 *)m, (u8 *)t, 0) == (u8 *)m);
        CHECK(memcmp(m, t, sizeof m) == 0);

        memcpy(m, base, sizeof m);
        func_003e0e20((u8 *)m, (u8 *)t, 2); /* matrix * transform: translation adds to 10,20,30 */
        CHECK(near(m[12], 11.0f) && near(m[13], 22.0f) && near(m[14], 33.0f) && near(m[0], 2.0f));

        memcpy(m, base, sizeof m);
        func_003e0e20((u8 *)m, (u8 *)t, 1); /* transform * matrix: translation is scaled by 2 */
        CHECK(near(m[12], 12.0f) && near(m[13], 24.0f) && near(m[14], 36.0f) && near(m[0], 2.0f));

        memcpy(m, base, sizeof m);
        error_calls = 0;
        CHECK(func_003e0e20((u8 *)m, (u8 *)t, 3) == 0);
        CHECK(error_calls == 1 && last_error_code == (s32)0x80000003);
        CHECK(memcmp(m, base, sizeof m) == 0);
    }

    if (failures) {
        fprintf(stderr, "matrix_override_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("matrix_override_test: all checks passed\n");
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(run_test, argc, argv);
}
