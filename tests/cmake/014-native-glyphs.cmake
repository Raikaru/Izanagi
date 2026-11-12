# Shared raw Config/MSG glyph syntax and native measure-only recipe.
add_executable(native_glyphs_test native_glyphs_test.c
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/native_glyphs_native.c" ${P4_GUEST_TEST_SRC})
target_compile_definitions(native_glyphs_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(native_glyphs_test m)
p4_ps2_exe(native_glyphs_test)
add_test(NAME native_glyphs COMMAND native_glyphs_test)
set_tests_properties(native_glyphs PROPERTIES SKIP_RETURN_CODE 77)
