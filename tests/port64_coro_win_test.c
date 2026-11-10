/* Windows x64 coroutines on caller-supplied stacks (src/platform/os/os_win32.c, docs/port64.md 2.7, 2.13). Checks that
 *  - values survive 100000 switches in a ring of three coroutines,
 *  - the callee-saved registers rbx, rsi, rdi, r12..r15 and xmm6..xmm15 of both sides survive a switch,
 *  - a recursion of about 200 KB and a frame of 64 KB (the __chkstk probe) work on a coroutine stack,
 *  - the NT_TIB stack fields describe the running coroutine's stack while it runs and the original ones are back after,
 *  - an access violation on a coroutine stack reaches a vectored exception handler (exception dispatch checks the stack pointer against the TIB).
 * Prints one line per check; exits 0 when all pass. Built for x86_64 Windows only (tests/CMakeLists.txt). */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/os.h"

#define STACK (256u * 1024u)
#define RING 3
#define ROUNDS 100000

static p4_coro *main_co, *ring_co[RING];
static volatile unsigned long counter;
static volatile unsigned long sum;
static int failures;

static void check(int ok, const char *what) {
    printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    if (!ok) failures++;
}

static void *new_stack(size_t size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

static void ring_entry(void) {
    static int next_index;
    int me = next_index++;
    for (;;) {
        counter++;
        sum += (unsigned long)me + 1;
        if (counter >= ROUNDS) p4_coro_switch(ring_co[me], main_co);
        p4_coro_switch(ring_co[me], ring_co[(me + 1) % RING]);
    }
}

/* Switch with known values in rbx, rsi, rdi, r12..r15 and xmm6..xmm15, and check them when control comes back. */
extern int p4t_regs_roundtrip(p4_coro *from, p4_coro *to);
__asm__(".text\n"
        ".globl p4t_regs_roundtrip\n"
        ".def p4t_regs_roundtrip; .scl 2; .type 32; .endef\n"
        "p4t_regs_roundtrip:\n"
        "\tpushq %rbx\n\tpushq %rsi\n\tpushq %rdi\n\tpushq %r12\n\tpushq %r13\n\tpushq %r14\n\tpushq %r15\n\tpushq %rbp\n"
        "\tsubq $200, %rsp\n"                                      /* 32 shadow space + 168 so that rsp is 16-aligned at the call */
        "\tmovq $0x1111, %rbx\n\tmovq $0x2222, %rsi\n\tmovq $0x3333, %rdi\n\tmovq $0x4444, %r12\n\tmovq $0x5555, %r13\n"
        "\tmovq $0x6666, %r14\n\tmovq $0x7777, %r15\n"
        "\tmovq $0x0101010101010101, %rax\n\tmovq %rax, %xmm6\n\tpunpcklqdq %xmm6, %xmm6\n"
        "\tmovaps %xmm6, %xmm7\n\tmovaps %xmm6, %xmm8\n\tmovaps %xmm6, %xmm9\n\tmovaps %xmm6, %xmm10\n"
        "\tmovaps %xmm6, %xmm11\n\tmovaps %xmm6, %xmm12\n\tmovaps %xmm6, %xmm13\n\tmovaps %xmm6, %xmm14\n\tmovaps %xmm6, %xmm15\n"
        "\tcall p4_coro_switch\n"                                    /* rcx = from, rdx = to are still ours */
        "\txorl %eax, %eax\n"
        "\tcmpq $0x1111, %rbx\n\tjne 1f\n\tcmpq $0x2222, %rsi\n\tjne 1f\n\tcmpq $0x3333, %rdi\n\tjne 1f\n"
        "\tcmpq $0x4444, %r12\n\tjne 1f\n\tcmpq $0x5555, %r13\n\tjne 1f\n\tcmpq $0x6666, %r14\n\tjne 1f\n\tcmpq $0x7777, %r15\n\tjne 1f\n"
        "\tmovq $0x0101010101010101, %rcx\n\tmovq %rcx, %xmm0\n\tpunpcklqdq %xmm0, %xmm0\n"
        "\tpcmpeqb %xmm0, %xmm6\n\tpmovmskb %xmm6, %ecx\n\tcmpl $0xffff, %ecx\n\tjne 1f\n"
        "\tpcmpeqb %xmm0, %xmm15\n\tpmovmskb %xmm15, %ecx\n\tcmpl $0xffff, %ecx\n\tjne 1f\n"
        "\tmovl $1, %eax\n"
        "1:\n"
        "\taddq $200, %rsp\n\tpopq %rbp\n\tpopq %r15\n\tpopq %r14\n\tpopq %r13\n\tpopq %r12\n\tpopq %rdi\n\tpopq %rsi\n\tpopq %rbx\n"
        "\tretq\n");

static p4_coro *clobber_co;
static void clobber_entry(void) {
    for (;;) {
        /* trash every register that must be preserved, then give control back */
        __asm__ volatile("movq $-1, %%rbx\n\tmovq $-1, %%rsi\n\tmovq $-1, %%rdi\n\tmovq $-1, %%r12\n\tmovq $-1, %%r13\n\tmovq $-1, %%r14\n\t"
                         "movq $-1, %%r15\n\txorps %%xmm6, %%xmm6\n\txorps %%xmm15, %%xmm15\n" ::: "rbx", "rsi", "rdi", "r12", "r13", "r14", "r15",
                         "xmm6", "xmm15", "memory");
        p4_coro_switch(clobber_co, main_co);
    }
}

static unsigned depth(unsigned n) {
    volatile char pad[512];
    pad[0] = (char)n;
    pad[511] = (char)(n >> 8);
    return n == 0 ? 0 : 1 + depth(n - 1) + (unsigned)(pad[0] - (char)n);
}

static unsigned deep_result;
static p4_coro *deep_co;
static void deep_entry(void) {
    deep_result = depth(300);                   /* ~150 KB */
    volatile char big[64 * 1024];               /* a frame of 64 KB: __chkstk probes it page by page */
    memset((void *)big, 7, sizeof big);
    deep_result += (unsigned)big[sizeof big - 1];
    p4_coro_switch(deep_co, main_co);
    for (;;) p4_coro_switch(deep_co, main_co);
}

static void *tib_base, *tib_limit, *tib_dealloc;
static p4_coro *tib_co;
static void tib_entry(void) {
    NT_TIB *t = (NT_TIB *)NtCurrentTeb();
    tib_base = t->StackBase;
    tib_limit = t->StackLimit;
    tib_dealloc = *(void **)((char *)t + 0x1478);
    p4_coro_switch(tib_co, main_co);
    for (;;) p4_coro_switch(tib_co, main_co);
}

static volatile int veh_hits;
static volatile void *veh_fault_sp;
static p4_coro *av_co;
static LONG CALLBACK veh(EXCEPTION_POINTERS *e) {
    if (e->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        veh_hits++;
        veh_fault_sp = (void *)e->ContextRecord->Rsp;
        /* skip the faulting instruction (a 3-byte `movb $1, (%rax)` with rax = 0 is at Rip) */
        e->ContextRecord->Rip += 3;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static void av_entry(void) {
    __asm__ volatile("xorl %%eax, %%eax\n\tmovb $1, (%%rax)\n" ::: "rax", "memory");   /* the 3-byte store of 0xC6 0x00 0x01 */
    p4_coro_switch(av_co, main_co);
    for (;;) p4_coro_switch(av_co, main_co);
}

int main(void) {
    void *stacks[RING + 4];
    int ok = 1;
    for (int i = 0; i < RING + 4; i++) {
        stacks[i] = new_stack(STACK);
        if (stacks[i] == NULL) ok = 0;
    }
    check(ok, "stacks allocated");
    main_co = p4_coro_adopt();
    for (int i = 0; i < RING; i++) ring_co[i] = p4_coro_create(stacks[i], STACK, ring_entry);
    p4_coro_switch(main_co, ring_co[0]);
    check(counter == ROUNDS, "100000 switches in a ring of three");
    check(sum >= ROUNDS && sum <= 3ul * ROUNDS, "each member ran");

    clobber_co = p4_coro_create(stacks[RING], STACK, clobber_entry);
    check(p4t_regs_roundtrip(main_co, clobber_co) == 1, "rbx, rsi, rdi, r12..r15, xmm6 and xmm15 survive a switch");

    deep_co = p4_coro_create(stacks[RING + 1], STACK, deep_entry);
    p4_coro_switch(main_co, deep_co);
    check(deep_result == 300 + 7, "deep recursion and a 64 KB frame on a coroutine stack");

    NT_TIB *t = (NT_TIB *)NtCurrentTeb();
    void *base0 = t->StackBase, *limit0 = t->StackLimit, *dealloc0 = *(void **)((char *)t + 0x1478);
    tib_co = p4_coro_create(stacks[RING + 2], STACK, tib_entry);
    p4_coro_switch(main_co, tib_co);
    check(tib_limit == stacks[RING + 2] && tib_dealloc == stacks[RING + 2] && (char *)tib_base == (char *)stacks[RING + 2] + STACK,
          "the TIB describes the coroutine's stack while it runs");
    check(t->StackBase == base0 && t->StackLimit == limit0 && *(void **)((char *)t + 0x1478) == dealloc0, "the thread's own TIB is back");

    void *h = AddVectoredExceptionHandler(1, veh);
    av_co = p4_coro_create(stacks[RING + 3], STACK, av_entry);
    p4_coro_switch(main_co, av_co);
    RemoveVectoredExceptionHandler(h);
    check(veh_hits == 1 && (char *)veh_fault_sp >= (char *)stacks[RING + 3] && (char *)veh_fault_sp < (char *)stacks[RING + 3] + STACK,
          "an access violation on a coroutine stack reaches the vectored handler");

    printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
