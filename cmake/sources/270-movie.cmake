# OpenGL 3.3 core device layer (src/render): the RenderWare device block, the system request
# dispatcher and the GL context on the shared window. See docs/renderer_port.md.
# Movie playback on the IPU seam: Sofdec's SFMPVP/viBuf layer and the frame upload (src/movie).
target_sources(p4port PRIVATE src/movie/sfmpvp.c src/movie/mpeg2_dec.c src/movie/movie_upload.c src/movie/mwply.c src/movie/sfd_demux.c src/movie/adx_dec.c)
target_include_directories(p4port PRIVATE src/movie src/render src/platform/files "${P4_FFMPEG_INC}")

