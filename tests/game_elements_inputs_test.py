"""Actual Core/VFS inputs for synthetic element fixtures; not native admission."""
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile
from game_skills_test import packed
from iso_fixture import disc


def input_disc(*, effective_ai=False):
    """Actual producer bank layout shared by element and table-source cases."""
    metadata = bytearray(440 * 40)
    metadata[0] = 2
    metadata[11 * 40 + 0x1E] = 8
    elements = bytearray(576 * 2)
    elements[10 * 2] = 1
    elements[11 * 2] = 6
    elements[12 * 2] = 255
    elements[13 * 2] = 128
    banks = {
        "BATTLE/SKILL.TBL": packed((elements, metadata)),
        "BATTLE/UNIT.TBL": packed((bytes(16), bytes(16), bytes(440 * 4))),
        "BATTLE/EFFECT.TBL": packed((bytes(440 * 4),)),
        "BATTLE/MSG.TBL": packed((bytes(16), bytes(576 * 19), bytes(16), bytes(16), bytes(16))),
    }
    if effective_ai:
        # Structurally genuine packed source spans; these opaque synthetic BF
        # payloads do NOT claim executable AI/parser or native-source proof.
        banks["BATTLE/PERSONA.TBL"] = packed((bytes(16),) * 6)
        banks["BATTLE/ENCOUNT.TBL"] = packed((bytes(16),) * 5)
        banks["BATTLE/MODEL.TBL"] = packed((bytes(16),) * 3)
        banks["BATTLE/AICALC.TBL"] = packed(tuple(bytes(164 if i == 8 else 16) for i in range(11)))
    item = struct.pack("<4I", 1536, 0, 0, 0) + bytes(1536 * 68)
    return disc({"DATA": {"INIT/ITEMTBL.BIN": item}, "BTL": banks})


def main():
    separator = sys.argv.index("--driver-end")
    driver = sys.argv[1:separator]
    if not driver:
        raise SystemExit("supply native driver before --driver-end")
    with tempfile.TemporaryDirectory(prefix="p4-elements-inputs-") as temporary:
        image = Path(temporary) / "fixture.iso"
        image.write_bytes(input_disc())
        environment = dict(os.environ)
        environment.update(P4PC_ISO=str(image), P4PC_MODS="0", P4PC_PACKS="0")
        environment.pop("P4PC_OVERRIDE_DIR", None)
        result = subprocess.run(driver, env=environment, timeout=30)
        if result.returncode == 77:
            return 77
        result.check_returncode()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
