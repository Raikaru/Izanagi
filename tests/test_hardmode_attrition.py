#!/usr/bin/env python3
"""Focused tests for tools/hardmode_attrition.py (pure Python, no game).

Fixtures are synthetic RESULT documents and a short walk-log excerpt using
the exact log markers hard mode writes; they never fabricate a metric the
logs do not carry (unobservable stays None).
"""
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import hardmode_attrition as attr


def result(outcome, initial_sp, final_sp, spent, hero_hp=None, wipe_cause=None):
    doc = {"outcome": outcome,
           "resources": {"initial_party": [{"id": 1, "hp": 100, "sp": initial_sp}],
                         "final_party": [{"id": 1, "hp": 50, "sp": final_sp}],
                         "sp_spent": spent}}
    if hero_hp is not None:
        doc["entry_observations"] = {
            "first_protagonist_command": {
                "observed": 1, "update": 10,
                "party": [{"id": 1, "hp": hero_hp, "sp": initial_sp}]}}
    if wipe_cause:
        doc["wipe_cause"] = wipe_cause
    return doc


WALK_LOG = (
    "p4pc: mod p4pc.hardmode: HARDMODE_ENCOUNTER id=887 boss=false epoch=1\n"
    "p4pc: mod p4pc.hardmode: HARDMODE_ENCOUNTER id=888 boss=false epoch=1\n"
    "p4pc: LOG frame 100 field 7 room 2 step 1\n"
    "end <dng_decease> once\n")


class AttritionTest(unittest.TestCase):
    def test_wilson_known_interval(self):
        lo, hi = attr.wilson(42, 80)
        self.assertAlmostEqual(lo, 0.417, places=3)
        self.assertAlmostEqual(hi, 0.631, places=3)
        self.assertIsNone(attr.wilson(0, 0))

    def test_healing_sp_from_resources(self):
        # final 80 - initial 100 + spent 30 = 10 recovered.
        rec = attr.parse_result(result("win", 100, 80, 30), "hard")
        self.assertEqual(rec["healing_sp"], 10.0)
        rec = attr.parse_result(result("defeat", 100, 40, 20, wipe_cause="protagonist_ko"), "hard")
        self.assertEqual(rec["deaths"], 1)
        rec = attr.parse_result(result("defeat", 100, 40, 20, wipe_cause="timer"), "hard")
        self.assertEqual(rec["deaths"], 0)

    def test_unobservable_stays_null(self):
        rec = attr.parse_result({"outcome": "win"}, "retail")
        self.assertIsNone(rec["healing_sp"])
        self.assertIsNone(rec["ambush_deaths"])
        # A walk log has no SP or first-command observation at all.
        walk = attr.parse_walk_log(WALK_LOG, "hard", "castle-6f.log")
        self.assertIsNone(walk["healing_sp"])
        self.assertIsNone(walk["ambush_deaths"])
        self.assertEqual(walk["encounters"], 2)
        self.assertEqual(walk["deaths"], 1)
        self.assertEqual(len(walk["floors"]), 1)

    def test_ambush_first_command(self):
        rec = attr.parse_result(result("defeat", 100, 40, 20, hero_hp=0), "hard")
        self.assertEqual(rec["ambush_deaths"], 1)
        rec = attr.parse_result(result("defeat", 100, 40, 20, hero_hp=40), "hard")
        self.assertEqual(rec["ambush_deaths"], 0)

    def test_summary_groups(self):
        records = [attr.parse_result(result("win", 100, 80, 30), "hard"),
                   attr.parse_result(result("defeat", 100, 40, 20,
                                            wipe_cause="protagonist_ko"), "hard"),
                   attr.parse_result(result("win", 50, 50, 0), "retail"),
                   attr.parse_walk_log(WALK_LOG, "hard", "castle-6f.log")]
        metrics = attr.summarize(records)
        self.assertEqual(metrics["hard"]["encounters"], 4)
        self.assertEqual(metrics["hard"]["healing_sp_per_fight"]["n"], 2)
        self.assertAlmostEqual(metrics["hard"]["healing_sp_per_fight"]["mean"], 5.0)
        self.assertEqual(metrics["hard"]["deaths_per_100_encounters"]["per_100"], 50.0)
        self.assertIsNone(metrics["retail"]["first_turn_ambush_deaths"])
        # Walk-log encounters carry no outcome: they never enter the win rate.
        self.assertEqual(metrics["hard"]["win_rate"]["n"], 2)
        self.assertEqual(metrics["hard"]["win_rate"]["wins"], 1)
        walk_only = attr.summarize([attr.parse_walk_log(WALK_LOG, "harder", "castle.log")])
        self.assertEqual(walk_only["harder"]["encounters"], 2)
        self.assertIsNone(walk_only["harder"]["win_rate"])

    def test_load_and_report_json(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "hard-result.json").write_text(
                json.dumps(result("win", 100, 90, 10)), encoding="utf-8")
            (tmp / "castle-6f.log").write_text(WALK_LOG, encoding="utf-8")
            records = []
            for name in ("hard-result.json", "castle-6f.log"):
                records.extend(attr.load_file(tmp / name))
            metrics = attr.summarize(records)
            self.assertIn("hard", metrics)
            self.assertEqual(metrics["hard"]["encounters"], 3)


if __name__ == "__main__":
    unittest.main()
