# Actual package core + native codec; no fake catalog, Lua or game snapshot.
get_target_property(_p4_save_core mod_files_test SOURCES)
list(REMOVE_ITEM _p4_save_core mod_files_test.c "${CMAKE_CURRENT_SOURCE_DIR}/mod_files_test.c")
add_executable(mod_save_test mod_save_test.c ${_p4_save_core}
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save_hash.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save_format.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/save_image.c")
target_include_directories(mod_save_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(mod_save_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_save_test m)
# Existing raw MC gates exercise the real transport/codec/package core too.
# Keep their actual OS source once; no fake catalog or successful UI stub.
set(_p4_mc_core ${_p4_save_core})
list(REMOVE_ITEM _p4_mc_core ${P4_OS_SRC})
set(_p4_save_transport
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save_hash.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save_format.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/save_image.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/save_transaction.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save_custody.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save_transport.c")
target_sources(memcard_test PRIVATE ${_p4_mc_core} ${_p4_save_transport})
target_include_directories(memcard_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
add_executable(mod_save_transport_test mod_save_transport_test.c ${_p4_save_core} ${_p4_save_transport}
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/ps2_memcard.c")
target_include_directories(mod_save_transport_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(mod_save_transport_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_save_transport_test m)
get_target_property(_p4_save_transport_runner mod_save_transport_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_save_transport_runner)
  unset(_p4_save_transport_runner)
endif()
add_test(NAME mod_save_transport COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/mod_save_transport_test.py"
  ${_p4_save_transport_runner} "$<TARGET_FILE:mod_save_transport_test>" --driver-end
  ${_p4_save_transport_runner} "$<TARGET_FILE:save_import_test>" --import-end)
set_tests_properties(mod_save_transport PROPERTIES LABELS "host" TIMEOUT 300
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_CONFIG=none")
unset(_p4_save_transport_runner)
unset(_p4_mc_core)
unset(_p4_save_transport)
get_target_property(_p4_save_runner mod_save_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_save_runner)
  unset(_p4_save_runner)
endif()
add_test(NAME mod_save_codec COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/mod_save_test.py"
  ${_p4_save_runner} "$<TARGET_FILE:mod_save_test>" --driver-end)
set_tests_properties(mod_save_codec PROPERTIES LABELS "host" TIMEOUT 60
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
unset(_p4_save_core)
unset(_p4_save_runner)
