/* The sky driver's VU0 matrix and vector multiplies (src/platform/rw_skytran.c: 00402390, 00402410, 00402470, 00402510, 00402570)
 * against the unchanged retail instructions in the R5900 interpreter (tools/mipsdiff/r5900.c). Both sides start from the same VU0
 * registers and guest bytes; every byte of the matrices, points and results and every VU0 register the routine leaves (vf1..vf31,
 * ACC) must be equal. The cases are deterministic: rotation-like and translation entries, points, zeros, negative zeros and
 * denormals, random w lanes (flags words, pads) and random initial registers (matrixASMMult stores vf10..vf12's w lanes). The
 * magnitudes stay within 2^-6..2^10, so no sum mixes terms more than 40 bits apart: there the interpreter's extended-precision
 * add rounds to nearest before it truncates, which is not the VU's exact truncated sum the natives compute. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/hostmath.h"
#include "p4pc/image.h"
#include "p4pc/rw_skytran.h"
#include "p4pc/vu0.h"
#include "r5900.h"

#define CASES 2000
#define AREA_ADDR 0x01c00000u   /* guest RAM the interpreter accepts (below 32 MiB), above the loaded image */
#define STACK_TOP 0x01ff0000u
#define AREA 0x300u
#define MAT_A 0x000u
#define MAT_B 0x040u
#define DST 0x080u
#define IN 0x100u              /* up to 8 points and the one retail reads past them */
#define OUT 0x200u

enum { MATRIX, POINT, POINTS, VECTOR, VECTORS, KINDS };
static const char *const names[KINDS] = {"00402390 matrixASMMult", "00402410 vectorASMMultPointSingle", "00402470 vectorASMMultPoints",
                                         "00402510 vectorASMMultVectorSingle", "00402570 vectors"};
static const uint32_t entries[KINDS] = {0x00402390u, 0x00402410u, 0x00402470u, 0x00402510u, 0x00402570u};

static unsigned char *area;
static uint64_t rng = UINT64_C(0x2545f4914f6cdd1d);
static uint32_t vu_init[33][4];

static uint32_t next(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return (uint32_t)(rng >> 32);
}

/* A float word: 1 in 16 a zero, a negative zero or a denormal, otherwise a random 24-bit mantissa with an exponent in 2^-6..2^10. */
static uint32_t value(void)
{
    const uint32_t r = next();

    switch (r & 15u) {
    case 0:
        return 0u;
    case 1:
        return 0x80000000u;
    case 2:
        return (r & 0x80000000u) | ((r >> 4) & 0x7fffffu) | 1u;
    default:
        return (r & 0x80000000u) | ((127u - 6u + (r >> 8) % 17u) << 23) | (next() & 0x7fffffu);
    }
}

static void put(unsigned off, uint32_t w)
{
    memcpy(area + off, &w, sizeof w);
}

/* Fills the area and the initial VU0 registers for one case and returns the point count (0..8; -1 once in a while). */
static int32_t fill(void)
{
    unsigned i;
    int32_t count;

    for (i = 0; i < AREA; i += 4) {
        put(i, i < IN && (i & 15u) == 12u ? next() : value());   /* a matrix row's w: flags or pad, any bits */
    }
    for (i = 1; i < 33; i++) {
        unsigned l;

        for (l = 0; l < 4; l++) {
            vu_init[i][l] = value();
        }
    }
    vu_init[0][0] = vu_init[0][1] = vu_init[0][2] = 0u;
    vu_init[0][3] = 0x3f800000u;
    count = (int32_t)(next() % 10u) - 1;
    return count;
}

static int retail(int kind, int32_t count, int alias, uint32_t *ret)
{
    R5900 c;
    int rc, i;

    memset(&c, 0, sizeof c);
    c.lo_pc = 0x00100000u;
    c.hi_pc = 0x005e4000u;
    c.max_steps = 1000000;
    c.r[28] = 0x007690f0u;
    c.r[29] = STACK_TOP;
    for (i = 0; i < 32; i++) {
        memcpy(c.vf[i], vu_init[i], 16);
    }
    memcpy(c.vacc, vu_init[32], 16);
    switch (kind) {
    case MATRIX:
        c.r[4] = AREA_ADDR + (alias ? MAT_A : DST);
        c.r[5] = AREA_ADDR + MAT_A;
        c.r[6] = AREA_ADDR + MAT_B;
        break;
    case POINT:
    case VECTOR:
        c.r[4] = AREA_ADDR + (alias ? IN : OUT);
        c.r[5] = AREA_ADDR + IN;
        c.r[6] = AREA_ADDR + MAT_A;
        break;
    default:
        c.r[4] = AREA_ADDR + (alias ? IN : OUT);
        c.r[5] = AREA_ADDR + IN;
        c.r[6] = (uint64_t)(int64_t)count;
        c.r[7] = AREA_ADDR + MAT_A;
        break;
    }
    rc = r5900_call(&c, entries[kind]);
    if (rc != R5900_OK) {
        fprintf(stderr, "%s: interpreter rc=%d pc=%08x word=%08x reason=%s\n", names[kind], rc, c.bad_pc, c.bad_word,
                c.fault_why != NULL ? c.fault_why : "none");
        exit(1);
    }
    for (i = 0; i < 32; i++) {
        memcpy(vu_init[i], c.vf[i], 16);   /* the registers the retail code leaves */
    }
    memcpy(vu_init[32], c.vacc, 16);
    *ret = (uint32_t)c.r[2];
    return 0;
}

static uint32_t native(int kind, int32_t count, int alias)
{
    void *r = NULL;

    switch (kind) {
    case MATRIX:
        func_00402390(area + (alias ? MAT_A : DST), area + MAT_A, area + MAT_B);
        return 0;
    case POINT:
        r = func_00402410(area + (alias ? IN : OUT), area + IN, area + MAT_A);
        break;
    case POINTS:
        r = func_00402470(area + (alias ? IN : OUT), area + IN, count, area + MAT_A);
        break;
    case VECTOR:
        r = func_00402510(area + (alias ? IN : OUT), area + IN, area + MAT_A);
        break;
    case VECTORS:
        r = func_00402570(area + (alias ? IN : OUT), area + IN, count, area + MAT_A);
        break;
    }
    return P4_H2G(r);
}

static int test_main(int argc, char **argv)
{
    const char *elf = getenv("P4PC_ELF");
    static unsigned char input[AREA], expected[AREA];
    static uint32_t start[33][4];
    int kind, n, failures = 0;

    (void)argc;
    (void)argv;
    if (elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "sky_vector_vu0_test: P4PC_ELF unset, skipped\n");
        return 77;
    }
    p4pc_load_image(elf);
    p4pc_apply_patches();
    if (!p4pc_fpu_ee_mode()) {
        fprintf(stderr, "sky_vector_vu0_test: P4PC_EE_ROUNDING=near, skipped\n");
        return 77;
    }
    area = P4_G2H(AREA_ADDR);
    for (kind = 0; kind < KINDS; kind++) {
        int differ = 0;

        for (n = 0; n < CASES; n++) {
            const int32_t count = fill();
            const int alias = (next() & 7u) == 0u;
            uint32_t ret_retail, ret_native;
            int i, bad;

            memcpy(input, area, AREA);
            memcpy(start, vu_init, sizeof start);
            retail(kind, count, alias, &ret_retail);
            memcpy(expected, area, AREA);
            memcpy(area, input, AREA);
            for (i = 0; i < 32; i++) {
                memcpy(p4pc_vu0.vf[i].u, start[i], 16);
            }
            memcpy(p4pc_vu0.acc.u, start[32], 16);
            ret_native = native(kind, count, alias);
            bad = memcmp(area, expected, AREA) != 0 || (kind != MATRIX && ret_native != ret_retail);
            for (i = 1; i < 32 && !bad; i++) {
                bad = memcmp(p4pc_vu0.vf[i].u, vu_init[i], 16) != 0;
            }
            if (!bad) {
                bad = memcmp(p4pc_vu0.acc.u, vu_init[32], 16) != 0;
            }
            if (bad) {
                if (differ < 4) {
                    unsigned off = 0;

                    while (off < AREA && area[off] == expected[off]) {
                        off++;
                    }
                    if (off < AREA) {
                        uint32_t got, want;

                        off &= ~3u;
                        memcpy(&got, area + off, 4);
                        memcpy(&want, expected + off, 4);
                        printf("FAIL %s case %d: word +0x%03x native %08x, retail %08x\n", names[kind], n, off, (unsigned)got,
                               (unsigned)want);
                    } else {
                        printf("FAIL %s case %d: return %08x/%08x or VU0 registers differ\n", names[kind], n, (unsigned)ret_native,
                               (unsigned)ret_retail);
                    }
                }
                differ++;
            }
        }
        printf("%s: %d of %d cases differ from the retail instructions\n", names[kind], differ, CASES);
        failures += differ;
    }
    return failures != 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
