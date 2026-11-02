/* x86_64 coroutines on guest stacks (include/p4pc/os.h, docs/port64.md 2.7): the context switch in os_posix.c runs each coroutine on
 * memory from the guest block's stack arena, so the address of a local is a valid guest address. Checks that
 *  - a coroutine's locals are in the arena (P4_H2G of their address is inside the stack range),
 *  - a switch preserves the callee-saved registers (r12..r15, rbx; x19..x22 on AArch64) of both sides,
 *  - values survive 100000 switches in a ring of three coroutines,
 *  - a deep recursion (about 200 KB of stack) works on a coroutine stack,
 *  - a stack allocation can be freed and reused.
 * Prints one line per check; exits 0 when all pass. Built for x86_64 only (tests/CMakeLists.txt). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "p4pc/guest.h"
#include "p4pc/os.h"

#define STACK (256u * 1024u)
#define RING 3
#define ROUNDS 100000

static p4_coro *main_co, *ring_co[RING];
/* volatile: they change while a member is switched out, and a compiler may otherwise keep them in a register across the call */
static volatile unsigned long counter;
static volatile unsigned long sum;
static int failures;

static void check(int ok, const char *what) {
    printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    if (!ok) failures++;
}

static int in_stack_arena(const void *p) {
    uint32_t g = P4_H2G(p);
    return g >= P4_GUEST_STACKS && g < P4_GUEST_STACKS + P4_GUEST_STACKS_SIZE;
}

static int bad_stack;

static unsigned depth(unsigned n) {
    volatile char pad[512];
    pad[0] = (char)n;
    pad[511] = (char)(n >> 8);
    if (!in_stack_arena((const void *)pad)) bad_stack = 1;
    return n == 0 ? 0 : 1 + depth(n - 1) + (unsigned)(pad[0] - (char)n);
}

static unsigned deep_result;

#if defined(__x86_64__)
/* Switches with known values in the callee-saved registers r12..r15 and checks them when control comes back. The call is made from
 * assembly so the compiler cannot keep the values anywhere else. Returns 1 when all four survived. */
static int switch_check(p4_coro *from, p4_coro *to, uint64_t k) {
    int ok;
    __asm__ volatile(
        "movq %%rsp, %%rbx\n\t"
        "movq %[k], %%r12\n\t"
        "leaq 1(%%r12), %%r13\n\t"
        "leaq 2(%%r12), %%r14\n\t"
        "leaq 3(%%r12), %%r15\n\t"
        "movq %[f], %%rdi\n\t"
        "movq %[t], %%rsi\n\t"
        "subq $256, %%rsp\n\t"
        "andq $-16, %%rsp\n\t"
        "call p4_coro_switch@PLT\n\t"
        "movq %%rbx, %%rsp\n\t"
        "movq %[k], %%rax\n\t"
        "cmpq %%rax, %%r12\n\t"
        "jne 1f\n\t"
        "incq %%rax\n\t"
        "cmpq %%rax, %%r13\n\t"
        "jne 1f\n\t"
        "incq %%rax\n\t"
        "cmpq %%rax, %%r14\n\t"
        "jne 1f\n\t"
        "incq %%rax\n\t"
        "cmpq %%rax, %%r15\n\t"
        "jne 1f\n\t"
        "movl $1, %[ok]\n\t"
        "jmp 2f\n"
        "1:\n\t"
        "movl $0, %[ok]\n"
        "2:"
        : [ok] "=m"(ok)
        : [k] "m"(k), [f] "m"(from), [t] "m"(to)
        : "rax", "rbx", "rcx", "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "xmm0", "xmm1", "xmm2", "xmm3",
          "xmm4", "xmm5", "xmm6", "xmm7", "memory", "cc");
    return ok;
}
#elif defined(__aarch64__)
/* The same on AArch64: x19..x22 hold known values across the switch and are stored to memory when control comes back. */
static int switch_check(p4_coro *from, p4_coro *to, uint64_t k) {
    uint64_t got[4] = {0, 0, 0, 0};
    __asm__ volatile(
        "mov x19, %[k]\n\t"
        "add x20, x19, #1\n\t"
        "add x21, x19, #2\n\t"
        "add x22, x19, #3\n\t"
        "mov x0, %[f]\n\t"
        "mov x1, %[t]\n\t"
        "bl p4_coro_switch\n\t"
        "mov x9, %[g]\n\t"
        "stp x19, x20, [x9]\n\t"
        "stp x21, x22, [x9, #16]\n"
        :
        : [k] "r"(k), [f] "r"(from), [t] "r"(to), [g] "r"(got)
        : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18",
          "x19", "x20", "x21", "x22", "x30", "memory", "cc");
    return got[0] == k && got[1] == k + 1 && got[2] == k + 2 && got[3] == k + 3;
}
#else
#error "port64_coro_test: no register check for this architecture"
#endif

/* Each ring member passes control to the next one, ROUNDS times in all, with its own register pattern. */
static void member(void) {
    int me = (int)(counter % RING);
    uint64_t keep = 0x1111111111111111ull * (uint64_t)(me + 1);
    for (;;) {
        if (counter >= ROUNDS) p4_coro_switch(ring_co[me], main_co);
        counter++;
        sum += (unsigned long)me + 1;
        if (!switch_check(ring_co[me], ring_co[(me + 1) % RING], keep)) {
            printf("FAIL: callee-saved registers changed across a switch in member %d\n", me);
            failures++;
        }
    }
}

static void deeper(void) {
    int local;
    if (!in_stack_arena(&local)) bad_stack = 1;
    deep_result = depth(300);
    p4_coro_switch(ring_co[0], main_co);
    for (;;) p4_coro_switch(ring_co[0], main_co);
}

int main(void) {
    if (p4_guest_init() != 0) {
        fprintf(stderr, "cannot map guest memory\n");
        return 77;
    }
    main_co = p4_coro_adopt();
    void *stacks[RING];
    for (int i = 0; i < RING; i++) {
        stacks[i] = p4_guest_stack_alloc(STACK);
        if (!stacks[i]) {
            printf("FAIL: stack %d not allocated\n", i);
            return 1;
        }
        ring_co[i] = p4_coro_create(stacks[i], STACK, member);
    }
    check(in_stack_arena(stacks[0]) && in_stack_arena((char *)stacks[2] + STACK - 1), "stacks are in the guest stack arena");

    /* the registers of the side that switches away and comes back (main) */
    check(switch_check(main_co, ring_co[0], 0xABCDEF0123456789ull), "the main side's callee-saved registers survive the ring");
    check(counter == ROUNDS, "the ring ran 100000 hand-overs");
    check(sum == (unsigned long)(ROUNDS / RING) * 6 + (ROUNDS % RING == 1 ? 1 : ROUNDS % RING == 2 ? 3 : 0),
          "each member ran its share in order");
    check(failures == 0, "callee-saved registers of every member survive every switch");

    /* a fresh coroutine on a recycled stack: deep recursion */
    p4_coro_destroy(ring_co[0]);
    p4_guest_stack_free(stacks[0]);
    void *again = p4_guest_stack_alloc(STACK);
    check(again == stacks[0], "a freed stack's arena space is the first fit for the next one");
    p4_guest_stack_free(again);
    void *big = p4_guest_stack_alloc(1024u * 1024u);
    check(big != NULL, "a 1 MB stack is allocated after the first three");
    ring_co[0] = p4_coro_create(big, 1024u * 1024u, deeper);
    p4_coro_switch(main_co, ring_co[0]);
    check(deep_result == 301u || deep_result == 300u, "300-deep recursion on a coroutine stack returns");
    check(bad_stack == 0, "every local seen on a coroutine stack is a guest stack address");
    return failures ? 1 : 0;
}
