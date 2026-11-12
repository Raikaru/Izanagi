cmake_minimum_required(VERSION 3.20)
# Also usable without the retail/decomp checkout: cmake -S tools/luau -B ...
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(p4pc_luau LANGUAGES C CXX)
  enable_testing()
  set(P4_LUAU_STANDALONE TRUE)
endif()
get_filename_component(P4_LUAU_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
include("${P4_LUAU_ROOT}/cmake/host-cxx.cmake")
include("${P4_LUAU_ROOT}/cmake/wine-tests.cmake")
# No game forced includes, ABI rewriting, EE float launcher or inherited -m32.
set_property(DIRECTORY PROPERTY COMPILE_OPTIONS "")
set_property(DIRECTORY PROPERTY LINK_OPTIONS "")
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "Luau profiles require clang/clang++ (C++17)")
endif()
# p4mod's build inputs: the API minor it validates against, the retail ELF fingerprint
# `p4mod setup --elf` checks, and the capability fragments its data files come from.
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(P4_LUAU_PYTHON "${Python3_EXECUTABLE}")
file(STRINGS "${P4_LUAU_ROOT}/include/p4pc/packages.h" P4_LUAU_API_MINOR
  REGEX "^#define P4_PACKAGE_API_MINOR [0-9]+$")
string(REGEX REPLACE ".* ([0-9]+)$" "\\1" P4_LUAU_API_MINOR "${P4_LUAU_API_MINOR}")
file(STRINGS "${P4_LUAU_ROOT}/config/retail_fnptr_words.txt" P4_LUAU_ELF_SHA256 REGEX "^sha256 [0-9a-f]+$")
string(REGEX REPLACE "^sha256 " "" P4_LUAU_ELF_SHA256 "${P4_LUAU_ELF_SHA256}")
if(NOT P4_LUAU_API_MINOR MATCHES "^[0-9]+$" OR NOT P4_LUAU_ELF_SHA256 MATCHES "^[0-9a-f]+$")
  message(FATAL_ERROR "p4mod: cannot read P4_PACKAGE_API_MINOR (include/p4pc/packages.h) or the ELF sha256 "
    "(config/retail_fnptr_words.txt)")
endif()
file(GLOB P4_LUAU_FRAGMENTS CONFIGURE_DEPENDS "${P4_LUAU_ROOT}/include/p4pc/p4.d/*.luau")

function(p4_luau_profile suffix)
  set(flags ${ARGN})
  set(vendor "${P4_LUAU_ROOT}/third_party/luau")
  foreach(component Common Ast Bytecode Compiler VM Config Analysis)
    # Every .cpp in these pinned component directories belongs to that component;
    # CodeGen, Inliner, Require, CLI and experimental external deps are excluded.
    file(GLOB sources CONFIGURE_DEPENDS "${vendor}/${component}/src/*.cpp")
    add_library(p4_luau_${component}${suffix} STATIC ${sources})
    target_include_directories(p4_luau_${component}${suffix} PUBLIC "${vendor}/${component}/include")
    p4_host_cxx_target(p4_luau_${component}${suffix} ${flags})
    target_compile_definitions(p4_luau_${component}${suffix} PRIVATE NDEBUG)
  endforeach()
  target_link_libraries(p4_luau_Ast${suffix} PUBLIC p4_luau_Common${suffix})
  target_link_libraries(p4_luau_Bytecode${suffix} PUBLIC p4_luau_Common${suffix})
  target_link_libraries(p4_luau_Compiler${suffix} PUBLIC p4_luau_Ast${suffix} p4_luau_Bytecode${suffix})
  target_link_libraries(p4_luau_VM${suffix} PUBLIC p4_luau_Common${suffix})
  target_link_libraries(p4_luau_Config${suffix} PUBLIC p4_luau_Ast${suffix} PRIVATE p4_luau_Compiler${suffix} p4_luau_VM${suffix})
  target_link_libraries(p4_luau_Analysis${suffix} PUBLIC p4_luau_Ast${suffix} p4_luau_Config${suffix}
    PRIVATE p4_luau_Compiler${suffix} p4_luau_VM${suffix})
  target_compile_options(p4_luau_VM${suffix} PRIVATE -fno-exceptions)
  target_compile_definitions(p4_luau_VM${suffix} PUBLIC LUA_USE_LONGJMP=1
    "$<$<COMPILE_LANGUAGE:CXX>:LUA_API=extern \"C\">")
  target_compile_definitions(p4_luau_Compiler${suffix} PUBLIC
    "$<$<COMPILE_LANGUAGE:CXX>:LUACODE_API=extern \"C\">")

  add_library(p4_luau${suffix} STATIC "${P4_LUAU_ROOT}/src/platform/mods/luau_c.cpp")
  target_include_directories(p4_luau${suffix} PUBLIC "${P4_LUAU_ROOT}/include")
  p4_host_cxx_target(p4_luau${suffix} ${flags})
  target_link_libraries(p4_luau${suffix} PUBLIC p4_luau_VM${suffix} p4_luau_Compiler${suffix})
  p4_host_cxx_runtime(p4_luau${suffix} "${suffix}" ${flags})
  target_link_options(p4_luau_Analysis${suffix} INTERFACE ${flags})

  add_executable(p4_luau_check${suffix} "${P4_LUAU_ROOT}/tools/luau_check.cpp")
  p4_host_cxx_target(p4_luau_check${suffix} ${flags})
  target_link_libraries(p4_luau_check${suffix} PRIVATE p4_luau_Analysis${suffix} p4_luau${suffix})
  # p4mod (docs/modding/p4mod.md): the modder's CLI, shipped beside the game with the composed
  # p4.d.luau and the capability index, both written into its output folder.
  set(p4mod_dir "${CMAKE_CURRENT_BINARY_DIR}/p4mod${suffix}")
  add_custom_command(OUTPUT "${p4mod_dir}/p4.d.luau" "${p4mod_dir}/p4mod-capabilities.txt"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${p4mod_dir}"
    COMMAND "${P4_LUAU_PYTHON}" "${P4_LUAU_ROOT}/tools/p4_capabilities.py" compose "${p4mod_dir}/p4.d.luau"
    COMMAND "${P4_LUAU_PYTHON}" "${P4_LUAU_ROOT}/tools/p4_capabilities.py" index "${p4mod_dir}/p4mod-capabilities.txt"
    DEPENDS "${P4_LUAU_ROOT}/tools/p4_capabilities.py" "${P4_LUAU_ROOT}/include/p4pc/p4.d.luau" ${P4_LUAU_FRAGMENTS}
    COMMENT "p4mod${suffix}: composing p4.d.luau and the capability index" VERBATIM)
  add_library(p4mod_miniz${suffix} STATIC "${P4_LUAU_ROOT}/third_party/miniz/miniz.c"
    "${P4_LUAU_ROOT}/third_party/miniz/miniz_tdef.c")
  target_include_directories(p4mod_miniz${suffix} PUBLIC "${P4_LUAU_ROOT}/third_party/miniz")
  target_compile_options(p4mod_miniz${suffix} PRIVATE ${flags} -O2 -g1)
  # The game's own package rules for p4mod check/test/pack, built as host code: the package
  # file index (file_index.c over the no-follow calls in tools/p4mod_os.c), the edits.xml
  # decoder and field rules (game_data_edit*.c, P4_DATA_EDIT_CHECK: nothing written), the
  # table-source metadata and XML decoder (game_table_*.c, P4_TABLE_XML_CHECK: no builder).
  # The check builds rely on clang dropping code after DECODED's return (see game_table_xml.c).
  set(p4mod_mods "${P4_LUAU_ROOT}/src/platform/mods")
  add_library(p4mod_gamedata${suffix} STATIC "${P4_LUAU_ROOT}/tools/p4mod_data_edits.c"
    "${P4_LUAU_ROOT}/tools/p4mod_package.c" "${P4_LUAU_ROOT}/tools/p4mod_tables.c" "${P4_LUAU_ROOT}/tools/p4mod_os.c"
    "${p4mod_mods}/game_data_edit_xml.c" "${p4mod_mods}/game_data_edit.c" "${p4mod_mods}/game_data.c"
    "${p4mod_mods}/game_table_manifest.c" "${p4mod_mods}/game_table_xml.c" "${p4mod_mods}/file_index.c"
    "${p4mod_mods}/compendium.c" "${p4mod_mods}/package_memory.c" "${P4_LUAU_ROOT}/src/platform/content_digest.c"
    "${p4mod_mods}/mod_setting_gate.c" "${p4mod_mods}/mod_setting_store.c" "${P4_LUAU_ROOT}/third_party/yxml/yxml.c")
  target_include_directories(p4mod_gamedata${suffix} PRIVATE "${P4_LUAU_ROOT}/include" "${p4mod_mods}"
    "${P4_LUAU_ROOT}/src/platform/files" PUBLIC "${P4_LUAU_ROOT}/third_party/yxml")
  # P4_MOD_SETTING_CHECK: the [setting] declarations and `when` gate rules, without stored values.
  target_compile_definitions(p4mod_gamedata${suffix} PRIVATE P4_DATA_EDIT_CHECK=1 P4_TABLE_XML_CHECK=1
    P4_MOD_SETTING_CHECK=1)
  target_compile_options(p4mod_gamedata${suffix} PRIVATE ${flags} -O2 -g1)
  add_executable(p4mod${suffix} "${P4_LUAU_ROOT}/tools/p4mod.cpp"
    "${p4mod_dir}/p4.d.luau" "${p4mod_dir}/p4mod-capabilities.txt")
  p4_host_cxx_target(p4mod${suffix} ${flags})
  target_compile_definitions(p4mod${suffix} PRIVATE P4MOD_API_MINOR=${P4_LUAU_API_MINOR}
    "P4MOD_ELF_SHA256=\"${P4_LUAU_ELF_SHA256}\"")
  target_include_directories(p4mod${suffix} PRIVATE "${p4mod_mods}") # package_identifier.h
  target_link_libraries(p4mod${suffix} PRIVATE p4_luau_Analysis${suffix} p4_luau${suffix} p4mod_miniz${suffix}
    p4mod_gamedata${suffix})
  if(suffix STREQUAL "64w")
    target_link_libraries(p4mod${suffix} PRIVATE advapi32) # private test-folder ACL (sddl)
  endif()
  set_target_properties(p4mod${suffix} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${p4mod_dir}")
  add_executable(p4mod_game_fixture${suffix} "${P4_LUAU_ROOT}/tests/p4mod_game_fixture.c")
  target_compile_options(p4mod_game_fixture${suffix} PRIVATE ${flags})
  target_link_options(p4mod_game_fixture${suffix} PRIVATE ${flags})
  add_executable(p4mod_zip32_test${suffix} "${P4_LUAU_ROOT}/tests/p4mod_zip32_test.c")
  target_compile_options(p4mod_zip32_test${suffix} PRIVATE ${flags})
  target_link_options(p4mod_zip32_test${suffix} PRIVATE ${flags})
  if(suffix STREQUAL "64w")
    set_target_properties(p4mod_game_fixture${suffix} p4mod_zip32_test${suffix} PROPERTIES SUFFIX ".exe")
  endif()
  add_executable(luau_sandbox_test${suffix} "${P4_LUAU_ROOT}/tests/luau_sandbox_test.c")
  target_compile_options(luau_sandbox_test${suffix} PRIVATE ${flags} -O2 -g1)
  target_link_libraries(luau_sandbox_test${suffix} PRIVATE p4_luau${suffix})
  # Headless mod-only qualification: bundled mods' Luau in this pinned VM with
  # a synthetic p4 fixture (no game, no budget) and the shared native digest.
  add_executable(luau_script_runner${suffix} "${P4_LUAU_ROOT}/tests/luau_script_runner.c"
    "${P4_LUAU_ROOT}/src/platform/content_digest.c")
  target_compile_options(luau_script_runner${suffix} PRIVATE ${flags} -O2 -g1)
  target_link_libraries(luau_script_runner${suffix} PRIVATE p4_luau${suffix})
  add_executable(fusion_arts_safety_test${suffix} "${P4_LUAU_ROOT}/tests/fusion_arts_safety_test.c")
  target_compile_options(fusion_arts_safety_test${suffix} PRIVATE ${flags} -O2 -g1)
  target_link_libraries(fusion_arts_safety_test${suffix} PRIVATE p4_luau${suffix})
  foreach(kind probe baseline)
    add_executable(p4_luau_size_${kind}${suffix} EXCLUDE_FROM_ALL "${P4_LUAU_ROOT}/tools/luau_size_probe.c")
    target_compile_options(p4_luau_size_${kind}${suffix} PRIVATE ${flags} -O2 -g1)
    target_link_libraries(p4_luau_size_${kind}${suffix} PRIVATE p4_luau${suffix})
  endforeach()
  target_compile_definitions(p4_luau_size_baseline${suffix} PRIVATE P4_LUAU_SIZE_BASELINE=1)
  if(suffix STREQUAL "64w")
    set_target_properties(p4_luau_check${suffix} p4mod${suffix} luau_sandbox_test${suffix} luau_script_runner${suffix}
      fusion_arts_safety_test${suffix}
      p4_luau_size_probe${suffix} p4_luau_size_baseline${suffix} PROPERTIES
      SUFFIX ".exe")
    find_program(P4_LUAU_WINE NAMES wine64 wine)
    if(NOT P4_LUAU_WINE)
      message(STATUS "Luau64w runtime tests skipped: Wine unavailable; provider and test binaries remain enabled")
      return()
    endif()
    add_test(NAME luau_sandbox${suffix} COMMAND "${P4_LUAU_WINE}" "$<TARGET_FILE:luau_sandbox_test${suffix}>")
    add_test(NAME fusion_arts_safety${suffix} COMMAND "${P4_LUAU_WINE}" "$<TARGET_FILE:fusion_arts_safety_test${suffix}>"
      "${P4_LUAU_ROOT}/tests/fusion_arts_safety_setup.luau" "${P4_LUAU_ROOT}/mods/p4pc.fusion-arts/main.luau"
      "${P4_LUAU_ROOT}/tests/fusion_arts_safety_cases.luau")
  else()
    add_test(NAME luau_sandbox${suffix} COMMAND luau_sandbox_test${suffix})
    add_test(NAME fusion_arts_safety${suffix} COMMAND fusion_arts_safety_test${suffix}
      "${P4_LUAU_ROOT}/tests/fusion_arts_safety_setup.luau" "${P4_LUAU_ROOT}/mods/p4pc.fusion-arts/main.luau"
      "${P4_LUAU_ROOT}/tests/fusion_arts_safety_cases.luau")
  endif()
  set_tests_properties(luau_sandbox${suffix} PROPERTIES LABELS "host" TIMEOUT 20
    ENVIRONMENT "P4PC_INPUT=script;P4PC_MODS=0;P4PC_PACKS=0")
  set_tests_properties(fusion_arts_safety${suffix} PROPERTIES LABELS "host" TIMEOUT 20
    ENVIRONMENT "P4PC_INPUT=script;P4PC_MODS=0;P4PC_PACKS=0")
  if(suffix STREQUAL "64")
    set_property(TEST luau_sandbox${suffix} APPEND PROPERTY LABELS port64)
    set_property(TEST fusion_arts_safety${suffix} APPEND PROPERTY LABELS port64)
  elseif(suffix STREQUAL "64w")
    set_tests_properties(luau_sandbox${suffix} PROPERTIES LABELS "host;port64win"
      FIXTURES_REQUIRED wine64_ready)
    set_property(TEST luau_sandbox${suffix} APPEND PROPERTY ENVIRONMENT
      WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    p4_wine_scale_timeouts(luau_sandbox${suffix})
    set_tests_properties(fusion_arts_safety${suffix} PROPERTIES LABELS "host;port64win"
      FIXTURES_REQUIRED wine64_ready)
    set_property(TEST fusion_arts_safety${suffix} APPEND PROPERTY ENVIRONMENT
      WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    p4_wine_scale_timeouts(fusion_arts_safety${suffix})
  endif()
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  get_target_property(emulator p4_luau_check${suffix} CROSSCOMPILING_EMULATOR)
  if(NOT emulator)
    set(emulator "")
  endif()
  if(suffix STREQUAL "64w")
    set(emulator "${P4_LUAU_WINE}")
  endif()
  add_test(NAME mod_golden_station${suffix} COMMAND ${emulator} "$<TARGET_FILE:luau_script_runner${suffix}>"
    "${P4_LUAU_ROOT}/tests/mod_golden_station_fixture.luau"
    "${P4_LUAU_ROOT}/mods/p4pc.golden/main.luau"
    "${P4_LUAU_ROOT}/tests/mod_golden_station_test.luau")
  set_tests_properties(mod_golden_station${suffix} PROPERTIES LABELS "host" TIMEOUT 30)
  if(suffix STREQUAL "64")
    set_property(TEST mod_golden_station${suffix} APPEND PROPERTY LABELS port64)
  elseif(suffix STREQUAL "64w")
    set_tests_properties(mod_golden_station${suffix} PROPERTIES LABELS "host;port64win"
      FIXTURES_REQUIRED wine64_ready ENVIRONMENT "WINEDEBUG=-all;WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    p4_wine_scale_timeouts(mod_golden_station${suffix})
  endif()
  # The seeded randomizer's engine, vectors and run lifecycle: the same pinned
  # digests on every target (Linux i386/x86_64/AArch64 under its emulator,
  # Windows x64 under Wine); a determinism witness, not a gameplay receipt.
  add_test(NAME mod_randomizer_engine${suffix} COMMAND "${Python3_EXECUTABLE}"
    "${P4_LUAU_ROOT}/tests/mod_randomizer_engine_test.py" -- ${emulator} "$<TARGET_FILE:luau_script_runner${suffix}>")
  set_tests_properties(mod_randomizer_engine${suffix} PROPERTIES LABELS "host" TIMEOUT 120
    ENVIRONMENT "P4PC_INPUT=script;P4PC_MODS=0;P4PC_PACKS=0;P4PC_ISO=")
  if(suffix STREQUAL "64")
    set_property(TEST mod_randomizer_engine${suffix} APPEND PROPERTY LABELS port64)
  elseif(suffix STREQUAL "64w")
    set_tests_properties(mod_randomizer_engine${suffix} PROPERTIES LABELS "host;port64win"
      FIXTURES_REQUIRED wine64_ready)
    set_property(TEST mod_randomizer_engine${suffix} APPEND PROPERTY ENVIRONMENT
      WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    p4_wine_scale_timeouts(mod_randomizer_engine${suffix})
  endif()
  # Hard mode's run lifecycle (save block reset/load, Camp badge) in the real VM
  # over a synthetic p4 provider; not a gameplay witness.
  add_test(NAME mod_hardmode_lifecycle${suffix} COMMAND "${Python3_EXECUTABLE}"
    "${P4_LUAU_ROOT}/tests/mod_hardmode_lifecycle_test.py" -- ${emulator} "$<TARGET_FILE:luau_script_runner${suffix}>")
  set_tests_properties(mod_hardmode_lifecycle${suffix} PROPERTIES LABELS "host" TIMEOUT 120
    ENVIRONMENT "P4PC_INPUT=script;P4PC_MODS=0;P4PC_PACKS=0")
  if(suffix STREQUAL "64")
    set_property(TEST mod_hardmode_lifecycle${suffix} APPEND PROPERTY LABELS port64)
  elseif(suffix STREQUAL "64w")
    set_tests_properties(mod_hardmode_lifecycle${suffix} PROPERTIES LABELS "host;port64win"
      FIXTURES_REQUIRED wine64_ready)
    set_property(TEST mod_hardmode_lifecycle${suffix} APPEND PROPERTY ENVIRONMENT
      WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
    p4_wine_scale_timeouts(mod_hardmode_lifecycle${suffix})
  endif()
  if(NOT CMAKE_CROSSCOMPILING OR suffix STREQUAL "64w")
    add_test(NAME luau_analysis${suffix} COMMAND "${Python3_EXECUTABLE}"
      "${P4_LUAU_ROOT}/tests/luau_check_test.py" ${emulator} "$<TARGET_FILE:p4_luau_check${suffix}>")
    add_test(NAME p4mod${suffix} COMMAND "${Python3_EXECUTABLE}"
      "${P4_LUAU_ROOT}/tests/p4mod_test.py" ${emulator} "$<TARGET_FILE:p4mod${suffix}>"
      --game-fixture ${emulator} "$<TARGET_FILE:p4mod_game_fixture${suffix}>")
    add_test(NAME p4mod_zip32${suffix} COMMAND ${emulator} "$<TARGET_FILE:p4mod_zip32_test${suffix}>")
    foreach(test luau_analysis${suffix} p4mod${suffix} p4mod_zip32${suffix})
      set_tests_properties(${test} PROPERTIES LABELS "host" TIMEOUT 30
        ENVIRONMENT "P4PC_INPUT=script;P4PC_MODS=0;P4PC_PACKS=0")
      if(suffix STREQUAL "64")
        set_property(TEST ${test} APPEND PROPERTY LABELS port64)
      elseif(suffix STREQUAL "64w")
        set_tests_properties(${test} PROPERTIES LABELS "host;port64win"
          FIXTURES_REQUIRED wine64_ready)
        set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT
          WINEDEBUG=-all "WINEPREFIX=${CMAKE_BINARY_DIR}/tests/wine64prefix")
        p4_wine_scale_timeouts(${test})
      endif()
    endforeach()
  endif()
endfunction()

if(P4_LUAU_STANDALONE)
  p4_luau_profile("")
else()
  p4_host_cxx_profiles(p4_luau_profile)
  foreach(suffix "" 64 64w)
    if(TARGET p4_luau${suffix})
      set(host_args --host-lib "$<TARGET_FILE:p4_luau${suffix}>"
        --host-lib "$<TARGET_FILE:p4_luau_VM${suffix}>"
        --host-lib "$<TARGET_FILE:p4_luau_Compiler${suffix}>")
      set(P4_LUAU_HOST_ARGS${suffix} ${host_args} PARENT_SCOPE)
    endif()
  endforeach()
endif()

# tests/cmake/230-platform-policy.cmake scales the time limits of tests/'s own tests by four for the AArch64
# profile, which runs under qemu-user on x86 hosts; this directory's tests are not in that directory's list.
# 2026-11-10: mod_randomizer_engine64 (19.5 s natively) was killed at its unscaled 120 s in the emulated a64
# lane, so the lane could give no receipt for the Randomizer milestone or anything stacked on it.
if(P4_PORT64_ARCH STREQUAL "aarch64")
  get_property(_p4_luau_tests DIRECTORY PROPERTY TESTS)
  foreach(_p4_luau_test IN LISTS _p4_luau_tests)
    if(_p4_luau_test MATCHES "64$")
      get_property(_p4_luau_timeout TEST ${_p4_luau_test} PROPERTY TIMEOUT)
      if(_p4_luau_timeout)
        math(EXPR _p4_luau_timeout "${_p4_luau_timeout} * 4")
        set_property(TEST ${_p4_luau_test} PROPERTY TIMEOUT ${_p4_luau_timeout})
      endif()
    endif()
  endforeach()
endif()
