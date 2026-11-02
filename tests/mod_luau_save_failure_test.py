"""Require the post-commit Lua failure to reach the native recovery owner."""
import subprocess
import sys

args = sys.argv[1:]
try:
    boundary = args.index("--driver-end")
    driver, root = args[:boundary], args[boundary + 1]
except (ValueError, IndexError):
    raise SystemExit("missing driver or fixture root")
if not driver:
    raise SystemExit("missing executable")
result = subprocess.run([*driver, "--callback-failure", root], timeout=45, check=False)
if result.returncode != 42:
    raise SystemExit(f"native recovery owner was not reached (exit {result.returncode})")
print("Lua load callback failure reached native recovery owner after commit")
