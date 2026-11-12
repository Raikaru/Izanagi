# The Sofdec IPU seam (src/movie/sfmpvp.c, movie_upload.c) driven like retail Sofdec drives it, on P4CT019.
# Skips (exit 77) unless P4PC_ISO names the image.
add_executable(sfmpvp_test sfmpvp_test.c guest_test.c "${CMAKE_SOURCE_DIR}/src/platform/gport.c" "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${CMAKE_SOURCE_DIR}/src/movie/sfmpvp.c" "${CMAKE_SOURCE_DIR}/src/movie/movie_upload.c"
  ${P4_PNG_SOURCES} "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c"
  "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}" $<TARGET_OBJECTS:p4movie>)
target_include_directories(sfmpvp_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src/movie" "${CMAKE_SOURCE_DIR}/src/viewer"
  "${CMAKE_SOURCE_DIR}/src/render" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(sfmpvp_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(sfmpvp_test ${P4_FFMPEG_LIBS} m)
p4_ps2_exe(sfmpvp_test)
if(P4_WIN32)
  target_link_options(sfmpvp_test PRIVATE -Wl,--large-address-aware)
endif()
add_test(NAME sfmpvp COMMAND sfmpvp_test)
set_tests_properties(sfmpvp PROPERTIES SKIP_RETURN_CODE 77)

# The movie player (src/movie/mwply.c) through the calls movieNativeTaskUpdate makes, on P4CT019 with a
# deterministic 1/60 s tick clock. Skips (exit 77) unless P4PC_ISO names the image.
add_executable(mwply_test mwply_test.c guest_test.c "${CMAKE_SOURCE_DIR}/src/platform/gport.c" "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${CMAKE_SOURCE_DIR}/src/movie/mwply.c" ${P4_PNG_SOURCES}
  "${CMAKE_SOURCE_DIR}/src/platform/files/vfs.c" "${CMAKE_SOURCE_DIR}/src/platform/files/iso9660.c" "${P4_OS_SRC}"
  $<TARGET_OBJECTS:p4movie>)
target_include_directories(mwply_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src/movie" "${CMAKE_SOURCE_DIR}/src/viewer"
  "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_definitions(mwply_test PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(mwply_test ${P4_FFMPEG_LIBS} m)
add_test(NAME mwply COMMAND mwply_test)
set_tests_properties(mwply PROPERTIES SKIP_RETURN_CODE 77)

# A movie frame through the game's upload call and the GL device: P4CT019 frame 120 into a raster made
# like the movie raster (0x584), gsUploadRasterTiles, an Im2D quad, readback, compared with the player's
# frame. Links the port objects like render_smoke. Skipped (exit 77) unless P4PC_ISO and P4PC_ELF are set.
add_executable(movie_smoke movie_smoke.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(movie_smoke PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/movie"
  "${CMAKE_SOURCE_DIR}/src/render" "${CMAKE_SOURCE_DIR}/src/viewer" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_options(movie_smoke PRIVATE -fno-builtin)
target_compile_definitions(movie_smoke PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(movie_smoke m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(movie_smoke)
add_test(NAME movie_smoke COMMAND movie_smoke "${CMAKE_BINARY_DIR}/movie_smoke.png")
set_tests_properties(movie_smoke PROPERTIES SKIP_RETURN_CODE 77 LABELS gl)

# The game's own movie task (movieNativeTaskUpdate) against the port's player and GL device: create, start,
# frames, upload, draw, end, on P4CT019 with the tick clock. Links the port objects like render_smoke.
# Skipped (exit 77) unless P4PC_ISO and P4PC_ELF are set. P4PC_MOVIE_INDEX=21 runs the opening (P4CTOP1).
add_executable(movie_task_smoke movie_task_smoke.c "${P4_TABLES}"
  $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>)
target_include_directories(movie_task_smoke PRIVATE "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src/render" "${CMAKE_SOURCE_DIR}/src/viewer" "${CMAKE_SOURCE_DIR}/src/platform/files")
target_compile_options(movie_task_smoke PRIVATE -fno-builtin)
target_compile_definitions(movie_task_smoke PRIVATE ${P4_PORT_DEFINES})
target_link_libraries(movie_task_smoke m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS})
p4_ps2_exe(movie_task_smoke)
add_test(NAME movie_task COMMAND movie_task_smoke)
set_tests_properties(movie_task PROPERTIES SKIP_RETURN_CODE 77 LABELS gl WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
# The same task on the opening (P4CTOP1, 2520 frames) and P4CT001 (1800 frames): the events the game sees (first frame
# after 18..21 updates, +1 frame per update, state 4/5/6 at the retail frame numbers) are compared with the
# PCSX2 traces in tests/movie_retail_events.h.
add_test(NAME movie_task_opening COMMAND movie_task_smoke)
add_test(NAME movie_task_p4ct001 COMMAND movie_task_smoke)
set_tests_properties(movie_task_opening PROPERTIES SKIP_RETURN_CODE 77 LABELS gl TIMEOUT 600 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  ENVIRONMENT "P4PC_MOVIE_INDEX=21;P4PC_MOVIE_SHOTS=0")
set_tests_properties(movie_task_p4ct001 PROPERTIES SKIP_RETURN_CODE 77 LABELS gl TIMEOUT 600 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  ENVIRONMENT "P4PC_MOVIE_INDEX=1;P4PC_MOVIE_SHOTS=0")

