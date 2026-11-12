import pathlib
import subprocess
import sys
import tempfile
from iso_fixture import disc


def main():
    split = sys.argv.index("--driver-end")
    command = sys.argv[1:split]
    if not command:
        raise SystemExit("supply the native driver before --driver-end")
    with tempfile.TemporaryDirectory(prefix="p4-vfs-origin-") as temporary:
        root = pathlib.Path(temporary)
        image = root / "fixture.iso"
        image.write_bytes(disc({"DATA": {"BARE.TXT": b"disc-bare", "PLAIN.TXT": b"disc-data"}}, {"NOTE.TXT": b"root-note"}))
        files = {
            "low/DATA/LOW.TXT": b"low",
            "low/PLAIN.TXT": b"low-unqualified",
            "low/DATA/CLASH.TXT": b"low",
            "high/DATA/PLAIN.TXT": b"mod-data",
            "high/DATA/CLASH.TXT": b"middle",
            "developer/DATA/CLASH.TXT": b"developer",
        }
        for name, data in files.items():
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        subprocess.run([*command, str(image), str(root)], check=True, timeout=20)


if __name__ == "__main__":
    main()
