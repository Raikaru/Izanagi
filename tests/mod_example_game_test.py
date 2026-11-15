#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS game/save proof; not a retail comparison."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mod_witness


def generation(slot):
    data = (slot / slot.name).read_bytes()
    sidecar = (slot / ".p4mods").read_bytes()
    marker = (slot / ".p4mods-commit").read_bytes()
    assert len(sidecar) >= 128 and sidecar[:8] == b"P4MODS\0\0"
    assert struct.unpack_from("<I", sidecar, 8)[0] == 1
    assert struct.unpack_from("<III", sidecar, 56) == (1, 1, 1)
    assert struct.unpack_from("<I", sidecar, 68)[0] & 1
    assert struct.unpack_from("<Q", sidecar, 16)[0] == len(data)
    assert sidecar[24:56] == hashlib.sha256(data).digest()
    assert sidecar[-32:] == hashlib.sha256(sidecar[:-32]).digest()
    assert len(marker) == 128 and marker[:8] == b"P4MCMT\0\0"
    assert marker[24:56] == hashlib.sha256(data).digest()
    assert struct.unpack_from("<Q", marker, 56)[0] == len(sidecar)
    assert marker[64:96] == hashlib.sha256(sidecar).digest()
    assert marker[96:] == hashlib.sha256(marker[:96]).digest()
    identity = b"p4pc.example"
    version = b"1.1.0"
    key = b"lua.value"
    owner_len, key_len, schema, flags = struct.unpack_from("<HHII", sidecar, 96)
    payload_len = struct.unpack_from("<Q", sidecar, 112)[0]
    assert (owner_len, key_len, schema, flags, payload_len) == (len(identity), len(key), 1, 1, 17)
    offset = 128
    assert sidecar[offset:offset + owner_len] == identity
    offset += owner_len
    assert sidecar[offset:offset + key_len] == key
    offset += key_len
    payload = sidecar[offset:offset + payload_len]
    assert payload[:9] == b"P4LV\x01\0\0\0\x03"
    count = struct.unpack_from("<d", payload, 9)[0]
    assert count > 0 and count.is_integer()
    offset += payload_len
    requirement = struct.pack("<IIHHII", 0, 1, len(identity), len(version), 1, 0) + identity + version
    history = struct.pack("<IHHQ", 1, len(identity), len(version), 0) + identity + version
    assert sidecar[offset:-32] == requirement + history, "example provider requirement/history mismatch"
    return sidecar, int(count)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix), "
                     "or python tools/android_prog.py run p4pc for the debug app on a device")
    android = any(Path(part).name == "android_prog.py" for part in args.driver)
    if android and "P4PC_PHONE_LOCK" not in os.environ:
        raise ValueError("Android example proof requires P4PC_PHONE_LOCK (empty only for a private emulator)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("example proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("example proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seed = Path(os.environ.get("P4PC_EXAMPLE_SEED", str(elf.parent / "saves/story0415")))
    if not elf.is_file() or not iso.is_file() or not (seed / "BASLUS-21782Save0").is_dir():
        print("mod_example: retail ELF/ISO or private story0415 seed unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-example-", dir=args.out)).resolve()
    mod_witness.begin(work)
    # A physical phone is shared: hold its lock (docs/private/android.md) for the whole
    # proof. P4PC_PHONE_LOCK= (empty) is for an emulator nobody else drives.
    lock = Path(os.environ["P4PC_PHONE_LOCK"]) if android else None
    if lock and lock.name:
        try:
            os.close(os.open(lock, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600))
        except FileExistsError:
            print(f"mod_example: the phone is busy ({lock} exists)", file=sys.stderr)
            mod_witness.finish(work, 1, outcome="phone-busy")
            return 1
    else:
        lock = None
    outcome = "failed"
    try:
        card = work / "card" / "mc0"
        shutil.copytree(seed, card)
        config = work / "p4pc.ini"
        config.write_text("[mods]\nenabled=p4pc.example\n", encoding="utf-8")
        note = (ROOT / "examples/mods/p4pc.example/files/DATA/MOD_EXAMPLE.TXT").read_text(encoding="ascii").strip()
        common = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(ROOT / "examples/mods"),
                      P4PC_MODS="1", P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1",
                      P4PC_ISO=str(iso.resolve()), P4PC_SAVE_DIR=str(card.parent),
                      P4PC_SCENE_ARTIFACTS=str(work), P4PC_HIDDEN_WINDOW="1", P4PC_OFFSCREEN="1",
                      SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2",
                      P4PC_VIRTUAL_CLOCK="1", P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0")
        if os.name == "nt" or android or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            common.pop("SDL_VIDEODRIVER", None)
        else:
            common["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED",
                    "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT"):
            common.pop(key, None)
        if android:
            # android_prog pushes the mod root, config, mod-settings and card to the
            # device, copies the card back, and keeps its outputs in this witness root.
            common.pop("P4PC_SCENE_ARTIFACTS")
            common["P4PC_MOD_WITNESS_OUT"] = str(work)
            common.setdefault("P4PC_ANDROID_TIMEOUT", "2100")
        cases = (("save", "save0415_to_home_0417.walk", "110000,7:2,4/17"),
                 ("reload", "save0417_to_school_0418.walk", "18000,6:6,4/18"))
        saved_count = None
        # The reload leg turns the declared Config toggle off through the same
        # mod-settings file the MODS hub writes.
        settings = work / "mod-settings" / "p4pc.example.ini"
        for label, walk, gate in cases:
            if label == "reload":
                settings.parent.mkdir()
                settings.write_text("counting = off\n", encoding="ascii")
            env = dict(common, P4PC_WALK_SCRIPT=str(ROOT / "tests/walks" / walk),
                       P4PC_NEWGAME_GATE=gate)
            log = work / (label + ".log")
            with log.open("wb") as output:
                result = subprocess.run([*args.driver, str(elf.resolve())], cwd=work, env=env,
                                        stdout=output, stderr=subprocess.STDOUT, timeout=2200, check=False,
                                        creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
            text = log.read_text(encoding="utf-8", errors="replace")
            assert result.returncode == 0, f"{label} exited {result.returncode}; see {log}"
            assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
            assert f"EXAMPLE_FILE {note}" in text, "the mod's file layer did not win the VFS read"
            assert "walk script done" in text and "SDKMEMCARD_SAVE OK" in text
            counts = [int(n) for n in re.findall(r"EXAMPLE_TIME_SLOT count=(\d+) value=\d+", text)]
            sidecar, count = generation(card / "BASLUS-21782Save0")
            if label == "save":
                assert "EXAMPLE_READY counter=0 counting=on" in text
                assert counts, "real hook/read/write not observed"
                restored = "EXAMPLE_RESTORED count=0"
                assert restored in text, "vanilla load did not reset the counter"
                after_load = [int(n) for n in re.findall(
                    r"EXAMPLE_TIME_SLOT count=(\d+) value=\d+", text.rsplit(restored, 1)[1])]
                assert after_load == list(range(1, len(after_load) + 1)) and count in after_load
                saved_count = count
            else:
                assert "EXAMPLE_READY counter=0 counting=off" in text
                assert not counts, "the hook counted with its Config toggle off"
                restored = f"EXAMPLE_RESTORED count={saved_count}"
                assert saved_count is not None and restored in text, "saved counter was not restored"
                paused = re.findall(r"EXAMPLE_PAUSED count=(\d+) value=\d+", text.split(restored, 1)[1])
                assert paused and all(int(n) == saved_count for n in paused), "toggle-off hook calls not observed"
                assert count == saved_count, "the restored counter was not saved again"
            (work / (label + ".p4mods-copy")).write_bytes(sidecar)
        outcome = "pass"
        print(f"mod_example_game: PASS mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)
        if lock:
            lock.unlink()


if __name__ == "__main__":
    sys.exit(main())
