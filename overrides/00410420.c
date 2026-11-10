/* RwIm3DTransform (retail 0x00410420). Same state writes as retail: the vertex pool, stride and
 * the stash (matrix and flags) of the Im3D state block. The retail RxPipelineExecute of the
 * PS2Im3DFastTransform pipeline (VU1 transform, DMA) is replaced by p4gl_im3d_transform, which
 * copies the vertices and moves them to clip space under the open camera
 * (src/render/p4gl_im3d.c). numElements is the 16-bit count the retail node reads, so exactly
 * 65536 vertices is a zero-vertex pool, as on retail. Returns the vertex array on success, NULL
 * on error 50 (E_RX_TOOMANYVERTS, more than 65536 vertices) or when no camera update is open. */
extern int p4gl_im3d_transform(const void *verts, RwUInt32 numVerts, const void *ltm, RwUInt32 flags);

void               *
RwIm3DTransform(RwIm3DVertex * pVerts,
                RwUInt32 numVerts, RwMatrix * ltm, RwUInt32 flags)
{
    RWAPIFUNCTION(RWSTRING("RwIm3DTransform"));

    RWASSERT(0 < _rwIm3DModule.numInstances);
    RWASSERT(NULL != pVerts);

    if (numVerts > 65536L)
    {
        RWERROR((E_RX_TOOMANYVERTS));
    }
    else
    {
        RWIMMEDIGLOBAL(curPool).numElements = (RwUInt16) numVerts;
        RWIMMEDIGLOBAL(curPool).elements = pVerts;
        RWIMMEDIGLOBAL(curPool).stride = sizeof(RxObjSpace3DLitVertex);
        RWIMMEDIGLOBAL(curPool).stash.ltm = ltm;
        RWIMMEDIGLOBAL(curPool).stash.flags =
            (flags | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);

        if (p4gl_im3d_transform(pVerts, RWIMMEDIGLOBAL(curPool).numElements, ltm,
                                RWIMMEDIGLOBAL(curPool).stash.flags))
        {
            RWRETURN(pVerts);
        }
    }

    RwDebugSendMessage(rwDEBUGMESSAGE, "RwIm3DTransform",
                       "Pipeline execution failed");

    RWRETURN(NULL);
}
