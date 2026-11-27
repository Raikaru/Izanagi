# The pinned version was checked when the compiler was determined (CMakeDetermineZigCompiler.cmake); every object's own compile
# is the working check, and a failure there stops the build rather than falling back.
set(CMAKE_Zig_COMPILER_WORKS 1 CACHE INTERNAL "")
