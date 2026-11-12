# Shared native-adapter callbacks use the same Win64 prefix gate as consumers.
set(P4_WIN64_PREFIX "/opt/p4win64" CACHE PATH "build_deps.sh output for x86_64-w64-mingw32")

enable_testing()
if(P4PC_LUAU)
  add_subdirectory(tools/luau)
endif()
