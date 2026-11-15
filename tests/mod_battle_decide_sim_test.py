#!/usr/bin/env python3
"""Boss witness of p4.battle.decide (capability battle.decide@1) on the native battle simulator,
with and without the bundled hard mode; never a retail reference or a fidelity run.

P4PC_DECIDE_BOSS_SCENARIO names a private battle_sim scenario of a boss encounter (for example the
Shadow Yukiko cohort: encounter 514 from the story0426trained5 card) whose runtime pins the user's
own ELF and disc; nothing private is committed. Each leg runs the same seeds:

  base   no mods: the bosses' own AI, which aims some single-target skills at a party member
         who is not the weakest;
  focus  a private package whose decider keeps every native command but aims an enemy's
         single-target skill at the living party member with the least HP, from d.candidates;
  hard   p4pc.hardmode at Hard (adopted by the loaded card) and its required Physical Split
         package beside the same decider: the run is armed, the encounter is classed as a boss
         and the decisions still apply.

Checks from each run's native trace: in focus and hard every enemy single-target skill that hits
the party first hits a living member with the least HP, at least one such command occurs, and the
stage refused nothing; in base at least one did not. The package also probes the snapshot-only
contract once inside its decider: p4.battle.announce and p4.log must raise there, which it reports
from a turn_start event."""
import argparse
import copy
import json
import os
from pathlib import Path
import re
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import battle_sim  # noqa: E402

PACKAGE = "p4pc.decide-boss-witness"
MAIN = r"""-- battle.decide@1 boss witness: an enemy's single-target skill goes to the weakest legal party member.
local probe: string? = nil
local reported = false
p4.battle.decide({ side = "enemy", fn = function(d)
    if probe == nil then
        local announced = pcall(p4.battle.announce, { unit = { uid = d.actor.uid }, text = "probe" })
        local logged = pcall(p4.log, "probe")
        probe = string.format("announce=%s log=%s", if announced then "accepted" else "refused",
            if logged then "accepted" else "refused")
    end
    local pick = d.pick
    if pick.kind ~= "skill" or pick.skill == nil or #pick.targets ~= 1 or pick.targets[1].side ~= "party" then
        return nil
    end
    local legal, all = d.candidates(pick.skill)
    if all then return nil end
    local weakest = nil
    for _, unit in legal do
        if unit.side == "party" and unit.alive and (weakest == nil or unit.hp < weakest.hp) then weakest = unit end
    end
    if weakest == nil or weakest.hp >= pick.targets[1].hp then return nil end
    return { skill = pick.skill, target = weakest }
end })
p4.events.on("turn_start", function(e)
    if probe ~= nil and not reported then
        reported = true
        p4.log("DECIDE_BOSS_PROBE " .. probe)
    end
end)
p4.log("DECIDE_BOSS_READY")
"""
INI = ("[mod]\nid = %s\nname = Decide boss witness\nversion = 1.0.0\napi = 1.6\npriority = 0\norder = 0\n"
       "requires = battle.decide@1, battle.announce@1, events.turn_start@1\n" % PACKAGE)
REFUSED = re.compile(r"p4\.battle\.decide: (?:skill \d+|pass) refused")


def check(out, focused, encounter, hard):
    """Counts from every run's trace and log; raises AssertionError on a contract breach."""
    weakest = other = runs = 0
    for attempt in sorted(out.glob("run-*/attempt-*")):
        trace, log = attempt / "trace.jsonl", attempt / "stderr.log"
        if not trace.is_file():
            continue
        runs += 1
        text = log.read_text(encoding="utf-8", errors="replace") if log.is_file() else ""
        party, pending = {}, None
        for line in trace.read_text(encoding="utf-8").splitlines():
            event = json.loads(line)
            kind = event["kind"]
            if "party" in event and kind in ("entry", "update", "terminal"):
                party = {p["id"]: p["hp"] for p in event["party"]}
            elif kind == "command_commit":
                actor = event["actor"]
                pending = event if (actor["side"] == 1 and event["action_kind"] == 2 and
                                    event["target_count"] == 1 and not event["free_action"]) else None
            elif kind == "target_commit":
                if pending is not None and event["source"]["side"] == 1 and event["target"]["side"] == 0:
                    living = [hp for hp in party.values() if hp > 0]
                    if living and event["hp_before"] == min(living):
                        weakest += 1
                    else:
                        other += 1
                        if focused:
                            raise AssertionError("%s: enemy skill %d hit party %d at %d HP, not the weakest %s" % (
                                attempt, event["skill_id"], event["target"]["id"], event["hp_before"], sorted(living)))
                pending = None
                if event["target"]["side"] == 0:
                    party[event["target"]["id"]] = event["hp_after"]
        if focused:
            assert "DECIDE_BOSS_READY" in text, "%s: witness package did not load" % attempt
            assert "DECIDE_BOSS_PROBE announce=refused log=refused" in text, \
                "%s: snapshot probe missing or accepted a presentation/log call" % attempt
            assert not REFUSED.search(text), "%s: the decide stage refused a choice from d.candidates" % attempt
        if hard:
            assert re.search(r"HARDMODE_RUN \S+ mode=hard ", text), "%s: hard mode run not armed" % attempt
            assert "HARDMODE_ENCOUNTER id=%d boss=true" % encounter in text, \
                "%s: encounter %d not classed as a hard-mode boss" % (attempt, encounter)
    assert runs, "%s: no run produced a trace" % out
    if focused:
        assert weakest, "%s: no enemy single-target skill reached the party" % out
    else:
        assert other, "%s: the native AI always hit the weakest; the base leg proves nothing" % out
    return {"runs": runs, "weakest": weakest, "other": other}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--runs", type=int, default=2)
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--legs", default="base,focus,hard")
    parser.add_argument("--worker", nargs="+", required=True, help="native EXE [ELF]; P4PC_ELF is appended when set")
    args = parser.parse_args()
    scenario_path = os.environ.get("P4PC_DECIDE_BOSS_SCENARIO", "")
    if not scenario_path or not Path(scenario_path).is_file():
        print("mod_battle_decide_sim: P4PC_DECIDE_BOSS_SCENARIO (private boss scenario) unavailable", file=sys.stderr)
        return 77
    scenario_path = Path(scenario_path).resolve()
    worker = list(args.worker)
    if os.environ.get("P4PC_ELF") and len(worker) == 1:
        worker.append(os.environ["P4PC_ELF"])
    source = battle_sim.load_json(scenario_path)
    args.out.mkdir(parents=True, exist_ok=True)
    package = args.out / "packages" / PACKAGE
    package.mkdir(parents=True, exist_ok=True)
    (package / "mod.ini").write_text(INI, encoding="utf-8", newline="\n")
    (package / "main.luau").write_text(MAIN, encoding="utf-8", newline="\n")
    witness = {"id": PACKAGE, "directory": str(package)}
    hardmode = {"id": "p4pc.hardmode", "directory": str(ROOT / "mods" / "p4pc.hardmode")}
    physsplit = {"id": "p4pc.physsplit", "directory": str(ROOT / "mods" / "p4pc.physsplit")}
    legs = {
        "base": ([], {}, False, False),
        "focus": ([witness], {PACKAGE: {}}, True, False),
        "hard": ([physsplit, hardmode, witness], {"p4pc.physsplit": {"split": "on"},
                 "p4pc.hardmode": {"mode": "Hard", "adopt_on_load": "on"}, PACKAGE: {}}, True, True),
    }
    results = {}
    for name in args.legs.split(","):
        packages, settings, focused, hard = legs[name]
        scenario = copy.deepcopy(source)
        scenario["n"] = args.runs
        # A careful player who guards against visible charges, the same in every leg.
        scenario["policy"] = {"kind": "attentive", "version": 1}
        scenario["label"] = "battle.decide@1 boss witness: " + name
        if packages:
            scenario["mods"] = {"packages": packages, "settings": settings}
        else:
            scenario.pop("mods", None)
        out = args.out / name
        shutil.rmtree(out, ignore_errors=True)  # never count an earlier batch's runs
        report = battle_sim.run_batch(scenario, scenario_path.parent, worker, out, args.jobs)
        assert not report["errors"], "%s: %d native errors" % (name, report["errors"])
        results[name] = {"outcomes": report["outcomes"],
                         **check(out, focused, scenario["encounter_id"], hard)}
        print(name, json.dumps(results[name]), flush=True)
    (args.out / "decide-boss-witness.json").write_text(json.dumps(results, indent=1), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
