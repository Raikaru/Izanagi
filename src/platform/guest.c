/* Guest memory block and function table of the 64-bit build (include/p4pc/guest.h, docs/port64.md).
 * On a 32-bit host a guest address is a host address and only the stack arena calls remain. */
#include "p4pc/guest.h"

/* Native metadata predicate; do not parse its body in every mirrored EE unit. */
int p4_guest_asset_buffer_contains(const void *pointer, size_t size)
{
    uintptr_t address=(uintptr_t)pointer;
    if (!pointer || !size) return 0;
#ifdef P4_GUEST64
    uintptr_t base=(uintptr_t)p4_guest_base;
    uintptr_t offset;
    if (!p4_guest_base || address<base) return 0;
    offset=address-base;
    if (offset<0x00100000u || offset>=P4_GUEST_SIZE || size>P4_GUEST_SIZE-offset) return 0;
    if (offset>=P4_GUEST_STACKS && offset<P4_GUEST_STACKS+P4_GUEST_STACKS_SIZE) return 0;
    if (offset<P4_GUEST_STACKS && size>P4_GUEST_STACKS-offset) return 0;
    return 1;
#else
    /* image.c maps retail RAM [1 MiB,32 MiB) in the identity reference. */
    return address>=0x00100000u && address<0x02000000u &&
        size<=0x02000000u-address;
#endif
}

#ifndef P4_GUEST64
#include <stdlib.h>

void *p4_guest_stack_alloc(size_t size) { return malloc(size); }
void p4_guest_stack_free(void *p) { free(p); }
#endif

#ifdef P4_GUEST64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/os.h"

uint8_t *p4_guest_base;
int p4_guest_check_off;   /* set by a fuzzing harness (tools/mipsdiff/ovdiff.c): arbitrary words round-trip through P4_H2G unchecked, as on i386 */

uint8_t *p4_guest_image;
size_t p4_guest_image_size;

static void (**init_fns)(void);
static size_t n_init, cap_init;
static int inited;

void p4_guest_on_init(void (*fn)(void))
{
    if (inited) {
        fn();
        return;
    }
    if (n_init == cap_init) {
        cap_init = cap_init ? cap_init * 2 : 64;
        init_fns = realloc(init_fns, cap_init * sizeof *init_fns);
    }
    init_fns[n_init++] = fn;
}

static int guest_guard_hit(uint64_t guest_off);   /* the stack arena's guard pages (below) */

#if defined(P4_GUEST_CHECK)
/* The OS handler owns signals/exceptions and the native trace on every target.
 * Keep guest address and stack-guard diagnosis identical across platforms. */
static void guest_fault_note(uintptr_t a)
{
    uint64_t base = (uint64_t)(uintptr_t)p4_guest_base;
    if (a >= base && a - base < 0x100000000ull && guest_guard_hit(a - base))
        fprintf(stderr, "p4 guest fault: access on guest address 0x%08x: a coroutine stack overflowed into its guard page\n", (unsigned)(a - base));
    else if (a >= base && a - base < 0x100000000ull)
        fprintf(stderr, "p4 guest fault: access on guest address 0x%08x\n", (unsigned)(a - base));
    else if (((uint64_t)a & 0xFFFF000000000000ull) == 0xDEAD000000000000ull)
        fprintf(stderr, "p4 guest fault: call of guest code address 0x%08x, which has no function in the table\n", (unsigned)a);
}

static void guest_segv_install(void) { p4_os_fault_note = guest_fault_note; }
#else
static void guest_segv_install(void) {}
#endif

int p4_guest_init(void)
{
    if (inited) return 0;
    /* 4 GB of address space, the first 64 MB usable: base + any 32-bit guest address is either the block or an inaccessible page */
    /* At a multiple of 4 GB when the address space allows it: then the low 32 bits of a host pointer into the block ARE its guest address (as on i386), whatever
     * ASLR does to the upper half. Words of the allocator's links and of other host pointers that the game reads before writing them (fresh heap memory) then
     * hold the same 32-bit value every run, and the one the i386 build holds. */
    void *p = p4_os_reserve_aligned(0x100000000ull + 0x1000, P4_GUEST_SIZE, 0x100000000ull);
    if (!p) p = p4_os_reserve_anywhere(0x100000000ull + 0x1000, P4_GUEST_SIZE);
    if (!p) return -1;
    p4_guest_base = (uint8_t *)p;
    uintptr_t lo, hi;
    if (p4_os_image_range(&lo, &hi) == 0 && hi > lo) {
        p4_guest_image = (uint8_t *)lo;
        p4_guest_image_size = hi - lo;
        p4_guest_window(P4_GUEST_IMAGE, p4_guest_image, p4_guest_image_size);
    }
    p4_guest_window(P4_GUEST_EEREGS, p4_os_map_anywhere(P4_GUEST_EEREGS_SIZE), P4_GUEST_EEREGS_SIZE);
    p4_guest_window(P4_GUEST_SPR, p4_os_map_anywhere(P4_GUEST_SPR_SIZE), P4_GUEST_SPR_SIZE);
    inited = 1;
    guest_segv_install();
    for (size_t i = 0; i < n_init; i++) init_fns[i]();
    free(init_fns);
    init_fns = NULL;
    n_init = cap_init = 0;
    return 0;
}

/* The stack arena: at most 64 live stacks, first fit on a table sorted by address. */
typedef struct { uint32_t off, size; } StackSeg;
static StackSeg stacks[64];
static int n_stacks;

#define STACK_PAGE 4096u

/* A stack is its usable memory behind one guard page (the page below it is inaccessible), so an overflow is a fault that names the stack and not
 * silent damage to the stack below; the usable memory starts on a page boundary, which Windows' stack-limit probing (__chkstk) needs. */
void *p4_guest_stack_alloc(size_t size)
{
    uint64_t want = STACK_PAGE + (((uint64_t)size + STACK_PAGE - 1) & ~(uint64_t)(STACK_PAGE - 1));
    uint32_t sz = (uint32_t)want, at = 0;
    if (!p4_guest_base || !size || want > P4_GUEST_STACKS_SIZE || n_stacks == 64) return NULL;
    int i = 0;
    for (; i < n_stacks; i++) {
        if (stacks[i].off - at >= sz) break;
        at = stacks[i].off + stacks[i].size;
    }
    if (i == n_stacks && P4_GUEST_STACKS_SIZE - at < sz) return NULL;
    memmove(&stacks[i + 1], &stacks[i], (size_t)(n_stacks - i) * sizeof *stacks);
    stacks[i] = (StackSeg){at, sz};
    n_stacks++;
    uint8_t *base = p4_guest_base + P4_GUEST_STACKS + at;
    p4_os_protect_pages(base, STACK_PAGE, 0);
    return base + STACK_PAGE;
}

void p4_guest_stack_free(void *p)
{
    if (!p) return;
    uint32_t off = (uint32_t)((uint8_t *)p - (p4_guest_base + P4_GUEST_STACKS)) - STACK_PAGE;
    for (int i = 0; i < n_stacks; i++)
        if (stacks[i].off == off) {
            p4_os_protect_pages(p4_guest_base + P4_GUEST_STACKS + off, STACK_PAGE, 1);
            memmove(&stacks[i], &stacks[i + 1], (size_t)(n_stacks - i - 1) * sizeof *stacks);
            n_stacks--;
            return;
        }
}

static int guest_guard_hit(uint64_t off)
{
    for (int i = 0; i < n_stacks; i++)
        if (off >= P4_GUEST_STACKS + stacks[i].off && off < P4_GUEST_STACKS + stacks[i].off + STACK_PAGE) return 1;
    return 0;
}

typedef struct { uint32_t guest; uint8_t *host; size_t size; } Window;
static Window windows[8];
static int n_windows;

void p4_guest_window(uint32_t guest, void *host, size_t size)
{
    if (n_windows < 8 && host && size) windows[n_windows++] = (Window){guest, (uint8_t *)host, size};
}

void *p4_g2h_slow(uint32_t a)
{
    if (!a) return NULL;
    for (int i = 0; i < n_windows; i++)
        if (a - windows[i].guest < windows[i].size) return windows[i].host + (a - windows[i].guest);
    /* Not in the block or a window: a sentinel, or a value that is not an address (the stale word a loop reads and never uses). The
     * block is followed by inaccessible address space up to 4 GB, so dereferencing it faults without touching any other mapping. */
    return (void *)(p4_guest_base + a);
}

uint32_t p4_h2g_slow(const void *p)
{
    if (!p) return 0;
    /* a "pointer" below the first page is an offset: address arithmetic on a null pointer at run time. The decomp's offsetof
     * (`&((T *)0)->member`, at any offset) never comes here: the rewriter converts it as an integer (docs/port64.md, the null-offset rule) */
    if ((uintptr_t)p < 0x10000) return (uint32_t)(uintptr_t)p;
    for (int i = 0; i < n_windows; i++) {
        uintptr_t q = (uintptr_t)p - (uintptr_t)windows[i].host;
        if (q < windows[i].size) return windows[i].guest + (uint32_t)q;
    }
    uintptr_t d = (uintptr_t)p - (uintptr_t)p4_guest_base;
    if (d == 0) return 0;
    if (d >= P4_GUEST_SENTINEL && d <= 0xFFFFFFFFu) return (uint32_t)d;
    /* Up to 4 GB past the reserved range: pointer arithmetic that the 32-bit original did modulo 2^32 (`frames + (u32)offset`, with an
     * offset the file stores as a negative number): the address it meant is the sum modulo 2^32. */
    if ((int64_t)d > -0x100000000ll && (int64_t)d < 0x200000000ll) return (uint32_t)d;      /* (and below the block: `base + (negative)`) */
#ifdef P4_GUEST_CHECK
    if (!p4_guest_check_off)
        p4_guest_fault("host pointer outside guest memory and every window stored as a guest address", (uint64_t)(uintptr_t)p);
#endif
    return (uint32_t)d;
}

#ifdef _WIN32
#include <windows.h>
#endif

void p4_guest_fault(const char *what, uint64_t value)
{
    fprintf(stderr, "p4 guest fault: %s (0x%llx)\n", what, (unsigned long long)value);
#ifdef _WIN32
    {   /* the call stack as offsets into the exe (llvm-symbolizer --relative-address): there is no debugger on the machines this runs on */
        void *frames[32];
        USHORT n = CaptureStackBackTrace(1, 32, frames, NULL);
        HMODULE exe = GetModuleHandleW(NULL);
        for (USHORT i = 0; i < n; i++)
            fprintf(stderr, "p4 guest fault:   frame %u: exe+0x%llx\n", (unsigned)i, (unsigned long long)((uintptr_t)frames[i] - (uintptr_t)exe));
    }
#endif
    abort();
}

/* ---- function table: two open-addressing maps, retail address -> host function and back --------------- */

#define FN_TAG 0xDEAD000000000000ull
#define HANDLE_BASE 0x08000000u

typedef struct { uint32_t addr; void *fn; uint32_t sig; } FnEnt;

static FnEnt *fwd, *rev;       /* fwd keyed by addr (0 = empty), rev keyed by fn (NULL = empty) */
static size_t cap, used;
static uint32_t next_handle = HANDLE_BASE;

static size_t hash_u32(uint32_t a) { return (size_t)((a * 2654435761u) >> 7); }
static size_t hash_ptr(const void *p) { uint64_t v = (uint64_t)(uintptr_t)p; return (size_t)((v * 0x9E3779B97F4A7C15ull) >> 40); }

static void table_put(uint32_t addr, void *fn, uint32_t sig)
{
    size_t i = hash_u32(addr) & (cap - 1);
    while (fwd[i].addr && fwd[i].addr != addr) i = (i + 1) & (cap - 1);
    if (!fwd[i].addr) used++;
    fwd[i].addr = addr;
    fwd[i].fn = fn;
    if (sig || !fwd[i].sig) fwd[i].sig = sig;
    i = hash_ptr(fn) & (cap - 1);
    while (rev[i].fn && rev[i].fn != fn) i = (i + 1) & (cap - 1);
    rev[i].fn = fn;
    rev[i].addr = addr;
}

static void fn_register(uint32_t addr, void *fn, uint32_t sig)
{
    if (!addr || !fn) return;
    if (!cap || used * 2 >= cap) {
        FnEnt *of = fwd;
        size_t ocap = cap;
        cap = cap ? cap * 2 : 1024;
        fwd = calloc(cap, sizeof *fwd);
        free(rev);
        rev = calloc(cap, sizeof *rev);
        used = 0;
        for (size_t i = 0; i < ocap; i++)
            if (of[i].addr) table_put(of[i].addr, of[i].fn, of[i].sig);
        free(of);
    }
    table_put(addr, fn, sig);
}

void p4_fn_register(uint32_t addr, void *fn) { fn_register(addr, fn, 0); }

/* "pii" -> P4_SIG_KNOWN | 2 << 8 | 1 | ...; 0 (unknown) for a letter other than p, i, v or more than 6 parameters */
static uint32_t sig_parse(const char *s)
{
    if (!s || !s[0]) return 0;
    size_t n = strlen(s) - 1;
    if (n > 6) return 0;
    uint32_t v = P4_SIG_KNOWN | (uint32_t)n << 8;
    for (size_t i = 0; s[i]; i++) {
        if (s[i] != 'p' && s[i] != 'f' && s[i] != 'i' && s[i] != 'v' && s[i] != 'x') return 0;
        if (s[i] == 'p' || s[i] == 'f') v |= 1u << i;
        if (s[i] == 'f') v |= 1u << (11 + i);
        if (s[i] == 'x') v |= 1u << (i == 0 ? 24 : 17 + i);          /* a float or double: xmm0 result / the next xmm parameter */
    }
    return v;
}

void p4_fn_register_sig(uint32_t addr, void *fn, const char *sig) { fn_register(addr, fn, sig_parse(sig)); }

static const FnEnt *fwd_find(uint32_t a)
{
    if (!cap) return NULL;
    size_t i = hash_u32(a) & (cap - 1);
    while (fwd[i].addr) {
        if (fwd[i].addr == a) return &fwd[i];
        i = (i + 1) & (cap - 1);
    }
    return NULL;
}

/* Other names (thunks) of registered functions, searched only when the reverse map misses. */
static struct { void *fn; uint32_t addr; } *aliasv;
static size_t n_aliasv, cap_aliasv;

void p4_fn_alias(uint32_t addr, void *fn)
{
    if (!addr || !fn || !fwd_find(addr)) return;
    if (n_aliasv == cap_aliasv) {
        cap_aliasv = cap_aliasv ? cap_aliasv * 2 : 256;
        aliasv = realloc(aliasv, cap_aliasv * sizeof *aliasv);
    }
    aliasv[n_aliasv].fn = fn;
    aliasv[n_aliasv++].addr = addr;
}

static uint32_t alias_addr(const void *f)
{
    for (size_t i = 0; i < n_aliasv; i++)
        if (aliasv[i].fn == f) return aliasv[i].addr;
    return 0;
}

/* The function and register signature a guest code address stands for. A function of the executable that has no retail address is
 * reached through its address in the image window (guest.h): the same address p4_h2g gives it, so a function pointer that went
 * through a data-pointer conversion (a `void *` parameter, a cast through `void *`) still comes back as the function; if that
 * function is a registered one (a thunk or native for a retail address) it is found by its host address and brings its signature. */
static int fn_find(uint32_t a, void **fn, uint32_t *sig)
{
    const FnEnt *e = fwd_find(a);
    if (e) {
        *fn = e->fn;
        *sig = e->sig;
        return 1;
    }
    if (p4_guest_image && a - P4_GUEST_IMAGE < p4_guest_image_size) {
        void *h = p4_guest_image + (a - P4_GUEST_IMAGE);
        if (cap) {
            size_t i = hash_ptr(h) & (cap - 1);
            while (rev[i].fn) {
                if (rev[i].fn == h) {
                    e = fwd_find(rev[i].addr);
                    if (e) {
                        *fn = e->fn;
                        *sig = e->sig;
                        return 1;
                    }
                    break;
                }
                i = (i + 1) & (cap - 1);
            }
        }
        uint32_t aa = alias_addr(h);
        if (aa && (e = fwd_find(aa))) {
            *fn = e->fn;
            *sig = e->sig;
            return 1;
        }
        *fn = h;
        *sig = 0;
        return 1;
    }
    return 0;
}

int p4_fn_known(uint32_t a)
{
    return a != 0 && fwd_find(a) != NULL;
}

void *p4_fn_g2h(uint32_t a)
{
    void *fn;
    uint32_t sig;
    if (!a) return NULL;
    return fn_find(a, &fn, &sig) ? fn : (void *)(uintptr_t)(FN_TAG | a);
}

static void *adapter_target(const void *f);       /* the function behind an adapter, or NULL (below, with the pool) */

uint32_t p4_fn_h2g(const void *f)
{
    if (!f) return 0;
    uint64_t v = (uint64_t)(uintptr_t)f;
    if ((v & 0xFFFF000000000000ull) == FN_TAG) return (uint32_t)v;
    if (cap) {
        size_t i = hash_ptr(f) & (cap - 1);
        while (rev[i].fn) {
            if (rev[i].fn == f) return rev[i].addr;
            i = (i + 1) & (cap - 1);
        }
    }
    uint32_t aa = alias_addr(f);
    if (aa) return aa;
    /* an adapter that escaped (a callback passed on to a function that stores it): the word stored is the function behind it */
    void *behind = adapter_target(f);
    if (behind) return p4_fn_h2g(behind);
    if ((uintptr_t)f - (uintptr_t)p4_guest_image < p4_guest_image_size && p4_guest_image)
        return P4_GUEST_IMAGE + (uint32_t)((const uint8_t *)f - p4_guest_image);
    uint32_t h = next_handle;
    next_handle += 4;
    p4_fn_register(h, (void *)f);
    return h;
}

uint32_t p4_fn_h2g_sig(const void *f, const char *sig)
{
    uint32_t a = p4_fn_h2g(f);
    if (a) fn_register(a, (void *)f, sig_parse(sig));
    return a;
}

/* ---- indirect calls between classes: adapters ----------------------------------------------------------------------------------
 * A pool of 1024 fixed entry points; each is bound to one (function, call-site signature) pair on first use and, when called with
 * up to six integer-register arguments, converts the pointer/integer pairs the two signatures disagree on, calls the function and
 * converts the result. A float or double parameter travels in an xmm register and takes no integer register: the adapter takes the six integer
 * registers and the eight xmm registers (declared as doubles, so they are handed on bit for bit), counts the integer registers over the
 * non-float parameters, and returns {rax, xmm0} so that an integer or a float result passes through. Stack arguments are not handled. */
typedef struct { void *fn; uint32_t declared, target; } Adapter;
#define P4_MAX_ADAPTERS 1024    /* four banks of sixteen rows of sixteen stubs below */
static Adapter adapters[P4_MAX_ADAPTERS];
static int n_adapters;

#if !defined(_WIN64)
typedef struct { uintptr_t r; double d; } AdRet;     /* returned in rax and xmm0 */
typedef AdRet (*Raw6)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, double, double, double, double, double, double, double, double);

static AdRet adapted(const Adapter *s, uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5,
                     double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    uintptr_t a[6] = {a0, a1, a2, a3, a4, a5};
    int ir = 0;                                     /* the next integer register: floats do not use one */
    for (int i = 0; i < 6; i++) {
        if (((s->declared | s->target) >> (18 + i)) & 1) continue;
        uint32_t d = (s->declared >> (1 + i)) & 1, t = (s->target >> (1 + i)) & 1;
        if (d && !t) a[ir] = ((s->declared >> (12 + i)) & 1) ? p4_fn_h2g((const void *)a[ir]) : p4_h2g((const void *)a[ir]);
        else if (!d && t) a[ir] = ((s->target >> (12 + i)) & 1) ? (uintptr_t)p4_fn_g2h((uint32_t)a[ir]) : (uintptr_t)p4_g2h((uint32_t)a[ir]);
        ir++;
    }
    AdRet r = ((Raw6)s->fn)(a[0], a[1], a[2], a[3], a[4], a[5], x0, x1, x2, x3, x4, x5, x6, x7);
    uint32_t d = s->declared & 1, t = s->target & 1;
    if (t && !d) r.r = p4_h2g((const void *)r.r);
    else if (!t && d) r.r = (uintptr_t)p4_g2h((uint32_t)r.r);
    return r;
}

#define AD(k) static AdRet ad##k(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d, uintptr_t e, uintptr_t f, \
                                 double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7) \
    { return adapted(&adapters[k], a, b, c, d, e, f, x0, x1, x2, x3, x4, x5, x6, x7); }
#define H16(x) 0x##x##0, 0x##x##1, 0x##x##2, 0x##x##3, 0x##x##4, 0x##x##5, 0x##x##6, 0x##x##7, \
               0x##x##8, 0x##x##9, 0x##x##A, 0x##x##B, 0x##x##C, 0x##x##D, 0x##x##E, 0x##x##F
#define ROW(b, x) AD(0x##b##x##0) AD(0x##b##x##1) AD(0x##b##x##2) AD(0x##b##x##3) AD(0x##b##x##4) AD(0x##b##x##5) AD(0x##b##x##6) AD(0x##b##x##7) \
                  AD(0x##b##x##8) AD(0x##b##x##9) AD(0x##b##x##A) AD(0x##b##x##B) AD(0x##b##x##C) AD(0x##b##x##D) AD(0x##b##x##E) AD(0x##b##x##F)
#define TROW(b, x) ad0x##b##x##0, ad0x##b##x##1, ad0x##b##x##2, ad0x##b##x##3, ad0x##b##x##4, ad0x##b##x##5, ad0x##b##x##6, ad0x##b##x##7, \
                   ad0x##b##x##8, ad0x##b##x##9, ad0x##b##x##A, ad0x##b##x##B, ad0x##b##x##C, ad0x##b##x##D, ad0x##b##x##E, ad0x##b##x##F,
#define BANK(b) ROW(b, 0) ROW(b, 1) ROW(b, 2) ROW(b, 3) ROW(b, 4) ROW(b, 5) ROW(b, 6) ROW(b, 7) ROW(b, 8) ROW(b, 9) ROW(b, A) ROW(b, B) ROW(b, C) ROW(b, D) ROW(b, E) ROW(b, F)
#define TBANK(b) TROW(b, 0) TROW(b, 1) TROW(b, 2) TROW(b, 3) TROW(b, 4) TROW(b, 5) TROW(b, 6) TROW(b, 7) TROW(b, 8) TROW(b, 9) TROW(b, A) TROW(b, B) TROW(b, C) TROW(b, D) TROW(b, E) TROW(b, F)
BANK(0) BANK(1) BANK(2) BANK(3)
typedef AdRet (*AdFn)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, double, double, double, double, double, double, double, double);
static const AdFn adapter_fns[P4_MAX_ADAPTERS] = {
    TBANK(0) TBANK(1) TBANK(2) TBANK(3)
};
#undef AD
#undef H16
#undef ROW
#undef TROW
#undef BANK
#undef TBANK

#else
/* Windows x64: the same adapters, by position. A parameter is passed in rcx/rdx/r8/r9 or, when it is a float or double, in xmm0..xmm3 of its own position
 * (the register counters are not separate as in System V), and the fifth and sixth are on the stack above 32 bytes of shadow space. Each entry
 * point is a stub in assembly that stores the four integer and four xmm registers and the address of the stack arguments, calls adapted_w64 (C) and
 * returns {rax, xmm0}; the target is called by p4_call_w64, which loads the registers back. */
typedef struct { uint64_t i[4]; uint64_t x[4]; } W64Regs;      /* 64 bytes: the layout the stub and p4_call_w64 use */

uint64_t p4_call_w64(void *fn, const W64Regs *regs, const uint64_t *stack, uint64_t *xmm0);
__asm__(".text\n"
        ".globl p4_call_w64\n"
        ".def p4_call_w64; .scl 2; .type 32; .endef\n"
        "p4_call_w64:\n"
        "\tpushq %rbp\n\tmovq %rsp, %rbp\n\tpushq %rbx\n\tsubq $0x38, %rsp\n"
        "\tmovq %r9, %rbx\n\tmovq %rcx, %rax\n"
        "\tmovq 0(%r8), %r10\n\tmovq %r10, 0x20(%rsp)\n\tmovq 8(%r8), %r10\n\tmovq %r10, 0x28(%rsp)\n"
        "\tmovsd 32(%rdx), %xmm0\n\tmovsd 40(%rdx), %xmm1\n\tmovsd 48(%rdx), %xmm2\n\tmovsd 56(%rdx), %xmm3\n"
        "\tmovq 24(%rdx), %r9\n\tmovq 16(%rdx), %r8\n\tmovq 0(%rdx), %rcx\n\tmovq 8(%rdx), %rdx\n"
        "\tcallq *%rax\n"
        "\tmovsd %xmm0, (%rbx)\n"
        "\taddq $0x38, %rsp\n\tpopq %rbx\n\tpopq %rbp\n\tretq\n");

__attribute__((used)) static uint64_t adapted_w64(unsigned idx, const W64Regs *r, const uint64_t *stack, uint64_t *xmm0)
{
    const Adapter *s = &adapters[idx];
    uint64_t v[6];
    for (int k = 0; k < 6; k++) {
        int is_float = (((s->declared | s->target) >> (18 + k)) & 1) != 0;
        v[k] = k < 4 ? (is_float ? r->x[k] : r->i[k]) : stack[k - 4];
        if (is_float) continue;
        uint32_t d = (s->declared >> (1 + k)) & 1, t = (s->target >> (1 + k)) & 1;
        if (d && !t) v[k] = ((s->declared >> (12 + k)) & 1) ? p4_fn_h2g((const void *)v[k]) : p4_h2g((const void *)v[k]);
        else if (!d && t) v[k] = ((s->target >> (12 + k)) & 1) ? (uint64_t)(uintptr_t)p4_fn_g2h((uint32_t)v[k]) : (uint64_t)(uintptr_t)p4_g2h((uint32_t)v[k]);
    }
    W64Regs out;
    for (int k = 0; k < 4; k++) out.i[k] = out.x[k] = v[k];
    uint64_t os[2] = { v[4], v[5] };
    uint64_t res = p4_call_w64(s->fn, &out, os, xmm0);
    uint32_t d = s->declared & 1, t = s->target & 1;
    if (t && !d) res = p4_h2g((const void *)res);
    else if (!t && d) res = (uint64_t)(uintptr_t)p4_g2h((uint32_t)res);
    return res;
}

__asm__(".text\n"
        ".def p4_ad_common; .scl 3; .type 32; .endef\n"
        "p4_ad_common:\n"
        "\tpushq %rbp\n\tmovq %rsp, %rbp\n\tsubq $0x70, %rsp\n"
        "\tmovq %rcx, 0x20(%rsp)\n\tmovq %rdx, 0x28(%rsp)\n\tmovq %r8, 0x30(%rsp)\n\tmovq %r9, 0x38(%rsp)\n"
        "\tmovsd %xmm0, 0x40(%rsp)\n\tmovsd %xmm1, 0x48(%rsp)\n\tmovsd %xmm2, 0x50(%rsp)\n\tmovsd %xmm3, 0x58(%rsp)\n"
        "\tmovl %r10d, %ecx\n"                   /* the adapter index */
        "\tleaq 0x20(%rsp), %rdx\n"              /* the saved registers */
        "\tleaq 0x30(%rbp), %r8\n"               /* the stack arguments: above the return address and the 32 bytes of shadow space */
        "\tleaq 0x60(%rsp), %r9\n"               /* where the result's xmm0 goes */
        "\tcallq adapted_w64\n"
        "\tmovsd 0x60(%rsp), %xmm0\n"
        "\tmovq %rbp, %rsp\n\tpopq %rbp\n\tretq\n");
#define AD(k) extern void ad##k(void); \
    __asm__(".text\n.def ad" #k "; .scl 3; .type 32; .endef\nad" #k ":\n\tmovl $" #k ", %r10d\n\tjmp p4_ad_common\n");
#define H16(x) 0
#define ROW(b, x) AD(0x##b##x##0) AD(0x##b##x##1) AD(0x##b##x##2) AD(0x##b##x##3) AD(0x##b##x##4) AD(0x##b##x##5) AD(0x##b##x##6) AD(0x##b##x##7) \
                  AD(0x##b##x##8) AD(0x##b##x##9) AD(0x##b##x##A) AD(0x##b##x##B) AD(0x##b##x##C) AD(0x##b##x##D) AD(0x##b##x##E) AD(0x##b##x##F)
#define TROW(b, x) ad0x##b##x##0, ad0x##b##x##1, ad0x##b##x##2, ad0x##b##x##3, ad0x##b##x##4, ad0x##b##x##5, ad0x##b##x##6, ad0x##b##x##7, \
                   ad0x##b##x##8, ad0x##b##x##9, ad0x##b##x##A, ad0x##b##x##B, ad0x##b##x##C, ad0x##b##x##D, ad0x##b##x##E, ad0x##b##x##F,
#define BANK(b) ROW(b, 0) ROW(b, 1) ROW(b, 2) ROW(b, 3) ROW(b, 4) ROW(b, 5) ROW(b, 6) ROW(b, 7) ROW(b, 8) ROW(b, 9) ROW(b, A) ROW(b, B) ROW(b, C) ROW(b, D) ROW(b, E) ROW(b, F)
#define TBANK(b) TROW(b, 0) TROW(b, 1) TROW(b, 2) TROW(b, 3) TROW(b, 4) TROW(b, 5) TROW(b, 6) TROW(b, 7) TROW(b, 8) TROW(b, 9) TROW(b, A) TROW(b, B) TROW(b, C) TROW(b, D) TROW(b, E) TROW(b, F)
BANK(0) BANK(1) BANK(2) BANK(3)
typedef void (*AdFn)(void);
static const AdFn adapter_fns[P4_MAX_ADAPTERS] = {
    TBANK(0) TBANK(1) TBANK(2) TBANK(3)
};
#undef AD
#undef H16
#undef ROW
#undef TROW
#undef BANK
#undef TBANK
#endif

/* Two open-addressing maps (indices + 1, half full at most) keep indirect calls cheap: every call through a function word runs fn_for_sig, which a linear scan of
 * the pool made the biggest single cost of the 64-bit game (20 percent of a New Game run): the adapter entry points (is `f` one?) and the (function, call-site
 * signature) pairs already bound. */
#define AD_HASH 4096
static int16_t ad_by_entry[AD_HASH], ad_by_pair[AD_HASH];
static int ad_entries_ready;

static size_t ad_hash(const void *p, uint32_t declared)
{
    uint64_t v = ((uint64_t)(uintptr_t)p * 0x9E3779B97F4A7C15ull) ^ ((uint64_t)declared * 0xC2B2AE3D27D4EB4Full);
    return (size_t)(v >> 40) & (AD_HASH - 1);
}

/* the pool index of the adapter entry point `f`, or -1 */
static int adapter_index(const void *f)
{
    if (!ad_entries_ready) {
        for (int k = 0; k < P4_MAX_ADAPTERS; k++) {
            size_t i = ad_hash((const void *)adapter_fns[k], 0);
            while (ad_by_entry[i]) i = (i + 1) & (AD_HASH - 1);
            ad_by_entry[i] = (int16_t)(k + 1);
        }
        ad_entries_ready = 1;
    }
    for (size_t i = ad_hash(f, 0); ad_by_entry[i]; i = (i + 1) & (AD_HASH - 1))
        if ((const void *)adapter_fns[ad_by_entry[i] - 1] == f) return ad_by_entry[i] - 1;
    return -1;
}

static void *adapter_target(const void *f)
{
    int k = adapter_index(f);
    return k < 0 ? NULL : adapters[k].fn;
}

/* `fn` as a function that is called through `declared` while it takes `target` (both numeric signatures): fn itself when the
 * classes agree, else its adapter. */
static void *fn_for_sig(void *fn, uint32_t target, uint32_t declared)
{
    /* an adapter is never adapted again (a callback handed on twice): the function behind it is, by the signature it was registered with */
    int ak = adapter_index(fn);
    if (ak >= 0) {
        if (adapters[ak].declared == declared) return fn;
        fn = adapters[ak].fn;
        target = adapters[ak].target;
    }
    if (!target || !declared) return fn;
    uint32_t nd = (declared >> 8) & 7, nt = (target >> 8) & 7, m = nd < nt ? nd : nt;
    if (((declared ^ target) & ((2u << m) - 1)) == 0) return fn;
    size_t i = ad_hash(fn, declared);
    for (; ad_by_pair[i]; i = (i + 1) & (AD_HASH - 1)) {
        int k = ad_by_pair[i] - 1;
        if (adapters[k].fn == fn && adapters[k].declared == declared) return (void *)adapter_fns[k];
    }
    if (n_adapters == P4_MAX_ADAPTERS) {
        fprintf(stderr, "p4: more than %d distinct indirect call adaptations\n", P4_MAX_ADAPTERS);
        abort();
    }
    if (getenv("P4_FN_TRACE"))       /* debugging a mismatch: which function is called through which pair of signatures (gdb: info symbol) */
        fprintf(stderr, "p4: indirect-call adapter #%d for %p: call site 0x%08x, function 0x%08x\n", n_adapters, fn, declared, target);
    adapters[n_adapters] = (Adapter){fn, declared, target};
    ad_by_pair[i] = (int16_t)(n_adapters + 1);          /* i is the empty slot the lookup ended on */
    return (void *)adapter_fns[n_adapters++];
}

void *p4_fn_g2h_sig(uint32_t a, uint32_t declared)
{
    void *fn;
    uint32_t target;
    if (!a) return NULL;
    if (!fn_find(a, &fn, &target)) return (void *)(uintptr_t)(FN_TAG | a);
    return fn_for_sig(fn, target, declared);
}

void *p4_fn_adapt(const void *fn, const char *own, const char *declared)
{
    return fn_for_sig((void *)fn, sig_parse(own), sig_parse(declared));
}

#endif /* P4_GUEST64 */
