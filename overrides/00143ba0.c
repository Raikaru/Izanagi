/* gsUploadRasterTiles (retail 0x00143ba0): the movie frame upload. The retail body builds GS texture-upload packets from the frame's macroblock columns; the port writes the frame into the raster's CPU shadow (src/movie/movie_upload.c). */
extern void p4pc_movie_upload(RwRaster *raster, u32 owner, s32 mbw, s32 mbh);

void gsUploadRasterTiles(RwRaster *source, u32 owner, s32 tileIndex, s32 tileCount)
{
    p4pc_movie_upload(source, owner, tileIndex, tileCount);
}
