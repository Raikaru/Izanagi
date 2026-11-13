#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS battle/save proof for p4pc.golden's
derived tables and Social Link scripts; not a retail comparison.

Enables the repository's p4pc.golden package over the user's own disc and own
Persona 4 Golden install. With every default group but random encounters (the
walk is written for PS2's 4F encounters) it plays the castle 5F training walk
from the 4/20 checkpoint (battles against Golden-numbered enemies) and saves,
then reloads that save in a fresh process and compares the live skill records
with the disc's. With every default group it restores the 4/20 castle board,
fights the walk's first encounter from Golden's encounter set and checks the
live ENCOUNT blocks, then loads a 6/6 save for the Social Link Persona skills
and a 09/11 daytime home save for a candidate bike-ride route. Nothing from
either game is written outside the private work directory. The route and its
free-time eligibility still need a real-player hardware witness."""
import argparse
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
import retail_tables

PACKAGE = "p4pc.golden"
# Every group (0x1F): numbers (skill rows, compendium learnsets, items), enemies
# (metadata, affinities), party (Persona base rows, the party block, affinities),
# the EXP-curve span and encounters (weighted groups, formations, selectors,
# treasure groups): thirteen row sets for Steam build 10836033; nine without
# encounters.
EXPECTED_GROUPS = 0x1F
EXPECTED_SETS = 13
TABLE_GROUPS = 0xF
TABLE_SETS = 9
EXPECTED_SKILL_ROWS = 54
# Golden's random formations past PS2's 912 rows (450 for build 10836033).
EXPECTED_FORMATION_ROWS = 1362
LIVE_SKILLS = 0x007644A8  # SKILL.TBL active records, 0x28 bytes each
# ENCOUNT block pointers (include/p4pc/game_encounters.h) by packed span.
LIVE_ENCOUNT = {0: 0x00764504, 1: 0x0076450C, 3: 0x00764508, 4: 0x00764514}
RAM_BASE = 0x00100000  # P4PC_DUMP_RAM starts here (src/platform/state_dump.c)
RAM_FLIP = 7000
# The first encounter of the walk ends at flip 6958 on 4F (field 40:0); the
# encounters leg keeps guest RAM just after it.
ENCOUNTER_FLIP = 7100
ENCOUNTER_GATE = "7300,40:0,4/20"


def check_encounters(ram, iso):
    """The live ENCOUNT blocks are the translated set: Golden's weighted groups name
    formations up to the grown block's last row, each naming only PS2 enemies, and
    every formation row no group names (story and boss rows) and selector 0 are the
    disc's. Returns the highest formation a group names."""
    disc = retail_tables.blocks(retail_tables.battle_files(iso, ["ENCOUNT.TBL"])["ENCOUNT.TBL"])
    live = {span: struct.unpack_from("<I", ram, slot - RAM_BASE)[0] - RAM_BASE
            for span, slot in LIVE_ENCOUNT.items()}
    weighted = ram[live[3]:live[3] + len(disc[3])]
    assert weighted != disc[3], "the live weighted groups are the disc's"
    named = {struct.unpack_from("<H", weighted, row + 8 + entry * 4)[0]
             for row in range(0, len(weighted), 248) for entry in range(60)} - {0}
    assert max(named) + 1 == EXPECTED_FORMATION_ROWS, f"groups name formations up to {max(named)}"
    for formation in sorted(named):
        enemies = struct.unpack_from("<5H", ram, live[0] + formation * 24 + 8)
        assert any(enemies) and all(enemy < 336 for enemy in enemies), \
            f"formation {formation} names enemies {enemies}"
    for formation in range(len(disc[0]) // 24):
        if formation not in named:
            assert ram[live[0] + formation * 24:live[0] + formation * 24 + 24] == \
                disc[0][formation * 24:formation * 24 + 24], f"unnamed formation {formation} changed"
    assert ram[live[1]:live[1] + 10] == disc[1][:10], "selector 0 changed"
    return max(named)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("Golden proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("Golden proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    golden = Path(os.environ.get("P4PC_GOLDEN", ""))
    seed = Path(os.environ.get("P4PC_ENEMY_SEED", str(elf.parent / "saves/story0420_5f")))
    ranks_seed = elf.parent / "saves/playthrough0606postkanji"
    ride_seed = Path(os.environ.get("P4PC_GOLDEN_RIDE_SEED", str(elf.parent / "saves/playthrough0911home")))
    if not elf.is_file() or not iso.is_file() or not all(
            (path / "BASLUS-21782Save0").is_dir() for path in (seed, ranks_seed, ride_seed)):
        print("mod_golden: retail ELF/ISO or private story0420_5f/playthrough0606postkanji/playthrough0911home "
              "seed unavailable", file=sys.stderr)
        return 77
    if not (golden / "data_e.cpk").is_file():
        print("mod_golden: P4PC_GOLDEN does not name a Persona 4 Golden install", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-golden-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        shutil.copytree(ROOT / "mods" / PACKAGE, work / "mods" / PACKAGE)
        card = work / "card" / "mc0"
        shutil.copytree(seed, card)
        config = work / "p4pc.ini"
        config.write_text("[mods]\nenabled=%s\n" % PACKAGE, encoding="utf-8")
        common = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(work / "mods"),
                      P4PC_MODS="1", P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1",
                      P4PC_ISO=str(iso.resolve()), P4PC_GOLDEN=str(golden.resolve()),
                      P4PC_SAVE_DIR=str(card.parent),
                      P4PC_SCENE_ARTIFACTS=str(work), P4PC_HIDDEN_WINDOW="1", P4PC_OFFSCREEN="1",
                      SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2",
                      P4PC_VIRTUAL_CLOCK="1", P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            common.pop("SDL_VIDEODRIVER", None)
        else:
            common["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED",
                    "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT"):
            common.pop(key, None)
        settings = work / "mod-settings"
        settings.mkdir()
        walks = ROOT / "tests/walks"
        def play(label, walk, gate, groups, sets, **extra):
            env = dict(common, P4PC_WALK_SCRIPT=str(walk), P4PC_NEWGAME_GATE=gate, **extra)
            log = work / (label + ".log")
            with log.open("wb") as output:
                result = subprocess.run([*args.driver, str(elf.resolve())], cwd=work, env=env,
                                        stdout=output, stderr=subprocess.STDOUT, timeout=2400, check=False,
                                        creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
            text = log.read_text(encoding="utf-8", errors="replace")
            assert result.returncode == 0, f"{label} exited {result.returncode}; see {log}"
            assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
            assert "table-derived refused" not in text, f"{label}: the package was refused; see {log}"
            derived = re.search(r"p4pc: golden derived from .*: groups 0x([0-9a-f]+), (\d+) row sets, (\d+) rows", text)
            assert derived, f"{label}: the Golden producer did not run"
            assert int(derived.group(1), 16) == groups and int(derived.group(2)) == sets, \
                derived.group(0)
            assert "walk script done" in text, f"{label}: the walk did not finish"
            markers = ["Golden Social Link perk order installed", "Golden Social Link Persona skills installed",
                       "Golden bike rides installed"]
            for marker in markers:
                assert marker in text, f"{label}: missing {marker!r}"
            return text

        # The training walk is written for PS2's 4F encounters, so the battle and reload legs
        # keep PS2's encounter set (encounters = off). The reload leg also keeps guest RAM at
        # flip 7000 (the 4/21 field after the load) to compare the live skill records with the disc's.
        (settings / (PACKAGE + ".ini")).write_text("encounters = off\n", encoding="ascii")
        for label, walk, gate, extra in (
                ("battle", "save0420_5f_to_training.walk", "38000,20:1,4/20", {}),
                ("reload", "entrance_sleep_to_school.walk", "13000,6:6,4/21",
                 {"P4PC_DUMP_RAM": str(work / "ram"), "P4PC_DUMP_RAM_AT": str(RAM_FLIP)})):
            text = play(label, walks / walk, gate, TABLE_GROUPS, TABLE_SETS, **extra)
            assert "SDKMEMCARD_SAVE OK" in text, f"{label}: the save did not complete"
            sidecar = card / "BASLUS-21782Save0" / ".p4mods"
            assert sidecar.is_file() and PACKAGE.encode() in sidecar.read_bytes(), \
                f"{label} save does not require {PACKAGE}"
            (work / (label + ".p4mods-copy")).write_bytes(sidecar.read_bytes())
        # The live SKILL.TBL records (pointer at 0x007644A8, 40 bytes each) differ from the
        # disc's in exactly the Numbers group's skill rows.
        ram = (work / f"ram_{RAM_FLIP}.bin").read_bytes()
        records = retail_tables.blocks(retail_tables.battle_files(iso, ["SKILL.TBL"])["SKILL.TBL"])[1]
        live = struct.unpack_from("<I", ram, LIVE_SKILLS - RAM_BASE)[0] - RAM_BASE
        changed = sum(ram[live + i:live + i + 40] != records[i:i + 40] for i in range(0, len(records), 40))
        assert changed == EXPECTED_SKILL_ROWS, f"{changed} live skill records differ from the disc"
        (work / f"ram_{RAM_FLIP}.bin").unlink()

        # Golden's encounters: a fresh story0420_5f (its castle board is restored from the save
        # against Golden's treasure groups) plays the training walk through its first encounter,
        # a Golden-picked formation on 4F; guest RAM after it holds the translated blocks.
        (settings / (PACKAGE + ".ini")).unlink()
        encounter_card = work / "encounters" / "mc0"
        shutil.copytree(seed, encounter_card)
        lines = (walks / "save0420_5f_to_training.walk").read_text(encoding="ascii").splitlines()
        first = work / "golden_encounter.walk"
        first.write_text("\n".join(lines[:lines.index("battle:off") + 1]) + "\n", encoding="ascii")
        text = play("encounters", first, ENCOUNTER_GATE, EXPECTED_GROUPS, EXPECTED_SETS,
                    P4PC_SAVE_DIR=str(encounter_card.parent), P4PC_DUMP_RAM=str(work / "ram"),
                    P4PC_DUMP_RAM_AT=str(ENCOUNTER_FLIP))
        assert "waittask:battle:off" in text, "encounters: no encounter was fought"
        last = check_encounters((work / f"ram_{ENCOUNTER_FLIP}.bin").read_bytes(), iso)
        print(f"mod_golden_game: encounters live, groups name formations up to {last}")
        (work / f"ram_{ENCOUNTER_FLIP}.bin").unlink()

        # The 6/6 save already has party Social Link ranks: once the loaded player has control,
        # each reached rank's Persona skill is taught or left waiting for a slot.
        ranks_card = work / "ranks" / "mc0"
        shutil.copytree(ranks_seed, ranks_card)
        text = play("ranks", walks / "qol_load_free.walk", "8000", EXPECTED_GROUPS, EXPECTED_SETS,
                    P4PC_SAVE_DIR=str(ranks_card.parent))
        grants = re.findall(r"rank skill (taught|waiting) member=(\d+) rank=(\d+) skill=(\d+)", text)
        assert grants, "ranks: no Social Link Persona skill was taught or left waiting"
        print("mod_golden_game: rank skills " + " ".join(f"{m}:{r}:{s}:{k}" for k, m, r, s in grants))

        # Candidate: the 09/11 home save goes out through the real town map,
        # then approaches Moel. The composed route needs a hardware witness;
        # do not use the 12/05 mid-investigation save as a free-time proof.
        ride_card = work / "rides" / "mc0"
        shutil.copytree(ride_seed, ride_card)
        text = play("rides", walks / "collection0911_home_to_moel.walk", "20000", EXPECTED_GROUPS, EXPECTED_SETS,
                    P4PC_SAVE_DIR=str(ride_card.parent),
                    P4PC_MENU_SCRIPT="walkdone,wait150,select,wait150,settings-check:active-page:lua/p4pc.golden/ride,"
                                     "cross,wait120,cross,wait1200," + ",".join(["cross,wait240"] * 20) + ",wait3600")
        assert "Golden ride skills read" in text, "rides: the ride table was not read from the install"
        opened = re.search(r"ride offer opened riders=(\d+)", text)
        assert opened and int(opened.group(1)) > 0, "rides: no ride was offered at the gas station"
        passed = re.search(r"ride time pass member=(\d+) ok=true", text)
        assert passed, "rides: the ride did not pass the time slot"
        ridden = re.search(r"ride skill (taught|waiting) member=(\d+) skill=(\d+) talk=future", text)
        assert ridden and ridden.group(2) == passed.group(1), "rides: the ride taught no skill"
        done = re.search(r"ride pass done (passed[^\n]*)", text)
        assert done, "rides: the calendar pass did not finish as passed"
        print(f"mod_golden_game: ride member={ridden.group(2)} skill={ridden.group(3)} {ridden.group(1)}; "
              f"{done.group(1)}")
        outcome = "pass"
        print(f"mod_golden_game: PASS mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
