# Movie decode (src/movie): the disc's shortest SFD through the VFS, the program-stream demuxer and
# libavcodec, then the p4pc_sfdview tool on the same file (PNGs land in the build directory).
# Both skip (exit 77) unless P4PC_ISO names the image.
add_executable(sfd_decode_test sfd_decode_test.c "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}" $<TARGET_OBJECTS:p4movie>)
target_include_directories(sfd_decode_test PRIVATE "${CMAKE_SOURCE_DIR}/src/movie" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(sfd_decode_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(sfd_decode_test ${P4_AVCODEC_LIB} ${P4_SWSCALE_LIB} ${P4_AVUTIL_LIB} m)
add_test(NAME sfd_decode COMMAND sfd_decode_test)
set_tests_properties(sfd_decode PROPERTIES SKIP_RETURN_CODE 77)
add_test(NAME sfdview_disc COMMAND p4pc_sfdview DATA/SFD/P4CT019.SFD "${CMAKE_BINARY_DIR}/sfdview" 3 1)
set_tests_properties(sfdview_disc PROPERTIES SKIP_RETURN_CODE 77)

# No disc/game state: exhaustive pinned-converter arithmetic and actual shader readback.
set(movie_colour_targets movie_colour_test)
if(NOT ANDROID)
  list(APPEND movie_colour_targets movie_colour_generic_test)
endif()
foreach(colour_target IN LISTS movie_colour_targets)
  add_executable(${colour_target} movie_colour_test.c
    "${CMAKE_SOURCE_DIR}/src/movie/mpeg2_dec.c"
    "${CMAKE_SOURCE_DIR}/src/render/p4gl_gl.c"
    "${CMAKE_SOURCE_DIR}/src/render/p4gl_profile.c")
  target_include_directories(${colour_target} PRIVATE "${CMAKE_SOURCE_DIR}/src/movie"
    "${CMAKE_SOURCE_DIR}/src/render" "${P4_FFMPEG_INC}")
  target_link_libraries(${colour_target} ${P4_SDL3_LIB} ${P4_AVCODEC_LIB} ${P4_SWSCALE_LIB} ${P4_AVUTIL_LIB} m)
  if(ANDROID)
    target_compile_definitions(${colour_target} PRIVATE SDL_GL_GetProcAddress=p4colour_gl_proc)
    target_link_libraries(${colour_target} EGL dl)
  endif()
endforeach()
add_test(NAME movie_colour_cpu COMMAND movie_colour_test --cpu)
add_test(NAME movie_colour_gles COMMAND movie_colour_test)
add_test(NAME movie_colour_gl COMMAND movie_colour_test --gl)
set_tests_properties(movie_colour_cpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
set_tests_properties(movie_colour_gles movie_colour_gl PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120 LABELS gl)
if(NOT ANDROID)
  target_compile_definitions(movie_colour_generic_test PRIVATE P4GL_TEST_GENERIC_C=1)
  add_test(NAME movie_colour_generic_cpu COMMAND movie_colour_generic_test --cpu)
  add_test(NAME movie_colour_generic_gles COMMAND movie_colour_generic_test)
  add_test(NAME movie_colour_generic_gl COMMAND movie_colour_generic_test --gl)
  set_tests_properties(movie_colour_generic_cpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
  set_tests_properties(movie_colour_generic_gles movie_colour_generic_gl PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120 LABELS gl)
endif()

