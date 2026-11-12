# [resource-model] admission (src/platform/mods/mod_models_boot.c): real selected provider plans, the
# runtime glTF cooker, its content-hash cache and the frozen override table; a developer tree stands in
# for the disc, and Python writes only synthetic packages (tools/models/make_test_weapon.py).
add_executable(mod_models_boot mod_models_boot_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(mod_models_boot PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform/mods")
target_compile_options(mod_models_boot PRIVATE -fno-builtin)
target_compile_definitions(mod_models_boot PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_models_boot m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(mod_models_boot)
if(TARGET p4_dlls)
  add_dependencies(mod_models_boot p4_dlls)
endif()
get_target_property(_p4_model_boot_runner mod_models_boot CROSSCOMPILING_EMULATOR)
if(NOT _p4_model_boot_runner)
  unset(_p4_model_boot_runner)
endif()
add_test(NAME mod_models_boot COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mod_models_boot_test.py"
  ${_p4_model_boot_runner} "$<TARGET_FILE:mod_models_boot>" --driver-end)
set_tests_properties(mod_models_boot PROPERTIES LABELS host TIMEOUT 300)
unset(_p4_model_boot_runner)
