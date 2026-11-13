#!/usr/bin/env python3
"""The Windows GL host tests, run natively on Windows.

They cannot pass under Wine: WGL has no OpenGL ES context, and SDL's ANGLE fallback is not shipped. So the land check runs them on
the user's PC, on its GPU (tools/local_gates.py --gl), from a bundle the land's x64w and win32 shards make from their own CTest
definitions (tools/p4pc_ci_run.sh P4PC_GL_EXPORT):

  python3 tools/wingl.py bundle <x64|x32> <build tree> <bundle dir> [--build] [--toolchain JSON]   (Linux, p4pc-ci image)

A bundle holds:
  - manifest.json: the output of `ctest --show-only=json-v1` with the ABI's filter (FILTERS).
      x64: the Windows x64 twins of the Linux x86_64 tree.
      x32: the Win32 tree's tests.
  - roots.json: the tree and checkout paths that the commands name, and the toolchain that built them.
  - tree/: every .exe that a test runs, at its tree path, with the DLLs beside it.

plan() turns one test into a Windows command:
  - the `cmake -E env` wrappers and the ENVIRONMENT property become the environment, with Wine's own variables dropped;
  - `wine <exe> ...` becomes the bundled exe;
  - a Python driver's `wine` launcher word is removed;
  - the tree's executables come from the bundle, the rest of the tree is the output directory, and the checkout is the local one.
Any other command, or any other absolute path, is refused (Unmappable): tools/local_gates.py fails the run, never runs it some other way.
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

# (-FS: CTest adds a selected test's fixture setups; the Win64 tests require wine64_ready, the Wine prefix bootstrap, which is no GL test)
FILTERS = {"x64": ["-L", "^gl$", "-L", "^port64win$", "-FS", "^wine64_ready$"], "x32": ["-L", "^gl$", "-LE", "scenes"]}
DLL_TARGETS = {"x64": "p4pc64w_dlls", "x32": "p4_dlls"}
WINE_ONLY = frozenset(("WINEDEBUG", "WINEPREFIX", "WINEDLLOVERRIDES", "WINEPATH", "WINEARCH", "WINESERVER"))
ASSIGNMENT = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)=(.*)", re.S)
PYTHON = object()   # argv[0] of a Python driver, replaced by the running interpreter


class Unmappable(ValueError):
    """A test whose command is not a Wine command this module can run natively."""


def properties(test):
    return {row["name"]: row["value"] for row in test.get("properties", [])}


def _base(word):
    return word.replace("\\", "/").rsplit("/", 1)[-1]


def _under(word, root):
    return word == root or word.startswith(root + "/")


def _exe(word, tree):
    return _under(word, tree) and word.endswith(".exe")


def unwrap(command):
    """(the assignments of its leading `cmake -E env` wrappers, nested ones included; the command they run)."""
    command, env = list(command), {}
    while len(command) >= 3 and _base(command[0]) == "cmake" and command[1:3] == ["-E", "env"]:
        command = command[3:]
        while command and ASSIGNMENT.fullmatch(command[0]):
            key, value = ASSIGNMENT.fullmatch(command.pop(0)).groups()
            env[key] = value
    return env, command


def script_gate(test):
    """A game gate labelled only gl: an `sh` script (tools/local_gates.py runs it as it runs the scene gates; nothing here maps it)."""
    command = unwrap(test["command"])[1]
    return bool(command) and _base(command[0]) == "sh"


def native(test, tree, src):
    """The test's command and environment without Wine, in the tree's own paths: (argv, environment, the tree-relative executables
    it runs). argv[0] is PYTHON for a Python driver. Three shapes are mappable: `wine <exe> ...`; the exe itself (a test whose exe is
    an argument of `cmake -E env`, where CTest adds no emulator); a Python driver from the checkout whose launcher words are an exe
    of the tree, with or without one `wine` before it."""
    env, command = unwrap(test["command"])
    for item in properties(test).get("ENVIRONMENT", []):
        match = ASSIGNMENT.fullmatch(item)
        if not match:
            raise Unmappable("environment item %r" % item)
        env[match.group(1)] = match.group(2)
    for key in WINE_ONLY:
        env.pop(key, None)
    if not command:
        raise Unmappable("an empty command")
    head = _base(command[0])
    if head == "wine" and len(command) > 1 and _exe(command[1], tree):
        argv = command[1:]
    elif _exe(command[0], tree):
        argv = command
    elif head in ("python3", "python") and len(command) > 1 and _under(command[1], src):
        words = command[1:]
        launchers = [index for index, word in enumerate(words) if _base(word) == "wine"]
        if len(launchers) > 1 or (launchers and not (launchers[0] + 1 < len(words) and _exe(words[launchers[0] + 1], tree))):
            raise Unmappable("the Python driver's wine launcher does not run an executable of the tree: %s" % " ".join(command))
        for index in reversed(launchers):
            del words[index]
        if not any(_exe(word, tree) for word in words):
            raise Unmappable("the Python driver runs no executable of the tree: %s" % " ".join(command))
        argv = [PYTHON] + words
    else:
        raise Unmappable("%s is neither Wine running an executable of the tree, nor one, nor a Python driver of one" % command[0])
    exes = {word[len(tree) + 1:] for word in argv if word is not PYTHON and _exe(word, tree)}
    return argv, env, exes


def plan(test, roots, bundle, out, checkout, python=sys.executable):
    """One test as a native Windows run: {"argv", "env", "cwd", "timeout", "skip"}; Unmappable if it cannot be run as defined."""
    tree, src = roots["tree"].rstrip("/"), roots["src"].rstrip("/")
    argv, env, _ = native(test, tree, src)
    binary = (Path(bundle) / "tree").as_posix()

    def place(word):
        for root, target in ((tree, None), (src, Path(checkout).as_posix())):
            if _under(word, root):
                rest = word[len(root) + 1:]
                if target is None:
                    target = binary if rest.endswith((".exe", ".dll")) else Path(out).as_posix()
                return target + ("/" + rest if rest else "")
        if word.startswith("/"):
            raise Unmappable("%s is outside the tree and the checkout" % word)
        return word

    def value(word):
        name, sep, rest = word.partition("=")
        return name + sep + place(rest) if sep and not word.startswith("/") else place(word)

    props = properties(test)
    result = {"argv": [python if word is PYTHON else value(word) for word in argv],
              "env": {key: ";".join(place(part) for part in text.split(";")) for key, text in env.items()},
              "cwd": place(props["WORKING_DIRECTORY"]) if props.get("WORKING_DIRECTORY") else Path(out).as_posix(),
              "timeout": float(props.get("TIMEOUT", 1500)),
              "skip": int(props["SKIP_RETURN_CODE"]) if "SKIP_RETURN_CODE" in props else None}
    if not result["cwd"].startswith(Path(out).as_posix()) and not result["cwd"].startswith(binary):
        raise Unmappable("working directory %s is not in the tree" % props.get("WORKING_DIRECTORY"))
    return result


def bundle(arguments):
    """Write a bundle of the tree's GL selection: the manifest, the roots and the executables (built first with --build)."""
    src = Path(__file__).resolve().parents[1].as_posix()
    tree = Path(arguments.tree).resolve().as_posix()
    target = Path(arguments.out)
    shown = subprocess.run(["ctest", "--test-dir", tree, "--show-only=json-v1", *FILTERS[arguments.abi]],
                           capture_output=True, text=True, check=True).stdout
    manifest = json.loads(shown)
    exes, refused, scripts = set(), {}, []
    for test in manifest["tests"]:
        if script_gate(test):
            # (the PC runs it as a scene gate, from the same bundle: its executables travel with the others)
            scripts.append(test["name"])
            exes |= {word[len(tree) + 1:] for word in unwrap(test["command"])[1] if _exe(word, tree)}
            continue
        try:
            exes |= native(test, tree, src)[2]
        except Unmappable as error:
            refused[test["name"]] = str(error)
    if arguments.build:
        targets = sorted({Path(rel).stem for rel in exes} | {DLL_TARGETS[arguments.abi]})
        subprocess.run(["cmake", "--build", tree, "--target", *targets], check=True)
    if target.exists():
        shutil.rmtree(target)
    (target / "tree").mkdir(parents=True)
    for rel in sorted(exes):
        destination = target / "tree" / rel
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(Path(tree) / rel, destination)
    for directory in sorted({Path(rel).parent for rel in exes}):
        for dll in sorted((Path(tree) / directory).glob("*.dll")):
            shutil.copy2(dll, target / "tree" / directory / dll.name)
    (target / "manifest.json").write_text(shown, encoding="utf-8")
    roots = {"abi": arguments.abi, "tree": tree, "src": src, "filter": FILTERS[arguments.abi], "exes": sorted(exes),
             "scripts": scripts, "refused": refused}
    (target / "roots.json").write_text(json.dumps(roots, sort_keys=True, indent=1) + "\n", encoding="utf-8")
    print("wingl: %s bundle: %d tests (%d sh game gates), %d executables, %d not mappable (%s) in %s" % (
        arguments.abi, len(manifest["tests"]), len(scripts), len(exes), len(refused), " ".join(sorted(refused)) or "none", target))
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    make = commands.add_parser("bundle", help="the GL selection of a build tree, with its executables")
    make.add_argument("abi", choices=sorted(FILTERS))
    make.add_argument("tree")
    make.add_argument("out")
    make.add_argument("--build", action="store_true", help="build the executables and their DLL copies first")
    make.set_defaults(handler=bundle)
    arguments = parser.parse_args(argv)
    return arguments.handler(arguments)


if __name__ == "__main__":
    sys.exit(main())
