if(P4_ANDROID AND NOT P4_ANDROID64)
  # The game as SDL's `main` library: SDL_main (src/platform/android/android_main.c) runs the loader's p4pc_main on a thread with a big stack.
  # The offline renderer (the scene gates' p4pc_viewer) is in the same library as p4pc_viewer_main, chosen with P4PC_PROGRAM=viewer: one APK runs
  # both kinds of gate (tools/android_run.py).
  set_source_files_properties(src/viewer/p4pc_viewer.c PROPERTIES COMPILE_DEFINITIONS "main=p4pc_viewer_main")
  add_library(main SHARED src/loader.c src/viewer/p4pc_viewer.c src/platform/android/android_main.c "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
  target_link_libraries(main m dl log android ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
  target_compile_options(main PRIVATE -fno-builtin)
  target_include_directories(main PRIVATE src/platform/files)
  target_compile_definitions(main PRIVATE ${P4_PORT_DEFINES})
  target_link_options(main PRIVATE -Wl,-z,max-page-size=16384 -Wl,--build-id=none -Wl,--wrap=exit)
  p4_crt_entry(main "${P4_GEN}/objects.txt" ${P4_CRT_FLAGS} ${P4_HOST_LIB_ARGS})
  set(_p4_source_stop TRUE)
  return()
endif()
