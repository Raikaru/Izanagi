#!/usr/bin/env python3
"""Semantic checks of the `zig build` graph (zigbuild/core.zig) against the CMake definitions it replaces.

    zig_build_graph_test.py sources  --build <build tree>
    zig_build_graph_test.py adapters --build <build tree>
    zig_build_graph_test.py profiles --build <Zig build tree> [--cmake-cache <qualified CMakeCache.txt>]

The Zig graph writes <build>/zig-graph.json, <build>/compile_commands.json and <build>/gen/* at configure time. These
checks read those files and the repository's own CMake fragments; they replace tests/cmake_sources_test.py and
tests/host_adapter_graph_test.py, which exercise CMake's include driver and host-adapter attachment, for the graph that
does not use CMake.

sources: the source-owner fragments (cmake/sources/*.cmake, in their sorted include order) define p4port and p4portee;
every p4port<suffix>/p4portee<suffix> of the graph has exactly those sources in that order (the profile's OS source, the
Android-only coroutine source, the CRI glob, the generated package boot table), each with exactly one compile command whose
output is its CMakeFiles/<target>.dir object and whose command carries the target's definitions (lodepng's
source-level definitions included); the generated package boot table registers the providers in fragment order; the
profile's final targets are those the fragments' Android exits leave (320: Android x86 ends with libmain.so, 350: the
64-bit Android build ends with libmain.so and the optional phone runner, desktop: p4pc, p4pc_viewer, p4pc_sfdview).

adapters: every final consumer of p4port<suffix> links exactly one copy of each host adapter of its own model and none of
another model, links as C++ exactly when the Luau binding is active, keeps its own libraries; consumers that do not take
p4port's objects (p4pc_sfdview, ffmpeg_license_test64w) and the Luau tools take no mod adapter; the Luau tools of a model
link only that model's provider; the 64-bit CRT native ABI manifest lists the generated package boot table under
abi/generated, every port C source under abi/port, the Zig exports, and no path that escapes its directory.

profiles: compare effective native/ELF/COFF optimization levels from the graph
manifest with an actual CMake cache, or the repository's qualified CI recipes.
"""
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class Failure(Exception):
    pass


def check(condition, message):
    if not condition:
        raise Failure(message)


def load(build):
    manifest = json.loads((build / "zig-graph.json").read_text(encoding="utf-8"))
    compdb = json.loads((build / "compile_commands.json").read_text(encoding="utf-8"))
    return manifest, compdb


OPTIMIZATIONS = {"P4_OPT": "opt", "P4_PORT64_OPT": "port64_opt", "P4_PORT64W_OPT": "port64w_opt"}


def qualified_optimizations():
    recipes = [ROOT / "tools/ci_shard.sh"]
    private = ROOT / "tools/p4pc_ci_run.sh"
    if private.is_file():
        recipes.append(private)
    levels = {name: set() for name in OPTIMIZATIONS}
    for recipe in recipes:
        text = "\n".join(line for line in recipe.read_text(encoding="utf-8").splitlines()
                         if not line.lstrip().startswith("#"))
        for name, level in re.findall(r"-D(P4_OPT|P4_PORT64_OPT|P4_PORT64W_OPT)(?::STRING)?=(O[0-3])\b", text):
            levels[name].add(level)
    required = {"P4_OPT", "P4_PORT64_OPT"} | ({"P4_PORT64W_OPT"} if private.is_file() else set())
    for name, values in levels.items():
        if name in required or values:
            check(len(values) == 1, f"qualified CMake recipes have absent/ambiguous {name}: {sorted(values)}")
    return {OPTIMIZATIONS[name]: next(iter(values)) for name, values in levels.items() if values}


def cache_optimizations(path):
    levels = {}
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        match = re.fullmatch(r"(P4_OPT|P4_PORT64_OPT|P4_PORT64W_OPT):[^=]+=(O[0-3])", line)
        if match:
            check(OPTIMIZATIONS[match[1]] not in levels, f"duplicate optimization in CMake cache: {match[1]}")
            levels[OPTIMIZATIONS[match[1]]] = match[2]
    return levels


def compare_optimizations(actual, expected):
    check(expected, "CMake optimization census is empty")
    for name, level in expected.items():
        check(name in actual, f"Zig optimization census lacks {name}")
        check(actual[name] == level, f"optimization drift for {name}: CMake {level}, Zig {actual[name]}")


def test_profiles(build, cmake_cache=None):
    manifest = json.loads((build / "zig-graph.json").read_text(encoding="utf-8"))
    expected = cache_optimizations(cmake_cache) if cmake_cache else qualified_optimizations()
    active = ["opt"] + (["port64_opt"] if manifest["port64"] else []) + (["port64w_opt"] if manifest["port64w"] else [])
    for name in active:
        check(name in expected, f"CMake optimization census lacks active {name}")
    compare_optimizations(manifest, {name: expected[name] for name in active})
    print("zig graph optimization census: " + ", ".join(f"{name}={expected[name]}" for name in active))


def commands(text):
    """The commands of a CMake file in order: (name, arguments). Quoted, bracket and unquoted arguments and comments are
    lexed as CMake does, so text inside strings (a generated C source's `if (...)`) is never taken for a command."""
    i, n = 0, len(text)

    def bracket_at(j):
        m = re.match(r"\[(=*)\[", text[j:])
        return m

    while i < n:
        ch = text[i]
        if ch == "#":
            m = bracket_at(i + 1)
            if m:
                end = text.find("]" + m.group(1) + "]", i + 1 + m.end())
                i = n if end < 0 else end + len(m.group(1)) + 2
            else:
                j = text.find("\n", i)
                i = n if j < 0 else j + 1
            continue
        m = re.match(r"[A-Za-z_][A-Za-z0-9_]*", text[i:])
        if not m:
            i += 1
            continue
        name = m.group(0)
        j = i + m.end()
        while j < n and text[j] in " \t":
            j += 1
        if j >= n or text[j] != "(":
            i = j
            continue
        args, i = arguments(text, j + 1)
        yield name.lower(), args


class Quoted(str):
    """A quoted or bracket argument: one word whatever it contains (if() splits only unquoted parentheses)."""


def arguments(text, i):
    """The arguments after a command's '(' and the index after its ')'."""
    n = len(text)
    args, depth, current, have = [], 0, [], False

    def flush():
        nonlocal current, have
        if have:
            args.append("".join(current))
        current, have = [], False

    while i < n:
        ch = text[i]
        if ch == '"':
            flush()
            j, out = i + 1, []
            while j < n and text[j] != '"':
                if text[j] == "\\" and j + 1 < n:
                    nxt = text[j + 1]
                    out.append({"n": "\n", "t": "\t", "r": "\r", "\n": ""}.get(nxt, nxt))
                    j += 2
                else:
                    out.append(text[j])
                    j += 1
            args.append(Quoted("".join(out)))
            i = j + 1
        elif ch == "[" and re.match(r"\[=*\[", text[i:]) and not have:
            m = re.match(r"\[(=*)\[", text[i:])
            close = "]" + m.group(1) + "]"
            end = text.find(close, i + m.end())
            check(end >= 0, "unterminated bracket argument")
            args.append(Quoted(text[i + m.end():end]))
            i = end + len(close)
        elif ch == "#":
            flush()
            j = text.find("\n", i)
            i = n if j < 0 else j + 1
        elif ch in " \t\r\n":
            flush()
            i += 1
        elif ch == "(":
            depth += 1
            current.append(ch)
            have = True
            i += 1
        elif ch == ")":
            if depth == 0:
                flush()
                return args, i + 1
            depth -= 1
            current.append(ch)
            have = True
            i += 1
        else:
            if ch == "\\" and i + 1 < n:
                current.append(text[i + 1])
                i += 2
            else:
                current.append(ch)
                i += 1
            have = True
    raise Failure("unterminated command arguments")


def facts(manifest):
    """The configure variables the source fragments branch on, as this profile set them (None: not known here)."""
    return {
        "P4_ANDROID": manifest["android"], "P4_ANDROID64": manifest["android64"],
        "P4_WIN32": manifest["win32"], "WIN32": manifest["win32"],
        "P4_APPLE": manifest["apple"], "APPLE": manifest["apple"],
        "P4PC_MOD_SUPPORT": manifest["mod_support"], "P4PC_LUAU": manifest["luau"],
        # 030-generation-setup.cmake: the port transcriptions the selected RenderWare clean subsystems retire.
        "P4_RW_CLEAN_PORT_DROP": bool(manifest["rw_clean_port_drop"]),
    }


def evaluate(words, known):
    """if() arguments as CMake evaluates them (NOT, AND, OR, parentheses, variables, STREQUAL), in three values: True,
    False, or None when a variable this profile record does not carry decides it."""
    pos = 0

    def value(token):
        if token in known:
            return known[token]
        if token.upper() in ("1", "ON", "YES", "TRUE", "Y"):
            return True
        if token.upper() in ("0", "OFF", "NO", "FALSE", "N", "") or token.upper().endswith("-NOTFOUND"):
            return False
        return None

    def text_of(token):
        # CMake dereferences a bare variable name in a comparison; this record holds no string variables, so any
        # variable reference (or a name that may be one) leaves the comparison undecided
        if re.fullmatch(r"\$\{\w+\}|[A-Za-z_][A-Za-z0-9_]*", token):
            return None
        return token

    binary = {"STREQUAL", "MATCHES", "EQUAL", "LESS", "GREATER", "LESS_EQUAL", "GREATER_EQUAL", "STRLESS", "STRGREATER",
              "VERSION_LESS", "VERSION_GREATER", "VERSION_EQUAL", "VERSION_LESS_EQUAL", "VERSION_GREATER_EQUAL",
              "IN_LIST", "IS_NEWER_THAN", "PATH_EQUAL"}
    unary = {"EXISTS", "TARGET", "DEFINED", "TEST", "COMMAND", "POLICY", "IS_DIRECTORY", "IS_ABSOLUTE", "IS_SYMLINK"}

    def primary():
        nonlocal pos
        token = words[pos]
        pos += 1
        if token == "(":
            v = disjunction()
            check(pos < len(words) and words[pos] == ")", f"unbalanced if({' '.join(words)})")
            pos += 1
            return v
        if token in unary and pos < len(words):
            pos += 1
            return None
        if pos < len(words) and words[pos] in binary:
            op = words[pos]
            pos += 2
            if op != "STREQUAL":
                return None
            left, right = text_of(token), text_of(words[pos - 1])
            return None if left is None or right is None else left == right
        return value(token)

    def negation():
        nonlocal pos
        if words[pos] == "NOT":
            pos += 1
            v = negation()
            return None if v is None else not v
        return primary()

    def conjunction():
        nonlocal pos
        v = negation()
        while pos < len(words) and words[pos] == "AND":
            pos += 1
            w = negation()
            v = False if v is False or w is False else (None if v is None or w is None else True)
        return v

    def disjunction():
        nonlocal pos
        v = conjunction()
        while pos < len(words) and words[pos] == "OR":
            pos += 1
            w = conjunction()
            v = True if v is True or w is True else (None if v is None or w is None else False)
        return v

    # tokens: split the parentheses CMake allows as separate arguments of if(); a quoted argument stays one word
    split = []
    for w in words:
        if isinstance(w, Quoted) or w in ("(", ")"):
            split.append(w)
        else:
            split.extend(t for t in re.split(r"(\(|\))", w) if t != "")
    words = split
    if not words:
        return False
    v = disjunction()
    if pos != len(words):
        return None  # an operator this evaluator does not model (MATCHES, TARGET, ...): undecided
    return v


def cmake_port_sources(manifest):
    """The p4port and p4portee SOURCES the fragments define for this profile, as repository-relative paths, and the
    package boot providers in registration order. Conditions are evaluated against the profile; a source or registration
    under a condition this profile does not decide, or inside a function/macro body, fails the check."""
    known = facts(manifest)
    win32 = manifest["win32"]
    variables = {}
    targets = {"p4port": [], "p4portee": []}
    providers = []
    for fragment in sorted((ROOT / "cmake/sources").glob("*.cmake")):
        # each frame: [active (True/False/None), some earlier branch taken (True/False/None), kind]
        stack = [[True, True, "file"]]
        for name, words in commands(fragment.read_text(encoding="utf-8")):
            parent = stack[-1][0]
            if name in ("function", "macro"):
                stack.append(["definition", True, name])
                continue
            if name in ("endfunction", "endmacro"):
                check(stack[-1][2] in ("function", "macro"), f"{fragment.name}: unbalanced {name}")
                stack.pop()
                continue
            if name == "if":
                if parent == "definition":
                    stack.append(["definition", True, "if"])
                else:
                    v = evaluate(words, known) if parent is True else (False if parent is False else None)
                    stack.append([v, v, "if"])
                continue
            if name in ("elseif", "else", "endif"):
                check(stack[-1][2] == "if", f"{fragment.name}: unbalanced {name}")
                frame = stack[-1]
                outer = stack[-2][0]
                if name == "endif":
                    stack.pop()
                elif outer == "definition":
                    pass
                elif frame[1] is True:
                    frame[0] = False
                elif outer is not True or frame[1] is None:
                    frame[0] = False if outer is False else None
                    frame[1] = None if frame[0] is None else frame[1]
                else:
                    v = evaluate(words, known) if name == "elseif" else True
                    frame[0] = v
                    frame[1] = v
                continue
            active = parent
            sources = name in ("add_library", "target_sources") and words and words[0] in targets
            registration = name == "p4_register_package_boot_provider"
            if not (sources or registration or name in ("set", "file", "list")):
                continue
            if active == "definition":
                check(not (sources or registration), f"{fragment.name}: p4port sources or a provider inside a {stack[-1][2]} body: {words}")
                continue
            if sources or registration:
                check(active is not None, f"{fragment.name}: {words[0] if sources else 'a provider registration'} under a condition this profile does not decide: {words}")
            if active is not True:
                continue
            if sources:
                for item in words[2:]:  # OBJECT / PRIVATE
                    if item.startswith("$<"):
                        raise Failure(f"{fragment.name}: generator expression in {words[0]} sources: {item}")
                    if item == "${P4_OS_SRC}":
                        targets[words[0]].append("src/platform/os/os_win32.c" if win32 else "src/platform/os/os_posix.c")
                    elif item == "${P4_GEN}/package_boot_providers.c":
                        targets[words[0]].append("<gen>/package_boot_providers.c")
                    elif item.startswith("${") and item.endswith("}"):
                        var = item[2:-1]
                        check(var in variables, f"{fragment.name}: unknown source variable {item}")
                        targets[words[0]].extend(variables[var])
                    else:
                        targets[words[0]].append(item.replace("${CMAKE_SOURCE_DIR}/", ""))
            elif registration:
                providers.append(words[0])
            elif name == "set" and words and words[0] in ("P4_BATTLE_SIM_ZIG", "P4_PNG_SOURCES", "P4_PORT_RW_SRCS", "P4_PORTEE_SRCS"):
                variables[words[0]] = [w.replace("${CMAKE_SOURCE_DIR}/", "") for w in words[1:]]
            elif name == "file" and words[:2] == ["GLOB", "P4_CRI_SRC"]:
                cri = sorted(p.relative_to(ROOT).as_posix() for p in (ROOT / "src/cri").glob("*.c"))
                variables["P4_CRI_SRC"] = [p for p in cri if not re.search(r"/(aixp|acs|cri_glue)\.c$", p)]
            elif name == "list" and words[:1] == ["REMOVE_ITEM"]:
                check(len(words) == 3 and words[1] in variables and words[2] == "${P4_RW_CLEAN_PORT_DROP}",
                      f"{fragment.name}: unsupported list(REMOVE_ITEM) of p4port sources: {words}")
                variables[words[1]] = [s for s in variables[words[1]] if s not in manifest["rw_clean_port_drop"]]
        check(len(stack) == 1, f"{fragment.name}: unbalanced if/function blocks")
    check(targets["p4port"] and targets["p4portee"], "no p4port/p4portee sources found in cmake/sources")
    return targets, providers


def cmake_mod_game_sources(text, luau_active):
    """Read the literal sources of the CMake mod game archive, including its Luau additions."""
    base, luau = [], []
    for name, words in commands(text):
        if not words or words[0] != "p4_mod_game${suffix}":
            continue
        if name == "add_library":
            check(words[1:3] == ["STATIC", "${bridges}"], "unexpected p4_mod_game declaration")
            dest, items = base, words[3:]
        elif name == "target_sources":
            check(words[1:2] == ["PRIVATE"], "unexpected p4_mod_game source scope")
            dest, items = luau, words[2:]
        else:
            continue
        for item in items:
            check(item.startswith("${P4_MOD_ROOT}/") and item.endswith(".c"),
                  f"unsupported p4_mod_game source: {item}")
            dest.append(item[len("${P4_MOD_ROOT}/"):])
    check(base and luau, "missing CMake p4_mod_game source declaration")
    return base + (luau if luau_active else [])


def compare_mod_game_sources(actual, expected, name):
    check(actual == expected, f"{name}: sources differ from tools/mods/CMakeLists.txt:\n"
          f"  graph only: {sorted(set(actual) - set(expected))}\n"
          f"  cmake only: {sorted(set(expected) - set(actual))}")


def relative(path, manifest):
    path = Path(path).as_posix()
    gen = Path(manifest["gen_dir"]).as_posix()
    root = Path(manifest["root"]).as_posix()
    if path.startswith(gen + "/"):
        return "<gen>/" + path[len(gen) + 1:]
    check(path.startswith(root + "/"), f"{path} is outside the source root")
    return path[len(root) + 1:]


def test_sources(build):
    manifest, compdb = load(build)
    expected, providers = cmake_port_sources(manifest)
    targets = {t["name"]: t for t in manifest["targets"]}
    by_output = {}
    for entry in compdb:
        check(entry["output"] not in by_output, f"two compile commands write {entry['output']}")
        by_output[entry["output"]] = entry
    mod_text = (ROOT / "tools/mods/CMakeLists.txt").read_text(encoding="utf-8")
    mod_want = cmake_mod_game_sources(mod_text, manifest["mod_support"] and manifest["luau"])
    for suffix in ("", "64", "64w"):
        name = "p4_mod_game" + suffix
        entries = [e for e in compdb if f"/CMakeFiles/{name}.dir/" in e["output"].replace("\\", "/")]
        if entries:
            actual = [relative(e["file"], manifest) for e in entries]
            compare_mod_game_sources([s for s in actual if not s.startswith("<gen>/")], mod_want, name)
    for entry in compdb:
        if entry["file"].endswith("/zigbuild/dso_handle.c"):
            includes = {arg[2:] for arg in entry["arguments"] if arg.startswith("-I")}
            check(str(Path(manifest["root"]) / "include") in includes,
                  "DSO storage compile lacks the include directory required by the forced compatibility header")
    checked = 0
    for suffix in ("", "64", "64w"):
        for base in ("p4port", "p4portee"):
            name = base + suffix
            if name not in targets:
                continue
            target = targets[name]
            want = list(expected[base])
            if suffix == "64w":
                want = ["src/platform/os/os_win32.c" if w == "src/platform/os/os_posix.c" else w for w in want]
            have = [relative(s, manifest) for s in target["sources"]]
            check(have == want, f"{name}: sources differ from cmake/sources:\n  graph only: {sorted(set(have) - set(want))}\n"
                                f"  cmake only: {sorted(set(want) - set(have))}\n  order equal: {sorted(have) == sorted(want)}")
            check(len(target["objects"]) == len(have), f"{name}: {len(target['objects'])} objects for {len(have)} sources")
            for source, obj in zip(target["sources"], target["objects"]):
                check(f"/CMakeFiles/{name}.dir/" in obj.replace("\\", "/"), f"{name}: object {obj} is not in CMakeFiles/{name}.dir")
                if source.endswith(".zig"):
                    check(obj not in by_output, f"{name}: Zig object {obj} has a C compile command")
                    continue
                entry = by_output.get(obj)
                check(entry is not None, f"{name}: no compile command for {source}")
                check(Path(entry["file"]).as_posix() == Path(source).as_posix(), f"{name}: {obj} is compiled from {entry['file']}, not {source}")
                args = entry["arguments"]
                if base == "p4port":
                    check("-DP4_MOD_GAME_BINDINGS=1" in args, f"{name}: {source} lacks the target definition P4_MOD_GAME_BINDINGS=1")
                if source.endswith("third_party/lodepng/lodepng.c"):
                    for d in ("LODEPNG_NO_COMPILE_DISK", "LODEPNG_NO_COMPILE_ENCODER", "LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS", "LODEPNG_NO_COMPILE_ALLOCATORS"):
                        check("-D" + d in args, f"{name}: lodepng.c lacks its source definition {d}")
                elif base == "p4port":
                    check(not any(a.startswith("-DLODEPNG_") for a in args), f"{name}: {source} received lodepng's source-level definitions")
            checked += 1
    check(checked or manifest["port64"], "the profile has neither a p4port target nor the 64-bit ABI manifest")
    if not checked:
        # 64-bit-only profile without the link: the port's C sources are the ABI inputs of the mirror
        listed = set((Path(manifest["gen_dir"]) / "crt-native-abi-files.txt").read_text(encoding="utf-8").split())
        for base in ("p4port", "p4portee"):
            for source in expected[base]:
                if source.endswith(".c") and "third_party/" not in source and not source.startswith("<gen>/"):
                    check(any(p.endswith("/abi/port/" + source + ".abi.json") for p in listed), f"no ABI of {base} source {source}")
    boot = (Path(manifest["gen_dir"]) / "package_boot_providers.c").read_text(encoding="utf-8")
    externs = re.findall(r"^extern const P4PackageHandler (\w+);$", boot, re.M)
    check(externs == providers, f"package boot providers {externs} differ from the fragments' registration order {providers}")
    finals = {t["name"] for t in manifest["targets"] if t["kind"] in ("executable", "shared_library") and t["product"]
              and not t["name"].startswith(("p4_luau", "luau_", "p4mod"))}
    if manifest["android"] and not manifest["android64"]:
        check(finals == {"main"}, f"Android x86 (320's exit) builds {sorted(finals)}, not only libmain.so")
    elif manifest["android64"]:
        check(finals <= {"main", "p4pc_phone"} and "main" in finals, f"64-bit Android (350's exit) builds {sorted(finals)}")
    elif manifest["has_i386"]:
        check({"p4pc", "p4pc_viewer", "p4pc_sfdview"} <= finals, f"desktop i386 profile lacks a final target: {sorted(finals)}")
    print(f"zig graph sources: {checked} object targets equal cmake/sources; {len(providers)} boot providers in order; exits hold")


ADAPTERS = {
    True: ["p4_mod_game", "p4_mod_luau", "p4_mod_core", "p4_luau", "p4_luau_Compiler", "p4_luau_VM", "p4_luau_Ast", "p4_luau_Bytecode", "p4_luau_Common"],
    False: ["p4_mod_game", "p4_mod_core"],
}
CONSUMERS = {"p4pc": "", "p4pc_viewer": "", "main": None, "p4pc64": "64", "p4pc_viewer64": "64", "p4pc_phone": "64",
             "p4pc64w": "64w", "p4pc_viewer64w": "64w"}


def lib_name(path):
    return re.sub(r"^lib|\.a$", "", Path(path).name)


def test_adapters(build):
    manifest, _ = load(build)
    luau_active = manifest["mod_support"] and manifest["luau"]
    targets = {t["name"]: t for t in manifest["targets"]}
    all_suffixes = ("", "64", "64w")
    consumers = 0
    for name, suffix in CONSUMERS.items():
        if name not in targets:
            continue
        if suffix is None:
            suffix = "64" if manifest["android64"] else ""
        t = targets[name]
        registered = manifest["adapters" + (suffix or "32")]
        want = [a + suffix for a in ADAPTERS[luau_active]] if registered else []
        have = [lib_name(p) for p in t["libraries"]]
        check(have == want, f"{name}: links {have}, expected the {suffix or 'i386'} adapters {want}")
        check(t["host_adapters"] == (ADAPTERS[luau_active] if registered else []), f"{name}: recorded adapters {t['host_adapters']}")
        for other in all_suffixes:
            if other != suffix:
                check(not any(h == a + other for h in have for a in ADAPTERS[True] if other), f"{name}: links a {other} adapter")
        check(t["linker_language"] == ("CXX" if registered and luau_active else "C"), f"{name}: links as {t['linker_language']}")
        consumers += 1
    check(consumers, "no final consumer of p4port in this profile")
    for name in ("p4pc_sfdview", "ffmpeg_license_test64w"):
        if name in targets:
            t = targets[name]
            check(not t["libraries"] and not t["host_adapters"] and t["linker_language"] == "C", f"{name}: unrelated target took host adapters")
    luau_tools = 0
    for suffix in all_suffixes:
        for tool in ("p4_luau_check", "luau_sandbox_test", "luau_script_runner"):
            name = tool + suffix
            if name not in targets:
                if manifest["luau"] and manifest["adapters" + (suffix or "32")]:
                    raise Failure(f"the Luau profile {suffix or 'i386'} lacks {name}")
                continue
            libs = [lib_name(p) for p in targets[name]["libraries"]]
            check(all(l.endswith(suffix) and (not suffix or not l.endswith("64w" if suffix == "64" else "\0")) for l in libs), f"{name}: {libs}")
            check(not any(l.startswith("p4_mod_") for l in libs), f"{name}: a Luau tool links mod adapters {libs}")
            check(f"p4_luau{suffix}" in libs, f"{name}: does not link p4_luau{suffix}")
            luau_tools += 1
    abi_checked = False
    if manifest["port64"]:
        listed = (Path(manifest["gen_dir"]) / "crt-native-abi-files.txt").read_text(encoding="utf-8").split()
        check(all("/../" not in p for p in listed), "a native ABI path escapes its directory")
        check(any(p.endswith("/abi/generated/package_boot_providers.c.abi.json") for p in listed), "no generated package boot ABI")
        check(any(p.endswith("/abi/port/zig-exports.abi.json") for p in listed), "no Zig export ABI")
        check(any(p.endswith("/abi/port/src/crt.c.abi.json") for p in listed), "no port C ABI (src/crt.c)")
        check(not any("third_party/" in p for p in listed), "a third_party source has an ABI entry")
        abi_checked = True
    print(f"zig graph adapters: {consumers} consumers, {luau_tools} Luau tools, native ABI manifest {'checked' if abi_checked else 'not built in this profile'}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("sources", "adapters", "profiles"))
    parser.add_argument("--build", required=True, type=Path)
    parser.add_argument("--cmake-cache", type=Path, help="compare profiles with this actual qualified CMake cache")
    a = parser.parse_args()
    try:
        if a.mode == "profiles":
            test_profiles(a.build.resolve(), a.cmake_cache)
        else:
            {"sources": test_sources, "adapters": test_adapters}[a.mode](a.build.resolve())
    except Failure as exc:
        print(f"zig graph {a.mode}: FAIL: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
