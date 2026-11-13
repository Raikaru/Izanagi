# EE float rewrite switches (read by the 64-bit mirror below and by p4decomp, p4portee further down).
option(P4_EE_FLOAT "rewrite add/sub/div/sqrt in the decomp mirror to the EE's arithmetic" ON)
set(P4_EE_FLOAT_OPS "div,sqrt,add" CACHE STRING "which rewrites tools/ee_float_cc.py applies")

# The 64-bit model (docs/port64.md): every unit of the mirror is rewritten so that pointers in guest memory are 4-byte guest
# addresses (tools/port64/gptr_rewrite.py) and compiled for x86_64 beside the i386 build. M1: a compile-only check, as part
# of every build, that the rewrite covers the whole decomp (a pointer/integer conversion or a failed layout assertion in
# any unit is a build error); M2 links these objects into the 64-bit runtime. Linux x86_64 hosts with clang only; the
# unit steps are tools/port64/unit64.py (abi: what each unit defines; merge; build: preprocess, rewrite, compile).
option(P4_PORT64 "rewrite and compile the decomp mirror for x86_64 (the 64-bit model's compile-only check)" ON)
# -DP4_PORT64_OPT=O2 builds the mirror64 objects the way the shipping build does (optimised, through the EE float rewrite); the default
# is the cheap compile-only check. -DP4_PORT64_LINK=ON also builds the 64-bit executables (p4pc64) from them.
set(P4_PORT64_OPT "O0" CACHE STRING "optimisation level of the 64-bit mirror: O0 (compile-only check), O1, O2, O3")
option(P4_PORT64_LINK "link the 64-bit executable p4pc64 (needs x86_64 SDL3 and FFmpeg)" OFF)
set(P4_PORT64_OBJS)
# The architecture of the 64-bit build: x86_64, or aarch64, compiled with clang either across from x86_64 and run under qemu-user or on an AArch64 Linux
# host itself (GitHub's ARM runners, ci/Dockerfile.arm64) (docs/port64.md 7: the arm64 Debian multiarch libraries and gcc's files are the sysroot;
# -fsigned-char because char is unsigned on AArch64 and MIPS's is signed; the decomp is built for that).
set(P4_PORT64_ARCH "x86_64" CACHE STRING "architecture of the 64-bit build: x86_64 or aarch64 (cross under qemu-user, or native on AArch64)")
if(P4_ANDROID64)
  # Android: the NDK clang finds bionic's headers itself, so the target is all the units and the port sources need. The libraries (bionic's stubs,
  # SDL3 and FFmpeg) are the 32-bit build's (the Android block above). PIC, because libmain.so is a shared library; the i386 layout oracle of the
  # rewrite is compiled for i686-linux-android, whose headers the NDK has (it has no glibc ones).
  set(P4_X64_TRIPLE ${CMAKE_C_COMPILER_TARGET})
  set(P4_X64_TARGET_FLAGS --target=${P4_X64_TRIPLE} -fsigned-char -ffp-contract=off -fPIC)
  set(P4_X64_LINK_FLAGS)
  set(P4_X64_LIBDIR)
  set(P4_X64_LIBDIR2)
  set(P4_X64_EXTRA_FLAGS)
  set(P4_X64_GEN_TOOLS --nm "${P4_NM}" --objdump "${P4_OBJDUMP}")
  if(ANDROID_ABI STREQUAL "arm64-v8a")
    list(APPEND P4_X64_EXTRA_FLAGS -mno-outline-atomics)
    list(APPEND P4_X64_GEN_TOOLS --arm64)
  endif()
  string(JOIN " " P4_X64_TARGET_STRING --target=${P4_X64_TRIPLE} -fsigned-char -ffp-contract=off)
  # Match the whole triple: older CMake regex engines re-anchor ^ at each remaining suffix.
  string(REGEX REPLACE "^[a-z0-9_]+-(.+)$" "i686-\\1" P4_X32_TRIPLE "${P4_X64_TRIPLE}")
  set(P4_PORT64_ENV "${CMAKE_COMMAND}" -E env "P4_PORT64_TARGET=${P4_X64_TARGET_STRING}" P4_PORT64_PIC=1 "P4_PORT32_TARGET=--target=${P4_X32_TRIPLE}")
elseif(P4_APPLE)
  # macOS: the host's own architecture for the deployment target, against the SDK CMake found (the decomp's headers need nothing else). Mach-O:
  # C names carry a leading underscore (--macho), the code is position independent, `long double` is the x87 one on x86_64 and a plain double on
  # arm64 (include/p4pc/eefloat.h takes the integer path there), and a va_list is a pointer on arm64 as on i386. tools/port64/unit64.py derives
  # the rest from the Apple triple: the symbol prefix of its asm labels, and the i386 layout oracle, compiled for i686-apple-macos against the
  # same SDK (whose i386 headers are the x86_64 ones).
  if(P4_PORT64_ARCH STREQUAL "aarch64")
    set(P4_X64_TRIPLE arm64-apple-macos${CMAKE_OSX_DEPLOYMENT_TARGET})
  else()
    set(P4_X64_TRIPLE x86_64-apple-macos${CMAKE_OSX_DEPLOYMENT_TARGET})
  endif()
  # (-fno-blocks: clang turns Apple's block extension on by default, and the SDK then declares block types the rewriter has no rule for.
  # -fno-stack-protector: so is stack protection, which the Linux and Windows builds do not have; its guard is a host data reference from
  # every decomp function, outside the reviewed CRT boundary.)
  set(P4_X64_TARGET_FLAGS --target=${P4_X64_TRIPLE} -isysroot "${CMAKE_OSX_SYSROOT}" -fsigned-char -ffp-contract=off -D_FORTIFY_SOURCE=0
    -fno-blocks -fno-stack-protector)
  # The run paths: the app bundle's Frameworks (tools/package_macos.sh), then the build prefix, for the build tree's own runs.
  set(P4_X64_LINK_FLAGS --target=${P4_X64_TRIPLE} -isysroot "${CMAKE_OSX_SYSROOT}" "-Wl,-rpath,@executable_path/../Frameworks"
    "-Wl,-rpath,${P4_MACOS_PREFIX}/lib" -Wl,-headerpad_max_install_names)
  set(P4_X64_LIBDIR)
  set(P4_X64_LIBDIR2)
  set(P4_X64_EXTRA_FLAGS)
  set(P4_X64_GEN_TOOLS --macho --nm "${P4_CRT_NM}" --objdump "${P4_CRT_OBJDUMP}")
  if(P4_PORT64_ARCH STREQUAL "aarch64")
    list(APPEND P4_X64_GEN_TOOLS --arm64)
  endif()
  string(JOIN " " P4_X64_TARGET_STRING ${P4_X64_TARGET_FLAGS})
  set(P4_PORT64_ENV "${CMAKE_COMMAND}" -E env "P4_PORT64_TARGET=${P4_X64_TARGET_STRING}")
elseif(P4_PORT64_ARCH STREQUAL "aarch64")
  set(P4_X64_TRIPLE aarch64-linux-gnu)
  # gcc's files: Debian's cross install on an x86_64 build host, the machine's own on an AArch64 one.
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(P4_X64_GCC_DIR /usr/lib/gcc/aarch64-linux-gnu/14)
  else()
    set(P4_X64_GCC_DIR /usr/lib/gcc-cross/aarch64-linux-gnu/14)
    # Its tests run under qemu-user (binfmt): a CPU-time budget test reports itself skipped there (P4PC_PERF_EMULATED).
    set(P4_PORT64_EMULATED qemu-user)
  endif()
  set(P4_X64_SYSROOT_FLAGS --gcc-install-dir=${P4_X64_GCC_DIR} -isystem/usr/aarch64-linux-gnu/include -isystem/usr/include/aarch64-linux-gnu)
  set(P4_X64_TARGET_FLAGS --target=${P4_X64_TRIPLE} ${P4_X64_SYSROOT_FLAGS} -fsigned-char -ffp-contract=off)
  set(P4_X64_LINK_FLAGS --target=${P4_X64_TRIPLE} --gcc-install-dir=${P4_X64_GCC_DIR} -fuse-ld=lld -no-pie)
  set(P4_X64_LIBDIR /usr/lib/aarch64-linux-gnu)
  set(P4_X64_EXTRA_FLAGS -mno-outline-atomics)   # (the libgcc helper __aarch64_ldadd4_rel the outlined atomics call is not a host function the tables know)
  set(P4_X64_LIBDIR2 /lib/aarch64-linux-gnu)
  string(JOIN " " P4_X64_TARGET_STRING ${P4_X64_TARGET_FLAGS})
  set(P4_PORT64_ENV "${CMAKE_COMMAND}" -E env "P4_PORT64_TARGET=${P4_X64_TARGET_STRING}")
else()
  set(P4_X64_TRIPLE x86_64-linux-gnu)
  set(P4_X64_TARGET_FLAGS --target=x86_64-linux-gnu -m64)
  set(P4_X64_LINK_FLAGS --target=x86_64-linux-gnu -m64 -no-pie)
  set(P4_X64_LIBDIR /usr/lib/x86_64-linux-gnu)
  set(P4_X64_EXTRA_FLAGS)
  set(P4_X64_LIBDIR2 /lib/x86_64-linux-gnu)
  set(P4_PORT64_ENV)
endif()
if(P4_ANDROID64 OR P4_APPLE)
  # (P4_X64_GEN_TOOLS is set above)
elseif(P4_PORT64_ARCH STREQUAL "aarch64")
  set(P4_X64_GEN_TOOLS --arm64 --nm /usr/bin/llvm-nm --objdump /usr/bin/llvm-objdump)   # GNU objdump cannot read AArch64 relocations on an x86 host
else()
  set(P4_X64_GEN_TOOLS)
endif()

# The rewriter's rule counts per unit are a contract (tools/port64/unit64.py report --census-baseline; docs/port64.md): the build of the 64-bit mirror fails when a
# unit's counts differ from the baseline file (a rule that stops firing, fires elsewhere or a different number of times: the decomp, a pin or the rewriter changed
# under it), and `unit64.py report --census-refresh <file>` rewrites the baseline for a diff that has been reviewed. One baseline per target (x86_64 Linux:
# config/port64_census.txt, x86_64 Windows: config/port64w_census.txt, AArch64: config/port64a64_census.txt, Android: config/port64android_<arch>_census.txt,
# macOS: config/port64macos_<arch>_census.txt), and a compiler of a major the baseline has no section for fails it too (the layout oracle is the compiler's own
# record dump, so a few counts depend on the major: the file holds the full baseline of the CI compiler and patches for other majors, tools/port64/unit64.py).
# -DP4_CENSUS_CHECK=OFF (a warning at every configure) is for experiments.
option(P4_CENSUS_CHECK "fail the 64-bit mirror build when the rewriter's per-unit rule counts differ from the baseline" ON)
function(p4_census_check var unitsdir baseline units)
  set(${var} "" PARENT_SCOPE)
  # Keep the exact active set available for explicit experimental census refreshes too.
  string(REPLACE ";" "\n" unit_lines "${units}")
  file(CONFIGURE OUTPUT "${unitsdir}/../active_units.txt" CONTENT "${unit_lines}\n" @ONLY NEWLINE_STYLE UNIX)
  if(NOT P4_CENSUS_CHECK)
    message(WARNING "port64: P4_CENSUS_CHECK=OFF: the rewriter's rule counts are not checked against ${baseline}")
    return()
  endif()
  if(NOT baseline OR NOT EXISTS "${baseline}")
    message(FATAL_ERROR "port64: no reviewed census baseline '${baseline}'; rule counts must not go unchecked")
  endif()
  set(stamp "${unitsdir}/../census.ok")
  add_custom_command(OUTPUT "${stamp}"
    COMMAND "${P4_PYTHON}" -W ignore "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" report --dir "${unitsdir}" --census-baseline "${baseline}" --clang "${CMAKE_C_COMPILER}"
            --active-units "${unitsdir}/../active_units.txt" --rw-clean-units "${P4_GEN}/rw_clean_units.txt"
            --rw-clean-stripped "${P4_GEN}/decomp/src/rwclean/stripped_units.txt"
    COMMAND "${CMAKE_COMMAND}" -E touch "${stamp}"
    DEPENDS ${ARGN} "${baseline}" "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" "${unitsdir}/../active_units.txt" "${P4_GEN}/rw_clean_units.txt" VERBATIM)
  set(${var} "${stamp}" PARENT_SCOPE)
endfunction()
set(P4_CENSUS_BASELINE)
if(P4_ANDROID64)
  # The NDK ABI selects the Android target; the desktop architecture cache may still be x86_64.
  if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(P4_CENSUS_BASELINE "${CMAKE_SOURCE_DIR}/config/port64android_aarch64_census.txt")
  else()
    set(P4_CENSUS_BASELINE "${CMAKE_SOURCE_DIR}/config/port64android_${ANDROID_ABI}_census.txt")
  endif()
elseif(P4_APPLE)
  set(P4_CENSUS_BASELINE "${CMAKE_SOURCE_DIR}/config/port64macos_${P4_PORT64_ARCH}_census.txt")
elseif(P4_PORT64_ARCH STREQUAL "x86_64")
  set(P4_CENSUS_BASELINE "${CMAKE_SOURCE_DIR}/config/port64_census.txt")
elseif(P4_PORT64_ARCH STREQUAL "aarch64")
  set(P4_CENSUS_BASELINE "${CMAKE_SOURCE_DIR}/config/port64a64_census.txt")
endif()
# (CMake's if() combines AND and OR left to right: the AArch64-host conjunct needs its own parentheses.)
if(P4_PORT64 AND NOT P4_WIN32 AND ((CMAKE_SYSTEM_NAME STREQUAL "Linux" AND (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64"
    OR (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$" AND P4_PORT64_ARCH STREQUAL "aarch64"))) OR P4_ANDROID64 OR P4_APPLE))
  execute_process(COMMAND "${CMAKE_C_COMPILER}" ${P4_X64_TARGET_FLAGS} -fsyntax-only -x c "${CMAKE_SOURCE_DIR}/include/p4pc/guest.h"
    RESULT_VARIABLE P4_X64_PROBE OUTPUT_QUIET ERROR_QUIET)
  if(P4_X64_PROBE EQUAL 0)
    set(P4_G64 "${P4_GEN}/obj64")
    cmake_host_system_information(RESULT P4_NCORES QUERY NUMBER_OF_LOGICAL_CORES)
    # (parallel rewrites/compiles of the 64-bit mirror: half the cores by default, -DP4_PORT64_JOBS=N for a host that does nothing else)
    if(NOT P4_PORT64_JOBS)
      math(EXPR P4_PORT64_JOBS "${P4_NCORES} / 2 + 1")
    endif()
    set_property(GLOBAL APPEND PROPERTY JOB_POOLS port64=${P4_PORT64_JOBS})
    set(P4_PORT64_TOOLS "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" "${CMAKE_SOURCE_DIR}/tools/port64/gptr_rewrite.py")
    set(P4_PORT64_ABIS)
    set(P4_PORT64_UNITS)
    foreach(unit IN LISTS P4_UNIT_SRCS)
      string(REPLACE "${P4_GEN}/decomp/" "" rel "${unit}")
      set(abi "${P4_G64}/abi/${rel}.abi.json")
      add_custom_command(OUTPUT "${abi}"
        COMMAND ${P4_PORT64_ENV} "${P4_PYTHON}" -W ignore "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" abi --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}"
                --unit "${rel}" --out "${abi}" --clang "${CMAKE_C_COMPILER}"
        DEPENDS "${unit}" ${P4_PORT64_TOOLS} DEPFILE "${abi}.d" JOB_POOL port64 VERBATIM)
      list(APPEND P4_PORT64_ABIS "${abi}")
      list(APPEND P4_PORT64_UNITS "${rel}")
    endforeach()
    # O0 compile-only objects still obey the CRT boundary: otherwise LLVM can
    # lower a float sqrt intrinsic to host sqrtf, notably on Win64.
    set(P4_PORT64_LAUNCHER)
    if(P4_EE_FLOAT)
      set(P4_PORT64_LAUNCHER "--launcher=${P4_PYTHON}|${CMAKE_SOURCE_DIR}/tools/ee_float_cc.py|--modes=${P4_EE_FLOAT_OPS}")
    endif()
    foreach(rel IN LISTS P4_PORT64_UNITS)
      set(obj "${P4_G64}/units/${rel}.64.o")
      add_custom_command(OUTPUT "${obj}"
        COMMAND ${P4_PORT64_ENV} "${P4_PYTHON}" -W ignore "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" build --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}"
                --unit "${rel}" --abi "${P4_G64}/abi.json" --out "${obj}" --clang "${CMAKE_C_COMPILER}" --check --opt "${P4_PORT64_OPT}"
                ${P4_PORT64_LAUNCHER} ${P4_AUTO_INIT_ARGS}
        DEPENDS "${P4_GEN}/decomp/${rel}" "${P4_G64}/abi.json" ${P4_PORT64_TOOLS} DEPFILE "${obj}.d" JOB_POOL port64 VERBATIM)
      list(APPEND P4_PORT64_OBJS "${obj}")
    endforeach()
    set(P4_CENSUS_STAMP)
    p4_census_check(P4_CENSUS_STAMP "${P4_G64}/units" "${P4_CENSUS_BASELINE}" "${P4_PORT64_UNITS}" ${P4_PORT64_OBJS})
    # The objects and the census get a target of their own, with no dependencies: CMake's Ninja generator gives a custom command that several
    # targets list the intersection of their dependency closures as order-only inputs, and p4decomp64 depends on the CRT audit (300-crt.cmake),
    # which reads the mod and Luau libraries, so every unit used to wait for those libraries to compile.
    add_custom_target(p4decomp64_objects DEPENDS ${P4_PORT64_OBJS} ${P4_CENSUS_STAMP})
    add_custom_target(p4decomp64 ALL)
    add_dependencies(p4decomp64 p4decomp64_objects)
    # The mobile land guard runs the same rewrite/ABI/census as the real build,
    # but keeps generated C instead of compiling 1,000+ NDK objects or linking.
    set(P4_G64_GENERATION "${P4_GEN}/census64")
    set(P4_PORT64_GENERATED)
    foreach(rel IN LISTS P4_PORT64_UNITS)
      set(generated "${P4_G64_GENERATION}/units/${rel}.64.o")
      add_custom_command(OUTPUT "${generated}.i"
        BYPRODUCTS "${generated}.census" "${generated}.unsup"
        COMMAND ${P4_PORT64_ENV} "${P4_PYTHON}" -W ignore "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" build --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}"
                --unit "${rel}" --abi "${P4_G64}/abi.json" --out "${generated}" --clang "${CMAKE_C_COMPILER}" --check --generate-only
        DEPENDS "${P4_GEN}/decomp/${rel}" "${P4_G64}/abi.json" ${P4_PORT64_TOOLS} DEPFILE "${generated}.d" JOB_POOL port64 VERBATIM)
      list(APPEND P4_PORT64_GENERATED "${generated}.i")
    endforeach()
    p4_census_check(P4_GENERATION_CENSUS_STAMP "${P4_G64_GENERATION}/units" "${P4_CENSUS_BASELINE}" "${P4_PORT64_UNITS}" ${P4_PORT64_GENERATED})
    add_custom_target(p4decomp64_generate DEPENDS ${P4_PORT64_GENERATED} ${P4_GENERATION_CENSUS_STAMP})
    message(STATUS "port64: ${P4_PORT64_JOBS} parallel jobs; objects in ${P4_G64}/units")
    # M6: the same units for x86_64 Windows (LLP64: `long` is 4 bytes, size_t 8; the layout assertions still come from the i386 oracle).
    # Compile-only, from this Linux tree with the llvm-mingw clang; the pointer/integer classes of the ABI table do not depend on the target,
    # so one abi.json serves both. docs/port64.md, "Windows x64".
    option(P4_PORT64_WIN "also compile the decomp mirror for x86_64-w64-mingw32 (the Windows x64 compile-only check)" ON)
    # its own optimisation level: the check stays cheap when the Linux mirror is built at O2; the Windows executable wants O2 (-DP4_PORT64W_OPT=O2)
    set(P4_PORT64W_OPT "O0" CACHE STRING "optimisation level of the Windows x64 mirror: O0 (compile-only check), O1, O2, O3")
    set(P4_PORT64W_LAUNCHER)
    if(P4_EE_FLOAT)
      set(P4_PORT64W_LAUNCHER "--launcher=${P4_PYTHON}|${CMAKE_SOURCE_DIR}/tools/ee_float_cc.py|--modes=${P4_EE_FLOAT_OPS}")
    endif()
    if(P4_PORT64_WIN)
      execute_process(COMMAND "${CMAKE_C_COMPILER}" --target=x86_64-w64-mingw32 -fsyntax-only -x c "${CMAKE_SOURCE_DIR}/include/p4pc/guest.h"
        RESULT_VARIABLE P4_WIN64_PROBE OUTPUT_QUIET ERROR_QUIET)
      if(P4_WIN64_PROBE EQUAL 0)
        set(P4_G64W "${P4_GEN}/obj64w")
        set(P4_PORT64W_OBJS)
        foreach(rel IN LISTS P4_PORT64_UNITS)
          set(obj "${P4_G64W}/units/${rel}.64.o")
          add_custom_command(OUTPUT "${obj}"
            COMMAND "${CMAKE_COMMAND}" -E env "P4_PORT64_TARGET=--target=x86_64-w64-mingw32"
                    "${P4_PYTHON}" -W ignore "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" build --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}"
                    --unit "${rel}" --abi "${P4_G64}/abi.json" --out "${obj}" --clang "${CMAKE_C_COMPILER}" --check --opt "${P4_PORT64W_OPT}"
                    ${P4_PORT64W_LAUNCHER} ${P4_AUTO_INIT_ARGS}
            DEPENDS "${P4_GEN}/decomp/${rel}" "${P4_G64}/abi.json" ${P4_PORT64_TOOLS} DEPFILE "${obj}.d" JOB_POOL port64 VERBATIM)
          list(APPEND P4_PORT64W_OBJS "${obj}")
        endforeach()
        p4_census_check(P4_CENSUS_STAMPW "${P4_G64W}/units" "${CMAKE_SOURCE_DIR}/config/port64w_census.txt" "${P4_PORT64_UNITS}" ${P4_PORT64W_OBJS})
        # (objects and census without dependencies, as p4decomp64_objects above)
        add_custom_target(p4decomp64w_objects DEPENDS ${P4_PORT64W_OBJS} ${P4_CENSUS_STAMPW})
        add_custom_target(p4decomp64w ALL)
        add_dependencies(p4decomp64w p4decomp64w_objects)
      else()
        message(STATUS "port64: ${CMAKE_C_COMPILER} cannot target x86_64-w64-mingw32; the Windows x64 mirror check is skipped")
      endif()
    endif()
  else()
    message(STATUS "port64: this host cannot compile for x86_64 (clang --target=x86_64-linux-gnu failed); mirror64 skipped")
  endif()
endif()
