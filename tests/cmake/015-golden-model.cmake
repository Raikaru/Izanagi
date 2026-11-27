# Pure CPU synthetic geometry/decoder vectors, plus optional owner-local Steam census.
set(P4_GOLDEN_MODEL_SOURCES
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_model.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_model_scene.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_model_motion.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_model_image.c"
  "${CMAKE_SOURCE_DIR}/src/platform/golden/golden_formats.c")
add_executable(golden_model_test golden_model_test.c ${P4_GOLDEN_MODEL_SOURCES})
target_include_directories(golden_model_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(golden_model_test m)
add_test(NAME golden_model COMMAND golden_model_test)
set_tests_properties(golden_model PROPERTIES TIMEOUT 30)
add_executable(golden_motion_test golden_motion_test.c ${P4_GOLDEN_MODEL_SOURCES})
target_include_directories(golden_motion_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(golden_motion_test m)
add_test(NAME golden_motion COMMAND golden_motion_test)
set_tests_properties(golden_motion PROPERTIES TIMEOUT 30)
add_executable(golden_model_cli golden_model_cli.c ${P4_GOLDEN_MODEL_SOURCES})
target_include_directories(golden_model_cli PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(golden_model_cli m)
p4_target_command(_p4_golden_model_driver golden_model_cli)
add_test(NAME golden_model_census COMMAND "${P4_PYTHON}"
  "${CMAKE_CURRENT_SOURCE_DIR}/golden_model_census_test.py" ${_p4_golden_model_driver} --driver-end)
set_tests_properties(golden_model_census PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
unset(_p4_golden_model_driver)
add_executable(golden_model_registration_test golden_model_registration_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(golden_model_registration_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(golden_model_registration_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(golden_model_registration_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(golden_model_registration_test)
add_test(NAME golden_model_ps2 COMMAND golden_model_registration_test)
set_tests_properties(golden_model_ps2 PROPERTIES TIMEOUT 30 ENVIRONMENT "P4PC_MODE=ps2")
