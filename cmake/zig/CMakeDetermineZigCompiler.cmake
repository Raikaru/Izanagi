# enable_language(Zig) (cmake/sources/055-zig.cmake): the compiler 055-zig.cmake resolved and checked against the pin at this configure.
if(NOT P4_ZIG_EXECUTABLE)
  message(FATAL_ERROR "enable_language(Zig) runs from cmake/sources/055-zig.cmake, which resolves the pinned Zig first")
endif()
set(CMAKE_Zig_COMPILER "${P4_ZIG_EXECUTABLE}")
set(CMAKE_Zig_COMPILER_ID "Zig")
set(CMAKE_Zig_COMPILER_VERSION "${P4_ZIG_VERSION}")
set(CMAKE_Zig_COMPILER_ENV_VAR "P4_ZIG")
configure_file("${CMAKE_CURRENT_LIST_DIR}/CMakeZigCompiler.cmake.in" "${CMAKE_PLATFORM_INFO_DIR}/CMakeZigCompiler.cmake" @ONLY)
