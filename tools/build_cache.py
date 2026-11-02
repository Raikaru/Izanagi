#!/usr/bin/env python3
"""Verified content-addressed build outputs; never a test-result cache."""
import contextlib
import errno
import functools
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import time
import uuid
import zlib

try:
    import resource
except ImportError:
    resource = None


class CacheCorruption(RuntimeError):
    pass


def digest(data):
    return hashlib.sha256(data).hexdigest()


def file_digest(path):
    with open(path, "rb") as source:
        checksum = hashlib.sha256()
        for block in iter(lambda: source.read(1024 * 1024), b""):
            checksum.update(block)
    return checksum.hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("ascii")


def enabled():
    return bool(os.environ.get("P4PC_BUILD_CACHE_DIR")) and os.environ.get("P4PC_BUILD_CACHE_DISABLE") != "1"


def _usage():
    if resource is None:
        return {"self_cpu_seconds": time.process_time(), "child_cpu_seconds": None}
    own = resource.getrusage(resource.RUSAGE_SELF)
    children = resource.getrusage(resource.RUSAGE_CHILDREN)
    return {"self_user_seconds": own.ru_utime, "self_system_seconds": own.ru_stime,
            "child_user_seconds": children.ru_utime, "child_system_seconds": children.ru_stime,
            "child_minor_faults": children.ru_minflt, "child_major_faults": children.ru_majflt,
            "child_input_blocks": children.ru_inblock, "child_output_blocks": children.ru_oublock,
            "child_voluntary_switches": children.ru_nvcsw, "child_involuntary_switches": children.ru_nivcsw}


_trace_file = None
_trace_owner = None
_trace_stack = []


@contextlib.contextmanager
def timed(name, **details):
    """Record inclusive wall/rusage deltas. Nested intervals must not be summed."""
    global _trace_file, _trace_owner
    destination = os.environ.get("P4PC_BUILD_TRACE_DIR")
    if not destination:
        yield details
        return
    owner = (os.getpid(), destination)
    if _trace_owner != owner:
        root = Path(destination)
        root.mkdir(parents=True, exist_ok=True)
        _trace_file = root / f"{os.getpid()}-{uuid.uuid4().hex}.jsonl"
        _trace_owner = owner
    identity = uuid.uuid4().hex
    parent = _trace_stack[-1] if _trace_stack else None
    _trace_stack.append(identity)
    started = time.monotonic()
    before = _usage()
    outcome = "failed"
    try:
        yield details
        outcome = "passed" if details.get("returncode", 0) == 0 else "failed"
    finally:
        after = _usage()
        _trace_stack.pop()
        record = {"schema": 1, "id": identity, "parent_interval": parent,
                  "pid": os.getpid(), "ppid": os.getppid(), "pass": name,
                  "profile": os.environ.get("P4PC_BUILD_PROFILE", ""),
                  "unit": os.environ.get("P4PC_BUILD_UNIT", ""),
                  "started_monotonic": started, "wall_seconds": time.monotonic() - started,
                  "outcome": outcome, "details": details,
                  "rusage": {key: after[key] - value if value is not None else None
                             for key, value in before.items()},
                  "accounting": "inclusive; child CPU includes waited-for descendants"}
        with _trace_file.open("a", encoding="utf-8", newline="\n") as output:
            output.write(json.dumps(record, sort_keys=True) + "\n")


@functools.lru_cache(maxsize=None)
def versions(*paths):
    return {str(Path(path).resolve()): file_digest(path) for path in paths}


def compiler_identity(compiler):
    identity = os.environ.get("P4PC_BUILD_TOOLCHAIN_ID", "")
    if enabled() and (not identity.startswith("sha256:") or len(identity) != 71 or
                      any(c not in "0123456789abcdef" for c in identity[7:])):
        raise RuntimeError("build cache needs P4PC_BUILD_TOOLCHAIN_ID from the resolved immutable CI image, not a mutable tag")
    resolved = shutil.which(compiler) or compiler
    return {"image": identity, "compiler": str(Path(resolved).resolve()), "invoked_as": str(compiler)}


def input_files(paths, *, root):
    """Hash bytes and names, including absent optional inputs and added files."""
    root = Path(root).absolute()
    result = {}
    # A symlink's logical name is consumed by mirror paths and include lookup.
    # Resolve bytes when opening it, not names before constructing the key.
    for path in sorted(set(Path(p).absolute() for p in paths)):
        try:
            name = path.relative_to(root).as_posix()
        except ValueError:
            name = str(path)
        result[name] = file_digest(path) if path.is_file() else None
    return result


@contextlib.contextmanager
def _locked(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a+b") as lock:
        if os.name == "nt":
            import msvcrt
            if lock.seek(0, os.SEEK_END) == 0:
                lock.write(b"\0")
                lock.flush()
            deadline = time.monotonic() + 900
            delay = 0.01
            while True:
                try:
                    lock.seek(0)
                    msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
                    break
                except OSError as error:
                    if error.errno not in (errno.EACCES, errno.EAGAIN, errno.EDEADLK) or time.monotonic() >= deadline:
                        raise
                    time.sleep(delay)
                    delay = min(delay * 2, 0.25)
            try:
                yield
            finally:
                lock.seek(0)
                msvcrt.locking(lock.fileno(), msvcrt.LK_UNLCK, 1)
        else:
            import fcntl
            fcntl.flock(lock.fileno(), fcntl.LOCK_EX)
            try:
                yield
            finally:
                fcntl.flock(lock.fileno(), fcntl.LOCK_UN)


def _read(entry, key):
    try:
        if entry.is_symlink() or not entry.is_dir() or {p.name for p in entry.iterdir()} != {"manifest.json", "payload.z"}:
            raise ValueError("unexpected entry members or type")
        manifest_path, payload_path = entry / "manifest.json", entry / "payload.z"
        if manifest_path.is_symlink() or payload_path.is_symlink():
            raise ValueError("symlink member")
        manifest = json.loads(manifest_path.read_bytes())
        if set(manifest) != {"schema", "key", "encoded_sha256", "sha256", "bytes"} or manifest["schema"] != 1 or manifest["key"] != key:
            raise ValueError("manifest identity/schema mismatch")
        encoded = payload_path.read_bytes()
        if digest(encoded) != manifest["encoded_sha256"]:
            raise ValueError("compressed payload hash mismatch")
        data = zlib.decompress(encoded)
        if len(data) != manifest["bytes"] or digest(data) != manifest["sha256"]:
            raise ValueError("decoded payload hash/size mismatch")
        return data
    except (OSError, ValueError, TypeError, KeyError, zlib.error) as error:
        raise CacheCorruption(f"immutable build cache mutation at {entry}: {error}; entry retained, never regenerated") from error


def run_bytes(kind, inputs, producer):
    """Run once per exact request, or return a byte-verified immutable winner.

    inputs may be lazy so the uncached qualification path pays no key cost.
    Failures are never cached. OS-owned per-key locks die with their process;
    no PID guesses, lease daemon, result PASS reuse or scheduler authority.
    """
    with timed(kind) as trace:
        if not enabled():
            trace["cache"] = "disabled"
            return producer()
        request = inputs() if callable(inputs) else inputs
        request = {"schema": 1, "kind": kind, "inputs": request,
                   "engine": versions(__file__), "python": sys.version,
                   "profile": os.environ.get("P4PC_BUILD_PROFILE", "")}
        key = digest(canonical(request))
        root = Path(os.environ["P4PC_BUILD_CACHE_DIR"])
        entry = root / "entries" / key[:2] / key
        trace.update(key=key, entry=str(entry))
        with timed("cache.lock", kind=kind, key=key):
            lock = _locked(root / "locks" / (key + ".lock"))
            lock.__enter__()
        try:
            if entry.exists() or entry.is_symlink():
                trace["cache"] = "hit"
                with timed("cache.verify", kind=kind, key=key):
                    return _read(entry, key)
            trace["cache"] = "miss"
            with timed(kind + ".compute", key=key):
                data = producer()
            if not isinstance(data, bytes):
                raise TypeError("build cache producers must return bytes")
            entry.parent.mkdir(parents=True, exist_ok=True)
            staging = Path(tempfile.mkdtemp(prefix=key + ".", dir=entry.parent))
            try:
                encoded = zlib.compress(data, 1)
                manifest = {"schema": 1, "key": key, "sha256": digest(data),
                            "encoded_sha256": digest(encoded), "bytes": len(data)}
                for name, payload in (("payload.z", encoded), ("manifest.json", canonical(manifest))):
                    with (staging / name).open("xb") as output:
                        output.write(payload)
                        output.flush()
                        os.fsync(output.fileno())
                if entry.exists() or entry.is_symlink():
                    raise CacheCorruption(
                        f"immutable build cache entry appeared under its exclusive lock at {entry}; never replaced")
                os.replace(staging, entry)
                with timed("cache.verify-published", kind=kind, key=key):
                    return _read(entry, key)
            finally:
                if staging.exists():
                    shutil.rmtree(staging)
        finally:
            lock.__exit__(None, None, None)


def main():
    import argparse
    import subprocess
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("run",))
    parser.add_argument("name")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        parser.error("run needs a command")
    with timed(args.name, argv=command) as trace:
        result = subprocess.call(command)
        trace["returncode"] = result
    return result if result >= 0 else 128 - result


if __name__ == "__main__":
    sys.exit(main())
