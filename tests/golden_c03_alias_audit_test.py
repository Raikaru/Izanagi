"""Keep the writable C03 view private to its implementation unit."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
owner = root / "src/platform/golden/c03_guard.c"
name = "write" + "_view"
text = owner.read_text(encoding="utf-8")
assert "static unsigned char *" + name + ";" in text
for directory in ("src", "include", "tests", "tools", "config"):
    for path in (root / directory).rglob("*"):
        if not path.is_file() or path == owner or path.suffix not in {".c", ".h", ".py", ".txt"}:
            continue
        if name in path.read_text(encoding="utf-8", errors="replace"):
            raise AssertionError(f"private C03 writable view named outside its owner: {path}")
print("C03 writable view is private to the guard implementation")
