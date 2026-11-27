# How CMake compiles a .zig source of a target: cmake/zig/zig_object.py turns the target's own compile options (the profile's
# clang --target/-m32/-m64, optimisation and debug levels), definitions and include directories into one `zig translate-c` of the
# root's binding header (when its module imports c_bindings) and one `zig build-obj`, so a Zig object is an ordinary object of its
# target: it is in $<TARGET_OBJECTS:...> (the tables, the CRT audits, every link) and in the copies of p4port's SOURCES that the
# 64-bit profiles compile. The rule's inputs come from cmake/sources/055-zig.cmake.
set(CMAKE_Zig_OUTPUT_EXTENSION "${CMAKE_C_OUTPUT_EXTENSION}")
set(CMAKE_INCLUDE_FLAG_Zig "-I")
set(CMAKE_INCLUDE_SYSTEM_FLAG_Zig "-isystem ")
set(CMAKE_Zig_DEFINE_FLAG "-D")
set(CMAKE_Zig_FLAGS_INIT "")
set(CMAKE_Zig_FLAGS "" CACHE STRING "extra flags for every Zig object (clang spelling, translated by cmake/zig/zig_object.py)")
foreach(_p4_zig_config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set(CMAKE_Zig_FLAGS_${_p4_zig_config} "")
endforeach()
unset(_p4_zig_config)

# The depfile (the source, the Zig files it imports, its binding header and the repository headers that includes, and the driver).
set(CMAKE_DEPFILE_FLAGS_Zig "--depfile <DEP_FILE>")
set(CMAKE_Zig_DEPFILE_FORMAT gcc)
set(CMAKE_NINJA_DEPTYPE_Zig gcc)
set(CMAKE_Zig_DEPENDS_USE_COMPILER TRUE)

set(_p4_zig_android)
if(P4_ZIG_ANDROID_SYSROOT)
  set(_p4_zig_android "--android-sysroot \"${P4_ZIG_ANDROID_SYSROOT}\"")
endif()
# The executable is the one 055-zig.cmake resolved at this configure (not the language's first-configure cache), so a changed
# -DP4_ZIG changes the rule and Ninja recompiles every Zig object.
set(CMAKE_Zig_COMPILE_OBJECT
  "\"${P4_PYTHON}\" \"${CMAKE_CURRENT_LIST_DIR}/zig_object.py\" --zig \"${P4_ZIG_EXECUTABLE}\" --default-target ${P4_ZIG_DEFAULT_TARGET} --optimize ${P4_ZIG_OPTIMIZE} --cache-dir \"${CMAKE_BINARY_DIR}/zig-cache\" --global-cache-dir \"${CMAKE_BINARY_DIR}/zig-global-cache\" ${_p4_zig_android} -o <OBJECT> -c <SOURCE> -- <DEFINES> <INCLUDES> <FLAGS>")
unset(_p4_zig_android)
set(CMAKE_Zig_INFORMATION_LOADED 1)
