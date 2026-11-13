#!/usr/bin/env python3
"""The Windows game and viewer must not allocate a console at launch."""
import struct
import sys
from pathlib import Path


def check(path):
    with Path(path).open("rb") as executable:
        if executable.read(2) != b"MZ":
            raise ValueError(f"{path}: missing DOS header")
        executable.seek(0x3c)
        pe_offset = struct.unpack("<I", executable.read(4))[0]
        executable.seek(pe_offset)
        if executable.read(4) != b"PE\0\0":
            raise ValueError(f"{path}: missing PE header")
        executable.seek(pe_offset + 24 + 68)
        subsystem = struct.unpack("<H", executable.read(2))[0]
        if subsystem != 2:
            raise ValueError(f"{path}: PE subsystem {subsystem}, expected Windows GUI (2)")
    print(f"{path}: Windows GUI subsystem")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: windows_game_subsystem_test.py GAME VIEWER")
    for executable in sys.argv[1:]:
        check(executable)
