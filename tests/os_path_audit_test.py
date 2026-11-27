"""Port code reaches the file system only through the UTF-8 host-path layer.

Every path the port builds is UTF-8 (SDL_GetPrefPath, the ISO the player picked, saves, mods, caches). The Windows C library's fopen, stat,
remove, mkdir, opendir and getenv take the ANSI codepage, so a raw call silently reads and writes nothing for a player whose profile folder has a
non-ASCII name. Code uses include/p4pc/hostpath.h (p4_host_fopen, p4_host_stat, p4_host_remove, p4_host_mkdir, p4_host_getenv) and include/p4pc/os.h
(p4_os_rename, p4_os_list_dir, p4_os_fopen_rb, ...). This scan fails on a raw call in src/ unless the file is a developer harness (paths come from
P4PC_* test variables, never from a player's profile) or the line carries a `path-audit-ok` comment saying why it is safe (POSIX-only code).

Usage: os_path_audit_test.py <repository root>
"""
import pathlib
import re
import sys

BANNED = (
    "fopen", "freopen", "fdopen", "stat", "lstat", "fstatat", "remove", "rename", "unlink", "mkdir", "rmdir", "opendir", "readdir",
    "access", "truncate", "open", "creat", "realpath", "getcwd", "_mkdir", "_rmdir", "_access", "_stat", "_open", "_findfirst",
    "CreateFileA", "CreateDirectoryA", "GetFileAttributesA", "MoveFileA", "DeleteFileA", "FindFirstFileA", "LoadLibraryA",
)
CALL = re.compile(r"(?<![\w.>])(" + "|".join(BANNED) + r")\s*\(")
GETENV = re.compile(r'(?<![\w.>])getenv\s*\(\s*"([^"]*)"')
# Environment variables whose value is a file or directory (an ASCII flag is fine through the ANSI getenv).
PATH_VARIABLES = {
    "P4PC_CONFIG", "P4PC_ISO", "P4PC_ELF", "P4PC_SAVE_DIR", "P4PC_OVERRIDE_DIR", "P4PC_MOD_DIR", "P4PC_BUNDLED_MOD_DIR", "P4PC_PACK_DIR",
    "P4PC_MODEL_CACHE", "P4PC_GOLDEN", "P4PC_LOG_FILE", "HOME", "USERPROFILE", "APPDATA", "LOCALAPPDATA", "ProgramFiles", "ProgramFiles(x86)",
}

# Whole directories and files exempt, with the reason. Everything else under src/ must go through the layer.
EXEMPT = {
    "src/platform/os/": "the OS shim itself (and the restart test marker)",
    "src/platform/android/": "Android: the native filesystem is UTF-8",
    "src/viewer/": "developer viewer tool",
    "src/movie/": "developer movie tools (P4PC_* variables)",
    "src/platform/lockstep": "lockstep developer harness (P4PC_LOCKSTEP_* paths)",
    "src/platform/battle_harness.c": "developer harness",
    "src/platform/battle_driver.c": "developer harness",
    "src/platform/menu_harness.c": "developer harness",
    "src/platform/newgame_gate.c": "developer harness",
    "src/platform/state_dump.c": "developer harness",
    "src/platform/capture_state.c": "developer harness",
    "src/platform/title_driver.c": "developer harness",
    "src/platform/shuffle_harness.c": "developer harness",
    "src/platform/kernel_": "developer tracing (P4PC_* paths)",
    "src/platform/guest.c": "developer tracing",
    "src/platform/profile.c": "developer profile output (P4PC_PROFILE)",
    "src/platform/pad/walk_script.c": "developer harness",
    "src/platform/pad/pad_sdl.c": "developer pad trace and walk script (P4PC_PAD_TRACE, P4PC_WALK_SCRIPT)",
    "src/render/": "developer render dumps and gate limits (P4PC_* paths)",
    "src/platform/p4content.c": "developer texture dump",
}


def blank_literals(text: str):
    """(code with comments and string/char contents blanked, set of line numbers that carry a path-audit-ok comment)."""
    out = []
    ok_lines = set()
    i, n, line = 0, len(text), 1
    while i < n:
        c = text[i]
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
            body = text[i:end]
            if "path-audit-ok" in body:
                ok_lines.update(range(line, line + body.count("\n") + 1))
            out.append(re.sub(r"[^\n]", " ", body))
            line += body.count("\n")
            i = end
        elif text.startswith("//", i):
            end = text.find("\n", i)
            end = n if end < 0 else end
            if "path-audit-ok" in text[i:end]:
                ok_lines.add(line)
            out.append(" " * (end - i))
            i = end
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            j = min(j + 1, n)
            out.append(c + " " * (j - i - 2) + c if j - i >= 2 else c)
            i = j
        else:
            out.append(c)
            if c == "\n":
                line += 1
            i += 1
    return "".join(out), ok_lines


def exempt(rel: str) -> bool:
    return any(rel.startswith(prefix) for prefix in EXEMPT)


def main(argv) -> int:
    root = pathlib.Path(argv[1] if len(argv) > 1 else ".").resolve()
    problems = []
    scanned = 0
    for path in sorted((root / "src").rglob("*")):
        if path.suffix not in (".c", ".h", ".cpp", ".cc", ".m", ".mm"):
            continue
        rel = path.relative_to(root).as_posix()
        if exempt(rel):
            continue
        scanned += 1
        raw = path.read_text(encoding="utf-8", errors="replace")
        code, ok_lines = blank_literals(raw)
        for match in CALL.finditer(code):
            line = code.count("\n", 0, match.start()) + 1
            if line not in ok_lines:
                problems.append(f"{rel}:{line}: raw {match.group(1)}() on a host path; use include/p4pc/hostpath.h or include/p4pc/os.h")
        for match in GETENV.finditer(raw):
            line = raw.count("\n", 0, match.start()) + 1
            # a getenv inside a comment is not a call
            if match.group(1) in PATH_VARIABLES and not code[match.start():match.start() + 6] == " " * 6 and line not in ok_lines:
                problems.append(f"{rel}:{line}: getenv({match.group(1)}) is a path; use p4_host_getenv (UTF-8 on Windows)")
    if scanned < 50:
        problems.append(f"only {scanned} source files scanned; wrong repository root {root}?")
    for problem in problems:
        print(problem, file=sys.stderr)
    if problems:
        print(f"os_path_audit: {len(problems)} raw host-path call(s)", file=sys.stderr)
        return 1
    print(f"os_path_audit: {scanned} files clean")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
