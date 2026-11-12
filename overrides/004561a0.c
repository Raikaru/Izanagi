/* H_Cdvd_StartPlayback (retail 0x004561a0): starts a movie on a player. Retail builds the upper-case path, selects its directory and hands the file name to Sofdec (mwPlyStartFnameLp, or func_0050c008 for the non-loop player). The port's player (src/movie/mwply.c) opens the whole path through the VFS. */
extern void p4pc_movie_start(void *handle, const char *path, s32 loop);

void H_Cdvd_StartPlayback(void* handle, const char* path, s32 synchronous)
{
    p4pc_movie_start(handle, path, synchronous);
}
