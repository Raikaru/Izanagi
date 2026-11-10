/* Live presentation observes virtual time; observation must not perform the
 * polling-clock advancement that would make a hidden replay diverge. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "p4pc/platform.h"

static int virtual_mode = 1;
static uint64_t wall;
int p4_os_virtual_clock(void) { return virtual_mode; }
uint64_t p4_os_now_ns(void) { return wall; }

int main(void)
{
    uint64_t initial = p4pc_scheduler_time_ns();
    assert(initial == 1000000000ull);
    for (unsigned n = 0; n < 5000; n++)
        assert(p4pc_scheduler_time_ns() == initial);
    virtual_mode = 0;
    wall = 123;
    assert(p4pc_scheduler_time_ns() == initial);
    wall = 456;
    assert(p4pc_scheduler_time_ns() == initial);
    virtual_mode = 1;
    assert(p4pc_scheduler_time_ns() == initial);
    puts("scheduler clock: 5000 observations leave virtual time unchanged PASS");
    return 0;
}
