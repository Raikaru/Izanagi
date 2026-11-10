# Native import transactions and format-boundary fixtures; no retail data.
add_executable(save_import_test save_import_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/save_import.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/save_image.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/save_transaction.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save_custody.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save_hash.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/mod_save_format.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/import_parse.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/import_card.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/import_max.c"
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/import_cbs.c"
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz_tinfl.c" "${P4_OS_SRC}")
target_link_libraries(save_import_test m)
add_dependencies(mod_save_transport_test save_import_test)
add_executable(import_cbs_test import_cbs_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/memcard/import_cbs.c"
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz_tinfl.c")
add_test(NAME import_cbs COMMAND import_cbs_test)
if(P4_PYTHON AND NOT P4_WIN32)
  add_test(NAME save_import_formats COMMAND ${P4_PYTHON}
    "${CMAKE_SOURCE_DIR}/tests/save_import_fixtures.py"
    --driver $<TARGET_FILE:save_import_test>)
  # The full transaction/rejection matrix includes sequential Wine startups.
  set_tests_properties(save_import_formats PROPERTIES TIMEOUT 600)
endif()
add_test(NAME save_import_load COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/save_import_load_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(save_import_load PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)

# Picker cancellation/completion uses the real native Config task, not an
# embedded standalone canvas; all save output belongs to the private gate.
add_test(NAME import_settings_native COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/import_settings_native_gate.sh" $<TARGET_FILE:p4pc>)
set_tests_properties(import_settings_native PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;scenes" TIMEOUT 600)
