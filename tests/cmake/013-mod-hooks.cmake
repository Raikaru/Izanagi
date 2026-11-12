# The fixture is instrumented by the same post-override generator as the game.
# Host twins compile the generated C with their own ABI, not an FFI emulation.
add_executable(mod_hooks_test mod_hooks_test.c ${P4_MOD_TEST_GAME} ${P4_GUEST_TEST_SRC}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/hooks.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/events.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/failure.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/numbers.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mods.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_registry.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_memory.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/compendium.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/file_index.c"
  "${CMAKE_SOURCE_DIR}/third_party/yxml/yxml.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c")
target_compile_definitions(mod_hooks_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mod_hooks_test m)
add_dependencies(mod_hooks_test p4_mod_hook_fixture)
p4_ps2_exe(mod_hooks_test)
add_test(NAME mod_hooks COMMAND mod_hooks_test)
get_target_property(_p4_hook_runner mod_hooks_test CROSSCOMPILING_EMULATOR)
if(NOT _p4_hook_runner)
  unset(_p4_hook_runner)
endif()
add_test(NAME mod_hook_quiescence COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/mod_callback_quiescence_test.py" hook
  ${_p4_hook_runner} "$<TARGET_FILE:mod_hooks_test>")
set_tests_properties(mod_hook_quiescence PROPERTIES LABELS "host" TIMEOUT 30 SKIP_RETURN_CODE 77)
unset(_p4_hook_runner)

# p4.battle's panel observers over the real dispatch and hand-built descriptors
# (no generated fixture): a first use inside an active dispatch still observes.
add_executable(battle_menu_test battle_menu_test.c ${P4_GUEST_TEST_SRC}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/battle_menu.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/hooks.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/events.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/failure.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/numbers.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mods.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_registry.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_memory.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/compendium.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/file_index.c"
  "${CMAKE_SOURCE_DIR}/third_party/yxml/yxml.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c")
target_compile_definitions(battle_menu_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(battle_menu_test m)
p4_ps2_exe(battle_menu_test)
add_test(NAME battle_menu_observers COMMAND battle_menu_test)
set_tests_properties(battle_menu_observers PROPERTIES SKIP_RETURN_CODE 77)

# p4.battle.defeat's seams over the real dispatch and the real "retry"
# authorizers, with hand-built descriptors and the retail DatUnit records.
add_executable(battle_defeat_test battle_defeat_test.c ${P4_GUEST_TEST_SRC}
  "${CMAKE_SOURCE_DIR}/src/platform/mods/battle_defeat.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/authorize.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/hooks.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/events.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/failure.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/numbers.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/mods.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_registry.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/package_memory.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/compendium.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/file_index.c"
  "${CMAKE_SOURCE_DIR}/third_party/yxml/yxml.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c")
target_compile_definitions(battle_defeat_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(battle_defeat_test m)
p4_ps2_exe(battle_defeat_test)
add_test(NAME battle_defeat_seams COMMAND battle_defeat_test)
set_tests_properties(battle_defeat_seams PROPERTIES SKIP_RETURN_CODE 77 LABELS "host" TIMEOUT 60)

add_test(NAME mod_codegen COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/mod_codegen_test.py")
set_tests_properties(mod_hooks PROPERTIES SKIP_RETURN_CODE 77)

add_executable(mod_numbers_test mod_numbers_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/numbers.c")
target_link_libraries(mod_numbers_test m)
add_test(NAME mod_numbers COMMAND mod_numbers_test)
