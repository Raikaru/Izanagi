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

add_executable(game_mode_test game_mode_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/config/game_mode.c"
  "${CMAKE_SOURCE_DIR}/src/platform/config/p4cfg.c" "${P4_OS_SRC}")
target_link_libraries(game_mode_test m ${P4_SDL3_LIB})
target_compile_definitions(game_mode_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_mode_default COMMAND game_mode_test)
add_test(NAME game_mode_golden COMMAND game_mode_test golden)
add_test(NAME game_mode_invalid COMMAND game_mode_test invalid)
set_tests_properties(game_mode_default game_mode_golden game_mode_invalid PROPERTIES TIMEOUT 30)

add_executable(golden_domains_test golden_domains_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_domains_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_domains_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_domains COMMAND golden_domains_test)
set_tests_properties(golden_domains PROPERTIES TIMEOUT 30)
add_executable(golden_mode_declarations_test golden_mode_declarations_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_declarations.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_mode_declarations_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_mode_declarations_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_mode_declarations COMMAND golden_mode_declarations_test)
set_tests_properties(golden_mode_declarations PROPERTIES TIMEOUT 30)

add_executable(golden_mode_declare_builder_test golden_mode_declare_builder_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_declare_builder.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_declare_cached.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_declarations.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_mode_declare_builder_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_mode_declare_builder_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_mode_declare_builder COMMAND golden_mode_declare_builder_test)
set_tests_properties(golden_mode_declare_builder PROPERTIES TIMEOUT 30)

add_executable(golden_mode_projection_test golden_mode_projection_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_projection.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_skill_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_enemy_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_item_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_persona_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_encounter_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_names.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_mode_projection_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_mode_projection_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_mode_projection COMMAND golden_mode_projection_test)
set_tests_properties(golden_mode_projection PROPERTIES TIMEOUT 30)

add_executable(golden_mode_identity_test golden_mode_identity_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_identity.c")
target_include_directories(golden_mode_identity_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_mode_identity_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_mode_identity COMMAND golden_mode_identity_test)
set_tests_properties(golden_mode_identity PROPERTIES TIMEOUT 30)

add_executable(golden_mode_boot_test golden_mode_boot_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_boot.c")
target_include_directories(golden_mode_boot_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_mode_boot_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_mode_boot COMMAND golden_mode_boot_test)
set_tests_properties(golden_mode_boot PROPERTIES TIMEOUT 30)

add_executable(golden_mode_outputs_test golden_mode_outputs_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_outputs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c")
target_include_directories(golden_mode_outputs_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_mode_outputs_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_mode_outputs COMMAND golden_mode_outputs_test)
set_tests_properties(golden_mode_outputs PROPERTIES TIMEOUT 30)

add_executable(golden_loader_policy_test golden_loader_policy_test.c)
target_include_directories(golden_loader_policy_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_loader_policy_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_loader_policy COMMAND golden_loader_policy_test)
set_tests_properties(golden_loader_policy PROPERTIES TIMEOUT 30)

add_executable(golden_cache_attest_test golden_cache_attest_test.c)
target_include_directories(golden_cache_attest_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_cache_attest_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_cache_attest COMMAND golden_cache_attest_test)
set_tests_properties(golden_cache_attest PROPERTIES TIMEOUT 30)


add_executable(golden_cache_test golden_cache_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_cache.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c" "${P4_OS_SRC}")
target_include_directories(golden_cache_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_cache_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(golden_cache_test m)
add_test(NAME golden_cache COMMAND golden_cache_test)
set_tests_properties(golden_cache PROPERTIES TIMEOUT 30)

# Real cache and sealed-input path; family registries are recording test sinks.
add_executable(golden_mode_pipeline_test golden_mode_pipeline_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_boot.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_cache_publish.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_declare_cached.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_declare_builder.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_declarations.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_skill_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_exp.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_table_cache.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_cache.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c" "${P4_OS_SRC}")
target_include_directories(golden_mode_pipeline_test PRIVATE
  "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(golden_mode_pipeline_test PRIVATE ${P4_PORT_DEFINES} P4_GOLDEN_STANDALONE_FIXTURE P4_GOLDEN_EXP_TEST)
target_link_libraries(golden_mode_pipeline_test m)
add_test(NAME golden_mode_pipeline COMMAND golden_mode_pipeline_test)
set_tests_properties(golden_mode_pipeline PROPERTIES TIMEOUT 120)

add_executable(golden_table_cache_test golden_table_cache_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_table_cache.c")
target_include_directories(golden_table_cache_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_table_cache_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_table_cache COMMAND golden_table_cache_test)
set_tests_properties(golden_table_cache PROPERTIES TIMEOUT 30)

add_test(NAME golden_cache_package_guard COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/check_package_golden_cache_test.py")
set_tests_properties(golden_cache_package_guard PROPERTIES LABELS "tools;port64" TIMEOUT 30)

add_executable(golden_exp_test golden_exp_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_exp.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c")
target_include_directories(golden_exp_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_exp_test PRIVATE ${P4_PORT_DEFINES} P4_GOLDEN_EXP_TEST)
add_test(NAME golden_exp COMMAND golden_exp_test)
set_tests_properties(golden_exp PROPERTIES TIMEOUT 30)

add_executable(golden_inheritance_test golden_inheritance_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_inheritance.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c")
target_include_directories(golden_inheritance_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_inheritance_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_inheritance COMMAND golden_inheritance_test)
set_tests_properties(golden_inheritance PROPERTIES TIMEOUT 30)

add_executable(golden_recipe_test golden_recipe_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_recipe.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_recipe_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_recipe_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_recipe COMMAND golden_recipe_test)
set_tests_properties(golden_recipe PROPERTIES TIMEOUT 30)

add_executable(golden_cmm_test golden_cmm_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_cmm.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c")
target_include_directories(golden_cmm_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_cmm_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_cmm COMMAND golden_cmm_test)
set_tests_properties(golden_cmm PROPERTIES TIMEOUT 30)

add_executable(golden_skill_convert_test golden_skill_convert_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_skill_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_skill_convert_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_skill_convert_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_skill_convert COMMAND golden_skill_convert_test)
set_tests_properties(golden_skill_convert PROPERTIES TIMEOUT 30)

add_executable(golden_item_convert_test golden_item_convert_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_item_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_item_convert_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_item_convert_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_item_convert COMMAND golden_item_convert_test)
set_tests_properties(golden_item_convert PROPERTIES TIMEOUT 30)

add_executable(golden_names_test golden_names_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_names.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_names_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_names_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_names COMMAND golden_names_test)
set_tests_properties(golden_names PROPERTIES TIMEOUT 30)

add_executable(golden_encounter_convert_test golden_encounter_convert_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_encounter_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_encounter_convert_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_encounter_convert_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_encounter_convert COMMAND golden_encounter_convert_test)
set_tests_properties(golden_encounter_convert PROPERTIES TIMEOUT 30)

add_executable(golden_persona_convert_test golden_persona_convert_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_persona_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_persona_convert_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_persona_convert_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_persona_convert COMMAND golden_persona_convert_test)
set_tests_properties(golden_persona_convert PROPERTIES TIMEOUT 30)

add_executable(golden_enemy_convert_test golden_enemy_convert_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_enemy_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_enemy_convert_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_enemy_convert_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_enemy_convert COMMAND golden_enemy_convert_test)
set_tests_properties(golden_enemy_convert PROPERTIES TIMEOUT 30)

add_executable(golden_fusion_convert_test golden_fusion_convert_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_fusion_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_fusion_convert_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_fusion_convert_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_fusion_convert COMMAND golden_fusion_convert_test)
set_tests_properties(golden_fusion_convert PROPERTIES TIMEOUT 30)

# Uses the owner's Steam install only when present; never writes converted data.
add_executable(golden_mode_steam_test golden_mode_steam_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_extra_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_identity.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_identity_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_inheritance.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_exp.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_skill_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_item_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_names.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_encounter_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_persona_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_enemy_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_fusion_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_fusion_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_cmm.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_cmm_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_recipe.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_inheritance_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c")
target_include_directories(golden_mode_steam_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_mode_steam_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_mode_steam COMMAND golden_mode_steam_test)
set_tests_properties(golden_mode_steam PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 LABELS "local")
