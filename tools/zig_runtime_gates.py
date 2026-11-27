#!/usr/bin/env python3
"""Runner and parity check of the runtime gates zig build registers (zigbuild/runtime_gates.zig).

  prepare --binary-dir=DIR --dirs=FILE
      Make the directories CMake makes at configure time (save roots, host work directories).
  run [options] -- COMMAND...
      Run one gate with CTest's semantics: TIMEOUT (the whole process tree dies), SKIP_RETURN_CODE,
      WORKING_DIRECTORY, ENVIRONMENT (in order, last wins), RESOURCE_LOCK and RUN_SERIAL (cross-process
      file locks) and `cmake -E env` assignments. Exit 0 only when the command exits 0: a skip code is
      NOT RUN and fails, as does any missing input. GL-capable gates run only under enforced software
      rendering (Linux, no accessible GPU device, Mesa forced to llvmpipe, no other renderer in the log);
      there is no hardware-GPU path. Every record is non-qualifying: the CTest gates stay the authority.
  show-only --registry=FILE [CTest arguments]
      Answer `ctest --test-dir DIR --show-only=json-v1` from the zig registry (for wine_timeout_test.py).
  parity --zig=FILE --ctest=FILE --source-dir=DIR --binary-dir=DIR [--g64=DIR] [--report=FILE]
      Compare the zig registry with `ctest --show-only=json-v1` of a configured CMake tree, restricted to the
      tests the runtime-gate fragments define (by the CTest backtrace).
"""
import argparse
import contextlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import threading
import time

CTEST_PLACEHOLDER = "@ZIG_RUNTIME_GATES_CTEST@"
RETAIL_ENV = ("P4PC_ISO", "P4PC_ELF", "P4PC_REF_DIR")
DOMAIN_FRAGMENT = re.compile(r"^(09\d|1[0-8]\d|19[0-3])-.*\.cmake$|^(210-port64-runtime|220-port64-host|290-port64-host-cases)\.cmake$")


class GateError(Exception):
    """An input or policy refusal: the gate fails without running."""


# ---- cross-process locks ------------------------------------------------------------------------------------------------

if os.name == "nt":
    import ctypes
    from ctypes import wintypes
    import msvcrt

    class _Overlapped(ctypes.Structure):
        _fields_ = [("Internal", ctypes.c_void_p), ("InternalHigh", ctypes.c_void_p), ("Offset", wintypes.DWORD),
                    ("OffsetHigh", wintypes.DWORD), ("hEvent", wintypes.HANDLE)]

    _k32 = ctypes.WinDLL("kernel32", use_last_error=True)
    _k32.LockFileEx.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD,
                                ctypes.POINTER(_Overlapped)]
    _k32.LockFileEx.restype = wintypes.BOOL
    _k32.UnlockFileEx.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD,
                                  ctypes.POINTER(_Overlapped)]
    _k32.UnlockFileEx.restype = wintypes.BOOL

    def _lock(f, exclusive):
        # Byte 0, length 1: the same range msvcrt.locking takes, so other host-tooling holders exclude this one.
        handle = msvcrt.get_osfhandle(f.fileno())
        if not _k32.LockFileEx(handle, 0x2 if exclusive else 0, 0, 1, 0, ctypes.byref(_Overlapped())):
            raise OSError("LockFileEx failed (%d)" % ctypes.get_last_error())

    def _unlock(f):
        handle = msvcrt.get_osfhandle(f.fileno())
        if not _k32.UnlockFileEx(handle, 0, 1, 0, ctypes.byref(_Overlapped())):
            raise OSError("UnlockFileEx failed (%d)" % ctypes.get_last_error())
else:
    import fcntl

    def _lock(f, exclusive):
        fcntl.flock(f, fcntl.LOCK_EX if exclusive else fcntl.LOCK_SH)

    def _unlock(f):
        fcntl.flock(f, fcntl.LOCK_UN)


@contextlib.contextmanager
def file_lock(path, exclusive):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a+b") as f:
        _lock(f, exclusive)
        try:
            yield
        finally:
            _unlock(f)


@contextlib.contextmanager
def gate_locks(lock_dir, resources, run_serial):
    """RUN_SERIAL excludes every other gate of this runner; RESOURCE_LOCK names exclude each other."""
    with contextlib.ExitStack() as stack:
        stack.enter_context(file_lock(lock_dir / "run-serial.lock", run_serial))
        for name in sorted(set(resources)):
            if not re.fullmatch(r"[A-Za-z0-9_.-]+", name):
                raise GateError("unsupported RESOURCE_LOCK name %r" % name)
            stack.enter_context(file_lock(lock_dir / (name + ".lock"), True))
        yield


# ---- process tree -------------------------------------------------------------------------------------------------------

def _pump(stream, sinks):
    for chunk in iter(lambda: stream.read1(65536), b""):
        for sink in sinks:
            sink.write(chunk)
            sink.flush()


def run_direct(cmd, env, cwd, log_path, timeout):
    """Run with output to stderr and the log; on timeout kill the whole tree. Returns (rc or None on timeout, secs)."""
    t0 = time.monotonic()
    kwargs = {}
    if os.name == "nt":
        kwargs["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP
    else:
        kwargs["start_new_session"] = True
    with open(log_path, "wb") as log:
        proc = subprocess.Popen(cmd, cwd=cwd, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, **kwargs)
        pump = threading.Thread(target=_pump, args=(proc.stdout, [sys.stderr.buffer, log]), daemon=True)
        pump.start()
        try:
            rc = proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            rc = None
            if os.name == "nt":
                subprocess.run(["taskkill", "/T", "/F", "/PID", str(proc.pid)], stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL, check=False)
            else:
                with contextlib.suppress(ProcessLookupError):
                    os.killpg(proc.pid, signal.SIGKILL)
            proc.wait()
        pump.join(timeout=30)
    return rc, time.monotonic() - t0


# ---- software-only GL ---------------------------------------------------------------------------------------------------
# Qualifying hardware-GPU gates run only through the CTest manifest and the local gate runner custody; this runner has no
# hardware path. A GL-capable gate runs only where no GPU device can be opened, with Mesa forced to software rendering,
# and fails when its output names any other renderer.

SOFTWARE_GL_ENV = {"LIBGL_ALWAYS_SOFTWARE": "1", "GALLIUM_DRIVER": "llvmpipe", "__GLX_VENDOR_LIBRARY_NAME": "mesa"}
GPU_DEVICE_GLOBS = ("/dev/dri/card*", "/dev/dri/renderD*", "/dev/nvidia*", "/dev/kfd", "/dev/dxg")
SOFTWARE_RENDERERS = ("llvmpipe", "softpipe")
P4GL_LINE = re.compile(rb"^p4gl: [^,\n]*, ([^,\n]*), ")


def software_gl_admission():
    """Refuse unless this host provably cannot open a GPU: Linux, and no GPU device node it may open."""
    if not sys.platform.startswith("linux"):
        raise GateError("software-only GL is enforced on Linux only (no GPU device check exists for %s)" % sys.platform)
    import glob
    reachable = [p for pattern in GPU_DEVICE_GLOBS for p in glob.glob(pattern) if os.access(p, os.R_OK | os.W_OK)]
    if reachable:
        raise GateError("GPU device nodes are accessible (%s): software-only gates refuse a host with a GPU" % ", ".join(sorted(reachable)))


def hardware_renderer_request(env, command):
    values = [env.get("P4PC_GATE_RENDERER")] + [w.split("=", 1)[1] for w in command if w.startswith("P4PC_GATE_RENDERER=")]
    return next((v for v in values if v is not None and not any(s in v for s in SOFTWARE_RENDERERS)), None)


def foreign_renderers(log_path):
    found = []
    for line in Path(log_path).read_bytes().splitlines():
        m = P4GL_LINE.match(line)
        if m:
            renderer = m.group(1).decode("utf-8", "replace")
            if not any(s in renderer.lower() for s in SOFTWARE_RENDERERS):
                found.append(renderer)
    return found

# ---- subcommands --------------------------------------------------------------------------------------------------------

def cmd_prepare(argv):
    ap = argparse.ArgumentParser(prog="zig_runtime_gates.py prepare")
    ap.add_argument("--binary-dir", required=True)
    ap.add_argument("--dirs", required=True)
    a = ap.parse_args(argv)
    root = Path(a.binary_dir)
    if not root.is_absolute():
        raise GateError("--binary-dir must be absolute: %s" % root)
    for line in Path(a.dirs).read_text(encoding="utf-8").splitlines():
        if line:
            if Path(line).is_absolute() or ".." in Path(line).parts:
                raise GateError("configure directory outside the binary directory: %s" % line)
            (root / line).mkdir(parents=True, exist_ok=True)
    return 0


def _assignment(text, what):
    key, eq, value = text.partition("=")
    if not eq or not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", key):
        raise GateError("malformed %s %r" % (what, text))
    return key, value


def _ctest_shim(python, registry, where):
    """An executable that answers CTest's registry listing from the zig manifest."""
    where.mkdir(parents=True, exist_ok=True)
    me = str(Path(__file__).resolve())
    if os.name == "nt":
        shim = where / "ctest-registry.cmd"
        shim.write_text('@"%s" "%s" show-only "--registry=%s" %%*\r\n' % (python, me, registry), encoding="utf-8")
    else:
        shim = where / "ctest-registry"
        quote = lambda s: "'" + s.replace("'", "'\\''") + "'"
        shim.write_text("#!/bin/sh\nexec %s %s show-only %s \"$@\"\n" % (quote(python), quote(me), quote("--registry=" + registry)),
                        encoding="utf-8")
        shim.chmod(0o755)
    return str(shim)


def cmd_run(argv):
    if "--" not in argv:
        raise GateError("run: missing `--` before the command")
    split = argv.index("--")
    opts, command = argv[:split], argv[split + 1:]
    ap = argparse.ArgumentParser(prog="zig_runtime_gates.py run")
    ap.add_argument("--name", required=True)
    ap.add_argument("--timeout", type=float, required=True)
    ap.add_argument("--skip-code", type=int)
    ap.add_argument("--cwd", required=True)
    ap.add_argument("--lock-dir", required=True)
    ap.add_argument("--lock", action="append", default=[])
    ap.add_argument("--run-serial", action="store_true")
    ap.add_argument("--label", action="append", default=[])
    ap.add_argument("--software-gl", action="store_true",
                    help="the gate can open a GL context: admit it only under enforced software rendering")
    ap.add_argument("--retail-iso")
    ap.add_argument("--retail-elf")
    ap.add_argument("--ref-dir")
    ap.add_argument("--env", action="append", default=[])
    ap.add_argument("--cmd-env", action="append", default=[])
    ap.add_argument("--registry")
    ap.add_argument("--result", required=True)
    a = ap.parse_args(opts)
    if not command:
        raise GateError("run: empty command")
    if a.timeout <= 0:
        raise GateError("run: TIMEOUT must be positive")
    result = Path(a.result)
    result.parent.mkdir(parents=True, exist_ok=True)
    log_path = result.with_suffix(".log")
    # Never leave an earlier record standing for this attempt: a refusal or crash must not read as a pass.
    for stale in (result, log_path):
        stale.unlink(missing_ok=True)
    # The CTest gates stay the authority: no record written here qualifies a candidate, a pass included.
    record = {"name": a.name, "command": command, "status": None, "rc": None, "secs": 0.0, "qualifying": False,
              "authority": "ctest", "software_gl": a.software_gl}

    def finish(status, rc=None, secs=0.0, reason=None):
        record.update(status=status, rc=rc, secs=round(secs, 3), reason=reason)
        result.write_text(json.dumps(record, indent=1) + "\n", encoding="utf-8")
        line = "runtime gate %s: %s, non-qualifying" % (a.name, status)
        if reason:
            line += " (%s)" % reason
        print(line, file=sys.stderr, flush=True)
        return 0 if status == "PASS" else 1

    try:
        if not a.software_gl and any(label in ("gl", "mod-witness") for label in a.label):
            raise GateError("a GL-capable gate (label %s) needs --software-gl; this runner has no hardware-GPU path"
                            % "/".join(label for label in a.label if label in ("gl", "mod-witness")))
        if a.software_gl:
            software_gl_admission()
        cwd = Path(a.cwd)
        if not cwd.is_dir():
            raise GateError("WORKING_DIRECTORY %s does not exist" % cwd)
        env = dict(os.environ)
        for key in RETAIL_ENV:
            env.pop(key, None)
        if a.retail_iso or a.retail_elf:
            if not (a.retail_iso and a.retail_elf):
                raise GateError("retail inputs need both the ISO and the ELF")
            for what, path in (("ISO", a.retail_iso), ("ELF", a.retail_elf)):
                if not Path(path).is_file():
                    raise GateError("retail %s %s does not exist" % (what, path))
            env["P4PC_ISO"], env["P4PC_ELF"] = a.retail_iso, a.retail_elf
            if a.ref_dir:
                if not Path(a.ref_dir).is_dir():
                    raise GateError("reference directory %s does not exist" % a.ref_dir)
                env["P4PC_REF_DIR"] = a.ref_dir
        for item in a.env:
            key, value = _assignment(item, "ENVIRONMENT entry")
            env[key] = value
        for item in a.cmd_env:
            key, value = _assignment(item, "`cmake -E env` assignment")
            env[key] = value
        if CTEST_PLACEHOLDER in command:
            if not a.registry or not Path(a.registry).is_file():
                raise GateError("the CTest registry listing needs --registry")
            shim = _ctest_shim(sys.executable, str(Path(a.registry).resolve()), result.parent / (a.name + ".ctest"))
            command = [shim if word == CTEST_PLACEHOLDER else word for word in command]
        if a.software_gl:
            requested = hardware_renderer_request(env, command)
            if requested is not None:
                raise GateError("the gate is judged on hardware renderer %r: only the CTest/local_gates custody path runs it" % requested)
            env.update(SOFTWARE_GL_ENV)
        lock_dir = Path(a.lock_dir)
        print("runtime gate %s: %s" % (a.name, subprocess.list2cmdline(command)), file=sys.stderr, flush=True)
        with gate_locks(lock_dir, a.lock, a.run_serial):
            rc, secs = run_direct(command, env, str(cwd), log_path, a.timeout)
        if a.software_gl:
            foreign = foreign_renderers(log_path)
            if foreign:
                return finish("FAIL", rc, secs, "ran on non-software renderer(s) %s; software-only policy violated" % ", ".join(sorted(set(foreign))))
    except GateError as e:
        return finish("NOT RUN", reason=str(e))
    except OSError as e:
        return finish("NOT RUN", reason="%s: %s" % (type(e).__name__, e))
    if rc is None:
        return finish("TIMEOUT", secs=secs, reason="exceeded %g s" % a.timeout)
    if a.skip_code is not None and rc == a.skip_code:
        return finish("NOT RUN", rc, secs, "exited with SKIP_RETURN_CODE %d; a skipped gate is not a pass" % rc)
    if rc != 0:
        return finish("FAIL", rc, secs, "exit status %d" % rc)
    return finish("PASS", rc, secs)


def cmd_show_only(argv):
    registry = None
    show_only = False
    for word in argv:
        if word.startswith("--registry="):
            registry = word.split("=", 1)[1]
        elif word == "--show-only=json-v1":
            show_only = True
    if registry is None or not show_only:
        raise GateError("show-only answers only `--show-only=json-v1` from --registry")
    sys.stdout.write(Path(registry).read_text(encoding="utf-8"))
    return 0


# ---- parity -------------------------------------------------------------------------------------------------------------

def _frames(data, test):
    graph = data.get("backtraceGraph", {})
    nodes, files = graph.get("nodes", []), graph.get("files", [])
    index = test.get("backtrace")
    while index is not None:
        node = nodes[index]
        yield files[node["file"]]
        index = node.get("parent")


def _norm_path(word, source, binary, g64):
    word = word.replace("\\", "/")
    for root, symbol in ((g64, "${P4_G64}"), (binary, "${CMAKE_BINARY_DIR}"), (source, "${CMAKE_SOURCE_DIR}")):
        if root:
            root = root.replace("\\", "/").rstrip("/")
            word = word.replace(root + "/", symbol + "/")
            if word == root:
                word = symbol
    return word


def _word_matches(zig, ctest):
    if zig == ctest:
        return True
    m = re.fullmatch(r"\$<(?:TARGET_FILE|ZIG_ARTIFACT):([A-Za-z0-9_]+)>", zig)
    base = ctest.rsplit("/", 1)[-1]
    if m:
        return base in (m.group(1), m.group(1) + ".exe")
    tools = {"${CMAKE_COMMAND}": ("cmake", "cmake.exe"), "${CMAKE_CTEST_COMMAND}": ("ctest", "ctest.exe")}
    if zig in tools:
        return base in tools[zig]
    return False


def _props(test):
    return {p["name"]: p["value"] for p in test.get("properties", []) if not p["name"].startswith("_")}


def cmd_parity(argv):
    ap = argparse.ArgumentParser(prog="zig_runtime_gates.py parity")
    ap.add_argument("--zig", required=True)
    ap.add_argument("--ctest", required=True)
    ap.add_argument("--source-dir", required=True)
    ap.add_argument("--binary-dir", required=True, help="the CMake build directory the ctest listing came from")
    ap.add_argument("--g64", help="the CMake tree's ${P4_G64}")
    ap.add_argument("--python", action="append", default=[], help="the CMake tree's ${P4_PYTHON}")
    ap.add_argument("--wine64", action="append", default=[], help="the CMake tree's ${P4_WINE64}")
    ap.add_argument("--report")
    a = ap.parse_args(argv)
    zig = json.loads(Path(a.zig).read_text(encoding="utf-8"))
    ct = json.loads(Path(a.ctest).read_text(encoding="utf-8"))
    header = zig["zigRuntimeGates"]
    zbin = header["binaryDir"]
    ztests = {t["name"]: t for t in zig["tests"]}
    ctests = {}
    for t in ct["tests"]:
        if any(DOMAIN_FRAGMENT.match(Path(f).name) and Path(f).parent.name == "cmake" for f in _frames(ct, t)):
            ctests[t["name"]] = t

    def zword(w):
        if w == header.get("python"):
            return "${P4_PYTHON}"
        if header.get("wine64") is not None and w == header["wine64"]:
            return "${P4_WINE64}"
        return _norm_path(w, None, zbin, None)

    def cword(w):
        if w in a.python:
            return "${P4_PYTHON}"
        if w in a.wine64:
            return "${P4_WINE64}"
        return _norm_path(w, a.source_dir, a.binary_dir, a.g64)

    report = {"missing_in_zig": sorted(set(ctests) - set(ztests)), "extra_in_zig": sorted(set(ztests) - set(ctests)),
              "different": {}}
    for name in sorted(set(ctests) & set(ztests)):
        diffs = []
        zc = [zword(w) for w in ztests[name]["command"]]
        cc = [cword(w) for w in ctests[name]["command"]]
        if len(zc) != len(cc) or not all(_word_matches(z, c) for z, c in zip(zc, cc)):
            diffs.append({"command": {"zig": zc, "ctest": cc}})
        zp, cp = _props(ztests[name]), _props(ctests[name])
        for key in sorted(set(zp) | set(cp)):
            zv, cv = zp.get(key), cp.get(key)
            if isinstance(zv, list):
                zv = [zword(v) for v in zv]
            elif isinstance(zv, str):
                zv = zword(zv)
            if isinstance(cv, list):
                cv = [cword(v) for v in cv]
            elif isinstance(cv, str):
                cv = cword(cv)
            if key in ("TIMEOUT", "SKIP_RETURN_CODE") and zv is not None and cv is not None:
                same = float(zv) == float(cv)
            else:
                same = zv == cv
            if not same:
                diffs.append({key: {"zig": zv, "ctest": cv}})
        if diffs:
            report["different"][name] = diffs
    text = json.dumps(report, indent=1)
    if a.report:
        Path(a.report).write_text(text + "\n", encoding="utf-8")
    print("runtime gate parity: %d CTest tests in scope, %d zig, %d missing in zig, %d extra, %d different"
          % (len(ctests), len(ztests), len(report["missing_in_zig"]), len(report["extra_in_zig"]), len(report["different"])))
    if not a.report:
        print(text)
    return 1 if report["missing_in_zig"] or report["extra_in_zig"] or report["different"] else 0


def main():
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    handlers = {"prepare": cmd_prepare, "run": cmd_run, "show-only": cmd_show_only, "parity": cmd_parity}
    handler = handlers.get(sys.argv[1])
    if handler is None:
        print("unknown subcommand %r\n%s" % (sys.argv[1], __doc__), file=sys.stderr)
        return 2
    try:
        return handler(sys.argv[2:])
    except GateError as e:
        print("zig_runtime_gates: %s" % e, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
