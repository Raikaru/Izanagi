# GP varargs are an ABI requirement independent of the optional EE arithmetic
# rewrite. Both phases use the real compiler flags and preserve original deps.
function(p4_printf_launcher target root modes)
  set(phase_args "--modes=${modes}" "--printf-root=${root}")
  if(P4_WIN32)
    list(APPEND phase_args "--printf-symbol-prefix=_")
  endif()
  set_target_properties(${target} PROPERTIES C_COMPILER_LAUNCHER
    "${P4_PYTHON};${CMAKE_SOURCE_DIR}/tools/ee_float_cc.py;${phase_args}")
  get_target_property(phase_sources ${target} SOURCES)
  set_property(SOURCE ${phase_sources} APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_SOURCE_DIR}/tools/ee_float_cc.py"
    "${CMAKE_SOURCE_DIR}/tools/port64/printf_rewrite.py"
    "${CMAKE_SOURCE_DIR}/tools/port64/gptr_rewrite.py")
endfunction()
set(P4_DECOMP_ARITHMETIC none)
if(P4_EE_FLOAT)
  set(P4_DECOMP_ARITHMETIC "${P4_EE_FLOAT_OPS}")
endif()
p4_printf_launcher(p4decomp "${P4_GEN}/decomp" "${P4_DECOMP_ARITHMETIC}")

