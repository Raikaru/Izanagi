"""Real Core/VFS CPU inputs, synthetic persona banks; no native gameplay proof."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from iso_fixture import disc

PERSONAS = 256


def packed(rows):
    result = bytearray()
    for row in rows:
        result += struct.pack("<I", len(row)) + row
        result += bytes((-len(result)) % 16)
    return result


def main():
    separator = sys.argv.index("--driver-end")
    driver = sys.argv[1:separator]
    if not driver:
        raise SystemExit("supply native driver before --driver-end")
    for mode in ("valid", "bad-metadata", "bad-names"):
        with tempfile.TemporaryDirectory(prefix="p4-personas-inputs-") as temporary:
            root = Path(temporary)
            metadata = bytes([0x55]) * (PERSONAS * 14 - (mode == "bad-metadata"))
            names = bytes([0x4E]) * (PERSONAS * 17 - (mode == "bad-names"))
            banks = {
                "BATTLE/PERSONA.TBL": packed((metadata, bytes([0x41]) * (PERSONAS * 70), bytes(16), bytes(16),
                                              bytes(16), bytes([0xA1]) * (PERSONAS * 4))),
                "BATTLE/MODEL.TBL": packed((bytes(16), bytes(16), bytes([0x4D]) * (PERSONAS * 88))),
                "BATTLE/MSG.TBL": packed((bytes(16), bytes(16), bytes(16), names, bytes(16))),
            }
            image = root / "fixture.iso"
            image.write_bytes(disc({"BTL": banks}))
            environment = dict(os.environ)
            environment["P4PC_ISO"] = str(image)
            environment.pop("P4PC_OVERRIDE_DIR", None)
            result = subprocess.run([*driver, mode], env=environment, timeout=30)
            if result.returncode == 77:
                return 77
            result.check_returncode()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
