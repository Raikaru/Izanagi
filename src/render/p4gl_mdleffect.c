/* The mdlEffect GS register emitters: P4's own packet writers, not RenderWare's. Each retail body
 * opens a two-quadword GIF packet (func_003f3eb0) and appends one A+D write to the packet pointer at
 * iGpffffb884 (a 0x1000000000008001 tag, then register and value): FRAME_1 (0x4c) to change the
 * frame-buffer write mask, or CLAMP_1 (0x08). The port has no GIF packet, so the overrides
 * (overrides/00489f80.c, 0048a000.c, 0048a070.c, 0048a0e0.c, 001852f0.c, 00185370.c) call these,
 * which set the GL state (src/render/p4gl_state.c) that every draw path reads. Names, addresses
 * and meaning, from the decomp and sky2hook's address list (src/pipelines mdlEffectDMAOpen*):
 *   0x00489f80 mdlEffectEmitMaskedTagPair  FRAME_1 = base | FBMSK 0x00FFFFFF: alpha-only mask
 *   0x0048a000 mdlEffectEmitTagPair        FRAME_1 = base: restores the frame mask
 *   0x0048a070 mdlEffectEmitValuePacket    CLAMP_1 = arg0 (the blur filter's region clamp)
 *   0x0048a0e0 mdlEffectEmitSlotPacket     CLAMP_1 = iGpffffb8e8, the saved clamp: drops the override
 *   0x001852f0 FldNoiseEmitTagged          FRAME_1 = base | FBMSK 0xFF000000: colour-only mask
 *   0x00185370 FldNoiseEmitPlain           FRAME_1 = base: restores the frame mask
 * The base is iGpffffb8c8, the current FRAME_1 value; its FBMSK is the high 32 bits. */
#include <stdint.h>

#include "p4gl.h"
#include "p4pc/gmem.h"

#define iGpffffb8c8 P4_GPDATA(uint64_t, 0xb8c8)

/* FRAME_1 = base with extra FBMSK bits set. */
void p4gl_mdleffect_frame_mask(uint32_t orMask)
{
    p4gl_frame_mask_set((uint32_t)(iGpffffb8c8 >> 32) | orMask);
}

void p4gl_mdleffect_clamp_set(int64_t clamp)
{
    p4gl_gs_clamp_set((uint64_t)clamp);
}

void p4gl_mdleffect_clamp_clear(void)
{
    p4gl_gs_clamp_clear();
}
