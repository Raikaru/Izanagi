#!/usr/bin/env python3
"""A failed synchronous execution listener must enter native recovery, not leave
an apparently enabled policy using stale facts. Runs only its own child VM.
"""
import subprocess
import sys


def main(argv):
    runner = argv[argv.index("--") + 1:] if "--" in argv else argv
    if len(runner) < 2:
        return 2
    result = subprocess.run([*runner, "--callback-failure"], capture_output=True, text=True, timeout=45)
    assert result.returncode == 42, (result.returncode, result.stdout, result.stderr)
    assert "execution_callback_recovery:" in result.stderr, result.stderr
    assert "execution deliberate callback failure" in result.stderr, result.stderr
    assert "mod_luau_execution: PASS" not in result.stdout, "continued after a dead observer"
    print("mod_luau_execution_failure: real listener exception enters its native recovery owner, never silent disable")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
