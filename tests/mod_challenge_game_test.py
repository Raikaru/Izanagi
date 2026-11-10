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
    local record = p4.mem.read("u32", event.battle + 0xc68)
    local encounter = if record ~= 0 then p4.mem.read("u16", record + 8) else 0
    p4.log(string.format("TEST_LEVELS outcome=%d yu=%d yosuke=%d chie=%d encounter=%d", event.outcome, yu, yosuke,
        chie, encounter))
end)
-- Each field entry: Yosuke's Persona level (his join on 04/15 must keep it).
p4.events.on("field_enter", function()
    local record = p4.call(0x0010a900, "pH", 2) :: number
    local skills = {}
    for slot = 0, 7 do
        local skill = p4.mem.read("u16", record + 0xc + slot * 2)
        if skill ~= 0 then table.insert(skills, tostring(skill)) end
    end
    p4.log("TEST_FIELD yosuke=" .. tostring(p4.call(0x00109400, "BH", 2)) .. " skills=" .. table.concat(skills, ","))
end)
-- Game Over: the field script's procedure 15 (field or script battle).
for _, address in {0x0029da90, 0x0029db50} do
    p4.hook(address, if address == 0x0029da90 then "iipi" else "iiiii", "before", function(context)
        if context.args[1] == 15 and context.args[2] == p4.mem.read("u32", 0x007690f0 + 0xb284 - 0x10000) then
            p4.log("TEST_GAME_OVER")
        end
    end)
end
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
    # Exercises p4.party (API 1.8) on Yosuke's record: refused writes change
    # nothing, an accepted one lands at the next frame safe point, and a
    # battle refuses it.
    "test.challenge.party": """--!strict
local done = false
local expected: P4PartyRecord? = nil
local function show(tag: string, r: P4PartyRecord?)
    if not r then p4.log(tag .. " none") return end
    local hp = p4.call(0x00104ce0, "Ih", 2) :: number
    local max = p4.call(0x00104dc0, "Hh", 2) :: number
    p4.log(string.format("%s persona=%d level=%d exp=%d stats=%s skills=%s hp=%d max=%d", tag, r.persona, r.level,
        r.exp, table.concat(r.stats :: {any}, ","), table.concat(r.skills :: {any}, ","), hp, max))
end
p4.events.on("field_enter", function()
    if done then return end
    done = true
    local r = p4.party.record(2)
    show("TEST_PARTY_READ", r)
    if not r then return end
    local ok, why = p4.party.set(2, {persona = r.persona, level = 1, exp = 999999, stats = r.stats, skills = r.skills})
    p4.log("TEST_PARTY_BAD_EXP ok=" .. tostring(ok) .. " why=" .. tostring(why))
    ok, why = p4.party.set(2, {persona = r.persona + 1, level = r.level, exp = r.exp, stats = r.stats, skills = r.skills})
    p4.log("TEST_PARTY_BAD_PERSONA ok=" .. tostring(ok) .. " why=" .. tostring(why))
    ok, why = p4.party.set(2, {persona = r.persona, level = r.level, exp = r.exp, stats = r.stats,
        skills = {r.skills[1], r.skills[1]}})
    p4.log("TEST_PARTY_BAD_SKILLS ok=" .. tostring(ok) .. " why=" .. tostring(why))
    show("TEST_PARTY_UNCHANGED", p4.party.record(2))
    expected = {persona = r.persona, level = 1, exp = 0, stats = {2, 2, 2, 2, 2}, skills = {r.skills[1]}}
    ok, why = p4.party.set(2, expected :: P4PartyRecord)
    p4.log("TEST_PARTY_SET ok=" .. tostring(ok) .. " why=" .. tostring(why))
    show("TEST_PARTY_SAME_FRAME", p4.party.record(2))
end)
p4.events.on("frame_start", function()
    if expected then
        expected = nil
        show("TEST_PARTY_APPLIED", p4.party.record(2))
    end
end)
p4.events.on("battle_start", function()
    local r = p4.party.record(2)
    if not r then return end
    local ok, why = p4.party.set(2, {persona = r.persona, level = r.level, exp = r.exp, stats = r.stats, skills = r.skills})
    p4.log("TEST_PARTY_IN_BATTLE ok=" .. tostring(ok) .. " why=" .. tostring(why))
end)
p4.log("TEST_PARTY_READY")
""",
    # Opens the Velvet Room's roster pair fusion (flag 0x1305, normally a late
    # unlock): at the root's first row dispatch (mode 0, state 0x1C, where the
    # selected row's own mode/state are copied in) it takes the roster row's
    # transition instead (mode 8, state 0x75), then reports whether the mode ran.
    "test.challenge.roster": """--!strict
local chosen, entered = false, false
local last, reported = -1, 0
p4.events.on("field_enter", function()
    p4.call(0x00106390, "vii", 0x1305, 1)
end)
p4.hook(0x002e7ac0, "ip", "before", function(context)
    local work = p4.mem.read("u32", (context.args[1] :: number) + 0x38)
    local now = p4.mem.read("u8", work) * 256 + p4.mem.read("u8", work + 1)
    if now ~= last and reported < 40 then
        last, reported = now, reported + 1
        p4.log(string.format("TEST_ROSTER_STATE mode=%d state=0x%x", now // 256, now % 256))
    end
    if not chosen and now == 0x1c then
        chosen = true
        p4.mem.write("u8", work, 8)
        p4.mem.write("u8", work + 1, 0x75)
        p4.log("TEST_ROSTER_ROW_CHOSEN")
    end
end)
p4.hook(0x00304580, "vp", "before", function()
    if not entered then
        entered = true
        p4.log("TEST_ROSTER_FUSION_ENTERED")
    end
end)
p4.log("TEST_ROSTER_READY")
""",
    # Knocks Yu out when a battle starts.
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
        env = {key: value for key, value in env.items() if value is not None}  # None removes a common key
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
    need(text, r"CAP_RESULT flags=0x[0-9a-f]+ shown=0x[0-9a-f]+", "level-up screens the cap took away are not shown")
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


def party(s):
    text = s.walk("party", "save0420_5f_to_training.walk", "999999,20:1,4/20", timeout=2400,
                  stop_on=r"TEST_PARTY_IN_BATTLE", grace=2)
    read = need(text, r"TEST_PARTY_READ persona=(\d+) level=(\d+) exp=(\d+) stats=(\S+) skills=(\S+) hp=\d+", "Yosuke's record")
    for label in ("BAD_EXP", "BAD_PERSONA", "BAD_SKILLS"):
        need(text, r"TEST_PARTY_%s ok=false why=\S" % label, "refused " + label)
    unchanged = need(text, r"TEST_PARTY_UNCHANGED persona=(\d+) level=(\d+) exp=(\d+) stats=(\S+) skills=(\S+) hp=\d+",
                     "record after refusals")
    assert unchanged.groups() == read.groups(), "a refused write changed the record"
    need(text, r"TEST_PARTY_SET ok=true why=nil", "accepted write")
    same = need(text, r"TEST_PARTY_SAME_FRAME persona=\d+ level=(\d+)", "same-frame read")
    assert same[1] == read[2], "the write applied before the frame safe point"
    need(text, r"p4pc: mods: party record 2 set by owner \d+: level 1", "native safe-point apply")
    applied = need(text, r"TEST_PARTY_APPLIED persona=\d+ level=1 exp=0 stats=2,2,2,2,2 skills=\d+ hp=(\d+) max=(\d+)",
                   "the applied record")
    assert int(applied[1]) <= int(applied[2]), "HP above the new maximum"
    need(text, r"TEST_PARTY_IN_BATTLE ok=false why=Party records cannot change during a battle", "battle refusal")


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


def roster(s, gated):
    shots = (2200,)
    text = s.run("roster", {"P4PC_WALK_SCRIPT": str(WALKS / "challenge_velvet_roster.walk"),
                            "P4PC_NEWGAME_GATE": "999999,20:1,4/22",
                            "P4PC_CAPTURE_FRAMES": ",".join(str(f) for f in shots),
                            "P4PC_CAPTURE_PNG": str(s.dir / "roster_%d.png")}, 1800,
                 stop_on=r"TEST_ROSTER_FUSION_ENTERED|fusion refused by owner", grace=5)
    need(text, r"TEST_ROSTER_ROW_CHOSEN", "the roster pair fusion row taken from the idle root menu")
    if gated:
        need(text, r"CHALLENGE_ARMED run=R1 rules=no_fusion custom=true", "CUSTOM no-fusion run")
        need(text, r"p4pc: mods: fusion refused by owner \d+: NO FUSION challenge rule", "roster fusion refused")
        assert "TEST_ROSTER_FUSION_ENTERED" not in text, "the denied roster fusion mode was entered"
    else:
        # Control: without the rule the same input reaches roster pair fusion.
        need(text, r"TEST_ROSTER_FUSION_ENTERED", "the row is roster pair fusion")
    assert all((s.dir / ("roster_%d.png" % f)).is_file() for f in shots), "Velvet Room captures missing"


# New Game through 04/15 (the story gate's route and battle driver): the first
# night's dream battle, the Persona awakening fights and Shadow Yosuke, to the
# Dojima living room on 04/15.
NEWGAME = {"P4PC_AUTOPRESS_LOAD": None, "P4PC_AUTOPRESS_NEWGAME": "1560",
           "P4PC_AUTOPRESS": "cross@2300:2/150*20,start@6500:2/300*14",
           # Circle puts the command wheel on Attack (it otherwise keeps each
           # character's last command, across fights too), then Skill, the
           # first skill, its target; when the skill cannot be paid, Circle
           # twice backs out to Attack and attacks.
           "P4PC_BATTLE_FIRST": "circle,down,cross,cross,cross,circle,circle,cross,cross",
           "P4PC_BATTLE_DRIVER": "circle,down,cross,cross,cross,circle,circle,cross,cross",
           "P4PC_BATTLE_SKIP": "140", "P4PC_WALK_SCRIPT": str(WALKS / "newgame_to_0413.walk"),
           "P4PC_NEWGAME_GATE": "220000,7:2,4/15", "P4PC_FACING_GATE": "1", "P4PC_BATTLE_LOG": "1"}


def solo_companion(s, member, name, plan=None, must_win=True):
    """The chosen companion fights every battle of a New Game alone, from level 1."""
    shots = (100000, 125000, 140000, 160000, 190000)
    env = dict(NEWGAME, P4PC_CAPTURE_FRAMES=",".join(str(f) for f in shots),
               P4PC_CAPTURE_PNG=str(s.dir / "newgame_%d.png"))
    if plan:
        env.update(P4PC_BATTLE_FIRST=plan, P4PC_BATTLE_DRIVER=plan)
    text = s.run("newgame", env, 4800,
                 stop_on=r"new-game gate (PASS|FAIL)|TEST_GAME_OVER", grace=3)
    need(text, r"CHALLENGE_ARMED run=R1 rules=solo custom=false epoch=1 solo=%d" % member, "New Game solo arming")
    curve = need(text, r"SOLO_CURVE member=%s persona=\d+ join_level=(\d+) join_stats=(\S+) base=\S+ skill=\d+ "
                       r"join_equipment=(\d+),(\d+),\d+ equipment=(\d+),(\d+),\d+" % name, "level-1 early curve")
    need(text, r"p4pc: mods: party record %d set by owner \d+: level 1" % member, "record set to level 1")
    projected = re.findall(r"p4pc: mods: battle lineup projected by owner \d+: (\d) -> 1 members", text)
    battles = re.findall(r"TEST_LEVELS outcome=(\d+) yu=\d+ yosuke=\d+ chie=\d+ encounter=(\d+)", text)
    assert projected and len(projected) == len(battles), f"every battle fought alone: {projected} {battles}"
    leaders = re.findall(r"p4pc: mods: battle leader role given to unit (\d+)", text)
    assert leaders and set(leaders) == {str(member)} and len(leaders) == len(battles), \
        f"the fighter takes Yu's leader role in every battle: {leaders}"
    # The dream battle (encounter 532) ends as a scripted loss in retail too.
    assert battles[0][1] == "532", battles
    assert not must_win or any(outcome == "1" for outcome, _ in battles), f"no battle won: {battles}"
    join_level, join_stats = curve.group(1), curve.group(2)
    if member == 2 and "new-game gate PASS" in text:
        # Yosuke joins the story party on 04/15: the field after it still
        # shows the level he earned, not the template's join level.
        earned = re.findall(r"TEST_LEVELS outcome=\d+ yu=\d+ yosuke=(\d+)", text)[-1]
        skills = re.findall(r"SOLO_CURVE_STEP member=YOSUKE level=\d+ stats=\S+ skills=(\S+)", text)[-1]
        assert re.findall(r"TEST_FIELD yosuke=(\d+) skills=(\S*)", text)[-1] == (earned, skills), \
            "the join reset Yosuke's record"
    # On the curve the join level brings exactly the story's join stats.
    for level, stats in re.findall(r"SOLO_CURVE_STEP member=%s level=(\d+) stats=(\S+)" % name, text):
        if level == join_level:
            assert stats == join_stats, f"level {level} stats {stats}, join stats {join_stats}"
    print(f"mod_challenge_game: {name} battles (outcome, encounter): {battles}; join kit "
          f"{curve.group(3)},{curve.group(4)} -> starter {curve.group(5)},{curve.group(6)}")
    if "SOLO_DEFEAT" in text:
        # The chosen fighter's real fall ends the run although Yu, benched,
        # stands: his record is knocked out and the field's Game Over runs.
        need(text, r"SOLO_DEFEAT member=%s" % name, "the fighter's fall")
        need(text, r"TEST_GAME_OVER", "the Game Over procedure after the fighter's fall")
    else:
        need(text, r"new-game gate PASS", "the story reached the Dojima living room on 04/15")
    return battles


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
    text = s.run("severe", {"P4PC_WALK_SCRIPT": str(WALKS / "save0420_5f_to_training.walk"),
                            "P4PC_NEWGAME_GATE": "38000,20:1,4/20", "P4PC_DAMAGE_TRACE": "1"}, 2400,
                 stop_on=r"walk script (done|failed)|CHALLENGE_DEFEAT|CHALLENGE_NEW_GAME armed=0", grace=10)
    if "CHALLENGE_RULE_UNAVAILABLE rule=severe_damage" in text:
        # A build without the damage stage never arms the rule it cannot enforce.
        need(text, r"CHALLENGE_NEW_GAME armed=0", "no run armed without its only rule")
        assert "SEVERE_DAMAGE" not in text and "CHALLENGE_ARMED" not in text
        print("mod_challenge_game: severe_damage unavailable in this build; refused to arm")
        return
    need(text, r"CHALLENGE_ARMED run=R1 rules=severe_damage custom=true", "CUSTOM severe-damage run")
    hits = re.findall(r"SEVERE_DAMAGE from_enemy=(true|false) kind=(\w+) stock=(\d+) factor=([\d.]+)", text)
    assert {enemy for enemy, *_ in hits} == {"true", "false"}, f"both directions must be observed: {hits}"
    for enemy, kind, stock, factor in hits:
        expected = 1.5 if enemy == "true" else 0.75
        if kind in ("critical", "weak"):
            expected *= 1.25
        assert abs(float(factor) - expected) < 1e-9, (enemy, kind, stock, factor)
    # The native stage composed each modifier exactly once and rounded the result.
    steps = re.findall(r"damage stage:   \S+ priority -?\d+ load \d+ multiply ([\d.e+-]+): ([\d.e+-]+) -> ([\d.e+-]+)", text)
    assert steps and all(abs(float(before) * float(f) - float(after)) < 1e-6 for f, before, after in steps), steps[:4]


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


def mandatory_boss(s):
    # Shadow Yukiko (encounter 0x202), a story boss: Mandatory Only lets it start.
    text = s.walk("boss", "save0426trained5_to_yukiko_rescue.walk", "999999,6:6,4/27", timeout=2400,
                  stop_on=r"TEST_LEVELS outcome=\d+ yu=\d+ yosuke=\d+ chie=\d+ encounter=514", grace=3)
    need(text, r"CHALLENGE_ARMED run=R1 rules=mandatory_only custom=true", "CUSTOM mandatory-only run")
    need(text, r"TEST_BATTLE_START", "the boss battle started")
    need(text, r"TEST_LEVELS outcome=\d+ yu=\d+ yosuke=\d+ chie=\d+ encounter=514", "the boss battle resolved")
    assert "ENCOUNTER_REFUSED" not in text, "a mandatory battle was refused"


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
    parser.add_argument("--scenario", action="append", choices=("camp", "training", "loss", "strict", "fusion", "roster", "solo", "yosuke", "naoto", "severe", "mandatory",
                                                            "boss", "quota", "party"))
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
    needed = ("story0420_5f", "story0422personas", "story0422", "story0426trained5")
    if not elf.is_file() or not iso.is_file() or any(not (seeds / s / SLOT).is_dir() for s in needed):
        print("mod_challenge: retail ELF/ISO or private story seeds unavailable", file=sys.stderr)
        return 77
    selected = args.scenario or ["camp", "training", "loss", "strict", "fusion", "roster", "solo", "yosuke", "naoto", "severe",
                                 "mandatory", "boss", "quota", "party"]
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
            else:
                s.card.mkdir(parents=True)  # New Game: an empty card
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
        if "party" in selected:
            party(scenario("party", ("custom_on_load",), ("test.challenge.party",), "story0420_5f"))
        if "strict" in selected:
            s = scenario("strict", STRICT_RULES, ("test.challenge.observe",), "story0420_5f")
            strict_training(s)
            strict_kill(s)
            strict_crash(s)
        if "roster" in selected:
            roster(scenario("roster-control", ("custom_on_load",), ("test.challenge.roster",), "story0422personas"), False)
            roster(scenario("roster", ("no_fusion", "custom_on_load"), ("test.challenge.roster",), "story0422personas"), True)
        if "yosuke" in selected:
            solo_companion(scenario("yosuke", ("solo=YOSUKE",), ("test.challenge.observe",)), 2, "YOSUKE")
        if "naoto" in selected:
            # Level 1 in Yu's starting-tier gun: the driver's shots do not win the
            # 04/15 fight, so her witness is the fall that ends the run.
            solo_companion(scenario("naoto", ("solo=NAOTO",), ("test.challenge.observe",)), 7, "NAOTO",
                           "circle,cross,cross", must_win=False)
        if "solo" in selected:
            solo(scenario("solo", ("solo=YU", "custom_on_load"), ("test.challenge.observe",), "story0420_5f"))
        if "mandatory" in selected:
            mandatory(scenario("mandatory", ("mandatory_only", "custom_on_load"), ("test.challenge.observe",),
                               "story0420_5f"))
        if "boss" in selected:
            mandatory_boss(scenario("boss", ("mandatory_only", "custom_on_load"), ("test.challenge.observe",), "story0426trained5"))
        if "quota" in selected:
            quota(scenario("quota", ("limited_saves", "custom_on_load"), ("test.challenge.observe",), "story0422"))
        outcome = "pass"
        print(f"mod_challenge_game: PASS scenarios={','.join(selected)} mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
