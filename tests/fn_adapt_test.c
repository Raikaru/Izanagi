/* Host checks for the indirect-call adapters of the 64-bit model (src/platform/guest.c, include/p4pc/guest.h): a function registered with its
 * own register signature, called through a call site that declares another one, with floats among the parameters and in the result. The
 * expected values are worked by hand. Nothing to check on a 32-bit host (every class is the same there): the test skips (exit 77). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/guest.h"

#ifdef P4_GUEST64

static int fails;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "fn_adapt_test: FAIL %s (line %d)\n", #c, __LINE__); fails++; } } while (0)

/* (void *, float): the pointer is a host pointer, the result a float. */
static float scale_of(void *p, float f)
{
    return f * (float)*(int32_t *)p;
}

/* A float between two pointers: the second pointer is the third integer register, not the second. */
static int32_t sum_mid(void *a, float f, void *b)
{
    return *(int32_t *)a + (int32_t)f + *(int32_t *)b;
}

/* A float first, an integer second: the result is a pointer. */
static void *pick(float f, int32_t index)
{
    static int32_t *table;
    (void)f;
    if (table == NULL) {
        table = P4_G2H(p4_gport_alloc(64, 16));
        for (int i = 0; i < 16; i++) table[i] = 100 + i;
    }
    return table + index;
}

/* The result is a double, the parameters two doubles and a pointer. */
static double blend(double a, void *p, double b)
{
    return a * *(int32_t *)p + b;
}

int main(void)
{
    uint32_t a, b;
    int32_t *pa, *pb;
    uint32_t sig;

    if (p4t_guest_init() != 0) {
        fprintf(stderr, "fn_adapt_test: no guest block\n");
        return 1;
    }
    a = p4_gport_alloc(16, 16);
    b = p4_gport_alloc(16, 16);
    pa = P4_G2H(a);
    pb = P4_G2H(b);
    pa[0] = 7;
    pb[0] = 30;

    p4_fn_register_sig(0x00480000u, (void *)scale_of, "xpx");
    p4_fn_register_sig(0x00480010u, (void *)sum_mid, "ipxp");
    p4_fn_register_sig(0x00480020u, (void *)pick, "pxi");
    p4_fn_register_sig(0x00480030u, (void *)blend, "xxpx");

    /* the call site of m2c code: the pointer is a guest address (an integer), the float is a float, the result a float */
    sig = P4_SIG(2, 0, 0, 0, 0, 0, 0, 0) | P4_SIGX(1) | P4_SIGXR;
    {
        float (*f)(uint32_t, float) = (float (*)(uint32_t, float))p4_fn_g2h_sig(0x00480000u, sig);
        CHECK(f != (void *)scale_of);                                   /* adapted: the classes differ */
        CHECK(f(a, 2.5f) == 17.5f);
    }
    /* the same function from a typed call site: no adapter */
    sig = P4_SIG(2, 0, 1, 0, 0, 0, 0, 0) | P4_SIGX(1) | P4_SIGXR;
    CHECK(p4_fn_g2h_sig(0x00480000u, sig) == (void *)scale_of);

    /* a float between two integer-declared pointers */
    sig = P4_SIG(3, 0, 0, 0, 0, 0, 0, 0) | P4_SIGX(1);
    {
        int32_t (*f)(uint32_t, float, uint32_t) = (int32_t (*)(uint32_t, float, uint32_t))p4_fn_g2h_sig(0x00480010u, sig);
        CHECK(f(a, 5.0f, b) == 42);
    }

    /* a float first, an integer next, a pointer result seen as an integer */
    sig = P4_SIG(2, 0, 0, 0, 0, 0, 0, 0) | P4_SIGX(0);
    {
        uint32_t (*f)(float, int32_t) = (uint32_t (*)(float, int32_t))p4_fn_g2h_sig(0x00480020u, sig);
        uint32_t r = f(1.0f, 3);
        CHECK(*(int32_t *)P4_G2H(r) == 103);
    }

    /* doubles: the result, two parameters around a pointer */
    sig = P4_SIG(3, 0, 0, 0, 0, 0, 0, 0) | P4_SIGX(0) | P4_SIGX(2) | P4_SIGXR;
    {
        double (*f)(double, uint32_t, double) = (double (*)(double, uint32_t, double))p4_fn_g2h_sig(0x00480030u, sig);
        CHECK(f(2.0, a, 0.5) == 14.5);
    }

    if (fails) return 1;
    printf("fn_adapt_test: ok\n");
    return 0;
}

#else

int main(void)
{
    fprintf(stderr, "fn_adapt_test: nothing to check on a 32-bit host, skipping\n");
    return 77;
}

#endif
