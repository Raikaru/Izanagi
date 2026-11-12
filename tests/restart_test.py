#!/usr/bin/env python3
"""Exercise a real executable/image restart; no game, mod or retail input."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time


def main():
    root = Path(tempfile.mkdtemp(prefix="p4pc_restart_"))
    marker = root / "restart.marker"
    env = dict(os.environ, P4PC_INPUT="script", P4PC_TEST_RESTART=str(marker),
               P4PC_RESTART_ENV_TEST="preserved environment")
    # The host-twin factory can prepend Wine to the actual executable.
    command = sys.argv[1:] + ["with spaces", 'quote"inside', "trailing\\"]
    try:
        with (root / "restart.log").open("wb") as log:
            child = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                assert child.wait(timeout=30) == 0, "first image failed"
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait()
                raise
            deadline = time.monotonic() + 30
            while time.monotonic() < deadline:
                if marker.exists() and marker.read_text().endswith("drained\nsecond\n"):
                    break
                time.sleep(0.02)
            else:
                raise AssertionError("fresh image never confirmed the drained exit")
        print("restart: PASS: real fresh image, preserved argv/environment, old exit drained")
    except BaseException:
        print(f"restart: failure evidence retained in {root}", flush=True)
        raise
    shutil.rmtree(root)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
