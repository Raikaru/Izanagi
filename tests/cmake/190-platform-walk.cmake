# The shared retail/port walker chooses a non-default answer from the live menu,
# releasing Cross while the menu opens and confirming only the requested cursor.
add_executable(walk_choice_test walk_choice_test.c "${CMAKE_SOURCE_DIR}/src/platform/pad/walk_script.c")
target_link_libraries(walk_choice_test m)
add_test(NAME walk_choice COMMAND walk_choice_test)

# The cross-target dungeon digest hashes guest data, not addresses or RwMatrix padding (tests/dungeon_digest_gate.sh).
add_executable(dungeon_digest_test dungeon_digest_test.c "${CMAKE_SOURCE_DIR}/src/platform/pad/walk_script.c")
target_link_libraries(dungeon_digest_test m)
add_test(NAME dungeon_digest COMMAND dungeon_digest_test)

# battlerule: conditional battle commands: parsing and the ordered selection over a native snapshot.
add_executable(walk_battle_rules_test walk_battle_rules_test.c "${CMAKE_SOURCE_DIR}/src/platform/pad/walk_script.c")
target_include_directories(walk_battle_rules_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(walk_battle_rules_test m)
add_test(NAME walk_battle_rules COMMAND walk_battle_rules_test)

