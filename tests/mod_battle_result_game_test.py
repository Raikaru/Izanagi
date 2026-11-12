#!/usr/bin/env python3
"""Real-game witness of the battle events one_more, reward, level_up and skill_learned on the
user's own disc; never a retail reference.

The private story0415 card (P4PC_EVENTS_SEED, default <ELF dir>/saves/story0415) is loaded and
tests/walks/save0415_to_castle_2f.walk plays 04/16-04/17 through Castle 1F's Shadow fights
to the 2F landing, Cross-only. How many Shadows cross the planned route is encounter RNG under the
virtual clock (the 2026-11-10 witness run met one), so the witness requires at least one won fight
and proves the reward events on whatever fights the walk wins. A private package logs every damage, knockdown,
skill_used, one_more, reward, level_up and skill_learned event. A reward modifier makes
the team's money 137 yen, and every reward event must report that modified amount as
well as EXP and a party member's share. Every one_more follows its own action's hits and
names a cause those hits show; every level_up raises a level (from < level); every learned skill
belongs to a Persona that levelled up just before. No retail byte is committed."""
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
local function who(u: P4BattleUnitRef): string
    return u.side .. ":" .. (u.character or u.key or tostring(u.id)) .. "#" .. tostring(u.uid)
end
p4.reward.on({kind = "multiply", fn = function(q: P4RewardQuery): number?
    if q.reward == "money" then return 0 end
    return nil
end})
p4.reward.on({kind = "add", fn = function(q: P4RewardQuery): number?
    if q.reward == "money" then return 137 end
    return nil
end})
p4.events.on("skill_used", function(e: P4SkillUsedEvent) p4.log("EVR skill_used " .. who(e.unit)) end)
p4.events.on("damage", function(e: P4DamageEvent)
    p4.log(string.format("EVR damage %s>%s %s", who(e.source), who(e.target), e.result))
end)
p4.events.on("knockdown", function(e: P4KnockdownEvent) p4.log("EVR knockdown " .. who(e.source) .. ">" .. who(e.unit)) end)
p4.events.on("one_more", function(e: P4OneMoreEvent) p4.log("EVR one_more " .. who(e.unit) .. " " .. e.cause) end)
p4.events.on("reward", function(e: P4RewardEvent)
    local party = {}
    for _, m in e.party do table.insert(party, (m.character or "?") .. "=" .. tostring(m.exp)) end
    p4.log(string.format("EVR reward exp=%d money=%d items=%d party=%s", e.exp, e.money, #e.items, table.concat(party, ",")))
end)
p4.events.on("level_up", function(e: P4LevelUpEvent)
    p4.log(string.format("EVR level_up %s persona=%s from=%d level=%d", e.character or "?",
        if e.persona then tostring(e.persona.id) else "none", e.from, e.level))
end)
p4.events.on("skill_learned", function(e: P4SkillLearnedEvent)
    p4.log(string.format("EVR skill_learned %s persona=%d skill=%d", e.character or "?", e.persona.id, e.skill.id))
end)
p4.log("EVR ready")
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("battle result witness requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seed = Path(os.environ.get("P4PC_EVENTS_SEED", str(elf.parent / "saves/story0415")))
    if not elf.is_file() or not iso.is_file() or not (seed / "BASLUS-21782Save0").is_dir():
        print("mod_battle_result_game: retail ELF/ISO or private story0415 card unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-battle-result-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        package = work / "mods" / "p4pc.result-witness"
        package.mkdir(parents=True)
        (package / "mod.ini").write_text(
            "[mod]\nid = p4pc.result-witness\nname = Battle result witness\nversion = 1.0.0\napi = 1.6\n"
            "requires = reward@1, events.skill_used@1, events.damage@1, events.knockdown@1, events.one_more@1, events.reward@1, "
            "events.level_up@1, events.skill_learned@1\npriority = 0\norder = 0\n", encoding="utf-8")
        (package / "main.luau").write_text(MAIN, encoding="utf-8")
        card = work / "card" / "mc0"
        shutil.copytree(seed, card)
        config = work / "p4pc.ini"
        config.write_text("[mods]\nenabled=p4pc.result-witness\n", encoding="utf-8")
        env = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(package.parent), P4PC_MODS="1",
                   P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1", P4PC_ISO=str(iso.resolve()),
                   P4PC_SAVE_DIR=str(card.parent), P4PC_SCENE_ARTIFACTS=str(work), P4PC_HIDDEN_WINDOW="1",
                   P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2",
                   P4PC_VIRTUAL_CLOCK="1", P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0",
                   P4PC_WALK_SCRIPT=str(ROOT / "tests/walks/save0415_to_castle_2f.walk"),
                   P4PC_NEWGAME_GATE="90000,23:3,4/17")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            env.pop("SDL_VIDEODRIVER", None)
        else:
            env["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT",
                    "P4PC_MOD_WITNESS_OUT", "P4PC_AUTOPRESS_NEWGAME", "P4PC_AUTOPRESS"):
            env.pop(key, None)
        log = work / "castle.log"
        with log.open("wb") as output:
            result = subprocess.run([*args.driver, str(elf.resolve())], cwd=work, env=env, stdout=output,
                                    stderr=subprocess.STDOUT, timeout=2400, check=False,
                                    creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
        text = log.read_text(encoding="utf-8", errors="replace")
        assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
        assert "EVR ready" in text and "walk script done" in text, f"the walk did not finish (exit {result.returncode}); see {log}"
        assert result.returncode == 0, f"exit {result.returncode}; see {log}"
        events = [line.split("EVR ", 1)[1].strip() for line in text.splitlines() if "EVR " in line and "EVR ready" not in line]
        rewards = [e for e in events if e.startswith("reward ")]
        assert len(rewards) >= 1, f"one won fight, {len(rewards)} rewards"
        for reward in rewards:
            m = re.match(r"reward exp=(\d+) money=(\d+) items=(\d+) party=(\S*)", reward)
            assert m and int(m.group(1)) > 0 and int(m.group(2)) == 137 and re.search(r"=\d+", m.group(4)), reward
        # Each one_more follows the hits of its own action, which show its cause.
        actor, hits = None, []
        one_mores = 0
        for event in events:
            if event.startswith("skill_used "):
                actor, hits = event.split()[1], []
            elif event.startswith("damage ") or event.startswith("knockdown "):
                source = event.split()[1].split(">")[0]
                if source == actor:
                    hits.append(event)
            elif event.startswith("one_more "):
                _, unit, cause = event.split()
                assert unit == actor, f"{event} after {actor}'s action"
                want = {"weakness": " weak", "critical": " critical", "down": "knockdown "}[cause]
                assert any(want in h for h in hits), f"{event}: no {cause} among {hits}"
                one_mores += 1
        previous = None
        level_ups = 0
        for event in events:
            if event.startswith("level_up "):
                m = re.match(r"level_up (\S+) persona=(\S+) from=(\d+) level=(\d+)", event)
                assert m and int(m.group(3)) < int(m.group(4)), event
                level_ups += 1
                previous = (m.group(1), m.group(2))
            elif event.startswith("skill_learned "):
                m = re.match(r"skill_learned (\S+) persona=(\d+) skill=(\d+)", event)
                assert m and previous == (m.group(1), m.group(2)), f"{event} not after its Persona's level_up"
            else:
                previous = None
        print(f"mod_battle_result_game: {len(rewards)} rewards, {level_ups} level-ups, {one_mores} One Mores")
        outcome = "pass"
        print(f"mod_battle_result_game: PASS mod_witness=true reference_eligible=false evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
