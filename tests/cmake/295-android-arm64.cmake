# The tests that exist only for the Android arm64 build (label arm64; tools/phone_units.py runs them on the phone, docs/android_arm64_tests.md). Linux AArch64
# under qemu never registered these: their registrations (210-port64-runtime.cmake) are x86_64 hosts only. They exercise the AArch64 parts of the platform layer
# that the rest of the 64-bit tests only reach through the game: the signal handler's register block and the coroutine context switch.
if(P4_ANDROID_ARM64)
  # the production crash handler (src/platform/os/os_posix.c), one signal at a time, reported by tests/crash_log_test.sh
  add_executable(crash_log_test_arm64 crash_log_test.c "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${P4_OS_SRC}")
  target_include_directories(crash_log_test_arm64 PRIVATE "${CMAKE_SOURCE_DIR}/include")
  target_compile_options(crash_log_test_arm64 PRIVATE ${P4_X64_FLAGS} -O2 -g -include p4pc/compat.h -ffunction-sections -fdata-sections)
  target_compile_definitions(crash_log_test_arm64 PRIVATE ${P4_PORT_DEFINES})
  target_link_options(crash_log_test_arm64 PRIVATE -Wl,--gc-sections)
  add_test(NAME crash_log_arm64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/crash_log_test.sh" $<TARGET_FILE:crash_log_test_arm64>)
  set_tests_properties(crash_log_arm64 PROPERTIES LABELS arm64 TIMEOUT 90)
  # coroutines on guest stacks: the AArch64 context switch keeps x19..x28 and the stack, 100000 switches in a ring of three
  add_executable(port64_coro_test_arm64 port64_coro_test.c "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${P4_OS_SRC}")
  target_include_directories(port64_coro_test_arm64 PRIVATE "${CMAKE_SOURCE_DIR}/include")
  target_compile_options(port64_coro_test_arm64 PRIVATE ${P4_X64_FLAGS} -O2 -g -include p4pc/compat.h -ffunction-sections -fdata-sections)
  target_compile_definitions(port64_coro_test_arm64 PRIVATE ${P4_PORT_DEFINES})
  target_link_options(port64_coro_test_arm64 PRIVATE -Wl,--gc-sections)
  add_test(NAME port64_coro_arm64 COMMAND port64_coro_test_arm64)
  set_tests_properties(port64_coro_arm64 PROPERTIES LABELS arm64 SKIP_RETURN_CODE 77 TIMEOUT 120)
  # the guest function-handle bounds (a 64-bit host's private cursor): registered for Linux hosts in 220-port64-host.cmake, which a cross build skips
  add_executable(guest_fn_handle_bounds_test_arm64 guest_fn_handle_bounds_test.c)
  target_include_directories(guest_fn_handle_bounds_test_arm64 PRIVATE "${CMAKE_SOURCE_DIR}/include")
  target_compile_options(guest_fn_handle_bounds_test_arm64 PRIVATE ${P4_X64_FLAGS} -ffunction-sections -fdata-sections)
  target_link_options(guest_fn_handle_bounds_test_arm64 PRIVATE -Wl,--gc-sections)
  add_test(NAME guest_fn_handle_bounds_arm64 COMMAND guest_fn_handle_bounds_test_arm64)
  set_tests_properties(guest_fn_handle_bounds_arm64 PROPERTIES LABELS arm64)
endif()
