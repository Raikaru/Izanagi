#!/usr/bin/env python3
"""Run one Zig-build tooling fixture with CTest's environment and host lock."""
import argparse
import contextlib
import errno
import os
from pathlib import Path
import signal
import subprocess
import sys


@contextlib.contextmanager
def host_lock(path):
    # CTest's host-tooling resource lock serializes all Python fixtures. Keep the
    # same property across independently selected Zig steps and build processes.
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a+b") as lock:
        if os.name == "nt":
            import msvcrt
            import time
            lock.seek(0, os.SEEK_END)
            if lock.tell() == 0:
                lock.write(b"\0")
                lock.flush()
            while True:
                try:
                    lock.seek(0)
                    msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
                    break
                except OSError as exc:
                    if exc.errno not in (errno.EACCES, errno.EAGAIN, errno.EDEADLK):
                        raise
                    time.sleep(0.1)
            try:
                yield
            finally:
                lock.seek(0)
                msvcrt.locking(lock.fileno(), msvcrt.LK_UNLCK, 1)
        else:
            import fcntl
            fcntl.flock(lock, fcntl.LOCK_EX)
            try:
                yield
            finally:
                fcntl.flock(lock, fcntl.LOCK_UN)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    parser.add_argument("--cwd", required=True)
    parser.add_argument("--lock", required=True)
    parser.add_argument("--timeout", type=float, required=True)
    parser.add_argument("--script-input", action="store_true")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.command[:1] == ["--"]:
        args.command.pop(0)
    if not args.command or args.timeout <= 0:
        parser.error("a command and positive timeout are required")
    cwd = Path(args.cwd).resolve()
    cwd.mkdir(parents=True, exist_ok=True)
    lock = Path(args.lock).resolve()
    env = os.environ.copy()
    env.update(P4PC_MODS="0", P4PC_PACKS="0", P4PC_BUNDLED_MOD_DIR="")
    if args.script_input:
        env["P4PC_INPUT"] = "script"
    with host_lock(lock):
        print(f"[tool test] {args.name}", flush=True)
        kwargs = {"cwd": cwd, "env": env}
        if os.name == "nt":
            kwargs["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP
        else:
            kwargs["start_new_session"] = True
        try:
            process = subprocess.Popen(args.command, **kwargs)
        except OSError as exc:
            print(f"{args.name}: could not start: {exc}", file=sys.stderr)
            return 1
        try:
            code = process.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            print(f"{args.name}: timed out after {args.timeout:g}s", file=sys.stderr, flush=True)
            if os.name == "nt":
                try:
                    subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"], check=False)
                except OSError as exc:
                    print(f"{args.name}: could not terminate process tree: {exc}", file=sys.stderr)
                finally:
                    if process.poll() is None:
                        process.kill()
            else:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            process.wait()
            return 1
        if code:
            print(f"{args.name}: failed with exit status {code}", file=sys.stderr)
            return 1
        return 0


if __name__ == "__main__":
    sys.exit(main())
