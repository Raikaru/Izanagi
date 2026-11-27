# The build id (include/p4pc/build_id.h): which git commit, and whether the tree was dirty, this binary was built from. tools/build_id.py reads the
# source tree at configure time and writes gen/p4_build_id.c, rewriting it only when its content changes, so only that translation unit rebuilds
# when the commit does. The text depends on the source tree alone (no time, host or build directory), keeping builds reproducible.
# A new commit or a changed index reconfigures the build, so the id follows the tree.
set(P4_BUILD_ID_C "${P4_GEN}/p4_build_id.c")
execute_process(COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/build_id.py" --source "${CMAKE_SOURCE_DIR}" --out "${P4_BUILD_ID_C}"
  RESULT_VARIABLE _p4_build_id_rc OUTPUT_QUIET ERROR_QUIET)
if(NOT _p4_build_id_rc EQUAL 0 OR NOT EXISTS "${P4_BUILD_ID_C}")
  file(WRITE "${P4_BUILD_ID_C}" "#include \"p4pc/build_id.h\"\nconst char *p4_build_id(void) { return \"unknown\"; }\nconst char *p4_build_commit(void) { return \"unknown\"; }\nint p4_build_dirty(void) { return 0; }\n")
endif()
execute_process(COMMAND git -C "${CMAKE_SOURCE_DIR}" rev-parse --absolute-git-dir OUTPUT_VARIABLE _p4_git_dir OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE _p4_git_rc ERROR_QUIET)
if(_p4_git_rc EQUAL 0)
  foreach(_p4_git_file logs/HEAD index)
    if(EXISTS "${_p4_git_dir}/${_p4_git_file}")
      set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_p4_git_dir}/${_p4_git_file}")
    endif()
  endforeach()
endif()
target_sources(p4port PRIVATE "${P4_BUILD_ID_C}")
unset(_p4_build_id_rc)
unset(_p4_git_dir)
unset(_p4_git_rc)
unset(_p4_git_file)
