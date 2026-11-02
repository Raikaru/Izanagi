/* RxHeap host test: the retail allocator (RenderWare RxHeap) as the overrides implement it.
 * func_00415070 creates a heap, func_00414e10 allocates (growing it through func_00414a30 and
 * carving through func_00414b50), func_00414c10 frees and merges with free neighbours. The
 * memory function table (alloc, free and realloc slots at 0x008873e8..f0) and the two RenderWare
 * error calls are test doubles; the table is mapped at its retail address (Linux, MAP_FIXED).
 * Checks: block spacing, the free-entry count after each free case (no merge, merge next, merge
 * both), full recovery of the initial free size, growth by a second superblock, and the
 * out-of-memory error (code 0x80000013 with the rounded size) when the allocator fails. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "p4pc/os.h"
#include "guest_test.h"

#ifdef P4_GUEST64
/* decomp type.h clashes with the system headers on a 64-bit host (uintptr_t); the names the test uses are these */
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
#else
/* The overrides read two jump-table slots as symbols at their retail addresses. */
P4_ABSOLUTE_SYMBOL(jtbl_008873E8, 0x008873E8);
P4_ABSOLUTE_SYMBOL(jtbl_008873EC, 0x008873EC);
#endif

/* The heap records are guest memory: a link is a 4-byte guest address, read here as a host pointer. */
#define LINK(x) ((u32 *)P4_G2H((uint32_t)(x)))

u32 *func_00415070(u32 arg0);
u8 *func_00414e10(u8 *heap, s32 requested);
void func_00414c10(u8 *heap, u8 *block);
void *RxHeapRealloc(u8 *heap, void *block, u32 new_size, int allow_copy);

static int checks;
static int failures;
static int alloc_fails;
static s32 error_code;
static s32 error_size;
static int error_raised;

#define CHECK(cond)                                                       \
    do {                                                                  \
        checks++;                                                         \
        if (!(cond)) {                                                    \
            failures++;                                                   \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                 \
    } while (0)

/* What the retail allocator slots hand out is stored in the heap's own records as guest addresses: guest memory (the test heap of tests/guest_test.c: guest storage) on a 64-bit host. */
#ifdef P4_GUEST64
#define T_CALLOC(n) p4pc_heap_calloc(1, (n))
#define T_FREE(p) p4pc_heap_free(p)
#define T_REALLOC(p, n) p4pc_heap_realloc((p), (n))
#else
#define T_CALLOC(n) calloc(1, (n))
#define T_FREE(p) free(p)
#define T_REALLOC(p, n) realloc((p), (n))
#endif

static void *tst_alloc(u32 size, u32 flags)
{
    (void)flags;
    return alloc_fails ? NULL : T_CALLOC(size);
}

static void tst_free(void *p)
{
    T_FREE(p);
}

static void *tst_realloc(void *p, u32 size, u32 flags)
{
    (void)flags;
    return T_REALLOC(p, size);
}

s32 func_003df590(s32 code, s32 size)
{
    error_code = code;
    error_size = size;
    return code;
}

void func_003df4d0(void *err)
{
    (void)err;
    error_raised++;
}

static u32 *entries_of(u32 *heap)
{
    return LINK(heap[3]);
}

static int run_test(int argc, char **argv)
{
    u32 *heap;
    u32 initial;
    u8 *p1, *p2, *p3, *big;
    u32 superblocks;
    u32 *sb;

    (void)argc;
    (void)argv;

    if (p4t_guest_map(0x00887000u, 4096u) != 0) {
        printf("rxheap_test: cannot map the memory function table\n");
        return 77;
    }
    *(uint32_t *)P4_G2H(0x008873E8u) = p4_fn_h2g_sig(tst_alloc, "pii");
    *(uint32_t *)P4_G2H(0x008873ECu) = p4_fn_h2g_sig(tst_free, "vp");
    *(uint32_t *)P4_G2H(0x008873F0u) = p4_fn_h2g_sig(tst_realloc, "ppii");

    heap = func_00415070(2048);
    CHECK(heap != NULL);
    CHECK(heap[5] == 1);
    initial = entries_of(heap)[0];
    CHECK(initial > 1800 && initial < 2048);

    /* Requests round up to 0x20; each block sits 0x20 (header) past the previous one's end. */
    p1 = func_00414e10((u8 *)heap, 100);
    p2 = func_00414e10((u8 *)heap, 200);
    p3 = func_00414e10((u8 *)heap, 50);
    CHECK(p1 && p2 && p3);
    CHECK(p2 == p1 + 128 + 0x20);
    CHECK(p3 == p2 + 224 + 0x20);
    CHECK(heap[5] == 1);
    CHECK(entries_of(heap)[0] == initial - (128 + 0x20) - (224 + 0x20) - (64 + 0x20));

    /* Neither neighbour free: a new entry. */
    func_00414c10((u8 *)heap, p2);
    CHECK(heap[5] == 2);
    /* Next neighbour free: merges into the existing entry. */
    func_00414c10((u8 *)heap, p1);
    CHECK(heap[5] == 2);
    /* Both neighbours free: the three blocks and the remainder become one. */
    func_00414c10((u8 *)heap, p3);
    CHECK(heap[5] == 1);
    CHECK(entries_of(heap)[0] == initial);

    /* RxHeapRealloc. Shrinking a large block splits off a free entry; growing into the free block
     * that follows keeps the pointer; growing with an allocated neighbour moves and copies. */
    {
        u8 *a = func_00414e10((u8 *)heap, 1000);
        u8 *b, *c;
        u32 used_before = heap[5];

        CHECK(a != NULL);
        CHECK(RxHeapRealloc((u8 *)heap, a, 100, 0) == a);
        CHECK(heap[5] == used_before + 1 || heap[5] == used_before);
        CHECK(*(u32 *)(a - 0x20 + 8) == 128);
        CHECK(RxHeapRealloc((u8 *)heap, a, 600, 0) == a);          /* the freed tail is next */
        CHECK(*(u32 *)(a - 0x20 + 8) == 608);

        b = func_00414e10((u8 *)heap, 200);
        memset(a, 0x5a, 100);
        c = RxHeapRealloc((u8 *)heap, a, 700, 0);                 /* b blocks the way: move */
        CHECK(c != NULL && c != a);
        CHECK(memcmp(c, a, 0) == 0 && c[0] == 0x5a && c[99] == 0x5a);
        func_00414c10((u8 *)heap, c);
        func_00414c10((u8 *)heap, b);
        CHECK(heap[5] == 1);
        CHECK(entries_of(heap)[0] == initial);
    }

    /* A request larger than the free block grows the heap by a superblock. */
    big = func_00414e10((u8 *)heap, (s32)initial + 1000);
    CHECK(big != NULL);
    superblocks = 0;
    for (sb = LINK(heap[1]); sb != NULL; sb = LINK(sb[2])) {
        superblocks++;
    }
    CHECK(superblocks == 2);
    func_00414c10((u8 *)heap, big);

    /* Allocator failure: NULL, and the out-of-memory error with the rounded size. */
    alloc_fails = 1;
    big = func_00414e10((u8 *)heap, 1000000);
    CHECK(big == NULL);
    CHECK(error_raised == 1);
    CHECK((u32)error_code == 0x80000013u);
    CHECK(error_size == 1000000);

    printf("rxheap_test: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(run_test, argc, argv);
}
