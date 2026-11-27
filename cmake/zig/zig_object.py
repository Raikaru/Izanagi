#!/usr/bin/env python3
"""Compile one Zig source to one object for the CMake `Zig` language (cmake/zig/CMakeZigInformation.cmake).

CMake gives a .zig source the same per-target compile options, definitions and include directories as a C source of
that target, so the profile's clang spelling of the target (--target=, -m32/-m64) selects the Zig target the same way
clang selects it: the last --target wins and a later -m32/-m64 overrides its x86 width. Code-generation flags with no Zig
meaning are dropped by name; any other flag is an error, never silently ignored.

A root whose module imports `c_bindings` gets its C declarations from its own header, bindings/<root>.h beside it:
`zig translate-c` turns that header, with the object's target, libc, optimisation mode, definitions and include
directories, into a content-addressed file below <local-cache>/c-bindings/<SHA256>/, which the root's
`zig build-obj` takes as the module `c_bindings`. The digest covers the actual translated bytes, not the temporary
object output directory. The depfile lists the source, the Zig files it imports, the binding header and the repository
headers it includes, and this script, so Ninja rebuilds the object when any of them changes.
"""
import argparse
import hashlib
import os
import re
import subprocess
import sys

# clang flags (one argument) that have no meaning for Zig code: C language and warning switches, x87/SSE selection
# (the CPU model below implies SSE2 on x86) and outline atomics. PIC/PIE are not here: they map to Zig's (PIC_FLAGS).
IGNORED = re.compile(
    r"^(-W.*|-w|-fno-strict-aliasing|-fstrict-aliasing|-fwrapv|-ffp-contract=.*|-mfpmath=.*|-msse.*|-mno-outline-atomics|"
    r"-moutline-atomics|-fsigned-char|-funsigned-char|-fno-blocks|-fno-builtin|-fbuiltin|-ftrivial-auto-var-init=.*|"
    r"-fdiagnostics-color.*|-fcolor-diagnostics|"
    r"-fno-stack-protector|-fstack-protector.*|--gcc-install-dir=.*|--sysroot=.*|-mmacosx-version-min=.*)$")
# clang flags that take the next argument and have no meaning for Zig code (the Apple SDK: Zig ships the macOS headers).
IGNORED_WITH_VALUE = {"-isysroot", "--sysroot"}
# The clang position-independence family: one setting, the last flag wins (as in clang); -fpic/-fpie are -fPIC/-fPIE (Zig has one
# PIC model). PIE implies PIC code. None given: Zig's own default for the target and libc.
PIC_FLAGS = {"-fpic": "pic", "-fPIC": "pic", "-fpie": "pie", "-fPIE": "pie",
             "-fno-pic": "none", "-fno-PIC": "none", "-fno-pie": "none", "-fno-PIE": "none"}

ARCHES = {"i386": "x86", "i486": "x86", "i586": "x86", "i686": "x86", "x86": "x86",
          "x86_64": "x86_64", "amd64": "x86_64", "aarch64": "aarch64", "arm64": "aarch64"}
ANDROID_DIRS = {"x86": "i686-linux-android", "x86_64": "x86_64-linux-android", "aarch64": "aarch64-linux-android"}


def fail(message):
    sys.exit(f"zig_object: {message}")


def zig_target(triple, width):
    """The Zig target query for a clang triple, with a -m32/-m64 override (None: as the triple says)."""
    parts = triple.split("-")
    arch = ARCHES.get(parts[0])
    if arch is None:
        fail(f"unsupported architecture in target {triple!r}")
    if width == 32 and arch == "x86_64":
        arch = "x86"
    elif width == 64 and arch == "x86":
        arch = "x86_64"
    elif width is not None and arch == "aarch64":
        fail(f"-m{width} does not apply to {triple!r}")
    rest = "-".join(parts[1:])
    android = re.search(r"linux-android(\d*)$", rest)
    macos = re.search(r"(?:macos|macosx|darwin)([\d.]*)$", rest)
    if android:
        api = android.group(1)
        return f"{arch}-linux-android" + (f".{api}" if api else ""), "android", arch, api
    if "mingw32" in rest or "windows-gnu" in rest:
        # Zig's bundled mingw-w64 headers default to UCRT (__MSVCRT_VERSION__ 0xE00), as llvm-mingw's ucrt toolchain does.
        return f"{arch}-windows-gnu", "windows", arch, None
    if macos and "apple" in rest:
        version = macos.group(1)
        return f"{arch}-macos" + (f".{version}" if version else "") + "-none", "macos", arch, None
    if re.search(r"linux-gnu$", rest):
        return f"{arch}-linux-gnu", "linux", arch, None
    fail(f"unsupported target {triple!r} (supported: Linux glibc, mingw-w64, Android, macOS)")


def android_libc(sysroot, arch, api, cache_dir):
    """A Zig libc installation file for the NDK's bionic (Zig ships no Android headers)."""
    if not sysroot or not os.path.isdir(os.path.join(sysroot, "usr", "include")):
        fail(f"an Android target needs the NDK sysroot (CMAKE_SYSROOT); got {sysroot!r}")
    if not api:
        fail("an Android target needs its API level in the target triple (CMAKE_C_COMPILER_TARGET, e.g. aarch64-none-linux-android24)")
    triple = ANDROID_DIRS[arch]
    crt = os.path.join(sysroot, "usr", "lib", triple, api)
    if not os.path.isdir(crt):
        fail(f"the NDK sysroot has no {crt}")
    os.makedirs(cache_dir, exist_ok=True)
    path = os.path.join(cache_dir, f"libc-{triple}{api}.txt")
    # Zig requires cc_dir on Linux targets (Android included); the NDK keeps its crtbegin/crtend objects in the API's crt_dir.
    text = (f"include_dir={os.path.join(sysroot, 'usr', 'include')}\n"
            f"sys_include_dir={os.path.join(sysroot, 'usr', 'include', triple)}\n"
            f"crt_dir={crt}\nmsvc_lib_dir=\nkernel32_lib_dir=\ncc_dir={crt}\ndarwin_sdk_dir=\n")
    write_if_changed(path, text)
    return path


def write_if_changed(path, text):
    try:
        with open(path, encoding="utf-8", newline="") as f:
            if f.read() == text:
                return
    except OSError:
        pass
    tmp = f"{path}.{os.getpid()}"
    with open(tmp, "w", encoding="utf-8", newline="") as f:
        f.write(text)
    os.replace(tmp, path)


ZIG_IMPORT = re.compile(r'@import\(\s*"([^"]+\.zig)"\s*\)')
BINDINGS_IMPORT = re.compile(r'@import\(\s*"c_bindings"\s*\)')
PP_INCLUDE = re.compile(r'^\s*#\s*include\s*(?:<([^>]+)>|"([^"]+)")', re.M)


def zig_files(source):
    """The source and the .zig files it imports by path (its module), with whether any of them imports c_bindings."""
    files, seen, pending, bindings = [], set(), [os.path.abspath(source)], False
    while pending:
        path = pending.pop()
        if path in seen:
            continue
        seen.add(path)
        files.append(path)
        with open(path, encoding="utf-8", errors="replace") as f:
            text = f.read()
        bindings = bindings or BINDINGS_IMPORT.search(text) is not None
        pending += [os.path.normpath(os.path.join(os.path.dirname(path), n)) for n in ZIG_IMPORT.findall(text)]
    return files, bindings


def header_dependencies(header, include_dirs):
    """The header and the headers under include_dirs it includes, transitively (quoted: the includer's directory first)."""
    def resolve(name, here):
        for base in ([here] if here else []) + include_dirs:
            path = os.path.normpath(os.path.join(base, name))
            if os.path.isfile(path):
                return path
        return None  # a libc or system header: provided by the pinned Zig or the NDK, not by this tree

    deps, seen, pending = [], set(), [os.path.abspath(header)]
    while pending:
        path = pending.pop()
        if path in seen:
            continue
        seen.add(path)
        deps.append(path)
        here = os.path.dirname(path)
        with open(path, encoding="utf-8", errors="replace") as f:
            text = f.read()
        pending += [p for p in (resolve(a or q, None if a else here) for a, q in PP_INCLUDE.findall(text)) if p]
    return deps


def write_depfile(depfile, obj, deps):
    def esc(p):
        return p.replace("\\", "/").replace(" ", "\\ ").replace("#", "\\#").replace("$", "$$")
    tmp = f"{depfile}.{os.getpid()}"
    with open(tmp, "w", encoding="utf-8") as f:
        f.write(esc(obj) + ": \\\n")
        f.write(" \\\n".join("  " + esc(d) for d in deps) + "\n")
    os.replace(tmp, depfile)

def main():
    argv = sys.argv[1:]
    if "--" not in argv:
        fail("usage: zig_object.py <driver options> -- <target compile flags>")
    split = argv.index("--")
    ap = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    ap.add_argument("--zig", required=True)
    ap.add_argument("--default-target", required=True)
    ap.add_argument("--optimize", required=True, choices=["auto", "debug", "safe", "fast", "small"])
    ap.add_argument("--cache-dir", required=True)
    ap.add_argument("--global-cache-dir", required=True)
    ap.add_argument("--android-sysroot", default="")
    ap.add_argument("-o", dest="obj", required=True)
    ap.add_argument("-c", dest="source", required=True)
    known = ap.parse_args(argv[:split])
    flags = argv[split + 1:]

    target, width, depfile, strip, frame, level, pic = known.default_target, None, None, False, None, None, None
    function_sections = data_sections = None
    defines, includes, system_includes = [], [], []
    i = 0

    def value():
        nonlocal i
        i += 1
        if i >= len(flags):
            fail(f"{flags[i - 1]} needs a value")
        return flags[i]

    while i < len(flags):
        a = flags[i]
        if a.startswith("--target="):
            target = a.split("=", 1)[1]
        elif a in ("-target", "--target"):
            target = value()
        elif a == "-m32":
            width = 32
        elif a == "-m64":
            width = 64
        elif a == "--depfile":
            depfile = value()
        elif a == "-D":
            defines.append(value())
        elif a.startswith("-D"):
            defines.append(a[2:])
        elif a == "-I":
            includes.append(value())
        elif a.startswith("-I"):
            includes.append(a[2:])
        elif a == "-isystem":
            system_includes.append(value())
        elif a.startswith("-isystem"):
            system_includes.append(a[len("-isystem"):])
        elif re.fullmatch(r"-O[0-3sgz]?", a):
            level = a
        elif re.fullmatch(r"-g(0|1|2|3)?|-gline-tables-only", a):
            strip = a == "-g0"
        elif a == "-fno-omit-frame-pointer":
            frame = "-fno-omit-frame-pointer"
        elif a == "-fomit-frame-pointer":
            frame = "-fomit-frame-pointer"
        elif a in ("-ffunction-sections", "-fno-function-sections"):
            function_sections = a
        elif a in ("-fdata-sections", "-fno-data-sections"):
            data_sections = a
        elif a in PIC_FLAGS:
            pic = PIC_FLAGS[a]
        elif a in IGNORED_WITH_VALUE:
            value()
        elif IGNORED.match(a):
            pass
        else:
            fail(f"unsupported compile flag {a!r} for {known.source} (add its Zig meaning to cmake/zig/zig_object.py, or keep it C-only "
                 f"with $<$<COMPILE_LANGUAGE:C>:...>)")
        i += 1

    # auto: the unit's own C optimisation level picks the mode (-O0: debug; otherwise safe, whose checked arithmetic
    # and bounds stop at the defect instead of corrupting guest memory). An explicit P4_ZIG_OPTIMIZE wins.
    optimize = known.optimize
    if optimize == "auto":
        optimize = "debug" if level == "-O0" else "safe"

    query, os_kind, arch, api = zig_target(target, width)
    # Zig (0.17 and the pinned 0.18 nightly: "the selected target requires position independent code") requires PIC for
    # libc-linked objects even when the surrounding C profile uses -fno-pic. PIC objects remain valid in its non-PIE executable.
    if pic == "none":
        pic = "pic"
    if os_kind == "linux":
        # The host glibc headers the clang profile names (multiarch -isystem) would mix with Zig's own glibc headers.
        system_includes = [d for d in system_includes if not os.path.abspath(d).replace("\\", "/").startswith("/usr/")]

    source = os.path.abspath(known.source)
    root = os.path.splitext(os.path.basename(source))[0]
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", root) or root in ("c_bindings", "std", "builtin", "root"):
        fail(f"{known.source}: the root's file name must be a Zig identifier other than c_bindings/std/builtin/root")
    zig_deps, uses_bindings = zig_files(source)
    includes_abs = [os.path.abspath(d) for d in includes + system_includes]

    # Per-module options: every module of the object (the root and its c_bindings) is compiled for the same target and mode.
    module = ["-target", query, "-mcpu", "baseline", "-O", optimize, "-lc", "-fno-stack-check", "-fno-stack-protector"]
    if strip:
        module.append("-fstrip")
    if frame:
        module.append(frame)
    if pic is not None:
        module.append("-fno-PIC" if pic == "none" else "-fPIC")
    libc = []
    if os_kind == "android":
        libc = ["--libc", android_libc(known.android_sysroot, arch, api, known.cache_dir)]
    caches = ["--cache-dir", known.cache_dir, "--global-cache-dir", known.global_cache_dir]

    cmd = [known.zig, "build-obj", "-fno-compiler-rt", "-fno-ubsan-rt", "-fno-emit-h", *caches, *libc, f"-femit-bin={known.obj}"]
    if function_sections:
        cmd.append(function_sections)
    if data_sections:
        cmd.append(data_sections)
    if os_kind == "windows":
        cmd.append("-fno-dll-export-fns")   # the C ABI exports are link-time symbols of the exe, not PE exports
    if pic is not None:
        cmd.append("-fPIE" if pic == "pie" else "-fno-PIE")

    os.makedirs(os.path.dirname(os.path.abspath(known.obj)), exist_ok=True)
    header_deps = []
    if uses_bindings:
        header = os.path.join(os.path.dirname(source), "bindings", f"{root}.h")
        if not os.path.isfile(header):
            fail(f"{known.source} imports c_bindings but has no binding header {header}")
        # The translation sees the object's target, libc, mode, definitions and include directories, as @cImport did.
        translate = [known.zig, "translate-c", header, "-target", query, "-mcpu", "baseline", "-O", optimize, "-lc", *libc, *caches]
        if pic is not None:
            translate += ["-fno-PIC" if pic == "none" else "-fPIC", "-fPIE" if pic == "pie" else "-fno-PIE"]
        translate += [f"-D{d}" for d in defines]
        if os_kind == "android":
            # Bionic's fortified stdio overloads cannot be represented by
            # translate-c. This affects declarations only, not C compilation.
            translate = [a for a in translate if a.partition("=")[0] != "-D_FORTIFY_SOURCE"]
            translate.append("-D_FORTIFY_SOURCE=0")
        for d in includes:
            translate += ["-I", d]
        for d in system_includes:
            translate += ["-isystem", d]
        translated = subprocess.run(translate, stdout=subprocess.PIPE)
        if translated.returncode != 0:
            sys.exit(translated.returncode)
        # Run steps stage object outputs in random directories; those must not become DWARF source paths.
        binding_dir = os.path.join(os.path.abspath(known.cache_dir), "c-bindings", hashlib.sha256(translated.stdout).hexdigest())
        os.makedirs(binding_dir, exist_ok=True)
        bindings = os.path.join(binding_dir, os.path.basename(known.obj) + ".c_bindings.zig")
        write_if_changed(bindings, translated.stdout.decode("utf-8"))
        cmd += [*module, "--dep", "c_bindings", f"-M{root}={source}", *module, f"-Mc_bindings={bindings}"]
        header_deps = header_dependencies(header, includes_abs)
    else:
        cmd += [*module, f"-M{root}={source}"]

    result = subprocess.run(cmd)
    if result.returncode != 0:
        sys.exit(result.returncode)
    if depfile:
        write_depfile(depfile, known.obj, zig_deps + header_deps + [os.path.abspath(__file__)])


if __name__ == "__main__":
    main()
