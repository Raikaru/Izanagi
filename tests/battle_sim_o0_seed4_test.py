#!/usr/bin/env python3
"""Run the private frozen Knight seed 4 in an O0 native CPU build."""
import hashlib
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import battle_sim as sim  # noqa: E402

FROZEN_SHA = "e33ce8146839b7b5ed1a83dfde29bb3c65f22e543f9611857c4f80aeb3524894"
ENTRY_SHA = "9077b5d91b902f4c7e530f2918299acc9541eaf3fc7fd368588ebd89d45202ef"
SEEDS = {"additive": 254788191, "effect": 746025878, "mt": 636318756, "policy": 3650968460}
COHORT = "7f634a9bc58256eafdfbc3740b668f8c421103207438038b65c73b924d701210"


def main():
    scenario_name = os.environ.get("P4PC_O0_SEED4_SCENARIO")
    frozen_name = os.environ.get("P4PC_O0_SEED4_FROZEN")
    golden_name = os.environ.get("P4PC_O0_SEED4_GOLDEN")
    if not scenario_name or not frozen_name or not golden_name:
        print("private P4PC_O0_SEED4_SCENARIO, P4PC_O0_SEED4_FROZEN and P4PC_O0_SEED4_GOLDEN are required", file=sys.stderr)
        return 77
    scenario_path = Path(scenario_name).resolve()
    scenario = sim.load_json(scenario_path)
    sim.validate_scenario(scenario)
    frozen = Path(frozen_name).read_bytes()
    assert hashlib.sha256(frozen).hexdigest() == FROZEN_SHA
    assert scenario["encounter_id"] == 811 and scenario["seed_start"] == 4 and scenario["n"] == 1
    assert sim.derive_seeds(scenario["master_seed"], 4) == SEEDS
    assert hashlib.sha256(sim.compile_entry(scenario, frozen, SEEDS)).hexdigest() == ENTRY_SHA
    cohort = sim.mod_cohort(scenario["mods"], scenario_path.parent)
    assert cohort["identity"] == COHORT
    runtime = scenario["runtime"]
    elf, iso = runtime["elf"]["path"], runtime["iso"]["path"]
    with tempfile.TemporaryDirectory(prefix="p4pc-o0-seed4-") as name:
        out = Path(name)
        cohort["root"] = sim.stage_mod_packages(cohort, out)
        row = sim.run_case(scenario, frozen, [sys.argv[1], elf], out, 4,
                           {"entry_sha256": FROZEN_SHA}, cohort=cohort,
                           runtime_environment={"P4PC_ISO": iso, "SDL_VIDEODRIVER": "dummy",
                                                "P4PC_GOLDEN": str(Path(golden_name).resolve())})
        assert row["seeds"] == SEEDS
        assert row["result"]["outcome"] not in sim.ERRORS | sim.CENSORED, row["result"]
        assert row["attempts"][0]["exit_code"] == 0, row["attempts"]
        print("seed 4:", row["result"]["outcome"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
