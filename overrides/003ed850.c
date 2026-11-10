/* Retail 003ed850 (basync.c, RwFrame hierarchy sync): walks a frame, its siblings (link at +0x9c) and
 * their children (first child at +0x98), recomputing the cached world matrix (+0x50) as
 * modelling matrix (+0x10) times the parent's world matrix (parent at +4) for every frame whose dirty
 * flag (bit 2 of the byte at +3, or in `flags` inherited from an ancestor) is set, and clearing the
 * frame's own dirty bit. */
#include "p4pc/ps2_low_ram.h"

/* The world matrix of a parent; a frame without a parent multiplies by the zero matrix at PS2 address 0x50, as retail does (include/p4pc/ps2_low_ram.h). */
#define P4_PARENT_WORLD(f) (*(unsigned char **)((f) + 4) != 0 ? *(unsigned char **)((f) + 4) + 0x50 : p4_ps2_low_ram_ptr(0x50))

extern void p4_matrix_mul(void *dst, const void *a, const void *b) __asm__("func_003e05f0");

void func_003ed850(unsigned char *frame, unsigned int flags)
{
    for (; frame != 0; frame = *(unsigned char **)(frame + 0x9c)) {
        unsigned int f = flags | frame[3];
        unsigned char *child;

        if (f & 4) {
            p4_matrix_mul(frame + 0x50, frame + 0x10, P4_PARENT_WORLD(frame));
            frame[3] &= 0xfb;
        }
        for (child = *(unsigned char **)(frame + 0x98); child != 0; child = *(unsigned char **)(child + 0x9c)) {
            unsigned int cf = f | child[3];

            if (cf & 4) {
                p4_matrix_mul(child + 0x50, child + 0x10, P4_PARENT_WORLD(child));
                child[3] &= 0xfb;
            }
            func_003ed850(*(unsigned char **)(child + 0x98), cf);
        }
    }
}
