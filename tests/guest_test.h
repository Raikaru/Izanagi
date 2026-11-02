/* Fixtures for host tests of port code that reads and writes guest memory (docs/port64.md).
 *
 * A test that touches retail data at a fixed address calls p4t_guest_map(addr, size) first: on i386 it maps
 * that range at its retail address (the host address IS the guest address), on a 64-bit host it reserves the
 * guest block (p4_guest_init), where the same address resolves through P4_G2H. Storage the code under test
 * reaches through a guest address (buffers, objects that hold pointers) comes from p4t_galloc, never from the
 * stack, a static or the host heap. Link tests/guest_test.c and src/platform/gport.c (and
 * src/platform/guest.c on a 64-bit host). */
#ifndef P4PC_TESTS_GUEST_TEST_H
#define P4PC_TESTS_GUEST_TEST_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "p4pc/gmem.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"

static inline int p4t_guest_map(uint32_t addr, uint32_t size)
{
#ifdef P4_GUEST64
    (void)addr;
    (void)size;
    return p4_guest_init();
#else
    const char *why = NULL;
    uint32_t lo = addr & ~0xFFFu;
    uint32_t hi = (addr + size + 0xFFFu) & ~0xFFFu;

    if (p4_os_map_fixed(lo, hi - lo, &why) != 0) {
        fprintf(stderr, "p4t_guest_map(0x%08x, %u): %s\n", (unsigned)addr, (unsigned)size, why != NULL ? why : "failed");
        return -1;
    }
    return 0;
#endif
}

/* The guest block, for a test that needs guest stacks or storage but maps no retail address: a no-op on i386. */
static inline int p4t_guest_init(void)
{
#ifdef P4_GUEST64
    return p4_guest_init();
#else
    return 0;
#endif
}

/* Runs fn(argc, argv) on a stack in guest memory and returns its result: a test that hands the address of one of its locals to the mirror
 * (an m2c int parameter takes a guest address) needs its stack in the block, as the game's has (src/crt.c). The process stack hosts
 * main() until the switch. A plain call on i386, where every address is a guest address. */
#ifdef P4_GUEST64
static int (*p4t_main_fn)(int, char **);
static int p4t_main_argc, p4t_main_rc;
static char **p4t_main_argv;
static p4_coro *p4t_boot_co, *p4t_test_co;

static void p4t_main_entry(void)
{
    p4t_main_rc = p4t_main_fn(p4t_main_argc, p4t_main_argv);
    p4_coro_switch(p4t_test_co, p4t_boot_co);
    abort();
}

static inline int p4t_on_guest_stack(int (*fn)(int, char **), int argc, char **argv)
{
    const size_t size = 4u << 20;
    void *stack;

    if (p4_guest_init() != 0 || (stack = p4_guest_stack_alloc(size)) == NULL) {
        fprintf(stderr, "p4t_on_guest_stack: no guest memory for the test stack\n");
        return 2;
    }
    p4t_main_fn = fn;
    p4t_main_argc = argc;
    p4t_main_argv = argv;
    p4t_boot_co = p4_coro_adopt();
    p4t_test_co = p4_coro_create(stack, size, p4t_main_entry);
    p4_coro_switch(p4t_boot_co, p4t_test_co);
    return p4t_main_rc;
}
#else
static inline int p4t_on_guest_stack(int (*fn)(int, char **), int argc, char **argv)
{
    return fn(argc, argv);
}
#endif

/* Zeroed guest storage of `size` bytes, `align` aligned, as a host pointer. */
static inline void *p4t_galloc(uint32_t size, uint32_t align)
{
    return P4_G2H(p4_gport_alloc(size, align));
}

/* A region a test wants at a low address (code that masks addresses to 28 bits): mapped at `addr` on i386, plain
 * guest storage on a 64-bit host, where every guest address is below 64 MB anyway. */
static inline void *p4t_garena(uint32_t addr, uint32_t size, uint32_t align)
{
#ifdef P4_GUEST64
    (void)addr;
    return p4t_galloc(size, align);
#else
    (void)align;
    return p4t_guest_map(addr, size) == 0 ? P4_G2H(addr) : NULL;
#endif
}

/* The game heap (p4pc_heap_*) over `size` bytes of guest storage, for a test that links code allocating from it
 * without running the boot that creates it. */
static inline int p4t_heap_init(uint32_t size)
{
    return p4pc_heap_init(p4t_galloc(size, 4096), size);
}

#endif
