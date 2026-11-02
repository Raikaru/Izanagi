# Real generated CRI entries against retail instructions, including alias and
# return-ABI contracts. Only the named external seam calls are wrapped.
add_executable(cri_cpu_adapter_test cri_cpu_adapter_test.c
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c" "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(cri_cpu_adapter_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
target_compile_definitions(cri_cpu_adapter_test PRIVATE ${P4_PORT_DEFINES})
target_compile_options(cri_cpu_adapter_test PRIVATE -fno-builtin)
target_link_options(cri_cpu_adapter_test PRIVATE LINKER:--gc-sections
  LINKER:--wrap=func_004e2108 LINKER:--wrap=cvFsSeek
  LINKER:--wrap=func_004e22f8 LINKER:--wrap=cvFsGetStat
  LINKER:--wrap=func_004e2750 LINKER:--wrap=cvFsGetNumTr
  LINKER:--wrap=func_004e4190
  LINKER:--wrap=cvFsCallUsrErrFn LINKER:--wrap=func_004f0578)
target_link_libraries(cri_cpu_adapter_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(cri_cpu_adapter_test)
add_test(NAME cri_cpu_adapters COMMAND cri_cpu_adapter_test)
set_tests_properties(cri_cpu_adapters PROPERTIES SKIP_RETURN_CODE 77)

# The raw SFD audio callback chain uses the real generated providers and
# production function-pointer bridge; only ADXT silence insertion is external.
add_executable(cri_sfd_callback_test cri_sfd_callback_test.c
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c" "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(cri_sfd_callback_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
target_compile_definitions(cri_sfd_callback_test PRIVATE ${P4_PORT_DEFINES})
target_compile_options(cri_sfd_callback_test PRIVATE -fno-builtin)
target_link_options(cri_sfd_callback_test PRIVATE LINKER:--gc-sections
  LINKER:--wrap=func_004d6910)
target_link_libraries(cri_sfd_callback_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(cri_sfd_callback_test)
add_test(NAME cri_sfd_callbacks COMMAND cri_sfd_callback_test)
set_tests_properties(cri_sfd_callbacks PROPERTIES SKIP_RETURN_CODE 77)

# ROFS metadata options preserve the PC device contract through the generated entry.
add_executable(cri_rofs_metadata_test cri_rofs_metadata_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(cri_rofs_metadata_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(cri_rofs_metadata_test PRIVATE ${P4_PORT_DEFINES})
target_compile_options(cri_rofs_metadata_test PRIVATE -fno-builtin)
target_link_options(cri_rofs_metadata_test PRIVATE LINKER:--gc-sections)
target_link_libraries(cri_rofs_metadata_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(cri_rofs_metadata_test)
add_test(NAME cri_rofs_metadata COMMAND cri_rofs_metadata_test)
set_tests_properties(cri_rofs_metadata PROPERTIES SKIP_RETURN_CODE 77)

# The AXRNA renderer behind ADXRNA on CRI's own stream joints: ring accounting, transfer,
# play/pause/stop, order and volume, with the mixer offline (tests/axrna_test.c). Links the same
# objects as p4pc. Skipped (exit 77) unless P4PC_ISO and P4PC_ELF are set.
add_executable(axrna_test axrna_test.c "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(axrna_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform/sound")
target_compile_options(axrna_test PRIVATE -fno-builtin)
target_compile_definitions(axrna_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(axrna_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(axrna_test)
add_test(NAME axrna COMMAND axrna_test)
set_tests_properties(axrna PROPERTIES SKIP_RETURN_CODE 77)

# The boot movie's player start in an exe linked like p4pc (generated tables included), with traps fatal:
# a host library name the tables step took for a retail function becomes a trap stub that shadows it
# (pthread_mutex_* on Windows) and only fails once the movie plays. Skipped (exit 77) unless P4PC_ISO is set.
add_executable(movie_start_test movie_start_test.c "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(movie_start_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src/movie" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_options(movie_start_test PRIVATE -fno-builtin)
target_compile_definitions(movie_start_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(movie_start_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(movie_start_test)
add_test(NAME movie_start COMMAND movie_start_test)
set_tests_properties(movie_start PROPERTIES SKIP_RETURN_CODE 77)

# The CRI_ADXI.IRX RPC server behind sceSifCallRpc (src/cri/adxi_rpc.c): one case per command the EE
# sends, remote stream joints, the IOPRNA core state machine and a PSM's PCM reaching the mixer
# (tests/cri_adxi_rpc_test.c). Links the same objects as p4pc. Skipped (exit 77) unless P4PC_ISO and
# P4PC_ELF are set.
add_executable(cri_adxi_rpc_test cri_adxi_rpc_test.c "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(cri_adxi_rpc_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/cri"
  "${CMAKE_SOURCE_DIR}/src/platform/sound")
target_compile_options(cri_adxi_rpc_test PRIVATE -fno-builtin)
target_compile_definitions(cri_adxi_rpc_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(cri_adxi_rpc_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(cri_adxi_rpc_test)
add_test(NAME cri_adxi_rpc COMMAND cri_adxi_rpc_test)
set_tests_properties(cri_adxi_rpc PROPERTIES SKIP_RETURN_CODE 77)

# The mixer on a headless output path in real time: SDL's dummy driver, and a driver name SDL
# lacks (the device fails to open and the pacing thread takes over).
add_executable(mixer_device_test mixer_device_test.c "${CMAKE_SOURCE_DIR}/src/platform/sound/p4snd_reverb.c" "${P4_OS_SRC}")
target_include_directories(mixer_device_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform/sound")
target_link_libraries(mixer_device_test m ${P4_SDL3_LIB})
add_test(NAME mixer_device_dummy COMMAND mixer_device_test dummy)
add_test(NAME mixer_device_pacer COMMAND mixer_device_test none)
add_test(NAME mixer_virtual_silent COMMAND mixer_device_test dummy virtual)
add_test(NAME mixer_virtual_live COMMAND mixer_device_test dummy live)
add_test(NAME mixer_virtual_live_no_device COMMAND mixer_device_test none live)
add_test(NAME mixer_zero_pitch COMMAND mixer_device_test dummy zero-pitch)
set_tests_properties(mixer_virtual_silent mixer_virtual_live mixer_virtual_live_no_device PROPERTIES TIMEOUT 90)
# Every sound effect bank on the disc through the .hd/.bd/.sq readers (tests/sebank_test.c).
# Skipped (exit 77) unless P4PC_ISO names the image.
add_executable(sebank_test sebank_test.c "${CMAKE_SOURCE_DIR}/src/platform/sound/hd_bank.c"
  "${CMAKE_SOURCE_DIR}/src/platform/sound/vag.c" "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}")
target_include_directories(sebank_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform/sound" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(sebank_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(sebank_test m)
add_test(NAME sebank COMMAND sebank_test)
set_tests_properties(sebank PROPERTIES SKIP_RETURN_CODE 77)

# CRI ACS server and fader (src/cri/acs.c): call order of ACSSVR_Init/Finish and the fader levels,
# against recording stubs for the retail callees.
add_executable(cri_acs_test cri_acs_test.c ${P4_GUEST_TEST_SRC}
  "${CMAKE_SOURCE_DIR}/src/platform/ee_dfloat.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_dlibm.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ps2_fpbit.c")
target_link_libraries(cri_acs_test m)
p4_ps2_exe(cri_acs_test)
add_test(NAME cri_acs COMMAND cri_acs_test)

# ADX through the game's CRI decoder and the AXRNA seam into the mixer, written as a WAV and
# checked against the ADX header and a reference decode (tests/adx_wav_test.c). Links the same
# objects as p4pc. Skipped (exit 77) unless P4PC_ISO and P4PC_ELF are set.
add_executable(adx_wav_test adx_wav_test.c "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(adx_wav_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform/sound" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_options(adx_wav_test PRIVATE -fno-builtin)
target_compile_definitions(adx_wav_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(adx_wav_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(adx_wav_test)
add_test(NAME adx_wav COMMAND adx_wav_test "${CMAKE_BINARY_DIR}/adx_wav_test.wav")
set_tests_properties(adx_wav PROPERTIES SKIP_RETURN_CODE 77)

# An AHX voice through ADXT_AttachAHX, the translated AHX decoder (src/cri/ahx.c) and the AXRNA seam into
# the mixer, written as a WAV and checked against the AHX header (tests/ahx_wav_test.c). Skipped (exit 77)
# unless P4PC_ISO and P4PC_ELF are set.
add_executable(ahx_wav_test ahx_wav_test.c "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(ahx_wav_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform/sound" "${CMAKE_SOURCE_DIR}/src/platform/files" "${P4_FFMPEG_INC}")
target_compile_options(ahx_wav_test PRIVATE -fno-builtin)
target_compile_definitions(ahx_wav_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(ahx_wav_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(ahx_wav_test)
add_test(NAME ahx_wav COMMAND ahx_wav_test "${CMAKE_BINARY_DIR}/ahx_wav_test.wav")
set_tests_properties(ahx_wav PROPERTIES SKIP_RETURN_CODE 77)

# A one track AIX built from an ADX of the disc through the translated AIX player (src/cri/aixp.c): the AIXSJD
# demultiplexer and the player state machine feed an ADXT track whose PCM reaches the mixer through AXRNA; the output
# is compared with a reference decode (tests/aixp_wav_test.c). Skipped (exit 77) unless P4PC_ISO and P4PC_ELF are set.
add_executable(aixp_wav_test aixp_wav_test.c "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(aixp_wav_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform/sound" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_options(aixp_wav_test PRIVATE -fno-builtin)
target_compile_definitions(aixp_wav_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(aixp_wav_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(aixp_wav_test)
add_test(NAME aixp_wav COMMAND aixp_wav_test "${CMAKE_BINARY_DIR}/aixp_wav_test.wav")
set_tests_properties(aixp_wav PROPERTIES SKIP_RETURN_CODE 77)

