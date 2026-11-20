# The actual shared native precedence body with scripted query facts only.
# Native battle, encoded UI and sidecar qualification remain separate gates.
get_target_property(_p4_element_input_sources mod_files_test SOURCES)
list(REMOVE_ITEM _p4_element_input_sources mod_files_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}")
add_executable(game_elements_affinity_test game_elements_affinity_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables_prepare.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_elements_affinity.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_elements_action.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_elements_native.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_skills.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_text.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mod_resources.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_damage_cover.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_passive_grant.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_damage_stage.c" ${_p4_element_input_sources} ${P4_GUEST_TEST_SRC})
# Real family/text/resource services; the fixture fail-stops if raster work is
# attempted. Actual RW/GL behavior remains the separate receiver/render gate.

# Exact issued inputs and real owned definitions; original equipment is a probe.
add_executable(game_elements_action_test game_elements_action_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_elements_action.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_skills.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_items.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_table_inputs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/content_digest.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_tables.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_text.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs.c" ${_p4_element_input_sources} ${P4_GUEST_TEST_SRC})
foreach(_p4_element_case affinity action)
  set(_p4_element_target "game_elements_${_p4_element_case}_test")
  target_compile_definitions(${_p4_element_target} PRIVATE ${P4_PORT_DEFINES})
  target_include_directories(${_p4_element_target} PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/files"
    "${CMAKE_SOURCE_DIR}/src/platform/mods")
  target_link_libraries(${_p4_element_target} m)
  p4_ps2_exe(${_p4_element_target})
  get_target_property(_p4_element_runner ${_p4_element_target} CROSSCOMPILING_EMULATOR)
  if(NOT _p4_element_runner)
    unset(_p4_element_runner)
  endif()
  add_test(NAME game_elements_${_p4_element_case} COMMAND "${P4_PYTHON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/game_elements_inputs_test.py"
    ${_p4_element_runner} "$<TARGET_FILE:${_p4_element_target}>" --driver-end)
  set_tests_properties(game_elements_${_p4_element_case} PROPERTIES SKIP_RETURN_CODE 77
    LABELS "host" TIMEOUT 120 ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0")
endforeach()
unset(_p4_element_input_sources)
unset(_p4_element_case)
unset(_p4_element_target)
unset(_p4_element_runner)

# Actual amount and formula overrides with explicit engine facts. Synthetic
# low-address banks are a calculation witness, not a gameplay qualification.
# The final-damage and recovery stages are linked as built: retail with no
# modifier, and the challenge + hard-mode composition over the real equation.
set(_p4_amount_stage_sources "${CMAKE_SOURCE_DIR}/src/platform/mods/game_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_damage_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_recovery_stage.c")
add_executable(game_elements_amount_test game_elements_amount_test.c ${_p4_amount_stage_sources})
target_include_directories(game_elements_amount_test PRIVATE "${P4_DECOMP}/include")
target_compile_definitions(game_elements_amount_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_elements_amount_test m)
add_test(NAME game_elements_amount COMMAND game_elements_amount_test)
set_tests_properties(game_elements_amount PROPERTIES SKIP_RETURN_CODE 77 LABELS "host" TIMEOUT 60)

# Final-damage stage order, single conversion, clamp and refusals.
add_executable(game_damage_stage_test game_damage_stage_test.c ${_p4_amount_stage_sources})
target_compile_definitions(game_damage_stage_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_damage_stage_test m)
add_test(NAME game_damage_stage COMMAND game_damage_stage_test)
set_tests_properties(game_damage_stage PROPERTIES LABELS "host" TIMEOUT 60)

# Recovery, skill cost, status/KO chance, ailment wear-off, buff, reward and
# unit stages on the shared registry, and the per-battle affliction count.
add_executable(game_amount_stages_test game_amount_stages_test.c ${_p4_amount_stage_sources}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_cost_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_chance_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_wearoff_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_buff_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_reward_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_unit_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_afflictions.c"
  "${CMAKE_SOURCE_DIR}/src/platform/battle_sim/events.c")
target_compile_definitions(game_amount_stages_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_amount_stages_test m)
add_test(NAME game_amount_stages COMMAND game_amount_stages_test)
set_tests_properties(game_amount_stages PROPERTIES LABELS "host" TIMEOUT 60)

# Actual hit resolver with scripted engine facts; compares retail and tagged
# physical/magic branches, never native battle admission. The status/KO
# chance and hit stages are linked as built.
add_executable(game_elements_hit_test game_elements_hit_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_damage_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_chance_stage.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_hit_stage.c")
target_compile_definitions(game_elements_hit_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(game_elements_hit_test m)
add_test(NAME game_elements_hit COMMAND game_elements_hit_test)
set_tests_properties(game_elements_hit PROPERTIES SKIP_RETURN_CODE 77 LABELS "host" TIMEOUT 60)

# The passive grant table and the game's passive checks over it (the static
# datCalcHasRequestedSkill/datCalcHasFixedSkill/cmpSkillListContains bodies and
# Growth in 0021e110) with scripted engine facts, never native battle admission.
add_executable(game_passive_grant_test game_passive_grant_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_passive_grant.c")
target_compile_definitions(game_passive_grant_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_passive_grant COMMAND game_passive_grant_test)
set_tests_properties(game_passive_grant PROPERTIES SKIP_RETURN_CODE 77 LABELS "host" TIMEOUT 60)

# Actual 001F14F0 knowledge-gate route body with scripted frames and a scripted
# knowledge owner; call order and refusal only, never native battle admission.
add_executable(game_elements_route_test game_elements_route_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_elements_route.c")
target_compile_definitions(game_elements_route_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_elements_route COMMAND game_elements_route_test)
set_tests_properties(game_elements_route PROPERTIES LABELS "host" TIMEOUT 60)
