# Inventory and money actions (p4.give/p4.take/p4.inventory) over stub game accessors.
add_executable(game_inventory_test game_inventory_test.c "${CMAKE_SOURCE_DIR}/src/platform/mods/game_inventory.c")
target_compile_definitions(game_inventory_test PRIVATE ${P4_PORT_DEFINES})
add_test(NAME game_inventory COMMAND game_inventory_test)
