# Licence of the linked FFmpeg libraries (third_party/README.md). Always runs and reports; with
# -DP4_RELEASE=ON it fails unless libavcodec, libavutil and libswscale are LGPL-only builds.
option(P4_RELEASE "Release build: require LGPL-only FFmpeg libraries (ctest ffmpeg_license)" OFF)
add_executable(ffmpeg_license_test ffmpeg_license_test.c)
target_include_directories(ffmpeg_license_test PRIVATE "${P4_FFMPEG_INC}")
target_link_libraries(ffmpeg_license_test ${P4_FFMPEG_LIBS} m)
add_test(NAME ffmpeg_license COMMAND ffmpeg_license_test)
if(P4_RELEASE)
  set_tests_properties(ffmpeg_license PROPERTIES ENVIRONMENT "P4PC_REQUIRE_LGPL=1")
endif()

# The movie audio hook on the mixer against a stand-in for the movie player's audio interface.
add_executable(movie_audio_test movie_audio_test.c "${CMAKE_SOURCE_DIR}/src/platform/sound/movie_audio.c"
  "${CMAKE_SOURCE_DIR}/src/platform/sound/p4snd_mixer.c" "${CMAKE_SOURCE_DIR}/src/platform/sound/p4snd_reverb.c" "${P4_OS_SRC}")
target_include_directories(movie_audio_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform/sound" "${CMAKE_SOURCE_DIR}/src/movie")
target_link_libraries(movie_audio_test m ${P4_SDL3_LIB})
add_test(NAME movie_audio COMMAND movie_audio_test)

