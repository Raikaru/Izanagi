# The Windows x64 executable (docs/port64.md 2.13, M6): the same rewritten mirror compiled for x86_64-w64-mingw32 (p4decomp64w), the port sources with
# os_win32.c for os_posix.c, and the tables of those objects (gen_port tables --ptr64 --coff64). Cross-built from this tree with the llvm-mingw clang;
# SDL3 and FFmpeg come from ci/win32/build_deps.sh with the x86_64 triple. -DP4_PORT64_LINK=ON and the prefix present.
if(P4_PORT64_LINK AND P4_PORT64W_OBJS AND EXISTS "${P4_WIN64_PREFIX}/lib")
  set(P4W_TRIPLE x86_64-w64-mingw32)
  find_library(P4W_SDL3 NAMES libSDL3.dll.a PATHS "${P4_WIN64_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
  find_library(P4W_AVCODEC NAMES libavcodec.dll.a PATHS "${P4_WIN64_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
  find_library(P4W_AVUTIL NAMES libavutil.dll.a PATHS "${P4_WIN64_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
  find_library(P4W_SWSCALE NAMES libswscale.dll.a PATHS "${P4_WIN64_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
  get_filename_component(P4W_BIN "${CMAKE_C_COMPILER}" DIRECTORY)
  find_program(P4W_NM llvm-nm HINTS "${P4W_BIN}" REQUIRED)
  find_program(P4W_OBJDUMP llvm-objdump HINTS "${P4W_BIN}" REQUIRED)
  foreach(lib ucrt mingwex mingw32 winpthread kernel32 user32 advapi32 shell32)
    execute_process(COMMAND "${CMAKE_C_COMPILER}" --target=${P4W_TRIPLE} -print-file-name=lib${lib}.a OUTPUT_VARIABLE P4W_LIB_${lib} OUTPUT_STRIP_TRAILING_WHITESPACE)
  endforeach()
  execute_process(COMMAND "${CMAKE_C_COMPILER}" --target=${P4W_TRIPLE} -print-libgcc-file-name OUTPUT_VARIABLE P4W_BUILTINS OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(P4W_CRT_HOST_ARGS --host-lib "${P4W_LIB_ucrt}" --host-lib "${P4W_LIB_mingwex}"
    --host-lib "${P4W_LIB_mingw32}" --host-lib "${P4W_BUILTINS}"
    --host-lib "${P4W_LIB_winpthread}" --host-lib "${P4W_LIB_kernel32}" --host-lib "${P4W_LIB_user32}"
    --host-lib "${P4W_LIB_advapi32}" --host-lib "${P4W_LIB_shell32}"
    --host-lib "${P4W_SDL3}" --host-lib "${P4W_AVCODEC}" --host-lib "${P4W_SWSCALE}" --host-lib "${P4W_AVUTIL}"
    --host-name __ImageBase)
  list(APPEND P4W_CRT_HOST_ARGS ${P4_MOD_HOST_ARGS64w})
  set(P4W_FLAGS --target=${P4W_TRIPLE} -m64 -DP4_GUEST_CHECK=1)
  set(P4W_LINK_FLAGS --target=${P4W_TRIPLE} -m64 ${P4_PE_REPRO_LINK_FLAGS})
  set(CMAKE_LINK_DEPENDS_USE_LINKER OFF)       # (lld in MinGW mode has no --dependency-file, which CMake passes to a Linux-hosted link)
  foreach(tgt p4port p4portee)
    get_target_property(srcs ${tgt} SOURCES)
    list(TRANSFORM srcs REPLACE "os/os_posix\\.c$" "os/os_win32.c")
    if(P4_PORT64_EXCLUDE)
      list(FILTER srcs EXCLUDE REGEX "${P4_PORT64_EXCLUDE}")
    endif()
    get_target_property(incs ${tgt} INCLUDE_DIRECTORIES)
    list(FILTER incs EXCLUDE REGEX "^/usr/")
    get_target_property(defs ${tgt} COMPILE_DEFINITIONS)
    get_target_property(launcher ${tgt} C_COMPILER_LAUNCHER)
    add_library(${tgt}64w OBJECT ${srcs})
    target_compile_options(${tgt}64w PRIVATE ${P4W_FLAGS})
    target_include_directories(${tgt}64w SYSTEM PRIVATE "${P4_WIN64_PREFIX}/include")
    if(incs)
      target_include_directories(${tgt}64w PRIVATE ${incs})
    endif()
    if(defs)
      target_compile_definitions(${tgt}64w PRIVATE ${defs})
    endif()
    if(launcher)
      set_target_properties(${tgt}64w PROPERTIES C_COMPILER_LAUNCHER "${launcher}")
    endif()
  endforeach()
  # (no wait for the mirror: as p4port64 in 350-port64-player.cmake)
  p4_crt_guard(native64w
    "${P4_PORT64W_OBJS};$<FILTER:$<TARGET_OBJECTS:p4port64w>,EXCLUDE,/(src/platform|third_party)/>;$<FILTER:$<TARGET_OBJECTS:p4portee64w>,EXCLUDE,/(src/platform|third_party)/>"
    "$<TARGET_OBJECTS:p4port64w>;$<TARGET_OBJECTS:p4portee64w>" ${P4W_CRT_HOST_ARGS} ${P4_MOD_CRT_PROJECT64w})
  if(P4_MOD_CRT_DEP64w)
    add_custom_command(OUTPUT "${P4_CRT_native64w_STAMP}" APPEND DEPENDS ${P4_MOD_CRT_DEP64w})
  endif()
  set(P4W_OBJS ${P4_PORT64W_OBJS} $<TARGET_OBJECTS:p4port64w> $<TARGET_OBJECTS:p4portee64w>)
  string(REPLACE ";" "\n" P4W_OBJ_LINES "${P4_PORT64W_OBJS}")
  file(GENERATE OUTPUT "${P4_GEN}/objects64w.txt"
    CONTENT "${P4W_OBJ_LINES}\n$<JOIN:$<TARGET_OBJECTS:p4port64w>;$<TARGET_OBJECTS:p4portee64w>,\n>\n")
  set(P4W_TABLES "${P4_GEN}/p4pc_tables64w.c")
  add_custom_command(
    OUTPUT "${P4W_TABLES}"
    COMMAND "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/gen_port.py" tables --ptr64 --coff64
            --decomp "${P4_DECOMP}" --build "${P4_GEN}"
            --objects-file "${P4_GEN}/objects64w.txt"
            ${P4_RETAIL_ARGS} --out "${P4W_TABLES}" --abi "${P4_G64}/abi.json"
            --nm "${P4W_NM}" --objdump "${P4W_OBJDUMP}"
            --host-lib "${P4W_LIB_ucrt}" --host-lib "${P4W_LIB_mingwex}" --host-lib "${P4W_LIB_mingw32}" --host-lib "${P4W_BUILTINS}"
            --host-lib "${P4W_LIB_winpthread}" --host-lib "${P4W_LIB_kernel32}" --host-lib "${P4W_LIB_user32}" --host-lib "${P4W_LIB_advapi32}"
            --host-lib "${P4W_LIB_shell32}" --host-lib "${P4W_SDL3}" --host-lib "${P4W_AVCODEC}" --host-lib "${P4W_SWSCALE}" --host-lib "${P4W_AVUTIL}"
            --host-name __ImageBase
            ${P4_MOD_HOST_ARGS64w} ${P4_MOD_CRT_PROJECT64w}
    DEPENDS p4decomp64w p4port64w p4portee64w ${P4W_OBJS} "${P4_G64}/abi.json"
      "${P4_CRT_native64w_STAMP}"
      "${P4_GEN}/units.txt" "${P4_GEN}/objects64w.txt" "${CMAKE_SOURCE_DIR}/tools/gen_port.py"
      "${CMAKE_SOURCE_DIR}/config/inactive_movie_decoder.json"
      "${CMAKE_SOURCE_DIR}/config/port_symbols.txt" "${CMAKE_SOURCE_DIR}/config/port_symbols_manual.txt"
      "${CMAKE_SOURCE_DIR}/config/port_owned.txt" "${CMAKE_SOURCE_DIR}/config/native_names.txt"
      "${CMAKE_SOURCE_DIR}/config/unbound_natives.txt" "${CMAKE_SOURCE_DIR}/include/p4pc/libc_rename.h"
      "${CMAKE_SOURCE_DIR}/config/call_traps.txt" "${CMAKE_SOURCE_DIR}/config/retail_fnptr_words.txt"
    VERBATIM
    COMMENT "Generating the Windows x64 port tables")
  set(P4W_OUT "${CMAKE_BINARY_DIR}/win64")
  add_executable(p4pc64w src/loader.c "${P4W_TABLES}" ${P4W_OBJS})
  target_compile_options(p4pc64w PRIVATE ${P4W_FLAGS} -fno-builtin)
  target_compile_definitions(p4pc64w PRIVATE ${P4_PORT_DEFINES})
  target_link_options(p4pc64w PRIVATE ${P4W_LINK_FLAGS} -Wl,--stack=8388608 -Wl,--subsystem,windows -Wl,-e,mainCRTStartup)
  target_link_libraries(p4pc64w ${P4W_SDL3} ${P4W_AVCODEC} ${P4W_SWSCALE} ${P4W_AVUTIL} -Wl,-Bstatic winpthread -Wl,-Bdynamic)
  # (lld in MinGW mode has no --dependency-file, which CMake would pass for a Linux-hosted link)
  set_target_properties(p4pc64w PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${P4W_OUT}" SUFFIX ".exe")
  add_dependencies(p4pc64w p4decomp64w)
  # the offline renderer on the same objects: the scene gates can run it on a Windows machine
  add_executable(p4pc_viewer64w src/viewer/p4pc_viewer.c "${P4W_TABLES}" ${P4W_OBJS})
  target_compile_options(p4pc_viewer64w PRIVATE ${P4W_FLAGS} -fno-builtin)
  target_include_directories(p4pc_viewer64w PRIVATE src/platform/files)
  target_compile_definitions(p4pc_viewer64w PRIVATE ${P4_PORT_DEFINES})
  target_link_options(p4pc_viewer64w PRIVATE ${P4W_LINK_FLAGS} -Wl,--stack=8388608 -Wl,--subsystem,windows -Wl,-e,mainCRTStartup)
  target_link_libraries(p4pc_viewer64w ${P4W_SDL3} ${P4W_AVCODEC} ${P4W_SWSCALE} ${P4W_AVUTIL} -Wl,-Bstatic winpthread -Wl,-Bdynamic)
  set_target_properties(p4pc_viewer64w PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${P4W_OUT}" SUFFIX ".exe")
  add_dependencies(p4pc_viewer64w p4decomp64w)
  # Package checks run this against the shipped x64 DLLs on Windows, not the host's FFmpeg.
  add_executable(ffmpeg_license_test64w tests/ffmpeg_license_test.c)
  target_compile_options(ffmpeg_license_test64w PRIVATE ${P4W_FLAGS})
  target_include_directories(ffmpeg_license_test64w SYSTEM PRIVATE "${P4_WIN64_PREFIX}/include")
  target_link_options(ffmpeg_license_test64w PRIVATE ${P4W_LINK_FLAGS})
  target_link_libraries(ffmpeg_license_test64w ${P4W_AVCODEC} ${P4W_AVUTIL} ${P4W_SWSCALE})
  set_target_properties(ffmpeg_license_test64w PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${P4W_OUT}" SUFFIX ".exe")
  add_dependencies(p4pc64w ffmpeg_license_test64w)
  # the DLLs the exe loads, next to it
  file(GLOB P4W_DLLS "${P4_WIN64_PREFIX}/bin/*.dll")
  set(P4W_DLL_COPIES)
  foreach(dll IN LISTS P4W_DLLS)
    get_filename_component(dllname "${dll}" NAME)
    list(APPEND P4W_DLL_COPIES "${P4W_OUT}/${dllname}")
    add_custom_command(OUTPUT "${P4W_OUT}/${dllname}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${P4W_OUT}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${dll}" "${P4W_OUT}/${dllname}"
      DEPENDS "${dll}" VERBATIM)
  endforeach()
  add_custom_target(p4pc64w_dlls ALL DEPENDS ${P4W_DLL_COPIES})
  add_dependencies(p4pc64w p4pc64w_dlls)
  add_dependencies(p4pc_viewer64w p4pc64w_dlls)
endif()

