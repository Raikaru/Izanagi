#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS proof of the bundled p4pc.hardmode
rules in the running game; not a retail comparison and not balance evidence.

Settings are chosen the way a player does it: declared [setting] values in
mod-settings/p4pc.hardmode.ini beside p4pc.ini. Private seeds are ordinary
saves, so a run on a seed is armed through the mod's one-shot "start from the
next loaded save" setting; the New Game scenario arms through a real New Game.

Every scenario checks each logged enemy (starting HP/SP and the five stats) and
each logged EXP/yen reward against the shipped multiplier of its difficulty
and class, with the script's rounding and limits.

Scenarios (--scenario, repeatable; default core):
  core      story0420_5f, castle training walk:
              harder      HARDER adopted on load; the run is saved; the one-shot
                          setting clears.
              reload      that card with the settings back at OFF: the save's own
                          block restores the HARDER run.
              off         default settings (OFF): no hard-mode rule runs.
              off-reload  the OFF card with HARDER selected but no adoption: the
                          boot and LOAD resets only mark a pending New Game and
                          the block-less save loads OFF.
  newgame   a real New Game through the first night's tutorial battle with HARD
            selected: the run arms at the first field after the New Game reset,
            and the tutorial formation is a boss-class encounter.
  knight    story0423tv to Avenger Knight (encounter 811, enemy 310), HARD
            adopted: the boss is boss class and scaled; the open-loop route's
            outcome is reported (it is proven only on the retail flow).
  yukiko    story0426trained5 to Shadow Yukiko (encounter 514, enemy 258), HARD
            adopted: boss class and scaling for the boss and her summons; outcome
            reported as for knight.
  knight-off, yukiko-off
            the same walks with hard mode OFF: the open-loop routes' own outcome,
            the control for a HARD loss.
  knight-neutral, yukiko-neutral
            HARD adopted from a staged copy whose HARD multipliers are all 1.00:
            every hook runs, no enemy changes, and the route wins as it does OFF.
  castle    Castle 6F, 7F and 8F walks, HARD adopted (ordinary enemies keep
            their stats) through formations with an enemy healer: every enemy
            support change is consistent with its rule and thresholds.
"""
import argparse
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mod_witness

PACKAGE = "p4pc.hardmode"
WALKS = ROOT / "tests/walks"
SLOT = "BASLUS-21782Save0"
TRAINING = ("save0420_5f_to_training.walk", "38000,20:1,4/20")
RULE_LINE = re.compile(r"HARDMODE_\w+")
STATS = ("st", "ma", "en", "ag", "lu")
ENEMY_LINE = re.compile(r"HARDMODE_ENEMY id=(\d+) boss=(true|false) hp=(\d+)>(\d+) sp=(\d+)>(\d+) "
                        + " ".join(rf"{s}=(\d+)>(\d+)" for s in STATS))
REWARD_LINE = re.compile(r"HARDMODE_REWARD id=(\d+) boss=(true|false) (exp|yen)=(\d+)>(\d+)")
AI_LINE = re.compile(r"HARDMODE_AI enemy=(\d+) native=(\d+) chosen=(\d+) reason=(\w+) lowest=([\d.]+) highest=([\d.]+)")
# The first night: New Game events, the dream door and the tutorial battle (story_gate.sh's route and inputs).
NEWGAME_ENV = {"P4PC_AUTOPRESS_NEWGAME": "1560", "P4PC_AUTOPRESS": "cross@2300:2/150*20,start@6500:2/300*14",
               "P4PC_FACING_GATE": "1", "P4PC_BATTLE_FIRST": "down,cross,cross,cross",
               "P4PC_BATTLE_DRIVER": "cross,cross,cross", "P4PC_BATTLE_SKIP": "140"}


def tuning():
    """({mode: {class: {field: multiplier}}}, {skip_heal_above, rescue_below}) from the shipped tuning file."""
    text = (ROOT / "mods" / PACKAGE / "files/DATA/HARDMODE/TUNING.TXT").read_text(encoding="ascii")
    modes, ai, section = {}, {}, None
    for line in text.splitlines():
        line = line.split("#", 1)[0].strip()
        header = re.fullmatch(r"\[(\w+)\]", line)
        if header:
            section = header.group(1)
            continue
        entry = re.fullmatch(r"(ordinary|boss)\.(\w+)\s*=\s*(\S+)", line)
        if entry and section in ("hard", "harder"):
            modes.setdefault(section, {}).setdefault(entry.group(1), {})[entry.group(2)] = float(entry.group(3))
        rule = re.fullmatch(r"(skip_heal_above|rescue_below)\s*=\s*(\S+)", line)
        if rule and section == "ai":
            ai[rule.group(1)] = float(rule.group(2))
    return modes, ai


def scaled(value, factor, low, high):
    """The script's rounding: nearest whole number, clamped; zero stays zero."""
    if factor == 1 or value == 0:
        return value
    return min(max(math.floor(value * factor + 0.5), low), high)


def verify_scaling(text, classes):
    """Every logged enemy and reward equals native x the multiplier of its class
    ({"ordinary": {...}, "boss": {...}} for the run's difficulty). Returns counts."""
    enemies = ENEMY_LINE.findall(text)
    for row in enemies:
        enemy, boss = row[0], row[1] == "true"
        table = classes["boss" if boss else "ordinary"]
        values = [int(v) for v in row[2:]]
        hp, new_hp, sp, new_sp = values[:4]
        assert new_hp == scaled(hp, table["hp"], 1, 0xffff), (enemy, boss, "hp", hp, new_hp)
        assert new_sp == scaled(sp, table["sp"], 1, 0xffff), (enemy, boss, "sp", sp, new_sp)
        for index, stat in enumerate(STATS):
            native, value = values[4 + 2 * index], values[5 + 2 * index]
            assert value == scaled(native, table[stat], 1, 99), (enemy, boss, stat, native, value)
    rewards = REWARD_LINE.findall(text)
    for enemy, boss, field, native, value in rewards:
        table = classes["boss" if boss == "true" else "ordinary"]
        assert int(value) == scaled(int(native), table[field], 0, 0xffff), (enemy, boss, field, native, value)
    unparsed = len(re.findall(r"HARDMODE_ENEMY ", text)) - len(enemies)
    assert unparsed == 0, f"{unparsed} enemy lines were not in the expected form"
    return enemies, rewards


class Leg:
    def __init__(self, work, name, settings, common, driver, elf):
        self.dir = work / name
        self.dir.mkdir()
        self.card = self.dir / "card" / "mc0"
        self.card.parent.mkdir()
        self.name = name
        self.common = common
        self.driver = driver
        self.elf = elf
        (self.dir / "p4pc.ini").write_text("[mods]\nenabled=%s\n" % PACKAGE, encoding="ascii")
        if settings:
            store = self.dir / "mod-settings"
            store.mkdir()
            (store / (PACKAGE + ".ini")).write_text(
                "".join(f"{key} = {value}\n" for key, value in settings.items()), encoding="ascii")

    def settings(self):
        path = self.dir / "mod-settings" / (PACKAGE + ".ini")
        return path.read_text(encoding="ascii") if path.is_file() else ""

    def walk(self, walk, gate, timeout, stop_on=None, grace=20, extra=None, drop=()):
        """Runs one process: returns its log text. stop_on: a regex whose
        appearance ends the run after `grace` seconds (a reload proof)."""
        env = dict(self.common, P4PC_CONFIG=str(self.dir / "p4pc.ini"), P4PC_SAVE_DIR=str(self.card.parent),
                   P4PC_SCENE_ARTIFACTS=str(self.dir), P4PC_WALK_SCRIPT=str(WALKS / walk), P4PC_NEWGAME_GATE=gate)
        env.update(extra or {})
        for key in drop:
            env.pop(key, None)
        log = self.dir / "run.log"
        stopped = False
        with log.open("wb") as output:
            process = subprocess.Popen([*self.driver, str(self.elf)], cwd=self.dir, env=env, stdout=output,
                                       stderr=subprocess.STDOUT,
                                       creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
            deadline = time.monotonic() + timeout
            seen_at = None
            while process.poll() is None:
                time.sleep(1)
                if stop_on and seen_at is None and re.search(stop_on, log.read_text(encoding="utf-8", errors="replace")):
                    seen_at = time.monotonic()
                if (seen_at is not None and time.monotonic() - seen_at > grace) or time.monotonic() > deadline:
                    process.kill()
                    process.wait()
                    stopped = True
        text = log.read_text(encoding="utf-8", errors="replace")
        assert "run_mode=mod-witness" in text and "reference_eligible=false" in text, f"{self.name}: not a witness run"
        assert "HARDMODE_READY" in text, f"{self.name}: hard mode did not start: see {log}"
        assert "budget exhausted" not in text, f"{self.name}: an interpreter budget was exhausted: see {log}"
        if not stopped:
            assert process.returncode == 0, f"{self.name} exited {process.returncode}: see {log}"
        elif not stop_on or seen_at is None:
            raise AssertionError(f"{self.name}: timed out after {timeout}s: see {log}")
        print(f"mod_hardmode_game: {self.name} ok ({'stopped' if stopped else 'exited'})")
        return text


def need(text, pattern, what):
    match = re.search(pattern, text)
    assert match, f"missing {what}: /{pattern}/"
    return match


def encounters_of(text):
    return re.findall(r"HARDMODE_ENCOUNTER id=(\d+) boss=(true|false)", text)


def core(leg, seeds, modes, ai):
    hard = leg("harder", {"mode": "Harder", "adopt_on_load": "on"})
    shutil.copytree(seeds / "story0420_5f", hard.card)
    text = hard.walk(*TRAINING, timeout=2400)
    need(text, r"HARDMODE_TUNING revision=1 ", "the tuning file read at start-up")
    need(text, r"HARDMODE_RUN adopted mode=harder features=stats,ai,rewards revision=1 adopted=true",
         "HARDER adopted on load")
    encounters = encounters_of(text)
    assert encounters, "no encounter was classified"
    assert all(boss == "false" for _, boss in encounters), f"the training walk met a boss encounter: {encounters}"
    enemies, rewards = verify_scaling(text, modes["harder"])
    assert enemies, "no enemy was scaled"
    assert {field for _, _, field, _, _ in rewards} == {"exp", "yen"}, f"rewards not both scaled: {rewards}"
    assert "walk script done" in text and "SDKMEMCARD_SAVE OK" in text, "training walk/save incomplete"
    assert not re.search(r"(?m)^adopt_on_load = (on|true)$", hard.settings()), "the one-shot setting did not clear"
    print(f"mod_hardmode_game: harder: {len(encounters)} encounters, {len(enemies)} enemies, "
          f"{len(rewards)} rewards scaled exactly")

    reload = leg("reload", {"mode": "Off"})
    shutil.copytree(hard.card, reload.card)
    text = reload.walk(*TRAINING, timeout=900, stop_on=r"HARDMODE_RUN (load|adopted) ", grace=5)
    need(text, r"HARDMODE_RUN load mode=harder features=stats,ai,rewards revision=1 adopted=true",
         "the saved HARDER run restored from its own block")

    off = leg("off", {})
    shutil.copytree(seeds / "story0420_5f", off.card)
    text = off.walk(*TRAINING, timeout=2400)
    need(text, r"HARDMODE_RESET pending=false", "a reset with nothing pending while OFF")
    need(text, r"HARDMODE_RUN load off", "an OFF load")
    rules = sorted(set(RULE_LINE.findall(text)) - {"HARDMODE_TUNING", "HARDMODE_READY", "HARDMODE_RESET", "HARDMODE_RUN"})
    assert not rules, f"hard mode acted while OFF: {rules}"
    assert "walk script done" in text and "SDKMEMCARD_SAVE OK" in text, "training walk/save incomplete"

    later = leg("off-reload", {"mode": "Harder"})
    shutil.copytree(off.card, later.card)
    text = later.walk(*TRAINING, timeout=900, stop_on=r"HARDMODE_RUN (load|adopted) ", grace=5)
    need(text, r"HARDMODE_RESET pending=true", "a reset marking a pending New Game while HARDER is selected")
    need(text, r"HARDMODE_RUN load off", "a save without a hard-mode block loading OFF")
    assert "HARDMODE_RUN new_game" not in text, "a reset that a load followed armed a New Game run"


def newgame(leg, seeds, modes, ai):
    run = leg("newgame", {"mode": "Hard"})
    text = run.walk("newgame_to_rain_scene.walk", "25100,-1:-1", timeout=2400, extra=NEWGAME_ENV,
                    drop=("P4PC_AUTOPRESS_LOAD",))
    need(text, r"HARDMODE_RESET pending=true", "the New Game reset marking a pending run")
    need(text, r"HARDMODE_RUN new_game mode=hard features=stats,ai,rewards revision=1 adopted=false",
         "HARD armed by the New Game")
    armed = text.index("HARDMODE_RUN new_game")
    encounters = encounters_of(text[armed:])
    assert encounters, "the tutorial battle was not classified after arming"
    assert any(boss == "true" for _, boss in encounters), f"the tutorial formation was not boss class: {encounters}"
    enemies, rewards = verify_scaling(text, modes["hard"])
    assert any(row[1] == "true" for row in enemies), "no boss-class enemy was scaled"
    print(f"mod_hardmode_game: newgame: encounters {encounters}, {len(enemies)} enemies, {len(rewards)} rewards")


def won_after(text, start, name):
    """The fight that begins at `start` was won: no dungeon game over followed it.
    A gate alone cannot tell: after a wipe the title's presses reload the seed,
    which already stands where the gate looks."""
    assert "dng_decease" not in text[start:], f"{name}: the party was wiped in or after the boss fight"
    assert "walk script done" in text, f"{name}: the walk did not finish"


def boss(name, seed, walk, gate, encounter, enemy, timeout):
    """Boss class and scaling in the real fight. These routes drive the fight's
    menus open-loop and are proven only on the retail flow, so the outcome is
    reported, not asserted: the -neutral control proves the hooks themselves
    leave the route intact, and the simulator cohort measures difficulty."""
    def scenario(leg, seeds, modes, ai):
        run = leg(name, {"mode": "Hard", "adopt_on_load": "on"})
        shutil.copytree(seeds / seed, run.card)
        resolved = rf"end <dng_decease>|HARDMODE_REWARD id={enemy} boss=true yen="
        text = run.walk(walk, gate, timeout=timeout, stop_on=resolved, grace=10)
        need(text, r"HARDMODE_RUN adopted mode=hard ", "HARD adopted on load")
        start = need(text, rf"HARDMODE_ENCOUNTER id={encounter} boss=true", f"encounter {encounter} as boss class").start()
        enemies, rewards = verify_scaling(text, modes["hard"])
        assert any(row[0] == str(enemy) and row[1] == "true" for row in enemies), f"enemy {enemy} not scaled as boss"
        assert "HARDMODE_AI" not in text[start:], "the support rule changed a boss-encounter pick"
        won = "dng_decease" not in text[start:] and re.search(rf"HARDMODE_REWARD id={enemy} boss=true exp=", text[start:])
        print(f"mod_hardmode_game: {name}: {len(encounters_of(text))} encounters, {len(enemies)} enemies, "
              f"{len(rewards)} rewards; open-loop route {'won' if won else 'lost'} on HARD")
    return scenario


def control(name, seed, walk, gate, encounter, timeout):
    """The same walk with hard mode OFF: the open-loop route's own retail outcome."""
    def scenario(leg, seeds, modes, ai):
        run = leg(name + "-off", {})
        shutil.copytree(seeds / seed, run.card)
        text = run.walk(walk, gate, timeout=timeout)
        need(text, r"HARDMODE_RUN load off", "an OFF load")
        assert "HARDMODE_ENCOUNTER" not in text, f"{name}-off: hard mode acted while OFF"
        start = need(text, rf"did = {encounter}\b", f"encounter {encounter}").start()
        won_after(text, start, name + "-off")
        print(f"mod_hardmode_game: {name}-off: retail route won")
    return scenario


def neutral(name, seed, walk, gate, encounter, timeout):
    """HARD armed exactly as in `boss`, from a staged copy whose HARD multipliers
    are all 1.00: every hook runs and changes nothing, so the route must win as
    it does OFF. Separates a perturbation by the hooks themselves from the
    effect of the shipped multipliers on an open-loop route."""
    def scenario(leg, seeds, modes, ai):
        run = leg(name + "-neutral", {"mode": "Hard", "adopt_on_load": "on"})
        package = run.dir.parent / "mods-neutral"
        if not package.exists():
            shutil.copytree(run.dir.parent / "mods", package)
            path = package / PACKAGE / "files" / "DATA" / "HARDMODE" / "TUNING.TXT"
            text = path.read_text(encoding="ascii")
            head, section, rest = text.partition("[hard]")
            body, marker, tail = rest.partition("[harder]")
            body = re.sub(r"(?m)^(\w+\.\w+) = [0-9.]+$", r"\1 = 1.00", body)
            path.write_text(head + section + body + marker + tail, encoding="ascii")
        shutil.copytree(seeds / seed, run.card)
        text = run.walk(walk, gate, timeout=timeout, extra={"P4PC_MOD_DIR": str(package)})
        need(text, r"HARDMODE_RUN adopted mode=hard ", "HARD adopted on load")
        start = need(text, rf"HARDMODE_ENCOUNTER id={encounter} boss=true", f"encounter {encounter} as boss class").start()
        enemies = ENEMY_LINE.findall(text)
        assert enemies, "no enemy was logged"
        for row in enemies:
            values = [int(v) for v in row[2:]]
            assert values[0::2] == values[1::2], (name, "neutral tuning changed an enemy", row)
        won_after(text, start, name + "-neutral")
        print(f"mod_hardmode_game: {name}-neutral: hooks armed at neutral tuning, route won")
    return scenario


CASTLE = (("6f", "story0424tv", "save0424tv_to_castle_6f.walk", "18000,20:1,4/24"),
          ("7f", "story0424forneus", "save0424forneus_to_castle_7f.walk", "30000,20:1,4/24"),
          ("8f", "story0424_7f", "save0424_7f_to_castle_8f.walk", "32000,20:1,4/24"))
# Formations with Magical Magus (enemy 13), whose AI table rows hold Dia (read
# from the retail ENCOUNT/AI tables; none in the castle training walk).
HEALER_ENCOUNTERS = {"56", "62", "64", "894", "895", "901"}


def castle(leg, seeds, modes, ai):
    """Every support change these routes produce agrees with its rule and
    thresholds. A fixed route cannot force the HP states either rule needs, so
    the rule firing is observed over seeds in the simulator cohort instead."""
    changes, met = [], set()
    for floor, seed, walk, gate in CASTLE:
        run = leg("castle-" + floor, {"mode": "Hard", "adopt_on_load": "on"})
        shutil.copytree(seeds / seed, run.card)
        text = run.walk(walk, gate, timeout=2400)
        need(text, r"HARDMODE_RUN adopted mode=hard features=stats,ai,rewards ", "HARD adopted on load")
        verify_scaling(text, modes["hard"])
        assert "walk script done" in text, f"castle {floor}: the walk did not finish"
        found = AI_LINE.findall(text)
        for enemy, native, chosen, reason, lowest, highest in found:
            assert native != chosen, (enemy, native, chosen)
            # The log prints the fractions to two decimals.
            if reason == "skip_heal":
                assert float(lowest) >= ai["skip_heal_above"] - 0.005, (enemy, reason, lowest)
            else:
                assert reason == "rescue" and float(lowest) <= ai["rescue_below"] + 0.005, (enemy, reason, lowest)
        encounters = {e for e, _ in encounters_of(text)}
        met |= encounters & HEALER_ENCOUNTERS
        print(f"mod_hardmode_game: castle {floor}: encounters {sorted(encounters)}, "
              f"{len(found)} support changes {found}")
        changes += found
    assert met, "the castle routes met no formation with an enemy healer"
    print(f"mod_hardmode_game: castle: healer formations {sorted(met)}, {len(changes)} support changes, all consistent")


# (seed, walk, gate, encounter): encounter 811 is Avenger Knight (DatUnit enemy
# 310), 514 is Shadow Yukiko (enemy 258, who summons Charming Prince 269).
KNIGHT = ("story0423tv", "save0423tv_to_knight.walk", "42000,20:1,4/23", 811)
YUKIKO = ("story0426trained5", "save0426trained5_to_yukiko_rescue.walk", "55000,6:6,4/27", 514)
SCENARIOS = {
    "core": core,
    "newgame": newgame,
    "knight": boss("knight", *KNIGHT, 310, 3600),
    "knight-off": control("knight", *KNIGHT, 3600),
    "knight-neutral": neutral("knight", *KNIGHT, 3600),
    "yukiko": boss("yukiko", *YUKIKO, 258, 3600),
    "yukiko-off": control("yukiko", *YUKIKO, 3600),
    "yukiko-neutral": neutral("yukiko", *YUKIKO, 3600),
    "castle": castle,
}
SEEDS = {"core": ("story0420_5f",), "newgame": (), "castle": tuple(seed for _, seed, _, _ in CASTLE),
         **{name + suffix: route[:1] for name, route in (("knight", KNIGHT), ("yukiko", YUKIKO))
            for suffix in ("", "-off", "-neutral")}}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--scenario", action="append", choices=sorted(SCENARIOS))
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True,
                        help="native player (and optional Wine prefix)")
    args = parser.parse_args()
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("hard-mode proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("hard-mode proof requires mods permitted and packs disabled")
    selected = args.scenario or ["core"]
    elf = Path(os.environ.get("P4PC_ELF", "")).resolve()
    iso = Path(os.environ.get("P4PC_ISO", "")).resolve()
    seeds = Path(os.environ.get("P4PC_HARDMODE_SEEDS", str(elf.parent / "saves")))
    needed = {seed for name in selected for seed in SEEDS[name]}
    if not elf.is_file() or not iso.is_file() or any(not (seeds / s / SLOT).is_dir() for s in needed):
        print("mod_hardmode: retail ELF/ISO or a private seed unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-hardmode-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        mods = work / "mods"
        mods.mkdir()
        shutil.copytree(ROOT / "mods" / PACKAGE, mods / PACKAGE)
        common = dict(os.environ, P4PC_MOD_DIR=str(mods), P4PC_MODS="1", P4PC_PACKS="0",
                      P4PC_INPUT="script", P4PC_MOD_WITNESS="1", P4PC_ISO=str(iso), P4PC_HIDDEN_WINDOW="1",
                      P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2",
                      P4PC_VIRTUAL_CLOCK="1", P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            common.pop("SDL_VIDEODRIVER", None)
        else:
            common["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED", "P4PC_DUMP_RAM",
                    "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT", "P4PC_WALK_SCRIPT", "P4PC_NEWGAME_GATE",
                    "P4PC_AUTOPRESS", "P4PC_AUTOPRESS_NEWGAME", "P4PC_BATTLE_FIRST", "P4PC_BATTLE_DRIVER",
                    "P4PC_BATTLE_SKIP", "P4PC_FACING_GATE"):
            common.pop(key, None)

        def leg(name, settings):
            return Leg(work, name, settings, common, args.driver, elf)

        modes, ai = tuning()
        for name in selected:
            SCENARIOS[name](leg, seeds, modes, ai)
        outcome = "pass"
        print(f"mod_hardmode_game: PASS scenarios={','.join(selected)} mod_witness=true reference_eligible=false "
              f"evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
