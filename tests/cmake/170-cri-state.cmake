# CRI ACSSND creation (src/cri/acssnd.c): work-area carving, object layout and error texts.
add_executable(cri_acssnd_test cri_acssnd_test.c ${P4_GUEST_TEST_SRC})
p4_ps2_exe(cri_acssnd_test)
add_test(NAME cri_acssnd COMMAND cri_acssnd_test)

# CRI ACS per-frame steps (src/cri/acs_step.c): fader tick, voice-highlight state machine, ACSSND frame.
add_executable(cri_acs_step_test cri_acs_step_test.c ${P4_GUEST_TEST_SRC})
p4_ps2_exe(cri_acs_step_test)
add_test(NAME cri_acs_step COMMAND cri_acs_step_test)

# CRI ADXM server passes and vsync body (src/cri/adxm.c), over memory mapped at the retail globals.
add_executable(cri_adxm_test cri_adxm_test.c ${P4_GUEST_TEST_SRC})
target_include_directories(cri_adxm_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
p4_ps2_exe(cri_adxm_test)
add_test(NAME cri_adxm COMMAND cri_adxm_test)
set_tests_properties(cri_adxm PROPERTIES SKIP_RETURN_CODE 77)

# CRI ADX setup, voice configuration and CVFS device servers (src/cri/adx_init.c), over memory mapped
# at the retail globals.
add_executable(cri_init_test cri_init_test.c ${P4_GUEST_TEST_SRC})
target_include_directories(cri_init_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
p4_ps2_exe(cri_init_test)
add_test(NAME cri_init COMMAND cri_init_test)
set_tests_properties(cri_init PROPERTIES SKIP_RETURN_CODE 77)

# CRI ADXSTM and ADX server entry points (src/cri/adxstm_api.c), over memory mapped at the retail globals.
add_executable(cri_stm_test cri_stm_test.c ${P4_GUEST_TEST_SRC})
target_include_directories(cri_stm_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
p4_ps2_exe(cri_stm_test)
add_test(NAME cri_stm COMMAND cri_stm_test)
set_tests_properties(cri_stm PROPERTIES SKIP_RETURN_CODE 77)

# CVFS device table scan (src/cri/cvfs_port.c), over memory mapped at the retail table.
add_executable(cri_cvfs_test cri_cvfs_test.c ${P4_EE_CRT_SOURCES} ${P4_EE_STDIO_GUEST_SOURCES})
target_include_directories(cri_cvfs_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform" "${CMAKE_SOURCE_DIR}/third_party")
p4_ps2_exe(cri_cvfs_test)
add_test(NAME cri_cvfs COMMAND cri_cvfs_test)
set_tests_properties(cri_cvfs PROPERTIES SKIP_RETURN_CODE 77)

# CRI ADXT start entry points, key derivation and sample adjust (src/cri/adxt_start.c), over memory mapped at the retail globals.
add_executable(cri_adxt_start_test cri_adxt_start_test.c ${P4_GUEST_TEST_SRC})
target_include_directories(cri_adxt_start_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
p4_ps2_exe(cri_adxt_start_test)
add_test(NAME cri_adxt_start COMMAND cri_adxt_start_test)
set_tests_properties(cri_adxt_start PROPERTIES SKIP_RETURN_CODE 77)

# How a CRI file name ("VOL:EVE_056.ADX", "ENV:ENV_346.ADX", "VOL:V162001.AFS" with a directory list) finds its file:
# every distinct open of a retail run through the ROFS volume devices and the selected directory lists
# (tests/rofs_open_test.c, tests/data/rofs_retail_opens.txt). Skipped (exit 77) unless P4PC_ISO and P4PC_ELF are set.
add_executable(rofs_open_test rofs_open_test.c "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(rofs_open_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform/sound" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_options(rofs_open_test PRIVATE -fno-builtin)
target_compile_definitions(rofs_open_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(rofs_open_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(rofs_open_test)
add_test(NAME rofs_open COMMAND rofs_open_test "${CMAKE_SOURCE_DIR}/tests/data/rofs_retail_opens.txt")
set_tests_properties(rofs_open PROPERTIES SKIP_RETURN_CODE 77)

# The real ISO through CRI's 64-bit file-size/seek chain and mirrored word wrappers.
add_executable(cri_file_size_test cri_file_size_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(cri_file_size_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_options(cri_file_size_test PRIVATE -fno-builtin)
target_compile_definitions(cri_file_size_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(cri_file_size_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(cri_file_size_test)
add_test(NAME cri_file_size COMMAND cri_file_size_test)
set_tests_properties(cri_file_size PROPERTIES SKIP_RETURN_CODE 77)

# The same checks with the file streamed by its CRI name through CVFS and the ROFS volumes, as the game plays
# its voice, event and environment sounds (the directory lists decide where "VOL:EVE_521.ADX" is).
add_test(NAME adx_stream_event COMMAND adx_wav_test "${CMAKE_BINARY_DIR}/adx_stream_event.wav")
set_tests_properties(adx_stream_event PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT
  "P4PC_ADX=DATA/SOUND/EVENT/EVE_521.ADX;P4PC_ADX_STREAM=VOL:EVE_521.ADX;P4PC_ADX_DIR=SOUND/EVENT")
add_test(NAME adx_stream_voice COMMAND adx_wav_test "${CMAKE_BINARY_DIR}/adx_stream_voice.wav")
set_tests_properties(adx_stream_voice PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT
  "P4PC_ADX=DATA/SOUND/V176001.AFS;P4PC_ADX_STREAM=VOL:V176001.AFS;P4PC_ADX_AFS_MEMBER=1")
add_test(NAME adx_stream_env COMMAND adx_wav_test "${CMAKE_BINARY_DIR}/adx_stream_env.wav")
set_tests_properties(adx_stream_env PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT
  "P4PC_ADX=ENV/ENV_346.ADX;P4PC_ADX_STREAM=ENV:ENV_346.ADX")

