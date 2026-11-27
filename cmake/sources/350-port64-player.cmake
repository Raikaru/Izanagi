# The 64-bit executable (docs/port64.md, M2): the rewritten mirror (p4decomp64), the port sources compiled for x86_64 (the same lists as
# p4port and p4portee) and the tables of the 64-bit objects. -DP4_PORT64_LINK=ON.
if(P4_ANDROID64 AND NOT (P4_PORT64_LINK AND P4_PORT64_OBJS))
  message(FATAL_ERROR "the 64-bit Android build needs -DP4_PORT64_LINK=ON and a clang that can target ${CMAKE_C_COMPILER_TARGET}")
endif()
if(P4_PORT64_LINK AND P4_PORT64_OBJS)
  if(P4_ANDROID64)
    # the libraries and the host names of the Android block above (they are per ABI, not per pointer model)
    set(P4_SDL3_LIB64 ${P4_SDL3_LIB})
    set(P4_FFMPEG_LIBS64 ${P4_FFMPEG_LIBS})
    set(P4_HOSTLIB_ARGS64 --pic ${P4_HOST_LIB_ARGS})
  elseif(P4_APPLE)
    # macOS builds only this model: the libraries and host names of the Apple block above (ci/macos/build_deps.sh)
    set(P4_SDL3_LIB64 ${P4_SDL3_LIB})
    set(P4_AVCODEC_LIB64 ${P4_AVCODEC_LIB})
    set(P4_AVUTIL_LIB64 ${P4_AVUTIL_LIB})
    set(P4_SWSCALE_LIB64 ${P4_SWSCALE_LIB})
    set(P4_FFMPEG_LIBS64 ${P4_FFMPEG_LIBS})
    set(P4_HOSTLIB_ARGS64 ${P4_HOST_LIB_ARGS})
  else()
    # -DP4_LINUX64_PREFIX=<dir>: SDL3 and the LGPL-only FFmpeg of ci/win32/build_deps.sh <dir> /usr x86_64-linux-gnu, which tools/package_linux.sh ships
    # (the distribution's FFmpeg is built with --enable-gpl); the system libraries otherwise
    set(P4_LINUX64_PREFIX "" CACHE PATH "x86_64 Linux SDL3 and FFmpeg prefix of the player package (empty: the system's)")
    set(P4_X64_AVLIBDIR ${P4_X64_LIBDIR})
    if(P4_LINUX64_PREFIX)
      set(P4_X64_AVLIBDIR "${P4_LINUX64_PREFIX}/lib")
    endif()
    find_library(P4_SDL3_LIB64 NAMES SDL3 PATHS ${P4_X64_AVLIBDIR} NO_DEFAULT_PATH REQUIRED)
    find_library(P4_LIBM_LIB64 NAMES libm.so.6 PATHS ${P4_X64_LIBDIR} ${P4_X64_LIBDIR2} NO_DEFAULT_PATH REQUIRED)
    find_library(P4_LIBC_LIB64 NAMES libc.so.6 PATHS ${P4_X64_LIBDIR} ${P4_X64_LIBDIR2} NO_DEFAULT_PATH REQUIRED)
    find_library(P4_AVCODEC_LIB64 NAMES avcodec PATHS ${P4_X64_AVLIBDIR} NO_DEFAULT_PATH REQUIRED)
    find_library(P4_AVUTIL_LIB64 NAMES avutil PATHS ${P4_X64_AVLIBDIR} NO_DEFAULT_PATH REQUIRED)
    find_library(P4_SWSCALE_LIB64 NAMES swscale PATHS ${P4_X64_AVLIBDIR} NO_DEFAULT_PATH REQUIRED)
    set(P4_FFMPEG_LIBS64 ${P4_AVCODEC_LIB64} ${P4_SWSCALE_LIB64} ${P4_AVUTIL_LIB64})
    set(P4_HOSTLIB_ARGS64 --host-lib "${P4_LIBC_LIB64}" --host-lib "${P4_LIBM_LIB64}" --host-lib "${P4_SDL3_LIB64}"
      --host-lib "${P4_AVCODEC_LIB64}" --host-lib "${P4_SWSCALE_LIB64}" --host-lib "${P4_AVUTIL_LIB64}")
    # The integer EE float helpers divide 128-bit values on AArch64 (include/p4pc/eefloat.h): clang calls the compiler runtime's __udivti3, which is a host
    # function and not an unresolved retail one (a trap stub of that name shadowed it and the divisions returned garbage).
    if(P4_PORT64_ARCH STREQUAL "aarch64")
      execute_process(COMMAND "${CMAKE_C_COMPILER}" ${P4_X64_TARGET_FLAGS} -print-libgcc-file-name OUTPUT_VARIABLE P4_BUILTINS_LIB64 OUTPUT_STRIP_TRAILING_WHITESPACE)
      list(APPEND P4_HOSTLIB_ARGS64 --host-lib "${P4_BUILTINS_LIB64}")
    endif()
  endif()
  list(APPEND P4_HOSTLIB_ARGS64 ${P4_MOD_HOST_ARGS64})
  set(P4_X64_FLAGS ${P4_X64_TARGET_FLAGS} ${P4_X64_EXTRA_FLAGS} -DP4_GUEST_CHECK=1)
  foreach(tgt p4port p4portee)
    get_target_property(srcs ${tgt} SOURCES)
    if(P4_PORT64_EXCLUDE)
      list(FILTER srcs EXCLUDE REGEX "${P4_PORT64_EXCLUDE}")
    endif()
    get_target_property(incs ${tgt} INCLUDE_DIRECTORIES)
    get_target_property(defs ${tgt} COMPILE_DEFINITIONS)
    get_target_property(launcher ${tgt} C_COMPILER_LAUNCHER)
    add_library(${tgt}64 OBJECT ${srcs})
    target_compile_options(${tgt}64 PRIVATE ${P4_X64_FLAGS})
    if(incs)
      target_include_directories(${tgt}64 PRIVATE ${incs})
    endif()
    if(defs)
      target_compile_definitions(${tgt}64 PRIVATE ${defs})
    endif()
    if(launcher)
      set_target_properties(${tgt}64 PROPERTIES C_COMPILER_LAUNCHER "${launcher}")
    endif()
  endforeach()
  # The port sources need no mirror output (their i386 objects compile without it), so p4port64 and p4portee64 compile beside the mirror64,
  # and this audit runs beside the mirror64's own (crt-mirror64): the tables and the executables wait for both.
  p4_crt_guard(native64
    "${P4_PORT64_OBJS};$<FILTER:$<TARGET_OBJECTS:p4port64>,EXCLUDE,/(src/platform|third_party)/>;$<FILTER:$<TARGET_OBJECTS:p4portee64>,EXCLUDE,/(src/platform|third_party)/>"
    "$<TARGET_OBJECTS:p4port64>;$<TARGET_OBJECTS:p4portee64>" ${P4_HOSTLIB_ARGS64} ${P4_MOD_CRT_PROJECT64})
  if(P4_MOD_CRT_DEP64)
    add_custom_command(OUTPUT "${P4_CRT_native64_STAMP}" APPEND DEPENDS ${P4_MOD_CRT_DEP64})
  endif()
  set(P4_OBJS64 ${P4_PORT64_OBJS} $<TARGET_OBJECTS:p4port64> $<TARGET_OBJECTS:p4portee64>)
  string(REPLACE ";" "\n" P4_PORT64_OBJ_LINES "${P4_PORT64_OBJS}")
  file(GENERATE OUTPUT "${P4_GEN}/objects64.txt"
    CONTENT "${P4_PORT64_OBJ_LINES}\n$<JOIN:$<TARGET_OBJECTS:p4port64>;$<TARGET_OBJECTS:p4portee64>,\n>\n")
  set(P4_TABLES64 "${P4_GEN}/p4pc_tables64.c")
  add_custom_command(
    OUTPUT "${P4_TABLES64}"
    COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/gen_port.py" tables --ptr64 ${P4_X64_GEN_TOOLS}
            --decomp "${P4_DECOMP}" --build "${P4_GEN}"
            --objects-file "${P4_GEN}/objects64.txt"
            ${P4_RETAIL_ARGS} --out "${P4_TABLES64}" --abi "${P4_G64}/abi.json"
            ${P4_HOSTLIB_ARGS64} ${P4_MOD_CRT_PROJECT64}
    DEPENDS p4decomp64 p4port64 p4portee64 ${P4_OBJS64} "${P4_G64}/abi.json"
      "${P4_CRT_native64_STAMP}"
      "${P4_GEN}/units.txt" "${P4_GEN}/objects64.txt" "${CMAKE_SOURCE_DIR}/tools/gen_port.py"
      "${CMAKE_SOURCE_DIR}/config/inactive_movie_decoder.json"
      "${CMAKE_SOURCE_DIR}/config/port_symbols.txt" "${CMAKE_SOURCE_DIR}/config/port_symbols_manual.txt"
      "${CMAKE_SOURCE_DIR}/config/port_owned.txt" "${CMAKE_SOURCE_DIR}/config/native_names.txt"
      "${CMAKE_SOURCE_DIR}/config/unbound_natives.txt" "${CMAKE_SOURCE_DIR}/include/p4pc/libc_rename.h"
      "${CMAKE_SOURCE_DIR}/config/call_traps.txt" "${CMAKE_SOURCE_DIR}/config/retail_fnptr_words.txt"
    VERBATIM
    COMMENT "Generating the 64-bit port tables")
  if(P4_ANDROID64)
    # The game as SDL's `main` library, as the 32-bit block above, from the 64-bit objects. libmain.so is the whole product: no p4pc64 executable.
    set_source_files_properties(src/viewer/p4pc_viewer.c PROPERTIES COMPILE_DEFINITIONS "main=p4pc_viewer_main")
    add_library(main SHARED src/loader.c src/viewer/p4pc_viewer.c src/platform/android/android_main.c "${P4_TABLES64}" ${P4_OBJS64})
    target_link_libraries(main m dl log android ${P4_SDL3_LIB64} ${P4_FFMPEG_LIBS64})
    target_compile_options(main PRIVATE ${P4_X64_FLAGS} -fno-builtin)
    target_include_directories(main PRIVATE src/platform/files)
    target_compile_definitions(main PRIVATE ${P4_PORT_DEFINES})
    target_link_options(main PRIVATE -Wl,-z,max-page-size=16384 -Wl,--build-id=none -Wl,--wrap=exit)
    add_dependencies(main p4decomp64)
    option(P4_ANDROID_PHONE_RUNNER "Build the separate headless adb-shell game runner" OFF)
    if(P4_ANDROID_PHONE_RUNNER)
      add_executable(p4pc_phone src/loader.c src/platform/android/phone_main.c "${P4_TABLES64}" ${P4_OBJS64})
      target_link_libraries(p4pc_phone m dl log android ${P4_SDL3_LIB64} ${P4_FFMPEG_LIBS64})
      target_compile_options(p4pc_phone PRIVATE ${P4_X64_FLAGS} -fno-builtin)
      target_include_directories(p4pc_phone PRIVATE src/platform/files)
      target_compile_definitions(p4pc_phone PRIVATE ${P4_PORT_DEFINES})
      target_link_options(p4pc_phone PRIVATE -Wl,-z,max-page-size=16384 -Wl,--build-id=none)
      add_dependencies(p4pc_phone p4decomp64)
      p4_crt_entry(p4pc_phone "${P4_GEN}/objects64.txt" ${P4_HOSTLIB_ARGS64})
    endif()
    p4_crt_entry(main "${P4_GEN}/objects64.txt" ${P4_HOSTLIB_ARGS64})
    # The arm64 sentinel tests (docs/android_arm64_tests.md): registers the test tree, whose NDK-built executables tools/phone_units.py runs on the phone.
    option(P4_ANDROID_PHONE_TESTS "Register the tests of the arm64 phone sentinel set (tools/phone_units.py)" OFF)
    if(P4_ANDROID_PHONE_TESTS)
      set(P4_ANDROID_ARM64 TRUE)   # (P4_PORT64_ARCH stays the desktop cache's x86_64 here: the test fragments take the arm64 decisions from this)
      # (380-movie-player.cmake's object library, which the movie tests link; its viewer executable needs the desktop FFmpeg names)
      add_library(p4movie OBJECT src/movie/sfd_demux.c src/movie/mpeg2_dec.c src/movie/adx_dec.c)
      target_include_directories(p4movie PRIVATE src/platform/files "${P4_FFMPEG_INC}")
      target_compile_definitions(p4movie PRIVATE ${P4_PORT_DEFINES})
      # The game gates name the desktop executables in their commands; on the phone they are never built or run (EXCLUDE_FROM_ALL), the names only have to resolve.
      # The test executables link what the game links: bionic's liblog (the crash handler writes to logcat) and the FFmpeg libraries under the 64-bit names.
      add_library(p4_android_phone_stubs OBJECT tests/android_phone_stubs.c)   # (objects, not an archive: the constructor in it has to be linked in)
      link_libraries(p4_android_phone_stubs log)
      set(P4_AVCODEC_LIB64 ${P4_AVCODEC_LIB})
      set(P4_AVUTIL_LIB64 ${P4_AVUTIL_LIB})
      set(P4_SWSCALE_LIB64 ${P4_SWSCALE_LIB})
      foreach(_p4_stub p4pc p4pc_viewer p4pc64 p4pc_viewer64)
        add_executable(${_p4_stub} EXCLUDE_FROM_ALL src/platform/android/phone_main.c)
      endforeach()
      include("${CMAKE_CURRENT_LIST_DIR}/400-tests.cmake")
      # These Luau tests define p4_luau_user themselves, as the ones the GNU linker takes from their object first; lld pulls luau_c.cpp.o in for the
      # rest of its symbols and then reports the second definition. The test's own (first on the command line) is the one meant.
      foreach(_p4_t lua_semantic_subscription_test64 mod_luau_test64 mod_luau_save_test64 mod_luau_status_test64 mod_luau_execution_test64)
        if(TARGET ${_p4_t})
          target_link_options(${_p4_t} PRIVATE -Wl,-z,muldefs)
          target_link_libraries(${_p4_t} PRIVATE log)
        endif()
      endforeach()
    endif()
    set(_p4_source_stop TRUE)
    return()
  endif()
  add_executable(p4pc64 src/loader.c "${P4_TABLES64}" ${P4_OBJS64})
  target_compile_options(p4pc64 PRIVATE ${P4_X64_FLAGS} -fno-builtin)
  target_compile_definitions(p4pc64 PRIVATE ${P4_PORT_DEFINES})
  target_link_options(p4pc64 PRIVATE ${P4_X64_LINK_FLAGS})
  target_link_libraries(p4pc64 m ${P4_SDL3_LIB64} ${P4_FFMPEG_LIBS64})
  add_dependencies(p4pc64 p4decomp64)
  # the offline renderer on the same objects (the scene gates, once they run on the 64-bit build)
  add_executable(p4pc_viewer64 src/viewer/p4pc_viewer.c "${P4_TABLES64}" ${P4_OBJS64})
  target_compile_options(p4pc_viewer64 PRIVATE ${P4_X64_FLAGS} -fno-builtin)
  target_include_directories(p4pc_viewer64 PRIVATE src/platform/files)
  target_compile_definitions(p4pc_viewer64 PRIVATE ${P4_PORT_DEFINES})
  target_link_options(p4pc_viewer64 PRIVATE ${P4_X64_LINK_FLAGS})
  target_link_libraries(p4pc_viewer64 m ${P4_SDL3_LIB64} ${P4_FFMPEG_LIBS64})
  add_dependencies(p4pc_viewer64 p4decomp64)
  if(TARGET p4_macos_runtime)
    # ANGLE beside the build tree's executables (060-platform-libraries.cmake): the 64-bit aggregate p4_x64 builds this one
    add_dependencies(p4pc64 p4_macos_runtime)
  endif()
endif()

