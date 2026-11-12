/* func_0040fcd0 (retail 0x0040fcd0): RpSkyTexCacheRasterLock(raster, flag). Retail manages the GS texture-memory ring at 0x0070C2xx and writes the raster's TBP0/TBP1 words, reading the sky2 raster's private layout (the word at the raster pointer is an offset from the sky raster base). The port has no GS texture memory and its rasters (src/render/p4gl_raster.c) do not have that layout, so the ring is never opened and there is nothing to lock: report success. PortMovie: the game's movie task brackets its frame upload with this call (movieNativeTaskUpdate). The retail body is in the git history of this file. */

s32 func_0040fcd0(u8 *arg0, s32 arg1)
{
    (void)arg0;
    (void)arg1;
    return 1;
}
