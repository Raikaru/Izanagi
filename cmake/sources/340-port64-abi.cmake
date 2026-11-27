# The ABI table the call sites of the mirror are adapted to: what every mirror unit and every port source defines, plus the signature
# rows of tools/mipsdiff/sigs.txt (tools/port64/unit64.py merge). A port native's register ABI is the one its definition gives; a Zig
# module's exports (the battle simulator, 187-battle-sim.cmake) have no C definition, so theirs is the prototype of each `export fn`
# in the modules' public headers (P4_BATTLE_SIM_ABI_HEADER), and an export without exactly one prototype fails the table.
if(P4_PORT64_OBJS)
  set(P4_PORT64_PORT_ABIS)
  set(zig_srcs)
  set(zig_cflags)
  foreach(tgt p4port p4portee)
    get_target_property(srcs ${tgt} SOURCES)
    get_target_property(incs ${tgt} INCLUDE_DIRECTORIES)
    get_target_property(defs ${tgt} COMPILE_DEFINITIONS)
    set(cflags)
    foreach(i IN LISTS incs)
      list(APPEND cflags "--cflag=-I${i}")
    endforeach()
    foreach(d IN LISTS defs)
      list(APPEND cflags "--cflag=-D${d}")
    endforeach()
    if(tgt STREQUAL "p4port")
      set(zig_cflags ${cflags} "--cflag=-I${CMAKE_SOURCE_DIR}/src/platform")
    endif()
    foreach(s IN LISTS srcs)
      if(s MATCHES "\\.c$" AND NOT s MATCHES "third_party/")
        if(IS_ABSOLUTE "${s}")
          set(abs "${s}")
        else()
          set(abs "${CMAKE_SOURCE_DIR}/${s}")
        endif()
        # Generated native sources belong to the build tree, not the source
        # tree. Keep their ABI outputs inside a dedicated subtree rather than
        # embedding `port/../...`, whose depfile needs a missing parent on a
        # cold parallel build.
        file(RELATIVE_PATH generated_rel "${P4_GEN}" "${abs}")
        if(NOT IS_ABSOLUTE "${generated_rel}" AND NOT generated_rel MATCHES "^\\.\\./")
          set(abi "${P4_G64}/abi/generated/${generated_rel}.abi.json")
        else()
          file(RELATIVE_PATH rel "${CMAKE_SOURCE_DIR}" "${abs}")
          set(abi "${P4_G64}/abi/port/${rel}.abi.json")
        endif()
        add_custom_command(OUTPUT "${abi}"
          COMMAND ${P4_PORT64_ENV} "${P4_PYTHON}" -W ignore "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" abi --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}"
                  --src "${abs}" --out "${abi}" --clang "${CMAKE_C_COMPILER}" ${cflags}
          DEPENDS "${abs}" ${P4_PORT64_TOOLS} DEPFILE "${abi}.d" JOB_POOL port64 VERBATIM)
        list(APPEND P4_PORT64_PORT_ABIS "${abi}")
      elseif(s MATCHES "\\.zig$")
        if(IS_ABSOLUTE "${s}")
          list(APPEND zig_srcs "${s}")
        else()
          list(APPEND zig_srcs "${CMAKE_SOURCE_DIR}/${s}")
        endif()
      endif()
    endforeach()
  endforeach()
  if(zig_srcs)
    set(abi "${P4_G64}/abi/port/zig-exports.abi.json")
    set(zig_args)
    foreach(z IN LISTS zig_srcs)
      list(APPEND zig_args --zig-source "${z}")
    endforeach()
    # the depfile holds the aggregate header's transitive includes (the preprocessing that reads the prototypes writes it)
    add_custom_command(OUTPUT "${abi}"
      COMMAND ${P4_PORT64_ENV} "${P4_PYTHON}" -W ignore "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" abi --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}"
              --zig-header "${P4_BATTLE_SIM_ABI_HEADER}" ${zig_args} --out "${abi}" --clang "${CMAKE_C_COMPILER}" ${zig_cflags}
      DEPENDS "${P4_BATTLE_SIM_ABI_HEADER}" ${zig_srcs} ${P4_PORT64_TOOLS} DEPFILE "${abi}.d" JOB_POOL port64 VERBATIM)
    list(APPEND P4_PORT64_PORT_ABIS "${abi}")
  endif()
  unset(zig_srcs)
  unset(zig_cflags)
  unset(zig_args)
  file(GENERATE OUTPUT "${P4_GEN}/crt-native-abi-files.txt"
    CONTENT "$<JOIN:${P4_PORT64_PORT_ABIS},\n>\n")
  foreach(profile mirror64 mirror64w)
    if(TARGET p4crt_${profile})
      add_custom_command(OUTPUT "${P4_CRT_${profile}_STAMP}" APPEND
        DEPENDS "${P4_GEN}/crt-native-abi-files.txt" ${P4_PORT64_PORT_ABIS})
    endif()
  endforeach()
  set(P4_MERGE_ABIS ${P4_PORT64_ABIS} ${P4_PORT64_PORT_ABIS})
  file(GENERATE OUTPUT "${P4_G64}/abi-files.txt"
    CONTENT "$<JOIN:${P4_MERGE_ABIS},\n>\n")
    add_custom_command(OUTPUT "${P4_G64}/abi.json"
      COMMAND ${P4_PORT64_ENV} "${P4_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/port64/unit64.py" merge --clang "${CMAKE_C_COMPILER}" --gen "${P4_GEN}" --root "${CMAKE_SOURCE_DIR}" --decomp "${P4_DECOMP}" --abi-files "${P4_G64}/abi-files.txt"
              --out "${P4_G64}/abi.json"
      DEPENDS ${P4_PORT64_ABIS} ${P4_PORT64_PORT_ABIS} ${P4_PORT64_TOOLS} ${P4_DECOMP_SYMBOL_FILES} "${CMAKE_SOURCE_DIR}/tools/mipsdiff/sigs.txt" "${CMAKE_SOURCE_DIR}/tools/gen_port.py"
              "${P4_G64}/abi-files.txt"
              "${CMAKE_SOURCE_DIR}/config/native_names.txt" "${P4_GEN}/units.txt" VERBATIM)
endif()

