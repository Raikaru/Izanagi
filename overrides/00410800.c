/* RwIm3DRenderTriangle (retail 0x00410800): a one-triangle triangle list over the vertices of the
 * pending Transform. The retail body took its three indices from the RxHeap; the port keeps them
 * on the stack, so no heap block exists and the stash index pointer is cleared before return.
 * The draw is p4gl_im3d_draw (src/render/p4gl_im3d.c). Returns TRUE on success, FALSE with error
 * 35 (E_RX_IM3DNOTACTIVE) or an index that is not below the transformed vertex count. */
extern int p4gl_im3d_draw(RwInt32 primType, const RwUInt16 *indices, RwInt32 numIndices);

RwBool
RwIm3DRenderTriangle(RwInt32 vert1, RwInt32 vert2, RwInt32 vert3)
{
    RwBool              im3dactive;

    RWAPIFUNCTION(RWSTRING("RwIm3DRenderTriangle"));

    im3dactive = (RWIMMEDIGLOBAL(curPool).elements != NULL);

    if (im3dactive)
    {
        _rwIm3DPoolStash    *stash = &RWIMMEDIGLOBAL(curPool).stash;
        RxVertexIndex       indices[3];
        RwBool              drawn;

        indices[0] = (RxVertexIndex) vert1;
        indices[1] = (RxVertexIndex) vert2;
        indices[2] = (RxVertexIndex) vert3;

        stash->pipeline = (RxPipeline *) NULL;
        stash->primType = (RwPrimitiveType) rwPRIMTYPETRILIST;
        stash->indices = indices;
        stash->numIndices = (RwUInt32) 3;

        drawn = p4gl_im3d_draw(rwPRIMTYPETRILIST, (const RwUInt16 *) indices, 3);
        stash->indices = (RxVertexIndex *) NULL;
        if (drawn)
        {
            RWRETURN(TRUE);
        }
    }
    else
    {
        RWERROR((E_RX_IM3DNOTACTIVE));
    }

    RWRETURN(FALSE);
}
