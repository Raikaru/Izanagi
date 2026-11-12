add_executable(mod_memory_policy_test mod_memory_policy_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/memory_policy.c" ${P4_GUEST_TEST_SRC})
target_include_directories(mod_memory_policy_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(mod_memory_policy_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_memory_policy_test m)
p4_ps2_exe(mod_memory_policy_test)
add_test(NAME mod_memory_policy COMMAND mod_memory_policy_test)

# Production EE allocator, not guest_test.c's allocation replacement.
add_executable(heap_guest_span_test heap_guest_span_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/kernel_heap.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_heap.c" ee_heap_kernel.c
  ${P4_EE_STDIO_GUEST_SOURCES})
target_include_directories(heap_guest_span_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(heap_guest_span_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(heap_guest_span_test m)
p4_ps2_exe(heap_guest_span_test)
add_test(NAME heap_guest_span COMMAND heap_guest_span_test)
set_tests_properties(mod_memory_policy heap_guest_span PROPERTIES TIMEOUT 30)

add_executable(image_guest_ranges_test image_guest_ranges_test.c
  "${CMAKE_SOURCE_DIR}/src/image.c" ${P4_GUEST_TEST_SRC})
target_include_directories(image_guest_ranges_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(image_guest_ranges_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(image_guest_ranges_test m)
p4_ps2_exe(image_guest_ranges_test)
get_target_property(_p4_image_ranges_runner image_guest_ranges_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_image_ranges_runner)
  unset(_p4_image_ranges_runner)
endif()
add_test(NAME image_guest_ranges COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/image_guest_ranges_test.py"
  ${_p4_image_ranges_runner} "$<TARGET_FILE:image_guest_ranges_test>" --driver-end)
set_tests_properties(image_guest_ranges PROPERTIES TIMEOUT 60)
unset(_p4_image_ranges_runner)

# Compile the loader into the fixture so its stdio boundary can simulate
# interrupted and partial transfers without relying on a host-specific stream.
add_executable(image_read_test image_read_test.c ${P4_GUEST_TEST_SRC})
target_include_directories(image_read_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(image_read_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(image_read_test m)
p4_ps2_exe(image_read_test)
add_test(NAME image_read COMMAND image_read_test)
set_tests_properties(image_read PROPERTIES TIMEOUT 30)
