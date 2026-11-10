# Action Replay MAX: generated LZARI fixtures, bounded parsing, integrity checks.
add_executable(import_max_test import_max_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/import_max.c" "${P4_OS_SRC}")
target_include_directories(import_max_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME import_max COMMAND import_max_test)
