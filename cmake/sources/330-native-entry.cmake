if(NOT P4_ANDROID64)
  add_executable(p4pc src/loader.c "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
  target_link_libraries(p4pc m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
  target_compile_options(p4pc PRIVATE -fno-builtin)
  target_compile_definitions(p4pc PRIVATE ${P4_PORT_DEFINES})
  p4_ps2_exe(p4pc)
endif()

