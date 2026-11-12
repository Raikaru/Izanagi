#!/usr/bin/env python3
"""In-game witness of randomized runs in the second and third dungeons, from reconstructed anchors.

Mod-witness lane only (P4PC_MOD_WITNESS=1, P4PC_INPUT=script), never a reference
gate. A test-only reconstruction, not a randomized playthrough: no chained
walk reaches the Steamy Bathhouse or Marukyu Striptease under a randomized run
(the story chain's fusion, menu and boss steps are bound to the stock game), so
each leg loads a stock checkpoint and a patched copy of the shipped mod adopts
the run there:

  * The copy's save `load` callback treats a block-less load like the New Game
    reset (the run's settings arm and the run goes live at the next field), and
    its two New Game guards are skipped: the New Game+ refusal (the compendium
    carry-over check) and the late-run refusal (a Persona obtained before the
    run was ready), since the anchor's compendium and roster are the stock
    playthrough's. All three are exact one-line replacements; everything else
    is the shipped mod and probe.

What it proves, per seed: the run is generated, published and applied mid-game
like a New Game run (the same code, digest and output as headless generation);
the dungeons' floors pick formations from the run's encounter tables, some the
original tables never held (R5), with the run's enemy profiles (R6); the
starter joins (R11); and whether the stock boss walks (searched battle tapes,
bound to their frames) still beat Shadow Kanji and Shadow Rise and Teddie with
the run's tables live. The bosses and the party's Personas are protected, but
the protagonist's Personas read their affinities live (R3).

What it does not prove: anything about early difficulty or legal progression.
The anchors' Personas, levels, items and compendium come from the stock
playthrough, not from the run (the design's "no stock-anchor persona" rule),
and the floors are crossed by escaping every encounter. A boss tape's failure is
reported as the outcome, not as a randomizer defect on its own, and so is a
floor walk that stops (an escape that fails until the party falls): the floor
checks then hold over what was played. A run fails only when the run is not
adopted as generated or a floor leg shows no run picks or profiles.

  bathhouse  story0520tv (port-made, 05/20 TV-world Entrance) and
             story0520tv_to_bathhouse_3f_escape.walk: Bathhouse 1F-3F.
  kanji      playthrough0520bathhouse11 (collection card2-slot08, Hall 11) and
             collection0520_bathhouse11_to_kanji_victory.walk.
  marukyu    playthrough0625tv (collection card2-slot14) and
             collection0625_tv_to_marukyu3f.walk: Marukyu 1F-3F, escaping.
  teddie     playthrough0625marukyu11f (collection card2-slot15) and
             collection0625_marukyu11f_to_teddie_victory.walk: Rise and Teddie.

The anchors are read from P4PC_RANDOMIZER_SEEDS (a folder per name holding a
normalized BASLUS-21782Save0, as tools/collection_save.py stages them).

usage: mod_randomizer_anchor_test.py --out DIR [--seeds HEX32,...] --runner RUNNER... --driver PLAYER...
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
import mod_randomizer_dungeon_test as dungeon  # noqa: E402

# (old, new) exact replacements in the copied main.luau; each must match once.
RECONSTRUCTION = (
    ('        if origin == "reset" then\n',
     '        if origin == "reset" or value == nil then -- witness reconstruction: a block-less load adopts the run\n'),
    ("    local carried = carriedOver()\n",
     "    local carried = nil -- witness reconstruction: the anchor's compendium is the stock playthrough's\n"),
    ("    local granted = personaGranted()\n",
     "    local granted = nil -- witness reconstruction: the anchor's roster is the stock playthrough's\n"),
)
# name: (anchor, walk, gate, kind); a floor leg checks the run's floors, a boss leg reports the outcome.
LEGS = {
    "bathhouse": ("story0520tv", "story0520tv_to_bathhouse_3f_escape.walk", "60000,-1:-1", "floors"),
    "kanji": ("playthrough0520bathhouse11", "collection0520_bathhouse11_to_kanji_victory.walk", "34000,24:2,5/20", "boss"),
    "marukyu": ("playthrough0625tv", "collection0625_tv_to_marukyu3f.walk", "30000,62:2,6/25", "floors"),
    "teddie": ("playthrough0625marukyu11f", "collection0625_marukyu11f_to_teddie_victory.walk", "82000,25:2,6/25", "boss"),
}


def patched_mod(target):
    shutil.copytree(ROOT / "mods" / game.PACKAGE, target)
    main = target / "main.luau"
    text = main.read_text(encoding="utf-8")
    for old, new in RECONSTRUCTION:
        assert text.count(old) == 1, f"reconstruction patch point changed: {old.strip()}"
        text = text.replace(old, new)
    main.write_text(text, encoding="utf-8", newline="\n")


def adopted(text, expected):
    game.need(text, r"RANDO_RESET pending=true", "the reconstruction arming on the block-less load")
    game.need(text, r"RANDO_NEW_GAME armed seed=" + expected["seed"], "the run arming at the next field")
    run_line = game.need(text, r"RANDO_RUN new_game code=(\S+) digest=([0-9a-f]+) features=" + expected["features"]
                         + r" output=([0-9a-f]+)", "the adopted run").groups()
    assert run_line == (expected["code"], expected["digest"], expected["output"]), (run_line, expected)


def floors(text, expected, tables, profiles):
    """The floor picks after the run went live come from the run's tables (R5), with its profiles (R6)."""
    live = text.split("RANDO_RUN new_game", 1)[1]
    picks = [(int(t), int(f)) for t, f, _ in re.findall(r"PROBE_PICK table=(\d+) formation=(\d+) kind=(\d+)", live)
             if int(f) != 0xffff]
    assert picks, "no floor encounter was picked after the run went live"
    moved = 0
    for table, formation in picks:
        assert formation in dungeon.table_formations(tables["EXPECT_ENCOUNT3"], table), (table, formation)
        moved += formation not in dungeon.table_formations(tables["STOCK_ENCOUNT3"], table)
    applied = re.findall(r"RANDO_R6 table=(\d+) enemy=(\d+) hp=\d+>(\d+) sp=\d+>(\d+)", live)
    assert applied and all(profiles[(int(t), int(e))] == (int(h), int(s)) for t, e, h, s in applied), applied
    battles = len(re.findall(r"PROBE_ROSTER battle", live))
    return (f"{len(picks)} floor picks from the run's tables (tables {sorted({t for t, _ in picks})}, {moved} formations "
            f"the original tables never held), {len(applied)} enemies with the run's profiles, {battles} encounter(s) "
            f"escaped")


def play(args, elf, iso, seed):
    work = Path(tempfile.mkdtemp(prefix="mod-randomizer-anchor-", dir=args.out)).resolve()
    mod_witness.begin(work)
    results = []
    try:
        banks = Path(tempfile.mkdtemp(prefix="banks-", dir=work))
        expected = game.expected_run(args.runner, str(iso), banks, seed, game.OPTIONS)
        tables, profiles = dungeon.expected_dungeon(expected["lines"])
        mods = work / "mods"
        mods.mkdir()
        patched_mod(mods / game.PACKAGE)
        shutil.copytree(ROOT / "tests" / "mod_fixtures" / "randomizer_probe", mods / game.PROBE)
        common = game.common_env(mods, iso, args.driver)
        anchors = Path(os.environ.get("P4PC_RANDOMIZER_SEEDS", str(elf.parent / "saves")))
        for name, (anchor, walk, gate, kind) in LEGS.items():
            run = game.Leg(work, name, expected["settings"], common, args.driver, elf)
            source = anchors / anchor
            assert (source / "BASLUS-21782Save0").is_dir(), f"no anchor {source}"
            shutil.copytree(source, run.card)
            try:
                text = run.walk(walk, gate, 3600)
                stopped = None
            except AssertionError as error:
                # A walk that stops (a failed escape, a boss tape that diverges) is an outcome to
                # report; the run's adoption and floors are still checked on what was played.
                log = run.dir / "run.log"
                if not log.is_file():
                    raise
                text = log.read_text(encoding="utf-8", errors="replace")
                failed = re.search(r"WALK SCRIPT FAILED at command \d+ '([^']*)': ([^;\n]*)", text)
                stopped = ("the party fell (game over)" if "dng_decease" in text else
                           f"the walk stopped at '{failed.group(1)}': {failed.group(2)}" if failed else
                           str(error).splitlines()[0][:200])
            adopted(text, expected)
            grants = re.findall(r"RANDO_R11 (granted|roster_full) persona=(\d+) date=(\d+)", text)
            starter = (f"; starter {int(grants[0][1]):03d} {'joined' if grants[0][0] == 'granted' else 'not added (roster full)'}"
                       f" on date {grants[0][2]}" if grants else "")
            if kind == "floors":
                state = "pass"
                detail = floors(text, expected, tables, profiles) + starter + (
                    "; reached the walk's end" if stopped is None else f"; {stopped}")
            elif stopped is None and "walk script done" in text:
                state, detail = "beaten", "the stock boss walk finished with the run live" + starter
            else:
                state, detail = "not beaten", (stopped or "the boss walk did not complete") + starter
            results.append((name, state, detail))
            print(f"mod_randomizer_anchor: {name}: run {expected['code']} adopted on the {anchor} anchor; {detail}")
        outcome = "pass"
    except AssertionError as error:
        outcome = "failed"
        results.append(("run", "failed", str(error).splitlines()[0][:300]))
        print(f"mod_randomizer_anchor: FAIL {seed}: {results[-1][2]}")
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)
    return outcome, results, work


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--seeds", default=game.SEED, help="comma-separated 32-digit hexadecimal seeds")
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
        print("mod_randomizer_anchor: retail ELF/ISO unavailable", file=sys.stderr)
        return 77
    anchors = Path(os.environ.get("P4PC_RANDOMIZER_SEEDS", str(elf.parent / "saves")))
    missing = [anchor for anchor, _, _, _ in LEGS.values() if not (anchors / anchor / "BASLUS-21782Save0").is_dir()]
    if missing:
        print(f"mod_randomizer_anchor: anchors {', '.join(missing)} not under {anchors}", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    seeds = [seed.strip().lower() for seed in args.seeds.split(",")]
    for seed in seeds:
        assert re.fullmatch(r"[0-9a-f]{32}", seed), seed
    failed = False
    for seed in seeds:
        outcome, results, work = play(args, elf, iso, seed)
        failed = failed or outcome != "pass"
        summary = "; ".join(f"{name} {state}" for name, state, _ in results)
        print(f"mod_randomizer_anchor: {'PASS' if outcome == 'pass' else 'FAIL'} seed={seed} {summary} "
              f"mod_witness=true reference_eligible=false reconstruction=true evidence={work}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
