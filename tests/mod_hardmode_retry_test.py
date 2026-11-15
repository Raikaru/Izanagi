#!/usr/bin/env python3
"""Headless check of the bundled hard mode's one-way step-down and in-place boss
retry (mods/p4pc.hardmode/main.luau) in the real pinned Luau VM.

luau_script_runner runs tests/mod_hardmode_fixture.luau (a synthetic p4
provider: no game, controlled memory and input, the shipped TUNING.TXT), then
main.luau, then tests/mod_hardmode_retry_test.luau. Checked:

  * a HARDER save (new runs are HARD only) loads with its rules and shows
    HARDER; a loaded run keeps its rules when the setting it was loaded under
    disagrees with it;
  * choosing Hard after loading it steps the run down once (persisted
    HARDMODE_STEPDOWN, STEPPED badge), and changing to Off leaves a
    stepped-off tombstone that is persisted, refuses upward and turns the
    battle retry off;
  * a lost boss battle (encounter 811) is offered through the p4.battle.defeat
    handler before the Game Over and held while the player decides; a Cross
    held from the battle answers nothing; GIVE UP lets the Game Over run;
    RETRY BATTLE retries, the retried battle starts with the battle-start
    HP/SP, the lost battle logs no reward and the won one each reward once;
    the run block never carries a retry state;
  * an Ironman "retry" refusal is shown and then lets the Game Over run; an
    ordinary encounter is never retried; a run started with BATTLE RETRY off
    never enables the handler.

The native restore and relaunch are tests/battle_defeat_test.c; this is not a
gameplay witness (real-game checks are listed in the milestone report).

    mod_hardmode_retry_test.py -- [emulator] luau_script_runner
"""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / "mods" / "p4pc.hardmode" / "main.luau"
TUNING = ROOT / "mods" / "p4pc.hardmode" / "files" / "DATA" / "HARDMODE" / "TUNING.TXT"
FIXTURE = ROOT / "tests" / "mod_hardmode_fixture.luau"
SCRIPT = ROOT / "tests" / "mod_hardmode_retry_test.luau"

EXPECTED = {
    "READY": "true",
    "ADOPTED": "HARDER, ADOPTED|true true true",
    "KEPT": "HARDER, ADOPTED|true harder",
    "STEP_HARD": "ADOPTED, STEPPED|true hard true true",
    "STEP_PERSIST": "ADOPTED, STEPPED|true true harder",
    "STEP_OFF": "OFF, ADOPTED, STEPPED|true off true false",
    "OFF_PERSIST": "OFF, ADOPTED, STEPPED|true off",
    "OFF_UP_REFUSED": "OFF, ADOPTED, STEPPED|true true off",
    "RETRY_RUN": "true true",
    "OFFERED": "hold THE PARTY HAS FALLEN|> RETRY BATTLE|  GIVE UP true",
    "EARLY": "hold > RETRY BATTLE",
    "DECLINED": "hold nil none true",
    "ACCEPTED": "hold > RETRY BATTLE retry none true",
    "NO_REWARD": "true",
    "RESTARTED": "true",
    "REWARDS": "1 1 true",
    "BLOCK": "hard true false",
    "IRONMAN": "hold BATTLE RETRY REFUSED|IRONMAN: A LOST BATTLE ENDS THIS RUN hold nil none true",
    "ORDINARY": "nil none true",
    "RETRY_OFF": "false false",
}


def main(argv):
    runner = argv[argv.index("--") + 1:] if "--" in argv else argv
    if not runner:
        print(__doc__, file=sys.stderr)
        return 2
    result = subprocess.run([*runner, str(FIXTURE), str(MOD), str(SCRIPT), "--", str(MOD), str(TUNING)],
                            capture_output=True, text=True, timeout=120)
    if result.returncode != 0:
        raise AssertionError(f"runner failed ({result.returncode}): {result.stderr}{result.stdout[-2000:]}")
    facts = {}
    for line in result.stdout.splitlines():
        key, _, rest = line.partition(" ")
        facts[key] = rest
    assert facts == EXPECTED, {k: (facts.get(k), v) for k, v in EXPECTED.items() if facts.get(k) != v} or facts
    print("mod_hardmode_retry: step-down persistence and upward refusal; boss retry offered, declined, accepted "
          "from the battle start with rewards once, Ironman refusal, ordinary encounters passed")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
