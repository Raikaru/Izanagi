add_executable(vfs_origin_test vfs_origin_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}")
target_include_directories(vfs_origin_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(vfs_origin_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(vfs_origin_test m)
# Preserve the driver's configured emulator when host Python launches it.
get_target_property(_p4_vfs_origin_runner vfs_origin_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_vfs_origin_runner)
  unset(_p4_vfs_origin_runner)
endif()
add_test(NAME vfs_origin COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/vfs_origin_test.py"
  ${_p4_vfs_origin_runner} "$<TARGET_FILE:vfs_origin_test>" --driver-end)
set_tests_properties(vfs_origin PROPERTIES TIMEOUT 30)
unset(_p4_vfs_origin_runner)

add_executable(vfs_source_test vfs_source_test.c vfs_source_iso.c
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c" "${P4_OS_SRC}")
target_include_directories(vfs_source_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(vfs_source_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(vfs_source_test m)
get_target_property(_p4_vfs_source_runner vfs_source_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_vfs_source_runner)
  unset(_p4_vfs_source_runner)
endif()
add_test(NAME vfs_source COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/vfs_source_test.py"
  ${_p4_vfs_source_runner} "$<TARGET_FILE:vfs_source_test>" --driver-end)
set_tests_properties(vfs_source PROPERTIES TIMEOUT 60)
unset(_p4_vfs_source_runner)

# The disc/container read loop over a fixture stream: interrupted, partial and failed host
# transfers (a short DrvFs transfer must never end a movie early).
add_executable(iso_read_test iso_read_test.c)
target_include_directories(iso_read_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
add_test(NAME iso_read COMMAND iso_read_test)
set_tests_properties(iso_read PROPERTIES TIMEOUT 30)
