/* func_00440bb8 (2368 bytes): qsort(base arg0, n arg1, element size arg2, compare arg3), the BSD / newlib quicksort with the median of three (nine for n > 40)
 * pivot, a three way partition that keeps the elements equal to the pivot at both ends and swaps them to the middle, an insertion sort when the
 * partition did not swap anything, a recursive call for the left part and a loop for the right. The swap routine depends on the alignment: swaptype 0
 * for an 8 byte aligned base and 8 byte elements (one 64 bit swap), 1 for an 8 byte aligned base and a multiple of 8 (64 bit words), 2 otherwise (bytes).
 * The comparison is called as cmp(a, b) and its int result is tested with its sign; the order of the calls is the original's. */
#include <stdint.h>

typedef int (*P4QsortCmp_00440bb8)(const void *, const void *);
/* Keep explicit long-long: LP64 uint64_t is unsigned long, which the guest
 * rewriter correctly treats as a 32-bit EE word in spliced source. */
typedef unsigned long long P4QsortWord_00440bb8;

static void p4_swapfunc_00440bb8(char *a, char *b, int n, int swaptype)
{
    if (swaptype < 2) {
        P4QsortWord_00440bb8 *pi = (P4QsortWord_00440bb8 *)a;
        P4QsortWord_00440bb8 *pj = (P4QsortWord_00440bb8 *)b;
        int32_t i = n / 8;

        do {
            P4QsortWord_00440bb8 t = *pi;
            *pi++ = *pj;
            *pj++ = t;
        } while (--i > 0);
    } else {
        char *pi = a;
        char *pj = b;
        int32_t i = n;

        do {
            char t = *pi;
            *pi++ = *pj;
            *pj++ = t;
        } while (--i > 0);
    }
}

static char *p4_med3_00440bb8(char *a, char *b, char *c, P4QsortCmp_00440bb8 cmp)
{
    return cmp(a, b) < 0 ? (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a))
                         : (cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c));
}

#define P4_SWAP_00440bb8(x, y)                                  \
    do {                                                        \
        if (swaptype == 0) {                                    \
            P4QsortWord_00440bb8 t_ = *(P4QsortWord_00440bb8 *)(x); \
            *(P4QsortWord_00440bb8 *)(x) = *(P4QsortWord_00440bb8 *)(y); \
            *(P4QsortWord_00440bb8 *)(y) = t_;                   \
        } else {                                                \
            p4_swapfunc_00440bb8((x), (y), (int)es, swaptype);  \
        }                                                       \
    } while (0)

#define P4_VECSWAP_00440bb8(x, y, len)                          \
    do {                                                        \
        if ((len) > 0) {                                        \
            p4_swapfunc_00440bb8((x), (y), (len), swaptype);    \
        }                                                       \
    } while (0)

/* Keep the public callback explicit: the seam/signature audit must see a code pointer. */
void func_00440bb8(char *a, uint32_t n, uint32_t es, int (*cmp)(const void *, const void *))
{
    char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
    int d, r, swaptype, swap_cnt;

loop:
    swaptype = ((uintptr_t)a % 8 != 0 || es % 8 != 0) ? 2 : (es == 8 ? 0 : 1);
    swap_cnt = 0;
    if (n < 7) {
        for (pm = a + es; pm < a + n * es; pm += es) {
            for (pl = pm; pl > a && cmp(pl - es, pl) > 0; pl -= es) {
                P4_SWAP_00440bb8(pl, pl - es);
            }
        }
        return;
    }
    pm = a + (n / 2) * es;
    if (n > 7) {
        pl = a;
        pn = a + (n - 1) * es;
        if (n > 40) {
            d = (n / 8) * es;
            pl = p4_med3_00440bb8(pl, pl + d, pl + 2 * d, cmp);
            pm = p4_med3_00440bb8(pm - d, pm, pm + d, cmp);
            pn = p4_med3_00440bb8(pn - 2 * d, pn - d, pn, cmp);
        }
        pm = p4_med3_00440bb8(pl, pm, pn, cmp);
    }
    P4_SWAP_00440bb8(a, pm);
    pa = pb = a + es;
    pc = pd = a + (n - 1) * es;
    for (;;) {
        while (pb <= pc && (r = cmp(pb, a)) <= 0) {
            if (r == 0) {
                swap_cnt = 1;
                P4_SWAP_00440bb8(pa, pb);
                pa += es;
            }
            pb += es;
        }
        while (pb <= pc && (r = cmp(pc, a)) >= 0) {
            if (r == 0) {
                swap_cnt = 1;
                P4_SWAP_00440bb8(pc, pd);
                pd -= es;
            }
            pc -= es;
        }
        if (pb > pc) {
            break;
        }
        P4_SWAP_00440bb8(pb, pc);
        swap_cnt = 1;
        pb += es;
        pc -= es;
    }
    if (swap_cnt == 0) {
        for (pm = a + es; pm < a + n * es; pm += es) {
            for (pl = pm; pl > a && cmp(pl - es, pl) > 0; pl -= es) {
                P4_SWAP_00440bb8(pl, pl - es);
            }
        }
        return;
    }
    pn = a + n * es;
    r = (int)(pa - a) < (int)(pb - pa) ? (int)(pa - a) : (int)(pb - pa);
    P4_VECSWAP_00440bb8(a, pb - r, r);
    r = (uint32_t)(pd - pc) < (uint32_t)(pn - pd - es) ? (int)(pd - pc) : (int)(pn - pd - es);
    P4_VECSWAP_00440bb8(pb, pn - r, r);
    if ((uint32_t)(r = (int)(pb - pa)) > es) {
        func_00440bb8(a, (uint32_t)r / es, es, cmp);
    }
    if ((uint32_t)(r = (int)(pd - pc)) > es) {
        a = pn - r;
        n = (uint32_t)r / es;
        goto loop;
    }
}
