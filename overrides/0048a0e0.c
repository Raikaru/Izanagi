/* mdlEffectEmitSlotPacket (retail 0x0048a0e0): CLAMP_1 = iGpffffb8e8, the saved clamp. The retail body appends the register write to a GIF packet; the port drops the clamp override so texture binds follow the render state again (src/render/p4gl_mdleffect.c). */
extern void p4gl_mdleffect_clamp_clear(void);

void mdlEffectEmitSlotPacket(void)
{
    p4gl_mdleffect_clamp_clear();
}
