# Golden install reader and PS2-layout translation on synthetic fixtures; no Golden or retail bytes.
add_executable(golden_source_test golden_source_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_tables.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c")
target_include_directories(golden_source_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_source_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_source COMMAND golden_source_test)
set_tests_properties(golden_source PROPERTIES TIMEOUT 30)
