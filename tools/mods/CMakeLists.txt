# Native core, generated game bridge and optional Luau binding. All are compiled
# outside guest-pointer/EE-float launchers; the provider alone owns C++/the VM.
set_property(DIRECTORY PROPERTY COMPILE_OPTIONS "")
set_property(DIRECTORY PROPERTY LINK_OPTIONS "")
if(P4_APPLE)
  # macOS's link requirement is the platform's, not a profile flag (cmake/sources/050-platform-flags.cmake: the weak kwlnTaskDumpAll)
  set_property(DIRECTORY PROPERTY LINK_OPTIONS ${P4_TARGET_LINK_FLAGS})
endif()
# Their only launcher is the compiler cache, when the configure line names ccache: the generated bridge (mod_bridges.c, an entry guard per
# hooked function) takes half a minute per profile to compile, and its bytes rarely change.
set(P4_MOD_LAUNCHER "")
if(CMAKE_C_COMPILER_LAUNCHER MATCHES "(^|/)ccache(\\.exe)?$")
  set(P4_MOD_LAUNCHER "${CMAKE_C_COMPILER_LAUNCHER}")
endif()
get_filename_component(P4_MOD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
include("${P4_MOD_ROOT}/cmake/wine-tests.cmake")
set(P4_MOD_BINDING_SOURCES)
foreach(unit lua_api lua_runtime lua_memory lua_hooks lua_events lua_input lua_files lua_settings lua_lifecycle lua_contract lua_artifacts lua_save failure_script)
  list(APPEND P4_MOD_BINDING_SOURCES "${P4_MOD_ROOT}/src/platform/mods/${unit}.c")
endforeach()

function(p4_mod_profile suffix)
  set(flags ${ARGN})
  set(model_definitions ${P4_PORT_DEFINES})
  if(NOT suffix STREQUAL "")
    list(APPEND model_definitions P4_GUEST64=1 P4_GUEST_CHECK=1)
  endif()
  add_library(p4_mod_core${suffix} STATIC ${P4_MOD_CORE_SOURCES})
  # Keep the game descriptors separate: witnesses provide their own real generated
  # fixture descriptors without pulling unresolved full-game functions into tests.
  set_source_files_properties("${P4_MOD_BRIDGES}" PROPERTIES GENERATED TRUE)
  set(bridges "${P4_MOD_BRIDGES}")
  if(NOT P4PC_MOD_SUPPORT)
    # Without entry guards the generated table is empty and only the core's hook
    # lookup reads it. The overrides' always-compiled seams (p4.authorize and
    # p4.battle.formation masks) pull authorize.c, whose failure path pulls this
    # core's hooks.c after the linker has passed the game archive, so the table
    # lives in the core archive here. A witness that defines its own table never
    # pulls this member.
    target_sources(p4_mod_core${suffix} PRIVATE "${P4_MOD_BRIDGES}")
    add_dependencies(p4_mod_core${suffix} p4_mirror)
    set(bridges)
  endif()
  # field_host.c is the field trigger slot overrides/001747d0.c calls in every
  # build; only the Luau services (game_party.c) ever arm it.
  add_library(p4_mod_game${suffix} STATIC ${bridges}
    "${P4_MOD_ROOT}/src/platform/mods/native_bindings.c"
    "${P4_MOD_ROOT}/src/platform/mods/authorize.c"
    "${P4_MOD_ROOT}/src/platform/mods/field_host.c")
  add_dependencies(p4_mod_game${suffix} p4_mirror)
  set(native_targets p4_mod_core${suffix} p4_mod_game${suffix})
  if(P4_MOD_LUAU_ACTIVE)
    if(NOT TARGET p4_luau${suffix})
      message(FATAL_ERROR "Mod binding profile${suffix} has no matching Luau provider")
    endif()
    add_library(p4_mod_luau${suffix} STATIC ${P4_MOD_BINDING_SOURCES})
    list(APPEND native_targets p4_mod_luau${suffix})
    # Game-only UI uses the real settings/hub backend. Core VM witnesses do
    # not link this provider and do not advertise a pretend UI implementation.
    target_sources(p4_mod_game${suffix} PRIVATE "${P4_MOD_ROOT}/src/platform/mods/lua_ui.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_overlay.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_rows.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_authorize.c" "${P4_MOD_ROOT}/src/platform/mods/lua_stages.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_passive.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_services.c"
      "${P4_MOD_ROOT}/src/platform/mods/game_services.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_battle_present.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_elements.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_data.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_data_edit.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_audio.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_tables.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_battle.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_battle_substitute.c"
      "${P4_MOD_ROOT}/src/platform/mods/battle_menu.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_party.c"
      "${P4_MOD_ROOT}/src/platform/mods/battle_semantic.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_semantic_events.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_battle_decide.c"
      "${P4_MOD_ROOT}/src/platform/mods/game_inventory.c"
      "${P4_MOD_ROOT}/src/platform/mods/game_party.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_inventory.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_persona.c" "${P4_MOD_ROOT}/src/platform/mods/lua_story.c"
      "${P4_MOD_ROOT}/src/platform/mods/lua_forge.c" "${P4_MOD_ROOT}/src/platform/mods/lua_equipment.c")
    target_link_libraries(p4_mod_game${suffix} PUBLIC p4_mod_luau${suffix})
  endif()
  foreach(target IN LISTS native_targets)
    target_compile_options(${target} PRIVATE ${flags} -O2 -g1
      -fno-strict-aliasing -ffp-contract=off -ffunction-sections -fdata-sections)
    target_compile_definitions(${target} PUBLIC ${model_definitions})
    target_include_directories(${target} PUBLIC "${P4_MOD_ROOT}/include" PRIVATE "${P4_GEN}")
    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON C_COMPILER_LAUNCHER "${P4_MOD_LAUNCHER}")
  endforeach()
  target_link_libraries(p4_mod_game${suffix} PUBLIC p4_mod_core${suffix})
  p4_host_adapter_register("${suffix}" C p4_mod_game${suffix})
  if(NOT P4_MOD_LUAU_ACTIVE)
    return()
  endif()
  target_compile_definitions(p4_mod_luau${suffix} PUBLIC P4_MOD_LUAU=1)
  target_link_libraries(p4_mod_luau${suffix} PUBLIC p4_mod_core${suffix} p4_luau${suffix})
  p4_host_adapter_register("${suffix}" CXX p4_mod_luau${suffix})

  # Real interpreter + production cooperative scheduler. Only hardware/game
  # snapshots and uninitialized heap/image evidence are explicit fixture inputs.
  set(os_source "${P4_OS_SRC}")
  if(suffix STREQUAL "64w")
    string(REGEX REPLACE "os/os_posix\\.c$" "os/os_win32.c" os_source "${os_source}")
  endif()
  set(core_sources)
  foreach(unit mods package_registry package_memory mod_setting_store lifecycle mod_artifacts compendium file_index)
    list(APPEND core_sources "${P4_MOD_ROOT}/src/platform/mods/${unit}.c")
  endforeach()
  add_executable(mod_luau_test${suffix} "${P4_MOD_ROOT}/tests/mod_luau_test.c" ${P4_MOD_TEST_GAME}
    ${core_sources} "${P4_MOD_ROOT}/third_party/yxml/yxml.c"
    "${P4_MOD_ROOT}/src/platform/mods/lua_battle_substitute.c"
    "${P4_MOD_ROOT}/src/platform/memcard/mod_save.c"
    "${P4_MOD_ROOT}/src/platform/memcard/mod_save_hash.c"
    "${P4_MOD_ROOT}/src/platform/memcard/mod_save_format.c"
    "${P4_MOD_ROOT}/src/platform/memcard/save_image.c"
    "${P4_MOD_ROOT}/src/platform/files/vfs.c" "${P4_MOD_ROOT}/src/platform/files/iso9660.c"
    "${P4_MOD_ROOT}/tests/guest_test.c" "${P4_MOD_ROOT}/src/platform/gport.c"
    "${P4_MOD_ROOT}/src/platform/guest.c" "${os_source}"
    "${P4_MOD_ROOT}/src/platform/ee_bytes.c" "${P4_MOD_ROOT}/src/platform/content_digest.c"
    "${P4_MOD_ROOT}/src/platform/kernel_thread.c" "${P4_MOD_ROOT}/src/platform/state_dump.c"
    "${P4_MOD_ROOT}/src/platform/ps2_kernel.c" "${P4_MOD_ROOT}/tests/state_dump_test_stubs.c"
    "${P4_MOD_ROOT}/src/platform/mods/lua_stages.c" "${P4_MOD_ROOT}/src/platform/mods/game_stage.c"
    "${P4_MOD_ROOT}/src/platform/mods/game_damage_stage.c" "${P4_MOD_ROOT}/src/platform/mods/game_recovery_stage.c"
    "${P4_MOD_ROOT}/src/platform/mods/game_cost_stage.c" "${P4_MOD_ROOT}/src/platform/mods/game_chance_stage.c"
    "${P4_MOD_ROOT}/src/platform/mods/game_wearoff_stage.c" "${P4_MOD_ROOT}/src/platform/mods/game_buff_stage.c"
    "${P4_MOD_ROOT}/src/platform/mods/game_reward_stage.c" "${P4_MOD_ROOT}/src/platform/mods/game_unit_stage.c"
    "${P4_MOD_ROOT}/src/platform/mods/game_hit_stage.c"
    "${P4_MOD_ROOT}/src/platform/mods/lua_passive.c" "${P4_MOD_ROOT}/src/platform/mods/game_passive_grant.c"
    "${P4_MOD_ROOT}/src/platform/mods/game_data.c" "${P4_MOD_ROOT}/tests/mod_passive_facts.c")
  add_dependencies(mod_luau_test${suffix} p4_mod_hook_fixture)
  target_include_directories(mod_luau_test${suffix} PRIVATE "${P4_MOD_ROOT}/tests"
    "${P4_MOD_ROOT}/src/platform/mods" "${P4_MOD_ROOT}/src/platform/files")
  target_compile_options(mod_luau_test${suffix} PRIVATE ${flags} -O2 -g1 -fno-strict-aliasing -ffp-contract=off)
  target_link_options(mod_luau_test${suffix} PRIVATE ${flags})
  target_link_libraries(mod_luau_test${suffix} PRIVATE p4_mod_luau${suffix} m)
  set_target_properties(mod_luau_test${suffix} PROPERTIES C_COMPILER_LAUNCHER "${P4_MOD_LAUNCHER}" LINKER_LANGUAGE CXX)
  if(suffix STREQUAL "")
    p4_ps2_exe(mod_luau_test${suffix})
  endif()
  get_target_property(runner mod_luau_test${suffix} CROSSCOMPILING_EMULATOR)
  if(NOT runner)
    set(runner "")
  endif()
  if(suffix STREQUAL "64w")
    set_target_properties(mod_luau_test${suffix} PROPERTIES SUFFIX ".exe" LINK_DEPENDS_USE_LINKER FALSE)
    find_program(P4_MOD_WINE NAMES wine64 wine REQUIRED)
    set(runner "${P4_MOD_WINE}")
  endif()
  # Real VM allocation failures at the semantic userdata and registry slots.
  add_executable(lua_semantic_subscription_test${suffix}
    "${P4_MOD_ROOT}/tests/lua_semantic_subscription_test.c"
    "${P4_MOD_ROOT}/src/platform/mods/battle_semantic.c")
  target_include_directories(lua_semantic_subscription_test${suffix} PRIVATE
    "${P4_MOD_ROOT}/include" "${P4_MOD_ROOT}/src/platform/mods")
  target_compile_options(lua_semantic_subscription_test${suffix} PRIVATE ${flags} -O2 -g1 -UNDEBUG)
  target_compile_definitions(lua_semantic_subscription_test${suffix} PRIVATE ${model_definitions})
  target_link_options(lua_semantic_subscription_test${suffix} PRIVATE ${flags})
  target_link_libraries(lua_semantic_subscription_test${suffix} PRIVATE p4_luau${suffix})
  set_target_properties(lua_semantic_subscription_test${suffix} PROPERTIES LINKER_LANGUAGE CXX)
  if(suffix STREQUAL "")
    p4_ps2_exe(lua_semantic_subscription_test${suffix})
  elseif(suffix STREQUAL "64w")
    set_target_properties(lua_semantic_subscription_test${suffix} PROPERTIES SUFFIX ".exe" LINK_DEPENDS_USE_LINKER FALSE)
  endif()
  add_test(NAME lua_semantic_subscription${suffix}
    COMMAND ${runner} "$<TARGET_FILE:lua_semantic_subscription_test${suffix}>")
  set_tests_properties(lua_semantic_subscription${suffix} PROPERTIES LABELS "host" TIMEOUT 120)
  if(suffix STREQUAL "64")
    set_property(TEST lua_semantic_subscription${suffix} APPEND PROPERTY LABELS port64)
  elseif(suffix STREQUAL "64w")
    set_tests_properties(lua_semantic_subscription${suffix} PROPERTIES
      LABELS "host;port64win" FIXTURES_REQUIRED wine64_ready)
    set_property(TEST lua_semantic_subscription${suffix} APPEND PROPERTY ENVIRONMENT
      WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    p4_wine_scale_timeouts(lua_semantic_subscription${suffix})
  endif()
  # Synthetic guest-memory test for the scene player-position reader.
  if(NOT suffix STREQUAL "" AND NOT suffix STREQUAL "64w" AND NOT P4_APPLE)
    add_executable(mod_scene_position_test${suffix}
      "${P4_MOD_ROOT}/tests/mod_scene_position_test.c"
      "${P4_MOD_ROOT}/src/platform/mods/game_services.c")
    target_include_directories(mod_scene_position_test${suffix} PRIVATE "${P4_MOD_ROOT}/include" "${P4_GEN}")
    target_compile_definitions(mod_scene_position_test${suffix} PRIVATE ${model_definitions})
    target_compile_options(mod_scene_position_test${suffix} PRIVATE ${flags} -ffunction-sections -fdata-sections)
    target_link_options(mod_scene_position_test${suffix} PRIVATE ${flags} -Wl,--gc-sections)
    add_test(NAME mod_scene_position${suffix} COMMAND ${runner} "$<TARGET_FILE:mod_scene_position_test${suffix}>")
    set_tests_properties(mod_scene_position${suffix} PROPERTIES LABELS "host" TIMEOUT 20)
  endif()
  # Completed scene results remain visible to their owner across other requests.
  if(NOT suffix STREQUAL "" AND NOT suffix STREQUAL "64w" AND NOT P4_APPLE)
    add_executable(mod_scene_status_test${suffix} "${P4_MOD_ROOT}/tests/mod_scene_status_test.c")
    target_include_directories(mod_scene_status_test${suffix} PRIVATE "${P4_MOD_ROOT}/include" "${P4_GEN}")
    target_compile_definitions(mod_scene_status_test${suffix} PRIVATE ${model_definitions})
    target_compile_options(mod_scene_status_test${suffix} PRIVATE ${flags} -ffunction-sections -fdata-sections)
    target_link_options(mod_scene_status_test${suffix} PRIVATE ${flags} -Wl,--gc-sections)
    add_test(NAME mod_scene_status${suffix} COMMAND ${runner} "$<TARGET_FILE:mod_scene_status_test${suffix}>")
    set_tests_properties(mod_scene_status${suffix} PROPERTIES LABELS "host" TIMEOUT 20)
    if(suffix STREQUAL "64")
      set_property(TEST mod_scene_status${suffix} APPEND PROPERTY LABELS port64)
    endif()
  endif()
  # The native calendar half: the hold (story flags, the slot's scheduler task),
  # the next slot and its staging, over synthetic game state.
  if(NOT suffix STREQUAL "" AND NOT suffix STREQUAL "64w" AND NOT P4_APPLE)
    add_executable(mod_calendar_hold_test${suffix} "${P4_MOD_ROOT}/tests/mod_calendar_hold_test.c")
    target_include_directories(mod_calendar_hold_test${suffix} PRIVATE "${P4_MOD_ROOT}/include" "${P4_GEN}")
    target_compile_definitions(mod_calendar_hold_test${suffix} PRIVATE ${model_definitions})
    target_compile_options(mod_calendar_hold_test${suffix} PRIVATE ${flags} -ffunction-sections -fdata-sections)
    target_link_options(mod_calendar_hold_test${suffix} PRIVATE ${flags} -Wl,--gc-sections)
    add_test(NAME mod_calendar_hold${suffix} COMMAND ${runner} "$<TARGET_FILE:mod_calendar_hold_test${suffix}>")
    set_tests_properties(mod_calendar_hold${suffix} PROPERTIES LABELS "host" TIMEOUT 20)
    if(suffix STREQUAL "64")
      set_property(TEST mod_calendar_hold${suffix} APPEND PROPERTY LABELS port64)
    endif()
  endif()
  # The snapshot witness uses the same real VM/scheduler fixture and adds the
  # production card transaction transport, rather than simulating commit.
  get_target_property(save_sources mod_luau_test${suffix} SOURCES)
  # The substitute binding needs mod_luau_test.c's stub enemy AI carrier.
  list(REMOVE_ITEM save_sources "${P4_MOD_ROOT}/tests/mod_luau_test.c"
    "${P4_MOD_ROOT}/src/platform/mods/lua_battle_substitute.c")
  add_executable(mod_luau_save_test${suffix} "${P4_MOD_ROOT}/tests/mod_luau_save_test.c" ${save_sources}
    "${P4_MOD_ROOT}/src/platform/memcard/save_transaction.c"
    "${P4_MOD_ROOT}/src/platform/memcard/mod_save_custody.c"
    "${P4_MOD_ROOT}/src/platform/memcard/mod_save_transport.c")
  add_dependencies(mod_luau_save_test${suffix} p4_mod_hook_fixture)
  target_include_directories(mod_luau_save_test${suffix} PRIVATE "${P4_MOD_ROOT}/tests"
    "${P4_MOD_ROOT}/src/platform/mods" "${P4_MOD_ROOT}/src/platform/files")
  target_compile_options(mod_luau_save_test${suffix} PRIVATE ${flags} -O2 -g1 -fno-strict-aliasing -ffp-contract=off)
  target_link_options(mod_luau_save_test${suffix} PRIVATE ${flags})
  target_link_libraries(mod_luau_save_test${suffix} PRIVATE p4_mod_luau${suffix} m)
  set_target_properties(mod_luau_save_test${suffix} PROPERTIES C_COMPILER_LAUNCHER "${P4_MOD_LAUNCHER}" LINKER_LANGUAGE CXX)
  if(suffix STREQUAL "")
    p4_ps2_exe(mod_luau_save_test${suffix})
  elseif(suffix STREQUAL "64w")
    set_target_properties(mod_luau_save_test${suffix} PROPERTIES SUFFIX ".exe" LINK_DEPENDS_USE_LINKER FALSE)
  endif()
  add_test(NAME mod_luau_save${suffix} COMMAND ${runner} "$<TARGET_FILE:mod_luau_save_test${suffix}>"
    "${CMAKE_CURRENT_BINARY_DIR}/save-fixture-data${suffix}")
  add_test(NAME mod_luau_save_recovery${suffix} COMMAND "${P4_PYTHON}"
    "${P4_MOD_ROOT}/tests/mod_luau_save_failure_test.py"
    ${runner} "$<TARGET_FILE:mod_luau_save_test${suffix}>" --driver-end
    "${CMAKE_CURRENT_BINARY_DIR}/save-failure-data${suffix}")
  set_tests_properties(mod_luau_save${suffix} mod_luau_save_recovery${suffix} PROPERTIES LABELS "host" TIMEOUT 60
    ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_VIRTUAL_CLOCK=1;SDL_AUDIODRIVER=dummy")
  if(suffix STREQUAL "64")
    set_property(TEST mod_luau_save${suffix} mod_luau_save_recovery${suffix} APPEND PROPERTY LABELS port64)
  elseif(suffix STREQUAL "64w")
    set_tests_properties(mod_luau_save${suffix} mod_luau_save_recovery${suffix}
      PROPERTIES LABELS "host;port64win" FIXTURES_REQUIRED wine64_ready)
    set_property(TEST mod_luau_save${suffix} mod_luau_save_recovery${suffix} APPEND PROPERTY ENVIRONMENT WINEDEBUG=-all
      "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    p4_wine_scale_timeouts(mod_luau_save${suffix} mod_luau_save_recovery${suffix})
  endif()
  add_test(NAME mod_luau${suffix} COMMAND ${runner} "$<TARGET_FILE:mod_luau_test${suffix}>"
    "${CMAKE_CURRENT_BINARY_DIR}/fixture-data${suffix}")
  set_tests_properties(mod_luau${suffix} PROPERTIES LABELS "host" TIMEOUT 60
    ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_VIRTUAL_CLOCK=1;SDL_AUDIODRIVER=dummy")
  add_test(NAME mod_luau_stage_failure${suffix} COMMAND "${P4_PYTHON}"
    "${P4_MOD_ROOT}/tests/mod_luau_stage_failure_test.py"
    ${runner} "$<TARGET_FILE:mod_luau_test${suffix}>" --driver-end
    "${CMAKE_CURRENT_BINARY_DIR}/stage-failure-data${suffix}")
  set_tests_properties(mod_luau_stage_failure${suffix} PROPERTIES LABELS "host" TIMEOUT 200
    ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_VIRTUAL_CLOCK=1;SDL_AUDIODRIVER=dummy")
  add_test(NAME mod_luau_quiescence${suffix} COMMAND "${P4_PYTHON}"
    "${P4_MOD_ROOT}/tests/mod_callback_quiescence_test.py" event
    ${runner} "$<TARGET_FILE:mod_luau_test${suffix}>")
  set_tests_properties(mod_luau_quiescence${suffix} PROPERTIES LABELS "host" TIMEOUT 30
    SKIP_RETURN_CODE 77 ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
  add_test(NAME mod_frame_cpu${suffix} COMMAND "${P4_PYTHON}" "${P4_MOD_ROOT}/tests/mod_frame_cpu_test.py"
    ${runner} "$<TARGET_FILE:mod_luau_test${suffix}>" --driver-end)
  set_tests_properties(mod_frame_cpu${suffix} PROPERTIES LABELS "host" TIMEOUT 200
    ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_CONFIG=none;SDL_AUDIODRIVER=dummy")
  if(NOT CMAKE_CROSSCOMPILING OR suffix STREQUAL "64w")
    add_test(NAME mod_api_check${suffix} COMMAND "${P4_PYTHON}" "${P4_MOD_ROOT}/tests/mod_api_check_test.py"
      ${runner} "$<TARGET_FILE:p4_luau_check${suffix}>" --driver-end "${P4_MOD_ROOT}/include/p4pc/p4.d.luau"
      "${P4_MOD_ROOT}/tests/mod_fixtures/native_ui/main.luau"
      "${P4_MOD_ROOT}/examples/mods/p4pc.example/main.luau"
      "${P4_MOD_ROOT}/examples/tutorials/00-first-mod/main.luau"
      "${P4_MOD_ROOT}/mods/p4pc.challenge/main.luau"
      "${P4_MOD_ROOT}/tests/mod_fixtures/game_services/main.luau"
      "${P4_MOD_ROOT}/tests/mod_fixtures/calendar_pass/main.luau"
      "${P4_MOD_ROOT}/mods/p4pc.practice/main.luau"
      "${P4_MOD_ROOT}/mods/p4pc.hardmode/main.luau"
      "${P4_MOD_ROOT}/mods/p4pc.golden/main.luau"
      "${P4_MOD_ROOT}/mods/p4pc.randomizer/main.luau"
      "${P4_MOD_ROOT}/mods/p4pc.qol/main.luau"
      "${P4_MOD_ROOT}/tests/mod_fixtures/ui_overlay/main.luau"
      "${P4_MOD_ROOT}/tests/mod_fixtures/formula_stages/main.luau"
      "${P4_MOD_ROOT}/tests/mod_fixtures/data_rebalance/data.luau"
      "${P4_MOD_ROOT}/tests/mod_fixtures/data_rebalance/main.luau"
      "${P4_MOD_ROOT}/tests/mod_fixtures/ui_native_row/main.luau"
      "${P4_MOD_ROOT}/mods/p4pc.fusion-arts/main.luau")
    set_tests_properties(mod_api_check${suffix} PROPERTIES LABELS "host" TIMEOUT 60
      ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script")
    add_test(NAME mod_hook_cost${suffix} COMMAND ${runner} "$<TARGET_FILE:mod_luau_test${suffix}>"
      --hook-cost "${CMAKE_CURRENT_BINARY_DIR}/hook-cost-data${suffix}")
    # A CPU-time budget (LABELS performance) is native-only: under qemu-user it exits 77 with its reason, never red
    # with a misleading one (batch 74843: every A64 test fell back to an x86_64 host's qemu).
    set_tests_properties(mod_hook_cost${suffix} PROPERTIES LABELS "host;performance" TIMEOUT 120 SKIP_RETURN_CODE 77
      ENVIRONMENT "P4PC_MODS=0;P4PC_PACKS=0;P4PC_INPUT=script;P4PC_VIRTUAL_CLOCK=1;SDL_AUDIODRIVER=dummy")
    if(suffix STREQUAL "64" AND P4_PORT64_EMULATED)
      set_property(TEST mod_hook_cost${suffix} APPEND PROPERTY ENVIRONMENT "P4PC_PERF_EMULATED=${P4_PORT64_EMULATED}")
    endif()
  endif()
  if(suffix STREQUAL "64")
    set_property(TEST mod_luau${suffix} mod_luau_stage_failure${suffix} mod_luau_quiescence${suffix} mod_frame_cpu${suffix}
      APPEND PROPERTY LABELS port64)
    if(TEST mod_api_check${suffix})
      set_property(TEST mod_api_check${suffix} APPEND PROPERTY LABELS port64)
    endif()
    if(TEST mod_hook_cost${suffix})
      set_property(TEST mod_hook_cost${suffix} APPEND PROPERTY LABELS port64)
    endif()
  elseif(suffix STREQUAL "64w")
    foreach(name mod_luau${suffix} mod_luau_stage_failure${suffix} mod_luau_quiescence${suffix} mod_api_check${suffix}
        mod_hook_cost${suffix} mod_frame_cpu${suffix})
      if(TEST ${name})
        set_tests_properties(${name} PROPERTIES LABELS "host;port64win" FIXTURES_REQUIRED wine64_ready)
        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
          WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
        p4_wine_scale_timeouts(${name})
      endif()
    endforeach()
  endif()
endfunction()
p4_host_cxx_profiles(p4_mod_profile)
# A compile-only COFF mirror still has guarded game objects even when the
# Win64 runtime prefix is absent. Compile the real generated bridge for its
# audit without enabling a runtime provider or linking an executable.
if(P4_PORT64W_OBJS AND NOT TARGET p4_mod_game64w)
  set_source_files_properties("${P4_MOD_BRIDGES}" PROPERTIES GENERATED TRUE)
  add_library(p4_mod_bridge64w OBJECT "${P4_MOD_BRIDGES}")
  target_compile_options(p4_mod_bridge64w PRIVATE --target=x86_64-w64-mingw32 -m64 -O2)
  target_compile_definitions(p4_mod_bridge64w PRIVATE ${P4_PORT_DEFINES} P4_GUEST64=1 P4_GUEST_CHECK=1)
  target_include_directories(p4_mod_bridge64w PRIVATE "${P4_MOD_ROOT}/include" "${P4_GEN}")
  set_target_properties(p4_mod_bridge64w PROPERTIES C_COMPILER_LAUNCHER "${P4_MOD_LAUNCHER}")
  add_dependencies(p4_mod_bridge64w p4_mirror)
endif()
foreach(suffix "" 64 64w)
  if(TARGET p4_mod_game${suffix})
    set(args)
    if(TARGET p4_mod_luau${suffix})
      list(APPEND args --host-lib "$<TARGET_FILE:p4_mod_luau${suffix}>")
    endif()
    list(APPEND args --host-lib "$<TARGET_FILE:p4_mod_core${suffix}>")
    if(TARGET p4_mod_luau${suffix})
      list(APPEND args ${P4_LUAU_HOST_ARGS${suffix}})
    endif()
    set(P4_MOD_HOST_ARGS${suffix} ${args} PARENT_SCOPE)
    set(P4_MOD_CRT_PROJECT${suffix} --project-lib "$<TARGET_FILE:p4_mod_game${suffix}>" PARENT_SCOPE)
    set(P4_MOD_CRT_DEP${suffix} p4_mod_game${suffix} PARENT_SCOPE)
  endif()
endforeach()
if(TARGET p4_mod_bridge64w)
  set(P4_MOD_CRT_PROJECT64w --project-lib "$<TARGET_OBJECTS:p4_mod_bridge64w>" PARENT_SCOPE)
  set(P4_MOD_CRT_DEP64w p4_mod_bridge64w PARENT_SCOPE)
endif()
