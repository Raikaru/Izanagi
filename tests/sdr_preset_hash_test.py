"""A synthetic, correctly sized IRX must not become an authenticated preset bank."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def main():
    command = sys.argv[1:]
    if not command:
        raise SystemExit("supply the native sdr_test command")
    with tempfile.TemporaryDirectory(prefix="p4-sdr-preset-hash-") as temporary:
        root = Path(temporary)
        irx = bytearray(30085)  # size metadata only; no retail file bytes
        irx[:4] = b"\x7fELF"
        for mode in range(9):
            # Deliberately synthetic register records, with zero header words.
            struct.pack_into("<32h", irx, 0x4ED4 + mode * 0x44 + 4,
                             *([100 + mode] * 32))
        path = root / "LIB31" / "LIBSD.IRX"
        path.parent.mkdir()
        path.write_bytes(irx)
        environment = dict(os.environ, P4PC_INPUT="script", P4PC_MODS="0", P4PC_PACKS="0")
        environment.pop("P4PC_ISO", None)
        result = subprocess.run([*command, "--bad-preset-hash"], cwd=root,
                                env=environment, capture_output=True, text=True, timeout=20)
        print(result.stdout, end="")
        print(result.stderr, end="", file=sys.stderr)
        if result.returncode != 0:
            raise SystemExit(f"sdr rejection witness failed: {result.returncode}")
        if "LIB31/LIBSD.IRX file SHA-256 mismatch: no reverb" not in result.stderr:
            raise SystemExit("missing explicit IRX authentication refusal")
        print("sdr_preset_hash: wrong identity refused; no active or audible reverb")


if __name__ == "__main__":
    main()
