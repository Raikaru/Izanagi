#!/usr/bin/env python3
"""Generate the actual SFD unit through the production mirror-fix pass.

Select this unit's real inputs, not substitute getter/setter implementations.
Mod-entry instrumentation is outside this isolated condition-array fixture.
"""
import argparse
from pathlib import Path
import sys
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import gen_port


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("decomp", type=Path)
    parser.add_argument("build", type=Path)
    args = parser.parse_args()
    unit = "src/cri/re4/sfd_set.c"
    destination = args.build / unit
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text((args.decomp / unit).read_text(encoding="utf-8"),
                           encoding="utf-8", newline="\n")
    fixes = [fix for fix in gen_port.read_mirror_fixes() if fix[1] == unit]
    with patch.object(gen_port, "read_mirror_fixes", return_value=fixes):
        gen_port.apply_mirror_fixes(args.build)


if __name__ == "__main__":
    main()
