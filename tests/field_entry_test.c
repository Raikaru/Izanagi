/* Sparse guest memory exercises the production semantic sampler, not a text pin. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/gmem.h"

static uint8_t memory[P4_GUEST_SIZE];
uint8_t *p4_guest_base = memory;
void *p4_g2h_slow(uint32_t address) { (void)address; abort(); }
uint64_t p4_os_now_ns(void) { return 0; }
int64_t calGetDayEntryByPhase(int32_t day, uint32_t phase)
{
    assert(day == 9 && phase == 5);
    return 2;
}
extern void p4pc_state_dump_flip(unsigned flip);
extern void p4pc_state_dump_request(unsigned flip);

int main(int argc, char **argv)
{
    assert(argc == 2);
    setenv("P4PC_FIELD_ENTRY_DUMP", argv[1], 1);
    char dump_prefix[512];
    snprintf(dump_prefix, sizeof dump_prefix, "%s.state", argv[1]);
    setenv("P4PC_DUMP_RAM", dump_prefix, 1);
    unsetenv("P4PC_DUMP_RAM_AT");
    unsetenv("P4PC_EVENT_LOG");
    unsetenv("P4PC_FPU_LOG");
    P4_G32(0x007d2510) = 0x00200000;
    P4_G32(0x00200038) = 0x00200100;
    P4_G32(0x00200100) = 11;
    P4_G32(0x00200118) = 7 | (2 << 16);
    P4_G32(0x00762ea0) = 0x00200200;
    P4_G32(0x00200200) = 7;
    P4_G32(0x00200204) = 2;
    P4_G32(0x00200208) = 0x00200300;
    P4_G32(0x0020020c) = 7;
    P4_G32(0x00200210) = 3 | (4 << 16);
    P4_G32(0x00764b70) = 0x00200400;
    P4_G32(0x00200480) = 0x3f000000;
    P4_G32(0x00200484) = 0x45bb8000;
    P4_G32(0x00200488) = 0x45600000;
    P4_G32(0x00797b78) = 9 << 16;
    P4_G32(0x00797b7c) = 5;
    P4_G32(0x007642cc) = 0x00200500;
    P4_G32(0x00200500) = 2;
    P4_G32(0x00200504) = 9 | (5 << 16);
    P4_G32(0x00200508) = 2 | (1 << 16);
    P4_G32(0x007643a8) = 2;
    for (unsigned i = 0; i < 8; i++) P4_G32(0x00764b3c + i * 4) = 0xa0 + i;
    P4_G32(0x00764b5c) = 0x45bb8000;
    for (unsigned i = 0; i < 0xb0; i++) P4_G32(0x0079b1cc + i * 4) = 0xabc00000 + i;
    P4_G32(0x00200304) = 0x00201000;
    P4_G32(0x00201000) = 0x400;
    P4_G32(0x00201138) = 0x00201200;
    P4_G32(0x00201200) = 0x401;
    P4_G32(0x00200330) = 0x00202000;
    P4_G32(0x00202000) = 0x3000;
    for (unsigned i = 0; i < 24; i++) P4_G32(0x00202140 + i * 4) = 0x3f800000 + i;
    P4_G32(0x00764290) = 10;
    p4pc_state_dump_flip(1); /* Still loading/fading. */
    P4_G32(0x00200100) = 12;
    P4_G32(0x0020020c) = 2;
    p4pc_state_dump_flip(2); /* Pending primary request. */
    P4_G32(0x0020020c) = 4;
    p4pc_state_dump_flip(3); /* Pending environment request. */
    P4_G32(0x0020020c) = 0;
    p4pc_state_dump_flip(4); /* Cold load: no reuse bit, real ready entry. */
    P4_G32(0x0020020c) = 1;
    p4pc_state_dump_flip(5); /* Same visit must not repeat. */
    P4_G32(0x00200100) = 11;
    p4pc_state_dump_flip(6);
    P4_G32(0x00200100) = 13;
    P4_G32(0x00764290) = 20;
    p4pc_state_dump_flip(7); /* Reused scene: second visit to the same field/room. */
    P4_G32(0x00200118) = 7 | (3 << 16);
    P4_G32(0x00200204) = 3;
    P4_G32(0x0020020c) = 0;
    P4_G32(0x00764290) = 30;
    p4pc_state_dump_flip(8); /* New requested room without an observed busy flip. */
    P4_G32(0x00200100) = 14;
    P4_G32(0x00200118) = 22 | (2 << 16);
    P4_G32(0x00200200) = 22;
    P4_G32(0x00200204) = 2;
    P4_G32(0x00764290) = 40;
    p4pc_state_dump_flip(9); /* Event root is ready too. */
    P4_G32(0x0079b1cc + 175 * 4) = 0xabc0cafe;
    P4_G32(0x00201200) = 0x402;
    P4_G32(0x00764290) = 41;
    P4_G32(0x00200100) = 5;
    P4_G32(0x0020020c) = 2;
    p4pc_state_dump_request(10);
    p4pc_state_dump_flip(10); /* Capture the live compositor scene while its root is loading. */
    P4_G32(0x00200100) = 12;
    P4_G32(0x0020020c) = 0;
    P4_G32(0x00764290) = 42;
    p4pc_state_dump_flip(11); /* A forced capture must not suppress the next ready entry. */
    P4_G32(0x00200100) = 17;
    P4_G32(0x00200118) = 7 | (4 << 16);
    P4_G32(0x00200200) = 7;
    P4_G32(0x00200204) = 4;
    p4pc_state_dump_flip(12); /* Teardown/transition is not a new entry. */
    return 0;
}
