# Standalone movie viewer: SFD from the disc through the VFS, demux, libavcodec decode, PNGs.
# Needs no game boot. See src/movie/p4pc_sfdview.c.
add_library(p4movie OBJECT src/movie/sfd_demux.c src/movie/mpeg2_dec.c src/movie/adx_dec.c)
target_include_directories(p4movie PRIVATE src/platform/files "${P4_FFMPEG_INC}")
target_compile_definitions(p4movie PRIVATE ${P4_PORT_DEFINES})
add_executable(p4pc_sfdview src/movie/p4pc_sfdview.c ${P4_PNG_SOURCES} src/platform/files/vfs.c
  src/platform/files/iso9660.c "${P4_OS_SRC}" $<TARGET_OBJECTS:p4movie>)
add_dependencies(p4pc_sfdview p4_mirror)
target_include_directories(p4pc_sfdview PRIVATE src/movie src/viewer src/platform/files)
target_compile_definitions(p4pc_sfdview PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(p4pc_sfdview ${P4_AVCODEC_LIB} ${P4_SWSCALE_LIB} ${P4_AVUTIL_LIB} m)
file(GENERATE OUTPUT "${P4_GEN}/crt-sfdview-definitions.txt"
  CONTENT "$<JOIN:$<TARGET_OBJECTS:p4movie>,\n>\n")
p4_crt_entry(p4pc_sfdview "${P4_GEN}/crt-sfdview-definitions.txt" ${P4_CRT_FLAGS} ${P4_HOST_LIB_ARGS})
