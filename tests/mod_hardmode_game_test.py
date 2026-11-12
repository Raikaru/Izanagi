#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS proof of the bundled p4pc.hardmode
rules in the running game; not a retail comparison and not balance evidence.

Settings are chosen the way a player does it: declared [setting] values in
mod-settings/p4pc.hardmode.ini beside p4pc.ini. Private seeds are ordinary
saves, so a run on a seed is armed through the mod's one-shot "start from the
next loaded save" setting; the New Game scenario arms through a real New Game.

Hard mode requires Physical Split ([require p4pc.physsplit] with split on), so
every leg enables both with PHYSICAL SPLIT on unless it names another set.

Every scenario checks each logged enemy (starting HP/SP and the five stats) and
each logged EXP/yen reward against the shipped multiplier of its difficulty
and class, with the script's rounding and limits.

Scenarios (--scenario, repeatable; default core):
  core      story0420_5f, castle training walk:
              hard        HARD adopted on load; the run is saved; the one-shot
                          setting clears.
              reload      that card with the settings back at OFF: the save's own
                          block restores the HARD run.
              reload-no-split  that card with only hard mode enabled: hard mode
                          is refused for its missing requirement and the save,
                          whose required blocks need it, refuses the load.
              off         default settings (OFF): no hard-mode rule runs.
              off-reload  the OFF card with a settings file that still holds the
                          former Harder choice: it reads as OFF, so the resets
                          mark nothing pending and the block-less save loads OFF.
  requires  hard mode without Physical Split enabled, and with PHYSICAL SPLIT
            off: admission refuses hard mode with the requirement's reason.
  physsplit-alone
            Physical Split alone on the Knight route: physical actions resolve
            to Slash/Strike/Pierce leaves and no hard-mode rule exists.
  newgame   a real New Game through the first night's tutorial battle with HARD
            selected: the run arms at the first field after the New Game reset,
            and the tutorial formation is a boss-class encounter.
  knight    story0423tv to Avenger Knight (encounter 811, enemy 310), HARD
            adopted: the boss is boss class and scaled, every turn follows the
            boss tactic (Power Charge on each knight.cycle-th base turn, Gale
            Slash on the next, his own pick otherwise; One More actions do not
            count) with no substitution refused by the enemy AI carrier, and
            physical actions resolve to Physical Split leaves; the open-loop
            route's outcome is reported (it is proven only on the retail flow).
  yukiko    story0426trained5 to Shadow Yukiko (encounter 514, enemy 258), HARD
            adopted: boss class and scaling for the boss and her summons; her
            redesign's base turns count 1..n with phases that never go back,
            every Burn to Ashes comes right after her own Mind Charge, at most
            one summon, and the Prince's Diarama keeps its spacing; outcome
            reported as for knight.
  knight-off, yukiko-off, kanji-off
            the same walks with hard mode OFF: the open-loop routes' own outcome,
            the control for a HARD loss.
  knight-retry, knight-giveup
            HARD with BATTLE RETRY on the Knight route; a witness-only package
            leaves Yu one HP at the Knight's battle start and every command is
            Guard, so the party falls. The defeat is offered before the Game Over
            (HARDMODE_RETRY offered); RETRY restarts the same encounter with the
            battle-start HP/SP and no Game Over (HARDMODE_RETRY restarted ...
            matches=true), GIVE UP lets the retail Game Over (dng_decease) run.
  knight-neutral, yukiko-neutral
            HARD adopted from a staged copy whose HARD multipliers are all 1.00
            (Technical and boss tactics off): every scaling hook runs, no enemy
            changes, and the route wins as it does OFF.
  kanji     collection card playthrough0520bathhouse11 to Shadow Kanji
            (encounter 515, enemy 259), HARD adopted: boss class and scaling;
            every charged strike right after his Power Charge, at most one
            formation change, phases that never go back, and Nice Guy's heals
            kept apart; every boss window that opens (here and on
            knight/yukiko) is the boss's own ailment, is cured at the boss's
            next turn and opens at most once per phase; outcome reported as for
            knight.
  knight-window, yukiko-window, kanji-window
            the boss route with a generated witness-only package that gives the
            boss its window ailment at its first own turn (these routes' parties
            have no such skill): the window opens and is cured at a boss turn.
  castle    Castle 6F, 7F and 8F walks, HARD adopted (ordinary enemies keep
            their stats) through formations with an enemy healer: every enemy
            support change is consistent with its rule and thresholds.
  bathhouse Bathhouse 1F to 2F and two Bathhouse training walks, HARD adopted
            after the Castle: the run is in epoch 2 and every ordinary enemy
            hit carries its damage factor.
  party     PARTY SKILLS on and off from story0420_5f: the composed party block
            in guest RAM holds every G1-G4 learn event (Frost Dive in place of
            Chie's Assault Dive, Kanji's Hysterical Slap and Thunder Dive), each
            row stays in level order, and no other row changes.
  technical the battle harness from a HARD New Game: the hero alone, guarding
            at level 30, against encounter 69 (Phantom Mage's Fear, Heat
            Balance's Ice) until a Technical hit; every Technical hit pairs its
            element with an ailment the target had and uses the tuned factor.
  floor-bosses  the same harness against Contrarian King (encounter 526):
            every hit on the hero carries the floor boss's own damage factor.
  dive      the same harness, the protagonist casting a skill a generated
            witness-only package puts in his first slot at a lone Ice-neutral
            enemy: Frost Dive plays Rainy Death's effect and hits as Ice; two
            probe Ice skills with critical 100 show the delivery decides the
            crit roll (physical: critical; none: not).
  freeze    the battle harness from a HARD New Game with FREEZE AND SHOCK on, the hero
            against a lone enemy weak to Elec (Shock) and one weak to Ice (Freeze): the
            riders land on the enemy, never on the hero (protected), a landed Shock costs
            the enemy its turn and recovers, and the Analyze panel differs with the gate
            on and off (the Freeze and Shock columns). Not a retail comparison.
  split     PARTY KIT on and off from story0420_5f, read from the live
            SKILL rows in guest RAM: on, every family-2 multi-hit physical
            holds its power, HP cost, split (effect kind 14) and approved
            accuracy, crit and hit range, the rest native; off, every one is
            native kind 1. Every single-target physical rider (mode 1, nonzero
            non-KO mask off) holds a 45% base rate on, its mask unchanged, and
            the single-hit ones their F2 rows. Then the dive harness: Myriad Arrows lands 2..5 hits,
            and a witness-only delta gives the 0xB0 Rampage row 3 fixed hits
            at power 20, accuracy 100 and crit 0 without and with split: the
            same seeded hits, each split one about a third of its whole one.
  golden    the castle training walk with Golden mechanics on: hard mode reads
            Golden's enemy rows and scales them as its tuning names.
"""
import argparse
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mod_witness
import retail_tables

PACKAGE = "p4pc.hardmode"
PHYSSPLIT = "p4pc.physsplit"
GOLDEN = "p4pc.golden"
# Physical Split's stable leaves (its tables.md contract) and its log line, printed
# once per action the first time the session resolves it (so a route's early
# battles may already account for the boss fight's actions).
LEAF_LINE = re.compile(r"element leaf (19|20|21) resolves (skill|weapon|enemy attack) (\d+)")
WALKS = ROOT / "tests/walks"
SLOT = "BASLUS-21782Save0"
TRAINING = ("save0420_5f_to_training.walk", "38000,20:1,4/20")
RULE_LINE = re.compile(r"HARDMODE_\w+")
STATS = ("st", "ma", "en", "ag", "lu")
ENEMY_LINE = re.compile(r"HARDMODE_ENEMY id=(\d+) boss=(true|false) hp=(\d+)>(\d+) sp=(\d+)>(\d+) "
                        + " ".join(rf"{s}=(\d+)>(\d+)" for s in STATS))
REWARD_LINE = re.compile(r"HARDMODE_REWARD id=(\d+) boss=(true|false) (exp|yen)=(\d+)>(\d+)")
# One support change: inside a mixed group (HARDMODE_AI, skill IDs) or of the group itself (HARDMODE_AIGROUP).
AI_LINE = re.compile(r"HARDMODE_AI(?:GROUP)? enemy=(\d+) native=(\d+) chosen=(\d+) reason=(\w+) lowest=([\d.]+) "
                     r"highest=([\d.]+)")
TECHNICAL_LINE = re.compile(r"HARDMODE_TECHNICAL skill=(\d+) element=(\d+) status=0x([0-9a-f]+) "
                            r"target=(party|enemy):(\d+) stock=(\d+) factor=([\d.]+)")
BOSS_LINE = re.compile(r"HARDMODE_BOSS enemy=(\d+) unit=0x([0-9a-f]+) turn=(\d+) native=(\d+) chosen=(\d+) "
                       r"phase=(\d+) beat=([\w-]+)( extra)?")
WINDOW_LINE = re.compile(r"HARDMODE_WINDOW enemy=(\d+) ailment=0x([0-9a-f]+) (landed phase=(\d+)|recovered)")
# Native skill IDs the boss tactics and redesigns use (mods/p4pc.hardmode/main.luau).
POWER_CHARGE, GALE_SLASH, MIND_CHARGE, BURN_TO_ASHES = 227, 145, 228, 354
SWIFT_STRIKE, DIA, DIARAMA = 126, 192, 193
KNIGHT, YUKIKO, KANJI, PRINCE, NICE_GUY = 310, 258, 259, 269, 272
# The DatUnit ailment masks and retail element IDs the mod's [technical] names stand for.
AILMENT_MASKS = {"rage": 0x02, "fear": 0x04, "silence": 0x08, "panic": 0x10}
ELEMENT_IDS = {"fire": 1, "ice": 2, "electric": 3, "wind": 4}
# The first night: New Game events, the dream door and the tutorial battle (story_gate.sh's route and inputs).
NEWGAME_ENV = {"P4PC_AUTOPRESS_NEWGAME": "1560", "P4PC_AUTOPRESS": "cross@2300:2/150*20,start@6500:2/300*14",
               "P4PC_FACING_GATE": "1", "P4PC_BATTLE_FIRST": "down,cross,cross,cross",
               "P4PC_BATTLE_DRIVER": "cross,cross,cross", "P4PC_BATTLE_SKIP": "140"}


def tuning_sections():
    """{section: {key: text}} of the shipped tuning file."""
    text = (ROOT / "mods" / PACKAGE / "files/DATA/HARDMODE/TUNING.TXT").read_text(encoding="ascii")
    sections, section = {"": {}}, ""
    for line in text.splitlines():
        line = line.split("#", 1)[0].strip()
        header = re.fullmatch(r"\[(.+)\]", line)
        if header:
            section = header.group(1)
            sections[section] = {}
        elif "=" in line:
            key, value = (part.strip() for part in line.split("=", 1))
            sections[section][key] = value
    return sections


class Classes:
    """One difficulty's multipliers as main.luau resolves them: an enemy's own
    override (Golden rows have their own), else its epoch's (an ordinary
    enemy's hp and damage, and its exp, the epoch's hp), else its class's."""
    def __init__(self, mode, sections):
        self.mode = mode
        self.classes = {}
        for key, value in sections[mode].items():
            group, field = key.split(".")
            self.classes.setdefault(group, {})[field] = float(value)
        self.epochs = []
        while f"epoch {len(self.epochs) + 1}" in sections:
            epoch = sections[f"epoch {len(self.epochs) + 1}"]
            self.epochs.append({field: float(epoch[f"{mode}.{field}"]) for field in ("hp", "damage")}
                               | {"encounter": int(epoch["encounter"], 0)})
        self.enemy = {}
        for name, keys in sections.items():
            found = re.fullmatch(r"enemy (\d+)", name)
            if not found:
                continue
            for key, value in keys.items():
                golden, key_mode, field = re.fullmatch(r"(golden\.)?(\w+)\.(\w+)", key).groups()
                if key_mode == mode:
                    self.enemy.setdefault(int(found.group(1)), {}).setdefault(
                        "golden" if golden else "retail", {})[field] = float(value)

    def factor(self, enemy, boss, field, epoch, golden=False):
        override = self.enemy.get(int(enemy), {}).get("golden" if golden else "retail", {})
        if field in override:
            return override[field]
        if not boss and field in ("hp", "damage", "exp"):
            return self.epochs[min(epoch, len(self.epochs)) - 1]["hp" if field == "exp" else field]
        return self.classes["boss" if boss else "ordinary"][field]


def tuning():
    """({mode: Classes}, {skip_heal_above, rescue_below}) from the shipped tuning file."""
    sections = tuning_sections()
    ai = {key: float(value) for key, value in sections["ai"].items()}
    return {mode: Classes(mode, sections) for mode in ("hard", "harder")}, ai


def tuning_revision():
    """The shipped tuning file's revision, which every run line records."""
    return int(tuning_sections()[""]["revision"], 0)


def technical_tuning():
    """(factor, {element ID: ailment mask}) from the shipped [technical] section."""
    text = (ROOT / "mods" / PACKAGE / "files/DATA/HARDMODE/TUNING.TXT").read_text(encoding="ascii")
    factor, pairs, section = None, {}, None
    for line in text.splitlines():
        line = line.split("#", 1)[0].strip()
        header = re.fullmatch(r"\[(\w+)\]", line)
        if header:
            section = header.group(1)
            continue
        if section != "technical" or "=" not in line:
            continue
        key, value = (part.strip() for part in line.split("=", 1))
        if key == "factor":
            factor = float(value)
        else:
            pairs[ELEMENT_IDS[value]] = AILMENT_MASKS[key]
    return factor, pairs


def boss_tuning():
    """The shipped [bosses] section as {key: number}."""
    return {key: float(value) for key, value in tuning_sections()["bosses"].items()}


def base_turns(lines, enemy):
    """{unit: [(turn, native, chosen, phase, beat)]} of `enemy`'s base turns (no One More or ailing turn)."""
    turns = {}
    for found, unit, turn, native, chosen, phase, beat, extra in lines:
        if int(found) == enemy and not extra:
            turns.setdefault(int(unit, 16), []).append((int(turn), int(native), int(chosen), int(phase), beat))
    return turns


def check_heals(lines, enemy, spacing):
    """`enemy`'s Dia/Diarama base turns are at least `spacing` of its base turns apart."""
    for unit, turns in base_turns(lines, enemy).items():
        heals = [turn for turn, _, chosen, _, _ in turns if chosen in (DIA, DIARAMA)]
        assert all(b - a >= spacing for a, b in zip(heals, heals[1:])), (enemy, hex(unit), "heals too close", heals)


def check_tactics(text, enemy):
    """Every logged decision of `enemy` (and its supports) follows its tactic or
    redesign (design §4, mods/p4pc.hardmode/main.luau). Returns (decisions, changed)."""
    lines = BOSS_LINE.findall(text)
    mine = [line for line in lines if int(line[0]) == enemy]
    assert mine, f"no boss tactic decision was logged for enemy {enemy}"
    data = boss_tuning()
    for unit, turns in base_turns(lines, enemy).items():
        phases = [phase for _, _, _, phase, _ in turns]
        assert phases == sorted(phases), ("a phase went back", enemy, hex(unit), phases)
        assert [turn for turn, *_ in turns] == list(range(1, len(turns) + 1)), ("base turns not counted 1..n", turns)
        previous = None
        for turn, native, chosen, phase, beat in turns:
            if enemy == KNIGHT:
                cycle = int(data["knight.cycle"])
                position = turn % (cycle + 1)
                want = POWER_CHARGE if position == cycle else GALE_SLASH if position == 0 else native
                assert chosen == want, ("Knight turn off the tactic", turn, native, chosen, want)
            elif enemy == YUKIKO and chosen == BURN_TO_ASHES:
                assert previous == MIND_CHARGE, ("Burn to Ashes without her Mind Charge first", hex(unit), turn)
            elif enemy == KANJI and (chosen == SWIFT_STRIKE or beat == "finisher"):
                assert previous == POWER_CHARGE, ("a charged strike without his Power Charge first", hex(unit), turn)
            previous = chosen
        beats = [beat for *_, beat in turns]
        assert beats.count("summon") <= 1 and beats.count("formation") <= 1, ("an entry repeated", beats)
    if enemy == YUKIKO:
        check_heals(lines, PRINCE, int(data["yukiko.prince_heal_turns"]))
    elif enemy == KANJI:
        check_heals(lines, NICE_GUY, int(data["kanji.heal_turns"]))
    return len(mine), sum(1 for line in mine if line[3] != line[4])


def scaled(value, factor, low, high):
    """The script's rounding: math.floor(value * factor + 0.5), clamped; zero
    stays zero. Scripts run in the host's default float state, never the game
    thread's EE mode (lua_runtime.c enter_script_float), so this is the same
    double arithmetic as here: 10 x 1.15 is 11.5, which gives 12."""
    if factor == 1 or value == 0:
        return value
    return min(max(math.floor(value * factor + 0.5), low), high)


EPOCH_MARK = re.compile(r"HARDMODE_(?:ENCOUNTER id=\d+ boss=(?:true|false)|RUN \w+ .*?|EPOCH) epoch=(\d+)")


def verify_scaling(text, classes, golden=False):
    """Every logged enemy and reward equals native x its multiplier (Classes of
    the run's difficulty), at the epoch the log last named before it. Returns
    (enemies, rewards)."""
    enemies, rewards, epoch = [], [], 1
    for line in text.splitlines():
        mark = EPOCH_MARK.search(line)
        if mark:
            epoch = int(mark.group(1))
        row = ENEMY_LINE.search(line)
        if row:
            row = row.groups()
            enemies.append(row)
            enemy, boss = row[0], row[1] == "true"
            values = [int(v) for v in row[2:]]
            hp, new_hp, sp, new_sp = values[:4]
            factor = lambda field: classes.factor(enemy, boss, field, epoch, golden)
            assert new_hp == scaled(hp, factor("hp"), 1, 0xffff), (enemy, boss, epoch, "hp", hp, new_hp)
            assert new_sp == scaled(sp, factor("sp"), 1, 0xffff), (enemy, boss, epoch, "sp", sp, new_sp)
            for index, stat in enumerate(STATS):
                native, value = values[4 + 2 * index], values[5 + 2 * index]
                assert value == scaled(native, factor(stat), 1, 99), (enemy, boss, stat, native, value)
        reward = REWARD_LINE.search(line)
        if reward:
            enemy, boss, field, native, value = reward.groups()
            rewards.append(reward.groups())
            expected = scaled(int(native), classes.factor(enemy, boss == "true", field, epoch, golden), 0, 0xffff)
            assert int(value) == expected, (enemy, boss, field, native, value)
    unparsed = len(re.findall(r"HARDMODE_ENEMY ", text)) - len(enemies)
    assert unparsed == 0, f"{unparsed} enemy lines were not in the expected form"
    return enemies, rewards


# The final-damage stage's trace (P4PC_DAMAGE_TRACE=1): one line per applied step.
STAGE_STEP = re.compile(r"p4pc: ([\w -]+?):\s+(\S+) priority (-?\d+) load \d+ (multiply|add) ([-\d.e+]+): ")
PRESSURE_PRIORITY = 40


def pressure_steps(text, owner=PACKAGE):
    """The factors hard mode's pressure modifier applied, in order."""
    return [float(step) for stage, who, priority, kind, step in STAGE_STEP.findall(text)
            if who == owner and int(priority) == PRESSURE_PRIORITY and kind == "multiply"]


class Leg:
    """One run's configuration. Hard mode requires Physical Split ([require] in
    its mod.ini), so a leg enables both, with PHYSICAL SPLIT on, unless it says
    otherwise."""
    def __init__(self, work, name, settings, common, driver, elf, enabled=(PHYSSPLIT, PACKAGE), split="on"):
        self.dir = work / name
        self.dir.mkdir()
        self.card = self.dir / "card" / "mc0"
        self.card.parent.mkdir()
        self.name = name
        # Hard mode can only start when it and Physical Split (split on) are enabled.
        self.hardmode = PACKAGE in enabled and PHYSSPLIT in enabled and split == "on"
        self.common = common
        self.driver = driver
        self.elf = elf
        (self.dir / "p4pc.ini").write_text("[mods]\nenabled=%s\n" % ", ".join(enabled), encoding="ascii")
        store = self.dir / "mod-settings"
        store.mkdir()
        if split is not None:
            (store / (PHYSSPLIT + ".ini")).write_text(f"split = {split}\n", encoding="ascii")
        if settings:
            (store / (PACKAGE + ".ini")).write_text(
                "".join(f"{key} = {value}\n" for key, value in settings.items()), encoding="ascii")

    def settings(self):
        path = self.dir / "mod-settings" / (PACKAGE + ".ini")
        return path.read_text(encoding="ascii") if path.is_file() else ""

    def walk(self, walk, gate, timeout, stop_on=None, grace=20, extra=None, drop=(), allow_timeout=False):
        """Runs one process: returns its log text. walk/gate may be None for a
        battle-harness run (extra carries P4PC_START_BATTLE). stop_on: a regex
        whose appearance ends the run after `grace` seconds (a reload proof).
        allow_timeout: a run that never shows stop_on is returned, not failed."""
        env = dict(self.common, P4PC_CONFIG=str(self.dir / "p4pc.ini"), P4PC_SAVE_DIR=str(self.card.parent),
                   P4PC_SCENE_ARTIFACTS=str(self.dir))
        if walk is not None:
            env["P4PC_WALK_SCRIPT"] = str(WALKS / walk)
        if gate is not None:
            env["P4PC_NEWGAME_GATE"] = gate
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
        if self.hardmode:
            assert "HARDMODE_READY" in text, f"{self.name}: hard mode did not start: see {log}"
        assert "budget exhausted" not in text, f"{self.name}: an interpreter budget was exhausted: see {log}"
        # A run that showed its stop marker may end on its own before the poll
        # sees it (a refused load fails its walk and exits): that is reached.
        reached = bool(stop_on) and re.search(stop_on, text) is not None
        if not stopped and not reached:
            assert process.returncode == 0, f"{self.name} exited {process.returncode}: see {log}"
        elif stopped and not reached and not allow_timeout:
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
    hard = leg("hard", {"mode": "Hard", "adopt_on_load": "on"})
    shutil.copytree(seeds / "story0420_5f", hard.card)
    text = hard.walk(*TRAINING, timeout=2400, extra={"P4PC_DAMAGE_TRACE": "1"})
    revision = tuning_revision()
    need(text, rf"HARDMODE_TUNING revision={revision} ", "the tuning file read at start-up")
    need(text, rf"HARDMODE_RUN adopted mode=hard features=stats,ai,rewards,technical,bosses revision={revision} "
         r"adopted=true physsplit=\S+ epoch=1", "HARD adopted on load in the castle epoch")
    encounters = encounters_of(text)
    assert encounters, "no encounter was classified"
    assert all(boss == "false" for _, boss in encounters), f"the training walk met a boss encounter: {encounters}"
    enemies, rewards = verify_scaling(text, modes["hard"])
    assert enemies, "no enemy was scaled"
    assert {field for _, _, field, _, _ in rewards} == {"exp", "yen"}, f"rewards not both scaled: {rewards}"
    # Every enemy hit of an ordinary castle encounter carries the epoch's damage.
    steps = pressure_steps(text)
    want = modes["hard"].factor(0, False, "damage", 1)
    assert steps and all(abs(step - want) < 1e-9 for step in steps), f"castle pressure steps {steps[:8]} != {want}"
    assert "walk script done" in text and "SDKMEMCARD_SAVE OK" in text, "training walk/save incomplete"
    assert not re.search(r"(?m)^adopt_on_load = (on|true)$", hard.settings()), "the one-shot setting did not clear"
    print(f"mod_hardmode_game: hard: {len(encounters)} encounters, {len(enemies)} enemies, "
          f"{len(rewards)} rewards scaled exactly, {len(steps)} enemy hits x{want}")

    reload = leg("reload", {"mode": "Off"})
    shutil.copytree(hard.card, reload.card)
    text = reload.walk(*TRAINING, timeout=900, stop_on=r"HARDMODE_RUN (load|adopted) ", grace=5)
    need(text, rf"HARDMODE_RUN load mode=hard features=stats,ai,rewards,technical,bosses revision={revision} "
         r"adopted=true", "the saved HARD run restored from its own block")

    # The same HARD save with Physical Split not enabled: hard mode is refused
    # at admission, and the save, whose required blocks need it (and Physical
    # Split's own, such as p4pc:enemy-knowledge), refuses to load.
    lone = leg("reload-no-split", {"mode": "Off"}, enabled=(PACKAGE,))
    shutil.copytree(hard.card, lone.card)
    text = lone.walk(*TRAINING, timeout=900, stop_on=r"Cannot load game: ", grace=5, allow_timeout=True)
    need(text, r"mods: refused p4pc\.hardmode: .*p4pc\.physsplit 1\.0\+, which is not enabled",
         "hard mode refused without Physical Split")
    # The save's required packages are checked before its blocks (docs/modding/files.md):
    # the refusal names the package and why it was refused at startup.
    need(text, r"Cannot load game: Save requires mod p4pc\.hardmode version [0-9.]+; p4pc\.hardmode was refused "
         r"at startup: Hard mode needs p4pc\.physsplit 1\.0\+, which is not enabled", "the HARD save refused")
    assert "HARDMODE_RUN" not in text, "hard mode ran without Physical Split"

    off = leg("off", {})
    shutil.copytree(seeds / "story0420_5f", off.card)
    text = off.walk(*TRAINING, timeout=2400)
    need(text, r"HARDMODE_RESET pending=false", "a reset with nothing pending while OFF")
    need(text, r"HARDMODE_RUN load off", "an OFF load")
    rules = sorted(set(RULE_LINE.findall(text)) - {"HARDMODE_TUNING", "HARDMODE_BASELINE", "HARDMODE_READY",
                                                   "HARDMODE_RESET", "HARDMODE_RUN"})
    assert not rules, f"hard mode acted while OFF: {rules}"
    assert "walk script done" in text and "SDKMEMCARD_SAVE OK" in text, "training walk/save incomplete"

    # A settings file from before the single tier still holds Harder: it is no
    # declared choice any more and reads as the default, OFF (mod.ini help).
    later = leg("off-reload", {"mode": "Harder"})
    shutil.copytree(off.card, later.card)
    text = later.walk(*TRAINING, timeout=900, stop_on=r"HARDMODE_RUN (load|adopted) ", grace=5)
    need(text, r"HARDMODE_RESET pending=false", "a stored Harder choice reading as OFF")
    need(text, r"HARDMODE_RUN load off", "a save without a hard-mode block loading OFF")
    assert "HARDMODE_RUN new_game" not in text, "a reset that a load followed armed a New Game run"


def requires(leg, seeds, modes, ai):
    """Hard mode only runs on Physical Split with its split on: without it, or
    with PHYSICAL SPLIT off, admission refuses hard mode with the reason and
    Physical Split itself still loads."""
    for name, options, reason in (
            ("no-split-mod", {"enabled": (PACKAGE,)}, r"p4pc\.physsplit 1\.0\+, which is not enabled"),
            ("split-off", {"split": "off"}, r"Physical Split with PHYSICAL SPLIT on")):
        run = leg("requires-" + name, {"mode": "Hard", "adopt_on_load": "on"}, **options)
        shutil.copytree(seeds / "story0420_5f", run.card)
        text = run.walk(*TRAINING, timeout=900, stop_on=r"mods: refused p4pc\.hardmode", grace=5, allow_timeout=True)
        need(text, r"mods: refused p4pc\.hardmode: .*" + reason, f"hard mode refused ({name})")
        assert "HARDMODE_READY" not in text, f"hard mode started without its requirement ({name})"
        print(f"mod_hardmode_game: requires-{name}: hard mode refused as declared")


def physsplit_alone(leg, seeds, modes, ai):
    """Physical Split alone: hard mode not enabled, the split on. The Knight
    route's physical actions resolve to Slash/Strike/Pierce and no hard-mode
    rule exists in the run."""
    seed, walk, gate, encounter = KNIGHT_ROUTE
    run = leg("physsplit-alone", {}, enabled=(PHYSSPLIT,))
    shutil.copytree(seeds / seed, run.card)
    text = run.walk(walk, gate, timeout=3600, stop_on=r"end <dng_decease>", grace=10, allow_timeout=True)
    need(text, r"mods: enabled p4pc\.physsplit ", "Physical Split enabled")
    assert "HARDMODE_" not in text and "p4pc.hardmode" not in text, "hard mode ran while not enabled"
    need(text, rf"did = {encounter}\b", f"encounter {encounter}")
    leaves = LEAF_LINE.findall(text)
    assert leaves, "no physical action resolved to a Physical Split leaf on the Knight route"
    print(f"mod_hardmode_game: physsplit-alone: {len(leaves)} leaf resolutions on the route, no hard-mode rule")


def newgame(leg, seeds, modes, ai):
    run = leg("newgame", {"mode": "Hard"})
    text = run.walk("newgame_to_rain_scene.walk", "25100,-1:-1", timeout=2400, extra=NEWGAME_ENV,
                    drop=("P4PC_AUTOPRESS_LOAD",))
    need(text, r"HARDMODE_RESET pending=true", "the New Game reset marking a pending run")
    need(text, r"HARDMODE_RUN new_game mode=hard features=stats,ai,rewards,technical,bosses "
         rf"revision={tuning_revision()} adopted=false", "HARD armed by the New Game")
    armed = text.index("HARDMODE_RUN new_game")
    encounters = encounters_of(text[armed:])
    assert encounters, "the tutorial battle was not classified after arming"
    assert any(boss == "true" for _, boss in encounters), f"the tutorial formation was not boss class: {encounters}"
    enemies, rewards = verify_scaling(text, modes["hard"])
    assert any(row[1] == "true" for row in enemies), "no boss-class enemy was scaled"
    print(f"mod_hardmode_game: newgame: encounters {encounters}, {len(enemies)} enemies, {len(rewards)} rewards")


def technical(leg, seeds, modes, ai):
    """The hero guards every turn at a level that outlasts encounter 69, so its
    enemies keep acting: Phantom Mage's Fear, then Heat Balance's Ice. The
    harness builds the formation before the New Game run arms, so the
    formation is checked from the engine's own line, not the class hook."""
    factor, pairs = technical_tuning()
    run = leg("technical", {"mode": "Hard"})
    text = run.walk(None, None, timeout=900, stop_on=r"HARDMODE_TECHNICAL skill=", grace=5, allow_timeout=True,
                    extra={"P4PC_START_BATTLE": "69", "P4PC_BATTLE_HERO_LEVEL": "30", "P4PC_BATTLE_DRIVER": "up,cross"})
    need(text, r"encounter adapter: encounter 69,", "encounter 69")
    need(text, r"HARDMODE_RUN new_game mode=hard features=stats,ai,rewards,technical,bosses ",
         "HARD armed by the harness's New Game")
    lines = TECHNICAL_LINE.findall(text)
    for skill, element, status, side, target, stock, used in lines:
        assert int(status, 16) & pairs.get(int(element), 0), (skill, element, status, "no paired ailment")
        assert abs(float(used) - factor) < 0.005, (skill, used, factor)
    assert lines, "the guarded harness fight of encounter 69 produced no Technical hit"
    print(f"mod_hardmode_game: technical: {len(lines)} Technical hits {lines[:3]}")


# Each unscripted floor boss's retail ENCOUNT row: Contrarian King (304) in 526.
# Daring Gigas (311, encounter 812) takes the same [enemy] damage override, but
# its fixed script (AI_EC_FIX_GIGA) did not hit the guarding hero within the
# harness's 900 seconds (2026-11-06), so it has no leg here.
FLOOR_BOSS_ENCOUNTERS = ((526, 304),)


def floor_bosses(leg, seeds, modes, ai):
    """The battle harness from a HARD New Game: the hero alone, guarding at
    level 30, against each floor boss's encounter until it hits him: every
    hard-mode pressure step is that boss's own damage factor."""
    for encounter, enemy in FLOOR_BOSS_ENCOUNTERS:
        run = leg(f"floor-boss-{enemy}", {"mode": "Hard"})
        text = run.walk(None, None, timeout=900, stop_on=r"p4pc\.hardmode priority 40 load \d+ multiply", grace=5,
                        allow_timeout=True, extra={"P4PC_START_BATTLE": str(encounter), "P4PC_BATTLE_HERO_LEVEL": "30",
                                                   "P4PC_BATTLE_DRIVER": "up,cross", "P4PC_DAMAGE_TRACE": "1"})
        need(text, rf"encounter adapter: encounter {encounter},", f"encounter {encounter}")
        need(text, r"HARDMODE_RUN new_game mode=hard ", "HARD armed by the harness's New Game")
        steps = pressure_steps(text)
        want = modes["hard"].factor(enemy, True, "damage", 1)
        assert steps, f"enemy {enemy} never hit the guarding hero"
        assert all(abs(step - want) < 1e-9 for step in steps), (enemy, steps, want)
        print(f"mod_hardmode_game: floor-boss-{enemy}: encounter {encounter}, {len(steps)} hits x{want}")


DIVE_PROBE = "witness.hm-dive"
DIVE_HIT = re.compile(r"DIVE_HIT skill=(\d+) element=(\S+) hit_kind=(\w+) stock=(\d+)")
# The probe's own skills, both derived from Assault Dive (class 1) as Ice with
# critical 100, the one the crit roll always takes (datCalcHitResult 002411A0):
# 702 with Frost Dive's physical delivery, 703 with none (the element decides).
SURE_PHYSICAL, SURE_ELEMENT = 702, 703


def dive_probe(mods, skill):
    """A MOD_WITNESS-only package: at battle start the game's own
    datPersonaReplaceSkill (0x0010CD70) puts `skill` in the protagonist's first
    Persona skill slot (datPersonaGetSkillsByPcId 0x001094E0, pc 1), and every
    resolved party hit of it is logged (element and the game's hit kind) from
    the next frame. Its table source adds SURE_PHYSICAL and SURE_ELEMENT."""
    package = Path(mods) / DIVE_PROBE
    (package / "data").mkdir(parents=True, exist_ok=True)
    (package / "mod.ini").write_text(
        f"[mod]\nid = {DIVE_PROBE}\nname = Dive probe\nversion = 1.0.0\napi = 1.6\n"
        "requires = table-source.skill-source@1, table-source.skill-derive@1, table-source.skill-delivery@1\n"
        "priority = 0\norder = 1\n\n"
        f"[table-source {DIVE_PROBE}:skills]\nfile = data/skills.xml\nformat = p4-tables-xml\nschema = 1\nrevision = 1\n",
        encoding="ascii")
    rows = []
    for sure, delivery in ((SURE_PHYSICAL, ' delivery="physical"'), (SURE_ELEMENT, "")):
        rows += [f'<skill id="{sure}" active="1" element="2" element-aux="0" inheritance-group="2" inheritance-rate="0" '
                 f'key="{DIVE_PROBE}:sure-{sure}" derive="0x7A" critical="100" effect-source="0x7A"{delivery}/>',
                 f'<string table="1" id="{sure}" role="0" bytes="{(b"Sure Dive" + bytes(1)).hex()}"/>',
                 f'<string table="1" id="{sure}" role="1" bytes="{(b"Probe." + bytes(1)).hex()}"/>']
    (package / "data/skills.xml").write_text('<p4-tables schema="1">\n' + "\n".join(rows) + "\n</p4-tables>\n",
                                             encoding="ascii")
    (package / "main.luau").write_text(f"""--!strict
local SKILL = {skill}
local SKILLS_BY_PC = 0x001094e0 -- datPersonaGetSkillsByPcId: the record's u16 skills[8] at +0xC
local REPLACE = 0x0010cd70 -- datPersonaReplaceSkill(record, old, new)
local function signature(address: number, name: string): string
    local info = p4.describe(address)
    assert(info ~= nil, name .. " is not in the hook catalog")
    return (info :: any).signature :: string
end
local bySkills, byReplace = signature(SKILLS_BY_PC, "datPersonaGetSkillsByPcId"), signature(REPLACE, "datPersonaReplaceSkill")
local seen: {{string}} = {{}}
p4.events.on("battle_start", function()
    -- The record is native-owned: the game's own replace writes it.
    local skills = p4.call(SKILLS_BY_PC, bySkills, 1) :: number
    local first = p4.mem.read("u16", skills)
    p4.call(REPLACE, byReplace, skills - 0xC, first, SKILL)
    p4.log(string.format("DIVE_PROBE gave the protagonist %d in place of %d: now %d", SKILL, first,
        p4.mem.read("u16", skills)))
end)
p4.damage.on({{kind = "multiply", priority = -100, fn = function(hit)
    if hit.resolved and hit.source_side == "party" and hit.skill == SKILL then
        table.insert(seen, string.format("DIVE_HIT skill=%d element=%s hit_kind=%s stock=%d", hit.skill,
            tostring(hit.element), hit.hit_kind, hit.stock))
    end
    return nil
end}})
p4.events.on("frame_start", function()
    for _, line in seen do
        p4.log(line)
    end
    table.clear(seen)
end)
""", encoding="ascii")


# A retail ENCOUNT row of one lone, unflagged enemy whose UNIT2 Ice word has no
# affinity bits (unit 161, Ice-neutral). The harness drives one cast per fight.
DIVE_ENCOUNTER = 112


def dive(leg, seeds, modes, ai):
    """The battle harness from a HARD New Game: the protagonist, level 30, casts
    his first Persona skill, put there by a witness-only probe, at one
    Ice-neutral enemy. Frost Dive's cast plays Rainy Death's effect and hits as
    Ice (leaf 2). Two probe skills isolate the delivery: derived like Frost
    Dive with critical 100, the physical-delivery one is critical and the one
    without a delivery (Ice decides, so no crit roll) is not."""
    found = {}
    for skill in (FROST_DIVE, SURE_PHYSICAL, SURE_ELEMENT):
        run = leg(f"dive-{skill}", {"mode": "Hard"}, enabled=(PHYSSPLIT, PACKAGE, DIVE_PROBE))
        dive_probe(run.common["P4PC_MOD_DIR"], skill)
        text = run.walk(None, None, timeout=180, grace=3, allow_timeout=True, stop_on=rf"DIVE_HIT skill={skill} ",
                        extra={"P4PC_START_BATTLE": str(DIVE_ENCOUNTER), "P4PC_BATTLE_HERO_LEVEL": "30",
                               "P4PC_BATTLE_DRIVER": "down,cross,cross,cross"})
        refused = re.findall(r"p4pc: table-\w+ refused: .*|mods: refused .*", text)
        assert not refused, f"dive-{skill}: {refused[:3]}"
        need(text, rf"DIVE_PROBE gave the protagonist {skill} ", "the probe's skill")
        hits = [(element, kind) for hit_skill, element, kind, _ in DIVE_HIT.findall(text) if int(hit_skill) == skill]
        assert hits, f"skill {skill} never hit"
        assert all(element == "2" for element, _ in hits), f"a skill {skill} hit was not Ice: {hits[:4]}"
        found[skill] = hits[0][1]
        if skill == FROST_DIVE:
            need(text, r"skill/0?A9\.BED|SKILL\\0?A9\.BED", "Rainy Death's effect for Frost Dive")
    assert found[SURE_PHYSICAL] == "critical", f"the physical-delivery Ice hit was {found[SURE_PHYSICAL]}"
    assert found[SURE_ELEMENT] != "critical", f"the element-delivery Ice hit was {found[SURE_ELEMENT]}"
    print(f"mod_hardmode_game: dive: Frost Dive hit as Ice ({found[FROST_DIVE]}) with Rainy Death's effect; "
          f"critical-100 Ice: physical delivery {found[SURE_PHYSICAL]}, no delivery {found[SURE_ELEMENT]}")


FREEZE_PROBE = "witness.hm-freeze"
# Retail skill IDs of Thunder Reign and Niflheim: the severe single-target Elec and
# Ice skills whose riders hard mode gives the base chance 60 (data/freezeshock.xml).
THUNDER_REIGN, NIFLHEIM = 0x2E, 0x21
FREEZESHOCK_LINE = re.compile(r"HARDMODE_FREEZESHOCK (landed|cleared|turn_lost) unit=([0-9a-f]+) key=(\S+) reason=(\w+)")
GUARD_LINE = re.compile(r"HARDMODE_FREEZESHOCK guard yu unit=([0-9a-f]+) protected=(\w+) why=(\S+)")
COMMAND_LINE = re.compile(r"p4pc: battle trace command actor (\d+):(\d+) kind (\d+) skill (\d+) .* flip (\d+)")
ANALYZE_COMMAND = (10, 272)  # the battle Analyze command: kind 10, skill 0x110
SHOCK_KEY, FREEZE_KEY = "p4pc.hardmode:shock-ailment", "p4pc.hardmode:freeze-ailment"
# DatUnit affinity words in UNIT.TBL (tests/mod_element_game_test.py decode()): the high byte
# holds the flags (0x08 weak; 0x01 null, 0x02 repel, 0x04 drain and 0x20 no-effect refuse a
# status), the word index is the retail element leaf (Ice 2, Electric 3).
WEAK_WORD, REFUSING_WORD = 0x0800, 0x2700


def weak_encounter(iso, leaf):
    """A durable ordinary lone retail enemy weak to `leaf`, read from the disc."""
    files = retail_tables.battle_files(iso, ["UNIT.TBL", "ENCOUNT.TBL"])
    unit = retail_tables.blocks(files["UNIT.TBL"])
    encount = files["ENCOUNT.TBL"]
    records = struct.unpack_from("<I", encount, 0)[0] // 0x18
    candidates = []
    for record in range(records):
        base = 4 + record * 0x18
        enemies = [enemy for enemy in struct.unpack_from("<5H", encount, base + 8) if enemy]
        # Ordinary formations carry 1 in the first word.
        if struct.unpack_from("<I", encount, base)[0] != 1 or len(enemies) != 1 or enemies[0] >= 256:
            continue
        enemy = enemies[0]
        word = struct.unpack_from("<H", unit[1], enemy * 32 + 2 * leaf)[0]
        if unit[0][enemy * 60 + 3] <= 15 and word & WEAK_WORD and not word & REFUSING_WORD:
            hp = struct.unpack_from("<H", unit[0], enemy * 60 + 4)[0]
            candidates.append((hp, -record))
    if candidates:
        return -max(candidates)[1]
    raise AssertionError(f"no lone ordinary retail enemy weak to element {leaf}")


def freeze_probe(mods, gate, skill=None, land=False, analyze=False):
    """A MOD_WITNESS-only package beside hard mode. gate: it holds the side-ailment gate
    from boot, so a New Game that arms inside the harness battle (the native gate changes
    only outside battle) finds it on, as the real flow does by arming at a field; skill:
    the game's own datPersonaReplaceSkill puts it in the protagonist's first Persona skill
    at battle start (the dive probe's route); land: at the first frame with live units it
    lands Shock on the first enemy through p4.status.set; analyze: at the first commands
    panel it chooses Analyze for the actor."""
    package = Path(mods) / FREEZE_PROBE
    package.mkdir(parents=True, exist_ok=True)
    (package / "mod.ini").write_text(
        f"[mod]\nid = {FREEZE_PROBE}\nname = Freeze probe\nversion = 1.0.0\napi = 1.6\n"
        "requires = status@1, battle.menu@1\npriority = 0\norder = 1\n", encoding="ascii")
    lua = lambda value: str(value).lower() if isinstance(value, bool) else "nil" if value is None else str(value)
    (package / "main.luau").write_text(f"""local GATE, SKILL, LAND, ANALYZE = {lua(gate)}, {lua(skill)}, {lua(land)}, {lua(analyze)}
local KEY = "{SHOCK_KEY}"
local SKILLS_BY_PC = 0x001094e0 -- datPersonaGetSkillsByPcId: the record's u16 skills[8] at +0xC
local REPLACE = 0x0010cd70 -- datPersonaReplaceSkill(record, old, new)
local function signature(address, name)
    local info = p4.describe(address)
    assert(info ~= nil, name .. " is not in the hook catalog")
    return info.signature
end
local landed, analyzed = false, false
if GATE then
    local ok, why = p4.status.enable(true)
    p4.log("FREEZE_PROBE gate ok=" .. tostring(ok) .. " why=" .. tostring(why))
end
p4.events.on("battle_start", function()
    landed, analyzed = false, false
    if SKILL then
        local skills = p4.call(SKILLS_BY_PC, signature(SKILLS_BY_PC, "datPersonaGetSkillsByPcId"), 1)
        local first = p4.mem.read("u16", skills)
        p4.call(REPLACE, signature(REPLACE, "datPersonaReplaceSkill"), skills - 0xC, first, SKILL)
        p4.log(string.format("FREEZE_PROBE gave the protagonist %d in place of %d: now %d", SKILL, first,
            p4.mem.read("u16", skills)))
    end
end)
p4.events.on("frame_start", function()
    if LAND and not landed then
        local units = p4.battle.units()
        if units ~= nil and #units.enemies > 0 then
            local target = units.enemies[1].data
            local ok, why = false, nil
            local called = pcall(function() ok, why = p4.status.set(target, KEY) end)
            if called and ok then
                landed = true
                p4.log(string.format("FREEZE_PROBE landed unit=%x", target))
            end
        end
    end
    if ANALYZE and not analyzed then
        local menu = p4.battle.menu()
        if menu ~= nil and menu.panel == "commands" then
            local ok, why = p4.battle.choose("analyze")
            analyzed = ok
            p4.log("FREEZE_PROBE analyze ok=" .. tostring(ok) .. " why=" .. tostring(why))
        end
    end
end)
""", encoding="ascii")


def freeze(leg, seeds, modes, ai):
    """The battle harness from a HARD New Game with FREEZE AND SHOCK on, the hero against a
    lone retail enemy weak to the element (the encounter is read from the user's disc):
      shock    Thunder Reign (put in his first skill slot by a witness-only probe) lands Shock
               on the Elec-weak enemy through hard mode's own rider (HARDMODE_FREEZESHOCK
               landed ... reason=rider), on the enemy and never on the hero, after hard mode
               logged its protection of the hero's unit (guard yu protected=true);
      ice      the same with Niflheim and an Ice-weak enemy (Freeze);
      turn     the probe lands Shock (reason mod) on the enemy while the hero guards: the
               enemy loses its next turn (turn_lost) and recovers (cleared, reason recovered);
      analyze  with FREEZE AND SHOCK on and off, the hero chooses Analyze at his first
               command; the run is replayed with captures around that command, and the on
               and off captures must differ (the Freeze and Shock Analyze columns of
               data/freezeshock.xml exist only while the side-ailment gate is on). The
               captures stay in the work directory for review.
    The probe holds the native side-ailment gate from boot: the harness's New Game arms hard
    mode inside the battle, where the gate cannot change (a real New Game arms at a field)."""
    features = "stats,ai,rewards,technical,bosses,freeze_shock"
    on_settings = {"mode": "Hard", "freeze_shock": "on"}

    def fight(name, leaf, probe, settings, extra, stop_on, grace=5):
        run = leg(name, settings, enabled=(PHYSSPLIT, PACKAGE, FREEZE_PROBE))
        freeze_probe(run.common["P4PC_MOD_DIR"], **probe)
        encounter = weak_encounter(run.common["P4PC_ISO"], leaf)
        text = run.walk(None, None, timeout=120, grace=grace, allow_timeout=True,
                        stop_on=rf"(?:{stop_on})|btlMain state -> 17",
                        extra={"P4PC_START_BATTLE": str(encounter), "P4PC_BATTLE_HERO_LEVEL": "15",
                               "P4PC_BATTLE_HERO_HP": "9999", **extra})
        need(text, rf"encounter adapter: encounter {encounter},", f"{name}: encounter {encounter}")
        return run, text

    def checked(name, text):
        """The run has FREEZE AND SHOCK, the gate and the hero's protection held, and no
        side-ailment event touched the hero. Returns the events as (kind, unit, key, reason)."""
        need(text, rf"HARDMODE_RUN new_game mode=hard features={features} ", f"{name}: FREEZE AND SHOCK in the run")
        refused = re.findall(r"HARDMODE_FREEZESHOCK gate refused.*|p4pc: table-\w+ refused: .*|mods: refused .*", text)
        assert not refused, f"{name}: {refused[:3]}"
        guards = GUARD_LINE.findall(text)
        assert guards and all(protected == "true" for _, protected, _ in guards), f"{name}: hero guard {guards}"
        hero = {unit for unit, _, _ in guards}
        events = FREEZESHOCK_LINE.findall(text)
        assert not any(unit in hero for _, unit, _, _ in events), f"{name}: a side-ailment event on the hero {events}"
        return events

    # Hard mode's own riders, against the weak enemy of each element.
    for name, leaf, skill, key in (("shock", 3, THUNDER_REIGN, SHOCK_KEY), ("ice", 2, NIFLHEIM, FREEZE_KEY)):
        landed = rf"HARDMODE_FREEZESHOCK landed unit=[0-9a-f]+ key={re.escape(key)} reason=rider"
        run, text = fight(f"freeze-{name}", leaf, {"gate": True, "skill": skill}, on_settings,
                          {"P4PC_BATTLE_DRIVER": "down,cross,cross,cross"}, landed)
        need(text, rf"FREEZE_PROBE gave the protagonist {skill} ", f"freeze-{name}: the probe's skill")
        need(text, landed, f"freeze-{name}: a rider landing of {key}")
        checked(f"freeze-{name}", text)
    # The lost turn and its recovery, deterministically: a landing (mod) on an enemy that acts.
    recovered = rf"HARDMODE_FREEZESHOCK cleared unit=[0-9a-f]+ key={re.escape(SHOCK_KEY)} reason=recovered"
    run, text = fight("freeze-turn", 3, {"gate": True, "land": True}, on_settings, {"P4PC_BATTLE_DRIVER": "up,cross"},
                      recovered)
    events = [(kind, reason) for kind, _, key, reason in checked("freeze-turn", text) if key == SHOCK_KEY]
    assert events[:3] == [("landed", "mod"), ("turn_lost", "none"), ("cleared", "recovered")], events
    # The Analyze columns: the same fight with the gate on and off, captured around the Analyze command.
    captures = {}
    for name, settings in (("on", on_settings), ("off", {"mode": "Hard"})):
        probe = {"gate": name == "on", "analyze": True}
        opened = r"battle trace command actor \d+:\d+ kind 10 skill 272"
        first, text = fight(f"freeze-analyze-{name}", 3, probe, settings,
                            {"P4PC_BATTLE_TRACE": "1", "P4PC_AUTOPRESS": "cross@600:6"}, opened, grace=3)
        flips = [int(flip) for _, _, kind, skill, flip in COMMAND_LINE.findall(text)
                 if (int(kind), int(skill)) == ANALYZE_COMMAND]
        need(text, r"FREEZE_PROBE analyze ok=true", f"freeze-analyze-{name}: the Analyze request")
        assert flips, f"freeze-analyze-{name}: the hero never opened Analyze"
        frames = ",".join(str(frame) for frame in range(flips[0] - 40, flips[0] + 1200, 20))
        second = f"freeze-analyze-{name}-capture"
        run = leg(second, settings, enabled=(PHYSSPLIT, PACKAGE, FREEZE_PROBE))
        freeze_probe(run.common["P4PC_MOD_DIR"], **probe)
        text = run.walk(None, None, timeout=900, grace=120, allow_timeout=True, stop_on=opened,
                        extra={"P4PC_START_BATTLE": str(weak_encounter(run.common["P4PC_ISO"], 3)),
                               "P4PC_BATTLE_HERO_LEVEL": "15", "P4PC_BATTLE_HERO_HP": "9999",
                               "P4PC_BATTLE_TRACE": "1", "P4PC_AUTOPRESS": "cross@600:6",
                               "P4PC_CAPTURE_FRAMES": frames, "P4PC_CAPTURE_PNG": str(run.dir / "capture_%d.png")})
        replayed = [int(flip) for _, _, kind, skill, flip in COMMAND_LINE.findall(text)
                    if (int(kind), int(skill)) == ANALYZE_COMMAND]
        assert replayed[:1] == flips[:1], f"{second}: the capture replay diverged ({replayed[:1]} != {flips[:1]})"
        shots = sorted(run.dir.glob("capture_*.png"), key=lambda path: int(path.stem.split("_")[1]))
        assert shots, f"{second}: no capture was written"
        captures[name] = shots
    differing = sum(a.read_bytes() != b.read_bytes() for a, b in zip(captures["on"], captures["off"]))
    assert differing, "the gate-on and gate-off Analyze captures are identical: no Freeze or Shock column"
    print(f"mod_hardmode_game: freeze: Shock and Freeze riders landed on the weak enemies, hero guarded, "
          f"turn lost and recovered; Analyze captures {len(captures['on'])} on / {len(captures['off'])} off, "
          f"{differing} differ")


def golden(leg, seeds, modes, ai):
    """Golden mechanics enabled with HARD (the player's own Golden install, found
    as p4pc.golden finds it): hard mode reads Golden's active groups, so its
    baseline is Golden's enemy rows; the castle training walk's enemies and
    rewards scale as the tuning names for Golden rows, and every ordinary enemy
    hit carries the first epoch's damage."""
    run = leg("golden", {"mode": "Hard", "adopt_on_load": "on"}, enabled=(PHYSSPLIT, GOLDEN, PACKAGE))
    shutil.copytree(seeds / "story0420_5f", run.card)
    text = run.walk(*TRAINING, timeout=2400, extra={"P4PC_DAMAGE_TRACE": "1"})
    need(text, r"mods: enabled p4pc\.golden ", "Golden mechanics enabled")
    need(text, r"HARDMODE_BASELINE enemies=golden", "hard mode on Golden's enemy rows")
    need(text, r"HARDMODE_RUN adopted mode=hard .* epoch=1\b", "HARD adopted in the castle epoch")
    enemies, rewards = verify_scaling(text, modes["hard"], golden=True)
    steps = pressure_steps(text)
    want = modes["hard"].factor(0, False, "damage", 1, golden=True)
    assert all(abs(step - want) < 1e-9 for step in steps), f"golden pressure steps {steps[:8]} != {want}"
    assert "walk script done" in text or "end <dng_decease>" in text, "golden training walk incomplete"
    print(f"mod_hardmode_game: golden: {len(encounters_of(text))} encounters, {len(enemies)} enemies, "
          f"{len(rewards)} rewards scaled on Golden rows, {len(steps)} enemy hits x{want}")


def window_tuning():
    """{"chance", "enemy": {id: ailment mask}} from the shipped [windows] section."""
    section = tuning_sections()["windows"]
    windows = {"enemy": {}}
    for key, value in section.items():
        if key.startswith("enemy."):
            windows["enemy"][int(key[6:])] = int(value, 0)
        else:
            windows[key] = float(value)
    return windows


def check_windows(text, enemy):
    """Every window that opened is the boss's own ailment and opened at most once
    per phase, and every cure follows its landing (the boss's next turn).
    Returns (landed, recovered)."""
    windows = window_tuning()
    landed, recovered, open_ = [], 0, False
    for found, ailment, event, phase in WINDOW_LINE.findall(text):
        if int(found) != enemy:
            continue
        assert int(ailment, 16) == windows["enemy"].get(enemy), ("a window opened on another ailment", enemy, ailment)
        if event == "recovered":
            assert open_, ("a cure without a landing", enemy)
            recovered, open_ = recovered + 1, False
        else:
            assert not open_, ("a second landing while a window was held", enemy)
            landed.append(int(phase))
            open_ = True
    assert len(landed) == len(set(landed)), ("a second window in one phase", enemy, landed)
    return len(landed), recovered


WINDOW_PROBE = "witness.hm-window"


def window_probe(mods, enemy, mask):
    """A MOD_WITNESS-only package: at the boss's first own turn it sets the
    boss's window ailment directly, as a landed hit would, so the window's
    detection, cure and guard run on a route whose party has no such skill."""
    package = Path(mods) / WINDOW_PROBE
    package.mkdir(parents=True, exist_ok=True)
    # Order -1: the probe's hook runs before hard mode's on the same boss turn,
    # so the window is detected and cured at that turn, before an ailment
    # (Fear's own dialogue) can hold the boss's next one.
    (package / "mod.ini").write_text(
        f"[mod]\nid = {WINDOW_PROBE}\nname = Window probe\nversion = 1.0.0\napi = 1.6\npriority = 0\norder = -1\n",
        encoding="ascii")
    (package / "main.luau").write_text(f"""--!strict
local given = false
p4.hook(0x001de640, "vppH", "before", function(context)
    if given then return end
    local actor = p4.mem.read("u32", (context.args[1] :: number) + 0x30)
    if actor == 0 or p4.mem.read("u8", actor + 0xA2) ~= 1 then return end
    local unit = p4.mem.read("u32", actor + 0xA64)
    if unit == 0 or p4.mem.read("u16", unit + 2) ~= {enemy} then return end
    p4.mem.write("u32", unit + 0xC, bit32.bor(p4.mem.read("u32", unit + 0xC), {mask:#x}))
    given = true
    p4.log("WINDOW_PROBE enemy={enemy} ailment={mask:#x}")
end)
""", encoding="ascii")


def boss(name, seed, walk, gate, encounter, enemy, timeout, probe=False):
    """Boss class and scaling in the real fight. These routes drive the fight's
    menus open-loop and are proven only on the retail flow, so the outcome is
    reported, not asserted: the -neutral control proves the hooks themselves
    change no enemy, and the simulator cohort measures difficulty. With the
    window probe, the boss's window must open and wear off."""
    def scenario(leg, seeds, modes, ai):
        run = leg(name, {"mode": "Hard", "adopt_on_load": "on"},
                  enabled=(PHYSSPLIT, PACKAGE) + ((WINDOW_PROBE,) if probe else ()))
        if probe:
            window_probe(run.common["P4PC_MOD_DIR"], enemy, window_tuning()["enemy"][enemy])
        shutil.copytree(seeds / seed, run.card)
        # A searched battle tape (the Kanji route) is bound to retail frames: on
        # HARD it diverges, an outcome like a loss. The divergence shows as a
        # failed walk, a new-game gate failure at the tape's frame bound, or a
        # command the game refuses (a heal at full HP, "Limit reached.") that
        # the tape repeats until that gate fails.
        resolved = (rf"end <dng_decease>|HARDMODE_REWARD id={enemy} boss=true yen=|WALK SCRIPT FAILED"
                    r"|new-game gate FAIL")
        text = run.walk(walk, gate, timeout=timeout, stop_on=resolved, grace=10,
                        extra={"P4PC_CHANCE_TRACE": "1"})
        need(text, r"HARDMODE_RUN adopted mode=hard ", "HARD adopted on load")
        start = need(text, rf"HARDMODE_ENCOUNTER id={encounter} boss=true", f"encounter {encounter} as boss class").start()
        enemies, rewards = verify_scaling(text, modes["hard"])
        assert any(row[0] == str(enemy) and row[1] == "true" for row in enemies), f"enemy {enemy} not scaled as boss"
        assert "HARDMODE_AI" not in text[start:], "the support rule changed a boss-encounter pick"
        refused = re.findall(r"HARDMODE_BOSS enemy=\d+ .* refused=(.*)", text[start:])
        assert not refused, f"the enemy AI carrier refused a boss tactic: {refused[:3]}"
        decisions, changed = check_tactics(text[start:], enemy)
        landed, recovered = check_windows(text[start:], enemy)
        guards = len(re.findall(rf"HARDMODE_GUARD enemy={enemy} ", text[start:]))
        if probe:
            need(text, rf"WINDOW_PROBE enemy={enemy} ", "the probe's ailment")
            assert landed, f"{name}: the probed window did not open"
        leaves = LEAF_LINE.findall(text)
        assert leaves, "no physical action resolved to a Physical Split leaf on the route"
        won = "dng_decease" not in text[start:] and re.search(rf"HARDMODE_REWARD id={enemy} boss=true exp=", text[start:])
        diverged = re.search(r"WALK SCRIPT FAILED|new-game gate FAIL", text[start:])
        outcome = "won" if won else "diverged" if diverged else "lost"
        print(f"mod_hardmode_game: {name}: {len(encounters_of(text))} encounters, {len(enemies)} enemies, "
              f"{len(rewards)} rewards, {decisions} boss decisions ({changed} by the tactic), "
              f"{landed} windows opened, {recovered} cured, {guards} guard decisions, "
              f"{len(leaves)} leaf resolutions; open-loop route {outcome} on HARD")
    return scenario


FALL_PROBE = "witness.hm-fall"
HERO_HP = 0x007973CC     # the protagonist's DatUnit record (0x007973C4) HP, u16
BATTLE_CONTEXT = 0x0076449C  # gp-0x4C54: the battle context, zero once the battle is released


def fall_probe(mods, encounter):
    """A MOD_WITNESS-only package: at the first battle start of `encounter` it
    leaves Yu one HP, after the port's battle-start snapshot, so the guarding
    party falls on the boss's first hit. A retry restores the snapshot (Yu's HP
    before the probe); the probe never writes again."""
    package = Path(mods) / FALL_PROBE
    package.mkdir(parents=True, exist_ok=True)
    (package / "mod.ini").write_text(
        f"[mod]\nid = {FALL_PROBE}\nname = Fall probe\nversion = 1.0.0\napi = 1.6\npriority = 0\norder = 1\n",
        encoding="ascii")
    (package / "main.luau").write_text(f"""--!strict
local given = false
p4.events.on("battle_start", function(event)
    if given or event.setup == 0 then return end
    local group = p4.mem.read("u32", event.setup + 0x14)
    if group == 0 or p4.mem.read("u16", group + 8) ~= {encounter} then return end
    given = true
    p4.log("FALL_PROBE encounter={encounter} hp=" .. tostring(p4.mem.read("u16", {HERO_HP:#x})) .. ">1")
    p4.call(0x001056e0, "vhh", 1, 1) -- datSetUnitHp, as used by p4pc.challenge
end)
""", encoding="ascii")


def boss_retry(name, seed, walk, gate, encounter, timeout, give_up=False):
    """BATTLE RETRY in the real fight: HARD adopted on the boss route, the fall
    probe leaves Yu one HP at the boss's battle start and every command is
    Guard, so the party falls. The defeat must be offered before the Game Over.
    RETRY (Cross on the default row) must restart the same encounter where it
    was lost with the battle-start HP/SP and no Game Over; GIVE UP (Down, Cross)
    must let the retail Game Over (dng_decease) run."""
    def scenario(leg, seeds, modes, ai):
        run = leg(name, {"mode": "Hard", "adopt_on_load": "on", "battle_retry": "on"},
                  enabled=(PHYSSPLIT, PACKAGE, FALL_PROBE))
        fall_probe(run.common["P4PC_MOD_DIR"], encounter)
        shutil.copytree(seeds / seed, run.card)
        # The route up to the boss fight (its driver off before the first command
        # menu), then Guard until Yu falls, the battle's release, and time for
        # the prompt (its first 30 polls take no input).
        source = (WALKS / walk).read_text(encoding="ascii").splitlines()
        cut = source.index("waitbtl:menu")
        answer = (["press:down", "wait:30", "press:cross", "wait:1800"] if give_up
                  else ["press:cross", "waitbtl:menu", "log", "wait:120"])
        derived = run.dir / f"{name}.walk"
        derived.write_text("\n".join(source[:cut] + [
            # waitbtl advances the door dialogue as well as waiting for the menu.
            "waitbtl:menu",
            "battle:first=circle,circle,circle,up,cross;plan=cross;skip=0;delay=80",
            f"waitstate16:{HERO_HP:#x}==0",
            "battle:off",
            f"waitstate:{BATTLE_CONTEXT:#x}==0",
            "wait:600",
        ] + answer) + "\n", encoding="ascii")
        stop = r"end <dng_decease>" if give_up else r"walk script done"
        # The original gate checks the post-victory return to the entrance;
        # this witness deliberately remains in the boss fight instead.
        text = run.walk(derived, None, timeout=timeout, stop_on=stop, grace=10)
        need(text, r"HARDMODE_RUN adopted mode=hard .* retry=true", "HARD adopted with BATTLE RETRY")
        probe = need(text, rf"FALL_PROBE encounter={encounter} hp=(\d+)>1", "the fall probe")
        start_hp = int(probe.group(1))
        offered = need(text, rf"HARDMODE_RETRY offered encounter={encounter} mode=hard source=field "
                             rf"party=1:{start_hp}/(\d+)", "the retry offered with the battle-start vitals")
        after = text[offered.end():]
        need(text, rf"p4pc: mods: battle defeat: encounter {encounter} \(field\) answered by owner \d+, retry allowed",
             "the native defeat seam")
        if give_up:
            declined = need(after, rf"HARDMODE_RETRY declined encounter={encounter}", "GIVE UP")
            need(after[declined.end():], r"end <dng_decease>", "the retail Game Over after GIVE UP")
            assert "HARDMODE_RETRY accepted" not in text and "retried from its battle start" not in text, \
                f"{name}: a declined defeat was retried"
            print(f"mod_hardmode_game: {name}: defeat at encounter {encounter} offered, declined, Game Over ran")
            return
        accepted = need(after, rf"HARDMODE_RETRY accepted encounter={encounter} party=1:{start_hp}/", "RETRY")
        need(after, rf"p4pc: mods: battle defeat: encounter {encounter} retried from its battle start", "the native retry")
        restarted = need(after, rf"HARDMODE_RETRY restarted encounter={encounter} party=1:{start_hp}/\d+\S* matches=true",
                         "the retried battle starting with the battle-start HP/SP")
        assert "dng_decease" not in text, f"{name}: the Game Over ran after RETRY"
        assert "HARDMODE_REWARD" not in after[:restarted.start()], f"{name}: the lost battle paid a reward"
        assert len(re.findall(rf"FALL_PROBE encounter={encounter} ", text)) == 1
        print(f"mod_hardmode_game: {name}: defeat at encounter {encounter} offered, retried from its start "
              f"(Yu {start_hp} HP restored), the fight restarted without a Game Over")
    return scenario


def control(name, seed, walk, gate, encounter, timeout):
    """The same walk with hard mode OFF (Physical Split on, as hard mode needs it):
    the open-loop route's reference outcome for the `-neutral` leg, reported."""
    def scenario(leg, seeds, modes, ai):
        run = leg(name + "-off", {})
        shutil.copytree(seeds / seed, run.card)
        # With only the split on, a searched tape can leave its line too.
        ended = r"end <dng_decease>|walk script done|WALK SCRIPT FAILED|new-game gate FAIL"
        text = run.walk(walk, gate, timeout=timeout, stop_on=ended, grace=10)
        need(text, r"HARDMODE_RUN load off", "an OFF load")
        assert "HARDMODE_ENCOUNTER" not in text, f"{name}-off: hard mode acted while OFF"
        start = need(text, rf"did = {encounter}\b", f"encounter {encounter}").start()
        won = "dng_decease" not in text[start:] and "walk script done" in text
        diverged = re.search(r"WALK SCRIPT FAILED|new-game gate FAIL", text[start:])
        outcome = "won" if won else "diverged" if diverged else "lost"
        print(f"mod_hardmode_game: {name}-off: open-loop route {outcome} with the split on")
    return scenario


def neutral(name, seed, walk, gate, encounter, enemy, timeout):
    """HARD armed exactly as in `boss`, from a staged copy whose HARD multipliers
    are all 1.00 and with BOSS PLAY off: every scaling and AI hook runs and
    changes no enemy. Physical Split (required by hard mode) is on in every leg,
    so the open-loop route's reference outcome is the `-off` control's under the
    same split, which can lose; the outcome is reported, the unchanged enemies
    are asserted."""
    def scenario(leg, seeds, modes, ai):
        run = leg(name + "-neutral", {"mode": "Hard", "adopt_on_load": "on", "boss_play": "off"})
        package = run.dir.parent / "mods-neutral"
        if not package.exists():
            shutil.copytree(run.dir.parent / "mods", package)
            path = package / PACKAGE / "files" / "DATA" / "HARDMODE" / "TUNING.TXT"
            text = path.read_text(encoding="ascii")
            head, section, rest = text.partition("[hard]")
            body, marker, tail = rest.partition("[harder]")
            body = re.sub(r"(?m)^(\w+\.\w+) = [0-9.]+$", r"\1 = 1.00", body)
            # The epochs' and the per-enemy HARD values too (Golden rows' as well).
            tail = re.sub(r"(?m)^((?:golden\.)?hard\.\w+) = [0-9.]+$", r"\1 = 1.00", tail)
            path.write_text(head + section + body + marker + tail, encoding="ascii")
        shutil.copytree(seeds / seed, run.card)
        resolved = rf"end <dng_decease>|HARDMODE_REWARD id={enemy} boss=true yen="
        text = run.walk(walk, gate, timeout=timeout, stop_on=resolved, grace=10,
                        extra={"P4PC_MOD_DIR": str(package)})
        need(text, r"HARDMODE_RUN adopted mode=hard ", "HARD adopted on load")
        start = need(text, rf"HARDMODE_ENCOUNTER id={encounter} boss=true", f"encounter {encounter} as boss class").start()
        enemies = ENEMY_LINE.findall(text)
        assert enemies, "no enemy was logged"
        for row in enemies:
            values = [int(v) for v in row[2:]]
            assert values[0::2] == values[1::2], (name, "neutral tuning changed an enemy", row)
        won = "dng_decease" not in text[start:] and re.search(rf"HARDMODE_REWARD id={enemy} boss=true exp=", text[start:])
        print(f"mod_hardmode_game: {name}-neutral: hooks armed at neutral tuning, enemies unchanged; "
              f"open-loop route {'won' if won else 'lost'}")
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
        need(text, r"HARDMODE_RUN adopted mode=hard features=stats,ai,rewards,technical,bosses ", "HARD adopted on load")
        verify_scaling(text, modes["hard"])
        # The retail route is open-loop: on HARD its party may fall (dng_decease)
        # before the walk's end; the support changes met until then still count.
        assert "walk script done" in text or "end <dng_decease>" in text, f"castle {floor}: the walk did not finish"
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


# (name, seed, walk, gate): Bathhouse 1F to 2F, the qualified Bathhouse training
# from the Kanji glimpse, and the Bathhouse training of 5/21 (three Daring Gigas,
# the ordinary enemy 156; the floor boss 311 is floor_bosses' leg).
BATHHOUSE = (("bathhouse-2f", "story0520bathhouse1f", "save0520bathhouse1f_to_bathhouse_2f.walk", "32000,20:1,5/20"),
             ("bathhouse-training", "story0520glimpse", "save0520glimpse_to_training.walk", "26000,20:1,5/20"),
             ("bathhouse-gigas", "story0521tv", "save0521tv_to_gigas_training.walk", "22000,20:1,5/21"))


def bathhouse(leg, seeds, modes, ai):
    """HARD adopted after the Castle on three Bathhouse routes: the run starts
    in the second epoch and every ordinary enemy hit carries that epoch's damage.
    Each route ends at its walk's end or its gate."""
    want = modes["hard"].factor(0, False, "damage", 2)
    hits = 0
    for name, seed, walk, gate in BATHHOUSE:
        run = leg(name, {"mode": "Hard", "adopt_on_load": "on"})
        shutil.copytree(seeds / seed, run.card)
        text = run.walk(walk, gate, timeout=2400, extra={"P4PC_DAMAGE_TRACE": "1"})
        need(text, r"HARDMODE_RUN adopted mode=hard .* epoch=2\b", "HARD adopted on load in the Bathhouse epoch")
        enemies, rewards = verify_scaling(text, modes["hard"])
        steps = pressure_steps(text)
        assert all(abs(step - want) < 1e-9 for step in steps), f"{name} pressure steps {steps[:8]} != {want}"
        assert "walk script done" in text or "new-game gate PASS" in text, f"{name} walk incomplete"
        hits += len(steps)
        print(f"mod_hardmode_game: {name}: {len(encounters_of(text))} encounters, {len(enemies)} enemies, "
              f"{len(rewards)} rewards scaled at epoch 2, {len(steps)} enemy hits x{want}")
    assert hits, "no ordinary enemy hit on the Bathhouse routes"


# The composed party block (PERSONA.TBL block 2) the native learn scan reads:
# datPersonaPreviewLevelUp 0x0010BE60 through the pointer at 0x007644D4. Rows
# are personas 0xC0.. in order; learn events at +4 are {u8 level, s8 kind (1
# learn, 2 forget), u16 skill}, 32 of them.
PARTY_POINTER, RAM_BASE = 0x007644D4, 0x00100000
PARTY_ROW, PARTY_ROWS, PARTY_FIRST = 0x26E, 24, 0xC0
ASSAULT_DIVE, FROST_DIVE, THUNDER_DIVE = 0x7A, 700, 701
# Hard mode's learn events, (skill, level) per persona (data/party.xml).
PARTY_LEARNS = {0xC0: ((0x0F, 23),), 0xC1: ((0x0F, 23),), 0xC2: ((FROST_DIVE, 22),), 0xC3: ((FROST_DIVE, 22),),
                0xC4: ((0x05, 34),), 0xC5: ((0x05, 34),),
                0xC6: ((0x9D, 0), (THUNDER_DIVE, 28), (0x2B, 36)), 0xC7: ((0x9D, 0), (THUNDER_DIVE, 28), (0x2B, 36))}


def learnsets(ram):
    """{persona: [(level, kind, skill), ...]} of the party block in a RAM dump."""
    pointer = struct.unpack_from("<I", ram, PARTY_POINTER - RAM_BASE)[0]
    assert RAM_BASE <= pointer and pointer - RAM_BASE + PARTY_ROWS * PARTY_ROW <= len(ram), \
        f"the party block pointer {pointer:#x} is outside guest RAM"
    rows = {}
    for row in range(PARTY_ROWS):
        events = []
        for index in range(32):
            level, kind, skill = struct.unpack_from("<BbH", ram, pointer - RAM_BASE + row * PARTY_ROW + 4 + 4 * index)
            if not kind:
                break
            events.append((level, kind, skill))
        rows[PARTY_FIRST + row] = events
    return rows


def party(leg, seeds, modes, ai):
    """PARTY KIT on and off, read from the composed party block in guest RAM:
    on, every G1-G4 learn event is there, Chie's Assault Dive is Frost Dive and
    each row stays sorted by level; every other row equals the off run's."""
    found = {}
    for name, settings in (("party", {}), ("party-off", {"party_kit": "off"})):
        run = leg(name, {"mode": "Hard", "adopt_on_load": "on", **settings})
        shutil.copytree(seeds / "story0420_5f", run.card)
        text = run.walk(*TRAINING, timeout=900, stop_on=r"HARDMODE_RUN adopted", grace=30,
                        extra={"P4PC_DUMP_RAM": str(run.dir / "ram"), "P4PC_DUMP_RAM_AT": "2000"})
        refused = re.findall(r"p4pc: table-(?:delta|derived) refused: .*", text)
        assert not refused, f"{name}: {refused[:3]}"
        dumps = sorted(run.dir.glob("ram_*.bin"))
        assert dumps, f"{name}: no RAM dump at flip 2000"
        found[name] = learnsets(dumps[-1].read_bytes())
        for dump in dumps:
            dump.unlink()  # 31 MB each
    on, off = found["party"], found["party-off"]
    for persona, learns in PARTY_LEARNS.items():
        for skill, level in learns:
            assert (level, 1, skill) in on[persona], (hex(persona), "missing", skill, level, on[persona])
        assert [level for level, _, _ in on[persona]] == sorted(level for level, _, _ in on[persona]), \
            (hex(persona), "learn events out of level order", on[persona])
    for persona in (0xC2, 0xC3):
        assert ASSAULT_DIVE not in [skill for _, _, skill in on[persona]], (hex(persona), "still learns Assault Dive")
        assert (22, 1, ASSAULT_DIVE) in off[persona], (hex(persona), "the off run lacks retail Assault Dive at 22")
    for persona in on:
        if persona not in PARTY_LEARNS:
            assert on[persona] == off[persona], (hex(persona), "a row hard mode does not name changed")
    changed = sum(on[p] != off[p] for p in on)
    print(f"mod_hardmode_game: party: {changed} learnsets changed as data/party.xml names, none else; "
          f"Chie learns Frost Dive at 22, Kanji Hysterical Slap at 0 and Thunder Dive at 28")


SKILL_POINTER, SKILL_ROW = 0x007644A8, 0x28  # iGpffffb3b8, the live SKILL 1 rows
# Hard mode's family-2 multi-hit rows (data/rebalance.xml): skill -> (power, HP %,
# accuracy, crit, fewest hits, most hits); None keeps the native field.
SPLIT_ROWS = {
    0x7C: (55, 6, None, None, None, None), 0x7D: (55, 6, None, None, None, None),
    0x7F: (55, 6, None, None, None, None), 0x9D: (45, 6, None, None, None, None),
    0xA1: (45, 6, None, None, None, None), 0xA5: (45, 6, None, None, None, None),
    0x82: (115, 11, None, None, None, None), 0xAB: (55, 8, None, None, None, None),
    0x7E: (50, 8, None, None, None, None), 0xA2: (55, 15, None, None, None, None),
    0x85: (75, 15, None, None, None, None), 0xAC: (220, 22, None, None, None, None),
    0xAD: (330, 28, None, None, None, None), 0xAA: (330, 28, None, None, None, None),
    0x89: (100, 11, 95, 10, 1, 4), 0x8B: (160, 22, 87, 5, 2, 5),
    0x87: (195, 22, 80, 5, None, None), 0x88: (160, 22, 85, 10, None, None),
}
# Family 5's single-target physical riders: the 45% base ailment rate (SKILL
# +0x19) on these rows, and the single-hit ones' F2 rows: skill -> (power, HP %,
# accuracy, crit); None keeps the native field.
RIDER_RATE = 45
RIDER_ROWS = {
    0x97: (45, 6, None, None), 0x9B: (45, 6, None, None), 0x9F: (45, 6, None, None),
    0xA3: (85, 11, None, None), 0x98: (75, 11, 95, 5),
}
RIDER_SKILLS = (*RIDER_ROWS, 0x9D, 0xA1, 0xA5)
# Family 2's single-hit all-target ailment physicals: retain the composed
# row's entire rider, hit/crit and delivery, including in the disabled run.
AREA_RIDER_ROWS = {
    0x99: (140, 22), 0x9A: (125, 22),
    0x9C: (70, 15), 0x9E: (70, 15), 0xA0: (70, 15),
    0xA4: (70, 15), 0xA6: (70, 15),
}
SPLIT_PROBE = "witness.hm-split"
SPLIT_HITS = re.compile(r"SPLIT_HIT skill=(\d+) stocks=([\d,]+)")
MYRIAD_ARROWS, OTHER_RAMPAGE = 0x8B, 0xB0  # 0xB0: the 300-power Rampage row hard mode does not name


def skill_rows(ram, skills):
    """{skill: fields} of the live SKILL rows in a RAM dump."""
    pointer = struct.unpack_from("<I", ram, SKILL_POINTER - RAM_BASE)[0]
    assert RAM_BASE <= pointer and pointer - RAM_BASE + 0x1B8 * SKILL_ROW <= len(ram), \
        f"the SKILL row pointer {pointer:#x} is outside guest RAM"
    rows = {}
    for skill in skills:
        row = ram[pointer - RAM_BASE + skill * SKILL_ROW:][:SKILL_ROW]
        rows[skill] = {"cost": struct.unpack_from("<H", row, 0x04)[0], "accuracy": row[0x0E], "hits": (row[0x0F], row[0x10]),
                       "effect": row[0x11], "power": struct.unpack_from("<h", row, 0x12)[0], "critical": row[0x25],
                       "status": (row[0x18], struct.unpack_from("<I", row, 0x1C)[0] & 0xFFFFFF), "rate": row[0x19]}
    return rows


def split_probe(mods, skill, split=None):
    """A MOD_WITNESS-only package: at battle start the game's own
    datPersonaReplaceSkill puts `skill` in the protagonist's first Persona skill
    slot (as dive_probe does), and each frame logs the stocks of the resolved
    party hits of it the previous frame computed: one action's hits on a lone
    target. With split 0 or 1, its own table delta gives the 0xB0 Rampage row
    power 20, accuracy 100, crit 0 and exactly 3 hits (no hit-count roll), and
    that split."""
    package = Path(mods) / SPLIT_PROBE
    (package / "data").mkdir(parents=True, exist_ok=True)
    manifest = (f"[mod]\nid = {SPLIT_PROBE}\nname = Split probe\nversion = 1.0.0\napi = 1.6\n"
                "requires = manifest.table-delta@2\npriority = 0\norder = 1\n")
    if split is not None:
        manifest += "\n[table-delta probe]\nfile = data/probe.xml\nrevision = 1\n"
        (package / "data/probe.xml").write_text(
            f'<p4-table-deltas schema="1">\n<skill-delta id="{OTHER_RAMPAGE:#x}" power="20" accuracy="100" '
            f'critical="0" min-hits="3" max-hits="3" split="{split}"/>\n</p4-table-deltas>\n', encoding="ascii")
    (package / "mod.ini").write_text(manifest, encoding="ascii")
    (package / "main.luau").write_text(f"""--!strict
local SKILL = {skill}
local SKILLS_BY_PC = 0x001094e0 -- datPersonaGetSkillsByPcId: the record's u16 skills[8] at +0xC
local REPLACE = 0x0010cd70 -- datPersonaReplaceSkill(record, old, new)
local function signature(address: number, name: string): string
    local info = p4.describe(address)
    assert(info ~= nil, name .. " is not in the hook catalog")
    return (info :: any).signature :: string
end
local bySkills, byReplace = signature(SKILLS_BY_PC, "datPersonaGetSkillsByPcId"), signature(REPLACE, "datPersonaReplaceSkill")
local stocks: {{string}} = {{}}
p4.events.on("battle_start", function()
    local skills = p4.call(SKILLS_BY_PC, bySkills, 1) :: number
    local first = p4.mem.read("u16", skills)
    p4.call(REPLACE, byReplace, skills - 0xC, first, SKILL)
    p4.log(string.format("SPLIT_PROBE gave the protagonist %d in place of %d: now %d", SKILL, first,
        p4.mem.read("u16", skills)))
end)
p4.damage.on({{kind = "multiply", priority = -100, fn = function(hit)
    if hit.resolved and hit.source_side == "party" and hit.skill == SKILL then
        table.insert(stocks, tostring(hit.stock))
    end
    return nil
end}})
p4.events.on("frame_start", function()
    if #stocks > 0 then
        p4.log(string.format("SPLIT_HIT skill=%d stocks=%s", SKILL, table.concat(stocks, ",")))
        table.clear(stocks)
    end
end)
""", encoding="ascii")


def split(leg, seeds, modes, ai):
    """PARTY KIT's skill rebalance, multi-hit physicals: the live rows on and off, then the
    dive harness for Myriad Arrows' range and a same-seed split comparison."""
    found = {}
    for name, settings in (("split-rows", {}), ("split-rows-off", {"party_kit": "off"})):
        run = leg(name, {"mode": "Hard", "adopt_on_load": "on", **settings})
        shutil.copytree(seeds / "story0420_5f", run.card)
        text = run.walk(*TRAINING, timeout=900, stop_on=r"HARDMODE_RUN adopted", grace=30,
                        extra={"P4PC_DUMP_RAM": str(run.dir / "ram"), "P4PC_DUMP_RAM_AT": "2000"})
        refused = re.findall(r"p4pc: table-(?:delta|derived) refused: .*", text)
        assert not refused, f"{name}: {refused[:3]}"
        dumps = sorted(run.dir.glob("ram_*.bin"))
        assert dumps, f"{name}: no RAM dump at flip 2000"
        found[name] = skill_rows(dumps[-1].read_bytes(), {**SPLIT_ROWS, **RIDER_ROWS, **AREA_RIDER_ROWS})
        for dump in dumps:
            dump.unlink()  # 31 MB each
    on, off = found["split-rows"], found["split-rows-off"]
    for skill, (power, cost, accuracy, critical, fewest, most) in SPLIT_ROWS.items():
        row, native = on[skill], off[skill]
        assert native["effect"] == 1, (hex(skill), "the off run's row is not formula damage", native)
        want = {"power": power, "cost": cost, "effect": 14,
                "accuracy": native["accuracy"] if accuracy is None else accuracy,
                "critical": native["critical"] if critical is None else critical,
                "hits": native["hits"] if fewest is None else (fewest, most),
                "status": native["status"], "rate": RIDER_RATE if skill in RIDER_SKILLS else native["rate"]}
        assert row == want, (hex(skill), "live row", row, "expected", want)
    for skill, (power, cost, accuracy, critical) in RIDER_ROWS.items():
        row, native = on[skill], off[skill]
        want = {**native, "power": power, "cost": cost, "rate": RIDER_RATE,
                "accuracy": native["accuracy"] if accuracy is None else accuracy,
                "critical": native["critical"] if critical is None else critical}
        assert row == want, (hex(skill), "live row", row, "expected", want)
    for skill in RIDER_SKILLS:
        mode, mask = off[skill]["status"]
        assert mode == 1 and mask and not mask & 0x80000, (hex(skill), "the off run's row has no rolled ailment",
                                                           off[skill])
    for skill, (power, cost) in AREA_RIDER_ROWS.items():
        row, native = on[skill], off[skill]
        mode, mask = native["status"]
        assert native["effect"] == 1 and native["hits"] == (1, 1), (hex(skill), native)
        assert mode in (1, 3) and mask and not mask & 0x80000, (hex(skill), "missing rolled ailment", native)
        assert row == {**native, "power": power, "cost": cost}, (hex(skill), row, native)

    def cast(name, skill, split_value=None):
        run = leg(name, {"mode": "Hard"}, enabled=(PHYSSPLIT, PACKAGE, SPLIT_PROBE))
        split_probe(run.common["P4PC_MOD_DIR"], skill, split_value)
        text = run.walk(None, None, timeout=180, grace=3, allow_timeout=True, stop_on=rf"SPLIT_HIT skill={skill} ",
                        extra={"P4PC_START_BATTLE": str(DIVE_ENCOUNTER), "P4PC_BATTLE_HERO_LEVEL": "30",
                               "P4PC_BATTLE_DRIVER": "down,cross,cross,cross"})
        refused = re.findall(r"p4pc: table-\w+ refused: .*|mods: refused .*", text)
        assert not refused, f"{name}: {refused[:3]}"
        need(text, rf"SPLIT_PROBE gave the protagonist {skill} ", "the probe's skill")
        batches = [[int(stock) for stock in stocks.split(",")]
                   for hit_skill, stocks in SPLIT_HITS.findall(text) if int(hit_skill) == skill]
        assert batches, f"{name}: skill {skill} never hit"
        return batches[0]

    myriad = cast("split-myriad", MYRIAD_ARROWS)
    assert 2 <= len(myriad) <= 5, f"Myriad Arrows landed {len(myriad)} hits on its target: {myriad}"
    whole = cast("split-whole", OTHER_RAMPAGE, 0)
    parts = cast("split-parts", OTHER_RAMPAGE, 1)
    assert len(whole) == len(parts) == 3, f"fixed 3-hit row: whole {whole}, split {parts}"
    for full, part in zip(whole, parts):
        assert part >= 1 and 2.5 * part <= full <= 3.5 * part, f"split hit {part} is not a third of {full}"
    print(f"mod_hardmode_game: split: {len(SPLIT_ROWS)} multi-hit rows live as approved and native off; "
          f"Myriad Arrows hit {len(myriad)} times {myriad}; 3-hit row whole {whole}, split {parts}")


# (seed, walk, gate, encounter) routes: encounter 811 is Avenger Knight (DatUnit
# enemy KNIGHT, 310), 514 is Shadow Yukiko (enemy YUKIKO, 258, who summons
# Charming Prince 269).
KNIGHT_ROUTE = ("story0423tv", "save0423tv_to_knight.walk", "42000,20:1,4/23", 811)
YUKIKO_ROUTE = ("story0426trained5", "save0426trained5_to_yukiko_rescue.walk", "55000,6:6,4/27", 514)
# Encounter 515: Shadow Kanji (KANJI, 259) with Nice Guy 272 and Tough Guy 273; the
# collection card's searched battle tape (tests/cmake/180-story.cmake).
KANJI_ROUTE = ("playthrough0520bathhouse11", "collection0520_bathhouse11_to_kanji_victory.walk", "34000,24:2,5/20", 515)
SCENARIOS = {
    "core": core,
    "requires": requires,
    "physsplit-alone": physsplit_alone,
    "newgame": newgame,
    "knight": boss("knight", *KNIGHT_ROUTE, KNIGHT, 3600),
    "knight-off": control("knight", *KNIGHT_ROUTE, 3600),
    "knight-neutral": neutral("knight", *KNIGHT_ROUTE, KNIGHT, 3600),
    "knight-retry": boss_retry("knight-retry", *KNIGHT_ROUTE, 3600),
    "knight-giveup": boss_retry("knight-giveup", *KNIGHT_ROUTE, 3600, give_up=True),
    "yukiko": boss("yukiko", *YUKIKO_ROUTE, YUKIKO, 3600),
    "yukiko-off": control("yukiko", *YUKIKO_ROUTE, 3600),
    "yukiko-neutral": neutral("yukiko", *YUKIKO_ROUTE, YUKIKO, 3600),
    "kanji": boss("kanji", *KANJI_ROUTE, KANJI, 3600),
    "kanji-off": control("kanji", *KANJI_ROUTE, 3600),
    "knight-window": boss("knight-window", *KNIGHT_ROUTE, KNIGHT, 3600, probe=True),
    "yukiko-window": boss("yukiko-window", *YUKIKO_ROUTE, YUKIKO, 3600, probe=True),
    "kanji-window": boss("kanji-window", *KANJI_ROUTE, KANJI, 3600, probe=True),
    "castle": castle,
    "bathhouse": bathhouse,
    "party": party,
    "technical": technical,
    "floor-bosses": floor_bosses,
    "dive": dive,
    "freeze": freeze,
    "split": split,
    "golden": golden,
}
SEEDS = {"core": ("story0420_5f",), "requires": ("story0420_5f",), "physsplit-alone": KNIGHT_ROUTE[:1],
         "newgame": (), "technical": (), "floor-bosses": (), "dive": (), "freeze": (), "split": ("story0420_5f",),
         "golden": ("story0420_5f",),
         "castle": tuple(seed for _, seed, _, _ in CASTLE),
         "bathhouse": tuple(seed for _, seed, _, _ in BATHHOUSE), "party": ("story0420_5f",),
         **{name + suffix: route[:1] for name, route in (("knight", KNIGHT_ROUTE), ("yukiko", YUKIKO_ROUTE),
                                                         ("kanji", KANJI_ROUTE))
            for suffix in ("", "-off", "-neutral", "-window")},
         "knight-retry": KNIGHT_ROUTE[:1], "knight-giveup": KNIGHT_ROUTE[:1]}


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
        shutil.copytree(ROOT / "mods" / PHYSSPLIT, mods / PHYSSPLIT)
        if (ROOT / "mods" / GOLDEN).is_dir():
            shutil.copytree(ROOT / "mods" / GOLDEN, mods / GOLDEN)
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

        def leg(name, settings, **options):
            return Leg(work, name, settings, common, args.driver, elf, **options)

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
