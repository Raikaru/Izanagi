#!/usr/bin/env python3
"""Synthetic regression check for the hard-mode window-weakness rule.

The two fictional affinity datasets exercise OFF, HARD and HARDER, including
multiple weak window entries and a weak non-window control. The private
variant retains the real-data oracle.

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
CONTROL = 269
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
    # Fictional weaknesses in an OFF run remain as supplied.
    assert rows[("golden_on", "off", 258)].get("electric") == "weak", "OFF lost the first synthetic weakness"
    assert rows[("golden_on", "off", 310)].get("ice") == "weak", "OFF lost the second synthetic weakness"
    for name in ("golden_off", "golden_on"):
        assert rows[(name, "off", CONTROL)].get("wind") == "weak", (name, "the synthetic control is not weak")
        for mode in ("hard", "harder"):
            for enemy, element in windows.items():
                row = rows[(name, mode, enemy)]
                assert row.get(element) != "weak", (name, mode, enemy, f"window element {element} is a weakness")
                assert "weak" not in row.values(), (name, mode, enemy, "a window boss kept a weakness", row)
            assert rows[(name, mode, CONTROL)].get("wind") == "weak", (name, mode, "the control lost its weakness")
    # Exactly one edit for each hard-mode run with synthetic weaknesses.
    assert edits == 2, f"{edits} session edits, expected 2"
    print(f"mod_hardmode_weakness: {len(windows)} window bosses keep no weakness on HARD and HARDER, "
          "both synthetic datasets; control and OFF weaknesses preserved")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
