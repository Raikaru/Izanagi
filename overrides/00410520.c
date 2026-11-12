/* RwIm3DRenderIndexedPrimitive (retail 0x00410520). Same stash writes and index rounding as
 * retail (line list: numIndices - numIndices % 2; triangle list: numIndices - numIndices % 3;
 * the other types unchanged). The retail RxPipelineExecute of the per-type terminal pipeline is
 * replaced by p4gl_im3d_draw (src/render/p4gl_im3d.c), which draws the vertices that
 * RwIm3DTransform copied. A count of zero or less after rounding draws nothing and succeeds.
 * Returns TRUE on success, FALSE with error 35 (E_RX_IM3DNOTACTIVE, no Transform pending), error
 * 37 (E_RX_INVALIDPRIMTYPE) or an index that is not below the transformed vertex count. */
extern int p4gl_im3d_draw(RwInt32 primType, const RwUInt16 *indices, RwInt32 numIndices);

RwBool
RwIm3DRenderIndexedPrimitive(RwPrimitiveType primType,
                             RwImVertexIndex * indices,
                             RwInt32 numIndices)
{
    RwBool              im3dactive;

    RWAPIFUNCTION(RWSTRING("RwIm3DRenderIndexedPrimitive"));

    RWASSERT(indices);
    im3dactive = (RWIMMEDIGLOBAL(curPool).elements != NULL);

    if (im3dactive)
    {
        _rwIm3DPoolStash    *stash = &(RWIMMEDIGLOBAL(curPool).stash);

        stash->pipeline = (RxPipeline *) NULL;
        stash->primType = (RwPrimitiveType) primType;
        stash->indices = (RxVertexIndex *) indices;
        stash->numIndices = (RwUInt32) numIndices;

        switch (primType)
        {
            case rwPRIMTYPETRILIST:
                stash->numIndices = numIndices - (numIndices % 3);
                break;
            case rwPRIMTYPETRIFAN:
            case rwPRIMTYPETRISTRIP:
            case rwPRIMTYPEPOLYLINE:
                break;
            case rwPRIMTYPELINELIST:
                stash->numIndices = numIndices - (numIndices % 2);
                break;
            default:
                RWERROR((E_RX_INVALIDPRIMTYPE, primType));
                RWRETURN(FALSE);
        }

        if (p4gl_im3d_draw(primType, (const RwUInt16 *) indices, (RwInt32) stash->numIndices))
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
