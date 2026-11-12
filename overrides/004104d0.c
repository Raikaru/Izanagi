/* RwIm3DEnd (retail 0x004104D0). Retail body, plus dropping the port's transformed vertex copy
 * (src/render/p4gl_im3d.c). */
extern void p4gl_im3d_end(void);

RwBool
RwIm3DEnd(void)
{
    RwBool              im3dactive;

    RWAPIFUNCTION(RWSTRING("RwIm3DEnd"));

    im3dactive = (RWIMMEDIGLOBAL(curPool).elements != NULL);

    if (!im3dactive)
    {
        RWRETURN(FALSE);
    }

    memset(&RWIMMEDIGLOBAL(curPool), 0, sizeof(rwIm3DPool));
    p4gl_im3d_end();

    RWRETURN(TRUE);
}
