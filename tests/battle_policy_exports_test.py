"""The production policy object must not expose its test-only adapter API."""
import subprocess
import sys
from pathlib import Path

nm, *objects = sys.argv[1:]
if len(objects) != 1 or not Path(objects[0]).is_file():
    raise SystemExit("expected exactly one built production policy object: " + repr(objects))
result = subprocess.run([nm, "--defined-only", "--extern-only", objects[0]],
                        check=True, capture_output=True, text=True)
symbols = {line.split()[-1].removeprefix("_") for line in result.stdout.splitlines() if line.split()}
if "p4_battle_policy_set_ratings" not in symbols:
    raise SystemExit("production policy export missing from " + objects[0])
leaked = sorted(symbol for symbol in symbols if symbol.startswith("p4pc_test_"))
if leaked:
    raise SystemExit("test-only policy exports in production: " + ", ".join(leaked))
