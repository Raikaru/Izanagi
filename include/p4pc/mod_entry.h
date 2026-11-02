/* Game-unit mod seams: scalar state only, no native descriptor pointers.
 * Kept separate from mod_hooks.h so the guest rewriter never sees host records. */
#ifndef P4PC_MOD_ENTRY_H
#define P4PC_MOD_ENTRY_H
#include <stdint.h>
#include <limits.h>

typedef struct P4ModEntry {
    volatile uint8_t enabled;
    uint8_t restore;
} P4ModEntry;
enum { P4_MOD_INACTIVE, P4_MOD_ACTIVE, P4_MOD_BYPASS };

/* Scheduler seam; no event records/callback pointers enter guest ABI census. */
#define P4_MOD_FRAME_START_BIT 1u
extern uint32_t p4_mod_event_mask;
void p4_mod_event_frame(uint32_t frame);

/* Compile-time ABI checks only. A typedef collision must not silently turn a
 * pointer/aggregate into an integer bridge. No expression is evaluated. */
#define P4_MOD_INTEGER_ABI(value, bytes) \
    (sizeof(value) == (bytes) && \
     (__builtin_classify_type(value) == 1 || __builtin_classify_type(value) == 3))
#define P4_MOD_SIGNED_ABI(value) _Generic((value), char: (CHAR_MIN < 0), \
    signed char: 1, short: 1, int: 1, long: 1, long long: 1, default: 0)
#endif
