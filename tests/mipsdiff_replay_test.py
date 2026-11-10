#!/usr/bin/env python3
"""Exercise the actual replay reader/comparator with synthetic, non-retail records."""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def check_paired(executable, golden, sigs, elf, env, reason=None, device=False):
    mode = ["--paired", "--paired-device", "-G"] if device else ["--paired"]
    result = subprocess.run([executable, *mode, "-R", str(golden), "-S", str(sigs), str(elf), "0044dd18"],
                            env=env, capture_output=True, text=True, timeout=30)
    rows = [line.split("\t") for line in result.stdout.splitlines() if line.startswith("0044dd18\t")]
    expected = [1, 1, 0, 0, 0, 0, 1, 0, 0] if reason else [1, 1, 1, 0, 0, 0, 0, 0, 0]
    if (result.returncode != (1 if reason else 0) or len(rows) != 1 or len(rows[0]) != 13
            or rows[0][2] != ("paired MISMATCH" if reason else "paired exact")
            or list(map(int, rows[0][3:12])) != expected or (reason and reason not in rows[0][12])):
        print(f"mipsdiff paired caller FAILED ({reason or 'exact'}), rc={result.returncode}", file=sys.stderr)
        print(result.stdout, file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        return False
    return True


def patched_body(elf, target, instructions):
    """Patch only a private ELF copy with a synthetic negative-test EE body."""
    data = bytearray(Path(elf).read_bytes())
    phoff, = struct.unpack_from("<I", data, 0x1C)
    phentsize, phnum = struct.unpack_from("<HH", data, 0x2A)
    code = struct.pack("<" + "I" * len(instructions), *instructions)
    for index in range(phnum):
        kind, offset, address, _, size = struct.unpack_from("<IIIII", data, phoff + index * phentsize)
        if kind == 1 and address <= 0x0044DD18 and 0x0044DD18 + len(code) <= address + size:
            start = offset + 0x0044DD18 - address
            data[start:start + len(code)] = code
            target.write_bytes(data)
            return target
    raise RuntimeError("Synthetic paired test body is outside the ELF's loadable file bytes")


def main():
    elf = os.environ.get("P4PC_ELF", "")
    if not elf or not Path(elf).is_file():
        print("mipsdiff replay oracle: skipped (P4PC_ELF not available)")
        return 77
    executable = str(Path(sys.argv[1]).resolve())
    env = os.environ.copy()
    env["P4PC_INPUT"] = "script"
    env["P4_MIPSDIFF_DIR"] = str(Path(__file__).resolve().parents[1] / "tools" / "mipsdiff")
    # Synthetic unpacked 1.0: class=NUMBER, sign=0, exponent=0,
    # fraction=(implicit 1 << 60). This is not captured retail memory.
    before = struct.pack("<iiiiQ", 3, 0, 0, 0, 1 << 60).hex()
    altered = struct.pack("<iiiiQ", 2, 0, 0, 0, 1 << 60).hex()
    # attempts, valid, exact, proven, float-tol, special, mismatch, invalid, inconclusive
    cases = [
        ("complete", {"v0h": 0x3FF00000, "v1": 0xA4}, 0, before, "ok", [1, 1, 1, 0, 0, 0, 0, 0, 0]),
        ("stack-input", {"v0h": 0x3FF00000, "v1": 0xA4}, 0, before, "ok", [1, 1, 1, 0, 0, 0, 0, 0, 0]),
        ("missing-high", {"v1": 0x3FF00000}, 0, before, "inconclusive", [1, 0, 0, 0, 0, 0, 0, 0, 1]),
        ("wrong-high", {"v0h": 0, "v1": 0x3FF00000}, 0, before, "MISMATCH", [1, 1, 0, 0, 0, 0, 1, 0, 0]),
        ("missing-high-wrong-low", {"v1": 0x3FF00000}, 1, before, "MISMATCH", [1, 1, 0, 0, 0, 0, 1, 0, 0]),
        ("missing-high-wrong-memory", {"v1": 0x3FF00000}, 0, altered, "MISMATCH", [1, 1, 0, 0, 0, 0, 1, 0, 0]),
    ]
    with tempfile.TemporaryDirectory(prefix="p4-replay-oracle-") as directory:
        root = Path(directory)
        sigs = root / "sigs.txt"
        sigs.write_text("0044dd18 func_0044dd18 q p\n", encoding="utf-8")
        for name, extra, low, after, status, counts in cases:
            address = 0x01FFF000 if name == "stack-input" else 0x01500000
            kind = "s" if name == "stack-input" else "p"
            record = {"addr": "0x0044dd18", "call": 1, "frame": 1,
                      "sp": "0x01fff000", "args": [address] + [0] * 7,
                      "fargs": [0] * 8, "v0": low, "f0": 0,
                      "win": [-1, -1], "mem": [{"a": address, "k": kind, "pre": before, "post": after}],
                      **extra}
            golden = root / (name + ".jsonl")
            golden.write_text(json.dumps(record, separators=(",", ":")) + "\n", encoding="utf-8")
            for interpreted in (False, True):
                mode = ["-E"] if interpreted else []
                # A deliberately false oracle is rejected as inconclusive when
                # native and independently interpreted retail both contradict it.
                # Direct -E mode exposes the comparator's actual mismatch verdict.
                expected_status = "inconclusive" if not interpreted and status == "MISMATCH" else status
                expected_counts = [1, 0, 0, 0, 0, 0, 0, 0, 1] if expected_status == "inconclusive" else counts
                result = subprocess.run([executable, *mode, "-R", str(golden), "-S", str(sigs), elf, "0044dd18"],
                                        env=env, capture_output=True, text=True, timeout=30)
                rows = [line.split("\t") for line in result.stdout.splitlines() if line.startswith("0044dd18\t")]
                expected_rc = 1 if expected_status == "MISMATCH" else 0
                if (result.returncode != expected_rc or len(rows) != 1 or len(rows[0]) != 13
                        or not rows[0][2].startswith(expected_status) or list(map(int, rows[0][3:12])) != expected_counts):
                    print(f"mipsdiff replay oracle: {name} ({'retail' if interpreted else 'native'}) FAILED, rc={result.returncode}", file=sys.stderr)
                    print(result.stdout, file=sys.stderr)
                    print(result.stderr, file=sys.stderr)
                    return 1
                if expected_status == "inconclusive":
                    reason = "omits bits 32..63" if name == "missing-high" else "interpreted retail code gives the native's answer"
                    if reason not in rows[0][2]:
                        print(f"mipsdiff replay oracle: wrong inconclusive reason: {rows[0][2]}", file=sys.stderr)
                        return 1
            # Paired mode has its own independent EE-body oracle: even a false
            # recorded return/post image cannot be relabelled as golden coverage.
            if not check_paired(executable, golden, sigs, elf, env):
                return 1
        # __pack_d's real native still packs the supplied 1.0. Mutate only the
        # independent interpreted body, proving that each observation can fail.
        ret_one = [0x3C023FF0, 0x0002103C, 0x03E00008, 0]  # v0 = 0x3ff0000000000000
        negative = [
            ("return", [0x3C023FE0, 0x0002103C, 0x03E00008, 0], "ret asm="),
            ("memory", [0xAC800000] + ret_one, "mem[0x01500000]"),
            # Call the actual pinned allocator callback (RwEngineInstance's
            # malloc slot), then return 1.0. Only the EE caller adds this call.
            ("callback", [0x27BDFFF0, 0xAFBF000C, 0x3C080088, 0x8D1973E8,
                          0x24040010, 0x0320F809, 0, 0x8FBF000C, 0x27BD0010] + ret_one,
             "stub calls asm="),
        ]
        record = {"addr": "0x0044dd18", "call": 1, "frame": 1, "sp": "0x01fff000",
                  "args": [0x01500000] + [0] * 7, "fargs": [0] * 8, "v0": 0, "v0h": 0x3FF00000,
                  "f0": 0, "win": [-1, -1],
                  "mem": [{"a": 0x01500000, "k": "p", "pre": before, "post": before}]}
        golden = root / "paired-negative.jsonl"
        golden.write_text(json.dumps(record, separators=(",", ":")) + "\n", encoding="utf-8")
        for name, instructions, reason in negative:
            mutated = patched_body(elf, root / f"paired-{name}.elf", instructions)
            if not check_paired(executable, golden, sigs, mutated, env, reason):
                return 1
        for conflict in ("-E", "-F"):
            result = subprocess.run([executable, "--paired", conflict, "-R", str(golden), elf, "0044dd18"],
                                    env=env, capture_output=True, text=True, timeout=30)
            if result.returncode != 2 or "--paired requires -R" not in result.stderr:
                print(f"mipsdiff paired caller did not reject {conflict}", file=sys.stderr)
                return 1
        result = subprocess.run([executable, "--paired-device", "-R", str(golden), elf, "0044dd18"],
                                env=env, capture_output=True, text=True, timeout=30)
        if result.returncode != 2 or "--paired-device requires --paired" not in result.stderr:
            print("mipsdiff paired device escaped paired-only boundary", file=sys.stderr)
            return 1
        no_device = dict(env, SDL_VIDEODRIVER="p4pc-no-such-video-driver",
                         SDL_AUDIODRIVER="dummy", P4PC_INPUT="script",
                         P4PC_HIDDEN_WINDOW="1", P4PC_CONFIG="none")
        result = subprocess.run([executable, "--paired", "--paired-device", "-G",
                                 "-R", str(golden), "-S", str(sigs), elf, "0044dd18"],
                                env=no_device, capture_output=True, text=True, timeout=30)
        if result.returncode != 1 or "production RW raster device initialization failed" not in result.stdout:
            print("mipsdiff paired device did not fail closed without a real context", file=sys.stderr)
            print(result.stdout, result.stderr, file=sys.stderr)
            return 1
        # The EE caller alone locks a real paletted raster. The expected
        # callback mismatch must survive production CREATE/DESTROY, including
        # palette/pixel frees. Teardown after guest-page rollback corrupts the
        # EE allocator's restored bins and crashes instead of reporting it.
        raster = bytearray(52)
        struct.pack_into("<iii", raster, 12, 2, 2, 8)
        raster[32], raster[35] = 4, 0x25  # texture, PAL8 | 8888
        record["mem"] = [
            {"a": 0x01500000, "k": "p", "pre": before, "post": before},
            {"a": 0x01500040, "k": "p", "pre": raster.hex(), "post": raster.hex()},
            {"a": 0x01500080, "k": "p", "pre": "00000000", "post": "00000000"},
        ]
        golden = root / "paired-device-cleanup.jsonl"
        golden.write_text(json.dumps(record, separators=(",", ":")) + "\n", encoding="utf-8")
        lock_raster = [
            0x27BDFFF0, 0xAFBF000C,  # save return address
            0x3C040150, 0x34840080,  # out = 0x01500080
            0x3C050150, 0x34A50040,  # raster = 0x01500040
            0x24060001,             # rwRASTERLOCKWRITE
            0x3C080088, 0x8D197364,  # production RASTERLOCK callback
            0x0320F809, 0, 0x8FBF000C, 0x27BD0010,
        ]
        mutated = patched_body(elf, root / "paired-device-cleanup.elf", lock_raster + ret_one)
        device_env = dict(env, SDL_VIDEODRIVER="offscreen", SDL_AUDIODRIVER="dummy",
                          P4PC_HIDDEN_WINDOW="1", P4PC_CONFIG="none")
        if not check_paired(executable, golden, sigs, mutated, device_env,
                            "stub calls asm=", device=True):
            return 1
    print(f"mipsdiff replay oracle: {len(cases)} cases x native/retail; paired exact and return/memory/callback negatives passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
