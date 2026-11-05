#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS proof of the API 1.4 game services and
the bundled practice mod (mods/p4pc.practice); not a retail comparison.

Three game runs, each from the menu harness field with the private story0420
card loaded through the memory-card screen:
  services  tests/mod_fixtures/game_services: clock, HUD rows and hides, the
            camera lease, a warp and an encounter launch with its return
  editors   tests/mod_practice_editors.script: the native Config pages of
            P02 date, P03 time slot, P07 money, P08 items, P10 Social Link
            rank and P11 flags, each staged, confirmed and read back
  session   tests/mod_practice_session.script: a clean save, P16/P17 on the
            HUD layer, and the practice session's refused save
  display   tests/mod_practice_display.script: P14 HUD groups and the P13 free
            camera, entered from the Config and moved and exited with the pad
  travel    tests/mod_practice_travel.script: a P01 warp and a P12 encounter
            launched from the Config, fought and returned from
  stats     tests/mod_practice_stats.script: P05 protagonist HP/SP and social
            points, a P06 level change with that level's minimum EXP, and a
            P09 lead Persona stat edit plus an added Persona, with read-back
"""
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


def run(args, env, work, label):
    log = work / (label + ".log")
    with log.open("wb") as output:
        result = subprocess.run([*args.driver, str(Path(env["P4PC_ELF"]).resolve())], cwd=work, env=env,
                                stdout=output, stderr=subprocess.STDOUT, timeout=2400, check=False,
                                creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
    text = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0, f"{label} exited {result.returncode}; see {log}"
    assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
    assert re.search(r"script finished: .* 0 dark, 0 failed writes", text), f"{label} menu script incomplete"
    assert "failed after replacement files" not in text, f"{label} entered mod recovery"
    return text


def services(text):
    lines = [line.split(" ", 2)[2] for line in re.findall(r"SERVICES f=\d+ [^\n]*", text)]
    joined = "\n".join(lines)
    for needed in ("READY ctx=field ready=true", "HUD rows=true", "HIDE field_banner ok=true",
                   "HIDE calendar ok=true", "UNHIDE hidden=false", "CAMERA acquire=true",
                   "second=false why2=this mod already holds the camera", "set=true bad=false input=true",
                   "released=true why=released by its owner owner=nil", "WARP bad=false", "WARP_DONE done arrived",
                   "ENCOUNTER launch ok=true", "ENCOUNTER_DONE done returned outcome=1", "RETURNED ctx=field ready=true", "DONE"):
        assert needed in joined, f"services witness missing: {needed}"
    first, second = (int(n) for n in re.findall(r"CLOCK updates=(\d+)", joined)[:2])
    assert second > first, "completed updates did not advance"
    assert "EVENT battle_end outcome=1" in joined


def editors(text, settings):
    applied = {kind: (int(before), int(after)) for kind, before, after in
               re.findall(r"PRACTICE_APPLY (\w+) [^\n]*?before=(-?\d+) after=(-?\d+)", text)}
    assert set(applied) == {"money", "items", "links", "date", "time", "flags"}, applied
    assert applied["money"][1] == applied["money"][0] + 3000
    assert applied["items"][1] == 5
    assert applied["links"][1] == 3 and applied["links"][0] != 3
    assert applied["date"][1] == applied["date"][0] + 3
    assert applied["time"][1] == (applied["time"][0] + 1) % 6
    assert applied["flags"] == (0, 1)
    for kind in ("money", "items", "links", "date", "time", "flags"):
        assert f"PRACTICE_RESULT {kind} APPLIED" in text
    stored = settings.read_text(encoding="utf-8")
    for key in ("money", "items", "links", "date", "time", "flags"):
        assert f"{key} = on" in stored, f"toggle {key} was not persisted"


def session(text):
    clean = text.split("PRACTICE_SESSION marked", 1)[0]
    assert "SDKMEMCARD_SAVE OK" in clean, "a clean session did not save"
    tainted = text.split("PRACTICE_SESSION marked", 1)[1]
    assert "PRACTICE_TIMER start" in tainted and "PRACTICE_TIMER split" in tainted
    assert "authorize refused (-1): PRACTICE SESSION - SAVING IS DISABLED" in tainted
    assert "SDKMEMCARD_SAVE OK" not in tainted, "a practice session saved"


def display(text):
    for needed in ("PRACTICE_SETTING hud=true", "PRACTICE_SESSION marked by P14", "PRACTICE_SETTING camera=true",
                   "PRACTICE_RESULT camera ACTIVE", "PRACTICE_SESSION marked by P13", "PRACTICE_CAMERA exited"):
        assert needed in text, f"display witness missing: {needed}"


def travel(text):
    for needed in ("PRACTICE_RESULT warp REQUESTED target=7:1", "PRACTICE_REQUEST warp done arrived",
                   "PRACTICE_SESSION marked by P01", "PRACTICE_RESULT encounter REQUESTED",
                   "PRACTICE_SESSION marked by P12"):
        assert needed in text, f"travel witness missing: {needed}"
    assert re.search(r"PRACTICE_REQUEST encounter done returned outcome=[12]", text), "the encounter did not return"


def stats(text):
    for needed in ("PRACTICE_SETTING stats=true", "PRACTICE_SESSION marked by P05"):
        assert needed in text, f"stats witness missing: {needed}"
    for field in ("hp", "sp", "social0"):
        match = re.search(r"PRACTICE_APPLY stats unit=1 %s before=(\d+) after=(\d+)" % field, text)
        assert match, f"stats witness missing the {field} apply"
        before, after = int(match.group(1)), int(match.group(2))
        delta = {"hp": -5, "sp": -3, "social0": 4}[field]
        assert after == before + delta, (field, before, after)
    assert "PRACTICE_SETTING levels=true" in text and "PRACTICE_SESSION marked by P06" in text
    match = re.search(r"PRACTICE_APPLY levels level=(\d+) exp=(\d+) before=LV(\d+) EXP\d+ after=LV(\d+) EXP(\d+)", text)
    assert match, "levels witness missing the apply"
    level, exp, old, new, readback = (int(value) for value in match.groups())
    assert level == old + 3 and new == level and readback == exp, match.group(0)
    assert "PRACTICE_SETTING personas=true" in text and "PRACTICE_SESSION marked by P09" in text
    match = re.search(r"PRACTICE_APPLY personas slot=\d+ stat0 before=(\d+) after=(\d+)", text)
    assert match and int(match.group(2)) == min(int(match.group(1)) + 2, 99), "persona stat apply missing"
    match = re.search(r"PRACTICE_APPLY personas add=\d+ before=(\d+) after=(\d+)", text)
    assert match and int(match.group(2)) == int(match.group(1)) + 1, "persona add missing"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("practice proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("practice proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seed = Path(os.environ.get("P4PC_PRACTICE_SEED", str(elf.parent / "saves/story0420")))
    if not elf.is_file() or not iso.is_file() or not (seed / "BASLUS-21782Save0").is_dir():
        print("mod_practice: retail ELF/ISO or private story0420 seed unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-practice-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        common = dict(os.environ, P4PC_MODS="1", P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1",
                      P4PC_ELF=str(elf.resolve()), P4PC_ISO=str(iso.resolve()), P4PC_HIDDEN_WINDOW="1",
                      P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_VIRTUAL_CLOCK="1",
                      P4PC_SETTINGS_TEST="1", P4PC_START_MENU="7:2", P4PC_MENU_EXIT="1")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            common.pop("SDL_VIDEODRIVER", None)
        else:
            common["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED", "P4PC_TURBO",
                    "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT", "P4PC_WALK_SCRIPT"):
            common.pop(key, None)
        cases = (
            ("services", ROOT / "tests/mod_fixtures", "witness.game-services",
             # The closing Config check is the harness's own pass condition.
             LOAD + ",wait7000,settings-touch:600:24,wait240,settings-check:open",
             {"P4PC_BATTLE_DRIVER": "cross,cross,cross"}),
            ("editors", ROOT / "mods", "p4pc.practice",
             (ROOT / "tests/mod_practice_editors.script").read_text(encoding="utf-8").strip(), {}),
            ("session", ROOT / "mods", "p4pc.practice",
             (ROOT / "tests/mod_practice_session.script").read_text(encoding="utf-8").strip(), {}),
            ("display", ROOT / "mods", "p4pc.practice",
             (ROOT / "tests/mod_practice_display.script").read_text(encoding="utf-8").strip(), {}),
            ("travel", ROOT / "mods", "p4pc.practice",
             (ROOT / "tests/mod_practice_travel.script").read_text(encoding="utf-8").strip(),
             {"P4PC_BATTLE_DRIVER": "cross,cross,cross"}),
            ("stats", ROOT / "mods", "p4pc.practice",
             (ROOT / "tests/mod_practice_stats.script").read_text(encoding="utf-8").strip(), {}),
        )
        # P4PC_PRACTICE_CASES=a,b reruns only those cases.
        selected = [name for name in os.environ.get("P4PC_PRACTICE_CASES", "").split(",") if name]
        for label, mods, enabled, script, extra in cases:
            if selected and label not in selected:
                continue
            case = work / label
            card = case / "card" / "mc0"
            shutil.copytree(seed, card)
            config = case / "p4pc.ini"
            config.write_text("[mods]\nenabled=%s\n" % enabled, encoding="utf-8")
            env = dict(common, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(mods), P4PC_SAVE_DIR=str(card.parent),
                       P4PC_SCENE_ARTIFACTS=str(case), P4PC_MENU_SCRIPT=script, **extra)
            text = run(args, env, case, label)
            if label == "services":
                services(text)
            elif label == "editors":
                editors(text, case / "mod-settings" / "p4pc.practice.ini")
            elif label == "session":
                session(text)
            elif label == "display":
                display(text)
            elif label == "travel":
                travel(text)
            else:
                stats(text)
            print(f"mod_practice_game: {label} PASS")
        outcome = "pass"
        print(f"mod_practice_game: PASS mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
