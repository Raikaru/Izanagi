/* Guest address model (docs/port64.md).
 *
 * A guest address is a u32: what a PS2 pointer is in the decomp, in the retail image and in every
 * struct the game keeps in memory. A host pointer into guest memory is derived from it:
 *
 *   i386 build        identity: the PS2 RAM image sits at its retail addresses, so a guest address
 *                     IS the host pointer (P4_G2H / P4_H2G are casts, P4_FN_* the same).
 *   64-bit build      guest RAM is one block at an arbitrary host address (p4_guest_base):
 *                     host = base + guest, guest 0 is NULL. Guest code addresses are not host
 *                     addresses: P4_FN_G2H looks the retail address up in the function table
 *                     (src/platform/guest.c), P4_FN_H2G is the reverse.
 *
 * Rules for port code (CONTRIBUTING.md): never assume sizeof(void *) == 4; a seam that takes guest memory
 * takes a u32 guest address and converts it here; never store a host pointer into guest memory (the
 * 64-bit build cannot represent it: P4_H2G faults in a P4_GUEST_CHECK build); keep host-only objects
 * (SDL, GL, FFmpeg, the host heap) behind handles in guest memory.
 *
 * Included through p4pc/compat.h, so every decomp unit and port file sees it. */
#ifndef P4PC_GUEST_H
#define P4PC_GUEST_H

#include <stddef.h>
#include <stdint.h>

#if __SIZEOF_POINTER__ == 8
#define P4_GUEST64 1
#endif

/* Guest RAM: 32 MB of PS2 RAM (retail addresses 0..0x01FFFFFF) and 32 MB the port owns above it
 * (data that has no retail address, string pool, handles). */
#define P4_GUEST_SIZE 0x04000000u
/* Sentinels: the decomp compares pointers with (T *)-1. G2H maps 0xFFFFFF00..0xFFFFFFFF to a pointer that is
 * never valid and H2G maps it back, so the comparison and a stored sentinel round-trip. */
#define P4_GUEST_SENTINEL 0xFFFFFF00u

#ifdef P4_GUEST64

extern uint8_t *p4_guest_base;

/* Reserves and zero-fills the block (mmap / VirtualAlloc). Returns 0, or -1. Idempotent. */
int p4_guest_init(void);

void p4_guest_fault(const char *what, uint64_t value);   /* P4_GUEST_CHECK builds: reports and aborts */
extern int p4_guest_check_off;                           /* nonzero: P4_H2G of a wild host pointer is not a fault (ovdiff's random words) */

/* Address windows: ranges of guest address space outside the block that map to host memory. The executable's own image (its code,
 * .rodata, .data and .bss) is one, at P4_GUEST_IMAGE: a string literal, a port-owned static or a function can be stored into guest
 * memory without a copy, and `static T *x = &y;` needs only a run-time initialiser. The hardware windows the game reaches
 * by absolute address are the others (the EE register page at 0x10000000, the scratchpad at 0x70000000), backed by host
 * memory p4_guest_init allocates. Nothing else outside the block can be stored or translated. */
#define P4_GUEST_IMAGE 0x04000000u
#define P4_GUEST_EEREGS 0x10000000u
#define P4_GUEST_EEREGS_SIZE 0x10000u
#define P4_GUEST_SPR 0x70000000u
#define P4_GUEST_SPR_SIZE 0x4000u
extern uint8_t *p4_guest_image;
extern size_t p4_guest_image_size;

/* Stacks live in guest memory (docs/port64.md 2.7): the address of a local is then a valid guest address. The port half's range
 * [P4_GUEST_STACKS, +P4_GUEST_STACKS_SIZE) is the stack arena: p4_guest_stack_alloc returns the host pointer of `size` bytes in it
 * (16-byte aligned, first fit; NULL when exhausted) and p4_guest_stack_free gives them back. */
#define P4_GUEST_STACKS 0x02000000u
#define P4_GUEST_STACKS_SIZE 0x01000000u
void *p4_guest_stack_alloc(size_t size);
void p4_guest_stack_free(void *p);

/* Adds a window (guest base, host memory, size). p4_guest_init adds the three above; at most 8 windows. */
void p4_guest_window(uint32_t guest, void *host, size_t size);

void *p4_g2h_slow(uint32_t a);
uint32_t p4_h2g_slow(const void *p);

/* Runs `fn` once the block exists (at once if it does): the initialisers of statics that hold guest addresses. The
 * rewriter registers one per unit from a constructor. */
void p4_guest_on_init(void (*fn)(void));

static inline void *p4_g2h(uint32_t a)
{
    if (a - 1u < P4_GUEST_SIZE - 1u) return (void *)(p4_guest_base + a);
    return p4_g2h_slow(a);
}

/* A guest value that is an offset or computed by arithmetic (m2c `(u8 *)(i * 4)`, later added to a base address): 0 is an ordinary value here,
 * the block's base, not NULL. The rewriter uses it for an integer-to-pointer cast of an arithmetic expression. */
static inline void *p4_g2h_off(uint32_t a)
{
    return a ? p4_g2h(a) : (void *)p4_guest_base;
}

static inline uint32_t p4_h2g(const void *p)
{
    uintptr_t d = (uintptr_t)p - (uintptr_t)p4_guest_base;
    if (d - 1u < P4_GUEST_SIZE - 1u) return (uint32_t)d;
    return p4_h2g_slow(p);
}

/* Function table. Retail code addresses map to the native function (or trap stub) that stands for
 * them; a function the port defines that has no retail address gets a handle in 0x08000000.. when it is
 * first stored (p4_fn_h2g). A value that is not an address in the table (-1, 1, a retail address
 * with no native) comes back as a tagged non-pointer that never compares equal to a function and
 * round-trips through p4_fn_h2g. */
void p4_fn_register(uint32_t addr, void *fn);
void *p4_fn_g2h(uint32_t a);
uint32_t p4_fn_h2g(const void *f);
int p4_fn_known(uint32_t a);   /* a is a code address the function table has an entry for (a retail address or a handle) */

/* Register ABI of a function reached through a guest code address. A direct call is adapted at compile time to the callee's
 * definition (docs/port64.md 2.6); an indirect call cannot be, because the callee is a run-time value and the same table slot is
 * called by m2c code that declares its pointers as integers and by typed code that declares them as pointers. So each table entry
 * records the register classes of its function and each indirect call site passes its own:
 *   a signature string: the result's class, then one letter per parameter: 'p' a host pointer, 'i' an integer or guest address, 'x' a float or
 *   double ("pii": returns a pointer, takes two integers; "vpx": void, a pointer and a float). A function the table has no signature for (traps,
 *   structs, more than six parameters) is called as it is.
 *   p4_fn_register_sig / p4_fn_h2g_sig    register a function with its signature (the h2g form returns its guest address)
 *   p4_fn_g2h_sig(addr, declared)         the function to call for a call site whose own signature is `declared` (the numeric form
 *                                         the rewriter emits, P4_SIG below): the function itself when the classes agree, else an
 *                                         adapter that converts each pointer/integer pair and the result
 * Port code that stores one of its own functions into guest memory uses p4_fn_h2g_sig, or the callers that declare integers get
 * truncated host pointers. */
void p4_fn_alias(uint32_t addr, void *fn);   /* `fn` is another name (a thunk) for the function registered at addr: p4_fn_h2g(fn) is addr */
void p4_fn_register_sig(uint32_t addr, void *fn, const char *sig);
uint32_t p4_fn_h2g_sig(const void *f, const char *sig);
void *p4_fn_g2h_sig(uint32_t a, uint32_t declared);
/* A function whose own signature is `own` converted to a function-pointer type whose calls use `declared` (signature strings): the
 * rewriter wraps a named function passed or cast to a function-pointer type of another register ABI (a callback m2c defines with
 * integers and passes to RenderWare, which calls it with pointers). */
void *p4_fn_adapt(const void *fn, const char *own, const char *declared);
/* The numeric form: P4_SIG_KNOWN | n << 8 | (result is a pointer) | (parameter i is a pointer) << (1 + i) | (that pointer is a function
 * pointer, so it converts with p4_fn_*) << (12 + i) | (parameter i is a float or double) << (18 + i) | (the result is a float or double) << 24;
 * at most 6 parameters. In a string 'f' is a function pointer and 'x' a float or double. A float travels in an xmm register, so it does not take
 * an integer register: the adapters (guest.c) count the integer registers over the non-float parameters and hand the xmm registers on unchanged. */
#define P4_SIG_KNOWN 0x80000000u
/* The constant for a call site written by hand: P4_SIG(n, r, a0, a1, a2, a3, a4, a5) with n the number of parameters, r and each
 * a_i 1 for a host pointer (P4_SIGF: a function pointer) and 0 for an integer or guest address; the unused ones 0.
 *   ((int32_t (*)(void *, void *, void *, int32_t))p4_fn_g2h_sig(fn, P4_SIG(4, 0, 1, 1, 1, 0, 0, 0)))(voice, cfg, work, size) */
#define P4_SIG(n, r, a0, a1, a2, a3, a4, a5) \
    (P4_SIG_KNOWN | (uint32_t)(n) << 8 | (r) | (a0) << 1 | (a1) << 2 | (a2) << 3 | (a3) << 4 | (a4) << 5 | (a5) << 6)
#define P4_SIGF(i) (1u << (12 + (i)))   /* parameter i is a function pointer (or'ed into a signature) */
#define P4_SIGX(i) (1u << (18 + (i)))   /* parameter i is a float or double (or'ed into a signature) */
#define P4_SIGXR (1u << 24)             /* the result is a float or double */

#define P4_G2H(a) p4_g2h((uint32_t)(a))
#define P4_H2G(p) p4_h2g((const void *)(p))
#define P4_FN_G2H(a) p4_fn_g2h((uint32_t)(a))
#define P4_FN_H2G(f) p4_fn_h2g((const void *)(f))

#else /* 32-bit host: identity */

#define P4_G2H(a) ((void *)(uintptr_t)(uint32_t)(a))
#define P4_H2G(p) ((uint32_t)(uintptr_t)(p))
#define P4_FN_G2H(a) P4_G2H(a)
#define P4_FN_H2G(f) P4_H2G(f)
#define p4_fn_register(addr, fn) ((void)(addr), (void)(fn))
#define p4_fn_register_sig(addr, fn, sig) ((void)(addr), (void)(fn), (void)(sig))
#define p4_fn_alias(addr, fn) ((void)(addr), (void)(fn))
#define p4_fn_h2g_sig(f, sig) ((void)(sig), P4_FN_H2G(f))
#define p4_fn_g2h_sig(a, declared) ((void)(declared), P4_FN_G2H(a))
#define p4_fn_adapt(fn, own, declared) ((void *)(fn))
#define P4_SIG(n, r, a0, a1, a2, a3, a4, a5) 0u
#define P4_SIGF(i) 0u
#define P4_SIGX(i) 0u
#define P4_SIGXR 0u

/* A stack is host memory here: the host address is the guest address (src/platform/guest.c). */
void *p4_guest_stack_alloc(size_t size);
void p4_guest_stack_free(void *p);

#endif

/* Persistent managed RAM, not transient/guardable coroutine stacks, host
 * heap/static storage, image or hardware windows. Allocation owners still
 * invalidate metadata on free; membership alone does not prove lifetime.
 * Unlike H2G this predicate is safe for an arbitrary native input pointer. */
int p4_guest_asset_buffer_contains(const void *pointer, size_t size);

#endif /* P4PC_GUEST_H */
