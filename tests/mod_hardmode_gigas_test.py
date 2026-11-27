#!/usr/bin/env python3
"""Exercise the production hardmode Gigas policy, not a source-text surrogate.

The real Luau runner supplies a synthetic p4 provider. The separate real-VM C
witness exercises production battle.execution delivery and native unit queries.
No gameplay/GPU or retail files. Stock successful infliction results are inputs;
normal chance scaling and guaranteed-rate veto/acceptance are asserted in Luau.
"""
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / "mods/p4pc.hardmode"
EXPECTED = {
    "charge_support_extra_shot", "same_enemy_distinct_uid", "generic_base_turn_phase_cap",
    "baseline_direct_end", "baseline_removed", "baseline_flushed", "baseline_battle_release",
    "baseline_replaced", "baseline_unproven", "hitless_shot_completion", "baseline_unknown_work",
    "baseline_capacity", "baseline_orphan_resume", "baseline_conflict", "baseline_uncertain_command",
    "baseline_missing_identity", "baseline_missing_context", "baseline_broken_pairing", "baseline_epoch",
    "baseline_recycled_data", "nested_unrelated_unchanged", "retry_resets_span", "battle_end_resets_span",
    "off_baseline", "death_baseline", "baseline_changed_data", "baseline_out_of_order", "baseline_status_capacity",
    "baseline_missing_actor_data",
    "kind3_infliction", "guarded_unguarded_cures_unchanged",
    "non_skill_alias_keeps_guard", "baseline_incomplete_completion",
}


def main(argv):
    runner = argv[argv.index("--") + 1:] if "--" in argv else argv
    if not runner:
        print(__doc__, file=sys.stderr)
        return 2
    rows = ET.parse(MOD / "data/windows.xml").getroot().findall("affinity")
    row, = [r for r in rows if r.get("entity") == "311"]
    assert {key: row.get(key) for key in ("kind", "profile", "element", "channel", "value", "scalar")} == {
        "kind": "1", "profile": "311", "element": "10", "channel": "2", "value": "100", "scalar": "100"}
    result = subprocess.run([
        *runner, str(ROOT / "tests/mod_hardmode_fixture.luau"), str(MOD / "main.luau"),
        str(ROOT / "tests/mod_hardmode_gigas_test.luau"), "--", str(MOD / "main.luau"),
        str(MOD / "files/DATA/HARDMODE/TUNING.TXT"),
    ], capture_output=True, text=True, timeout=120)
    assert result.returncode == 0, f"runner failed ({result.returncode}): {result.stderr}{result.stdout[-3000:]}"
    cases = {line[5:-5] for line in result.stdout.splitlines() if line.startswith("CASE ") and line.endswith(" PASS")}
    assert cases == EXPECTED, ("missing/extra witnesses", EXPECTED - cases, cases - EXPECTED)
    assert "mod_hardmode_gigas: PASS" in result.stdout
    print(f"mod_hardmode_gigas: {len(cases)} policy cases PASS; rolled and guaranteed Fear, per-UID guard, baseline abandonment, base aging and resets")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
