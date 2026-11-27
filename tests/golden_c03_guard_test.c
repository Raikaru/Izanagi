#include "p4pc/c03_guard.h"
#include "p4pc/guest.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <sys/mman.h>
#endif

#if defined(P4_GUEST64) && (defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__)))
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "C03 test line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static int arena_is_unmapped(void *address)
{
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION info;
    return VirtualQuery(address, &info, sizeof info) && info.State == MEM_FREE;
#else
    unsigned char present;
    return mincore(address, 4096, &present) == -1 && errno == ENOMEM;
#endif
}
#if defined(__x86_64__)
/* Both target ABIs preserve r12 across the shutdown call. Reserve the Windows
 * home area as well; the stack is restored before returning to C. */
static void shutdown_on_guest_stack(void *top)
{
    __asm__ volatile(
        "mov %%rsp, %%r12\n\t"
        "mov %0, %%rsp\n\t"
        "and $-16, %%rsp\n\t"
        "sub $32, %%rsp\n\t"
        "call p4_c03_guard_shutdown\n\t"
        "mov %%r12, %%rsp\n\t"
        : : "r"(top) : "r12", "rax", "rcx", "rdx", "rsi", "rdi",
          "r8", "r9", "r10", "r11", "xmm0", "xmm1", "xmm2", "xmm3",
          "xmm4", "xmm5", "xmm6", "xmm7", "xmm8", "xmm9", "xmm10",
          "xmm11", "xmm12", "xmm13", "xmm14", "xmm15", "memory", "cc");
}
#else
/* The guest-stack switch is x86-64 assembly; other hosts report the case as skipped. */
static void shutdown_on_guest_stack(void *top)
{
    (void)top;
    exit(77);
}
#endif
int main(int argc, char **argv)
{
    CHECK(argc == 2);
    if (!strcmp(argv[1], "ps2")) {
        CHECK(p4_guest_init() == 0);
        unsigned char expected[4096];
        unsigned char *page = P4_G2H(0x00745000u);
        for (size_t i = 0; i < sizeof expected; ++i)
            expected[i] = (unsigned char)(i * 37u + 11u);
        memcpy(page, expected, sizeof expected);
        CHECK(page == p4_guest_base + 0x00745000u);
        CHECK(!memcmp(page, expected, sizeof expected));
        CHECK(!p4_c03_guard_active() && !p4_c03_guard_tainted());
        return 0;
    }
    CHECK(p4_guest_arena_factory(p4_c03_guard_reserve) == 0);
    CHECK(p4_guest_init() == 0);
    unsigned char *view = P4_G2H(0x007458c0u);
    view[8] = 23; /* Image loading precedes arming. */
    CHECK(p4_c03_guard_start() == 0);
    CHECK(view == p4_guest_base + 0x007458c0u);
    CHECK(P4_H2G(view) == 0x007458c0u);
    CHECK(P4_G2H(0x007458c8u) == view + 8);
    CHECK(view[8] == 23 && !p4_c03_guard_tainted());
    uint32_t value = 0x12345678;
    p4_c03_guard_store(16, &value, sizeof value);
    CHECK(!memcmp(view + 16, &value, sizeof value));
    CHECK(!p4_c03_guard_tainted());
    if (!strcmp(argv[1], "admitted")) {
        p4_c03_guard_shutdown();
        CHECK(arena_is_unmapped(p4_guest_base));
        CHECK(!p4_c03_guard_active() && p4_c03_guard_alias_released());
        return 0;
    }
    if (!strcmp(argv[1], "stack")) {
        shutdown_on_guest_stack(p4_guest_base + 0x02100000u);
        CHECK(!p4_c03_guard_active() && p4_c03_guard_alias_released());
        CHECK(view[8] == 23); /* Arena remains valid until the guest stack is left. */
        p4_c03_guard_shutdown();
        CHECK(arena_is_unmapped(p4_guest_base));
        CHECK(p4_c03_guard_alias_released());
        return 0;
    }
    if (!strcmp(argv[1], "unknown")) {
        *(volatile unsigned char *)P4_G2H(0x007458c8u) = 42;
        CHECK(view[8] == 42);
    } else if (!strcmp(argv[1], "neighbor")) {
        *(volatile unsigned char *)P4_G2H(0x00745000u) = 99;
    } else return 2;
    CHECK(p4_c03_guard_tainted() && p4_c03_guard_origin() && p4_c03_guard_address());
    p4_c03_guard_shutdown();
    CHECK(!p4_c03_guard_active() && p4_c03_guard_alias_released());
    return 0;
}
#else
/* The guard exists only for 64-bit Windows and Linux hosts; the port64 variants of these cases run it. */
int main(void)
{
    puts("golden_c03_guard_test: nothing to check on this host (the guard is 64-bit only)");
    return 0;
}
#endif
