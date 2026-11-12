/* PS2PTank nodeBody (retail 0x003a8840). The retail body opens a GIF packet (func_003f3eb0), sends
 * the PTank's instanced DMA chunks and the GS register packet, and VU1 expands the particles. The
 * port draws the particle arrays of the atomic's PTank record with OpenGL (src/render/p4gl_ptank.c).
 * node is the RxPipelineNode; params is the RxPipelineNodeParam, whose dataParam (+0) is the RpAtomic.
 * Returns 1 when the particles were drawn, 0 when the tank was skipped or failed. */
extern s32 p4gl_ptank_draw(void *atomic);

s32 func_003a8840(void *node, void *params)
{
    (void)node;
    return p4gl_ptank_draw(*(void **)params);
}
