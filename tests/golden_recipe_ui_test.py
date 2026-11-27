#!/usr/bin/env python3
"""Compile and execute the exact twelve-recipe cycling mirror splice."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
marker = "[Golden mode] cycle all twelve-material recipes"
rows = [line.split("\t") for line in (root / "config/mirror_fixes.txt").read_text(encoding="utf-8").splitlines()
        if marker in line]
if len(rows) != 1 or len(rows[0]) < 4:
    raise SystemExit("expected exactly one recipe cycling splice")
replacement = rows[0][2].replace("\\n", "\n")
selected = [line.split("\t") for line in (root / "config/mirror_fixes.txt").read_text(encoding="utf-8").splitlines()
            if "[Golden mode] require the selected result" in line]
if len(selected) != 1 or replacement.count(selected[0][1]) != 1:
    raise SystemExit("recipe result-match splice mismatch")
replacement = replacement.replace(selected[0][1], selected[0][2])
template = (root / "tests/golden_recipe_ui_fixture.c").read_text(encoding="utf-8")
if template.count("/*@@RECIPE_CYCLE@@*/") != 1:
    raise SystemExit("recipe cycling fixture marker mismatch")
with tempfile.TemporaryDirectory(prefix="p4-golden-recipe-ui-") as directory:
    work = Path(directory)
    source = work / "ui.c"
    executable = work / ("ui.exe" if os.name == "nt" else "ui")
    source.write_text(template.replace("/*@@RECIPE_CYCLE@@*/", replacement), encoding="utf-8")
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "clang", "-std=c11", "-I", str(root / "include"),
                    str(source), str(root / "src/platform/golden/golden_recipe_runtime.c"), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
