"""Require a raising p4.recovery, p4.chance or p4.cost modifier to reach the
native failure owner, as a raising p4.damage modifier does: each stage pushes
its own hit table before the shared dispatch."""
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
for stage in ("recovery", "chance", "cost"):
    result = subprocess.run([*driver, "--stage-failure", stage, f"{root}-{stage}"], timeout=60, check=False)
    if result.returncode != 42:
        raise SystemExit(f"a raising p4.{stage} modifier did not reach the failure owner (exit {result.returncode})")
print("raising p4.recovery, p4.chance and p4.cost modifiers reach the native failure owner")
