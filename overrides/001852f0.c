/* FldNoiseEmitTagged (retail 0x001852f0): FRAME_1 = base | FBMSK 0xFF000000, the colour-only write mask. The retail body appends the register write to a GIF packet; the port sets the GL mask (src/render/p4gl_mdleffect.c). */
extern void p4gl_mdleffect_frame_mask(u32 orMask);

void FldNoiseEmitTagged(void)
{
    p4gl_mdleffect_frame_mask(0xFF000000);
}
