#!/usr/bin/env python3
"""Headless check of the bundled hard mode's run lifecycle
(mods/p4pc.hardmode/main.luau) in the real pinned Luau VM.

luau_script_runner runs tests/mod_hardmode_fixture.luau (a synthetic p4
provider: no game, zero memory, the shipped TUNING.TXT), then main.luau, then
tests/mod_hardmode_lifecycle_test.luau, which calls the save block main.luau
registered as the game does: a reset at boot and before every LOAD GAME, then
the load. The fixture's sprites follow the native pool (ui_overlay_native.c:
32 records for every mod, released only when the VM closes). Checked:

  * no badge at boot; a run adopted from START FROM THE NEXT LOADED SAVE shows
    the Camp badge and clears the setting;
  * forty LOAD GAMEs of that run's save in one session (more than the pool)
    each remove the badge at the reset and show it again at the load, from one
    sprite, with no refusal;
  * a load without a hard-mode block ends the run and the badge.

Not a gameplay witness; mod_hardmode_game covers the badge in the real Camp.

    mod_hardmode_lifecycle_test.py -- [emulator] luau_script_runner
"""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / "mods" / "p4pc.hardmode" / "main.luau"
TUNING = ROOT / "mods" / "p4pc.hardmode" / "files" / "DATA" / "HARDMODE" / "TUNING.TXT"
FIXTURE = ROOT / "tests" / "mod_hardmode_fixture.luau"
SCRIPT = ROOT / "tests" / "mod_hardmode_lifecycle_test.luau"

EXPECTED = {
    "READY": "true",
    "BOOT_BADGE": "false",
    "ADOPTED": "true true false",
    "BADGE_CYCLES": "40 true",
    "OFF_AFTER_PLAIN_LOAD": "true",
    "SPRITES": "1",
    "REFUSED": "0",
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
    print("mod_hardmode_lifecycle: boot, adoption, 40 reset/load cycles from one Camp badge sprite, plain load passed")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
