/* Differential harness: runs the C translation of a function on a memory image and writes the
 * result back. Used by run.py against the retail asm interpreter (mips.py).
 *
 * Case file (little endian): u32 ncases; per case: u32 fn_index, u32 args[8], u32 npages,
 * then npages x { u32 addr, u8 data[4096] }. Result file: per case u32 ret, then every page's
 * 4096 bytes after the call, in order. Pages are mapped at their addresses (MAP_FIXED). Slots
 * listed in the optional fixup section are overwritten with native hook addresses first:
 * u32 nfix; per fixup { u32 addr, u32 hook_index }. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

typedef struct {
    const char *name;
    void *fn;
} Entry;

extern const Entry p4_table[];
extern const int p4_table_len;

/* Hook log: native stand-ins for indirect callees record their arguments here. */
uint32_t p4_log[1280];
int p4_log_n;
uint32_t p4_ret[256];          /* per-callee return values for this case (callee ids 0..127, hooks 128+) */

uint32_t p4_ncalls[256];
uint32_t p4_case_id;

/* Stub body: ids with a DEREF index (1..3, in p4_deref) log the word the argument at that
 * position points at instead of the pointer.
 * A return value 0xFExxxxxx means "return 1 for the first (x & 0xFFFF) calls, then 0". */
uint32_t p4_deref[256];

uint32_t p4_record(uint32_t id, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    uint32_t n = p4_ncalls[id]++;
    uint32_t r = p4_ret[id];

    if (p4_deref[id] == 9 && b) {
        uint32_t *w = (uint32_t *)(uintptr_t)b;

        b = w[0] + 3 * w[1] + 5 * w[2] + 7 * w[3] + 11 * w[4];
    } else if (p4_deref[id] == 1 && b) {
        b = *(uint32_t *)(uintptr_t)b;
    } else if (p4_deref[id] == 2 && c) {
        c = *(uint32_t *)(uintptr_t)c;
    } else if (p4_deref[id] == 3 && d) {
        d = *(uint32_t *)(uintptr_t)d;
    }
    if ((r >> 24) == 0xFD) {
        r = (r >> n) & 1;
    } else if ((r >> 24) == 0xFE) {
        r = n < (r & 0xFFFF) ? 1 : 0;
    }
    if (p4_log_n + 5 <= 1280) {
        p4_log[p4_log_n++] = id;
        p4_log[p4_log_n++] = a;
        p4_log[p4_log_n++] = b;
        p4_log[p4_log_n++] = c;
        p4_log[p4_log_n++] = d;
    }
    return r;
}

uint32_t p4_hook0(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return p4_record(128, a, b, c, d);
}

uint32_t p4_hook1(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return p4_record(129, a, b, c, d);
}

/* Allocator stand-ins: a bump allocator over the pool region mapped by the case (POOL_BASE). */
#define POOL_BASE 0x38000000u
static uint32_t p4_bump;

uint32_t p4_hook2(uint32_t size, uint32_t tag, uint32_t c, uint32_t d)
{
    uint32_t r = p4_bump;

    p4_record(130, size, tag, c, d);
    p4_bump += (size + 15) & ~15u;
    return r;
}

uint32_t p4_hook3(uint32_t pool, uint32_t tag, uint32_t c, uint32_t d)
{
    uint32_t r = p4_bump;

    p4_record(131, pool, tag, c, d);
    p4_bump += 0x100;
    return r;
}

uint32_t p4_hook4(uint32_t ptr, uint32_t b, uint32_t c, uint32_t d)
{
    return p4_record(132, ptr, b, c, d);
}

/* Writer: sets four flag words of the work block a (+0x10, +0x18, +0x20, +0x28) from bits of d. */
uint32_t p4_hook5(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    uint32_t *w = (uint32_t *)(uintptr_t)a;

    p4_record(133, a, b, c, d);
    if (a) {
        w[4] = d & 1;
        w[6] = (d >> 1) & 1;
        w[8] = (d >> 2) & 1;
        w[10] = (d >> 3) & 1;
    }
    return 0;
}

/* strlen stand-in: logs the pointer and returns the length. */
uint32_t p4_hook6(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    p4_record(134, a, 0, 0, 0);
    return (uint32_t)strlen((const char *)(uintptr_t)a);
}

/* Hit callback stand-in: (a, &hit, c, float d); logs a, a hash of the 12-word hit record (word 10 is padding),
 * c and the bits of d. Case arguments equal to HOOK_BASE + 7 are replaced by this function's address. */
/* The host callback shape RpCollisionTriangleCallBack(intersection, triangle, distance, data): the float is the third parameter (retail passes it in $f12 and the data in $a2). */
uint32_t p4_hook7(uint32_t a, uint32_t hitp, float d, uint32_t c)
{
    uint32_t *w = (uint32_t *)(uintptr_t)hitp;
    uint32_t h = 0;
    uint32_t i;
    union {
        float f;
        uint32_t u;
    } fb;

    for (i = 0; i < 12; i++) {
        if (i != 10 && !(i >= 7 && i <= 9 && w[i] >= 0x80000000u)) {
            h = h * 31u + w[i];
        }
    }
    fb.f = d;
    return p4_record(135, a, h, c, fb.u);
}

/* Key frame to matrix stand-in: (out, key); logs a hash of the first 12 key words and fills the 16 output words. */
uint32_t p4_hook8(uint32_t outp, uint32_t keyp, uint32_t c, uint32_t d)
{
    uint32_t *o = (uint32_t *)(uintptr_t)outp;
    uint32_t *k = (uint32_t *)(uintptr_t)keyp;
    uint32_t h = 0;
    uint32_t i;

    for (i = 0; i < 12; i++) {
        h = h * 31u + k[i];
    }
    p4_record(136, 0, h, 0, 0);
    for (i = 0; i < 16; i++) {
        o[i] = k[i % 12] + i * 7u;
    }
    return 0;
}

/* Register read stand-in: (reg, out); logs reg and stores a value derived from it and the call number. */
uint32_t p4_hook9(uint32_t reg, uint32_t outp, uint32_t c, uint32_t d)
{
    uint32_t n = p4_ncalls[137];

    p4_record(137, reg, 0, 0, 0);
    *(uint32_t *)(uintptr_t)outp = reg * 0x101u + n;
    return 0;
}

typedef struct { unsigned a; void *f; } FnEntry;
extern const FnEntry p4_fn_table[];

/* Ray-hit callback stand-in: (a, node, &hit, ctx, float t); logs a + 31 * node, a hash of the 12-word hit record (word 10 padding, stack-frame
 * pointers in words 7 to 9 skipped), ctx and the bits of t. Case arguments equal to HOOK_BASE + 10 are replaced by this function's address. */
uint32_t p4_hook10(uint32_t a, uint32_t node, uint32_t hitp, uint32_t ctx, float t)
{
    uint32_t *w = (uint32_t *)(uintptr_t)hitp;
    uint32_t h = 0;
    uint32_t i;
    union {
        float f;
        uint32_t u;
    } fb;

    for (i = 0; i < 12; i++) {
        if (i != 10 && !(i >= 7 && i <= 9 && w[i] >= 0x80000000u)) {
            h = h * 31u + w[i];
        }
    }
    fb.f = t;
    return p4_record(138, a + 31u * node, h, ctx, fb.u);
}

/* Handler stand-in that shows the object's data word: (a): logs a and the word at (a[1] + 0x18) (func_0039f050 advances that word around the call). */
uint32_t p4_hook11(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    uint32_t *o = (uint32_t *)(uintptr_t)a;
    uint32_t *dd = (uint32_t *)(uintptr_t)o[1];

    return p4_record(139, a, dd[6], 0, 0);
}

static void *const hooks[] = {(void *)p4_hook0, (void *)p4_hook1, (void *)p4_hook2, (void *)p4_hook3, (void *)p4_hook4, (void *)p4_hook5, (void *)p4_hook6, (void *)p4_hook7, (void *)p4_hook8, (void *)p4_hook9, (void *)p4_hook10, (void *)p4_hook11};

int main(int argc, char **argv)
{
    FILE *in = fopen(argv[1], "rb");
    FILE *out = fopen(argv[2], "wb");
    uint32_t ncases;
    uint32_t c;

    if (!in || !out) {
        return 2;
    }
    fread(&ncases, 4, 1, in);
    for (c = 0; c < ncases; c++) {
        uint32_t idx;
        uint32_t args[9];
        uint32_t npages;
        uint32_t nfix;
        uint32_t i;
        uint32_t (*fn)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
        uint32_t ret;
        uint32_t *addrs;

        fread(&idx, 4, 1, in);
        fread(args, 4, 9, in);
        fread(&npages, 4, 1, in);
        addrs = malloc(npages * 4);
        for (i = 0; i < npages; i++) {
            uint32_t a;
            void *p;

            fread(&a, 4, 1, in);
            addrs[i] = a;
            p = mmap((void *)(uintptr_t)a, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
            if (p != (void *)(uintptr_t)a) {
                perror("mmap");
                return 3;
            }
            fread(p, 1, 4096, in);
        }
        fread(&nfix, 4, 1, in);
        for (i = 0; i < nfix; i++) {
            uint32_t a;
            uint32_t h;

            fread(&a, 4, 1, in);
            fread(&h, 4, 1, in);
            if (h >= 0x10000u) {
                unsigned k;

                for (k = 0; p4_fn_table[k].a != 0; k++) {
                    if (p4_fn_table[k].a == h - 0x10000u) {
                        *(uint32_t *)(uintptr_t)a = (uint32_t)(uintptr_t)p4_fn_table[k].f;
                    }
                }
            } else {
                *(uint32_t *)(uintptr_t)a = (uint32_t)(uintptr_t)hooks[h];
            }
        }
        p4_log_n = 0;
        p4_case_id = c + 1;
        p4_bump = POOL_BASE;
        {
            uint32_t nret;

            memset(p4_ret, 0, sizeof p4_ret);
            memset(p4_ncalls, 0, sizeof p4_ncalls);
            memset(p4_deref, 0, sizeof p4_deref);
            fread(&nret, 4, 1, in);
            fread(p4_ret, 4, nret, in);
            fread(&nret, 4, 1, in);
            fread(p4_deref, 4, nret, in);
        }
        for (i = 0; i < 9; i++) {
            if (args[i] >= 0x00500000u && args[i] < 0x0050000Bu) {
                args[i] = (uint32_t)(uintptr_t)hooks[args[i] - 0x00500000u];
            }
        }
        fn = (void *)p4_table[idx].fn;
        ret = fn(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
        fwrite(&ret, 4, 1, out);
        {
            uint32_t n = (uint32_t)p4_log_n;

            fwrite(&n, 4, 1, out);
            fwrite(p4_log, 4, 1280, out);
        }
        for (i = 0; i < npages; i++) {
            fwrite((void *)(uintptr_t)addrs[i], 1, 4096, out);
            munmap((void *)(uintptr_t)addrs[i], 4096);
        }
        free(addrs);
    }
    fclose(out);
    return 0;
}
