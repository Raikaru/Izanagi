if(P4_WIN32)
  # SDL3 and FFmpeg were built for the target by ci/win32/build_deps.sh (shared: DLLs next to the
  # exes, import libraries here).
  find_library(P4_SDL3_LIB NAMES SDL3 PATHS "${P4_WIN32_PREFIX}/lib" NO_DEFAULT_PATH REQUIRED)
  set(P4_LIBM_LIB "")
  find_library(P4_AVCODEC_LIB NAMES avcodec PATHS "${P4_WIN32_PREFIX}/lib" NO_DEFAULT_PATH REQUIRED)
  find_library(P4_AVUTIL_LIB NAMES avutil PATHS "${P4_WIN32_PREFIX}/lib" NO_DEFAULT_PATH REQUIRED)
  find_library(P4_SWSCALE_LIB NAMES swscale PATHS "${P4_WIN32_PREFIX}/lib" NO_DEFAULT_PATH REQUIRED)
  find_path(P4_FFMPEG_INC libavcodec/avcodec.h PATHS "${P4_WIN32_PREFIX}/include" NO_DEFAULT_PATH REQUIRED)
  include_directories(SYSTEM "${P4_WIN32_PREFIX}/include")
  # Names the host provides for gen_port's tables step: the UCRT and mingw-w64 runtime, SDL3 and
  # FFmpeg, from their import libraries. Win32 API imports (`_name@N`, `__imp__name`) are always host.
  execute_process(COMMAND "${CMAKE_C_COMPILER}" -print-file-name=libucrt.a OUTPUT_VARIABLE P4_UCRT_LIB OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND "${CMAKE_C_COMPILER}" -print-file-name=libmingwex.a OUTPUT_VARIABLE P4_MINGWEX_LIB OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND "${CMAKE_C_COMPILER}" -print-file-name=libmingw32.a OUTPUT_VARIABLE P4_MINGW32_LIB OUTPUT_STRIP_TRAILING_WHITESPACE)
  # Every library the exe links by name must be listed here, or its functions become trap stubs that
  # shadow them (gen_port.py refuses that for pthread_*, SDL_*, av*: it was winpthreads once).
  execute_process(COMMAND "${CMAKE_C_COMPILER}" -print-file-name=libwinpthread.a OUTPUT_VARIABLE P4_WINPTHREAD_LIB OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND "${CMAKE_C_COMPILER}" -print-libgcc-file-name OUTPUT_VARIABLE P4_BUILTINS_LIB OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(P4_HOST_LIB_ARGS --host-lib "${P4_UCRT_LIB}" --host-lib "${P4_MINGWEX_LIB}" --host-lib "${P4_MINGW32_LIB}"
    --host-lib "${P4_BUILTINS_LIB}" --host-lib "${P4_WINPTHREAD_LIB}" --host-lib "${P4_SDL3_LIB}" --host-lib "${P4_AVCODEC_LIB}"
    --host-lib "${P4_SWSCALE_LIB}" --host-lib "${P4_AVUTIL_LIB}")
  set(P4_GEN_TOOL_ARGS --coff --nm "${P4_NM}" --objdump "${P4_OBJDUMP}")
  # The loader exe links the ps2ram section; every exe that maps PS2 memory calls this.
  function(p4_ps2_exe target)
    target_sources(${target} PRIVATE "${P4_PS2RAM_SRC}")
    target_link_options(${target} PRIVATE ${P4_PS2RAM_LINK_FLAGS})
  endfunction()
  # DLLs the exes load, next to them: the build directory and tests/ (ctest runs tests from there).
  file(GLOB P4_WIN32_DLLS "${P4_WIN32_PREFIX}/bin/*.dll")
  foreach(dir "${CMAKE_BINARY_DIR}" "${CMAKE_BINARY_DIR}/tests")
    foreach(dll IN LISTS P4_WIN32_DLLS)
      get_filename_component(dllname "${dll}" NAME)
      list(APPEND P4_DLL_COPIES "${dir}/${dllname}")
      add_custom_command(OUTPUT "${dir}/${dllname}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${dir}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${dll}" "${dir}/${dllname}"
        DEPENDS "${dll}" VERBATIM)
    endforeach()
  endforeach()
  add_custom_target(p4_dlls ALL DEPENDS ${P4_DLL_COPIES})
elseif(P4_ANDROID)
  # SDL3 and FFmpeg for the ABI were built by ci/android/build_deps.sh into P4_ANDROID_PREFIX/<abi> (shared libraries; the Java side of SDL
  # is packaged by android/). Host names for the tables step: bionic's libc, libm, libdl and liblog stubs of the NDK sysroot, SDL3 and FFmpeg.
  set(P4_ANDROID_PREFIX "/android-cache/deps" CACHE PATH "build_deps.sh output (per-ABI subdirectories)")
  set(P4_ABI_PREFIX "${P4_ANDROID_PREFIX}/${ANDROID_ABI}")
  # the NDK toolchain re-roots find_* under its sysroot: NO_CMAKE_FIND_ROOT_PATH keeps these absolute
  find_library(P4_SDL3_LIB NAMES SDL3 PATHS "${P4_ABI_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
  find_library(P4_AVCODEC_LIB NAMES avcodec PATHS "${P4_ABI_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
  find_library(P4_AVUTIL_LIB NAMES avutil PATHS "${P4_ABI_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
  find_library(P4_SWSCALE_LIB NAMES swscale PATHS "${P4_ABI_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
  find_path(P4_FFMPEG_INC libavcodec/avcodec.h PATHS "${P4_ABI_PREFIX}/include" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
  include_directories(SYSTEM "${P4_ABI_PREFIX}/include")
  set(P4_HOST_LIB_ARGS)
  foreach(lib c m dl log)
    execute_process(COMMAND "${CMAKE_C_COMPILER}" --target=${CMAKE_C_COMPILER_TARGET} -print-file-name=lib${lib}.so
      OUTPUT_VARIABLE P4_NDK_${lib} OUTPUT_STRIP_TRAILING_WHITESPACE)
    list(APPEND P4_HOST_LIB_ARGS --host-lib "${P4_NDK_${lib}}")
  endforeach()
  list(APPEND P4_HOST_LIB_ARGS --host-lib "${P4_SDL3_LIB}" --host-lib "${P4_AVCODEC_LIB}" --host-lib "${P4_SWSCALE_LIB}" --host-lib "${P4_AVUTIL_LIB}"
    --host-name atexit --host-name at_quick_exit --host-name pthread_atfork) # bionic's libc_nonshared.a defines these, libc.so exports none
  # the compiler runtime's 64-bit helpers (__divdi3 ...), which the clang driver links statically into libmain
  execute_process(COMMAND "${CMAKE_C_COMPILER}" --target=${CMAKE_C_COMPILER_TARGET} -print-libgcc-file-name OUTPUT_VARIABLE P4_BUILTINS_LIB OUTPUT_STRIP_TRAILING_WHITESPACE)
  list(APPEND P4_HOST_LIB_ARGS --host-lib "${P4_BUILTINS_LIB}")
  # Native crash traces use the NDK unwinder, linked by the clang driver.
  execute_process(COMMAND "${CMAKE_C_COMPILER}" --target=${CMAKE_C_COMPILER_TARGET} -print-file-name=libunwind.a
    OUTPUT_VARIABLE P4_NDK_UNWIND OUTPUT_STRIP_TRAILING_WHITESPACE)
  list(APPEND P4_HOST_LIB_ARGS --host-lib "${P4_NDK_UNWIND}")
  get_filename_component(P4_NDK_BIN "${CMAKE_C_COMPILER}" DIRECTORY)
  set(P4_NM "${P4_NDK_BIN}/llvm-nm" CACHE FILEPATH "nm for the target objects")
  set(P4_OBJDUMP "${P4_NDK_BIN}/llvm-objdump" CACHE FILEPATH "objdump for the target objects")
  set(P4_GEN_TOOL_ARGS --pic --nm "${P4_NM}" --objdump "${P4_OBJDUMP}")
  function(p4_ps2_exe target)
  endfunction()
else()
  # Host SDL3 for the pad and window (src/platform/pad/pad_sdl.c). The build is 32-bit, so the
  # library comes from the i386 multiarch directory (NO_DEFAULT_PATH: the CI image also carries the
  # x86_64 libraries for the 64-bit build, which a default search would find first); the headers are
  # the shared /usr/include copy.
  find_library(P4_SDL3_LIB NAMES SDL3 PATHS /usr/lib/i386-linux-gnu /usr/lib32 NO_DEFAULT_PATH REQUIRED)
  # libm: linked as `m` below. Its exports are host names for gen_port tables (never stubbed).
  find_library(P4_LIBM_LIB NAMES libm.so.6 PATHS /usr/lib/i386-linux-gnu /usr/lib32 NO_DEFAULT_PATH REQUIRED)

  # libavcodec/libavutil/libswscale (FFmpeg, LGPL, linked dynamically; third_party/README.md) for the
  # movie picture decode (src/movie/mpeg2_dec.c). i386 multiarch, like SDL3.
  find_library(P4_AVCODEC_LIB NAMES avcodec PATHS /usr/lib/i386-linux-gnu /usr/lib32 NO_DEFAULT_PATH REQUIRED)
  find_library(P4_AVUTIL_LIB NAMES avutil PATHS /usr/lib/i386-linux-gnu /usr/lib32 NO_DEFAULT_PATH REQUIRED)
  find_library(P4_SWSCALE_LIB NAMES swscale PATHS /usr/lib/i386-linux-gnu /usr/lib32 NO_DEFAULT_PATH REQUIRED)
  find_path(P4_FFMPEG_INC libavcodec/avcodec.h PATHS /usr/include/i386-linux-gnu /usr/include REQUIRED)
  set(P4_HOST_LIB_ARGS --host-lib "${P4_LIBC32}" --host-lib "${P4_LIBM_LIB}" --host-lib "${P4_SDL3_LIB}"
    --host-lib "${P4_AVCODEC_LIB}" --host-lib "${P4_SWSCALE_LIB}" --host-lib "${P4_AVUTIL_LIB}")
  set(P4_GEN_TOOL_ARGS)
  function(p4_ps2_exe target)
  endfunction()
endif()
set(P4_FFMPEG_LIBS ${P4_AVCODEC_LIB} ${P4_SWSCALE_LIB} ${P4_AVUTIL_LIB})

