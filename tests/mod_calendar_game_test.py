#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS proof of p4.calendar.pass (calendar.pass@1);
not a retail comparison.

Two runs from the menu harness field, each loading the private card's save
through the memory-card screen (default seed story0420, P4PC_CALENDAR_SEED
overrides). The tests/mod_fixtures/calendar_pass mod passes the time slot from
the first ready field and checks that a second request in flight is refused:
"staged" with the slot the game staged, "computed" after the fixture clears it
(the request stages it itself). Each must end passed with the predicted slot
read back on the same date, the game's own calendar_advance for that change,
and a refused pass from the day's last slot (never across midnight)."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mod_witness

LOAD = ("wait60,cross,wait90,load,wait150,cross,wait250,cross,wait250,cross,wait300,cross,wait400,wait300")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("calendar proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("calendar proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seed = Path(os.environ.get("P4PC_CALENDAR_SEED", str(elf.parent / "saves/story0420")))
    if not elf.is_file() or not iso.is_file() or not (seed / "BASLUS-21782Save0").is_dir():
        print("mod_calendar: retail ELF/ISO or private seed unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-calendar-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        reports = []
        # "staged": the game has staged the next slot (as after every calendar step);
        # "computed": the fixture clears it first, so the request stages it itself.
        for label, clear in (("staged", False), ("computed", True)):
            case = work / label
            card = case / "card" / "mc0"
            shutil.copytree(seed, card)
            config = case / "p4pc.ini"
            config.write_text("[mods]\nenabled=witness.calendar-pass\n", encoding="utf-8")
            if clear:
                (case / "mod-settings").mkdir()
                (case / "mod-settings/witness.calendar-pass.ini").write_text("clear_staging = on\n", encoding="ascii")
            env = dict(os.environ, P4PC_MODS="1", P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1",
                       P4PC_ELF=str(elf.resolve()), P4PC_ISO=str(iso.resolve()), P4PC_HIDDEN_WINDOW="1",
                       P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_VIRTUAL_CLOCK="1",
                       P4PC_SETTINGS_TEST="1", P4PC_START_MENU="7:2", P4PC_MENU_EXIT="1",
                       P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(ROOT / "tests/mod_fixtures"),
                       P4PC_SAVE_DIR=str(card.parent), P4PC_SCENE_ARTIFACTS=str(case),
                       # The harness's own pass condition is the closing Config check, as in mod_practice_game.
                       P4PC_MENU_SCRIPT=LOAD + ",wait7000,settings-open,wait240,settings-check:open")
            if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
                env.pop("SDL_VIDEODRIVER", None)
            else:
                env["SDL_VIDEODRIVER"] = "offscreen"
            for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED", "P4PC_TURBO",
                        "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT", "P4PC_WALK_SCRIPT"):
                env.pop(key, None)
            log = case / "calendar.log"
            with log.open("wb") as output:
                result = subprocess.run([*args.driver, str(elf.resolve())], cwd=case, env=env, stdout=output,
                                        stderr=subprocess.STDOUT, timeout=2400, check=False,
                                        creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
            text = log.read_text(encoding="utf-8", errors="replace")
            assert result.returncode == 0, f"{label} exited {result.returncode}; see {log}"
            assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
            assert "failed after replacement files" not in text, f"{label}: the fixture entered mod recovery"
            ready = re.search(r"CALENDAR f=\d+ READY date=(-?\d+) slot=(\d+) ctx=field ready=true .* "
                              r"(pending=(\d+) pending_date=-?\d+ pending_slot=\d+)", text)
            assert ready, f"{label}: no ready field after the load"
            date, slot, staging = int(ready.group(1)), int(ready.group(2)), ready.group(3)
            assert (int(ready.group(4)) == 0) == clear, f"{label}: unexpected staging {staging}"
            assert re.search(r"CALENDAR f=\d+ PASS ok=true why=nil", text), f"{label}: the pass was refused"
            assert "PASS_AGAIN ok=false why=another scene request is in flight" in text, \
                f"{label}: a second request was not refused"
            predicted = re.search(r"p4pc: mods: calendar pass from date (-?\d+) slot (\d+) to slot (\d+) \(([^)]*)\)", text)
            assert predicted and (int(predicted.group(1)), int(predicted.group(2))) == (date, slot), \
                f"{label}: no predicted slot"
            assert predicted.group(4) == ("computed and staged by the request" if clear else "staged by the game"), \
                predicted.group(0)
            done = re.search(r"CALENDAR f=\d+ PASS_DONE done (passed[^\n]*?) date=(-?\d+) slot=(\d+) ctx=(\w+)", text)
            assert done, f"{label}: the pass did not finish as passed"
            after = (int(done.group(2)), int(done.group(3)))
            assert after == (date, int(predicted.group(3))), (done.group(0), predicted.group(0))
            assert re.search(r"EVENT calendar_advance %d/%d -> %d/%d" % (date, slot, date, after[1]), text), \
                f"{label}: the game's calendar_advance did not report the slot change"
            if after[1] == 5:
                # From the day's last slot the staged next slot is tomorrow: never across midnight.
                assert re.search(r"CALENDAR f=\d+ PASS_MIDNIGHT ok=false why=the next time slot is tomorrow", text), \
                    f"{label}: a pass from the last slot of the day was not refused"
            reports.append(f"{label} {date}/{slot} -> {after[0]}/{after[1]} ({done.group(1)}, ctx={done.group(4)}; "
                           f"predicted {predicted.group(3)}, {predicted.group(4)}; at the request {staging})")
        outcome = "pass"
        print("mod_calendar_game: PASS " + "; ".join(reports) +
              f" mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
