# The 64-bit builds and tests as one unit (the target p4_x64): it builds everything the tests labelled port64 need and nothing of the i386
# mirror, and every 64-bit test carries the label.
foreach(t boot64 port64_coro port64_task port64_task_retail port64_coro_win port64_mirror port64_census seam_abi_mirror)
  if(TEST ${t})
    set_property(TEST ${t} APPEND PROPERTY LABELS port64)
  endif()
endforeach()
add_custom_target(p4_x64)
foreach(t p4pc64 p4pc_viewer64 p4decomp64 port64_task64 port64_task_test)
  if(TARGET ${t})
    add_dependencies(p4_x64 ${t})
  endif()
endforeach()
if(P4_PORT64_OBJS)
  add_custom_target(p4_x64_abi DEPENDS "${P4_G64}/abi.json")
  add_dependencies(p4_x64 p4_x64_abi)
endif()
# the Windows x64 half (p4_x64w): the mirror for x86_64-w64-mingw32, the exe, and the coroutine test under 64-bit Wine
add_custom_target(p4_x64w)
foreach(t p4decomp64w p4pc64w p4pc_viewer64w p4pc64w_dlls port64_coro_win_test)
  if(TARGET ${t})
    add_dependencies(p4_x64w ${t})
  endif()
endforeach()
if(P4_PORT64_OBJS)
  add_dependencies(p4_x64w p4_x64_abi)
endif()

# The properties of the cloned full-game gates (add_test in the registration prelude): the i386 original's, labelled port64.
get_property(P4_GATE_CLONES GLOBAL PROPERTY P4_GATE_CLONES)
foreach(t IN LISTS P4_GATE_CLONES)
  foreach(prop TIMEOUT SKIP_RETURN_CODE WORKING_DIRECTORY ENVIRONMENT)
    get_test_property(${t} ${prop} v)
    if(v)
      set_tests_properties(${t}64 PROPERTIES ${prop} "${v}")
    endif()
  endforeach()
  # `slow`: minutes natively, hours under qemu-user (the a64 scope runs -LE ^(slow|gl)$: no renderer tests either).
  # Keep the 75k/110k corpus walks and 90k/110k castle walks in the long-run class.
  if(t MATCHES "^(story_to_0415|street_0417|castle_gate_0417|castle_2f_0417|home_0417|save_play|fields_load|newgame_to_field|systems_club_floor_menu|scene_tv_world_enter|scene_shadow_haze)$")
    set_tests_properties(${t}64 PROPERTIES LABELS "gl;port64;slow")
  else()
    set_tests_properties(${t}64 PROPERTIES LABELS "gl;port64")
  endif()
endforeach()

# The game gates of the 64-bit build: every test labelled `scenes` that has a 64-bit twin gives the twin the label `scenes64`. They need the disc
# (P4PC_ISO) and the reference captures, and run on a hardware GPU.
foreach(t IN LISTS P4_ALL_TESTS)
  get_test_property(${t} LABELS lbl)
  if(lbl MATCHES "(^|;)scenes(;|$)" AND TEST ${t}64)
    set_property(TEST ${t}64 APPEND PROPERTY LABELS scenes64)
    # The renderer gates run on the scheduler's virtual clock (the loop above gives it to the tests labelled `scenes`, which the twins made by p4_scene_gate were
    # not yet at that point): on the wall clock the 64-bit sky and butterfly gates read different frames from run to run of one binary (3 and 2 distinct PNGs in 4
    # runs), with it 6 of 6 runs are byte-identical.
    get_test_property(${t}64 ENVIRONMENT env64)
    if(NOT env64 MATCHES "P4PC_VIRTUAL_CLOCK=")
      set_property(TEST ${t}64 APPEND PROPERTY ENVIRONMENT "P4PC_VIRTUAL_CLOCK=1")
    endif()
  endif()
endforeach()

