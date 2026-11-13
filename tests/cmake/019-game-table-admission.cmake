# Selected source bytes, actual descriptor vectors and one-use handoff; no live-image claim.
get_target_property(_p4_admission_sources mod_files_test SOURCES)
list(REMOVE_ITEM _p4_admission_sources mod_files_test.c "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c")
add_executable(game_table_admission_test game_table_admission_test.c vfs_source_iso.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_admission.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_derived.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables_prepare.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c" ${_p4_admission_sources})
target_include_directories(game_table_admission_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_compile_definitions(game_table_admission_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_table_admission_test m)
get_target_property(_p4_admission_runner game_table_admission_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_admission_runner)
  unset(_p4_admission_runner)
endif()
add_test(NAME game_table_admission COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/game_table_inputs_test.py"
  ${_p4_admission_runner} "$<TARGET_FILE:game_table_admission_test>" --driver-end)
set_tests_properties(game_table_admission PROPERTIES LABELS "host" TIMEOUT 240
  ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_TABLE_ADMISSION_BOUNDS_CASES=1")
unset(_p4_admission_sources)
unset(_p4_admission_runner)

# Actual loader bodies with scripted request/admission facts: refusal must
# precede both the ITEM copy and every packed slot store, without losing extents.
get_target_property(_p4_boundary_item_sources game_items_test SOURCES)
list(REMOVE_ITEM _p4_boundary_item_sources game_items_test.c "${P4_PS2RAM_SRC}")
add_executable(game_table_loader_boundary_test game_table_loader_boundary_test.c
  game_table_loader_boundary_support.c ${_p4_boundary_item_sources})
unset(_p4_boundary_item_sources)
target_compile_definitions(game_table_loader_boundary_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_table_loader_boundary_test m)
p4_ps2_exe(game_table_loader_boundary_test)
add_test(NAME game_table_loader_boundary COMMAND game_table_loader_boundary_test)
set_tests_properties(game_table_loader_boundary PROPERTIES SKIP_RETURN_CODE 77 LABELS "host" TIMEOUT 60)
