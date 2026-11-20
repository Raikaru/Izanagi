#!/usr/bin/env python3
"""Run the ROFS lookup test against a disc made entirely from synthetic files."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from iso_fixture import disc


def main():
    files = {
        "DATA": {"ALPHA/SAMPLE.DAT": b"A" * 2048, "BETA/SECOND.DAT": b"B" * 2048},
        "BTL": {"ARENA/ARENA.DAT": b"C" * 2048},
        "BGM": {"MUSIC.DAT": b"D" * 2048},
        "ENV": {"AMBIENT.DAT": b"E" * 2048},
    }
    with tempfile.TemporaryDirectory(prefix="p4pc-rofs-") as directory:
        iso = Path(directory) / "synthetic.iso"
        iso.write_bytes(disc(files))
        env = dict(os.environ, P4PC_ISO=str(iso))
        return subprocess.run(sys.argv[1:], env=env, check=False).returncode


if __name__ == "__main__":
    sys.exit(main())
