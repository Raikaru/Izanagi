# Mod banners and message lines in battle (p4.battle.announce/say) over synthetic guest battle records.
add_executable(battle_present_test battle_present_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/battle_present.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(battle_present_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(battle_present_test)
add_test(NAME battle_present COMMAND battle_present_test)
set_tests_properties(battle_present PROPERTIES SKIP_RETURN_CODE 77)
