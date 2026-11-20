# Party actions (p4.party.persona/level, p4.give.skill/exp) and party record writes (p4.party.set,
# whose call order the actions keep) over stub Persona records and level code.
add_executable(game_party_test game_party_test.c "${CMAKE_SOURCE_DIR}/src/platform/mods/game_party.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/party_native.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(game_party_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_party COMMAND game_party_test)
set_tests_properties(game_party PROPERTIES SKIP_RETURN_CODE 77)

# The give.skill choice's field host over stub native calls and the guest words it reads.
add_executable(field_host_test field_host_test.c "${CMAKE_SOURCE_DIR}/src/platform/mods/field_host.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(field_host_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME field_host COMMAND field_host_test)
set_tests_properties(field_host PROPERTIES SKIP_RETURN_CODE 77)
