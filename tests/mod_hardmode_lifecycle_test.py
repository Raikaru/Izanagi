#!/usr/bin/env python3
"""Headless check of the bundled hard mode's run lifecycle and boss window
(mods/p4pc.hardmode/main.luau) in the pinned Luau VM.

luau_script_runner runs tests/mod_hardmode_fixture.luau (a synthetic p4
provider and a synthetic override appended to the shipped TUNING.TXT), then
main.luau and tests/mod_hardmode_lifecycle_test.luau. The fixture's sprites follow the native
pool (ui_overlay_native.c: 32 records for every mod, released only when the VM
closes). Checked for both retail and Golden enemy contract states:

  * no badge at boot; a run adopted from START FROM THE NEXT LOADED SAVE shows
    the Camp badge and clears the setting;
  * forty LOAD GAMEs of that run's save in one session (more than the pool)
    each remove the badge at the reset and show it again at the load, from one
    sprite, with no refusal;
  * a load without a hard-mode block ends the run and the badge;
  * Off does not arm a run, a plain load cancels a pending New Game, and saved
    runs load under Off while malformed run blocks are refused;
  * a saved run restores under different current settings (a settings/save
    fixture, not evidence that either SKILL-table variant was selected);
  * the Contrarian King's Silence window under the v1 rules (designed landing
    chance, one turn, once per phase, no skill lock), encounter reset, the
    Technical factors (party x1.5, enemy x1.25) and run/feature gates, plus
    the Knight's charge guard;
  * per-enemy reward callbacks preserve boss/ordinary and enemy-specific tuning,
    rounding, limits, new-game arming and saved-run enable/disable behavior.
The table row is checked separately. This is not a composed-table or gameplay
witness; mod_hardmode_game covers the badge in the real Camp.

    mod_hardmode_lifecycle_test.py -- [emulator] luau_script_runner
"""
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / "mods" / "p4pc.hardmode" / "main.luau"
TUNING = ROOT / "mods" / "p4pc.hardmode" / "files" / "DATA" / "HARDMODE" / "TUNING.TXT"
FIXTURE = ROOT / "tests" / "mod_hardmode_fixture.luau"
SCRIPT = ROOT / "tests" / "mod_hardmode_lifecycle_test.luau"
WINDOWS = ROOT / "mods" / "p4pc.hardmode" / "data" / "windows.xml"
SIDE = ROOT / "mods" / "p4pc.hardmode" / "data" / "freezeshock.xml"

EXPECTED = {
    "READY": "true",
    "BOOT_BADGE": "false",
    "BOOT_FREEZESHOCK": "false 0",
    "BOOT_POLICY": "false",
    "ADOPT_POLICY": "true",
    "ADOPTED": "true true false",
    "ADOPTED_FREEZESHOCK": "true true",
    "BADGE_CYCLES": "40 true",
    "OFF_AFTER_PLAIN_LOAD": "true",
    "FREEZESHOCK_AFTER_PLAIN_LOAD": "false 82",
    "PLAIN_POLICY": "false",
    "NEW_GAME_POLICY": "true",
    "SPRITES": "1",
    "REFUSED": "0",
    "OFF_NO_ARM": "true",
    "SAVED_OFF_AND_REFUSAL": "true",
    "PLAIN_CANCELS_PENDING": "true",
    "BOSS_WINDOW": "true",
    "REWARD_CONSUMER": "true",
}


def main(argv):
    runner = argv[argv.index("--") + 1:] if "--" in argv else argv
    if not runner:
        print(__doc__, file=sys.stderr)
        return 2
    side = ET.parse(SIDE).getroot()
    bosses = {258, 317, 269, 272, 273, *range(257, 268), *range(270, 275),
              *range(288, 298), *range(304, 320)}
    for boss in bosses:
        for leaf in ("24", "25"):
            rows = [row for row in side.findall("affinity")
                    if row.get("entity") == str(boss) and row.get("element") == leaf]
            assert rows and all(row.get("value") == "0x01000064" for row in rows), (boss, leaf)
    burn = [row for row in side.findall("status-ailment") if row.get("element") == "26"]
    assert len(burn) == 1 and burn[0].get("flags") == "54" and burn[0].get("default-from") == "1"
    riders = {int(row.get("skill"), 0): int(row.get("rate"))
              for row in side.findall("status-rider") if row.get("element") == "26"}
    assert riders == {1: 10, 2: 12, 3: 15, 4: 6, 5: 8, 6: 10, 7: 10, 8: 60}, riders
    assert all(row.get("id") != "27" for row in side.findall("element"))
    assert all(row.get("element") != "27" for row in side.findall("status-ailment") + side.findall("status-rider"))
    rows = ET.parse(WINDOWS).getroot().findall("affinity")
    king = [row.attrib for row in rows if row.get("kind") == "1" and row.get("entity") == "304"]
    assert king == [{"kind": "1", "entity": "304", "profile": "304", "element": "15",
                     "channel": "2", "value": "100", "scalar": "100"}], king
    for baseline in ("retail", "golden"):
        result = subprocess.run([*runner, str(FIXTURE), str(MOD), str(SCRIPT), "--", str(MOD), str(TUNING), baseline],
                                capture_output=True, text=True, timeout=120)
        if result.returncode != 0:
            raise AssertionError(f"{baseline} runner failed ({result.returncode}): {result.stderr}{result.stdout[-2000:]}")
        facts = {}
        for line in result.stdout.splitlines():
            key, _, rest = line.partition(" ")
            facts[key] = rest
        assert facts == EXPECTED, {k: (facts.get(k), v) for k, v in EXPECTED.items() if facts.get(k) != v} or facts
    print("mod_hardmode_lifecycle: badge, save transitions and synthetic boss window callbacks passed for both baselines")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
