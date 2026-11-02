# The first-registered exit tail must replace/spawn the same image after cleanup.
add_executable(restart_probe restart_probe.c "${CMAKE_SOURCE_DIR}/src/platform/os/restart.c" "${P4_OS_SRC}")
target_include_directories(restart_probe PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(restart_probe m)
# Python is CTest's outer command, so forward the target's emulator to its child.
get_target_property(_p4_restart_runner restart_probe CROSSCOMPILING_EMULATOR)
if(NOT _p4_restart_runner)
  unset(_p4_restart_runner)
endif()
add_test(NAME process_restart COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/restart_test.py"
  ${_p4_restart_runner} "$<TARGET_FILE:restart_probe>")
unset(_p4_restart_runner)
set_tests_properties(process_restart PROPERTIES TIMEOUT 90)

# The game calls the retail qsort through the same libc alias as this fixture.
# Equal keys have a fixed retail permutation, independent of the host CRT.
add_executable(qsort_test qsort_test.c "${CMAKE_SOURCE_DIR}/src/platform/ee_bytes.c" ${P4_GUEST_TEST_SRC})
target_include_directories(qsort_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(qsort_test m)
add_test(NAME qsort_retail_order COMMAND qsort_test)
set_tests_properties(qsort_retail_order PROPERTIES SKIP_RETURN_CODE 77)

# One shared allocation-free native SHA core; no disc, renderer or package runtime.
add_executable(content_digest_test content_digest_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c")
target_include_directories(content_digest_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME content_digest COMMAND content_digest_test)
set_tests_properties(content_digest PROPERTIES TIMEOUT 30)

# The table service's battle-bank extent registry; no retail bytes/combat mocks.
add_executable(game_table_extents_test game_table_extents_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_extents.c" ${P4_GUEST_TEST_SRC})
target_include_directories(game_table_extents_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(game_table_extents_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_table_extents_test m)
add_test(NAME game_table_extents COMMAND game_table_extents_test)
set_tests_properties(game_table_extents PROPERTIES SKIP_RETURN_CODE 77)

# Canonical saves, preparation wire and extent-bounded encounter start.
add_executable(battle_sim_table_test battle_sim_table_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/battle_sim/entry.c"
  "${CMAKE_SOURCE_DIR}/src/platform/battle_sim/prepare.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_extents.c" ${P4_GUEST_TEST_SRC})
target_include_directories(battle_sim_table_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(battle_sim_table_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(battle_sim_table_test m)
add_test(NAME battle_sim_table COMMAND battle_sim_table_test)
set_tests_properties(battle_sim_table PROPERTIES SKIP_RETURN_CODE 77)

# Host C broker contracts only; synthetic packets do not qualify combat.
add_executable(battle_events_test battle_events_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/battle_sim/events.c")
target_include_directories(battle_events_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME battle_events COMMAND battle_events_test)

add_executable(battle_live_entry_test battle_live_entry_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/battle_sim/live_entry.c")
target_include_directories(battle_live_entry_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME battle_live_entry COMMAND battle_live_entry_test)

# Original-retail collision seam decoding with synthetic memory only.
add_executable(lockstep_battle_boundary_test lockstep_battle_boundary_test.c)
target_include_directories(lockstep_battle_boundary_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME lockstep_battle_boundary COMMAND lockstep_battle_boundary_test)

# Execute the actual card-restore override against a synthetic section image.
add_executable(lockstep_restore_witness_test lockstep_restore_witness_test.c
  lockstep_align_guest_header_test.c "${CMAKE_SOURCE_DIR}/src/platform/ee_bytes.c" ${P4_GUEST_TEST_SRC})
target_include_directories(lockstep_restore_witness_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(lockstep_restore_witness_test m)
add_test(NAME lockstep_restore_witness COMMAND lockstep_restore_witness_test)
set_tests_properties(lockstep_restore_witness PROPERTIES SKIP_RETURN_CODE 77)

set(P4_EE_DECIMAL_SOURCES
  "${CMAKE_SOURCE_DIR}/src/platform/ee_decimal.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_dtoa.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ps2_fpbit.c"
  "${CMAKE_SOURCE_DIR}/src/platform/kernel_heap.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_heap.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/ee_heap_kernel.c")
# These fixtures exercise the production heap, not guest_test.c's bump allocator.
set(P4_EE_STDIO_GUEST_SOURCES ${P4_GUEST_TEST_SRC})
list(REMOVE_ITEM P4_EE_STDIO_GUEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/guest_test.c")
set(P4_EE_CRT_SOURCES
  "${CMAKE_SOURCE_DIR}/src/platform/ee_bytes.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ps2_libc.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_printf.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_scanf.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_strtod.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_libm.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c"
  "${CMAKE_SOURCE_DIR}/overrides/00440bb8.c"
  ${P4_EE_DECIMAL_SOURCES})

# Test the shipping CRT seam. PE links must resolve the real libm wrapper
# dependencies even when section GC later discards their unused code.
add_executable(ps2_libc_test ps2_libc_test.c ${P4_EE_CRT_SOURCES} ${P4_EE_STDIO_GUEST_SOURCES})
target_include_directories(ps2_libc_test PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/platform" "${CMAKE_SOURCE_DIR}/third_party")
target_compile_options(ps2_libc_test PRIVATE -ffunction-sections -fdata-sections)
target_link_options(ps2_libc_test PRIVATE LINKER:--gc-sections)
target_link_libraries(ps2_libc_test m)
add_test(NAME ps2_libc COMMAND ps2_libc_test)
set_tests_properties(ps2_libc PROPERTIES SKIP_RETURN_CODE 77)

# Original-ELF dtoa differential, including the original allocator and IRQ
# brackets. No native patch registrations or call hooks enter the interpreter.
add_executable(ee_decimal_oracle_test ee_decimal_oracle_test.c
  ${P4_EE_DECIMAL_SOURCES}
  "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c"
  "${CMAKE_SOURCE_DIR}/src/image.c"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c" ${P4_EE_STDIO_GUEST_SOURCES})
target_include_directories(ee_decimal_oracle_test PRIVATE
  "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform"
  "${CMAKE_SOURCE_DIR}/third_party" "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
target_link_libraries(ee_decimal_oracle_test m)
p4_ps2_exe(ee_decimal_oracle_test)
add_test(NAME ee_decimal_retail_oracle COMMAND ee_decimal_oracle_test)
set_tests_properties(ee_decimal_retail_oracle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)

add_executable(ee_printf_oracle_test ee_printf_oracle_test.c ${P4_EE_DECIMAL_SOURCES}
  "${CMAKE_SOURCE_DIR}/src/platform/ee_printf.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c"
  "${CMAKE_SOURCE_DIR}/src/image.c"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c" ${P4_EE_STDIO_GUEST_SOURCES})
target_include_directories(ee_printf_oracle_test PRIVATE
  "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform"
  "${CMAKE_SOURCE_DIR}/third_party" "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
target_link_libraries(ee_printf_oracle_test m)
add_test(NAME ee_printf_retail_oracle COMMAND ee_printf_oracle_test)
set_tests_properties(ee_printf_retail_oracle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)

# Actual narrow call expressions, not pre-widened va_list words. The 64-bit
# factory reuses explicitly full-rewritten object twins from 220-port64-host.
add_library(printf_abi_calls OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/data/ee_printf_abi_calls.c")
p4_printf_launcher(printf_abi_calls "${CMAKE_CURRENT_SOURCE_DIR}/data" none)
add_executable(ee_printf_abi_oracle_test ee_printf_abi_oracle_test.c
  $<TARGET_OBJECTS:printf_abi_calls> ${P4_EE_DECIMAL_SOURCES}
  "${CMAKE_SOURCE_DIR}/src/platform/ee_printf.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c"
  "${CMAKE_SOURCE_DIR}/src/image.c"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c" ${P4_EE_STDIO_GUEST_SOURCES})
target_include_directories(ee_printf_abi_oracle_test PRIVATE
  "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform"
  "${CMAKE_SOURCE_DIR}/third_party" "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
target_link_libraries(ee_printf_abi_oracle_test m)
p4_ps2_exe(ee_printf_abi_oracle_test)
add_test(NAME ee_printf_abi_retail_oracle COMMAND ee_printf_abi_oracle_test)
set_tests_properties(ee_printf_abi_retail_oracle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)

add_executable(ee_scanf_oracle_test ee_scanf_oracle_test.c ${P4_EE_CRT_SOURCES}
  "${CMAKE_SOURCE_DIR}/src/image.c"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c" ${P4_EE_STDIO_GUEST_SOURCES})
target_include_directories(ee_scanf_oracle_test PRIVATE
  "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform"
  "${CMAKE_SOURCE_DIR}/third_party" "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
target_link_libraries(ee_scanf_oracle_test m)
add_test(NAME ee_scanf_retail_oracle COMMAND ee_scanf_oracle_test)
set_tests_properties(ee_scanf_retail_oracle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)

# The retail libm cores (ee_libm.c, through the EE float launcher like the game) in the EE float mode: one hash per function over a fixed sweep, the same on
# both targets (tests/ee_libm_test.c, hashes in ee_libm_expect.h, recorded from the Linux build).
if(P4_EE_FLOAT)
  add_library(ee_libm_obj OBJECT "${CMAKE_SOURCE_DIR}/src/platform/ee_libm.c")
  set_target_properties(ee_libm_obj PROPERTIES C_COMPILER_LAUNCHER "${P4_PYTHON};${CMAKE_SOURCE_DIR}/tools/ee_float_cc.py;--modes=${P4_EE_FLOAT_OPS}")
  add_executable(ee_libm_test ee_libm_test.c "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c" $<TARGET_OBJECTS:ee_libm_obj>)
  target_link_libraries(ee_libm_test m)
  add_test(NAME ee_libm COMMAND ee_libm_test)
  set_tests_properties(ee_libm PROPERTIES SKIP_RETURN_CODE 77)
endif()

# The float-state check (state_dump.c, p4pc_fpu_check) restores the EE mode after a driver call changed it.
add_executable(fpu_state_test fpu_state_test.c state_dump_test_stubs.c "${CMAKE_SOURCE_DIR}/src/platform/state_dump.c" "${P4_OS_SRC}")
target_link_libraries(fpu_state_test m)
add_test(NAME fpu_state COMMAND fpu_state_test)
set_tests_properties(fpu_state PROPERTIES SKIP_RETURN_CODE 77)

# The GL call filter tracks the driver's bindings through deletes (src/render/p4gl_gl.c; an offscreen context, skipped without one).
add_executable(gl_cache_test gl_cache_test.c "${CMAKE_SOURCE_DIR}/src/render/p4gl_gl.c")
target_sources(gl_cache_test PRIVATE "${CMAKE_SOURCE_DIR}/src/render/p4gl_profile.c")
target_include_directories(gl_cache_test PRIVATE "${CMAKE_SOURCE_DIR}/src/render")
target_link_libraries(gl_cache_test m ${P4_SDL3_LIB})

add_executable(gpu_trace_test gpu_trace_test.c "${CMAKE_SOURCE_DIR}/src/render/p4gl_profile.c"
  "${CMAKE_SOURCE_DIR}/src/render/p4gl_gl.c" "${P4_OS_SRC}")
target_include_directories(gpu_trace_test PRIVATE "${CMAKE_SOURCE_DIR}/src/render")
target_compile_definitions(gpu_trace_test PRIVATE SDL_GL_GetProcAddress=p4gpu_test_proc SDL_STATIC_LIB)
target_link_libraries(gpu_trace_test m ${P4_SDL3_LIB})
add_test(NAME gpu_trace COMMAND gpu_trace_test)
add_test(NAME gl_cache COMMAND gl_cache_test)
set_tests_properties(gl_cache PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl")

# VU0 helper equivalence checks.
add_executable(vu0_helpers_test vu0_helpers_test.c "${CMAKE_SOURCE_DIR}/src/vu0.c" "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c")
target_include_directories(vu0_helpers_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(vu0_helpers_test m)
add_test(NAME vu0_helpers COMMAND vu0_helpers_test)

# Memory-card folder shim: save round-trip under a build-dir save root.
add_executable(memcard_test memcard_test.c "${CMAKE_SOURCE_DIR}/src/platform/memcard/ps2_memcard.c" "${P4_OS_SRC}")
target_link_libraries(memcard_test m)
target_compile_definitions(memcard_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME memcard_roundtrip COMMAND memcard_test "${CMAKE_BINARY_DIR}/memcard_test_save")
add_test(NAME memcard_long_path COMMAND memcard_test "${CMAKE_BINARY_DIR}/memcard_long_path_save" --long-path)

# Keyboard-to-pad: key array to snapshot to DualShock 2 packet (SDL is linked for pad_sdl.c).
add_executable(pad_keyboard_test pad_keyboard_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/pad/pad_sdl.c" "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c" "${CMAKE_SOURCE_DIR}/src/platform/pad/walk_script.c" "${CMAKE_SOURCE_DIR}/src/platform/pad/ps2_pad.c" "${P4_OS_SRC}")
target_sources(pad_keyboard_test PRIVATE battle_sim_inactive.c)
target_link_libraries(pad_keyboard_test m ${P4_SDL3_LIB})
add_test(NAME pad_keyboard COMMAND pad_keyboard_test)

# The gamepad path on SDL virtual gamepads (layout, sticks, triggers, hotplug); skipped where the game's GL window cannot be created.
add_executable(pad_gamepad_test pad_gamepad_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/pad/pad_sdl.c" "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c" "${CMAKE_SOURCE_DIR}/src/platform/pad/walk_script.c" "${CMAKE_SOURCE_DIR}/src/platform/pad/ps2_pad.c" "${P4_OS_SRC}")
target_sources(pad_gamepad_test PRIVATE battle_sim_inactive.c)
target_link_libraries(pad_gamepad_test m ${P4_SDL3_LIB})
target_compile_definitions(pad_gamepad_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME pad_gamepad COMMAND pad_gamepad_test)
add_test(NAME pad_gamepad_frame_clock COMMAND pad_gamepad_test --frame-clocked)
set_tests_properties(pad_gamepad pad_gamepad_frame_clock PROPERTIES SKIP_RETURN_CODE 77 LABELS gl)

add_executable(pad_isolation_test pad_isolation_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/pad/pad_sdl.c" "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c" "${CMAKE_SOURCE_DIR}/src/platform/pad/walk_script.c" "${CMAKE_SOURCE_DIR}/src/platform/pad/ps2_pad.c" "${P4_OS_SRC}")
target_sources(pad_isolation_test PRIVATE battle_sim_inactive.c)
target_link_libraries(pad_isolation_test m ${P4_SDL3_LIB})
target_compile_definitions(pad_isolation_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME pad_isolation_script COMMAND pad_isolation_test script)
add_test(NAME pad_isolation_virtual COMMAND pad_isolation_test virtual)
set_tests_properties(pad_isolation_script pad_isolation_virtual PROPERTIES SKIP_RETURN_CODE 77 LABELS gl)

# The settings file (INI parser, environment precedence, the video policy of src/platform/config/p4cfg.h).
add_executable(config_test config_test.c "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c" "${P4_OS_SRC}")
target_link_libraries(config_test m ${P4_SDL3_LIB})
target_compile_definitions(config_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME config COMMAND config_test)

# The player's disc: the settings file keeps its path, the executable is found on the image (P4PC_ISO, when set).
add_executable(p4iso_test p4iso_test.c "${CMAKE_SOURCE_DIR}/src/platform/config/p4iso.c" "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}")
target_link_libraries(p4iso_test m ${P4_SDL3_LIB})
target_compile_definitions(p4iso_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME p4iso COMMAND p4iso_test)
target_compile_definitions(pad_keyboard_test PRIVATE ${P4_PORT_DEFINES})

# File layer against the retail disc. Skipped (exit 77) unless P4PC_ISO names the image.
add_executable(files_test files_test.c "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}")
target_include_directories(files_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
add_test(NAME files_disc COMMAND files_test)
set_tests_properties(files_disc PROPERTIES SKIP_RETURN_CODE 77)
target_compile_definitions(files_test PRIVATE ${P4_PORT_DEFINES})

# Paletted native texture decode (src/render/p4gl_nativetex.c) on the textures of one disc model.
# Skipped (exit 77) unless P4PC_ISO names the image.
add_executable(native_tex_test native_tex_test.c "${CMAKE_SOURCE_DIR}/src/render/p4gl_nativetex.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c" "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}")
target_include_directories(native_tex_test PRIVATE "${CMAKE_SOURCE_DIR}/src/render" "${CMAKE_SOURCE_DIR}/src/platform/files")
add_test(NAME native_tex COMMAND native_tex_test)
set_tests_properties(native_tex PROPERTIES SKIP_RETURN_CODE 77)
target_compile_definitions(native_tex_test PRIVATE ${P4_PORT_DEFINES})

# fp-bit helpers (src/platform/ps2_fpbit.c): retail __pack_f, __unpack_f, __unpack_d and
# __fpcmp_parts_d, checked against input/output pairs worked from the retail asm.
add_executable(fpbit_test fpbit_test.c "${CMAKE_SOURCE_DIR}/src/platform/ps2_fpbit.c" "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${P4_OS_SRC}")
target_include_directories(fpbit_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform")
target_link_libraries(fpbit_test m)
add_test(NAME fpbit COMMAND fpbit_test)

# RwMatrix overrides (overrides/003df9f0.c, 003e0680.c, 003e0e20.c): orthonormalise, axis rotation
# and combine modes, with the override sources included as they are spliced into the mirror.
add_executable(matrix_override_test matrix_override_test.c)
target_include_directories(matrix_override_test PRIVATE "${CMAKE_SOURCE_DIR}/overrides")
target_link_libraries(matrix_override_test m)
add_test(NAME matrix_override COMMAND matrix_override_test)

