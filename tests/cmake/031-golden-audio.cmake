# No retail input: bounded containers, FFmpeg packets, existing mixer, mode isolation.
add_executable(golden_audio_cli golden_audio_cli.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_awb.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_acb.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_hca_header.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_hca.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_adx.c"
  "${CMAKE_SOURCE_DIR}/src/movie/adx_dec.c"
  "${CMAKE_SOURCE_DIR}/src/platform/sound/p4snd_golden.c"
  "${CMAKE_SOURCE_DIR}/src/platform/sound/p4snd_mixer.c"
  "${CMAKE_SOURCE_DIR}/src/platform/sound/p4snd_reverb.c"
  "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c"
  "${CMAKE_SOURCE_DIR}/src/platform/config/game_mode.c" "${P4_OS_SRC}")
target_include_directories(golden_audio_cli PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform/sound" "${P4_FFMPEG_INC}")
target_link_libraries(golden_audio_cli ${P4_SDL3_LIB} ${P4_AVCODEC_LIB} ${P4_AVUTIL_LIB} m)
p4_target_command(golden_audio_command golden_audio_cli)
add_test(NAME golden_audio COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_audio_test.py" ${golden_audio_command})
set_tests_properties(golden_audio PROPERTIES TIMEOUT 120)
add_test(NAME golden_audio_steam COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_audio_steam_test.py" ${golden_audio_command})
set_tests_properties(golden_audio_steam PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1800)
