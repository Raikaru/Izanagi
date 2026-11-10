/* Drives the production timed RAM dump (src/platform/state_dump.c) for flips 1..3; tests/field_entry_test.py checks that
 * P4PC_DUMP_RAM_MAX_MB stops the run before the dump that would pass it. */
#include <stdint.h>
#include <stdlib.h>
#include "p4pc/gmem.h"

static uint8_t memory[P4_GUEST_SIZE];
uint8_t *p4_guest_base = memory;
void *p4_g2h_slow(uint32_t address) { (void)address; abort(); }
uint64_t p4_os_now_ns(void) { return 0; }
int64_t calGetDayEntryByPhase(int32_t day, uint32_t phase) { (void)day; (void)phase; return 0; }
extern void p4pc_state_dump_flip(unsigned flip);

int main(void)
{
    for (unsigned flip = 1; flip <= 3; flip++)
        p4pc_state_dump_flip(flip);
    return 0;
}
