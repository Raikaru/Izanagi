/* rpPTankAtomicRenderCB (retail 0x003a2290), the render callback of a PTank atomic. The retail body,
 * when the atomic has active particles, runs the tank's instance callback (rpPTankSkyIns*, which
 * builds the VU1 input from the particle arrays), clears instFlags, runs the render callback and,
 * when that returns TRUE, the default atomic render callback, which executes the PS2PTank pipeline
 * (func_003a8840). The port draws the particle arrays directly (src/render/p4gl_ptank.c), so no
 * instance step has a consumer; instFlags (+0x40) is cleared as retail does after instancing.
 * The record is the pointer at atomic + the PTank plugin offset (iGpffffb610); +0x04 is actPCount. */
extern s32 p4gl_ptank_draw(void *atomic);

void *func_003a2290(void *atomic)
{
    u8 *record = *(u8 **)((u8 *)atomic + iGpffffb610);

    if (*(s32 *)(record + 4) > 0) {
        *(s32 *)(record + 0x40) = 0;
        p4gl_ptank_draw(atomic);
    }
    return atomic;
}
