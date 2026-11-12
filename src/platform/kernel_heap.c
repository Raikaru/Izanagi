/* The guest heap uses the retail EE newlib allocator in ee_heap.c. Chunk
 * headers, bins, alignment, splitting, coalescing and reuse use guest words,
 * never host pointer-sized dlmalloc metadata. RW retains its matched chkmem
 * wrapper and original checked-header/backpointer layout above that heap.
 *
 * overrides/00100230.c supplies the mapped EE arena. Only the sbrk syscall
 * boundary is modeled here; all allocator decisions are the original code. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/os.h"
#include "p4pc/gmem.h"
#include "p4pc/platform.h"
#include "p4pc/ps2_libc.h"
#include "ee_heap.h"

static uint32_t heap_base, heap_end;
static P4HeapObserver heap_observer;
int p4pc_heap_observer(P4HeapObserver observer)
{
    if (observer && heap_observer && observer!=heap_observer) return -1;
    heap_observer=observer;
    return 0;
}

int32_t p4pc_heap_init(void *base, size_t capacity) {
    if (heap_end != 0) return -1;
    heap_base = P4_H2G(base);
    heap_end = heap_base + (uint32_t)capacity;
    P4_G32(0x0070c5d4u) = heap_base;
    /* Original av_[0..257] initializer: 128 self-linked bins and top. */
    P4_G32(0x00710080u) = P4_G32(0x00710084u) = 0;
    for (uint32_t i = 0; i < 128; ++i) {
        uint32_t bin = 0x00710080u + 8u * i;
        P4_G32(bin + 8u) = P4_G32(bin + 12u) = bin;
    }
    memset(P4_G2H(0x00710488u), 0, 0x58);
    P4_G32(0x00710488u) = 0x20000u; /* trim threshold */
    P4_G32(0x00710498u) = UINT32_MAX; /* no sbrk region yet */
    return 0;
}

int p4pc_heap_guest_span(uint32_t *begin, uint32_t *end) {
    if (begin == NULL || end == NULL || heap_base == 0 || heap_end <= heap_base)
        return -1;
    *begin = heap_base;
    *end = heap_end;
    return 0;
}

static uint32_t heap_reent(void) {
    if (heap_end == 0) {
        fprintf(stderr, "p4pc: heap used before p4pc_heap_init\n");
        abort();
    }
    return P4_G32(0x00710070u);
}

/* Retail 00421ee0: old break is returned, addition is EE-width, only the
 * syscall-reported upper heap boundary is compared, and ENOMEM is guest errno. */
uint64_t p4pc_ee_heap_sbrk(int32_t increment) {
    uint32_t old = P4_G32(0x0070c5d4u);
    uint32_t next = old + (uint32_t)increment;
    if (next > heap_end) {
        P4_G32(heap_reent()) = 12;
        return UINT64_MAX;
    }
    P4_G32(0x0070c5d4u) = next;
    return (uint64_t)(int64_t)(int32_t)old;
}

static void *heap_raw_malloc(size_t size) {
    return P4_G2H(p4pc_ee_heap_malloc(heap_reent(), (uint32_t)size));
}

static void heap_raw_free(void *pointer) {
    p4pc_ee_heap_free(heap_reent(), P4_H2G(pointer));
}

/* P4PC_HEAP_CHECK=1: an explicitly perturbing debugging layer over the retail heap. Every block gets a 48-byte header (magic, size, the three
 * return addresses that allocated it, links) and a 16-byte tail guard, both pattern-filled; a freed block is poisoned
 * and kept in a quarantine of 8192 blocks before its memory is really freed. Every block is verified when it is freed
 * and the whole list once per presented frame (p4pc_heap_verify, called from the flip): a header or guard that was
 * written, or a freed block written to (a use after free), is reported with its address, size and allocation sites
 * (resolve them with gdb `info symbol`) and the program aborts. P4PC_HEAP_CHECK=2 also fills new blocks with 0xCD.
 * P4PC_HEAP_CHECK=3 also makes the whole pages of a freed block of 4 KB or more inaccessible while it is quarantined, so
 * the first stale read or write of such a block faults at the user (the gdb report names it); smaller blocks stay poisoned. */

/* The debug header holds host pointers (return addresses, links): 48 bytes on i386, 80 on a 64-bit host. */
enum { HDR = sizeof(void *) == 4 ? 48 : 80, GUARD = 16, QUARANTINE = 8192 };
typedef struct hdr {
    uint32_t magic, size;
    void *site[3];
    struct hdr *prev, *next;
    uint32_t state, protected_pages; /* mode 3: the block's whole pages are PROT_NONE while quarantined */
    void *freed_by[3]; /* the callers of the free that poisoned the block (frame-pointer walk) */
} hdr;

_Static_assert(sizeof(hdr) <= HDR, "header fits");
static int dbg_mode = -1;
static hdr *dbg_all;
static hdr *quarantine[QUARANTINE];
static unsigned qhead, qcount;
static size_t qbytes;
#define QUARANTINE_BYTES (2u << 20)

int p4pc_heap_guest_host_overlap(uint32_t address, size_t size) {
    if (size == 0 || dbg_mode <= 0) return 0;
    if (size > UINT64_C(0x100000000) - address) return 1;
    uint64_t end = (uint64_t)address + size;
    for (const hdr *h = dbg_all; h != NULL; h = h->next) {
        uint64_t begin = P4_H2G(h);
        uint64_t payload = begin + HDR;
        uint64_t tail = payload + h->size;
        if ((address < payload && begin < end) ||
            (address < tail + GUARD && tail < end) ||
            (h->state == 1 && address < tail && payload < end))
            return 1;
    }
    return 0;
}

static int dbg_on(void) {
    if (dbg_mode < 0) {
        const char *e = getenv("P4PC_HEAP_CHECK");
        dbg_mode = (e != NULL && *e != '\0') ? atoi(e) : 0;
        if (dbg_mode) {
            fprintf(stderr, "p4pc: heap check on (mode %d)\n", dbg_mode);
        }
    }
    return dbg_mode;
}

static void dbg_fail(const hdr *h, const char *what) {
    fprintf(stderr, "p4pc: HEAP CORRUPTION: %s: block %p size %u allocated by %p %p %p (state %u)\n", what,
            (const void *)((const uint8_t *)h + HDR), h->size, h->site[0], h->site[1], h->site[2], h->state);
    if (h->state == 1) {
        fprintf(stderr, "p4pc: freed by %p %p %p\n", h->freed_by[0], h->freed_by[1], h->freed_by[2]);
    }
    fflush(stderr);
    abort();
}

static void dbg_check(const hdr *h) {
    const uint8_t *raw = (const uint8_t *)h, *user = raw + HDR;
    uint32_t i;

    if (h->magic != 0xC0FFEE01u) {
        dbg_fail(h, "header magic");
    }
    for (i = sizeof(hdr); i < HDR; i++) {
        if (raw[i] != 0xA5) {
            dbg_fail(h, "header padding written");
        }
    }
    for (i = 0; i < GUARD; i++) {
        if (user[h->size + i] != 0xA5) {
            dbg_fail(h, "tail guard written (overflow)");
        }
    }
    if (h->state == 1 && !h->protected_pages) {
        for (i = 0; i < h->size; i++) {
            if (user[i] != 0xDD) {
                fprintf(stderr, "p4pc: first stale write at block offset %u (%02x), block holds:", i, user[i]);
                for (uint32_t k = 0; k < 16 && i + k < h->size; k++) {
                    fprintf(stderr, " %02x", user[i + k]);
                }
                fprintf(stderr, "\n");
                dbg_fail(h, "freed block written (use after free)");
            }
        }
    }
}

static void dbg_link(hdr *h) {
    h->prev = NULL;
    h->next = dbg_all;
    if (dbg_all != NULL) {
        dbg_all->prev = h;
    }
    dbg_all = h;
}

static void dbg_unlink(hdr *h) {
    if (h->prev != NULL) {
        h->prev->next = h->next;
    } else {
        dbg_all = h->next;
    }
    if (h->next != NULL) {
        h->next->prev = h->prev;
    }
}

void p4pc_heap_verify(void) {
    hdr *h;

    if (dbg_on()) {
        for (h = dbg_all; h != NULL; h = h->next) {
            dbg_check(h);
        }
    }
}

static void *dbg_malloc(size_t n, void *s0, void *s1, void *s2) {
    static unsigned count;

    if ((++count & 31) == 0) {
        p4pc_heap_verify();                           /* narrows a corruption to a few allocations */
    }
    uint8_t *raw = heap_raw_malloc(HDR + n + GUARD);
    hdr *h = (hdr *)raw;

    if (raw == NULL) {
        {
            fprintf(stderr, "p4pc: heap check: allocation of %lu bytes failed (caller %p): footprint %lu\n", (unsigned long)n, s0,
                    (unsigned long)(P4_G32(0x0070c5d4u) - heap_base));
        }
        return NULL;
    }
    memset(raw, 0xA5, HDR);                       /* leave fresh payload bytes as the retail heap supplied them */
    memset(raw + HDR + n, 0xA5, GUARD);
    h->magic = 0xC0FFEE01u;
    h->size = (uint32_t)n;
    h->site[0] = s0;
    h->site[1] = s1;
    h->site[2] = s2;
    h->state = 0;
    h->protected_pages = 0;
    dbg_link(h);
    if (dbg_mode >= 2) {
        memset(raw + HDR, 0xCD, n);
    }
    return raw + HDR;
}

static void dbg_free(void *p, void *ra) {
    hdr *h;
    void **fp = (void **)__builtin_frame_address(0);
    unsigned k;

    if (p == NULL) {
        return;
    }
    h = (hdr *)((uint8_t *)p - HDR);
    if (h->state == 1) {
        dbg_fail(h, "double free");
    }
    dbg_check(h);
    h->state = 1;
    h->freed_by[0] = ra;                         /* func_0043ed08's caller, then the frames above it while the chain is sane */
    h->freed_by[1] = h->freed_by[2] = NULL;
    for (k = 1; k < 3; k++) {
        void **next = (void **)fp[0];

        if (next <= fp || (uintptr_t)next - (uintptr_t)fp > (1u << 20) || ((uintptr_t)next & 3) != 0) {
            break;
        }
        fp = next;
        h->freed_by[k] = fp[1];
    }
    memset(p, 0xDD, h->size);
    if (dbg_mode >= 3 && h->size >= 4096) {
        h->protected_pages = p4_os_protect_pages(p, h->size, 0) == 0;
    }
    quarantine[(qhead + qcount++) % QUARANTINE] = h;
    qbytes += h->size + HDR + GUARD;
    while (qcount == QUARANTINE || qbytes > QUARANTINE_BYTES) {      /* release the oldest, verified, as fresh memory */
        hdr *old = quarantine[qhead];
        if (old->protected_pages) {
            p4_os_protect_pages((uint8_t *)old + HDR, old->size, 1);
            old->protected_pages = 0;
        }
        dbg_check(old);
        dbg_unlink(old);
        qbytes -= old->size + HDR + GUARD;
        qhead = (qhead + 1) % QUARANTINE;
        qcount--;
        memset((uint8_t *)old + HDR, 0, old->size);
        heap_raw_free(old);
    }
}

static inline __attribute__((always_inline)) void *p4_malloc(size_t n) {
    return dbg_on() ? dbg_malloc(n, __builtin_return_address(0), 0, 0)
                    : heap_raw_malloc(n);
}

/* Attribution is host-only metadata. Forget before poisoning/protecting/freeing
 * bytes; never change the EE allocator's chunk headers or allocation choices. */
static void forget_content_allocation(void *p)
{
    if (heap_observer && p) {
        if (dbg_on()) {
            hdr *h=(hdr *)((uint8_t *)p-HDR);
            heap_observer(p,h->size);
        } else {
            /* Retail free 0043e194 reads chunk size at user-4. The next
             * chunk's prev_size word is usable while this chunk is live. */
            uint32_t size=P4_G32(P4_H2G(p)-4u)&~3u;
            if (size>=8u) heap_observer(p,size-4u);
        }
    }
}

static inline __attribute__((always_inline)) void p4_free(void *p) {
    forget_content_allocation(p);
    if (dbg_on()) {
        dbg_free(p, __builtin_return_address(0));
    } else {
        heap_raw_free(p);
    }
}

static inline __attribute__((always_inline)) void *p4_realloc(void *p, size_t n) {
    void *q;
    forget_content_allocation(p);

    if (!dbg_on()) {
        return P4_G2H(p4pc_ee_heap_realloc(heap_reent(), P4_H2G(p), (uint32_t)n));
    }
    if (p == NULL) {
        return p4_malloc(n);
    }
    q = dbg_malloc(n, __builtin_return_address(0), 0, 0);
    if (q != NULL) {
        uint32_t old = ((hdr *)((uint8_t *)p - HDR))->size;
        memcpy(q, p, old < n ? old : n);
        dbg_free(p, __builtin_return_address(0));
    }
    return q;
}

static inline __attribute__((always_inline)) void *p4_calloc(size_t count, size_t size) {
    if (!dbg_on())
        return P4_G2H(p4pc_ee_heap_calloc(heap_reent(), (uint32_t)count, (uint32_t)size));
    void *p = p4_malloc(count * size);

    if (p != NULL) {
        memset(p, 0, count * size);
    }
    return p;
}

/* malloc(n): returns the block, or 0. */
uint8_t *func_0043ece8(int32_t n) {
    return p4_malloc((size_t)(uint32_t)n);
}

/* free(p). */
void func_0043ed08(uint8_t *p) {
    p4_free(p);
}
/* 0043c710 -> 0043c738: mult a1,a1,a2 at 0043c748 passes the low
 * 32-bit product to _malloc_r, not a host-size_t product or checked multiply.
 * The raw core also clears the original chunk's usable bytes, including padding. */
void *func_0043c710(uint32_t count, uint32_t size) {
    return p4_calloc(1, (uint32_t)(count * size));
}

/* 00441558 forwards (active reent, pointer, 32-bit size) to 00441580.
 * A null pointer allocates; zero size retains a minimum allocation rather
 * than implicitly freeing it (004415b4..00441620). */
void *func_00441558(void *pointer, uint32_t size) {
    return p4_realloc(pointer, size);
}


/* Native adapters that explicitly need guest-visible storage, not the host heap. */
void *p4pc_heap_realloc(void *p, size_t n) {
    return p4_realloc(p, n);
}

void *p4pc_heap_calloc(size_t count, size_t size) {
    return p4_calloc(count, size);
}

void *p4pc_heap_malloc(size_t n) {
    return p4_malloc(n);
}

void p4pc_heap_free(void *p) {
    p4_free(p);
}

/* _malloc_r(reent, n). */
uint8_t *func_0043ef80(void *reent, int32_t n) {
    return dbg_on() ? p4_malloc((uint32_t)n)
                    : P4_G2H(p4pc_ee_heap_malloc(P4_H2G(reent), (uint32_t)n));
}

/* _free_r(reent, p). */
void func_0043e170(void *reent, uint8_t *p) {
    if (dbg_on()) p4_free(p);
    else p4pc_ee_heap_free(P4_H2G(reent), P4_H2G(p));
}
