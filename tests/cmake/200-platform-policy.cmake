# Tests run in parallel (ctest -j), so none may share scratch space. The
# memory-card save root defaults to ./p4pc_save in the working directory, which several tests share,
# so every test gets its own. Output files (PNGs, WAVs, the files_disc override directory) already
# have per-test names.
get_directory_property(P4_ALL_TESTS TESTS)
foreach(t IN LISTS P4_ALL_TESTS)
  # The save root must exist: the memory-card layer creates mc0/mc1 under it, not the root itself.
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/save/${t}")
  set_property(TEST ${t} APPEND PROPERTY ENVIRONMENT "P4PC_SAVE_DIR=${CMAKE_BINARY_DIR}/save/${t}")
  get_test_property(${t} LABELS t_labels)
  # The renderer gates run on the scheduler's virtual clock (src/platform/kernel_thread.c): the game's
  # timers follow VBLANK ticks, so a busy host must not change how many ticks a frame sees.
  if(t_labels MATCHES "(^|;)scenes(;|$)")
    set_property(TEST ${t} APPEND PROPERTY ENVIRONMENT "P4PC_VIRTUAL_CLOCK=1")
  endif()
endforeach()

