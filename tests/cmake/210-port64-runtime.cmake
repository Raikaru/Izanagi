# 64-bit port prototype (docs/port64.md): the kernel task list (decomp/src/Kernel/sdkTask.c) with a fixed
# scenario, built twice from the same mirrored unit. port64_task_test is the i386 build as shipped (guest
# address == host pointer). port64_task_test64 is x86_64: tools/port64/build64.py rewrites the unit so guest
# pointers are u32 guest addresses into one allocated block (tools/port64/gptr_rewrite.py), and tests/port64_task.sh
# compares the two traces. Linux only; skipped when this host cannot build or run x86_64.
if(NOT P4_WIN32 AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64" AND P4_PORT64_ARCH STREQUAL "x86_64")
  execute_process(COMMAND "${CMAKE_C_COMPILER}" --target=x86_64-linux-gnu -fsyntax-only -x c "${CMAKE_SOURCE_DIR}/include/p4pc/guest.h"
    RESULT_VARIABLE P4_X64_PROBE OUTPUT_QUIET ERROR_QUIET)
  set(P4_TASK_UNIT "${P4_GEN}/decomp/src/Kernel/sdkTask.c")
  set_source_files_properties("${P4_TASK_UNIT}" PROPERTIES GENERATED TRUE COMPILE_OPTIONS
    "-malign-double;-mno-implicit-float;-DP4_DECOMP_UNIT=1;-std=gnu89;-fno-builtin;-DINCLUDE_ASM(FOLDER,NAME)=;-DINCLUDE_RODATA(FOLDER,NAME)=;-Wno-int-conversion;-Wno-incompatible-function-pointer-types;-Wno-return-mismatch;-Wno-incompatible-pointer-types")
  add_executable(port64_task_test port64_task_test.c "${P4_TASK_UNIT}" "${P4_OS_SRC}" "${CMAKE_SOURCE_DIR}/src/platform/ee_bytes.c")
  add_dependencies(port64_task_test p4_mirror)
  target_include_directories(port64_task_test PRIVATE "${P4_GEN}/decomp" "${P4_GEN}/decomp/include" "${P4_GEN}/decomp/src")
  target_compile_definitions(port64_task_test PRIVATE ${P4_PORT_DEFINES})
  set(P4_TASK_BRIDGE64_ARGS)
  if(P4PC_MOD_SUPPORT)
    # The standalone task comparison uses the same real entry/dispatch code as
    # the game. Section GC retains only the task unit's generated bridges.
    add_dependencies(port64_task_test p4_mod_game p4_mod_core)
    target_link_options(port64_task_test PRIVATE -Wl,--gc-sections)
    target_link_libraries(port64_task_test "$<TARGET_FILE:p4_mod_game>" "$<TARGET_FILE:p4_mod_core>")
    if(TARGET p4_mod_game64)
      list(APPEND P4_TASK_BRIDGE64_ARGS --lib=-Wl,--gc-sections
        "--lib=$<TARGET_FILE:p4_mod_game64>" "--lib=$<TARGET_FILE:p4_mod_core64>")
      set(P4_TASK_BRIDGE64_DEP p4_mod_game64 p4_mod_core64)
    else()
      list(APPEND P4_TASK_BRIDGE64_ARGS --src "${P4_MOD_BRIDGES}"
        --src src/platform/mods/hooks.c --src src/platform/mods/numbers.c
        --src src/platform/mods/failure.c --cflag=-ffunction-sections
        --cflag=-fdata-sections --lib=-Wl,--gc-sections)
    endif()
  endif()
  if(P4_X64_PROBE EQUAL 0)
    set(P4_TASK64 "${CMAKE_CURRENT_BINARY_DIR}/port64_task_test64")
    add_custom_command(OUTPUT "${P4_TASK64}"
      COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/port64/build64.py" --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}"
              --out "${CMAKE_CURRENT_BINARY_DIR}/port64" --unit src/Kernel/sdkTask.c --src tests/port64_task_test.c
              --src src/platform/ee_bytes.c
              ${P4_TASK_BRIDGE64_ARGS}
              --src src/platform/guest.c --src src/platform/os/os_posix.c --exe "${P4_TASK64}" --python "${P4_PYTHON}" --clang "${CMAKE_C_COMPILER}"
      DEPENDS p4_mirror ${P4_TASK_BRIDGE64_DEP} "${CMAKE_SOURCE_DIR}/tools/port64/build64.py" "${CMAKE_SOURCE_DIR}/tools/port64/gptr_rewrite.py"
              "${CMAKE_CURRENT_SOURCE_DIR}/port64_task_test.c" "${CMAKE_SOURCE_DIR}/src/platform/guest.c"
              "${CMAKE_SOURCE_DIR}/src/platform/ee_bytes.c"
              "${CMAKE_SOURCE_DIR}/include/p4pc/guest.h" "${CMAKE_SOURCE_DIR}/include/p4pc/compat.h"
      VERBATIM COMMENT "Building the x86_64 task prototype")
    # the same scenario with the retail instructions of the task unit run by the R5900 interpreter over the
    # x86_64 guest block (tools/mipsdiff/r5900.c is guest-block aware): the retail code is the reference
    set(P4_TASKR64 "${CMAKE_CURRENT_BINARY_DIR}/port64_task_retail64")
    add_custom_command(OUTPUT "${P4_TASKR64}"
      COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/port64/build64.py" --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}"
              --out "${CMAKE_CURRENT_BINARY_DIR}/port64r" --src tests/port64_task_test.c --src tools/mipsdiff/r5900.c
              --src src/platform/guest.c --src src/platform/os/os_posix.c --cflag=-DP4_RUN_RETAIL=1 "--cflag=-I${CMAKE_SOURCE_DIR}/tools/mipsdiff" --lib=-lm
              --exe "${P4_TASKR64}" --python "${P4_PYTHON}" --clang "${CMAKE_C_COMPILER}"
      DEPENDS p4_mirror "${CMAKE_SOURCE_DIR}/tools/port64/build64.py" "${CMAKE_CURRENT_SOURCE_DIR}/port64_task_test.c"
              "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${CMAKE_SOURCE_DIR}/tools/mipsdiff/r5900.c" "${CMAKE_SOURCE_DIR}/include/p4pc/guest.h"
      VERBATIM COMMENT "Building the x86_64 retail-interpreter task reference")
    # coroutines on guest stacks (the x86_64 context switch in os_posix.c)
    set(P4_CORO64 "${CMAKE_CURRENT_BINARY_DIR}/port64_coro_test64")
    add_custom_command(OUTPUT "${P4_CORO64}"
      COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/port64/build64.py" --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}"
              --out "${CMAKE_CURRENT_BINARY_DIR}/port64c" --src tests/port64_coro_test.c
              --src src/platform/guest.c --src src/platform/os/os_posix.c --exe "${P4_CORO64}" --python "${P4_PYTHON}" --clang "${CMAKE_C_COMPILER}"
      DEPENDS "${CMAKE_SOURCE_DIR}/tools/port64/build64.py" "${CMAKE_CURRENT_SOURCE_DIR}/port64_coro_test.c"
              "${CMAKE_SOURCE_DIR}/src/platform/guest.c" "${CMAKE_SOURCE_DIR}/src/platform/os/os_posix.c"
              "${CMAKE_SOURCE_DIR}/include/p4pc/guest.h" "${CMAKE_SOURCE_DIR}/include/p4pc/os.h"
      VERBATIM COMMENT "Building the x86_64 coroutine test")
    set(P4_CRASH64 "${CMAKE_CURRENT_BINARY_DIR}/crash_log_test64")
    add_custom_command(OUTPUT "${P4_CRASH64}"
      COMMAND "${CMAKE_C_COMPILER}" --target=x86_64-linux-gnu -m64 -O2 -g -DP4_GUEST_CHECK
              -ffunction-sections -fdata-sections "-I${CMAKE_SOURCE_DIR}/include"
              -include p4pc/compat.h
              "${CMAKE_CURRENT_SOURCE_DIR}/crash_log_test.c" "${CMAKE_SOURCE_DIR}/src/platform/guest.c"
              "${CMAKE_SOURCE_DIR}/src/platform/os/os_posix.c" -Wl,--gc-sections -ldl -o "${P4_CRASH64}"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/crash_log_test.c" "${CMAKE_SOURCE_DIR}/src/platform/guest.c"
              "${CMAKE_SOURCE_DIR}/src/platform/os/os_posix.c" "${CMAKE_SOURCE_DIR}/include/p4pc/os.h"
              "${CMAKE_SOURCE_DIR}/include/p4pc/guest.h" "${CMAKE_SOURCE_DIR}/include/p4pc/compat.h" VERBATIM)
    add_test(NAME crash_log64 COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/crash_log_test.py" "${P4_CRASH64}")
    set_tests_properties(crash_log64 PROPERTIES LABELS port64 TIMEOUT 90)
    add_custom_target(port64_task64 ALL DEPENDS "${P4_TASK64}" "${P4_TASKR64}" "${P4_CORO64}" "${P4_CRASH64}")
    add_test(NAME port64_coro COMMAND "${P4_CORO64}")
    set_tests_properties(port64_coro PROPERTIES SKIP_RETURN_CODE 77)
    if(P4_RETAIL)
      add_test(NAME port64_task COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/port64_task.sh" $<TARGET_FILE:port64_task_test> "${P4_TASK64}" "${P4_RETAIL}")
      add_test(NAME port64_task_retail COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/port64_task.sh" "${P4_TASKR64}" "${P4_TASK64}" "${P4_RETAIL}")
      set_tests_properties(port64_task port64_task_retail PROPERTIES SKIP_RETURN_CODE 77)
    endif()
  else()
    message(STATUS "port64: this host cannot build x86_64 (clang --target=x86_64-linux-gnu failed); port64_task skipped")
  endif()
endif()

# Windows x64 coroutines on caller-supplied stacks (the context switch in os_win32.c), run under 64-bit Wine (tests/port64_coro_win_test.c).
if(TARGET p4decomp64w)
  find_program(P4_WINE64 NAMES wine64 wine)
  if(P4_WINE64)
    set(CMAKE_LINK_DEPENDS_USE_LINKER OFF)   # lld in MinGW mode has no --dependency-file
    add_executable(port64_coro_win_test port64_coro_win_test.c "${CMAKE_SOURCE_DIR}/src/platform/os/os_win32.c")
    target_compile_options(port64_coro_win_test PRIVATE --target=x86_64-w64-mingw32 -m64 -fno-builtin)
    target_link_options(port64_coro_win_test PRIVATE --target=x86_64-w64-mingw32 -m64 ${P4_PE_REPRO_LINK_FLAGS})
    target_link_libraries(port64_coro_win_test kernel32 user32)
    set_target_properties(port64_coro_win_test PROPERTIES SUFFIX ".exe")
    add_test(NAME port64_coro_win COMMAND "${CMAKE_COMMAND}" -E env WINEDEBUG=-all "WINEPREFIX=${CMAKE_CURRENT_BINARY_DIR}/wine64prefix"
             "${P4_WINE64}" $<TARGET_FILE:port64_coro_win_test>)
    set_tests_properties(port64_coro_win PROPERTIES TIMEOUT 120)
  endif()
endif()

# The 64-bit executable (-DP4_PORT64_LINK=ON) boots against the retail image and disc as far as the marker (tests/boot64_gate.sh).
if(TARGET p4pc64)
  add_test(NAME boot64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/boot64_gate.sh" $<TARGET_FILE:p4pc64>)
  set_tests_properties(boot64 PROPERTIES SKIP_RETURN_CODE 77 LABELS gl TIMEOUT 300)
  # The game-level gates of the 64-bit build are clones of the i386 ones (add_test wrapper in the registration prelude); this one has its own limit.
  add_test(NAME scene_battle_floor_a64 COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/battle_mad.sh" $<TARGET_FILE:p4pc64> 512 p4-battle-floor-a 1400 14.4 "cross@360:6,cross@420:6,cross@480:6,cross@540:6,cross@600:6,cross@660:6,cross@720:6,cross@780:6,cross@840:6,cross@900:6,cross@960:6,cross@1020:6,cross@1080:6,cross@1140:6,cross@1200:6" P4PC_VIRTUAL_CLOCK=1)
  set_tests_properties(scene_battle_floor_a64 PROPERTIES SKIP_RETURN_CODE 77 LABELS "gl;port64" TIMEOUT 300)
endif()

# The rewrite of the whole mirror left nothing unsupported that the baseline does not name (tools/port64/unit64.py report).
if(P4_PORT64_OBJS)
  add_test(NAME port64_mirror COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" report --dir "${P4_G64}/units"
           --baseline "${CMAKE_SOURCE_DIR}/config/port64_unsupported.txt")
  # Port code that calls decomp functions agrees with the mirror's classes (the build's merged abi.json): a host pointer
  # given to an m2c `Uint32` parameter would arrive truncated.
  # The rewriter's rule counts per unit are a contract (config/port64_census.txt, tools/port64/unit64.py report): a rule that stops firing, fires elsewhere or a
  # different number of times (the decomp or a pin changed under it, the rewriter changed) fails here until the new counts are reviewed and refreshed.
  # One baseline per target (another target rewrites some constructs differently): the build of the mirror enforces it too (CMakeLists.txt, P4_CENSUS_BASELINE).
  if(P4_CENSUS_BASELINE)
    add_test(NAME port64_census COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" report --dir "${P4_G64}/units"
             --census-baseline "${P4_CENSUS_BASELINE}" --clang "${CMAKE_C_COMPILER}" --active-units "${P4_G64}/active_units.txt"
             --rw-clean-units "${P4_GEN}/rw_clean_units.txt" --rw-clean-stripped "${P4_GEN}/decomp/src/rwclean/stripped_units.txt")
  endif()
  add_test(NAME seam_abi_mirror COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/port64/seam_abi.py" --check --mirror "${P4_G64}/abi.json")
endif()

