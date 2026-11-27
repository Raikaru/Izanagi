"""Build the actual datLevelFromExp body with its configured mirror splices.

This executes behaviour, not a source-text pin. The build's pinned decomp supplies
its existing PS2 static table; no retail or Golden input is read by this step.
"""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
decomp, output = map(Path, sys.argv[1:])
unit = "src/Main/g_data.c"
text = (decomp / unit).read_text(encoding="utf-8")
# This owning TU's following selector is unrelated to the EXP consumer.
text = text[:text.index("extern s8 D_00641E60")]
for row in (root / "config/mirror_fixes.txt").read_text(encoding="utf-8").splitlines():
    parts = row.split("\t")
    if len(parts) < 5 or parts[0] != unit:
        continue
    before, after = (part.replace("\\n", "\n") for part in parts[1:3])
    if text.count(before) != int(parts[4]):
        raise SystemExit(f"EXP fixture splice count differs: {parts[3]}")
    text = text.replace(before, after)
text = text.replace('#include "type.h"',
                    '#include <stdint.h>\ntypedef uint8_t u8;\n'
                    'typedef uint32_t u32;\ntypedef int32_t s32;')
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(text, encoding="utf-8")
