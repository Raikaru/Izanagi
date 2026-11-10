/* p4_coro_* (include/p4pc/os.h) on i386 without ucontext: Android's bionic has none. The x86_64 and aarch64 switches are in os_posix.c (the 64-bit
 * port's); this is the same scheme for the 32-bit x86 build that runs on the Android x86 emulator. A coroutine is a saved stack pointer; switching
 * pushes the callee-saved registers on the running stack, stores the stack pointer, loads the other one and pops. A new coroutine's stack is built to
 * look like a switch that is about to return into `entry`. The caller's stack memory is the guest's (kernel_thread.c), as with ucontext.
 * Compiled by CMake for Android only (os_posix.c keeps ucontext for Linux i386). */
#if defined(__i386__)
#include <stdint.h>
#include <stdlib.h>

#include "p4pc/os.h"

struct p4_coro {
    void *sp;
};

extern void p4_coro_swap(void **save_sp, void *load_sp);

__asm__(".text\n.globl p4_coro_swap\n.hidden p4_coro_swap\n.type p4_coro_swap,@function\np4_coro_swap:\n"
        "  movl 4(%esp), %eax\n"
        "  movl 8(%esp), %edx\n"
        "  pushl %ebp\n  pushl %ebx\n  pushl %esi\n  pushl %edi\n"
        "  movl %esp, (%eax)\n"
        "  movl %edx, %esp\n"
        "  popl %edi\n  popl %esi\n  popl %ebx\n  popl %ebp\n"
        "  ret\n.size p4_coro_swap,.-p4_coro_swap\n");

p4_coro *p4_coro_create(void *stack, size_t stack_size, void (*entry)(void)) {
    p4_coro *c = calloc(1, sizeof *c);
    uintptr_t top = ((uintptr_t)stack + stack_size) & ~(uintptr_t)15;
    uint32_t *sp = (uint32_t *)(top - 24);

    if (c == NULL) return NULL;
    /* edi, esi, ebx, ebp, then `ret` takes entry and entry finds a null return address: esp is 12 mod 16 on entry (esp + 4 is 16-aligned, as after a
     * call), which bionic's SSE code needs */
    sp[0] = sp[1] = sp[2] = sp[3] = 0;
    sp[4] = (uint32_t)(uintptr_t)entry;
    sp[5] = 0;
    c->sp = sp;
    return c;
}

p4_coro *p4_coro_adopt(void) { return calloc(1, sizeof(p4_coro)); }

void p4_coro_switch(p4_coro *from, p4_coro *to) { p4_coro_swap(&from->sp, to->sp); }

void p4_coro_destroy(p4_coro *c) { free(c); }

#endif
