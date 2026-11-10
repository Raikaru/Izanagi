# Thread, semaphore, alarm and VSync model (src/platform/kernel_thread.c), with the INTC table
# from ps2_kernel.c. The cases share one process and run in order.
add_executable(kernel_threads_test kernel_threads_test.c state_dump_test_stubs.c
  "${CMAKE_SOURCE_DIR}/src/platform/kernel_thread.c" "${CMAKE_SOURCE_DIR}/src/platform/state_dump.c" "${CMAKE_SOURCE_DIR}/src/platform/ps2_kernel.c"
  "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${P4_OS_SRC}")
target_include_directories(kernel_threads_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(kernel_threads_test m)
add_test(NAME kernel_threads COMMAND kernel_threads_test)
set_tests_properties(kernel_threads PROPERTIES ENVIRONMENT "P4PC_VIRTUAL_CLOCK=1;P4PC_TURBO=0" TIMEOUT 60)
add_test(NAME kernel_threads_paced COMMAND kernel_threads_test)
set_tests_properties(kernel_threads_paced PROPERTIES ENVIRONMENT "P4PC_VIRTUAL_CLOCK=0;P4PC_TURBO=0" TIMEOUT 60)

add_executable(capture_state_test capture_state_test.c ${P4_GUEST_TEST_SRC})
target_include_directories(capture_state_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(capture_state_test m)
p4_ps2_exe(capture_state_test)
add_test(NAME capture_state COMMAND capture_state_test)

# Shared retail/port controller: stop advancing dialogue when a modal task becomes active.
add_executable(walk_tasks_test walk_tasks_test.c "${CMAKE_SOURCE_DIR}/src/platform/pad/walk_script.c")
target_include_directories(walk_tasks_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(walk_tasks_test m)
add_test(NAME walk_tasks COMMAND walk_tasks_test)

# The EE's nearest-rounded division and square roots: the integer implementations (hosts without the x87 unit use them) against the long double formulas and the
# x87 path (include/p4pc/eefloat.h).
add_executable(ee_float_test ee_float_test.c)
target_include_directories(ee_float_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(ee_float_test m)
add_test(NAME ee_float COMMAND ee_float_test)

# The indirect-call adapters of the 64-bit model (register classes per parameter, floats in xmm registers). Skips on i386.
add_executable(fn_adapt_test fn_adapt_test.c guest_test.c "${CMAKE_SOURCE_DIR}/src/platform/gport.c" "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${P4_OS_SRC}")
target_include_directories(fn_adapt_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(fn_adapt_test m)
add_test(NAME fn_adapt COMMAND fn_adapt_test)
set_tests_properties(fn_adapt PROPERTIES SKIP_RETURN_CODE 77)
p4_ps2_exe(fn_adapt_test)

