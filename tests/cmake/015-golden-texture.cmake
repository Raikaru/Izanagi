# Synthetic native codecs and a GL-call recording mode-isolation seam.
set(P4_GOLDEN_TEXTURE_SOURCES
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_texture.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_atlas.c"
  "${CMAKE_SOURCE_DIR}/src/render/p4tmx.c"
  "${CMAKE_SOURCE_DIR}/src/platform/mods/p4content_image.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/golden_texture_alloc.c"
  "${CMAKE_SOURCE_DIR}/third_party/lodepng/lodepng.c"
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz.c"
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz_tdef.c"
  "${CMAKE_SOURCE_DIR}/third_party/miniz/miniz_tinfl.c")
add_executable(golden_texture_test golden_texture_test.c ${P4_GOLDEN_TEXTURE_SOURCES})
target_include_directories(golden_texture_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/render")
add_test(NAME golden_texture COMMAND golden_texture_test)
set_tests_properties(golden_texture PROPERTIES TIMEOUT 30)
add_executable(golden_texture_gl_test golden_texture_gl_test.c "${CMAKE_SOURCE_DIR}/src/render/p4gl_golden.c")
target_include_directories(golden_texture_gl_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME golden_texture_gl COMMAND golden_texture_gl_test)
set_tests_properties(golden_texture_gl PROPERTIES TIMEOUT 30)
add_executable(golden_texture_cli golden_texture_cli.c ${P4_GOLDEN_TEXTURE_SOURCES})
target_include_directories(golden_texture_cli PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/render")
p4_target_command(_p4_golden_texture_driver golden_texture_cli)
add_test(NAME golden_texture_steam COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/golden_texture_steam_test.py" ${_p4_golden_texture_driver} --driver-end)
set_tests_properties(golden_texture_steam PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
unset(_p4_golden_texture_driver)
add_executable(golden_icons_registration_test golden_icons_registration_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(golden_icons_registration_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_icons_registration_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(golden_icons_registration_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(golden_icons_registration_test)
add_test(NAME golden_icons_ps2 COMMAND golden_icons_registration_test)
add_test(NAME golden_icons_golden COMMAND golden_icons_registration_test golden)
set_tests_properties(golden_icons_ps2 golden_icons_golden PROPERTIES TIMEOUT 30)
p4_target_command(_p4_golden_icons_driver golden_icons_registration_test)
add_test(NAME golden_icons_cache COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/golden_icons_cache_test.py" ${_p4_golden_icons_driver} --driver-end)
set_tests_properties(golden_icons_cache PROPERTIES TIMEOUT 60)
unset(_p4_golden_icons_driver)
add_test(NAME golden_texture_census COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/golden_texture_census_test.py")
set_tests_properties(golden_texture_census PROPERTIES TIMEOUT 30)
add_test(NAME bcdec_license COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/bcdec_license_test.py")
set_tests_properties(bcdec_license PROPERTIES TIMEOUT 30)
