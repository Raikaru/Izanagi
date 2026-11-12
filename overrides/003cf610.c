/* PS2AllNodeBody (retail 0x003cf610, the sky2 DMA draw). The port draws the atomic with OpenGL
 * (src/render/p4gl_atomic.c). packet is {privateData, 0, atomic}, built by func_003cf140. */
extern s32 p4gl_atomic_draw(void *atomic);

s32 func_003cf610(void *packet, void *unused, s32 nI)
{
    (void)unused;
    (void)nI;
    return p4gl_atomic_draw(((void **)packet)[2]);
}
