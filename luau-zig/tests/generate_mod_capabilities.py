"""Write <out>/p4_capabilities.inc from include/p4pc/p4.d exactly as cmake/sources/052-capabilities.cmake does."""
import pathlib
import re
import sys

HEADER = re.compile(r"--!capability ([a-z][a-z0-9_-]*(?:\.[a-z][a-z0-9_-]*)*)@([1-9][0-9]*)")

fragments = pathlib.Path(sys.argv[1])
entries = []
for path in fragments.glob("*.luau"):
    # file(STRINGS ... LIMIT_COUNT 1): the first non-empty line, without its line ending.
    first = next((line.rstrip("\r") for line in path.read_text(encoding="utf-8").split("\n") if line.rstrip("\r")), "")
    match = HEADER.fullmatch(first)
    if not match or f"{match.group(1)}.luau" != path.name:
        sys.exit(f"include/p4pc/p4.d/{path.name}: the first line must be "
                 "`--!capability <key>@<revision>` naming this file's key (python tools/p4_capabilities.py check)")
    entries.append(f"{match.group(1)} {match.group(2)}")
text = "/* Generated from include/p4pc/p4.d/<key>.luau by cmake/sources/052-capabilities.cmake. */\n"
for entry in sorted(entries):
    key, revision = entry.split(" ")
    text += f'P4_CAPABILITY("{key}", {revision}u)\n'
out = pathlib.Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
(out / "p4_capabilities.inc").write_bytes(text.encode())
