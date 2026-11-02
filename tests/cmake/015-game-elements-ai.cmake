# Expanded enemy AI carrier boundary witnesses. Native formula/platform parity
# is qualified by the composed element cohort, not by these controlled bodies.
add_executable(game_knowledge_ai_test game_knowledge_ai_test.c)
target_compile_definitions(game_knowledge_ai_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_knowledge_ai COMMAND game_knowledge_ai_test)

add_executable(game_elements_ai_proof_test game_elements_ai_proof_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_elements_ai_proof.c")
target_compile_definitions(game_elements_ai_proof_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_elements_ai_proof COMMAND game_elements_ai_proof_test)

add_executable(game_elements_ai_roles_test game_elements_ai_roles_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_elements_ai_roles.c")
target_compile_definitions(game_elements_ai_roles_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_elements_ai_roles COMMAND game_elements_ai_roles_test)

add_executable(game_elements_ai_decision_test game_elements_ai_decision_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/game_elements_ai_decision.c"
  ${P4_GUEST_TEST_SRC})
target_compile_definitions(game_elements_ai_decision_test PRIVATE ${P4_PORT_DEFINES})
p4_ps2_exe(game_elements_ai_decision_test)
add_test(NAME game_elements_ai_decision COMMAND game_elements_ai_decision_test)
