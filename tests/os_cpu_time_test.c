#include "p4pc/os.h"
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    p4_os_init();
    uint64_t before, worked, after_sleep;
    if (p4_os_thread_cpu_ns(NULL) != -1 || p4_os_thread_cpu_ns(&before)) return 1;
    uint64_t resolution;
    if (p4_os_thread_cpu_resolution_ns(NULL) != -1 ||
        p4_os_thread_cpu_resolution_ns(&resolution) || resolution == 0)
        return 6;
    volatile uint32_t value = 7;
    uint64_t deadline = p4_os_now_ns() + UINT64_C(3000000000);
    do {
        for (unsigned i = 0; i < 65536; i++) value = value * 1664525u + 1013904223u;
        if (p4_os_thread_cpu_ns(&worked) || worked < before) return 2;
        if (p4_os_now_ns() > deadline) return 3;
    } while (worked - before < UINT64_C(20000000));
    uint64_t sleep_start = p4_os_now_ns();
    p4_os_sleep_ns(UINT64_C(100000000));
    uint64_t slept = p4_os_now_ns() - sleep_start;
    if (p4_os_thread_cpu_ns(&after_sleep) || after_sleep < worked) return 4;
    /* Win7-compatible GetThreadTimes can be scheduler-tick quantized. Even
     * allowing one coarse tick, sleeping must not be charged as interpreter CPU. */
    if (after_sleep - worked > slept / 2 + UINT64_C(25000000)) return 5;
    printf("os_cpu_time: PASS (resolution=%llu ns, work=%llu ns, sleep wall=%llu ns, sleep CPU=%llu ns, value=%u)\n",
           (unsigned long long)resolution, (unsigned long long)(worked - before),
           (unsigned long long)slept, (unsigned long long)(after_sleep - worked), (unsigned)value);
    return 0;
}
