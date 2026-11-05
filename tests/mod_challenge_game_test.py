#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS proof of the bundled p4pc.challenge
rules in the running game; not a retail comparison and not balance evidence.

Every scenario has its own settings directory, so each has its own run ledger.
Rules are enabled the way a player does it: declared [setting] values in
mod-settings/p4pc.challenge.ini beside p4pc.ini. Ordinary private seed saves
become CUSTOM runs through the mod's one-shot "start from the next loaded save"
setting. Witness-only fixture mods (written privately into the work directory,
never shipped) observe native calls and, where a scenario needs it, force a
condition: test.challenge.boost multiplies battle EXP so the level cap is
reached, test.challenge.frail knocks Yu out as a battle starts so it is lost.

Scenarios:
  camp      New Game through the menu harness: every rule armed, Camp status
            rows captured (menu_challenge_camp.png).
  training  story0420_5f, castle training walk: Camp Medicine/Dia refused (no
            field recovery), the battle Item command refused, EXP clamped at
            the castle cap, the run saved once (limited saves).
  loss      the training save: its unclean predecessor counts one Ironman
            recovery, the fixture loses a battle, Game Over locks the run.
  locked    the same save again: the native load refuses it.
  fusion    story0422personas: the Velvet Room offers no fusion kind, the
            compendium summon still happens, no fusion commits.
  severe    story0420_5f, castle training walk: enemy hits x1.5, party hits
            x0.75, an extra x1.25 on weakness/critical, recomputed from the
            logged stock amounts.
  mandatory story0420_5f, castle training walk: ordinary Shadows are refused
            both when the player attacks and when they touch; no battle starts.
  quota     story0422 through four save walks: saves 1..3 publish, the fourth
            is refused before any slot byte is written.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mod_witness

PACKAGE = "p4pc.challenge"
WALKS = ROOT / "tests/walks"
SLOT = "BASLUS-21782Save0"
ALL_RULES = ("ironman", "ironman_confirm", "boss_cap", "no_fusion", "solo=YU", "no_battle_items",
             "no_field_recovery", "limited_saves")

FIXTURES = {
    # Observes the native seams the rules act on. Reads only.
    "test.challenge.observe": """--!strict
p4.hook(0x0030f4f0, "vpp", "before", function()
    p4.log("TEST_FUSION_COMMIT")
end)
p4.hook(0x0010b190, "ip", "after", function(context)
    p4.log("TEST_ROSTER_INSERT persona=" .. tostring(p4.mem.read("u16", (context.args[1] :: number) + 2)))
end)
p4.events.on("battle_start", function()
    p4.log("TEST_BATTLE_START")
end)
p4.events.on("battle_end", function(event)
    local yu = p4.call(0x00104c70, "ii", 1) :: number
    local yosuke = p4.call(0x00109400, "BH", 2) :: number
    local chie = p4.call(0x00109400, "BH", 3) :: number
    p4.log(string.format("TEST_LEVELS outcome=%d yu=%d yosuke=%d chie=%d", event.outcome, yu, yosuke, chie))
end)
p4.log("TEST_OBSERVE_READY")
""",
    # Multiplies each recipient's base battle EXP, so the cap clamps real awards.
    "test.challenge.boost": """--!strict
-- btlXpComputeAward(level, reward): one recipient's battle EXP (0x0021e050 is the money award).
p4.hook(0x0021de90, "iip", "replace", function(context)
    local award = context.next() :: number
    context.result = math.min(0xffff, award * 60)
end)
p4.log("TEST_BOOST_READY")
""",
    # Leaves Yu, Yosuke and Chie at 1 HP when a battle starts.
    "test.challenge.frail": """--!strict
-- Yu enters the battle already knocked out (his record's HP is 0), so the
-- battle's own defeat check ends it as a loss; Yosuke and Chie stay at 1 HP.
p4.events.on("battle_start", function()
    p4.call(0x001056e0, "vhh", 1, 0)
    for id = 2, 3 do p4.call(0x001056e0, "vhh", id, 1) end
    p4.log("TEST_FRAIL")
end)
p4.log("TEST_FRAIL_READY")
""",
}


def write_fixtures(root):
    for name, source in FIXTURES.items():
        package = root / name
        package.mkdir(parents=True)
        (package / "mod.ini").write_text(
            f"[mod]\nid = {name}\nname = Challenge witness fixture\nversion = 1.0.0\napi = 1.3\npriority = 0\norder = 0\n",
            encoding="ascii")
        (package / "main.luau").write_text(source, encoding="utf-8")
    shutil.copytree(ROOT / "mods" / PACKAGE, root / PACKAGE)


class Scenario:
    def __init__(self, work, name, rules, fixtures, common, driver, elf):
        self.dir = work / name
        self.dir.mkdir()
        self.card = self.dir / "card" / "mc0"
        self.name = name
        self.common = common
        self.driver = driver
        self.elf = elf
        config = self.dir / "p4pc.ini"
        config.write_text("[mods]\nenabled=%s\n" % ",".join((PACKAGE,) + tuple(fixtures)), encoding="utf-8")
        store = self.dir / "mod-settings"
        store.mkdir()
        (store / (PACKAGE + ".ini")).write_text("".join(
            "%s = %s\n" % tuple(rule.split("=")) if "=" in rule else f"{rule} = on\n" for rule in rules), encoding="ascii")
        self.config = config

    def seed(self, seed):
        shutil.copytree(seed, self.card)

    def settings(self):
        return (self.dir / "mod-settings" / (PACKAGE + ".ini")).read_text(encoding="ascii")

    def keep(self, name):
        """Copies the card, settings and lifecycle records aside as `name`."""
        for part in ("card", "mod-settings", "mod-lifecycle"):
            if (self.dir / part).is_dir():
                shutil.copytree(self.dir / part, self.dir / name / part)

    def restore(self, name):
        for part in ("card", "mod-settings", "mod-lifecycle"):
            shutil.rmtree(self.dir / part, ignore_errors=True)
            if (self.dir / name / part).is_dir():
                shutil.copytree(self.dir / name / part, self.dir / part)

    def run(self, label, extra, timeout, stop_on=None, grace=20, stop_signal=None):
        """Runs one process; returns its log text. stop_on: a regex whose
        appearance ends the run after `grace` seconds (a Game Over or refused
        load never reaches a walk's end), by SIGKILL or `stop_signal`."""
        env = dict(self.common, P4PC_CONFIG=str(self.config), P4PC_SAVE_DIR=str(self.card.parent),
                   P4PC_SCENE_ARTIFACTS=str(self.dir), **extra)
        log = self.dir / (label + ".log")
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
                    if stop_signal is not None and seen_at is not None:
                        process.send_signal(stop_signal)
                        try:
                            process.wait(timeout=60)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            process.wait()
                    else:
                        process.kill()
                        process.wait()
                    stopped = True
        text = log.read_text(encoding="utf-8", errors="replace")
        if stop_on and not stopped and re.search(stop_on, text):
            # The awaited line came, then the process ended on its own before the
            # grace ran out (a walk whose next step can no longer happen).
            stopped = seen_at = True
        assert "run_mode=mod-witness" in text and "reference_eligible=false" in text, f"{label}: not a witness run"
        assert "CHALLENGE_READY" in text, f"{label}: challenge mod did not start; see {log}"
        if not stopped:
            assert process.returncode == 0, f"{label} exited {process.returncode}; see {log}"
        elif not stop_on or seen_at is None:
            raise AssertionError(f"{label}: timed out after {timeout}s; see {log}")
        print(f"mod_challenge_game: {self.name}/{label} ok ({'stopped' if stopped else 'exited'})")
        return text

    def walk(self, label, walk, gate, timeout=2400, **kwargs):
        return self.run(label, {"P4PC_WALK_SCRIPT": str(WALKS / walk), "P4PC_NEWGAME_GATE": gate}, timeout, **kwargs)


def need(text, pattern, what):
    match = re.search(pattern, text)
    assert match, f"missing {what}: /{pattern}/"
    return match


def camp(s):
    text = s.run("camp", {"P4PC_START_MENU": "7:2", "P4PC_MENU_EXIT": "1", "P4PC_DRAW_WINDOW": "10",
                          "P4PC_MENU_SCRIPT": "open,wait90,shot:challenge_camp"}, 900)
    need(text, r"CHALLENGE_ARMED run=R1 rules=ironman,boss_cap,no_fusion,solo,no_battle_items,no_field_recovery,limited_saves "
         r"custom=false epoch=1 solo=1", "New Game arming of every rule")
    need(text, r"CHALLENGE_STATUS CHALLENGE 7 RULES / IRONMAN RECOVERY x0 SOLO YU CAP 15 SAVES 3", "Camp status rows")
    need(text, r"script finished: camp opened, 1 shots, 0 dark", "Camp capture")
    assert (s.dir / "menu_challenge_camp.png").is_file(), "Camp capture missing"
    assert re.search(r"(?m)^run\.r1 = s:A\|0\|0\|O\d+\|1\|0\|-$", s.settings()), "ledger entry for R1"


def training(s):
    text = s.walk("training", "save0420_5f_to_training.walk", "38000,20:1,4/20")
    need(text, r"CHALLENGE_ARMED run=R1 rules=ironman,boss_cap,no_battle_items,no_field_recovery,limited_saves custom=true epoch=1",
         "CUSTOM continuation arming")
    need(text, r"FIELD_RECOVERY_REFUSED item=\d+ effect=\d+", "Camp item refusal")
    need(text, r"FIELD_RECOVERY_REFUSED skill=\d+", "Camp skill refusal")
    need(text, r"BATTLE_ITEMS_REFUSED", "battle Item command refusal")
    need(text, r"CAP_AWARD persona=\d+ cap=15 requested=\d+ allowed=\d+", "prepared battle awards clamped")
    caps = re.findall(r"CAP_(?:PERSONA|HERO|HERO_LEVEL|AWARD persona=\d+) cap=(\d+) requested=(\d+) allowed=(\d+)", text)
    assert caps and all(cap == "15" and int(allowed) < int(requested) for cap, requested, allowed in caps), caps
    levels = [tuple(int(v) for v in m) for m in re.findall(r"TEST_LEVELS outcome=1 yu=(\d+) yosuke=(\d+) chie=(\d+)", text)]
    assert levels and all(max(level) <= 15 for level in levels), f"level above the cap: {levels}"
    assert max(max(level) for level in levels) == 15, f"the boosted EXP never reached the cap: {levels}"
    assert "walk script done" in text and "SDKMEMCARD_SAVE OK" in text, "training walk/save incomplete"
    need(text, r"CHALLENGE_SAVED run=R1 gen=1 saves=1", "published save")
    sidecar = (s.card / SLOT / ".p4mods").read_bytes()
    assert PACKAGE.encode() in sidecar, "save does not carry the challenge block"
    shutil.copytree(s.card, s.dir / "card-after-training")
    return levels


def loss(s):
    # After Game Over the title's loads of this run are refused, so the walk
    # never reaches its gate: the run ends shortly after the tombstone.
    text = s.walk("loss", "challenge_entrance_to_loss.walk", "999999,20:1,4/20", stop_on=r"IRONMAN_LOSS",
                  grace=5, timeout=2400)
    # The training process ended without an orderly quit: a default-mode recovery.
    need(text, r"CHALLENGE_LOADED run=R1 rules=\S+ gen=1 epoch=1 saves=1 recoveries=1 recovered=true", "counted recovery")
    need(text, r"CHALLENGE_STATUS CHALLENGE 5 RULES CUSTOM / IRONMAN RECOVERY x1 CAP 15 SAVES 2", "status after load")
    need(text, r"TEST_FRAIL", "fixture")
    need(text, r"CHALLENGE_DEFEAT run=R1", "battle defeat")
    need(text, r"IRONMAN_LOSS run=R1 via=\w+ locked=true", "durable tombstone at Game Over")
    assert re.search(r"(?m)^run\.r1 = s:D\|1\|1\|C\|1\|1\|-$", s.settings()), "ledger tombstone"


def locked(s):
    shutil.rmtree(s.card)
    shutil.copytree(s.dir / "card-after-training", s.card)
    text = s.walk("locked", "challenge_entrance_to_loss.walk", "999999,20:1,4/20",
                  stop_on=r"Cannot load game", grace=5, timeout=900)
    need(text, r"Cannot load game: .*IRONMAN run R1 ended in defeat\. Its saves are locked, not erased\.", "native load refusal")
    assert "CHALLENGE_LOADED run=R1" not in text, "a locked run was loaded"
    assert (s.card / SLOT / SLOT).is_file(), "the locked save must not be erased"


STRICT_RULES = ("ironman", "ironman_confirm", "strict_ironman", "strict_confirm", "custom_on_load")


def strict_training(s):
    text = s.walk("training", "save0420_5f_to_training.walk", "38000,20:1,4/20")
    need(text, r"CHALLENGE_ARMED run=R1 rules=ironman custom=true epoch=1 solo=0 strict=true", "STRICT IRONMAN arming")
    opened = re.findall(r"STRICT_BATTLE_OPEN run=R1 battle=(B\d+\.\d+)", text)
    closed = re.findall(r"STRICT_BATTLE_CLOSED run=R1 battle=(B\d+\.\d+) via=(\w+)", text)
    assert opened and [battle for battle, _ in closed] == opened, f"every battle opens and resolves: {opened} {closed}"
    assert all(via == "victory" for _, via in closed), closed
    need(text, r"CHALLENGE_STATUS CHALLENGE 1 RULES CUSTOM / STRICT IRONMAN RECOVERY x0", "STRICT Camp marker")
    need(text, r"CHALLENGE_SAVED run=R1 gen=1", "published save")
    assert re.search(r"(?m)^run\.r1 = s:A\|1\|0\|O\d+\|1\|0\|-$", s.settings()), s.settings()
    s.keep("after-training")


def strict_exit(s, label, stop_signal):
    """Loads the run and ends the process while its first battle is open."""
    s.restore("after-training")
    # The walk leaves the battle waiting at its first command menu, unresolved.
    text = s.walk(label, "challenge_entrance_to_battle_menu.walk", "999999,20:1,4/20",
                  stop_on=r"walk script (\d+)/\1: wait:60000", grace=2, stop_signal=stop_signal, timeout=2400)
    assert "STRICT_BATTLE_CLOSED" not in text, "the battle resolved before the process ended"
    need(text, r"CHALLENGE_LOADED run=R1 rules=ironman gen=1 epoch=1 saves=0 recoveries=1 recovered=true",
         "out-of-battle unclean exit counted once")
    return need(text, r"STRICT_BATTLE_OPEN run=R1 battle=(B\d+\.\d+)", "open battle record")[1], text


def strict_kill(s):
    battle, _ = strict_exit(s, "kill", None)
    text = s.walk("after-kill", "challenge_entrance_to_loss.walk", "999999,20:1,4/20", stop_on=r"Cannot load game",
                  grace=5, timeout=900)
    need(text, r"IRONMAN_LOSS run=R1 via=unclean_exit_in_battle battle=%s crash=false locked=true" % re.escape(battle),
         "a killed open battle ends the run")
    need(text, r"Cannot load game: .*STRICT IRONMAN run R1 ended: the game exited during a battle without a recorded crash",
         "native load refusal")
    assert "CHALLENGE_LOADED run=R1" not in text
    assert re.search(r"(?m)^run\.r1 = s:U\|1\|1\|C\|1\|0\|-$", s.settings()), s.settings()


def strict_crash(s):
    battle, text = strict_exit(s, "crash", signal.SIGSEGV)
    need(text, r"fatal signal; writing native crash report", "the port's crash handler ran")
    markers = list((s.dir / "mod-lifecycle").glob("crash-*.marker"))
    assert any(("p4pc.challenge\tR1:%s\n" % battle).encode() in m.read_bytes() for m in markers), markers
    text = s.walk("after-crash", "challenge_entrance_to_loss.walk", "999999,20:1,4/20", stop_on=r"CHALLENGE_LOADED",
                  grace=5, timeout=900)
    need(text, r"STRICT_RECOVERY run=R1 battle=%s recoveries=2" % re.escape(battle), "a recorded crash is a counted recovery")
    need(text, r"CHALLENGE_LOADED run=R1 rules=ironman gen=1 epoch=1 saves=0 recoveries=2 recovered=false",
         "counted once, and the run loads")
    assert not any(("R1:%s" % battle).encode() in m.read_bytes() for m in (s.dir / "mod-lifecycle").glob("crash-*.marker")), \
        "the consumed crash record cannot be reused"


def fusion(s):
    # The walk expects the retail fusion rows; once they are gone its later
    # inputs no longer match, so the run ends at the walk's end, never at its gate.
    shots = (1550, 1600, 1650, 1700, 1750)
    text = s.run("fusion", {"P4PC_WALK_SCRIPT": str(WALKS / "save0422personas_to_omoikane.walk"),
                            "P4PC_NEWGAME_GATE": "999999,20:1,4/22",
                            "P4PC_CAPTURE_FRAMES": ",".join(str(f) for f in shots),
                            "P4PC_CAPTURE_PNG": str(s.dir / "fusion_%d.png")}, 1800,
                 stop_on=r"walk script (done|failed)", grace=0)
    need(text, r"CHALLENGE_ARMED run=R1 rules=no_fusion custom=true", "CUSTOM no-fusion run")
    refused = re.findall(r"p4pc: mods: fusion refused by owner \d+: NO FUSION challenge rule", text)
    assert refused, "the Velvet Room fusion rows were not refused"
    assert "TEST_FUSION_COMMIT" not in text, "a fusion committed"
    assert all((s.dir / ("fusion_%d.png" % f)).is_file() for f in shots), "Velvet Room captures missing"


def solo(s):
    text = s.walk("solo", "save0420_5f_to_training.walk", "38000,20:1,4/20", timeout=2400,
                  stop_on=r"walk script (done|failed)|CHALLENGE_DEFEAT", grace=10)
    need(text, r"CHALLENGE_ARMED run=R1 rules=solo custom=true epoch=\d+ solo=1", "CUSTOM solo-Yu run")
    projected = re.findall(r"p4pc: mods: battle lineup projected by owner \d+: (\d) -> 1 members", text)
    assert projected and all(int(n) >= 2 for n in projected), f"no party battle was cut to Yu alone: {projected}"
    need(text, r"SOLO_LINEUP member=YU from=1,\d", "the walk's party lineup reached the projector")
    need(text, r"TEST_BATTLE_START", "a projected battle started")
    return text


def severe(s):
    text = s.walk("severe", "save0420_5f_to_training.walk", "38000,20:1,4/20", timeout=2400,
                  stop_on=r"walk script (done|failed)|CHALLENGE_DEFEAT|CHALLENGE_NEW_GAME armed=0", grace=10)
    if "CHALLENGE_RULE_UNAVAILABLE rule=severe_damage" in text:
        # A build without the damage stage never arms the rule it cannot enforce.
        need(text, r"CHALLENGE_NEW_GAME armed=0", "no run armed without its only rule")
        assert "SEVERE_DAMAGE" not in text and "CHALLENGE_ARMED" not in text
        print("mod_challenge_game: severe_damage unavailable in this build; refused to arm")
        return
    need(text, r"CHALLENGE_ARMED run=R1 rules=severe_damage custom=true", "CUSTOM severe-damage run")
    hits = re.findall(r"SEVERE_DAMAGE from_enemy=(true|false) kind=(\d+) stock=(\d+) scaled=(\d+)", text)
    assert {enemy for enemy, *_ in hits} == {"true", "false"}, f"both directions must be observed: {hits}"
    for enemy, kind, stock, scaled in hits:
        numerator, denominator = (3, 2) if enemy == "true" else (3, 4)
        if kind in ("2", "4"):
            numerator, denominator = numerator * 5, denominator * 4
        assert int(scaled) == max(1, int(stock) * numerator // denominator), (enemy, kind, stock, scaled)


def mandatory(s):
    # The walk waits for the training battles it expects; once they are refused
    # its battle wait times out (the player stays in control on the field).
    text = s.walk("mandatory", "save0420_5f_to_training.walk", "999999,20:1,4/20", timeout=1800,
                  stop_on=r"WALK SCRIPT FAILED at command \d+ 'waitbtl|walk script done", grace=0)
    need(text, r"WALK SCRIPT FAILED at command \d+ 'waitbtl:menu': timeout; player under control",
         "no battle: the field kept control")
    need(text, r"CHALLENGE_ARMED run=R1 rules=mandatory_only custom=true", "CUSTOM mandatory-only run")
    refusals = re.findall(r"ENCOUNTER_REFUSED encounter=(\d+) via=(attack|contact)", text)
    assert refusals, "no ordinary Shadow was refused"
    assert "TEST_BATTLE_START" not in text, "an optional battle started"


def quota(s):
    legs = (("q1", "school_to_tv.walk", "11000,20:1,4/22"),
            ("q2", "save0422tv_to_orobas.walk", "16000,20:1,4/22"),
            ("q3", "save0422orobas_to_personas.walk", "33000,20:1,4/22"))
    for index, (label, walk, gate) in enumerate(legs, 1):
        text = s.walk(label, walk, gate)
        assert "SDKMEMCARD_SAVE OK" in text, f"{label}: save {index} did not publish"
        need(text, rf"CHALLENGE_SAVED run=R1 gen={index} saves={index}", f"save {index} debit")
    kept = (s.card / SLOT / SLOT).read_bytes()
    text = s.walk("q4", "save0422personas_to_omoikane.walk", "24000,20:1,4/22", timeout=1800,
                  stop_on=r"Cannot save game", grace=10)
    need(text, r"CHALLENGE_STATUS CHALLENGE 1 RULES CUSTOM / SAVES 0", "no saves left")
    need(text, r"Cannot save game: .*LIMITED SAVES: no saves left until the next story dungeon boss is defeated\.",
         "native save refusal")
    assert "CHALLENGE_SAVED run=R1 gen=4" not in text, "a fourth save published"
    assert (s.card / SLOT / SLOT).read_bytes() == kept, "the refused save changed the slot"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--scenario", action="append", choices=("camp", "training", "loss", "strict", "fusion", "solo", "severe", "mandatory", "quota"))
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("challenge proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("challenge proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", "")).resolve()
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seeds = Path(os.environ.get("P4PC_CHALLENGE_SEEDS", str(elf.parent / "saves")))
    needed = ("story0420_5f", "story0422personas", "story0422")
    if not elf.is_file() or not iso.is_file() or any(not (seeds / s / SLOT).is_dir() for s in needed):
        print("mod_challenge: retail ELF/ISO or private story seeds unavailable", file=sys.stderr)
        return 77
    selected = args.scenario or ["camp", "training", "loss", "strict", "fusion", "solo", "severe", "mandatory", "quota"]
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-challenge-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        write_fixtures(work / "mods")
        common = dict(os.environ, P4PC_MOD_DIR=str(work / "mods"), P4PC_MODS="1", P4PC_PACKS="0",
                      P4PC_INPUT="script", P4PC_MOD_WITNESS="1", P4PC_ISO=str(iso.resolve()),
                      P4PC_HIDDEN_WINDOW="1", P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy",
                      P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2", P4PC_VIRTUAL_CLOCK="1",
                      P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            common.pop("SDL_VIDEODRIVER", None)
        else:
            common["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED",
                    "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT", "P4PC_WALK_SCRIPT",
                    "P4PC_NEWGAME_GATE"):
            common.pop(key, None)

        def scenario(name, rules, fixtures, seed=None):
            s = Scenario(work, name, rules, fixtures, common, args.driver, elf)
            if seed:
                s.seed(seeds / seed)
            return s

        if "camp" in selected:
            camp(scenario("camp", ALL_RULES, ("test.challenge.observe",)))
        if "training" in selected or "loss" in selected:
            rules = ("ironman", "ironman_confirm", "boss_cap", "no_battle_items", "no_field_recovery",
                     "limited_saves", "custom_on_load")
            s = scenario("ironman", rules, ("test.challenge.observe", "test.challenge.boost"), "story0420_5f")
            training(s)
            if "loss" in selected:
                # The training save carries the boost fixture's provenance; a load
                # needs every mod the save was played with, so it stays enabled.
                s.config.write_text("[mods]\nenabled=%s,test.challenge.observe,test.challenge.boost,"
                                    "test.challenge.frail\n" % PACKAGE,
                                    encoding="utf-8")
                loss(s)
                locked(s)
        if "fusion" in selected:
            fusion(scenario("fusion", ("no_fusion", "custom_on_load"), ("test.challenge.observe",), "story0422personas"))
        if "severe" in selected:
            severe(scenario("severe", ("severe_damage", "custom_on_load"), ("test.challenge.observe",), "story0420_5f"))
        if "strict" in selected:
            s = scenario("strict", STRICT_RULES, ("test.challenge.observe",), "story0420_5f")
            strict_training(s)
            strict_kill(s)
            strict_crash(s)
        if "solo" in selected:
            solo(scenario("solo", ("solo=YU", "custom_on_load"), ("test.challenge.observe",), "story0420_5f"))
        if "mandatory" in selected:
            mandatory(scenario("mandatory", ("mandatory_only", "custom_on_load"), ("test.challenge.observe",),
                               "story0420_5f"))
        if "quota" in selected:
            quota(scenario("quota", ("limited_saves", "custom_on_load"), ("test.challenge.observe",), "story0422"))
        outcome = "pass"
        print(f"mod_challenge_game: PASS scenarios={','.join(selected)} mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
