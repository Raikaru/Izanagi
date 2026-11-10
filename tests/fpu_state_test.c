/* The EE float mode survives a call that changes the thread's float state (src/platform/state_dump.c, p4pc_fpu_check): the NVIDIA OpenGL driver on
 * Windows sets the x87 control word to round toward zero in its first call (0x0F7F instead of 0x037F), which made every nearest division and square root of
 * the game truncate. The test sets the EE mode, spoils the x87 word and MXCSR the way a driver does, and requires the check to notice and restore both. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h>

#include "p4pc/hostmath.h"
#include "p4pc/os.h"

static unsigned short x87_cw(void)
{
    unsigned short cw;

    __asm__ volatile("fnstcw %0" : "=m"(cw));
    return cw;
}

static void set_x87_cw(unsigned short cw)
{
    __asm__ volatile("fldcw %0" : : "m"(cw));
}

int main(void)
{
    int bad = 0;

    if (!p4pc_fpu_ee_mode()) {
        fprintf(stderr, "fpu_state: no EE float mode on this target\n");
        return 77;
    }
    if (x87_cw() != 0x037F || (_mm_getcsr() & P4PC_MXCSR_EE) != P4PC_MXCSR_EE) {
        printf("fpu_state: EE mode not set: x87 %04x mxcsr %08x\n", x87_cw(), _mm_getcsr());
        bad = 1;
    }
    p4pc_fpu_check("an unchanged state");
    if (x87_cw() != 0x037F) {
        printf("fpu_state: an unchanged state was touched: x87 %04x\n", x87_cw());
        bad = 1;
    }
    set_x87_cw(0x0F7F);
    p4pc_fpu_check("a driver call (x87 round toward zero)");
    if (x87_cw() != 0x037F) {
        printf("fpu_state: x87 word not restored: %04x\n", x87_cw());
        bad = 1;
    }
    set_x87_cw(0x027F);
    p4pc_fpu_check("a runtime start-up (x87 53-bit precision)");
    if (x87_cw() != 0x037F) {
        printf("fpu_state: x87 precision not restored: %04x\n", x87_cw());
        bad = 1;
    }
    _mm_setcsr(_mm_getcsr() & ~P4PC_MXCSR_EE);
    p4pc_fpu_check("a call that reset MXCSR");
    if ((_mm_getcsr() & P4PC_MXCSR_EE) != P4PC_MXCSR_EE) {
        printf("fpu_state: MXCSR not restored: %08x\n", _mm_getcsr());
        bad = 1;
    }
    if (!bad) {
        printf("fpu_state: x87 and MXCSR restored after each change\n");
    }
    return bad;
}
