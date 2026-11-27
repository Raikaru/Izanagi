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

add_executable(golden_cpk_vfs_test golden_cpk_vfs_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_cpk_vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_tables.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}")
target_include_directories(golden_cpk_vfs_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(golden_cpk_vfs_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(golden_cpk_vfs_test m)
get_target_property(_p4_golden_vfs_runner golden_cpk_vfs_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_golden_vfs_runner)
  unset(_p4_golden_vfs_runner)
endif()
add_test(NAME golden_cpk_vfs COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_cpk_vfs_test.py"
  ${_p4_golden_vfs_runner} "$<TARGET_FILE:golden_cpk_vfs_test>" --driver-end)
set_tests_properties(golden_cpk_vfs PROPERTIES TIMEOUT 60)
add_test(NAME golden_cpk_vfs_steam COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_cpk_vfs_steam_test.py"
  ${_p4_golden_vfs_runner} "$<TARGET_FILE:golden_cpk_vfs_test>" --driver-end)
set_tests_properties(golden_cpk_vfs_steam PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
unset(_p4_golden_vfs_runner)
add_test(NAME golden_census COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_census_test.py")
set_tests_properties(golden_census PROPERTIES TIMEOUT 1800)

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
add_executable(golden_names_native_test golden_names_native_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_names_native.c")
target_include_directories(golden_names_native_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_names_native_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_names_native COMMAND golden_names_native_test)
set_tests_properties(golden_names_native PROPERTIES TIMEOUT 30)

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
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_image.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_vm_program.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_skill_convert.c"
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
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_pe_data.c"
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
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_pe_data.c"
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
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_recipe_runtime.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_recipe_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_recipe_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_recipe COMMAND golden_recipe_test)
set_tests_properties(golden_recipe PROPERTIES TIMEOUT 30)

add_executable(golden_fusion_pool_test golden_fusion_pool_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_recipe_runtime.c")
target_include_directories(golden_fusion_pool_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_fusion_pool_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_fusion_pool COMMAND golden_fusion_pool_test)

add_executable(golden_fusion_listing_test golden_fusion_listing_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_recipe_runtime.c")
target_include_directories(golden_fusion_listing_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_fusion_listing_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_fusion_listing COMMAND golden_fusion_listing_test)
set_tests_properties(golden_fusion_listing PROPERTIES TIMEOUT 30)
set_tests_properties(golden_fusion_pool PROPERTIES TIMEOUT 30)
if(NOT CMAKE_CROSSCOMPILING)
  add_test(NAME golden_recipe_ui COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_recipe_ui_test.py" "${CMAKE_C_COMPILER}")
  set_tests_properties(golden_recipe_ui PROPERTIES TIMEOUT 30 LABELS "host")
endif()

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

add_executable(golden_battle_rows_test golden_battle_rows_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_battle_rows.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_battle_rows_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/gport.c")
target_include_directories(golden_battle_rows_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_battle_rows_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(golden_battle_rows_test)
add_test(NAME golden_battle_rows COMMAND golden_battle_rows_test)
set_tests_properties(golden_battle_rows PROPERTIES TIMEOUT 30 LABELS "host")

add_executable(golden_item_convert_test golden_item_convert_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_item_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c")
target_include_directories(golden_item_convert_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_item_convert_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_item_convert COMMAND golden_item_convert_test)
set_tests_properties(golden_item_convert PROPERTIES TIMEOUT 30)

add_executable(golden_items_test golden_items_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_items.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_items_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c")
target_include_directories(golden_items_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_items_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_items COMMAND golden_items_test)
set_tests_properties(golden_items PROPERTIES TIMEOUT 30)
add_executable(golden_encounter_rates_test golden_encounter_rates_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_encounter_rates.c")
target_include_directories(golden_encounter_rates_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_encounter_rates_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_encounter_rates COMMAND golden_encounter_rates_test)
set_tests_properties(golden_encounter_rates PROPERTIES TIMEOUT 30)
if(NOT CMAKE_CROSSCOMPILING)
  add_test(NAME golden_encounter_consumer COMMAND "${P4_PYTHON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/golden_encounter_consumer_test.py" "${CMAKE_C_COMPILER}")
  set_tests_properties(golden_encounter_consumer PROPERTIES TIMEOUT 120 LABELS "host;port64")
endif()

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
add_executable(golden_fusion_runtime_test golden_fusion_runtime_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_fusion_runtime.c")
target_include_directories(golden_fusion_runtime_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_fusion_runtime_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_fusion_runtime COMMAND golden_fusion_runtime_test)
set_tests_properties(golden_fusion_runtime PROPERTIES TIMEOUT 30)
add_executable(golden_fusion_lookup_test golden_fusion_lookup_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_fusion_runtime.c")
target_include_directories(golden_fusion_lookup_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_fusion_lookup_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_fusion_lookup COMMAND golden_fusion_lookup_test)
set_tests_properties(golden_fusion_lookup PROPERTIES TIMEOUT 30)
add_executable(golden_fusion_gate_test golden_fusion_gate_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_fusion_runtime.c")
target_include_directories(golden_fusion_gate_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_fusion_gate_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_fusion_gate COMMAND golden_fusion_gate_test)
set_tests_properties(golden_fusion_gate PROPERTIES TIMEOUT 30)
add_executable(golden_fusion_list_test golden_fusion_list_test.c)
target_include_directories(golden_fusion_list_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_fusion_list_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_fusion_list COMMAND golden_fusion_list_test)
set_tests_properties(golden_fusion_list PROPERTIES TIMEOUT 30)
add_executable(golden_fusion_triangle_test golden_fusion_triangle_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_fusion_runtime.c")
target_include_directories(golden_fusion_triangle_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_fusion_triangle_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_fusion_triangle COMMAND golden_fusion_triangle_test)
set_tests_properties(golden_fusion_triangle PROPERTIES TIMEOUT 30)
add_executable(golden_fusion_candidate_test golden_fusion_candidate_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_fusion_runtime.c")
target_include_directories(golden_fusion_candidate_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_fusion_candidate_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_fusion_candidate COMMAND golden_fusion_candidate_test)
set_tests_properties(golden_fusion_candidate PROPERTIES TIMEOUT 30)


# Uses the owner's Steam install only when present; never writes converted data.
add_executable(golden_mode_steam_test golden_mode_steam_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_extra_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_identity.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_identity_source.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_image.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_projection.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_outputs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_vm_program.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_domains.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_inheritance.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_exp.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_pe_data.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_levels.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_skill_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_item_convert.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_items.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_encounter_rates.c"
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

add_executable(golden_c03_guard_test golden_c03_guard_test.c ${P4_GUEST_TEST_SRC}
  "${CMAKE_SOURCE_DIR}/src/platform/golden/c03_guard.c")
target_include_directories(golden_c03_guard_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(golden_c03_guard_test m)
foreach(case admitted unknown neighbor ps2 stack)
  add_test(NAME golden_c03_${case} COMMAND golden_c03_guard_test ${case})
  set_tests_properties(golden_c03_${case} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
endforeach()
add_test(NAME golden_c03_alias_audit COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/golden_c03_alias_audit_test.py")
add_executable(golden_guest_skill_name_test golden_guest_skill_name_test.c)
target_include_directories(golden_guest_skill_name_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_guest_skill_name_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_guest_skill_name COMMAND golden_guest_skill_name_test)
set_tests_properties(golden_guest_skill_name PROPERTIES TIMEOUT 30)

add_executable(golden_tag_expand_test golden_tag_expand_test.c)
target_include_directories(golden_tag_expand_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_tag_expand_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_tag_expand COMMAND golden_tag_expand_test)
set_tests_properties(golden_tag_expand PROPERTIES TIMEOUT 30)

# These host witnesses compile and execute i386 code with SSE2, not native ARM.
if(NOT CMAKE_CROSSCOMPILING AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i[3-6]86)$")
  add_test(NAME golden_model_rows COMMAND "${P4_PYTHON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/golden_model_rows_test.py" "${CMAKE_C_COMPILER}")
  set_tests_properties(golden_model_rows PROPERTIES TIMEOUT 30 LABELS "host")
  add_test(NAME golden_skill_flags COMMAND "${P4_PYTHON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/golden_skill_flags_test.py" "${CMAKE_C_COMPILER}")
  set_tests_properties(golden_skill_flags PROPERTIES TIMEOUT 30 LABELS "host")
  add_test(NAME golden_battle_lifetime COMMAND "${P4_PYTHON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/golden_battle_lifetime_test.py" "${CMAKE_C_COMPILER}")
  set_tests_properties(golden_battle_lifetime PROPERTIES TIMEOUT 30 LABELS "host")
  add_test(NAME golden_interception COMMAND "${P4_PYTHON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/golden_interception_test.py" "${CMAKE_C_COMPILER}")
  set_tests_properties(golden_interception PROPERTIES TIMEOUT 30 LABELS "host")
endif()
# Golden AI semantic engine: no retail bytes, VM or live guest memory required.
add_executable(golden_ai_rng_test golden_ai_rng_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_ai_rng.c")
target_include_directories(golden_ai_rng_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_ai_rng_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(golden_ai_rng_test m)
add_test(NAME golden_ai_rng COMMAND golden_ai_rng_test)
set_tests_properties(golden_ai_rng PROPERTIES TIMEOUT 30)

add_executable(golden_ai_test golden_ai_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_ai.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_ai_rng.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_bf.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_vm.c")
target_include_directories(golden_ai_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_ai_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_ai COMMAND golden_ai_test)
set_tests_properties(golden_ai PROPERTIES TIMEOUT 30)

add_executable(golden_bf_tables_test golden_bf_tables_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_bf.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_ai.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_ai_rng.c")
target_include_directories(golden_bf_tables_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_bf_tables_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_bf_tables COMMAND golden_bf_tables_test)
set_tests_properties(golden_bf_tables PROPERTIES TIMEOUT 30)

add_executable(golden_vm_test golden_vm_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_vm.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_vm_program.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_bf.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_ai.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_ai_rng.c")
target_include_directories(golden_vm_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_vm_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(golden_vm_test m)
add_test(NAME golden_vm COMMAND golden_vm_test)
set_tests_properties(golden_vm PROPERTIES TIMEOUT 30)

add_executable(golden_ai_knowledge_test golden_ai_knowledge_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_ai_knowledge.c")
target_include_directories(golden_ai_knowledge_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_ai_knowledge_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME golden_ai_knowledge COMMAND golden_ai_knowledge_test)
set_tests_properties(golden_ai_knowledge PROPERTIES TIMEOUT 30)

add_executable(golden_levels_test golden_levels_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_levels.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_pe_data.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c")
target_include_directories(golden_levels_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_levels_test PRIVATE ${P4_PORT_DEFINES} P4_GOLDEN_LEVELS_TEST)
add_test(NAME golden_levels COMMAND golden_levels_test)
set_tests_properties(golden_levels PROPERTIES TIMEOUT 30)
