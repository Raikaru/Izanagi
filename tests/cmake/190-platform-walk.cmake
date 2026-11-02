# The shared retail/port walker chooses a non-default answer from the live menu,
# releasing Cross while the menu opens and confirming only the requested cursor.
add_executable(walk_choice_test walk_choice_test.c "${CMAKE_SOURCE_DIR}/src/platform/pad/walk_script.c")
target_link_libraries(walk_choice_test m)
add_test(NAME walk_choice COMMAND walk_choice_test)

