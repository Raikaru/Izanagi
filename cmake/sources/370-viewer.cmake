# Offline renderer: loads the same image and drives the RenderWare engine directly, with
# no game boot (src/viewer/p4pc_viewer.c). See docs/platform/testing.md "Viewer and renderer smoke".
add_executable(p4pc_viewer src/viewer/p4pc_viewer.c "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_link_libraries(p4pc_viewer m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
target_compile_options(p4pc_viewer PRIVATE -fno-builtin)
target_include_directories(p4pc_viewer PRIVATE src/platform/files)
target_compile_definitions(p4pc_viewer PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(p4pc_viewer)

