/* RwIm3DRenderLine (retail 0x00410930): a one-line line list over the vertices of the pending
 * Transform. As in RwIm3DRenderTriangle, the two indices are on the stack instead of the RxHeap.
 * The draw is p4gl_im3d_draw (src/render/p4gl_im3d.c). Returns TRUE on success, FALSE with error
 * 35 (E_RX_IM3DNOTACTIVE) or an index that is not below the transformed vertex count. */
extern int p4gl_im3d_draw(RwInt32 primType, const RwUInt16 *indices, RwInt32 numIndices);

RwBool
RwIm3DRenderLine(RwInt32 vert1, RwInt32 vert2)
{
    RwBool              im3dactive;

    RWAPIFUNCTION(RWSTRING("RwIm3DRenderLine"));

    im3dactive = (RWIMMEDIGLOBAL(curPool).elements != NULL);

    if (im3dactive)
    {
        _rwIm3DPoolStash    *stash = &RWIMMEDIGLOBAL(curPool).stash;
        RxVertexIndex       indices[2];
        RwBool              drawn;

        indices[0] = (RxVertexIndex) vert1;
        indices[1] = (RxVertexIndex) vert2;

        stash->pipeline = (RxPipeline *) NULL;
        stash->primType = (RwPrimitiveType) rwPRIMTYPELINELIST;
        stash->indices = indices;
        stash->numIndices = (RwUInt32) 2;

        drawn = p4gl_im3d_draw(rwPRIMTYPELINELIST, (const RwUInt16 *) indices, 2);
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
