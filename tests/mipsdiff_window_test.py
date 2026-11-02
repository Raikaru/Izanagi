#!/usr/bin/env python3
"""Synthetic PTank calls need the recorded plugin-offset global, in either window layout."""
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile


def main():
    elf = os.environ.get("P4PC_ELF", "")
    if not elf or not Path(elf).is_file():
        print("mipsdiff windows: skipped (P4PC_ELF not available)")
        return 77
    executable = str(Path(sys.argv[1]).resolve())
    env = os.environ.copy()
    env["P4PC_INPUT"] = "script"
    env["P4_MIPSDIFF_DIR"] = str(Path(__file__).resolve().parents[1] / "tools" / "mipsdiff")
    atomic, record, output, particle = 0x01500000, 0x01501000, 0x01503000, 0x01502000
    atomic_bytes = bytearray(256)
    struct.pack_into("<I", atomic_bytes, 0x94, record)
    before = bytearray(0xAC)
    struct.pack_into("<II", before, 0x54, particle, 12)
    struct.pack_into("<I", before, 0xA8, 3)
    after = bytearray(before)
    struct.pack_into("<I", after, 0x3C, 0x40000001)
    # This fixture is hand-built, not a copy of retail memory. The asm reads
    # atomic + W32(gp-0x49f0), fetches selector 1's pair and marks lock flags.
    call = {"addr": "0x003a2770", "call": 1, "frame": 1, "sp": "0x01fff000",
            "args": [atomic, output, 1, 0x40000000, 0, 0, 0, 0], "fargs": [0] * 8,
            "v0": 1, "win": [0, 0], "mem": [
                {"a": atomic, "k": "p", "pre": atomic_bytes.hex(), "post": atomic_bytes.hex()},
                {"a": record, "k": "p", "pre": before.hex(), "post": after.hex()},
                {"a": output, "k": "p", "pre": "00" * 8, "post": struct.pack("<II", particle, 12).hex()}]}
    layouts = [
        ("narrow-rw", [[0x00764000, 4096], [0x008872E0, 544]], False),
        ("wide-rw", [[0x00764000, 4096], [0x00887000, 4096]], False),
        ("wide-rw-spaced", [[0x00764000, 4096], [0x00887000, 4096]], True),
        ("reversed", [[0x008872E0, 544], [0x00764000, 4096]], False),
        ("explicit-fragments", [[0x00764700, 4], [0x008873E8, 20]], False),
    ]
    with tempfile.TemporaryDirectory(prefix="p4-replay-windows-") as directory:
        root = Path(directory)
        golden, sigs, windows = root / "golden.jsonl", root / "sigs.txt", root / "windows.jsonl"
        golden.write_text(json.dumps(call, separators=(",", ":")) + "\n", encoding="utf-8")
        sigs.write_text("003a2770 func_003a2770 i ppii\n", encoding="utf-8")
        count = 0
        for name, ranges, spaced in layouts:
            pieces = []
            for address, length in ranges:
                blob = bytearray(length)
                for location, value in [(0x00764700, 0x94), (0x008873F8, 0x11223344),
                                        *[(0x008873E8 + i * 4, 0x3FFFFFFF) for i in range(4)]]:
                    if address <= location and location + 4 <= address + length:
                        struct.pack_into("<I", blob, location - address, value)
                pieces.append(blob)
            window = {"id": 0, "w": ranges, "hex": b"".join(pieces).hex()}
            windows.write_text(json.dumps(window, separators=None if spaced else (",", ":")) + "\n", encoding="utf-8")
            for interpreted in (False, True):
                command = [executable, *(["-E"] if interpreted else []), "-G", "-R", str(golden),
                           "-W", str(windows), "-S", str(sigs), "-P", "00764700,008873e8,008873f8", elf, "003a2770"]
                result = subprocess.run(command, env=env, capture_output=True, text=True, timeout=30)
                rows = [line.split("\t") for line in result.stdout.splitlines() if line.startswith("003a2770\t")]
                peek = re.search(r"\[00764700\]=([0-9a-f]+).*\[008873e8\]=([0-9a-f]+).*\[008873f8\]=([0-9a-f]+)", result.stderr)
                if (result.returncode != 0 or len(rows) != 1 or len(rows[0]) != 13 or rows[0][2] != "ok"
                        or list(map(int, rows[0][3:12])) != [1, 1, 1, 0, 0, 0, 0, 0, 0]
                        or not peek or int(peek[1], 16) != 0x94 or int(peek[2], 16) == 0x3FFFFFFF
                        or int(peek[3], 16) != 0x11223344):
                    print(f"mipsdiff windows: {name} ({'retail' if interpreted else 'native'}) FAILED, rc={result.returncode}", file=sys.stderr)
                    print(result.stdout, file=sys.stderr)
                    print(result.stderr, file=sys.stderr)
                    return 1
                count += 1
        malformed = [
            {"id": 0, "hex": "00" * 8},
            {"id": 0, "w": [[0x00764700, 4], [0x008873F8, 4]], "hex": "00" * 4},
            {"id": 0, "w": [[0x00764701, 4], [0x008873F8, 4]], "hex": "00" * 8},
        ]
        duplicate = {"id": 0, "w": [[0x00764700, 4], [0x008873F8, 4]], "hex": "00" * 8}
        malformed_lines = [json.dumps(bad) + "\n" for bad in malformed]
        malformed_lines.append((json.dumps(duplicate) + "\n") * 2)
        for bad_lines in malformed_lines:
            windows.write_text(bad_lines, encoding="utf-8")
            result = subprocess.run([executable, "-G", "-R", str(golden), "-W", str(windows), "-S", str(sigs), elf, "003a2770"],
                                    env=env, capture_output=True, text=True, timeout=30)
            if result.returncode != 2 or "invalid recorded window" not in result.stderr:
                print(f"mipsdiff windows: malformed window did not fail loudly, rc={result.returncode}", file=sys.stderr)
                print(result.stdout + result.stderr, file=sys.stderr)
                return 1
            count += 1
    print(f"mipsdiff windows: {count} cases passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
