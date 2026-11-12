# API capability registry (CONTRIBUTING.md "API capabilities"): one declaration fragment per
# key, include/p4pc/p4.d/<key>.luau, whose first line is `--!capability <key>@<revision>`.
# This generates the native table behind p4.has, p4.capabilities and mod.ini `requires=`
# (package_registry.c); tools/p4_capabilities.py composes the same files into p4.d.luau.
# Before any target exists, so every target that compiles package_registry.c sees it.
file(GLOB _p4_capability_files CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/p4pc/p4.d/*.luau")
# GLOB CONFIGURE_DEPENDS notices added and removed fragments, but not an
# existing fragment's changed revision. Its contents drive the generated table.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_p4_capability_files})
set(_p4_capability_entries)
foreach(_p4_capability_file IN LISTS _p4_capability_files)
  get_filename_component(_p4_capability_name "${_p4_capability_file}" NAME)
  file(STRINGS "${_p4_capability_file}" _p4_capability_header LIMIT_COUNT 1)
  # Two steps: CMAKE_MATCH_1 is set by the MATCHES, after this if()'s own arguments expanded.
  if(_p4_capability_header MATCHES "^--!capability ([a-z][a-z0-9_-]*(\\.[a-z][a-z0-9_-]*)*)@([1-9][0-9]*)$")
    set(_p4_capability_key "${CMAKE_MATCH_1}")
    set(_p4_capability_revision "${CMAKE_MATCH_3}")
  else()
    set(_p4_capability_key "")
  endif()
  if(NOT "${_p4_capability_key}.luau" STREQUAL "${_p4_capability_name}")
    message(FATAL_ERROR "include/p4pc/p4.d/${_p4_capability_name}: the first line must be "
      "`--!capability <key>@<revision>` naming this file's key (python tools/p4_capabilities.py check)")
  endif()
  # A space sorts before every key character, so this orders by key.
  list(APPEND _p4_capability_entries "${_p4_capability_key} ${_p4_capability_revision}")
endforeach()
list(SORT _p4_capability_entries)
set(_p4_capability_source "/* Generated from include/p4pc/p4.d/<key>.luau by cmake/sources/052-capabilities.cmake. */\n")
foreach(_p4_capability_entry IN LISTS _p4_capability_entries)
  string(REPLACE " " ";" _p4_capability_pair "${_p4_capability_entry}")
  list(GET _p4_capability_pair 0 _p4_capability_key)
  list(GET _p4_capability_pair 1 _p4_capability_revision)
  string(APPEND _p4_capability_source "P4_CAPABILITY(\"${_p4_capability_key}\", ${_p4_capability_revision}u)\n")
endforeach()
# Rewritten only when the registry changes, so an unchanged tree recompiles nothing.
file(CONFIGURE OUTPUT "${P4_GEN}/capabilities/p4_capabilities.inc" CONTENT "${_p4_capability_source}" @ONLY)
include_directories("${P4_GEN}/capabilities")
unset(_p4_capability_files)
unset(_p4_capability_file)
unset(_p4_capability_name)
unset(_p4_capability_header)
unset(_p4_capability_entries)
unset(_p4_capability_entry)
unset(_p4_capability_pair)
unset(_p4_capability_key)
unset(_p4_capability_revision)
unset(_p4_capability_source)
