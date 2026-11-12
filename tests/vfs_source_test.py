"""Read-only early VFS authority tests; synthetic ISO/CVM bytes, no retail data."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from iso_fixture import disc


def main():
    separator = sys.argv.index("--driver-end")
    driver = sys.argv[1:separator]
    if not driver:
        raise SystemExit("supply the native driver before --driver-end")
    for mode in ("good", "disc-change", "developer-change", "absent"):
        with tempfile.TemporaryDirectory(prefix="p4-vfs-source-") as temporary:
            root = Path(temporary)
            image = root / "fixture.iso"
            image.write_bytes(disc({"DATA": {"COMMON.BIN": b"data-base"},
                                    "BTL": {"COMMON.BIN": b"btl-base"},
                                    "ZZZZZ": {"COMMON.BIN": b"other-base"}}))
            for relative, content in (("developer/DATA/COMMON.BIN", b"developer-base"),
                                      ("candidate/DATA/COMMON.BIN", b"lower-candidate"),
                                      ("candidate/DATA/MOD.BIN", b"candidate")):
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(content)
            environment = dict(os.environ)
            if mode == "absent":
                environment.pop("P4PC_ISO", None)
                environment.pop("P4PC_OVERRIDE_DIR", None)
            else:
                environment["P4PC_ISO"] = str(image)
                environment["P4PC_OVERRIDE_DIR"] = str(root / "developer")
            subprocess.run([*driver, mode, str(root)], env=environment, check=True, timeout=30)


if __name__ == "__main__":
    main()
