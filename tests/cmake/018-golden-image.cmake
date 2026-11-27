# Actual cache, CPU builder/source seal, native-use history and guest arena.
get_target_property(_p4_image_services game_table_builder_test SOURCES)
list(REMOVE_ITEM _p4_image_services game_table_builder_test.c
  "${CMAKE_CURRENT_SOURCE_DIR}/game_table_builder_test.c")
get_target_property(_p4_image_readers golden_mode_steam_test SOURCES)
list(REMOVE_ITEM _p4_image_readers golden_mode_steam_test.c
  "${CMAKE_CURRENT_SOURCE_DIR}/golden_mode_steam_test.c")
set(_p4_image_sources golden_mode_image_test.c ${_p4_image_services} ${_p4_image_readers}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_resources.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_image.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_mode_image_bind.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_fusion_runtime.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_recipe_runtime.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_cache.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_table_cache.c")
list(REMOVE_DUPLICATES _p4_image_sources)
set(_p4_exp_consumer "${CMAKE_CURRENT_BINARY_DIR}/golden_exp_consumer.c")
add_custom_command(OUTPUT "${_p4_exp_consumer}"
  COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_exp_consumer_fixture.py"
    "${P4_DECOMP}" "${_p4_exp_consumer}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/golden_exp_consumer_fixture.py"
    "${P4_DECOMP}/src/Main/g_data.c" "${CMAKE_SOURCE_DIR}/config/mirror_fixes.txt"
  VERBATIM)
set(_p4_literal_consumers "${CMAKE_CURRENT_BINARY_DIR}/golden_literal_consumers_generated.inc")
add_custom_command(OUTPUT "${_p4_literal_consumers}"
  COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_literal_consumer_fixture.py"
    "${P4_DECOMP}" "${_p4_literal_consumers}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/golden_literal_consumer_fixture.py"
    "${CMAKE_SOURCE_DIR}/config/mirror_fixes.txt"
    "${P4_DECOMP}/src/datCalendar_inferred_001104a0.c"
    "${P4_DECOMP}/src/unplaced/unplaced_inferred_00110850.c"
    "${P4_DECOMP}/src/Event/mt_misc_inferred_002981f0.c"
    "${P4_DECOMP}/src/Event/Fcl/y_fclCombine.c"
    "${P4_DECOMP}/src/unplaced/unplaced_inferred_00311900.c"
    "${P4_DECOMP}/src/Battle/btlMain_inferred_001b5ec0.c"
    "${P4_DECOMP}/src/unplaced/unplaced_inferred_00383c00.c"
    "${P4_DECOMP}/src/Script/scrScriptProcess.c"
    "${P4_DECOMP}/src/Graphics/Effect/tnEffect.c"
    "${P4_DECOMP}/src/Graphics/Model/mdlEffect_inferred_004b11d0.c"
    "${P4_DECOMP}/src/cutinDraw.c"
    "${P4_DECOMP}/src/cmmRankUp.c"
    "${P4_DECOMP}/src/unplaced/unplaced_inferred_00210c70.c"
    "${P4_DECOMP}/src/Scene/mt_scene.c"
    "${P4_DECOMP}/src/Kosaka/Field/k_sceneDraw_inferred_0028c3f0.c"
    "${P4_DECOMP}/src/Event/Fcl/y_fclCombineDraw.c"
    "${P4_DECOMP}/src/unplaced/unplaced_inferred_003136b0.c"
    "${P4_DECOMP}/src/unplaced/unplaced_inferred_002e78a0.c"
    "${P4_DECOMP}/src/Yajima/y_misc_inferred.c"
    "${P4_DECOMP}/src/evtMisc.c"
    "${P4_DECOMP}/src/Scene/mt_sceneFunc_inferred_0026cef0.c"
    "${P4_DECOMP}/src/Kosaka/k_testMenu.c"
  VERBATIM)
add_executable(golden_mode_image_test ${_p4_image_sources} "${_p4_exp_consumer}")
target_sources(golden_mode_image_test PRIVATE golden_literal_consumers_test.c "${_p4_literal_consumers}")
target_include_directories(golden_mode_image_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(golden_mode_image_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
  "${CMAKE_SOURCE_DIR}/src/platform/mods" "${CMAKE_SOURCE_DIR}/src/platform/memcard"
  "${CMAKE_SOURCE_DIR}/third_party/yxml")
target_compile_definitions(golden_mode_image_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(golden_mode_image_test)
target_link_libraries(golden_mode_image_test m)
add_test(NAME golden_mode_image COMMAND golden_mode_image_test)
add_test(NAME golden_mode_image_steam COMMAND golden_mode_image_test steam)
set_tests_properties(golden_mode_image PROPERTIES TIMEOUT 60)
set_tests_properties(golden_mode_image_steam PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77 LABELS "local")
unset(_p4_image_services)
unset(_p4_image_readers)
unset(_p4_image_sources)
