"""Require raising stage modifiers and invalid enemy rewards to reach the
native failure owner without applying an invalid enemy-reward result."""
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
for stage in ("recovery", "chance", "cost", "reward-exp", "reward-yen", "reward-range", "reward-fraction"):
    result = subprocess.run([*driver, "--stage-failure", stage, f"{root}-{stage}"], timeout=60, check=False)
    if result.returncode != 42:
        raise SystemExit(f"p4.{stage} failure did not reach the failure owner safely (exit {result.returncode})")
print("raising modifiers and invalid enemy rewards reach the native failure owner")
