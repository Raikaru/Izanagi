set(P4_DECOMP "${CMAKE_SOURCE_DIR}/decomp" CACHE PATH
    "Persona4-Decompilation checkout (provides src/, include/, config/)")
set(P4_RETAIL "" CACHE FILEPATH
    "Optional retail SLUS_217.82 executable (never committed): checks config/retail_fnptr_words.txt and enables the tests that read it")
set(P4_LIBC32 "/usr/lib32/libc.so.6" CACHE FILEPATH
    "32-bit libc used to exclude libc names from the undefined set (Linux build)")
set(P4_WIN32_PREFIX "/opt/p4win32" CACHE PATH
    "SDL3 and FFmpeg for i686-w64-mingw32 (ci/win32/build_deps.sh); Windows build only")

if(NOT EXISTS "${P4_DECOMP}/config/symbol_addrs.txt")
  message(FATAL_ERROR "P4_DECOMP is not a decomp checkout: ${P4_DECOMP}")
endif()
# The build reads the retail function-pointer words from config/retail_fnptr_words.txt; the executable itself is only a
# runtime input. Given here, the tables step checks it against that file.
set(P4_RETAIL_ARGS)
if(P4_RETAIL)
  if(NOT EXISTS "${P4_RETAIL}")
    message(FATAL_ERROR "P4_RETAIL is missing: '${P4_RETAIL}'")
  endif()
  set(P4_RETAIL_ARGS --retail "${P4_RETAIL}")
else()
  message(STATUS "P4_RETAIL unset: the tables use config/retail_fnptr_words.txt; tests that need the retail executable are not registered")
endif()

find_program(P4_PYTHON NAMES python3 python REQUIRED)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

