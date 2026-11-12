/* Actual kernel_init.c timer seam against a callback-mapped COUNT register.
 * Other platform initialization is discarded by section GC, not stubbed. */
#include "p4pc/guest.h"
#include "p4pc/platform.h"
#include <assert.h>
#include <stdio.h>

uint8_t *p4_guest_base;
static uint32_t count_register;
void *p4_g2h_slow(uint32_t address) {
    assert(address==0x10000000u);
    return &count_register;
}
int main(void) {
    _Static_assert(sizeof(void *)==8,"host witness uses the 64-bit guest window");
    p4pc_timer0_boot_reset(0x14bc);
    assert(p4pc_timer0_boot_count()==0x14bc);
    assert(p4pc_timer0_boot_count()==0x16c3);
    assert(count_register==0x16c3);
    count_register=0; /* game's first-frame reset is still an ordinary MMIO store */
    assert(p4pc_timer0_boot_count()==0);
    assert(p4pc_timer0_boot_count()==0);
    p4pc_timer0_boot_reset(0xff00);
    assert(p4pc_timer0_boot_count()==0xff00);
    assert(p4pc_timer0_boot_count()==0x0107); /* hardware's 16-bit wrap */
    p4pc_timer0_boot_reset(0x10000+0x14bc);
    assert(p4pc_timer0_boot_count()==0x14bc);
    assert(p4pc_timer0_boot_count()==0x16c3);
    puts("TIMER0 boot: retail14bc->16c3, reset and 16-bit wrap PASS");
    return 0;
}
