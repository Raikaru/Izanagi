# RtAnim keyframe callbacks (overrides 00415ad0 004165a0 00415bc0 00417310 00416e10), the interpolator
# (003d5840 003d59d0 003d5bc0) and the 16-bit float they share. Each override is its own translation unit.
set(RTANIM_OVERRIDES 00415ad0 004165a0 00415bc0 00417310 00416e10 003d5840 003d59d0 003d5bc0 003dcc70)
set(RTANIM_SRCS rtanim_test.c)
foreach(addr IN LISTS RTANIM_OVERRIDES)
  list(APPEND RTANIM_SRCS "${CMAKE_SOURCE_DIR}/overrides/${addr}.c")
endforeach()
list(APPEND RTANIM_SRCS "${CMAKE_SOURCE_DIR}/src/platform/rw_rtanim_record.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_bytes.c"
  "${CMAKE_SOURCE_DIR}/src/platform/ee_libm.c" "${CMAKE_SOURCE_DIR}/src/platform/ee_float.c")
add_executable(rtanim_test ${RTANIM_SRCS})
target_include_directories(rtanim_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(rtanim_test m)
add_test(NAME rtanim COMMAND rtanim_test)

# The battle opening camera's curve keys from the production mirror and overrides in the EE float mode (slerp blend 001ba790, slerp setup
# 003dcc70, three-pose curve 001baff0), bit for bit against the retail keys of the collision 900 first-action anchor (tests/battle_camera_curve_test.c).
add_executable(battle_camera_curve_test battle_camera_curve_test.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(battle_camera_curve_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_options(battle_camera_curve_test PRIVATE -fno-builtin)
target_compile_definitions(battle_camera_curve_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(battle_camera_curve_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(battle_camera_curve_test)
add_test(NAME battle_camera_curve COMMAND battle_camera_curve_test)
set_tests_properties(battle_camera_curve PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)

# The quaternion overrides that sum retail accumulator chains (003dcc70 00415bc0 004165a0 00416e10 00417310), production objects against the
# unchanged retail instructions in the R5900 interpreter on 4096 deterministic cases each, every guest byte equal (tests/anim_accumulator_order_test.c).
add_executable(anim_accumulator_order_test anim_accumulator_order_test.c "${P4_TABLES}"
  "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(anim_accumulator_order_test PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/tools/mipsdiff")
target_compile_options(anim_accumulator_order_test PRIVATE -fno-builtin)
target_compile_definitions(anim_accumulator_order_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(anim_accumulator_order_test m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(anim_accumulator_order_test)
add_test(NAME anim_accumulator_order COMMAND anim_accumulator_order_test)
set_tests_properties(anim_accumulator_order PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)

# The BMP reader (override 003d8c00) against a memory-backed file layer and stub image callees.
add_executable(bmp_test bmp_test.c "${CMAKE_SOURCE_DIR}/overrides/003d8c00.c" ${P4_GUEST_TEST_SRC})
target_include_directories(bmp_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(bmp_test m)
add_test(NAME bmp COMMAND bmp_test)

# The cubic spline control-point fit (override 003b9a40) with stub allocator slots, weight table and errors.
add_executable(spline_test spline_test.c "${CMAKE_SOURCE_DIR}/overrides/003b9a40.c" ${P4_GUEST_TEST_SRC})
target_include_directories(spline_test PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(spline_test m)
add_test(NAME spline COMMAND spline_test)

