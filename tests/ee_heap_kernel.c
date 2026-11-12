/* Kernel-register fixture for isolated CRT tests, not an allocator replacement.
 * EE DIntr/EIntr modify EIE (SR16); QueryIntrContext reads inverted IE (SR0).
 * The game links its real kernel primitives and matched QueryIntrContext body. */
#include <stdint.h>

uint32_t ee_heap_test_sr = 0x00010001u;

uint32_t QueryIntrContext(void)
{
    return (ee_heap_test_sr ^ 1u) & 1u;
}

int32_t func_0042ba20(void)
{
    int32_t previous = (ee_heap_test_sr & 0x10000u) != 0;
    ee_heap_test_sr &= ~0x10000u;
    return previous;
}

int32_t func_0042ba70(void)
{
    int32_t previous = (ee_heap_test_sr & 0x10000u) != 0;
    ee_heap_test_sr |= 0x10000u;
    return previous;
}
