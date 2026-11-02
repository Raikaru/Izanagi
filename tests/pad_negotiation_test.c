/* Exercise the actual libpad request implementation, without SDL/devices.
 * Retail trace: mode g1 -> stable g10; align g11 -> complete g13;
 * sdkPadUpdateRecord therefore first calls scePadRead at g14. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "p4pc/guest.h"

static _Alignas(16) uint8_t memory[0x00764294u];
uint8_t *p4_guest_base = memory;
void *p4_g2h_slow(uint32_t address)
{
    (void)address;
    assert(!"unexpected hardware window in pad negotiation");
    return NULL;
}

extern int32_t func_0042fb50(int32_t, int32_t, uint8_t *);
extern int32_t func_00430018(int32_t, int32_t);
extern int32_t func_00430578(int32_t, int32_t, int32_t, int32_t);
extern int32_t func_00430210(int32_t, int32_t, int32_t, int32_t);
extern int32_t func_004306f8(int32_t, int32_t, void *);
extern int32_t func_00430180(int32_t, int32_t, uint16_t);

static void frame(uint32_t value)
{
    *(uint32_t *)(memory + 0x00764290u) = value;
}

int main(void)
{
    uint8_t align[6] = {0, 1, 0xff, 0xff, 0xff, 0xff};
    assert(func_00430018(0, 0) == 0);
    assert(func_0042fb50(0, 0, NULL) == 1);
    assert(func_00430018(0, 0) == 6);
    assert(func_00430018(1, 0) == 0);
    assert(func_00430578(1, 0, 1, 3) == 0);
    frame(1);
    assert(func_00430578(0, 0, 1, 3) == 1);
    for (uint32_t n = 2; n <= 9; n++) {
        frame(n);
        assert(func_00430018(0, 0) == 5);
        assert(func_00430018(0, 0) == 5); /* reads do not advance requests */
        assert(func_00430180(0, 0, 4) == 2);
        assert(func_00430578(0, 0, 1, 3) == 0);
    }
    frame(10);
    assert(func_00430018(0, 0) == 6);
    frame(11);
    assert(func_00430210(0, 0, -1, 0) == 2);
    assert(func_004306f8(0, 0, align) == 1);
    frame(12);
    assert(func_00430018(0, 0) == 5);
    assert(func_00430180(0, 0, 4) == 2);
    frame(13);
    assert(func_00430018(0, 0) == 6);
    assert(func_00430180(0, 0, 4) == 0);
    frame(14);
    assert(func_00430018(0, 0) == 6);
    frame(0xfffffffcu);
    assert(func_00430578(0, 0, 0, 0) == 1);
    frame(4);
    assert(func_00430018(0, 0) == 5);
    frame(5);
    assert(func_00430018(0, 0) == 6);
    puts("libpad boot: mode g1->g10, align g11->g13, frame-keyed completion PASS");
    return 0;
}
