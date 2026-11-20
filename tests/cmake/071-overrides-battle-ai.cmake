# Battle script pick-by-character-id (overrides/001e6d20.c): the retail lookup and result, the
# "no unit" value in place of retail's NULL dereference, and the leader action a formation
# projector's lineup without the protagonist answers with. The override source is included as it
# is spliced into the mirror.
add_executable(battle_ai_pick_test battle_ai_pick_test.c)
target_include_directories(battle_ai_pick_test PRIVATE "${CMAKE_SOURCE_DIR}/overrides")
add_test(NAME battle_ai_pick COMMAND battle_ai_pick_test)

# The ally AI's HP and ailment ranking of the leader (overrides/001e1530.c and 001e1800.c with
# 001e6d20.c): every answer and parameter read equals the retail bodies wherever Yu is in the lineup,
# and a lineup projected without him ranks, heals and cures its leader-role fighter by that fighter's
# own HP and ailments, with the protagonist's weight and attack gate.
add_executable(battle_ai_leader_test battle_ai_leader_test.c)
target_include_directories(battle_ai_leader_test PRIVATE "${CMAKE_SOURCE_DIR}/overrides")
add_test(NAME battle_ai_leader COMMAND battle_ai_leader_test)

# Verify the native turn-start counter for Yu and a projected leader.
add_executable(battle_leader_turn_test battle_leader_turn_test.c)
target_include_directories(battle_leader_turn_test PRIVATE "${CMAKE_SOURCE_DIR}/overrides")
add_test(NAME battle_leader_turn COMMAND battle_leader_turn_test)
