#!/usr/bin/env python3
"""In-game witness of a randomized run played from New Game into the first dungeon.

Mod-witness lane only (P4PC_MOD_WITNESS=1, P4PC_INPUT=script), never a reference
gate. A cohort of seeds and feature sets (by default one seed with every
feature ON); each run plays:

  story0415  New Game with the randomizer ON, the same scripted route as
             tests/story_gate.sh --make-save (newgame_to_0413.walk: the first
             night, the tutorial fight, 04/12-04/15 with the Persona awakening
             and Shadow Yosuke) and a save at the 04/15 calendar. The run must
             go live at the first field and travel in that save.
  castle_fight That save into Castle 1F (save0415_to_castle_fight.walk): one
             ordinary floor fight (the run's enemies, profiles and drops).
  castle     That randomized save loaded with every setting OFF, then
             save0415_to_home_0417_randomizer.walk (the stock walk with
             battle plans that heal): 04/16-04/17, Yukiko's Castle 1F (a
             generated floor; the planner reaches its stairs) and Castle 2F's
             boss, Shadow Chie (the walk's own plan: Izanagi's Cleave and
             Jiraiya's Garu, both protected stock), to the living room on 04/17.
  school0418 That save through 04/18 (save0417_to_school_0418.walk) to the
             after-school save.
  daidara    That save to Daidara (save0418_to_daidara_sale.walk): sell every
             material, then open Yu's accessory list. The material is a
             disclosed test-only reconstruction: the probe adds one of a
             material whose count-1 unlock gate the run swapped, through the
             game's own inventory setter; the sale, Daidara's unlock check and
             the list are the game's.

Checks against the headless generation of the same run (luau_script_runner),
each only when its feature is in the run: the run code/digest/output hash on
load and its regeneration check; every floor pick names a formation the run's
table holds, some of them ones the original table never held (R5); every enemy
profile the game applied is the generated one and the in-battle readers scale
by it (R6); drops come from the run's drop tables (R7); the R11 starter joins
the roster once from 04/16; each Shuffle Time draws from the run's stream in
award order (R10); the gate reaches 04/17 at home, i.e. Shadow Chie fell; and
Daidara's buy list after the sale offers an item only the run's swapped unlock
gates unlock and not one only the stock gates would (R9; the school0418 and
daidara legs run only with R9). Battle outcomes use the game's own RNG; the
randomizer never draws from it.

The design's early-difficulty cohort is --seeds A,B,C --configs 4095,1029: the
same walks with every feature ON and with only R1, R3 and R11 (starter, loadout
and affinity interactions). Each run prints its battles and the outcome; a
cohort run fails if any run fails, after running them all.

usage: mod_randomizer_dungeon_test.py --out DIR [--seeds HEX32,...] [--configs OPTIONS,...]
                                      --runner RUNNER... --driver PLAYER...
"""
import argparse
import os
import re
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests"))
sys.path.insert(0, str(ROOT / "tools"))
import mod_witness  # noqa: E402
import mod_randomizer_game_test as game  # noqa: E402

# tests/story_gate.sh --make-save's tail, steering into the calendar without the stuck check (the player is
# released inside the save_point area, x 19..188 z -362..-72) and without automatic Cross presses.
SAVE_TAIL = ("# 04/15 evening, free in the living room: the calendar, Save, slot 1 of the fresh card", "timeout:3000",
             "autoskip:off", "push:103,-217,150", "wait:300", "press:cross", "wait:300", "press:cross", "wait:300",
             "press:cross", "wait:300", "press:cross", "wait:900")
STORY_ENV = {"P4PC_AUTOPRESS_NEWGAME": "1560", "P4PC_AUTOPRESS": "cross@2300:2/150*20,start@6500:2/300*14",
             "P4PC_FACING_GATE": "1", "P4PC_BATTLE_FIRST": "down,cross,cross,cross",
             "P4PC_BATTLE_DRIVER": "cross,cross,cross", "P4PC_BATTLE_SKIP": "140"}


def expected_dungeon(lines):
    """The generated and stock floor tables, formations and R6 records."""
    found = {}
    for line in lines:
        key, _, value = line.partition(" ")
        if key in ("EXPECT_ENCOUNT3", "STOCK_ENCOUNT3", "STOCK_ENCOUNT0", "EXPECT_R6", "EXPECT_ITEMTBL", "STOCK_ITEMTBL",
                   "EXPECT_UNIT0", "STOCK_UNIT0"):
            found[key] = bytes.fromhex(value)
        elif key == "EXPECT_R11":
            found["starter"] = None if value == "none" else int(value)
    assert set(found) == {"EXPECT_ENCOUNT3", "STOCK_ENCOUNT3", "STOCK_ENCOUNT0", "EXPECT_R6", "starter", "EXPECT_ITEMTBL",
                          "STOCK_ITEMTBL", "EXPECT_UNIT0", "STOCK_UNIT0"}, sorted(found)
    profiles = {}
    records = found["EXPECT_R6"]
    for at in range(0, len(records), 24):
        table, enemy = int.from_bytes(records[at:at + 2], "little"), int.from_bytes(records[at + 2:at + 4], "little")
        profiles[(table, enemy)] = (int.from_bytes(records[at + 8:at + 10], "little"),
                                    int.from_bytes(records[at + 10:at + 12], "little"))
    return found, profiles


def unlocked(items, sold):
    """Material-gated equipment whose unlock gates the sold counts meet (fclMoonPhaseAtLeast: a count of 0
    always holds); the opening stock (material 0x500) is left out, R9 never moves it."""
    offers = set()
    for item in range(1, 0x2ff):
        row = items[item * 68:item * 68 + 68]
        gates = [int.from_bytes(row[0x3c:0x40], "little"), int.from_bytes(row[0x40:0x44], "little")]
        if (gates[0] >> 8) & 0xfff != 0x500 and int.from_bytes(row[0x34:0x38], "little") and \
                all(not gate & 0xff or sold.get((gate >> 8) & 0xfff, 0) >= gate & 0xff for gate in gates):
            offers.add(item)
    return offers


def table_formations(encount3, table):
    row = encount3[table * 0xf8 + 8:table * 0xf8 + 8 + 240]
    return {int.from_bytes(row[i:i + 2], "little") for i in range(0, 240, 4)} - {0}


def has(expected, feature):
    return bool(expected["options"] >> (feature - 1) & 1)


def story0415(leg, expected):
    run = leg("story0415", expected["settings"])
    walk = run.dir / "newgame_to_0415_save.walk"
    walk.write_text((game.WALKS / "newgame_to_0413.walk").read_text(encoding="utf-8") + "\n".join(SAVE_TAIL) + "\n",
                    encoding="utf-8")
    text = run.walk(str(walk), "160000,7:2,4/15", 3600, extra=STORY_ENV, drop=("P4PC_AUTOPRESS_LOAD",))
    game.need(text, r"RANDO_NEW_GAME armed seed=" + expected["seed"], "the New Game arming")
    run_line = game.need(text, r"RANDO_RUN new_game code=(\S+) digest=([0-9a-f]+) features=" + expected["features"]
                         + r" output=([0-9a-f]+)", "the New Game run").groups()
    assert run_line == (expected["code"], expected["digest"], expected["output"]), (run_line, expected)
    game.need(text, r"RANDO_SAVED code=" + re.escape(expected["code"]), "the 04/15 save carrying the run")
    battles = len(re.findall(r"PROBE_ROSTER battle", text))
    print(f"mod_randomizer_dungeon: story0415: run {expected['code']} live from the first field, {battles} story "
          f"battles played (the tutorial, the awakening, Shadow Yosuke), saved at the 04/15 calendar")
    return run


def castle(leg, expected, dungeon, previous):
    tables, profiles = dungeon
    run = leg("castle", {})
    shutil.copytree(previous.card, run.card)
    text = run.walk("save0415_to_home_0417_randomizer.walk", "110000,7:2,4/17", 3600)
    run_line = game.need(text, r"RANDO_RUN load code=(\S+) digest=([0-9a-f]+) features=" + expected["features"]
                         + r" output=([0-9a-f]+)", "the loaded run").groups()
    assert run_line == (expected["code"], expected["digest"], expected["output"]), (run_line, expected)
    game.need(text, r"RANDO_VERIFY ok code=" + re.escape(expected["code"]), "the background regeneration check")
    picks = [(int(t), int(f)) for t, f, kind in re.findall(r"PROBE_PICK table=(\d+) formation=(\d+) kind=(\d+)", text)
             if int(f) != 0xffff]
    assert picks, "no floor encounter was picked"
    moved = 0
    for table, formation in picks:
        assert formation in table_formations(tables["EXPECT_ENCOUNT3"], table), (table, formation)
        moved += formation not in table_formations(tables["STOCK_ENCOUNT3"], table)
    assert moved or not has(expected, 5), "every picked formation was one the original table held"
    applied = [(int(t), int(e), int(h0), int(h1), int(s0), int(s1)) for t, e, h0, h1, s0, s1 in
               re.findall(r"RANDO_R6 table=(\d+) enemy=(\d+) hp=(\d+)>(\d+) sp=(\d+)>(\d+)", text)]
    assert bool(applied) == has(expected, 6), "R6 profiles applied without R6, or none with it"
    for table, enemy, _, hp, _, sp in applied:
        assert profiles[(table, enemy)] == (hp, sp), (table, enemy, hp, sp, profiles.get((table, enemy)))
    groups = re.findall(r"PROBE_GROUP formation=(\d+)", text)
    fights = len(re.findall(r"PROBE_ROSTER battle", text))
    facts = [f"{len(picks)} floor picks all from the run's tables"
             + (f" ({moved} formations the original table never held)" if has(expected, 5) else " (stock with R5 off)"),
             f"{len(groups)} enemy groups, {fights} battles"]
    if has(expected, 6):
        facts.append(f"{len(applied)} enemies with the run's floor profile")
    # R11: the starter joined the roster once, on the first field from 04/16, and stays.
    starter = tables["starter"]
    grants = re.findall(r"RANDO_R11 (\w+) persona=(\d+) date=(\d+)", text)
    if has(expected, 11):
        assert grants == [("granted", str(starter), grants[0][2])] and int(grants[0][2]) >= 15, grants
        after = re.findall(r"PROBE_ROSTER field id=1 level=\d+ skills=\S+ members=(\S+)",
                           text.split("RANDO_R11 granted")[1])
        assert after and all(str(starter) in members.split(",") for members in after), after[:3]
        facts.append(f"starter {starter:03d} joined the roster on date {grants[0][2]} and stayed")
    else:
        assert not grants, grants
    # R10: each offered Shuffle Time redrew its Persona cards with the run's stream, award by award.
    awards = re.findall(r"RANDO_R10 shuffle-time/([0-9a-f]{16}) cards=(\S+)>(\S+)", text)
    assert [int(ordinal, 16) for ordinal, _, _ in awards] == list(range(len(awards))), awards
    assert has(expected, 10) or not awards, awards
    if has(expected, 10):
        redrawn = sum(before != "".join(c[:3] for c in after.split(",")) for _, before, after in awards)
        facts.append(f"{len(awards)} Shuffle Times drew from the run's stream ({redrawn} with new cards)")
    print(f"mod_randomizer_dungeon: castle: the save's run restored and re-verified; {'; '.join(facts)}; "
          f"Shadow Chie (Castle 2F) defeated, home on 04/17")
    return run


def drops(unit0, enemy):
    return {int.from_bytes(unit0[enemy * 60 + 0x22 + slot * 4:enemy * 60 + 0x24 + slot * 4], "little")
            for slot in range(4)} - {0}


def fight(leg, expected, dungeon, previous):
    tables, profiles = dungeon
    run = leg("castle_fight", {})
    shutil.copytree(previous.card, run.card)
    text = run.walk("save0415_to_castle_fight.walk", "60000,-1:-1", 3600)
    game.need(text, r"RANDO_RUN load code=" + re.escape(expected["code"]), "the loaded run")
    battles = len(re.findall(r"PROBE_ROSTER battle", text))
    assert battles >= 1, "no floor Shadow was fought"
    # R6: the floor's enemies got the run's profiles; R7: what they dropped is the run's drop list.
    applied = re.findall(r"RANDO_R6 table=(\d+) enemy=(\d+) hp=\d+>(\d+) sp=\d+>(\d+)", text)
    reads = [int(n) for n in re.findall(r"RANDO_R6_BATTLE reads=(\d+)", text)]
    facts = [f"{battles} floor battle(s)"]
    if has(expected, 6):
        assert applied and all(profiles[(int(t), int(e))] == (int(h), int(s)) for t, e, h, s in applied), applied
        # In battle, the stat/attack/reward readers return the profiled values for those units.
        assert len(reads) == battles and all(reads), (battles, reads)
        facts[0] += (f" against enemies with the run's profiles ({', '.join(sorted({f'{int(e):03d}' for _, e, _, _ in applied}))}),"
                     f" {sum(reads)} in-battle stat reads scaled by them")
    else:
        assert not applied and not reads, (applied, reads)
    # Every defeated enemy's drop roll reads the live drop table: a drop it gives is one of
    # the run's (the roll itself is the game's RNG, so a fight may give none).
    rolls = [(int(e), int(i, 16)) for e, i in re.findall(r"PROBE_DROP enemy=(\d+) item=([0-9a-f]+)", text)]
    given = [(enemy, item) for enemy, item in rolls if item]
    assert all(item in drops(tables["EXPECT_UNIT0"], enemy) for enemy, item in given), given
    unlike_stock = sum(item not in drops(tables["STOCK_UNIT0"], enemy) for enemy, item in given)
    facts.append(f"{len(rolls)} drop roll(s), drops {', '.join(f'{item:03x}' for _, item in given) or 'none this time'}"
                 + (f" from the run's drop tables ({unlike_stock} not the stock drop)" if has(expected, 7) else ""))
    awards = re.findall(r"RANDO_R10 shuffle-time/([0-9a-f]{16}) cards=(\S+)>(\S+)", text)
    if has(expected, 10):
        facts.append(f"{len(awards)} Shuffle Time(s)")
    print(f"mod_randomizer_dungeon: castle_fight: {'; '.join(facts)}")


def school(leg, expected, previous):
    run = leg("school0418", {})
    shutil.copytree(previous.card, run.card)
    text = run.walk("save0417_to_school_0418.walk", "60000,-1:-1", 3600)
    game.need(text, r"RANDO_RUN load code=" + re.escape(expected["code"]), "the loaded run")
    game.need(text, r"RANDO_SAVED code=" + re.escape(expected["code"]), "the 04/18 save carrying the run")
    print("mod_randomizer_dungeon: school0418: 04/18 played, saved after school with the run")
    return run


YU_ACCESSORY = 0x20 # the users-mask bit of the protagonist


def accessories_for_yu(items, offers):
    return {item for item in offers if item >> 8 == 2 and
            int.from_bytes(items[item * 68 + 0x30:item * 68 + 0x34], "little") & YU_ACCESSORY}


def sale_material(tables):
    """A material whose one sale unlocks, for Yu's accessory list, an item only the run's swapped gates
    open and keeps closed one the stock gates would open."""
    for material in range(0x501, 0x600):
        sold = {material: 1}
        gained = unlocked(tables["EXPECT_ITEMTBL"], sold) - unlocked(tables["STOCK_ITEMTBL"], sold)
        lost = unlocked(tables["STOCK_ITEMTBL"], sold) - unlocked(tables["EXPECT_ITEMTBL"], sold)
        if accessories_for_yu(tables["EXPECT_ITEMTBL"], gained) and accessories_for_yu(tables["STOCK_ITEMTBL"], lost):
            return material
    raise AssertionError("no single material sale shows a swapped accessory unlock for this run")


def daidara(leg, expected, dungeon, previous):
    tables, _ = dungeon
    run = leg("daidara", {})
    shutil.copytree(previous.card, run.card)
    # Test-only reconstruction, disclosed in the report: the probe adds one of that
    # material through the game's own inventory setter when the street loads (the
    # castle's few fights rarely drop the one material a count-1 gate needs); the
    # sale, Daidara's unlock check and the buy list are the game's own.
    material = sale_material(tables)
    (run.dir / "mod-settings" / (game.PROBE + ".ini")).write_text(f"grant = s:{material:x}:1\n", encoding="ascii")
    text = run.walk("save0418_to_daidara_sale.walk", "40000,-1:-1", 3600)
    game.need(text, rf"PROBE_GRANT {material:x}:1 witness-only", "the witness-only material")
    game.need(text, r"RANDO_RUN load code=" + re.escape(expected["code"]), "the loaded run")
    # R9: after the sale, Daidara's buy list holds what the run's unlock gates allow for
    # the counts sold (the witness-only one and whatever the castle's Shadows dropped):
    # an item only the swapped gates unlock is offered, one only the stock gates would
    # unlock is not.
    lists = re.findall(r"PROBE_SHOP filters=(\S+) sold=(\S*) items=(\S*)", text)
    assert lists, "Daidara's buy list was not built"
    _, sold_text, items = lists[-1]
    sold = {int(m, 16): int(c) for m, c in (part.split(":") for part in sold_text.split(",") if part)}
    shown = {int(item, 16) for item in items.split(",") if item}
    gained = unlocked(tables["EXPECT_ITEMTBL"], sold) - unlocked(tables["STOCK_ITEMTBL"], sold)
    lost = unlocked(tables["STOCK_ITEMTBL"], sold) - unlocked(tables["EXPECT_ITEMTBL"], sold)
    lost = accessories_for_yu(tables["STOCK_ITEMTBL"], lost)
    assert sold.get(material) == 1 and shown & gained and lost and not shown & lost, \
        (sold, sorted(gained), sorted(lost), sorted(shown))
    dropped = ", ".join(f"{m:03x} x{c}" for m, c in sorted(sold.items()) if m != material) or "nothing else"
    print(f"mod_randomizer_dungeon: daidara: one {material:03x} (witness-only) and the castle's drops ({dropped}) sold "
          f"through the game's sale; Yu's "
          f"accessory list offers {', '.join(f'{item:03x}' for item in sorted(shown & gained))}, which only the "
          f"run's swapped unlock gate opens, and not {', '.join(f'{item:03x}' for item in sorted(lost))}, which the "
          f"stock gate would have opened")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--seeds", default=game.SEED, help="comma-separated 32-digit hexadecimal seeds")
    parser.add_argument("--configs", default=str(game.OPTIONS), help="comma-separated feature option sets (4095 all)")
    parser.add_argument("--runner", nargs="+", required=True, help="luau_script_runner (and an emulator before it)")
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True, help="native player")
    args = parser.parse_args()
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("randomizer proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("randomizer proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", "")).resolve()
    iso = Path(os.environ.get("P4PC_ISO", "")).resolve()
    if not elf.is_file() or not iso.is_file():
        print("mod_randomizer_dungeon: retail ELF/ISO unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    cohort = [(seed.strip().lower(), int(options, 0)) for seed in args.seeds.split(",")
              for options in args.configs.split(",")]
    for seed, options in cohort:
        assert re.fullmatch(r"[0-9a-f]{32}", seed) and 0 < options < 4096, (seed, options)
    results = []
    for seed, options in cohort:
        results.append((seed, options, play(args, elf, iso, seed, options)))
    if len(cohort) > 1:
        for seed, options, (outcome, code, detail) in results:
            print(f"mod_randomizer_dungeon: cohort {seed} {game.features_text(options)}: {outcome} {code} {detail}")
    return 0 if all(outcome == "pass" for _, _, (outcome, _, _) in results) else 1


def play(args, elf, iso, seed, options):
    """One run of the cohort: (outcome, run code, evidence or the failure)."""
    work = Path(tempfile.mkdtemp(prefix="mod-randomizer-dungeon-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome, code, detail = "failed", "-", ""
    try:
        banks = Path(tempfile.mkdtemp(prefix="banks-", dir=work))
        expected = game.expected_run(args.runner, str(iso), banks, seed, options)
        code = expected["code"]
        dungeon = expected_dungeon(expected["lines"])
        mods = work / "mods"
        mods.mkdir()
        shutil.copytree(ROOT / "mods" / game.PACKAGE, mods / game.PACKAGE)
        shutil.copytree(ROOT / "tests" / "mod_fixtures" / "randomizer_probe", mods / game.PROBE)
        common = game.common_env(mods, iso, args.driver)

        def leg(name, settings):
            return game.Leg(work, name, settings, common, args.driver, elf)

        first = story0415(leg, expected)
        fight(leg, expected, dungeon, first)
        home = castle(leg, expected, dungeon, first)
        if has(expected, 9):
            daidara(leg, expected, dungeon, school(leg, expected, home))
        outcome, detail = "pass", f"evidence={work}"
        print(f"mod_randomizer_dungeon: PASS run={code} mod_witness=true reference_eligible=false evidence={work}")
    except AssertionError as error:
        detail = f"{error}".splitlines()[0][:300]
        print(f"mod_randomizer_dungeon: FAIL run={code}: {detail}")
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)
    return outcome, code, detail


if __name__ == "__main__":
    sys.exit(main())
