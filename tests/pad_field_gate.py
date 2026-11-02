#!/usr/bin/env python3
"""Exercise the SDL virtual gamepad -> libpad -> retail field movement path."""
import csv
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    iso, elf = os.environ.get("P4PC_ISO"), os.environ.get("P4PC_ELF")
    if not iso or not elf:
        print("pad_field_gate: P4PC_ISO/P4PC_ELF unset, skipped")
        return 77
    seed = Path(elf).resolve().parent / "saves" / "story0415"
    if not seed.is_dir():
        print(f"pad_field_gate: missing {seed}, skipped")
        return 77
    prog = str(Path(sys.argv[1]).resolve())
    elf = str(Path(elf).resolve())
    cases = (("full", "0,-32768,2000,0"), ("rim", "0,-32075,2000,0"), ("dpad", "0,0,0,1"))
    case_timeout = float(os.environ.get("P4PC_GATE_TIMEOUT", "1500")) / len(cases)
    with tempfile.TemporaryDirectory(prefix="p4pc_pad_field_") as scratch:
        retained_root = os.environ.get("P4PC_SCENE_ARTIFACTS") if not os.environ.get("P4PC_PAD_GATE_OUT") else None
        if retained_root:
            Path(retained_root).mkdir(parents=True, exist_ok=True)
            root = Path(tempfile.mkdtemp(prefix="pad_field.", dir=retained_root)).resolve()
        else:
            root = Path(os.environ.get("P4PC_PAD_GATE_OUT", scratch)).resolve()
        try:
            for name, values in cases:
                work = root / name
                work.mkdir(parents=True, exist_ok=True)
                shutil.copytree(seed, work / "card" / "mc0", dirs_exist_ok=True)
                walk = work / "load.walk"
                walk.write_text("timeout:90000\nwait:140\npress:cross\nwait:140\npress:cross\nwait:140\npress:cross\nskipmsg\nwait:90000\n")
                trace = work / "pad.csv"
                env = {k: v for k, v in os.environ.items() if not k.startswith("P4PC_")}
                temporary = work / "tmp"
                temporary.mkdir(parents=True, exist_ok=True)
                env.update(TMPDIR=str(temporary), TMP=str(temporary), TEMP=str(temporary))
                env.update(P4PC_INPUT="virtual", P4PC_CONFIG="none", P4PC_ISO=iso, P4PC_HIDDEN_WINDOW="1", SDL_AUDIODRIVER="dummy",
                           SDL_VIDEODRIVER="windows" if os.name == "nt" else os.environ.get("SDL_VIDEODRIVER", "offscreen"),
                           P4PC_SAVE_DIR=str(work / "card"), P4PC_TRAP_CONTINUE="1",
                           P4PC_AUTOPRESS_LOAD="0", P4PC_AUTOPRESS_FRAMES="1",
                           P4PC_WALK_SCRIPT=str(walk), P4PC_PAD_VIRTUAL=f"3000,{values},1000",
                           P4PC_PAD_TRACE=str(trace), P4PC_NEWGAME_GATE="4200,7:2,4/15")
                if "--paced" not in sys.argv:
                    env["P4PC_TURBO"] = "2"  # input/guest-state witness, not an image gate
                    if "P4PC_VIRTUAL_CLOCK" in os.environ:
                        env["P4PC_VIRTUAL_CLOCK"] = os.environ["P4PC_VIRTUAL_CLOCK"]
                with (work / "boot.log").open("w") as log:
                    result = subprocess.run([prog, elf], cwd=work, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=case_timeout)
                if result.returncode:
                    print((work / "boot.log").read_text(errors="replace")[-6000:])
                    raise AssertionError(f"{name}: process exited {result.returncode}")
                with trace.open(newline="") as stream:
                    trace_rows = list(csv.DictReader(stream))
                    rows = [r for r in trace_rows if 3100 <= int(r["flip"]) <= 4100]
                    active_rows = [r for r in trace_rows if 3000 <= int(r["flip"]) <= 4100]
                assert len(active_rows) >= 1101, (name, "missing activation-boundary reads", len(active_rows))
                for r in active_rows:
                    assert int(r["ly"]) == 0, (name, "stale SDL state at or after START", r)
                    assert int(r["raw_ly"]) == (-32768 if name == "full" else -32075 if name == "rim" else 0), (name, r)
                    if name != "dpad":
                        report = (int(r["flip"]) - 3000 + 1) * 1000 // 30 - 1
                        assert int(r["raw_lx"]) == (report % 3 - 1) * 2000, (name, "nondeterministic SDL jitter phase", r)
                assert len(rows) >= 1001, (name, "missing frame samples", len(rows))
                assert all(int(b["frame"]) == int(a["frame"]) + 1 for a, b in zip(rows, rows[1:])), (name, "missing game-frame read")
                for r in rows:
                    assert int(r["mode"]) == 0x79 and int(r["ly"]) == 0, (name, r)
                    assert int(r["matrix"]) and int(r["event"]) == 0, (name, "no field control", r)
                    if name == "dpad":
                        assert int(r["buttons"]) & 0x10 == 0, (name, r)
                    else:
                        assert int(r["raw_ly"]) == (-32768 if name == "full" else -32075), (name, r)
                        assert int(r["raw_lx"]) in (-2000, 0, 2000), (name, r)
                speeds = [float(r["speed"]) for r in rows]
                assert min(speeds) > 13.0 and max(speeds) - min(speeds) < 0.0001, (name, "run speed dropped", min(speeds), max(speeds))
                print(f"pad_field_gate: {name}: {len(rows)} frames, LY=0, constant run speed {speeds[0]:.9g}, 1000 Hz frame-clocked SDL reports: PASS")
        except Exception:
            kept = root
            if root == Path(scratch).resolve():
                kept = Path(tempfile.mkdtemp(prefix="p4pc_pad_field_failed_", dir=os.getcwd()))
                shutil.copytree(root, kept, dirs_exist_ok=True)
            print(f"pad_field_gate: failure diagnostics retained in {kept}", flush=True)
            raise
        if retained_root:
            shutil.rmtree(root)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
