/* Shared by the retail override tests (*_test.c files compiling overrides/<addr>.c): the RenderWare allocator slots the overrides call through, as the test
 * provides them on both targets. On i386 the override names the slots by `__asm__` label and the test defines them as symbols. On a 64-bit host the override is
 * rewritten (unit64.py build --src) and reads guest memory at the retail address 0x008873E8 (rwmalloc) and 0x008873EC (rwfree), where the slots hold guest
 * function addresses: ovr_slots_init() maps that memory and stores them. Include after guest_test.h. */
#ifndef P4PC_TESTS_OVR_GUEST_H
#define P4PC_TESTS_OVR_GUEST_H

#include <stdint.h>

#include "guest_test.h"

#ifdef P4_GUEST64
#define OVR_RW_SLOTS(malloc_fn, free_fn)                                                  \
    static int ovr_slots_init(void)                                                       \
    {                                                                                     \
        if (p4t_guest_map(0x008873E8u, 0x10) != 0) {                                      \
            return -1;                                                                    \
        }                                                                                 \
        ((uint32_t *)P4_G2H(0x008873E8u))[0] = p4_fn_h2g_sig(malloc_fn, "pii");          \
        ((uint32_t *)P4_G2H(0x008873ECu))[0] = p4_fn_h2g_sig(free_fn, "vp");             \
        return 0;                                                                         \
    }
#else
#define OVR_RW_SLOTS(malloc_fn, free_fn)                                                  \
    void *(*D_008873E8[1])(int, int) = {malloc_fn};                                       \
    void (*D_008873EC[1])(void *) = {free_fn};                                            \
    static int ovr_slots_init(void)                                                       \
    {                                                                                     \
        return 0;                                                                         \
    }
#endif

#endif
