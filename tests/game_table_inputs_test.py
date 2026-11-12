"""Actual CPU table source plans; synthetic caller-owned bytes, no retail data."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from iso_fixture import disc


LABELS = ("INIT/ITEMTBL.BIN", "BATTLE/SKILL.TBL", "BATTLE/UNIT.TBL", "BATTLE/MSG.TBL",
          "BATTLE/PERSONA.TBL", "BATTLE/ENCOUNT.TBL", "BATTLE/EFFECT.TBL", "BATTLE/MODEL.TBL", "BATTLE/AICALC.TBL")
BLOCKS = (1, 2, 3, 5, 6, 5, 1, 3, 11)


def table(kind, tag):
    if kind == 0:
        return struct.pack("<4I", 1536, 0, 0, 0) + f"{tag}-0-0".encode("ascii")
    result = bytearray()
    for block in range(BLOCKS[kind]):
        payload = f"{tag}-{kind}-{block}".encode("ascii")
        result += struct.pack("<I", len(payload)) + payload
        result += bytes((-len(result)) % 16)
    return result

def payloads(*values):
    result = bytearray()
    for value in values:
        result += struct.pack("<I", len(value)) + value
        result += bytes((-len(result)) % 16)
    return result


def main():
    separator = sys.argv.index("--driver-end")
    driver = sys.argv[1:separator]
    if not driver:
        raise SystemExit("supply native driver before --driver-end")
    modes = ["native", "package", "developer", "unqualified", "bad-bank", "oversize", "absent"]
    if os.environ.get("P4PC_TABLE_ADMISSION_BOUNDS_CASES") == "1":
        modes += ["large-file", "large-footprint", "no-disc-package"]
    for mode in modes:
        with tempfile.TemporaryDirectory(prefix="p4-table-inputs-") as temporary:
            root = Path(temporary)
            image = root / "fixture.iso"
            battle = {label: table(kind, "disc") for kind, label in enumerate(LABELS) if kind}
            if mode in ("large-footprint", "developer", "unqualified"):
                battle[LABELS[1]] = payloads(b"R" * 16, b"R" * 16)
            image.write_bytes(disc({"DATA": {LABELS[0]: table(0, "disc")}, "BTL": battle}))
            mods = root / "mods"
            mods.mkdir()
            for folder, priority, version in (("low", 10, "1.0.0"), ("high", 20, "2.0.0")):
                package = mods / folder
                package.mkdir()
                (package / "mod.ini").write_text(
                    f"[mod]\nid=source.{folder}\nname=Source {folder}\nversion={version}\napi=1.1\npriority={priority}\norder=0\n", encoding="ascii")
                path = package / "files/BTL/BATTLE/SKILL.TBL"
                path.parent.mkdir(parents=True)
                data = b"\xff" * 16 if mode == "bad-bank" and folder == "high" else table(1, folder)
                if folder == "high" and mode == "large-file":
                    data = payloads(b"A" * 28, b"B" * 28)
                if folder == "high" and mode == "large-footprint":
                    data = payloads(b"A" * 28, b"B" * 28)
                    assert len(data) == len(battle[LABELS[1]])
                path.write_bytes(data)
                if mode in ("unqualified", "no-disc-package") and folder == "high":
                    path = package / "files/BATTLE/SKILL.TBL"
                    path.parent.mkdir(parents=True)
                    path.write_bytes(table(1, "unqualified"))
            environment = dict(os.environ)
            environment.pop("P4PC_OVERRIDE_DIR", None)
            if mode in ("absent", "no-disc-package"):
                environment.pop("P4PC_ISO", None)
            else:
                environment["P4PC_ISO"] = str(image)
            if mode == "developer":
                developer = root / "developer/BTL/BATTLE/SKILL.TBL"
                developer.parent.mkdir(parents=True)
                developer.write_bytes(table(1, "developer"))
                environment["P4PC_OVERRIDE_DIR"] = str(root / "developer")
            subprocess.run([*driver, mode, str(mods)], env=environment, check=True, timeout=30)


if __name__ == "__main__":
    main()
