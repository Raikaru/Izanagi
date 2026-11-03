# Inspect actual object references, not source spellings or the final executable's
# merged host/game imports. LLVM tools understand every ELF/COFF/Mach-O target we build.
# macOS: the LLVM tools beside the pinned clang (ci/macos/build_deps.sh) come first, not Xcode's.
set(_p4_crt_tool_hints)
if(APPLE)
  get_filename_component(_p4_crt_tool_hints "${CMAKE_C_COMPILER}" DIRECTORY)
endif()
find_program(P4_CRT_NM NAMES llvm-nm nm HINTS ${_p4_crt_tool_hints} REQUIRED)
find_program(P4_CRT_OBJDUMP NAMES llvm-objdump objdump HINTS ${_p4_crt_tool_hints} REQUIRED)
add_custom_target(p4crt ALL)
function(p4_crt_guard profile objects definitions)
  set(object_file "${P4_GEN}/crt-${profile}-objects.txt")
  set(definition_file "${P4_GEN}/crt-${profile}-definitions.txt")
  file(GENERATE OUTPUT "${object_file}" CONTENT "$<JOIN:${objects},\n>\n")
  file(GENERATE OUTPUT "${definition_file}" CONTENT "$<JOIN:${definitions},\n>\n")
  set(stamp "${P4_GEN}/crt-${profile}.ok")
  add_custom_command(OUTPUT "${stamp}"
    BYPRODUCTS "${P4_GEN}/crt-${profile}.json" "${P4_GEN}/crt-${profile}.tsv"
    COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/crt_guard.py"
      --objects-file "${object_file}" --definitions-file "${definition_file}"
      --decomp "${P4_DECOMP}" --build "${P4_GEN}" --nm "${P4_CRT_NM}" --objdump "${P4_CRT_OBJDUMP}"
      --compile-commands "${CMAKE_BINARY_DIR}/compile_commands.json"
      --report "${P4_GEN}/crt-${profile}.json" ${ARGN}
    COMMAND "${CMAKE_COMMAND}" -E touch "${stamp}"
    DEPENDS ${objects} ${definitions} "${object_file}" "${definition_file}"
      "${CMAKE_SOURCE_DIR}/tools/crt_guard.py" "${CMAKE_SOURCE_DIR}/tools/gen_port.py"
      "${CMAKE_SOURCE_DIR}/config/host_symbols_allow.txt"
      "${CMAKE_SOURCE_DIR}/config/crt_host_adapters.json"
      "${CMAKE_SOURCE_DIR}/config/call_traps.txt"
      "${CMAKE_SOURCE_DIR}/tools/crt_flow.py" "${CMAKE_BINARY_DIR}/compile_commands.json"
      "${CMAKE_SOURCE_DIR}/config/port_symbols.txt" "${CMAKE_SOURCE_DIR}/config/port_symbols_manual.txt"
      "${CMAKE_SOURCE_DIR}/config/native_names.txt"
      "${CMAKE_SOURCE_DIR}/include/p4pc/libc_rename.h" ${P4_DECOMP_SYMBOL_FILES}
    VERBATIM COMMENT "Auditing ${profile} game CRT bindings and callers")
  add_custom_target(p4crt_${profile} DEPENDS "${stamp}")
  add_dependencies(p4crt p4crt_${profile})
  set(P4_CRT_${profile}_STAMP "${stamp}" PARENT_SCOPE)
endfunction()

# Bootstrap/viewer sources belong to executable targets rather than p4port.
# Audit their actual compile-command objects after compilation and before link.
function(p4_crt_entry target definitions)
  if(APPLE)
    target_link_options(${target} PRIVATE "LINKER:-map,${P4_GEN}/crt-link-${target}.map")   # (ld64's spelling)
  else()
    target_link_options(${target} PRIVATE "LINKER:-Map,${P4_GEN}/crt-link-${target}.map")
  endif()
  add_custom_command(TARGET ${target} PRE_LINK
    COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/crt_guard.py"
      --compile-commands "${CMAKE_BINARY_DIR}/compile_commands.json" --compile-target "${target}"
      --definitions-file "${definitions}"
      --decomp "${P4_DECOMP}" --build "${P4_GEN}" --nm "${P4_CRT_NM}" --objdump "${P4_CRT_OBJDUMP}"
      --report "${P4_GEN}/crt-entry-${target}.json" ${ARGN}
    VERBATIM COMMENT "Auditing ${target} native entry CRT bindings")
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_SOURCE_DIR}/tools/crt_guard.py" "${CMAKE_SOURCE_DIR}/config/crt_host_adapters.json"
    "${CMAKE_SOURCE_DIR}/config/host_symbols_allow.txt" "${CMAKE_SOURCE_DIR}/tools/crt_flow.py"
    "${CMAKE_SOURCE_DIR}/config/call_traps.txt")
endfunction()

