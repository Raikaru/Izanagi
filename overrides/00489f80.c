/* mdlEffectEmitMaskedTagPair (retail 0x00489f80): FRAME_1 = base | FBMSK 0x00FFFFFF, the alpha-only write mask. The retail body appends the register write to a GIF packet; the port sets the GL mask (src/render/p4gl_mdleffect.c). */
extern void p4gl_mdleffect_frame_mask(u32 orMask);

void mdlEffectEmitMaskedTagPair(void)
{
    p4gl_mdleffect_frame_mask(0x00FFFFFF);
}
