# Every test that runs the i386 game executable (`p4pc`) is also registered for the 64-bit one (`p4pc64`) as <name>64, with the same command line: the full-game
# gates (battle, camp, save, systems, story, fields ...) then give the same verdicts on both builds (docs/port64.md M5). The properties follow in the final policy fragments.
function(add_test)
  _add_test(${ARGN})
  if(ARGV0 STREQUAL "NAME")
    cmake_parse_arguments(PT "" "NAME" "COMMAND" ${ARGN})
    set(pt_cmd ${PT_COMMAND})
    # the command, for the 64-bit twins of the host tests (p4_host_twins below)
    set_property(GLOBAL PROPERTY P4_CMD_${PT_NAME} "${PT_COMMAND}")
    set_property(GLOBAL APPEND PROPERTY P4_CMD_TESTS ${PT_NAME})
    list(GET pt_cmd 0 pt_first)
    if(pt_first STREQUAL "${CMAKE_COMMAND}")
      list(LENGTH pt_cmd pt_n)
      set(pt_i 1)
      while(pt_i LESS pt_n)      # cmake -E env NAME=VALUE ... <program> ...
        list(GET pt_cmd ${pt_i} pt_item)
        if(NOT pt_item MATCHES "^(-E|env|[A-Za-z_][A-Za-z0-9_]*=.*)$")
          set(pt_first "${pt_item}")
          break()
        endif()
        math(EXPR pt_i "${pt_i} + 1")
      endwhile()
    endif()
    if(NOT TARGET "${pt_first}" AND NOT "${pt_cmd}" MATCHES "[$]<TARGET")
      set_property(GLOBAL APPEND PROPERTY P4_PLAIN_TESTS ${PT_NAME})
      # Python tooling fixtures spawn bounded process trees. Serialize this
      # entire class, including future fixtures, instead of racing deadlines.
      if(pt_first STREQUAL "${P4_PYTHON}")
        set_tests_properties(${PT_NAME} PROPERTIES RESOURCE_LOCK host-tooling)
      endif()
    endif()
  endif()
  if(ARGV0 STREQUAL "NAME" AND TARGET p4pc64)
    cmake_parse_arguments(AT "" "NAME" "COMMAND" ${ARGN})
    set(cmd64)
    set(found FALSE)
    foreach(a IN LISTS AT_COMMAND)
      if(a STREQUAL "$<TARGET_FILE:p4pc>")
        list(APPEND cmd64 "$<TARGET_FILE:p4pc64>")
        set(found TRUE)
      else()
        list(APPEND cmd64 "${a}")
      endif()
    endforeach()
    # scene_battle_floor_a: its 64-bit twin (own limit) is registered further down: do not clone it twice
    if(AT_NAME STREQUAL "scene_battle_floor_a")
      set(found FALSE)
    endif()
    if(found AND NOT TEST ${AT_NAME}64)
      _add_test(NAME ${AT_NAME}64 COMMAND ${cmd64})
      set_property(GLOBAL APPEND PROPERTY P4_GATE_CLONES ${AT_NAME})
    endif()
  endif()
endfunction()

# p4_target_command(<var> <target>): the argv that runs a built program from a test harness (a Python or shell
# script given the program on its command line): the target's CROSSCOMPILING_EMULATOR (wine for the Windows
# builds) followed by $<TARGET_FILE:target>. A harness runs that argv as given and never execs the .exe itself:
# a direct exec only works on a machine whose kernel has a binfmt handler for Windows programs (Fedora's
# wine-systemd installs one), so the result would depend on the machine that runs the test.
function(p4_target_command var target)
  get_target_property(runner ${target} CROSSCOMPILING_EMULATOR)
  if(NOT runner)
    set(runner)
  endif()
  set(${var} ${runner} "$<TARGET_FILE:${target}>" PARENT_SCOPE)
endfunction()

# Host tests. The compile and link flags of the target (Linux i386 or 32-bit Windows) come from the
# root CMakeLists.txt (add_compile_options / add_link_options), so the targets below do not repeat
# them. Tests that map memory at PS2 addresses call p4_ps2_exe(): on Windows that links the exe's
# `.ps2ram` section (src/platform/os/win32_ps2ram.S). Tests that use the OS shim (include/p4pc/os.h)
# list ${P4_OS_SRC}. Hook up from the root with:
#   enable_testing()
#   add_subdirectory(tests)

# On Windows the test exes run from this directory, so the DLLs (SDL3, FFmpeg) sit beside them
# (p4_dlls, root CMakeLists.txt). Under Wine, ctest finds them the same way.

add_test(NAME event_scene_state COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/event_scene_state_test.py")
add_test(NAME input_guard COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/input_guard_test.py")
add_test(NAME gen_port_contracts COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/gen_port_test.py")
set_tests_properties(gen_port_contracts PROPERTIES ENVIRONMENT "P4PC_TEST_DECOMP=${P4_DECOMP}")
add_test(NAME live_argument_discovery COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/audit_live_args_test.py")

# Tests of port code that reads and writes guest memory (docs/port64.md M3): the guest fixtures, the port storage
# arena, the guest block and the OS shim. Such a test also calls p4_ps2_exe() (it maps retail addresses on i386).
set(P4_GUEST_TEST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/guest_test.c" "${CMAKE_SOURCE_DIR}/src/platform/gport.c"
  "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${P4_OS_SRC}")

