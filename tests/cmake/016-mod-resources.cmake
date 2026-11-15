# Pure token/ownership fixture: backend is intentionally mocked here.
add_executable(mod_resources_test mod_resources_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_resources.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(mod_resources_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_resources_test m)
p4_ps2_exe(mod_resources_test)
add_test(NAME mod_resources COMMAND mod_resources_test)
set_tests_properties(mod_resources PROPERTIES SKIP_RETURN_CODE 77)

# Actual game RW/plugin initialization and authored GL raster upload, not a
# fabricated freelist or host-only raster. Uses only synthetic authored pixels.
add_executable(mod_resources_render mod_resources_render_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(mod_resources_render PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/render")
target_compile_options(mod_resources_render PRIVATE -fno-builtin)
target_compile_definitions(mod_resources_render PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_resources_render m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(mod_resources_render)
add_test(NAME mod_resources_render COMMAND mod_resources_render)
set_tests_properties(mod_resources_render PROPERTIES SKIP_RETURN_CODE 77 LABELS gl TIMEOUT 120)
# The same authored raster on the battle simulator's no-device backend: no GL context, shadow pixels only.
add_test(NAME mod_resources_render_no_device COMMAND mod_resources_render no-device)
set_tests_properties(mod_resources_render_no_device PROPERTIES SKIP_RETURN_CODE 77 LABELS "host" TIMEOUT 120)

# Real selected provider plans, bounded PNG decoding, early/late products and
# the actual RW resource backend. Python makes only synthetic authored inputs.
add_executable(mod_resources_boot mod_resources_boot_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(mod_resources_boot PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/render" "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_compile_options(mod_resources_boot PRIVATE -fno-builtin)
target_compile_definitions(mod_resources_boot PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_resources_boot m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(mod_resources_boot)
if(TARGET p4_dlls)
  add_dependencies(mod_resources_boot p4_dlls)
endif()
get_target_property(_p4_resource_boot_runner mod_resources_boot CROSSCOMPILING_EMULATOR)
if(NOT _p4_resource_boot_runner)
  unset(_p4_resource_boot_runner)
endif()
add_test(NAME mod_resources_boot COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mod_resources_boot_test.py"
  ${_p4_resource_boot_runner} "$<TARGET_FILE:mod_resources_boot>" --driver-end)
set_tests_properties(mod_resources_boot PROPERTIES LABELS gl TIMEOUT 180)
add_test(NAME mod_resources_boot_cpu COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mod_resources_boot_test.py"
  ${_p4_resource_boot_runner} "$<TARGET_FILE:mod_resources_boot>" --driver-end
  ResourceBoot.test_early_decode_without_graphics_context
  ResourceBoot.test_early_ownership_conflict_without_graphics_context
  ResourceBoot.test_late_failure_keeps_real_selected_provider
  ResourceBoot.test_invalid_or_changed_source_refused_before_exposure
  ResourceBoot.test_item_ui_missing_or_wrong_revision_refused_early)
set_tests_properties(mod_resources_boot_cpu PROPERTIES LABELS host TIMEOUT 180)
unset(_p4_resource_boot_runner)
