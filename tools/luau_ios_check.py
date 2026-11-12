"""Apple-target Luau syntax qualification without distributing an Apple SDK.

Uses the AArch64 CI image's GNU C/C++ headers as declaration-only surrogates,
plus the two Apple platform APIs Luau calls. This is NOT a Darwin libc/libc++
ABI, link, binary-size or runtime qualification. A real iOS SDK build uses
system libc++ and remains required for packaging. No source is rewritten.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang", default="clang++")
    parser.add_argument("--clang-c", default="clang")
    parser.add_argument("--gcc-root", type=Path, default=Path("/usr/aarch64-linux-gnu/include/c++/14"))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    vendor = root / "third_party/luau"
    components = ("Common", "Ast", "Bytecode", "Compiler", "VM", "Config", "Analysis")
    includes = ["-isystem", str(args.gcc_root), "-isystem", str(args.gcc_root / "aarch64-linux-gnu"),
                "-isystem", str(args.gcc_root / "backward"), "-isystem", "/usr/aarch64-linux-gnu/include",
                "-isystem", "/usr/include/aarch64-linux-gnu", "-I", str(root / "include")]
    for component in components:
        includes += ["-I", str(vendor / component / "include")]
    with tempfile.TemporaryDirectory(prefix="p4-luau-ios-") as temporary:
        stubs = Path(temporary)
        (stubs / "mach").mkdir()
        (stubs / "mach/mach.h").write_text("#pragma once\n#include <stdint.h>\n", encoding="utf-8")
        (stubs / "mach/mach_time.h").write_text(
            "#pragma once\n#include <stdint.h>\nextern \"C\" {\n"
            "typedef struct { uint32_t numer, denom; } mach_timebase_info_data_t;\n"
            "int mach_timebase_info(mach_timebase_info_data_t *);\n"
            "uint64_t mach_absolute_time(void);\n}\n", encoding="utf-8")
        (stubs / "pthread.h").write_text(
            "#pragma once\n#include_next <pthread.h>\nextern \"C\" {\n"
            "void *pthread_get_stackaddr_np(pthread_t);\n"
            "size_t pthread_get_stacksize_np(pthread_t);\n}\n", encoding="utf-8")
        count = 0
        # Apple's predefined __nonnull=_Nonnull hides glibc's function-like
        # attribute macro. Undefine that alias only for these GNU surrogates.
        for component in components:
            for source in sorted((vendor / component / "src").glob("*.cpp")):
                command = [args.clang, "--target=arm64-apple-ios13.0", "-fsyntax-only", "-std=c++17",
                           "-fno-rtti", "-U__nonnull", "-D_GNU_SOURCE", "-DLUA_USE_LONGJMP=1", '-DLUA_API=extern "C"',
                           '-DLUACODE_API=extern "C"', "-I", str(stubs), *includes]
                if component == "VM":
                    command += ["-fno-exceptions"]
                subprocess.run([*command, str(source)], check=True)
                count += 1
        subprocess.run([args.clang, "--target=arm64-apple-ios13.0", "-fsyntax-only", "-std=c++17",
                        "-fno-rtti", "-U__nonnull", "-D_GNU_SOURCE", '-DLUA_API=extern "C"', '-DLUACODE_API=extern "C"',
                        "-I", str(stubs), *includes, str(root / "src/platform/mods/luau_c.cpp")], check=True)
    subprocess.run([args.clang_c, "--target=arm64-apple-ios13.0", "-fsyntax-only", "-nostdlibinc",
                    "-Wall", "-Wextra", "-Werror", "-isystem", str(root / "ci/ios/stubs"),
                    "-I", str(root / "include"), "-I", str(vendor / "VM/include"),
                    str(root / "tests/luau_sandbox_test.c")], check=True)
    print(f"luau iOS: {count} vendor units + C++ boundary + C test passed arm64-apple-ios13.0 syntax only; "
          "GNU surrogate headers, no SDK/link/runtime/size claim")


if __name__ == "__main__":
    main()
