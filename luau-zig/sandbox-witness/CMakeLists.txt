cmake_minimum_required(VERSION 3.20)
# p4_luau sandbox behavioral witnesses. One set of C sources, built against:
#   reference - the authoritative production bridge src/platform/mods/luau_c.cpp
#               over vendored upstream Luau 0.744 (third_party/luau, MIT);
#   candidate - a prebuilt library implementing include/p4pc/luau_c.h and the
#               lua.h/lualib.h API it exposes (the Zig `p4pc-luau` archive).
# No upstream Luau code is compiled into the candidate executables.
project(p4pc_luau_sandbox_witness LANGUAGES C)
enable_testing()

get_filename_component(P4_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(P4_LUAU_VENDOR "${P4_ROOT}/third_party/luau")

set(P4PC_LUAU_CANDIDATE_LIBRARY "" CACHE FILEPATH
  "Candidate p4_luau library, e.g. luau-zig/zig-out/lib/libp4pc-luau.a (zig build p4pc)")
set(P4PC_LUAU_CANDIDATE_LINK_LIBRARIES "" CACHE STRING
  "Extra link inputs the candidate archive needs on this platform (;-separated)")
option(P4PC_LUAU_WITNESS_REFERENCE "Also build the witnesses against production luau_c.cpp" ON)
set(P4PC_LUAU_WITNESS_EMULATOR "" CACHE STRING
  "Command prefix used to run the witness executables (for example wine64)")

if(NOT P4PC_LUAU_CANDIDATE_LIBRARY AND NOT P4PC_LUAU_WITNESS_REFERENCE)
  message(FATAL_ERROR "Set P4PC_LUAU_CANDIDATE_LIBRARY and/or P4PC_LUAU_WITNESS_REFERENCE=ON")
endif()

set(P4PC_LUAU_WITNESSES globals budget memory allocator heap mod)

function(p4pc_luau_witnesses flavor)
  foreach(witness IN LISTS P4PC_LUAU_WITNESSES)
    set(target sandbox_witness_${witness}_${flavor})
    add_executable(${target} "${CMAKE_CURRENT_LIST_DIR}/witness_${witness}.c")
    set_target_properties(${target} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_LIST_DIR}" "${P4_ROOT}/include"
      "${P4_LUAU_VENDOR}/VM/include")
    target_link_libraries(${target} PRIVATE ${ARGN})
    add_test(NAME ${target} COMMAND ${P4PC_LUAU_WITNESS_EMULATOR} "$<TARGET_FILE:${target}>")
    # 77 is an explicit SKIP (property not observable on this platform).
    set_tests_properties(${target} PROPERTIES LABELS "luau-sandbox-witness;${flavor}" TIMEOUT 300
      SKIP_RETURN_CODE 77 ENVIRONMENT "P4PC_INPUT=script;P4PC_MODS=0;P4PC_PACKS=0;P4PC_ISO=")
  endforeach()
endfunction()

if(P4PC_LUAU_WITNESS_REFERENCE)
  enable_language(CXX)
  # Same component split and VM/compiler definitions as tools/luau/CMakeLists.txt.
  foreach(component Common Ast Bytecode Compiler VM)
    file(GLOB sources CONFIGURE_DEPENDS "${P4_LUAU_VENDOR}/${component}/src/*.cpp")
    add_library(witness_ref_${component} STATIC ${sources})
    target_include_directories(witness_ref_${component} PUBLIC "${P4_LUAU_VENDOR}/${component}/include")
    target_compile_features(witness_ref_${component} PUBLIC cxx_std_17)
    target_compile_definitions(witness_ref_${component} PRIVATE NDEBUG)
  endforeach()
  target_link_libraries(witness_ref_Ast PUBLIC witness_ref_Common)
  target_link_libraries(witness_ref_Bytecode PUBLIC witness_ref_Common)
  target_link_libraries(witness_ref_Compiler PUBLIC witness_ref_Ast witness_ref_Bytecode)
  target_link_libraries(witness_ref_VM PUBLIC witness_ref_Common)
  if(NOT MSVC)
    target_compile_options(witness_ref_VM PRIVATE -fno-exceptions)
  endif()
  target_compile_definitions(witness_ref_VM PUBLIC LUA_USE_LONGJMP=1
    "$<$<COMPILE_LANGUAGE:CXX>:LUA_API=extern \"C\">")
  target_compile_definitions(witness_ref_Compiler PUBLIC
    "$<$<COMPILE_LANGUAGE:CXX>:LUACODE_API=extern \"C\">")
  add_library(witness_ref_bridge STATIC "${P4_ROOT}/src/platform/mods/luau_c.cpp")
  target_include_directories(witness_ref_bridge PUBLIC "${P4_ROOT}/include")
  target_compile_features(witness_ref_bridge PUBLIC cxx_std_17)
  target_link_libraries(witness_ref_bridge PUBLIC witness_ref_VM witness_ref_Compiler)
  p4pc_luau_witnesses(reference witness_ref_bridge)
endif()

if(P4PC_LUAU_CANDIDATE_LIBRARY)
  if(NOT EXISTS "${P4PC_LUAU_CANDIDATE_LIBRARY}")
    message(FATAL_ERROR "P4PC_LUAU_CANDIDATE_LIBRARY does not exist: ${P4PC_LUAU_CANDIDATE_LIBRARY}")
  endif()
  add_library(witness_candidate STATIC IMPORTED)
  set_target_properties(witness_candidate PROPERTIES IMPORTED_LOCATION "${P4PC_LUAU_CANDIDATE_LIBRARY}")
  set(candidate_system_libraries "")
  if(UNIX AND NOT APPLE)
    list(APPEND candidate_system_libraries m) # libc math the archive may import
  endif()
  p4pc_luau_witnesses(candidate witness_candidate ${P4PC_LUAU_CANDIDATE_LINK_LIBRARIES}
    ${candidate_system_libraries})
endif()
