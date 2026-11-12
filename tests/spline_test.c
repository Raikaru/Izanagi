/* overrides/003b9a40.c (cubic spline control-point fit) with stub allocator slots, table and error
 * callees. Open: an identity weight table reproduces the inputs and mirrored endpoints.
 * Closed: constant inputs, odd/even kernels, a hand-worked n = 4 case, and closing outputs.
 * Low-count cases check retail's exact allocator requests, unconditional seeds, and failure path. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ovr_guest.h"

void *func_003b9a40(unsigned char *object, float *points);

static int checks, failures, errors_posted, spline_count;
static int allocation_count, free_count, fail_next_allocation;
static int allocation_sizes[4], allocation_hints[4], freed_seeds[4][2];

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (!(cond)) {                                                   \
            failures++;                                                  \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
        }                                                                \
    } while (0)

int func_003bb330(void *object) { (void)object; return spline_count; }
int func_003df590(unsigned int code, int size) { (void)code; (void)size; return 7; }
void func_003df4d0(void *record) { (void)record; errors_posted++; }

/* The standalone caller witness reserves padding and a checked-style backpointer even for a
 * zero-byte request. It verifies exact requested sizes, not heap internals; the original-ELF
 * ee_heap_retail fixture separately compares the production checked/newlib heap and headers. */
static void *rw_malloc(int size, int flags)
{
    unsigned char *raw;

    if (allocation_count < 4) {
        allocation_sizes[allocation_count] = size;
        allocation_hints[allocation_count] = flags;
    }
    allocation_count++;
    if (fail_next_allocation) {
        fail_next_allocation = 0;
        return NULL;
    }
    raw = p4pc_heap_calloc(1, (size_t)size + 64);
    if (raw == NULL) {
        return NULL;
    }
    *(uint32_t *)(raw + 12) = P4_H2G(raw);
    return raw + 16;
}

static void rw_free(void *p)
{
    if (free_count < 4) {
        memcpy(freed_seeds[free_count], p, sizeof freed_seeds[free_count]);
    }
    free_count++;
    p4pc_heap_free((unsigned char *)p - 16);
}
/* The retail globals the override reads: the allocator slots (tests/ovr_guest.h) and the weight-table pointers at 0x008864C0. On i386 they are symbols of this test
 * (the override names them by `__asm__` label); on a 64-bit host the override is rewritten (unit64.py build --src) and reads guest memory at those addresses. */
OVR_RW_SLOTS(rw_malloc, rw_free)
#ifdef P4_GUEST64
#define TABLE_SET(k, p) (((uint32_t *)P4_G2H(0x008864C0u))[k] = (p) ? P4_H2G(p) : 0u)
#define TABLES_MAP() (p4t_guest_map(0x008864C0u, 0x2C) == 0)
#else
float *D_008864C0[11];
#define TABLE_SET(k, p) (D_008864C0[k] = (p))
#define TABLES_MAP() 1
#endif

static int near_f(float a, float b) { return fabsf(a - b) < 1e-4f; }

static int run_test(int argc, char **argv)
{
    _Alignas(16) unsigned char object[0x38 + 3 * 4 * 16];
    float *out = (float *)(object + 0x38);
    float pts[8 * 3];
    float identity[25];
    int i;

    (void)argc;
    (void)argv;

    if (!TABLES_MAP() || ovr_slots_init() != 0) {
        return 1;
    }

    for (i = 0; i < 8; i++) {
        pts[i * 3 + 0] = (float)i;
        pts[i * 3 + 1] = (float)(i * i);
        pts[i * 3 + 2] = 1.0f;
    }

    /* Open spline, n = 5, k = 5, identity weights. */
    memset(identity, 0, 25 * sizeof(float));
    for (i = 0; i < 5; i++) {
        identity[i * 5 + i] = 1.0f;
    }
    memset(object, 0, sizeof object);
    TABLE_SET(5, identity);
    *(int *)(object + 0xc) = 1;
    spline_count = 5;
    CHECK(func_003b9a40(object, pts) == object);
    for (i = 0; i < 5; i++) {
        CHECK(near_f(out[(i + 1) * 3 + 0], pts[i * 3 + 0]) && near_f(out[(i + 1) * 3 + 1], pts[i * 3 + 1]));
    }
    /* output 0 is the vector at +0x50, which is output 2 (the second input point): a mirrored start */
    CHECK(near_f(out[0], pts[1 * 3 + 0]) && near_f(out[1], pts[1 * 3 + 1]) && near_f(out[2], pts[1 * 3 + 2]));
    CHECK(near_f(out[6 * 3 + 0], pts[3 * 3 + 0]) && near_f(out[6 * 3 + 1], pts[3 * 3 + 1]));

    /* A missing table and an unknown type report an error and fail. */
    TABLE_SET(5, NULL);
    errors_posted = 0;
    CHECK(func_003b9a40(object, pts) == NULL && errors_posted == 1);
    *(int *)(object + 0xc) = 3;
    CHECK(func_003b9a40(object, pts) == NULL && errors_posted == 2);

    /* Closed spline, n = 4 (k = 4): out_i = (7 P_i - 2 P_{i+1} + P_{i+2} - 2 P_{i-1}) / 4, wrapped. */
    memset(object, 0, sizeof object);
    *(int *)(object + 0xc) = 2;
    spline_count = 4;
    CHECK(func_003b9a40(object, pts) == object);
    for (i = 0; i < 4; i++) {
        float expect = (7 * pts[i * 3] - 2 * pts[((i + 1) % 4) * 3] + pts[((i + 2) % 4) * 3]
                        - 2 * pts[((i + 3) % 4) * 3]) / 4.0f;

        CHECK(near_f(out[(i + 1) * 3], expect));
    }
    CHECK(near_f(out[0], out[4 * 3]));                       /* output 0 repeats output n */
    /* +0x44 and +0x50 are outputs 1 and 2, so the curve closes: outputs n+1 and n+2 repeat them */
    CHECK(near_f(out[5 * 3], out[1 * 3]) && near_f(out[6 * 3], out[2 * 3]));

    /* Constant input stays constant for the odd (5, 7) and even (6, 8) kernels and k = 1. */
    {
        int sizes[5] = {5, 6, 7, 8, 1};
        int s;

        for (s = 0; s < 5; s++) {
            float constant[8 * 3];
            int n = sizes[s];

            for (i = 0; i < n; i++) {
                constant[i * 3 + 0] = 3.0f; constant[i * 3 + 1] = -2.0f; constant[i * 3 + 2] = 0.5f;
            }
            memset(object, 0, sizeof object);
            *(int *)(object + 0xc) = 2;
            spline_count = n;
            CHECK(func_003b9a40(object, constant) == object);
            for (i = 0; i < n; i++) {
                CHECK(near_f(out[(i + 1) * 3], 3.0f) && near_f(out[(i + 1) * 3 + 1], -2.0f)
                      && near_f(out[(i + 1) * 3 + 2], 0.5f));
            }
        }
    }

    /* k = 0/1/2 exposes the old min-two clamp. Retail requests 0/4, 4/4 and 4/8
     * bytes respectively, but stores both seeds in each physical minimum block. */
    {
        static const int requested[3][2] = {{0, 4}, {4, 4}, {4, 8}};
        int n;

        for (n = 0; n <= 2; n++) {
            memset(object, 0, sizeof object);
            *(int *)(object + 0xc) = 2;
            spline_count = n;
            allocation_count = free_count = errors_posted = 0;
            CHECK(func_003b9a40(object, pts) == object);
            CHECK(allocation_count == 2 && free_count == 2 && errors_posted == 0);
            CHECK(allocation_sizes[0] == requested[n][0] && allocation_sizes[1] == requested[n][1]);
            CHECK(allocation_hints[0] == 0x30102 && allocation_hints[1] == 0x30102);
            CHECK(freed_seeds[0][0] == 1 && freed_seeds[0][1] == (n & 1 ? 3 : 4));
            CHECK(freed_seeds[1][0] == (n == 2 ? -1 : 1) && freed_seeds[1][1] == (n & 1 ? 5 : 2));
        }
    }

    /* Differential case 240 has raw count 0 and kind 2, hence n = k = -3.
     * Retail's signed half truncates toward zero: half = -1, length = 0.
     * An arithmetic shift would make length -1 and request UINT32_MAX - 3. */
    memset(object, 0, sizeof object);
    *(int *)(object + 0xc) = 2;
    spline_count = -3;
    allocation_count = free_count = errors_posted = 0;
    CHECK(func_003b9a40(object, pts) == object);
    CHECK(allocation_count == 2 && free_count == 2 && errors_posted == 0);
    CHECK(allocation_sizes[0] == 0 && allocation_sizes[1] == 0);
    CHECK(allocation_hints[0] == 0x30102 && allocation_hints[1] == 0x30102);
    CHECK(freed_seeds[0][0] == 1 && freed_seeds[0][1] == 3);
    CHECK(freed_seeds[1][0] == 1 && freed_seeds[1][1] == 5);

    /* A failed zero-byte allocation must take the retail error path. No seeds or
     * second allocation may happen after the failure. */
    memset(object, 0, sizeof object);
    *(int *)(object + 0xc) = 2;
    spline_count = 0;
    allocation_count = free_count = errors_posted = 0;
    fail_next_allocation = 1;
    CHECK(func_003b9a40(object, pts) == NULL);
    CHECK(allocation_count == 1 && allocation_sizes[0] == 0 && allocation_hints[0] == 0x30102);
    CHECK(free_count == 0 && errors_posted == 1);

    printf("spline_test: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(run_test, argc, argv);
}
