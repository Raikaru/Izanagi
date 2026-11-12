/* rpPTankSkyCreateCallBack (retail 0x003a3e90), the create callback of the sky platform:
 * func_003a3e90(atomic, pd, maxP, dataFlags, platFlags) returns 1 when the tank is created.
 *
 * The retail body has three branches by the tank's data flags: the instancing ones pick the rpPTankSkyIns*
 * setups (0x3a5280, 0x3a72a0, 0x3a6fb0, 0x3a8340, 0x3aa0b0, 0x3b4470 ...) that build VU1 input from the
 * particle arrays, and one (user data +0xD8 set) makes the atomic a geometry holder without an instancer.
 * The port draws the particle arrays directly (src/render/p4gl_ptank.c) from the render callback
 * (func_003a2290), so every tank takes the last shape: an empty geometry (bounding sphere at the origin,
 * radius 1) with one fresh material, which is what the callers' texture and colour writes land on, set on
 * the atomic; the callback slots of the record (+0x1C render callback func_003a8640, +0x24..+0x38
 * instancers) are cleared; and the atomic's pipeline (+0x6C) is the PTank module's, read from the module
 * globals at 0x008872E0 + the plugin offset at 0x00764704. */
extern void *p4c_003c2630_003a3e90(s32 a, s32 b, s32 c) __asm__("func_003c2630");
extern void *p4c_003c22f0_003a3e90(void *geometry) __asm__("func_003c22f0");
extern void *p4c_003c0210_003a3e90(void *atomic, void *geometry, s32 flags) __asm__("func_003c0210");
extern s32 p4c_003c2a80_003a3e90(void *geometry) __asm__("func_003c2a80");
extern void *p4c_003c4140_003a3e90(void) __asm__("func_003c4140");
extern void *p4c_003c4a80_003a3e90(void *list, void *material) __asm__("func_003c4a80");
extern s32 p4c_003c4220_003a3e90(void *material) __asm__("func_003c4220");
extern void p4c_003a8640_003a3e90(void) __asm__("func_003a8640");

s32 func_003a3e90(void *atomic, u8 *pd, u32 maxP, u32 dataFlags, u32 platFlags)
{
    u8 *a = (u8 *)atomic;
    u8 *rec;
    u8 *geometry;
    u8 *morph;
    void *material;

    (void)pd;
    (void)maxP;
    (void)dataFlags;
    (void)platFlags;
    geometry = (u8 *)p4c_003c2630_003a3e90(0, 0, 0);
    if (geometry == (u8 *)0) {
        return 0;
    }
    morph = *(u8 **)(geometry + 0x5C);
    *(u32 *)(morph + 4) = 0;
    *(u32 *)(morph + 8) = 0;
    *(u32 *)(morph + 0xC) = 0;
    *(u32 *)(morph + 0x10) = 0x3F800000;
    if (p4c_003c22f0_003a3e90(geometry) == (void *)0) {
        return 0;
    }
    p4c_003c0210_003a3e90(atomic, geometry, 0);
    p4c_003c2a80_003a3e90(geometry);
    material = p4c_003c4140_003a3e90();
    p4c_003c4a80_003a3e90(geometry + 0x20, material);
    p4c_003c4220_003a3e90(material);
    rec = *(u8 **)(a + iGpffffb610);
    *(void **)(rec + 0x1C) = (void *)p4c_003a8640_003a3e90;
    *(u32 *)(rec + 0x24) = 0;
    *(u32 *)(rec + 0x28) = 0;
    *(u32 *)(rec + 0x2C) = 0;
    *(u32 *)(rec + 0x30) = 0;
    *(u32 *)(rec + 0x34) = 0;
    *(u32 *)(rec + 0x38) = 0;
    *(u32 *)(a + 0x6C) = **(u32 **)(0x008872E0 + *(s32 *)0x00764704);
    return 1;
}
