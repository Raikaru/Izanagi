/* func_003ed960 (rw/basync, 248 bytes): frame hierarchy update. Flags byte at +3.
 * If bit 4 is set, the local matrix (+16, 64 bytes) is copied to the world matrix
 * (+80). For each child at +152 (linked through +156): if (flags | child flags)
 * has bit 4, child world = RwMatrixMultiply(child local, parent world) and the
 * child's bit 4 is cleared. Then func_003ed850 recurses into the child's children
 * with the combined flags. Finally this frame's flags are set to flags & ~5 (retail: `li v1,-6; and`:
 * bits 0 and 2, rwFRAMEPRIVATEHIERARCHYSYNCLTM and ...SYNCOBJ). A child with no parent reads the world matrix of the parent from PS2 low RAM (retail: parent = 0, world matrix at 0x50, all zero). */

#include "p4pc/ps2_low_ram.h"

extern void func_003e05f0(u8 *dst, u8 *a, u8 *b);
extern void func_003ed850(u8 *node, s32 flags);

void func_003ed960(u8 *arg0)
{
    u8 flags;
    u8 childFlags;
    u8 *child;
    s32 i;

    flags = arg0[3];
    if (flags & 4) {
        for (i = 0; i < 16; i++) {
            ((u32 *)(arg0 + 80))[i] = ((u32 *)(arg0 + 16))[i];
        }
    }

    child = *(u8 **)(arg0 + 152);
    while (child != NULL) {
        childFlags = flags | child[3];
        if (childFlags & 4) {
            {
                u8 *parent = *(u8 **)(child + 4);

                /* a null parent: retail multiplies by the zero matrix at PS2 address 0x50 (include/p4pc/ps2_low_ram.h) */
                func_003e05f0(child + 80, child + 16, parent != NULL ? parent + 80 : p4_ps2_low_ram_ptr(0x50));
            }
            child[3] = child[3] & ~4;
        }
        func_003ed850(*(u8 **)(child + 152), childFlags);
        child = *(u8 **)(child + 156);
    }

    arg0[3] = flags & ~5;
}
