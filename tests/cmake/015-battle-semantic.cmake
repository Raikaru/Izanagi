# Semantic battle events (p4.events.on "damage", "ailment", ...) from synthetic observer packets
# and native notifications over synthetic guest records.
add_executable(battle_semantic_test battle_semantic_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/battle_semantic.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/battle_notify.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(battle_semantic_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME battle_semantic COMMAND battle_semantic_test)
set_tests_properties(battle_semantic PROPERTIES SKIP_RETURN_CODE 77)
