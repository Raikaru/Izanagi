"""Build a real native C/C++ fixture for deferred per-profile object consumers."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cmake", default="cmake")
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--coff", action="store_true",
                        help="Also compile/link a real native-consumer COFF probe; requires llvm-mingw")
    args = parser.parse_args()
    helper = Path(__file__).resolve().parents[1] / "cmake/host-cxx.cmake"
    with tempfile.TemporaryDirectory(prefix="p4-host-adapter-graph-") as temporary:
        source = Path(temporary) / "source"
        build = Path(temporary) / "build"
        (source / "fixtures").mkdir(parents=True)
        (source / "main.c").write_text(
            "int port_value(void);\nint main(void) { return port_value() == EXPECTED ? 0 : 1; }\n",
            encoding="utf-8")
        (source / "port.c").write_text(
            "int adapter_value(void);\nint port_value(void) { return adapter_value(); }\n",
            encoding="utf-8")
        (source / "adapter.cpp").write_text(
            '#include <string>\nextern "C" int adapter_value(void) { return std::stoi(std::to_string(EXPECTED)); }\n',
            encoding="utf-8")
        (source / "adapter.c").write_text("int adapter_value(void) { return EXPECTED; }\n", encoding="utf-8")
        (source / "unrelated.c").write_text("int main(void) { return 0; }\n", encoding="utf-8")
        (source / "coff_main.c").write_text(
            "int adapter_value(void);\nint main(void) { return adapter_value() == 55 ? 0 : 1; }\n",
            encoding="utf-8")
        (source / "CMakeLists.txt").write_text(r'''
cmake_minimum_required(VERSION 3.20)
project(host_adapter_graph LANGUAGES C)
include("${P4_HELPER}")
p4_host_cxx_enable()
function(profile suffix value language)
  if(language STREQUAL "CXX")
    add_library(adapter${suffix} STATIC adapter.cpp)
    p4_host_cxx_target(adapter${suffix})
  else()
    add_library(adapter${suffix} STATIC adapter.c)
  endif()
  target_compile_definitions(adapter${suffix} PRIVATE EXPECTED=${value})
  p4_host_adapter_register("${suffix}" "${language}" adapter${suffix})
  add_library(p4port${suffix} OBJECT port.c)
  set_property(TARGET p4port${suffix} PROPERTY POSITION_INDEPENDENT_CODE ON)
endfunction()
profile("" 11 CXX)
profile(64 22 CXX)
profile(64w 33 CXX)
profile(_c 44 C)
# Duplicate registration must not duplicate the final link closure.
p4_host_adapter_register("" CXX adapter)
add_library(existing_dep INTERFACE)
add_executable(player main.c $<TARGET_OBJECTS:p4port>)
target_link_libraries(player existing_dep)
target_compile_definitions(player PRIVATE EXPECTED=11)
set_property(TARGET player PROPERTY LINKER_LANGUAGE C)
add_executable(c_only main.c $<TARGET_OBJECTS:p4port_c>)
target_compile_definitions(c_only PRIVATE EXPECTED=44)
set_property(TARGET c_only PROPERTY LINKER_LANGUAGE C)
p4_host_adapter_attach(c_only p4port_c)
add_executable(p4pc_named_but_unrelated unrelated.c)
set_property(TARGET p4pc_named_but_unrelated PROPERTY LINKER_LANGUAGE C)
add_subdirectory(fixtures)
if(P4_TEST_COFF)
  set(P4_PORT64_LINK ON)
  set(P4_PORT64W_OBJS "available_coff_mirror")
  set(P4_WIN64_PREFIX "${CMAKE_BINARY_DIR}/native-win64")
  file(MAKE_DIRECTORY "${P4_WIN64_PREFIX}/lib")
  function(linker_profile suffix)
    if(suffix STREQUAL "64w")
      add_library(coff_adapter STATIC adapter.cpp)
      p4_host_cxx_target(coff_adapter ${ARGN})
      target_compile_definitions(coff_adapter PRIVATE EXPECTED=55)
      p4_host_cxx_runtime(coff_adapter "${suffix}" ${ARGN})
      add_executable(coff_link_probe coff_main.c)
      target_compile_options(coff_link_probe PRIVATE ${ARGN})
      target_link_libraries(coff_link_probe PRIVATE coff_adapter)
      set_target_properties(coff_link_probe PROPERTIES LINKER_LANGUAGE CXX SUFFIX ".exe")
    endif()
  endfunction()
  p4_host_cxx_profiles(linker_profile)
endif()
# A later native target must not inherit the COFF callback's directory options.
add_executable(late_native unrelated.c)
set(report "")
foreach(t player fixture64 fixture64w direct64 c_only p4pc_named_but_unrelated shared64 module64w late_native)
  string(APPEND report "${t}|$<TARGET_PROPERTY:${t},LINKER_LANGUAGE>|$<TARGET_PROPERTY:${t},LINK_LIBRARIES>\n")
endforeach()
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/graph.txt" CONTENT "${report}")
''', encoding="utf-8")
        (source / "fixtures/CMakeLists.txt").write_text(r'''
# Mirrors the real fixture factory's early LINK_LIBRARIES snapshot. Deferred
# attachment must leave it free of the base ABI's adapter.
get_target_property(copied player LINK_LIBRARIES)
foreach(suffix 64 64w)
  add_executable(fixture${suffix} ../main.c $<TARGET_OBJECTS:p4port${suffix}>)
  set_property(TARGET fixture${suffix} PROPERTY LINKER_LANGUAGE C)
  if(copied)
    target_link_libraries(fixture${suffix} PRIVATE ${copied})
  endif()
endforeach()
target_compile_definitions(fixture64 PRIVATE EXPECTED=22)
target_compile_definitions(fixture64w PRIVATE EXPECTED=33)
add_executable(direct64 ../main.c)
target_compile_definitions(direct64 PRIVATE EXPECTED=22)
target_link_libraries(direct64 PRIVATE p4port64)
set_property(TARGET direct64 PROPERTY LINKER_LANGUAGE C)
add_library(shared64 SHARED $<TARGET_OBJECTS:p4port64>)
target_link_libraries(shared64 existing_dep)
add_library(module64w MODULE $<TARGET_OBJECTS:p4port64w>)
target_link_libraries(module64w PRIVATE existing_dep)
set_target_properties(shared64 module64w PROPERTIES LINKER_LANGUAGE C)
''', encoding="utf-8")
        subprocess.run([args.cmake, "-S", str(source), "-B", str(build), "-G", "Ninja",
                        f"-DCMAKE_C_COMPILER={args.cc}",
                        "-DCMAKE_CXX_COMPILER:FILEPATH=CMAKE_CXX_COMPILER-NOTFOUND",
                        f"-DP4_HELPER={helper.as_posix()}",
                        f"-DP4_TEST_COFF={'ON' if args.coff else 'OFF'}"], check=True)
        report = {}
        for line in (build / "graph.txt").read_text(encoding="utf-8").splitlines():
            target, language, libraries = line.split("|", 2)
            report[target] = language, libraries.split(";")
        expected = {"player": ("CXX", "adapter"), "fixture64": ("CXX", "adapter64"),
                    "fixture64w": ("CXX", "adapter64w"), "direct64": ("CXX", "adapter64"),
                    "c_only": ("C", "adapter_c"), "shared64": ("CXX", "adapter64"),
                    "module64w": ("CXX", "adapter64w")}
        all_adapters = {"adapter", "adapter64", "adapter64w", "adapter_c"}
        for target, (language, adapter) in expected.items():
            actual_language, libraries = report[target]
            assert actual_language == language, (target, report[target])
            assert libraries.count(adapter) == 1, (target, libraries)
            assert all_adapters.intersection(libraries) == {adapter}, (target, libraries)
        for target in ("player", "fixture64", "fixture64w", "shared64", "module64w"):
            assert "existing_dep" in report[target][1], (target, report[target])
        assert report["p4pc_named_but_unrelated"][0] == "C", report
        assert not all_adapters.intersection(report["p4pc_named_but_unrelated"][1]), report
        assert report["late_native"][0] == "C", report
        assert not all_adapters.intersection(report["late_native"][1]), report
        subprocess.run([args.cmake, "--build", str(build), "--parallel", "2"], check=True)
        for relative in ("player", "c_only", "p4pc_named_but_unrelated", "late_native",
                         "fixtures/fixture64", "fixtures/fixture64w", "fixtures/direct64"):
            executable = build / relative
            if not executable.exists():
                executable = executable.with_suffix(".exe")
            subprocess.run([str(executable)], check=True)
        if args.coff:
            assert (build / "coff_link_probe.exe").is_file(), "COFF link probe was not built"
        # Configure the real provider without building it here: cross-ABI
        # execution belongs to the profile driver, not this native graph test.
        provider = source / "luau-config"
        provider.mkdir()
        (provider / "CMakeLists.txt").write_text(r'''
cmake_minimum_required(VERSION 3.20)
project(luau_profile_policy LANGUAGES C)
include("${P4_HELPER}")
p4_host_cxx_enable()
enable_testing()
set(P4_PORT64_LINK ON)
set(P4_PORT64W_OBJS "compiler_only_coff_mirror")
set(P4_WIN64_PREFIX "${CMAKE_BINARY_DIR}/win64-prefix")
if(NATIVE_WIN64_PREFIX)
  file(MAKE_DIRECTORY "${P4_WIN64_PREFIX}/lib")
endif()
set(P4_LUAU_WINE OFF CACHE FILEPATH "No runtime emulation in this configuration fixture" FORCE)
add_subdirectory("${P4_LUAU_SOURCE}" luau EXCLUDE_FROM_ALL)
foreach(t p4_luau64w p4_luau_VM64w p4_luau_Compiler64w p4_luau_Analysis64w
    p4_luau_check64w luau_sandbox_test64w)
  if(NATIVE_WIN64_PREFIX AND NOT TARGET ${t})
    message(FATAL_ERROR "Native prefix did not enable ${t}")
  elseif(NOT NATIVE_WIN64_PREFIX AND TARGET ${t})
    message(FATAL_ERROR "Compile-only mirror incorrectly enabled ${t}")
  endif()
endforeach()
get_property(runtime_tests DIRECTORY "${P4_LUAU_SOURCE}" PROPERTY TESTS)
if("luau_sandbox64w" IN_LIST runtime_tests OR "luau_analysis64w" IN_LIST runtime_tests)
  message(FATAL_ERROR "Unavailable Wine still registered 64w runtime tests")
endif()
if(NOT "luau_sandbox" IN_LIST runtime_tests OR NOT "luau_analysis" IN_LIST runtime_tests)
  message(FATAL_ERROR "Optional Wine disabled native runtime tests")
endif()
''', encoding="utf-8")
        luau_source = helper.parent.parent / "tools/luau"
        for native_prefix in (False, True):
            subprocess.run([args.cmake, "-S", str(provider), "-B", str(build / f"luau-{native_prefix}"),
                            "-G", "Ninja", f"-DCMAKE_C_COMPILER={args.cc}",
                            "-DCMAKE_CXX_COMPILER:FILEPATH=CMAKE_CXX_COMPILER-NOTFOUND",
                            f"-DP4_HELPER={helper.as_posix()}",
                            f"-DP4_LUAU_SOURCE={luau_source.as_posix()}",
                            f"-DNATIVE_WIN64_PREFIX={'ON' if native_prefix else 'OFF'}"], check=True)
        closure = source / "luau-closure"
        closure.mkdir()
        (closure / "CMakeLists.txt").write_text(r'''
cmake_minimum_required(VERSION 3.20)
project(luau_closure NONE)
set(P4PC_LUAU ON)
foreach(suffix 64 64w)
  add_custom_target(p4_x${suffix})
  add_custom_target(p4_luau${suffix})
  add_custom_target(p4_luau_check${suffix})
  add_custom_target(luau_sandbox_test${suffix})
  add_custom_target(mod_luau_test${suffix})
  add_custom_target(mod_luau_save_test${suffix})
endforeach()
include("${P4_CLOSURE}")
foreach(suffix 64 64w)
  get_target_property(deps p4_x${suffix} MANUALLY_ADDED_DEPENDENCIES)
  foreach(required p4_luau_check${suffix} luau_sandbox_test${suffix} mod_luau_test${suffix} mod_luau_save_test${suffix})
    if(NOT required IN_LIST deps)
      message(FATAL_ERROR "p4_x${suffix} is missing ${required}: ${deps}")
    endif()
  endforeach()
endforeach()
''', encoding="utf-8")
        subprocess.run([args.cmake, "-S", str(closure), "-B", str(build / "luau-closure"),
                        "-G", "Ninja", f"-DP4_CLOSURE={(helper.parent / 'sources/410-luau-closure.cmake').as_posix()}"],
                       check=True)
        abi_source = source / "native-abi"
        abi_source.mkdir()
        (abi_source / "native.c").write_text("int native(void) { return 0; }\n", encoding="utf-8")
        (abi_source / "CMakeLists.txt").write_text(r'''
cmake_minimum_required(VERSION 3.20)
project(native_abi_paths C)
set(P4_PORT64_OBJS ON)
set(P4_GEN "${CMAKE_BINARY_DIR}/gen")
set(P4_G64 "${P4_GEN}/obj64")
set(P4_PYTHON "${CMAKE_COMMAND}")
file(MAKE_DIRECTORY "${P4_GEN}")
file(WRITE "${P4_GEN}/package_boot_providers.c" "int generated(void) { return 0; }\n")
add_library(p4port OBJECT "${P4_GEN}/package_boot_providers.c")
add_library(p4portee OBJECT native.c)
include("${P4_ABI_FRAGMENT}")
''', encoding="utf-8")
        abi_build = build / "native-abi"
        subprocess.run([args.cmake, "-S", str(abi_source), "-B", str(abi_build), "-G", "Ninja",
                        f"-DCMAKE_C_COMPILER={args.cc}",
                        f"-DP4_ABI_FRAGMENT={(helper.parent / 'sources/340-port64-abi.cmake').as_posix()}"],
                       check=True)
        paths = (abi_build / "gen/crt-native-abi-files.txt").read_text(encoding="utf-8").splitlines()
        assert len(paths) == 2, paths
        assert any(path.endswith("/abi/generated/package_boot_providers.c.abi.json") for path in paths), paths
        assert any(path.endswith("/abi/port/native.c.abi.json") for path in paths), paths
        assert all("/../" not in path for path in paths), paths
    print("host adapter graph: deferred links, C/C++ discovery, native-prefix selection and optional emulation passed")


if __name__ == "__main__":
    main()
