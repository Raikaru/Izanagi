# The battle decision stage (p4.battle.decide) over synthetic guest battle records.
add_executable(battle_decide_test battle_decide_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/battle_decide.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(battle_decide_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(battle_decide_test)
add_test(NAME battle_decide COMMAND battle_decide_test)
