/* FldNoiseEmitPlain (retail 0x00185370): FRAME_1 = base, restoring the frame write mask. The retail body appends the register write to a GIF packet; the port sets the GL mask (src/render/p4gl_mdleffect.c). */
extern void p4gl_mdleffect_frame_mask(u32 orMask);

void FldNoiseEmitPlain(void)
{
    p4gl_mdleffect_frame_mask(0);
}
