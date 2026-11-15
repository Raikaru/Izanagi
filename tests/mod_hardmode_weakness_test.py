#!/usr/bin/env python3
"""Regression check: on a hard-mode run no boss window element is also that
boss's weakness, on Golden-off or Golden-on rows, on HARD or a HARDER save
(mods/p4pc.hardmode/main.luau, decision of 2026-11-10).

luau_script_runner runs tests/mod_hardmode_fixture.luau (a synthetic p4
provider whose p4.data rows stand in for the loaded tables), then main.luau,
then tests/mod_hardmode_weakness_test.luau, which drives an OFF load, a HARD
New Game and a loaded HARDER run to their first battle for both row sets and
emits every row afterwards. Checked against the shipped TUNING.TXT:

  * on HARD and HARDER, each [windows] boss is no longer weak to its window's
    Technical element ([technical]) nor to any other element;
  * the Charming Prince (269), no window boss, keeps his Ice weakness;
  * an OFF run leaves every row as loaded (Golden's weaknesses are still there).

Not a gameplay witness: mod_hardmode_game's yukiko scenario logs the rule in
the real fight (HARDMODE_WEAKNESS).

    mod_hardmode_weakness_test.py -- [emulator] luau_script_runner
"""
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / "mods" / "p4pc.hardmode" / "main.luau"
TUNING = ROOT / "mods" / "p4pc.hardmode" / "files" / "DATA" / "HARDMODE" / "TUNING.TXT"
FIXTURE = ROOT / "tests" / "mod_hardmode_fixture.luau"
SCRIPT = ROOT / "tests" / "mod_hardmode_weakness_test.luau"
PRINCE = 269
# The DatUnit ailment masks main.luau's [technical] names stand for.
AILMENTS = {0x02: "rage", 0x04: "fear", 0x08: "silence", 0x10: "panic"}


def window_elements():
    """{window boss ID: its Technical element} from the shipped tuning file."""
    sections, section = {}, ""
    for line in TUNING.read_text(encoding="ascii").splitlines():
        line = line.split("#", 1)[0].strip()
        header = re.fullmatch(r"\[(.+)\]", line)
        if header:
            section = header.group(1)
        elif "=" in line:
            key, value = (part.strip() for part in line.split("=", 1))
            sections.setdefault(section, {})[key] = value
    technical = sections["technical"]
    return {int(key[6:]): technical[AILMENTS[int(value, 0)]]
            for key, value in sections["windows"].items() if key.startswith("enemy.")}


def main(argv):
    runner = argv[argv.index("--") + 1:] if "--" in argv else argv
    if not runner:
        print(__doc__, file=sys.stderr)
        return 2
    result = subprocess.run([*runner, str(FIXTURE), str(MOD), str(SCRIPT), "--", str(MOD), str(TUNING)],
                            capture_output=True, text=True, timeout=120)
    if result.returncode != 0:
        raise AssertionError(f"runner failed ({result.returncode}): {result.stderr}{result.stdout[-2000:]}")
    rows, edits = {}, None
    for line in result.stdout.splitlines():
        found = re.fullmatch(r"ROW (\w+) (\w+) (\d+) (\S*)", line)
        if found:
            name, mode, enemy, text = found.groups()
            rows[(name, mode, int(enemy))] = dict(pair.split("=") for pair in text.split(",") if pair)
        elif line.startswith("EDITS "):
            edits = int(line[6:])
    windows = window_elements()
    assert windows, "the shipped tuning names no window boss"
    # Golden's documented weaknesses on window bosses: an OFF run keeps them.
    assert rows[("golden_on", "off", 258)].get("ice") == "weak", "OFF lost Golden Yukiko's Ice weakness"
    assert rows[("golden_on", "off", 310)].get("fire") == "weak", "OFF lost the Golden Knight's Fire weakness"
    for name in ("golden_off", "golden_on"):
        assert rows[(name, "off", PRINCE)].get("ice") == "weak", (name, "the fixture Prince is not weak to Ice")
        for mode in ("hard", "harder"):
            for enemy, element in windows.items():
                row = rows[(name, mode, enemy)]
                assert row.get(element) != "weak", (name, mode, enemy, f"window element {element} is a weakness")
                assert "weak" not in row.values(), (name, mode, enemy, "a window boss kept a weakness", row)
            assert rows[(name, mode, PRINCE)].get("ice") == "weak", (name, mode, "the Prince lost his Ice weakness")
    # One edit per hard-mode run with something to clear (Golden on: HARD and
    # HARDER), none for OFF, the retail rows or a later battle.
    assert edits == 2, f"{edits} session edits, expected 2"
    print(f"mod_hardmode_weakness: {len(windows)} window bosses keep no weakness on HARD and HARDER, "
          "Golden off and on; the Prince keeps Ice; OFF keeps Golden's weaknesses")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
