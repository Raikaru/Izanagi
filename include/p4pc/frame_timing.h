/* Optional application observer of the scheduler's actual frame boundary.
 * Timestamps are paced scheduler nanoseconds (virtual time in headless runs),
 * not renderer elapsed time. Installation primes the observer's baseline. */
#ifndef P4PC_FRAME_TIMING_H
#define P4PC_FRAME_TIMING_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void p4pc_frame_timing_install(void (*publish)(uint64_t scheduler_ns));
#ifdef __cplusplus
}
#endif
#endif
