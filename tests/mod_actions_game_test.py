#!/usr/bin/env python3
"""Real-game witness of p4.inventory, p4.give, p4.take and p4.party on the user's own disc; never a retail reference.

New Game is driven to the prologue's first event; a private package then gives and takes
Medicine and yen through the actions and reads the counts back through the game's own
accessors, checks the 99 stack cap and the catalog's suggestion for a misspelled name, raises
the protagonist's own level with p4.give.exp and is refused for a member who has not joined.
With the private story0415 card (P4PC_ACTIONS_SEED, default <ELF dir>/saves/story0415) a second
run loads it and teaches the protagonist's Persona a skill, refuses it a second time, and gives
EXP to that Persona and to Yosuke's, reading levels and skills back through p4.party. Nothing is
queued (no battle). A third run loads the card again, fills the protagonist's Persona to 8
skills and asks twice with give.skill.choice: the walk answers the game's own panel, forgetting a
skill for the first and keeping every skill for the second, and the mod logs each outcome, the
skills after it and that the scene reported the field held. No retail byte is committed."""
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
import mod_witness  # noqa: E402

MAIN = r"""--!strict
assert(p4.has("inventory@1") and p4.has("give.item@1") and p4.has("give.money@1"), "this build lacks the actions")
local done = false
p4.events.on("frame_start", function(e: P4FrameEvent)
    if done then return end
    local context = p4.scene.state().context
    if context ~= "event" and context ~= "field" and context ~= "dungeon" then return end
    done = true
    local before = p4.inventory.count("Medicine")
    local yen = p4.inventory.money()
    local given: number, why: string? = p4.give.item("Medicine", 3)
    local after = p4.inventory.count("Medicine")
    local taken = p4.take.item(p4.data.item("Medicine"), 1)
    local final = p4.inventory.count(p4.data.item("Medicine").id)
    p4.log(string.format("ACT_ITEMS context=%s before=%d given=%d why=%s after=%d taken=%d final=%d",
        context, before, given, tostring(why), after, taken, final))
    local paid = p4.give.money(1234)
    local spent = p4.take.money(34)
    p4.log(string.format("ACT_MONEY before=%d paid=%d spent=%d after=%d", yen, paid, spent, p4.inventory.money()))
    local capped = p4.give.item("Medicine", 99)
    p4.log(string.format("ACT_CAP final=%d given=%d count=%d", final, capped, p4.inventory.count("Medicine")))
    local _, typo = pcall(p4.give.item, "Medecine", 1)
    p4.log("ACT_TYPO " .. tostring(typo))
    local hero_before = p4.party.level("protagonist") or -1
    local hero_ok = p4.give.exp("protagonist", 5000)
    p4.log(string.format("ACT_HERO before=%d ok=%s after=%d", hero_before, tostring(hero_ok),
        p4.party.level("protagonist") or -1))
    local persona, why_persona = p4.party.persona("protagonist")
    local yosuke, why_yosuke = p4.party.persona("Yosuke")
    if persona then
        local skill = p4.data.skill("Agi").id
        local before_level = persona.level
        local learned = table.find(persona.skills, skill) ~= nil
        local ok1, why1 = p4.give.skill("protagonist", "Agi")
        local ok2, why2 = p4.give.skill("protagonist", skill)
        local after = p4.party.persona(1)
        local ok3 = p4.give.exp("protagonist", 3000, { persona = true })
        local leveled = p4.party.persona(1)
        p4.log(string.format("ACT_PERSONA id=%d level=%d knew=%s give=%s/%s again=%s/%s has=%s exp=%s level_after=%d",
            persona.id, before_level, tostring(learned), tostring(ok1), tostring(why1), tostring(ok2), tostring(why2),
            tostring(after ~= nil and table.find(after.skills, skill) ~= nil), tostring(ok3),
            leveled and leveled.level or -1))
    else
        p4.log("ACT_PERSONA none why=" .. tostring(why_persona))
    end
    if yosuke then
        local ok = p4.give.exp("Yosuke", 3000)
        p4.log(string.format("ACT_YOSUKE level=%d ok=%s after=%d", yosuke.level, tostring(ok), p4.party.level("Yosuke") or -1))
    else
        local ok, why = p4.give.skill("Yosuke", "Garu")
        p4.log(string.format("ACT_YOSUKE none why=%s give=%s/%s", tostring(why_yosuke), tostring(ok), tostring(why)))
    end
    p4.log("ACT_DONE")
end)
"""

# The third leg: fill the protagonist's Persona to 8 skills, then two choices (CHOICE_WALK answers them).
CHOICE_MAIN = r"""--!strict
assert(p4.has("give.skill.choice@1"), "this build lacks the skill choice")
local fillers = { "Agi", "Garu", "Bufu", "Zio", "Dia", "Tarukaja", "Rakukaja", "Sukukaja", "Tarunda", "Rakunda", "Sukunda", "Patra" }
local asked, held, outcomes = false, false, 0
local function list(): string
    local persona = p4.party.persona("protagonist")
    local parts = {}
    for _, id in (persona and persona.skills or {}) do table.insert(parts, tostring(id)) end
    return table.concat(parts, ",")
end
local function done(outcome: P4SkillChoice)
    outcomes += 1
    p4.log(string.format("CHOICE_DONE n=%d skill=%d learned=%s forgot=%s reason=%s skills=%s", outcomes, outcome.skill,
        tostring(outcome.learned), tostring(outcome.forgot), tostring(outcome.reason), list()))
    if outcomes == 2 then p4.log("ACT_DONE") end
end
p4.events.on("frame_start", function(e: P4FrameEvent)
    local state = p4.scene.state()
    if asked then
        if not held and state.reason == "the game's skill choice holds the field" then
            held = true
            p4.log("CHOICE_HELD")
        end
        return
    end
    if state.context ~= "field" or not state.ready then return end
    asked = true
    for _, name in fillers do
        local persona = p4.party.persona("protagonist")
        if not persona or #persona.skills >= 8 then break end
        p4.give.skill("protagonist", name) -- refused when already known
    end
    local count = #((p4.party.persona("protagonist") or { skills = {} }).skills)
    local before = list()
    local ok1, why1 = p4.give.skill("protagonist", "Maragi", { choice = true, done = done })
    local ok2, why2 = p4.give.skill("protagonist", "Mazio", { choice = true, done = done })
    p4.log(string.format("CHOICE_ASK count=%d before=%s first=%s/%s second=%s/%s ids=%d,%d", count, before,
        tostring(ok1), tostring(why1), tostring(ok2), tostring(why2), p4.data.skill("Maragi").id, p4.data.skill("Mazio").id))
end)
"""

# Answers both panels through the game's own UI: the intro line, the cursor (Down: skill slot 1;
# Left: the offered skill, row 8), the confirmation text, then Up from the Yes/No's default No
# to Yes, and the result line. The nm current window handle stays open for the whole replace task.
CHOICE_WALK = """# LOAD GAME as the Persona leg, then until the player is free on the 04/15 evening.
timeout:6000
wait:140
press:cross
wait:140
press:cross
wait:140
press:cross
skipmsg
waitcontrol:on
# The first choice: forget skill slot 1.
waitstate:0x00882F60!=0xFFFFFFFF
wait:90
press:cross
wait:60
press:down
wait:20
press:cross
wait:60
press:cross
wait:60
press:up
wait:20
press:cross
wait:120
press:cross
waitstate:0x00882F60==0xFFFFFFFF
# The second choice: keep every skill.
waitstate:0x00882F60!=0xFFFFFFFF
wait:90
press:cross
wait:60
press:left
wait:20
press:cross
wait:60
press:cross
wait:60
press:up
wait:20
press:cross
wait:120
press:cross
waitstate:0x00882F60==0xFFFFFFFF
wait:60
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("actions witness requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    if not elf.is_file() or not iso.is_file():
        print("mod_actions_game: retail ELF/ISO unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-actions-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        package = work / "mods" / "p4pc.actions-witness"
        package.mkdir(parents=True)
        (package / "mod.ini").write_text("[mod]\nid = p4pc.actions-witness\nname = Actions witness\nversion = 1.0.0\n"
                                         "api = 1.6\nrequires = inventory@1, give.item@1, give.money@1, data.catalog@1, "
                                         "party.persona@1, give.skill@1, give.exp@1, give.skill.choice@1\n"
                                         "priority = 0\norder = 0\n", encoding="utf-8")
        (package / "main.luau").write_text(MAIN, encoding="utf-8")
        config = work / "p4pc.ini"
        config.write_text("[mods]\nenabled=p4pc.actions-witness\n", encoding="utf-8")
        common = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(package.parent), P4PC_MODS="1",
                      P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1", P4PC_ISO=str(iso.resolve()),
                      P4PC_SCENE_ARTIFACTS=str(work), P4PC_HIDDEN_WINDOW="1", P4PC_OFFSCREEN="1",
                      SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2", P4PC_VIRTUAL_CLOCK="1",
                      P4PC_AUTOPRESS_FRAMES="1")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            common.pop("SDL_VIDEODRIVER", None)
        else:
            common["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_WALK_SCRIPT",
                    "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT", "P4PC_AUTOPRESS_LOAD"):
            common.pop(key, None)

        def run(label, **extra):
            log = work / f"{label}.log"
            with log.open("wb") as output:
                result = subprocess.run([*args.driver, str(elf.resolve())], cwd=work, env=dict(common, **extra),
                                        stdout=output, stderr=subprocess.STDOUT, timeout=900, check=False,
                                        creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
            text = log.read_text(encoding="utf-8", errors="replace")
            assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
            assert "ACT_DONE" in text, f"the actions never ran (exit {result.returncode}); see {log}"
            return text

        text = run("newgame", P4PC_SAVE_DIR=str(work / "card"), P4PC_AUTOPRESS_NEWGAME="1560",
                   P4PC_AUTOPRESS="cross@2300:2/150*20", P4PC_NEWGAME_GATE="4200,-1:-1")
        items = re.search(r"ACT_ITEMS context=(\w+) before=(\d+) given=(\d+) why=(\S+) after=(\d+) taken=(\d+) final=(\d+)", text)
        before, given, after, taken, final = (int(items.group(i)) for i in (2, 3, 5, 6, 7))
        assert items.group(4) == "nil" and given == 3 and after == before + 3, items.group(0)
        assert taken == 1 and final == after - 1, items.group(0)
        money = re.search(r"ACT_MONEY before=(\d+) paid=(\d+) spent=(\d+) after=(\d+)", text)
        yen, paid, spent, now = (int(money.group(i)) for i in range(1, 5))
        assert paid == 1234 and spent == 34 and now == yen + 1200, money.group(0)
        cap = re.search(r"ACT_CAP final=(\d+) given=(\d+) count=(\d+)", text)
        assert int(cap.group(2)) == 99 - int(cap.group(1)) and int(cap.group(3)) == 99, cap.group(0)
        typo = re.search(r"ACT_TYPO (.*)", text).group(1)
        assert 'no item named "Medecine". Did you mean "Medicine"' in typo, typo
        hero = re.search(r"ACT_HERO before=(-?\d+) ok=(\w+) after=(-?\d+)", text)
        assert hero.group(2) == "true" and int(hero.group(3)) > int(hero.group(1)) > 0, hero.group(0)
        assert "ACT_YOSUKE none why=that member has not joined the party yet give=false/that member has not joined" in text

        seed = Path(os.environ.get("P4PC_ACTIONS_SEED", str(elf.parent / "saves/story0415")))
        if not (seed / "BASLUS-21782Save0").is_dir():
            print(f"mod_actions_game: the Persona leg needs the private story0415 card ({seed}); skipped", file=sys.stderr)
        else:
            card = work / "card-0415" / "mc0"
            shutil.copytree(seed, card)
            walk = work / "load0415.walk"
            walk.write_text("# The title's LOAD GAME (P4PC_AUTOPRESS_LOAD=0): slot, Yes, Load successful, then until the\n"
                            "# player is free on the 04/15 evening.\ntimeout:6000\nwait:140\npress:cross\nwait:140\n"
                            "press:cross\nwait:140\npress:cross\nskipmsg\nwaitcontrol:on\nwait:60\n", encoding="ascii")
            text = run("story0415", P4PC_SAVE_DIR=str(card.parent), P4PC_AUTOPRESS_LOAD="0",
                       P4PC_WALK_SCRIPT=str(walk), P4PC_NEWGAME_GATE="8000,-1:-1")
            persona = re.search(r"ACT_PERSONA id=(\d+) level=(\d+) knew=(\w+) give=(\w+)/(.*?) again=(\w+)/(.*?) "
                                r"has=(\w+) exp=(\w+) level_after=(-?\d+)", text)
            assert persona, "the protagonist has no Persona in the story0415 save"
            knew = persona.group(3) == "true"
            assert persona.group(4) == ("false" if knew else "true"), persona.group(0)
            assert persona.group(6) == "false" and persona.group(7) == "the Persona already knows that skill", persona.group(0)
            assert persona.group(8) == "true" and persona.group(9) == "true", persona.group(0)
            assert int(persona.group(10)) > int(persona.group(2)), persona.group(0)
            yosuke = re.search(r"ACT_YOSUKE level=(\d+) ok=(\w+) after=(\d+)", text)
            if yosuke:
                assert yosuke.group(2) == "true" and int(yosuke.group(3)) > int(yosuke.group(1)), yosuke.group(0)

            # The skill choice: the same card, its own copy, a mod that asks twice and a walk that answers.
            (package / "main.luau").write_text(CHOICE_MAIN, encoding="utf-8")
            choice_card = work / "card-choice" / "mc0"
            shutil.copytree(seed, choice_card)
            choice_walk = work / "choice0415.walk"
            choice_walk.write_text(CHOICE_WALK, encoding="ascii")
            text = run("choice0415", P4PC_SAVE_DIR=str(choice_card.parent), P4PC_AUTOPRESS_LOAD="0",
                       P4PC_WALK_SCRIPT=str(choice_walk), P4PC_NEWGAME_GATE="11000,-1:-1")
            ask = re.search(r"CHOICE_ASK count=(\d+) before=([\d,]+) first=(\w+)/(.*?) second=(\w+)/(.*?) ids=(\d+),(\d+)", text)
            assert ask and int(ask.group(1)) == 8, ask and ask.group(0)
            before = [int(s) for s in ask.group(2).split(",")]
            maragi, mazio = int(ask.group(7)), int(ask.group(8))
            assert ask.group(3) == "true" and ask.group(4).startswith("queued"), ask.group(0)
            assert ask.group(5) == "true" and ask.group(6) == "queued behind another skill choice", ask.group(0)
            assert "CHOICE_HELD" in text, "the scene never reported the field held by the choice"
            first = re.search(r"CHOICE_DONE n=1 skill=(\d+) learned=(\w+) forgot=(\S+) reason=(\S+) skills=([\d,]+)", text)
            assert first and int(first.group(1)) == maragi and first.group(2) == "true", first and first.group(0)
            forgot, after = int(first.group(3)), [int(s) for s in first.group(5).split(",")]
            assert forgot in before and after[before.index(forgot)] == maragi, first.group(0)
            assert sorted(after) == sorted([s for s in before if s != forgot] + [maragi]), first.group(0)
            second = re.search(r"CHOICE_DONE n=2 skill=(\d+) learned=(\w+) forgot=(\S+) reason=(\S+) skills=([\d,]+)", text)
            assert second and int(second.group(1)) == mazio and second.group(2) == "false", second and second.group(0)
            assert second.group(4) == "declined" and [int(s) for s in second.group(5).split(",")] == after, second.group(0)
        outcome = "pass"
        print(f"mod_actions_game: PASS mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
