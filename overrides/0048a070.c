/* mdlEffectEmitValuePacket (retail 0x0048a070): CLAMP_1 = arg0. The retail body appends the register write to a GIF packet; the port records the clamp override that texture binds follow (src/render/p4gl_mdleffect.c). */
extern void p4gl_mdleffect_clamp_set(s64 clamp);

void mdlEffectEmitValuePacket(u64 arg0)
{
    p4gl_mdleffect_clamp_set((s64)arg0);
}
