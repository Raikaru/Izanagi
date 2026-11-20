# SPU ADPCM decoder against vectors worked by hand (no disc).
add_executable(vag_test vag_test.c "${CMAKE_SOURCE_DIR}/src/platform/sound/vag.c")
target_include_directories(vag_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform/sound")
add_test(NAME vag COMMAND vag_test)

# The IOP sound driver commands on the mixer, through the EE wrappers in the order the game
# issues them: a synthetic bank always, the disc's spu/test bank when P4PC_ISO is set.
add_executable(sdr_test sdr_test.c "${CMAKE_SOURCE_DIR}/src/platform/sound/ps2_sndcom.c"
  "${CMAKE_SOURCE_DIR}/src/platform/sound/sdr.c" "${CMAKE_SOURCE_DIR}/src/platform/sound/sdr_mem.c"
  "${CMAKE_SOURCE_DIR}/src/platform/sound/spu_pitch.c"
  "${CMAKE_SOURCE_DIR}/src/platform/sound/hd_bank.c" "${CMAKE_SOURCE_DIR}/src/platform/sound/vag.c"
  "${CMAKE_SOURCE_DIR}/src/platform/sound/p4snd_mixer.c" "${CMAKE_SOURCE_DIR}/src/platform/sound/p4snd_reverb.c" "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${CMAKE_SOURCE_DIR}/src/platform/gport.c" "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${P4_OS_SRC}")
target_sources(sdr_test PRIVATE state_dump_test_stubs.c "${CMAKE_SOURCE_DIR}/src/platform/kernel_thread.c"
  "${CMAKE_SOURCE_DIR}/src/platform/state_dump.c" "${CMAKE_SOURCE_DIR}/src/platform/ps2_kernel.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c")
target_include_directories(sdr_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/src/platform/sound" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(sdr_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(sdr_test m ${P4_SDL3_LIB})
add_test(NAME sdr COMMAND sdr_test)
set_tests_properties(sdr PROPERTIES ENVIRONMENT "P4PC_VIRTUAL_CLOCK=1;P4PC_TURBO=0")

p4_target_command(_sdr_test_command sdr_test)
add_test(NAME sdr_preset_hash COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/sdr_preset_hash_test.py" ${_sdr_test_command})
set_tests_properties(sdr_preset_hash PROPERTIES ENVIRONMENT "P4PC_VIRTUAL_CLOCK=1;P4PC_TURBO=0" TIMEOUT 30)

# A synthetic impulse response pins the reverb samples without a disc.
add_executable(p4snd_reverb_checksum_test p4snd_reverb_checksum_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/sound/p4snd_reverb.c")
target_include_directories(p4snd_reverb_checksum_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/sound")
add_test(NAME reverb_checksum COMMAND p4snd_reverb_checksum_test)

