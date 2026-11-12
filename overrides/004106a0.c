/* RwIm3DRenderPrimitive (retail 0x004106A0). Same stash writes as retail (numIndices is the
 * vertex count of the pending Transform). The retail RxPipelineExecute of the per-type terminal
 * pipeline is replaced by p4gl_im3d_draw (src/render/p4gl_im3d.c), which draws the transformed
 * vertices in order. Returns TRUE on success, FALSE with error 35 (E_RX_IM3DNOTACTIVE, no
 * Transform pending) or error 37 (E_RX_INVALIDPRIMTYPE). */
extern int p4gl_im3d_draw(RwInt32 primType, const RwUInt16 *indices, RwInt32 numIndices);

RwBool
RwIm3DRenderPrimitive(RwPrimitiveType primType)
{
    RwBool              im3dactive;

    RWAPIFUNCTION(RWSTRING("RwIm3DRenderPrimitive"));

    im3dactive = (RWIMMEDIGLOBAL(curPool).elements != NULL);

    if (im3dactive)
    {
        _rwIm3DPoolStash    *stash = &(RWIMMEDIGLOBAL(curPool).stash);

        stash->pipeline = (RxPipeline *) NULL;
        stash->primType = (RwPrimitiveType) primType;
        stash->indices = (RxVertexIndex *) NULL;
        stash->numIndices = (RwUInt32) RWIMMEDIGLOBAL(curPool).numElements;

        switch (primType)
        {
            case rwPRIMTYPETRILIST:
            case rwPRIMTYPETRIFAN:
            case rwPRIMTYPETRISTRIP:
            case rwPRIMTYPELINELIST:
            case rwPRIMTYPEPOLYLINE:
                break;
            default:
                RWERROR((E_RX_INVALIDPRIMTYPE, primType));
                RWRETURN(FALSE);
        }

        if (p4gl_im3d_draw(primType, NULL, 0))
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
