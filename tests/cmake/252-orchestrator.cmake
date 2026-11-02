add_test(NAME battle_sim_coordinator COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/test_battle_sim.py")
set_tests_properties(battle_sim_coordinator PROPERTIES LABELS port64 TIMEOUT 300)
