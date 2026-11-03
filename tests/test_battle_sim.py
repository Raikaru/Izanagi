#!/usr/bin/env python3
"""Game-free native-contract tests; fake worker uses the real CLI/binary envelope."""
from contextlib import contextmanager
import copy
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import battle_sim as sim


def fake_preparation(args):
    """Contract mock only: these byte fixtures are not engine-valid save images."""
    def argument(name):
        return Path(args[args.index(name) + 1])
    request_file = argument("--battle-prepare")
    image_file = argument("--battle-prepared-image")
    receipt_file = argument("--battle-prepare-result")
    request = request_file.read_bytes()
    assert request[:8] == b"P4BPRE01" and struct.unpack_from("<I", request, 8)[0] == 160
    source_size = struct.unpack_from("<I", request, 16)[0]
    actors, personas, items, decisions = struct.unpack_from("<4I", request, 20)
    assert len(request) == 160 + source_size + actors * 64 + personas * 112 + items * 8 + decisions * 24
    source = request[160:160 + source_size]
    mode = source[4:].decode("ascii")
    attempt = int(request_file.parent.name.split("-")[-1])
    assert os.environ["P4PC_INPUT"] == "script" and os.environ["P4PC_PACKS"] == "0"
    assert Path(os.environ["P4PC_SAVE_DIR"]).resolve() == Path.cwd().resolve()   # (macOS's temporary directory is under the /var symlink)
    if mode == "prep_crash" or (mode == "prep_retry" and attempt == 1):
        print("fake preparation crash retained", file=sys.stderr, flush=True)
        return 9
    if mode == "prep_timeout":
        time.sleep(10)
    if mode == "prep_reject":
        receipt_file.write_text(json.dumps({"schema": 1, "operation": "battle_prepare",
                                           "outcome": "preparation_error", "terminal_cause": "native_declared_capability_error"}))
        return 3
    effective = struct.pack("<I", 1) + b"prepared"
    receipt = {"schema": 1, "operation": "battle_prepare", "outcome": "prepared",
               "terminal_cause": "native_save_build_image_complete",
               "definition_sha256": request[128:160].hex(), "request_sha256": sim.digest(request),
               "source_sha256": sim.digest(source), "effective_image_sha256": sim.digest(effective),
               "effective_image_bytes": len(effective), "effective_image_native_valid": True,
               "classification": "source_attested" if struct.unpack_from("<I", request, 36)[0] == 1 else "synthetic_lab"}
    if mode.startswith("prep_bad_"):
        key = mode[len("prep_bad_"):]
        if key == "effective_image_native_valid":
            receipt[key] = False
        elif key == "effective_image_bytes":
            receipt[key] += 1
        elif key == "classification":
            receipt[key] = "unrelated_classification"
        else:
            receipt[key] = "0" * 64
    temporary = image_file.with_suffix(".tmp")
    temporary.write_bytes(effective)
    os.replace(temporary, image_file)
    temporary = receipt_file.with_suffix(".tmp")
    temporary.write_text(json.dumps(receipt), encoding="utf-8")
    os.replace(temporary, receipt_file)
    print("fake native preparation completed", flush=True)
    return 0


def fake_worker():
    args = sys.argv[sys.argv.index("--fake-worker") + 1:]
    if "--battle-prepare" in args:
        return fake_preparation(args)
    def argument(name):
        return Path(args[args.index(name) + 1])
    entry = argument("--battle-sim")
    result = argument("--battle-result")
    trace = argument("--battle-trace")
    assert os.environ["P4PC_ASPECT"] == "4:3"
    assert os.environ["P4PC_RESOLUTION"] == "640x448"
    assert os.environ["P4PC_WINDOW_W"] == "640" and os.environ["P4PC_WINDOW_H"] == "448"
    assert os.environ["P4PC_VSYNC"] == "off" and os.environ["P4PC_CONFIG"] == "none"
    assert Path(os.environ["P4PC_SAVE_DIR"]).resolve() == Path.cwd().resolve()
    assert os.environ["P4PC_INPUT"] == "script" and os.environ["P4PC_PACKS"] == "0"
    assert not any(key in os.environ for key in ("P4PC_BOARD_SEED", "P4PC_WALK_SCRIPT", "P4PC_START_BATTLE", "P4PC_LOCKSTEP_DIR"))
    header = struct.unpack("<8s14I", entry.read_bytes()[:64])
    assert header[0] == b"P4BSIM01" and header[1] == 64
    request = json.loads((entry.parent / "request.json").read_text())
    index = request["run_index"]
    mode = entry.read_bytes()[64 + 4:64 + header[8]].decode("ascii")
    attempt = int(entry.parent.name.split("-")[-1])
    if mode == "crash" or (mode == "retry" and attempt == 1):
        print("retained crash diagnostic", file=sys.stderr, flush=True)
        return 9
    if mode == "timeout":
        time.sleep(10)
    if mode == "invalid":
        result.write_text('{"schema":1,"outcome":"invented"}')
        return 0
    if mode == "missing":
        return 0
    if mode == "success_nonzero":
        outcome = "win"
    elif mode in ("update_limit", "policy_error"):
        outcome = mode
    else:
        outcome = "win" if index % 2 == 0 else "defeat"
    body = {"schema": 1, "outcome": outcome, "terminal_cause": "protagonist_ko" if outcome == "defeat" else "native_terminal",
            "counter_capability": "fake-contract-v1", "trace_capability": "fake-trace-v1", "observer_attached": 1,
            "turns": {"actor_turns": index + 1, "actions": index + 3,
                      "actor_turns_definition": "fake_actor_admissions", "actions_definition": "fake_execution_attempts"},
            "resources": {"hp_spent": 10 + index, "sp_spent": 3, "items_consumed": index % 2,
                          "final_party": [{"id": 1, "hp": max(0, 100 - index), "sp": 20}],
                          "initial_party_boundary": "after_restore_before_constructor"},
            "rng": {"additive": {"draws": index + 4}},
            "worker_environment": {key: os.environ.get(key) for key in ("P4PC_ISO", "SDL_VIDEODRIVER")}}
    temporary = result.with_suffix(".tmp")
    temporary.write_text(json.dumps(body), encoding="utf-8")
    os.replace(temporary, result)
    trace.write_text(json.dumps({"event": "turn", "index": index}) + "\n", encoding="utf-8")
    print("fake native completed", flush=True)
    return 7 if mode == "success_nonzero" else (1 if outcome in sim.CENSORED | sim.ERRORS else 0)


class BattleSimTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.image = self.root / "native.bin"
        self.image.write_bytes(struct.pack("<I", 1) + b"normal")
        self.scenario = {"schema": 1, "entry": {"source": {"kind": "save", "id": "fixture",
                         "image": "native.bin", "sha256": sim.digest(self.image.read_bytes())}},
                         "encounter_id": 7, "policy": {"kind": "tape", "version": 1,
                         "tape": [{"update": 3, "buttons": 0x4000, "hold": 2}]},
                         "master_seed": "000102030405060708090a0b0c0d0e0f", "seed_start": 0, "n": 4,
                         "options": {"update_limit": 100, "bootstrap_limit": 100, "action_limit": 50,
                                     "latency_updates": 1, "fill_resources": False, "timeout_seconds": 10}}
        # The worker identity hashes an executable, not the unreadable Microsoft
        # Store execution alias sometimes exposed as sys.executable.
        python = str(Path(sys.base_prefix) / "python.exe") if os.name == "nt" else sys.executable
        self.worker = [python, str(Path(__file__).resolve()), "--fake-worker"]
        self.lock = threading.Lock()
        self.active = 0
        self.peak = 0
        self.admissions = 0

    @contextmanager
    def admission(self, wait, rendered):
        with self.lock:
            self.admissions += 1
            self.active += 1
            self.peak = max(self.peak, self.active)
        try:
            yield
        finally:
            with self.lock:
                self.active -= 1

    def batch(self, **kwargs):
        return sim.run_batch(self.scenario, self.root, self.worker, self.root / "out",
                             admission=self.admission, **kwargs)

    def mode(self, value):
        self.image.write_bytes(struct.pack("<I", 1) + value.encode("ascii"))
        source = self.scenario["entry"]["source"]
        reference = source["definition"]["baseline"] if source["kind"] == "declarative" else source
        reference["sha256"] = sim.digest(self.image.read_bytes())

    def definition(self, classification="source_attested"):
        return {"schema": 1, "classification": classification,
                "baseline": {"kind": "save", "id": "fake-source", "image": self.image.name,
                             "sha256": sim.digest(self.image.read_bytes())},
                "preparation": {"master_seed": "fedcba98765432100123456789abcdef",
                                "resource_mode": "inherit", "clear_temporary_statuses": False,
                                "bootstrap_limit": 100, "field_resume_limit": 100, "timeout_seconds": 10},
                "context": {}, "party": {"hero": {"mode": "preserve"}, "roster_mode": "preserve",
                                       "selected_slot": "preserve", "personas": [],
                                       "actors": [{"id": 1, "equipment": {"weapon": "preserve", "armor": "preserve", "accessory": "preserve"},
                                                   "resources": {"mode": "preserve"}, "tactic": "preserve"}]},
                "inventory": {"mode": "preserve"}}

    def declarative(self, definition=None):
        definition = self.definition() if definition is None else definition
        self.scenario["entry"]["source"] = {"kind": "declarative", "id": "fake-preparation", "definition": definition}
        return definition

    def test_valid_source_variants(self):
        sim.validate_scenario(self.scenario)
        source = self.scenario["entry"]["source"]
        source.update(kind="walker", parent_id="source-save", walk_sha256="a" * 64)
        sim.validate_scenario(self.scenario)
        self.declarative()
        sim.validate_scenario(self.scenario)

    def test_schema_errors_have_field_paths(self):
        changes = [(lambda s: s.pop("n"), "scenario: missing n"),
                   (lambda s: s.update(typo=True), "unknown fields typo"),
                   (lambda s: s.update(n=True), "scenario.n"),
                   (lambda s: s.update(master_seed="xyz"), "master_seed"),
                   (lambda s: s.update(seed_start=2**64 - 1), "seed range"),
                   (lambda s: s.update(encounter_id=65536), "encounter_id"),
                   (lambda s: s["entry"]["source"].update(kind="bogus"), "source.kind"),
                   (lambda s: s["entry"]["source"].update(kind="walker"), "parent_id"),
                   (lambda s: s["entry"]["source"].update(kind="declarative"), "definition"),
                   (lambda s: s["entry"]["source"].update(sha256="a"), "sha256"),
                   (lambda s: s["policy"].update(kind="telepathic"), "policy.kind"),
                   (lambda s: s["policy"].update(tape=[{"update": 99, "buttons": 1, "hold": 2}]), "exceeds update limit"),
                   (lambda s: s["options"].update(timeout_seconds=float("nan")), "timeout_seconds"),
                   (lambda s: s["options"].update(fill_resources=1), "fill_resources")]
        for change, message in changes:
            with self.subTest(message=message):
                value = copy.deepcopy(self.scenario)
                change(value)
                with self.assertRaisesRegex(sim.ScenarioError, message):
                    sim.validate_scenario(value)

    def test_tape_overlap_and_shape(self):
        self.scenario["policy"]["tape"] *= 2
        with self.assertRaisesRegex(sim.ScenarioError, "overlapping"):
            sim.validate_scenario(self.scenario)
        self.scenario["policy"]["tape"] = "script"
        with self.assertRaisesRegex(sim.ScenarioError, "policy.tape"):
            sim.validate_scenario(self.scenario)

    def test_seed_protocol_known_vector(self):
        # Independent direct encoding of the design, not host random/hash().
        data = b"P4PC-BATTLE-SEEDS\0" + bytes(range(16)) + bytes(8) + b"\x01\0\0\0"
        expected = int.from_bytes(hashlib.sha256(data).digest()[:4], "little")
        self.assertEqual(sim.derive_seeds(self.scenario["master_seed"], 0)["additive"], expected)
        self.assertEqual(sim.derive_seeds(self.scenario["master_seed"].upper(), 0), sim.derive_seeds(self.scenario["master_seed"], 0))
        self.assertNotEqual(sim.derive_seeds(self.scenario["master_seed"], 0), sim.derive_seeds(self.scenario["master_seed"], 1))
        self.assertEqual(len(set(sim.derive_seeds(self.scenario["master_seed"], 0).values())), 4)
        with self.assertRaises(sim.ScenarioError):
            sim.derive_seeds("0" * 32, 2**64)

    def test_binary_entry_matches_contract(self):
        seeds = sim.derive_seeds(self.scenario["master_seed"], 0)
        image = self.image.read_bytes()
        binary = sim.compile_entry(self.scenario, image, seeds)
        fields = struct.unpack("<8s14I", binary[:64])
        self.assertEqual(fields[:4], (b"P4BSIM01", 64, 1, 7))
        self.assertEqual(fields[4:9], (seeds["additive"], seeds["mt"], 100, 1, len(image)))
        self.assertEqual(fields[9:], (0, 100, 0, 1, 50, seeds["effect"]))
        self.assertEqual(binary[64:64 + len(image)], image)
        self.assertEqual(struct.unpack("<III", binary[64 + len(image):]), (3, 0x4000, 2))

    def walk_policy(self, plan="battle:plan=circle,cross,cross;guard=261\nbattlerule:dead@1=circle,down,down,down,cross,cross,cross\n"):
        self.scenario["policy"] = {"kind": "walk", "version": 1, "plan": plan}
        return plan

    def test_walk_policy_plan_is_battle_lines_only(self):
        self.walk_policy()
        sim.validate_scenario(self.scenario)
        changes = [(lambda p: p.pop("plan"), "policy.plan: required"),
                   (lambda p: p.update(plan=""), "expected 1..16384"),
                   (lambda p: p.update(plan="b" * 16385), "expected 1..16384"),
                   (lambda p: p.update(plan="battle:plan=cross\twait:1"), "printable ASCII"),
                   (lambda p: p.update(plan="battle:plan=cross\nwalk:0,0"), "line 2"),
                   (lambda p: p.update(plan="battle:"), "line 1"),
                   (lambda p: p.update(tape=[{"update": 1, "buttons": 1, "hold": 1}]), "policy.tape: only valid"),
                   (lambda p: p.update(kind="greedy"), "only valid with, the walk policy")]
        for change, message in changes:
            with self.subTest(message=message):
                value = copy.deepcopy(self.scenario)
                change(value["policy"])
                with self.assertRaisesRegex(sim.ScenarioError, message):
                    sim.validate_scenario(value)

    def test_walk_entry_carries_plan_bytes_in_place_of_tape(self):
        plan = self.walk_policy()
        image = self.image.read_bytes()
        binary = sim.compile_entry(self.scenario, image, sim.derive_seeds(self.scenario["master_seed"], 0))
        fields = struct.unpack("<8s14I", binary[:64])
        self.assertEqual((fields[7], fields[8], fields[11]), (len(plan), len(image), 4))
        self.assertEqual(binary[64 + len(image):], plan.encode("ascii"))

    def test_walk_result_carries_its_plan_identity_only(self):
        base = {"schema": 1, "outcome": "win", "party_policy": {"id": 4, "name": "walk-battle-rules-v1", "latency_updates": 1,
                                                               "party_control": "native_direct_commands",
                                                               "knowledge": "walker_battle_rules_native_unit_records"}}
        with self.assertRaisesRegex(sim.ScenarioError, "plan_sha256"):
            sim.validate_result(copy.deepcopy(base))
        walk = copy.deepcopy(base)
        walk["party_policy"]["plan_sha256"] = "a" * 64
        sim.validate_result(walk)
        named = copy.deepcopy(walk)
        named["party_policy"]["id"] = 1
        with self.assertRaisesRegex(sim.ScenarioError, "plan_sha256"):
            sim.validate_result(named)

    def test_wilson_and_percentiles(self):
        self.assertIsNone(sim.wilson(0, 0))
        low, high = sim.wilson(5, 10)
        self.assertAlmostEqual(low, 0.2365930905)
        self.assertAlmostEqual(high, 0.7634069095)
        self.assertAlmostEqual(sim.wilson(0, 10)[1], 0.2775327999)
        self.assertAlmostEqual(sim.wilson(10, 10)[0], 0.7224672001)
        stats = sim.distribution([1, 2, 3, 4])
        self.assertEqual(stats["p50"], 2.5)
        self.assertAlmostEqual(stats["p90"], 3.7)
        self.assertEqual(stats["histogram"], {"1": 1, "2": 1, "3": 1, "4": 1})
        self.assertIsNone(sim.distribution([])["mean"])

    def test_parallel_batch_sorted_and_reports_written(self):
        report = self.batch(jobs=min(2, os.cpu_count() or 1))
        self.assertEqual(report["completed"], 4)
        self.assertEqual(report["win_rate"]["rate"], 0.5)
        self.assertEqual([row["run_index"] for row in report["runs"]], [0, 1, 2, 3])
        cohort = report["measurement_cohorts"][0]
        self.assertEqual(cohort["turns"]["all"]["actor_turns"]["p50"], 2.5)
        self.assertEqual(cohort["resources"]["all"]["sp_spent"]["mean"], 3)
        self.assertEqual(report["resource_profile"], "native_inherit")
        self.assertEqual(report["initial_party_boundaries"], {"after_restore_before_constructor": 4})
        self.assertEqual(report["wipe_causes"], {"protagonist_ko": 2})
        self.assertEqual(report["best_seed"]["run_index"], 0)
        self.assertEqual(report["worst_seed"]["run_index"], 3)
        self.assertEqual(self.admissions, 4)
        self.assertLessEqual(self.peak, 2)
        self.assertTrue((self.root / "out/report.json").is_file())
        self.assertIn("Wilson 95% CI", (self.root / "out/summary.md").read_text(encoding="utf-8"))

    def test_retry_once_retains_both_attempts(self):
        self.mode("retry")
        report = self.batch()
        self.assertEqual(report["errors"], 0)
        self.assertEqual(self.admissions, 8)
        for row in report["runs"]:
            self.assertEqual(len(row["attempts"]), 2)
            self.assertEqual(row["attempts"][0]["status"], "infrastructure_error")
            self.assertTrue((Path(row["attempts"][0]["directory"]) / "stderr.log").read_text())

    def test_crash_recorded_not_dropped(self):
        self.mode("crash")
        report = self.batch()
        self.assertEqual((report["requested"], report["errors"], report["completed"]), (4, 4, 0))
        self.assertIsNone(report["win_rate"]["rate"])
        self.assertEqual(report["win_rate"]["all_requested_bounds"], [0, 1])
        self.assertEqual(self.admissions, 8)

    def test_timeout_retried_once(self):
        self.mode("timeout")
        self.scenario["n"] = 1
        self.scenario["options"]["timeout_seconds"] = 0.1
        report = self.batch()
        self.assertEqual(report["errors"], 1)
        self.assertEqual(len(report["runs"][0]["attempts"]), 2)
        self.assertIn("TimeoutExpired", report["runs"][0]["result"]["error"])

    def test_invalid_missing_and_nonzero_success_are_errors(self):
        for mode in ("invalid", "missing", "success_nonzero"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as directory:
                self.mode(mode)
                self.scenario["n"] = 1
                report = sim.run_batch(self.scenario, self.root, self.worker, directory, admission=self.admission)
                self.assertEqual(report["errors"], 1)
                self.assertEqual(len(report["runs"][0]["attempts"]), 2)

    def test_nonzero_native_limit_and_policy_error_are_preserved(self):
        for mode in ("update_limit", "policy_error"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as directory:
                self.mode(mode)
                self.scenario["n"] = 1
                report = sim.run_batch(self.scenario, self.root, self.worker, directory, admission=self.admission)
                self.assertEqual(report["outcomes"], {mode: 1})
                self.assertEqual(len(report["runs"][0]["attempts"]), 1)
                self.assertEqual(report["censored"], int(mode == "update_limit"))

    def test_resume_skips_finished_and_rejects_changed_identity(self):
        first = self.batch()
        second = self.batch(resume=True)
        self.assertEqual(first, second)
        self.assertEqual(self.admissions, 4)
        self.scenario["master_seed"] = "f" * 32
        with self.assertRaisesRegex(sim.ScenarioError, "identical"):
            self.batch(resume=True)

    def test_resume_retains_interrupted_attempt_artifacts(self):
        self.batch()
        case = self.root / "out/run-00000000000000000001"
        (case / "run.json").unlink()
        (case / "interrupted.log").write_text("retain")
        report = self.batch(resume=True)
        self.assertEqual(report["requested"], 4)
        self.assertEqual(self.admissions, 5)
        self.assertEqual((case.with_name(case.name + ".interrupted-1") / "interrupted.log").read_text(), "retain")

    def test_replay_keeps_original_seed_and_full_trace(self):
        report = self.batch(run_index=27)
        row = report["runs"][0]
        self.assertEqual(row["seeds"], sim.derive_seeds(self.scenario["master_seed"], 27))
        directory = Path(row["attempts"][0]["directory"])
        self.assertEqual(sim.load_json(directory / "request.json")["trace"], "full")
        self.assertIn('"event": "turn"', (directory / "trace.jsonl").read_text())

    def test_seed_schedule_independent_of_parallelism(self):
        first = self.batch()
        with tempfile.TemporaryDirectory() as directory:
            second = sim.run_batch(self.scenario, self.root, self.worker, directory,
                                   min(2, os.cpu_count() or 1), admission=self.admission)
        self.assertEqual([(r["seeds"], r["result"]) for r in first["runs"]], [(r["seeds"], r["result"]) for r in second["runs"]])

    def test_hash_mismatch_fails_before_admission(self):
        self.image.write_bytes(b"changed")
        with self.assertRaisesRegex(sim.ScenarioError, "hash mismatch"):
            self.batch()
        self.assertEqual(self.admissions, 0)

    def test_default_admission_is_immediate(self):
        with sim.admitted_case(7) as admitted:
            self.assertIsNone(admitted)
        with sim.admitted_case(-1, rendered=True):
            pass

    def test_unresolved_are_not_silently_removed(self):
        rows = [{"run_index": i, "seeds": sim.derive_seeds("0" * 32, i),
                 "result": {"schema": 1, "outcome": outcome}}
                for i, outcome in enumerate(("win", "defeat", "escape", "update_limit", "engine_error"))]
        report = sim.aggregate(rows)
        self.assertEqual(report["completed"], 3)
        self.assertEqual(report["win_rate"]["rate"], 1 / 3)
        self.assertEqual(report["win_rate"]["resolved_only"]["rate"], 0.5)
        self.assertEqual(report["win_rate"]["all_requested_bounds"], [0.2, 0.6])

    def test_native_unavailable_counters_and_actor_resources(self):
        result = {"schema": 1, "outcome": "win", "turns": {"actor_turns": None, "rounds": None},
                  "resources": {"hp_spent": None, "sp_spent": 5,
                                "initial_party": [{"id": 1, "hp": 100, "sp": 20}],
                                "final_party": [{"id": 1, "hp": 90, "sp": 15}],
                                "items_consumed": {"42": 2}},
                  "capabilities": {"actor_turns": "unqualified native owner"}}
        sim.validate_result(result)
        report = sim.aggregate([{"run_index": 0, "seeds": sim.derive_seeds("0" * 32, 0), "result": result}])
        cohort = report["measurement_cohorts"][0]
        self.assertEqual(cohort["turns"]["all"], {})
        self.assertNotIn("hp_spent", cohort["resources"]["all"])
        self.assertEqual(cohort["resources"]["all"]["final_party.1.hp"]["mean"], 90)
        self.assertEqual(cohort["resources"]["all"]["items_consumed.42"]["mean"], 2)
        self.assertIn("Not reported", sim.markdown(report))
        self.assertIn("additive-stream", report["rng"]["draw_count_scope"])
        self.assertIn("not MT/effect/private", sim.markdown(report))

    def test_admission_failure_is_recorded_and_never_launches_worker(self):
        @contextmanager
        def unavailable(wait, rendered):
            raise TimeoutError("admission deadline")
            yield
        with mock.patch.object(sim.subprocess, "run") as launch:
            report = sim.run_batch(self.scenario, self.root, self.worker, self.root / "out", admission=unavailable)
        launch.assert_not_called()
        self.assertEqual(report["errors"], 4)
        self.assertEqual(len(report["runs"][0]["attempts"]), 2)
        self.assertIsNone(report["runs"][0]["attempts"][0]["wall_seconds"])

    def test_malformed_outcome_is_clear_contract_error(self):
        with self.assertRaisesRegex(sim.ScenarioError, "invalid outcome"):
            sim.validate_result({"schema": 1, "outcome": []})

    def test_empty_aggregation_is_defined(self):
        report = sim.aggregate([])
        self.assertEqual(report["requested"], 0)
        self.assertIsNone(report["win_rate"]["all_requested_bounds"])
        self.assertIsNone(report["best_seed"])

    def test_declarative_definition_errors_precede_any_worker_admission(self):
        self.declarative()
        self.scenario["entry"]["source"]["definition"]["party"]["actors"][0]["id"] = 5
        with self.assertRaisesRegex(sim.ScenarioError, "navigator 5"):
            self.batch()
        self.assertEqual(self.admissions, 0)

    def test_mod_witness_presence_cannot_be_sanitized_into_fidelity(self):
        for value in ("1", ""):
            with self.subTest(value=value), mock.patch.dict(os.environ, {"P4PC_MOD_WITNESS": value}):
                with self.assertRaisesRegex(sim.ScenarioError, "MOD_WITNESS"):
                    sim.run_batch({}, self.root, [], self.root / "out")
                with self.assertRaisesRegex(sim.ScenarioError, "MOD_WITNESS"):
                    sim.prepare_entry({}, b"", [], self.root / "preparation", {})
                with self.assertRaisesRegex(sim.ScenarioError, "MOD_WITNESS"):
                    sim.worker_environment({}, self.root)
        self.assertEqual(self.admissions, 0)
        self.assertFalse((self.root / "out").exists())

    def test_mod_witness_runtime_override_is_refused_even_empty(self):
        for key in ("P4PC_MOD_WITNESS", "p4pc_mod_witness"):
            for value in ("1", ""):
                with self.subTest(key=key, value=value), self.assertRaisesRegex(sim.ScenarioError, "MOD_WITNESS"):
                    sim.worker_environment({key: value}, self.root)

    def test_mod_witness_cli_refuses_before_scenario_read(self):
        for value in ("1", ""):
            with self.subTest(value=value), mock.patch.dict(os.environ, {"P4PC_MOD_WITNESS": value}), \
                    mock.patch.object(sim, "load_json") as read, mock.patch("sys.stderr"):
                with self.assertRaises(SystemExit) as exit_result:
                    sim.main(["batch", "--scenario", str(self.root / "absent.json"),
                              "--worker", self.worker[0], "--out", str(self.root / "out")])
                self.assertEqual(exit_result.exception.code, 2)
                read.assert_not_called()
        self.assertFalse((self.root / "out").exists())

    def test_inherited_display_settings_cannot_change_reference(self):
        with mock.patch.dict(os.environ, {"P4PC_ASPECT": "auto", "P4PC_RESOLUTION": "1920x1080",
                                        "P4PC_BOARD_SEED": "123", "P4PC_WALK_SCRIPT": "competing",
                                        "P4PC_START_BATTLE": "99", "P4PC_LOCKSTEP_DIR": "competing",
                                        "P4PC_ISO": "undeclared-disc", "SDL_VIDEODRIVER": "undeclared-backend"}):
            report = self.batch(run_index=0, rendered=True)
        self.assertEqual(report["errors"], 0)
        self.assertEqual(report["runs"][0]["result"]["worker_environment"],
                         {"P4PC_ISO": None, "SDL_VIDEODRIVER": None})

    def test_declared_runtime_files_are_hashed_and_passed_explicitly(self):
        iso = self.root / "private-disc.iso"
        iso.write_bytes(b"fake-worker-only-disc-fixture")
        self.scenario["runtime"] = {"iso": {"path": iso.name, "sha256": sim.file_digest(iso)},
                                    "video_driver": "declared-test-driver"}
        report = self.batch(run_index=0)
        environment = report["runs"][0]["result"]["worker_environment"]
        self.assertEqual(environment, {"P4PC_ISO": str(iso), "SDL_VIDEODRIVER": "declared-test-driver"})
        self.assertEqual(report["identity"]["runtime"]["iso"]["sha256"], sim.file_digest(iso))
        iso.write_bytes(b"changed disc")
        with self.assertRaisesRegex(sim.ScenarioError, "disc image hash mismatch"):
            self.batch(run_index=0, resume=True)

    def test_runtime_schema_rejects_arbitrary_environment(self):
        self.scenario["runtime"] = {"environment": {"P4PC_BOARD_SEED": "123"}}
        with self.assertRaisesRegex(sim.ScenarioError, "runtime: unknown fields environment"):
            sim.validate_scenario(self.scenario)

    def test_port_executable_and_retail_elf_have_distinct_labeled_pins(self):
        # In this fake invocation the script stands in for the ELF input.
        elf = Path(self.worker[1])
        self.scenario["runtime"] = {"elf": {"path": str(elf), "sha256": sim.file_digest(elf)}}
        report = self.batch(run_index=0)
        self.assertEqual(report["identity"]["port_executable"]["sha256"], sim.file_digest(self.worker[0]))
        self.assertEqual(report["identity"]["runtime"]["elf"]["sha256"], sim.file_digest(elf))
        self.assertNotEqual(report["identity"]["port_executable"]["sha256"],
                            report["identity"]["runtime"]["elf"]["sha256"])

    def test_declared_elf_must_be_the_actual_worker_argument(self):
        self.scenario["runtime"] = {"elf": {"path": str(self.image), "sha256": sim.file_digest(self.image)}}
        with self.assertRaisesRegex(sim.ScenarioError, "explicit worker ELF argument"):
            self.batch()
        self.assertEqual(self.admissions, 0)

    def test_declared_fill_profile_is_explicit_and_transported(self):
        self.scenario["options"]["fill_resources"] = True
        report = self.batch(run_index=0)
        self.assertEqual(report["resource_profile"], "declared_fill_living_hp_sp")
        directory = Path(report["runs"][0]["attempts"][0]["directory"])
        flags = struct.unpack_from("<I", (directory / "entry.bin").read_bytes(), 12)[0]
        self.assertEqual(flags & 4, 4)

    def phase2_result(self):
        # Source-contract fixture only, never an observed native metrics witness.
        return {"schema": 1, "outcome": "win", "entry_kind": "harness-script6",
                "counter_capability": "native-queue-ready-cost-target-v1",
                "trace_capability": "committed-events-and-scoped-state-rng", "observer_attached": 1,
                "table_catalog": {"expanded": 0, "identity": sim.RETAIL_TABLE_CATALOG},
                "party_policy": {"id": 3, "name": "typical-player-v1", "latency_updates": 1,
                                 "party_control": "native_direct_commands", "knowledge": "observed_damage_weakness_only"},
                "entry_observations": {"first_protagonist_command": {
                    "boundary": "native_input_ready", "observed": 1, "update": 100,
                    "party": [{"id": 1, "hp": 999, "sp": 999, "max_hp": 207, "max_sp": 128}]}},
                "turns": {"actor_turns": 4, "actions": 6, "rounds": None,
                          "actor_turns_definition": "ordinary_queue_admissions_including_skips",
                          "actions_definition": "ready_to_execution_attempts_excluding_persona_analyze"},
                "resources": {"hp_spent": 3, "sp_spent": 5, "items_consumed": 1,
                              "cost_hp_adjustment": -792,
                              "spend_definition": "native_party_skill_costs_and_actual_item_decrements",
                              "recovery_scope": "committed_target_packets_not_direct_postbattle_gifts",
                              "initial_party_boundary": "after_restore_before_constructor"}}

    def metric_rows(self, results):
        return [{"run_index": index, "seeds": sim.derive_seeds("0" * 32, index),
                 "result": sim.validate_result(result)} for index, result in enumerate(results)]

    def test_native_definition_labels_are_not_numeric_measurements(self):
        report = sim.aggregate(self.metric_rows([self.phase2_result()]))
        cohort = report["measurement_cohorts"][0]
        self.assertEqual(cohort["contract"]["capabilities"]["counter_capability"], "native-queue-ready-cost-target-v1")
        self.assertEqual(cohort["contract"]["turn_definitions"]["actor_turns_definition"],
                         "ordinary_queue_admissions_including_skips")
        self.assertEqual(cohort["turns"]["all"]["actor_turns"]["mean"], 4)
        self.assertNotIn("actor_turns_definition", cohort["turns"]["all"])
        self.assertNotIn("rounds", cohort["turns"]["all"])
        self.assertEqual(cohort["resources"]["all"]["hp_spent"]["mean"], 3)
        self.assertEqual(cohort["resources"]["all"]["cost_hp_adjustment"]["mean"], -792)
        self.assertIn("native_party_skill_costs", sim.markdown(report))

    def test_differently_defined_turns_are_not_pooled_or_cross_ranked(self):
        first, second = self.phase2_result(), self.phase2_result()
        first["turns"]["actor_turns"] = 100
        second["turns"].update(actor_turns=1, actor_turns_definition="different_turn_definition")
        report = sim.aggregate(self.metric_rows([first, second]))
        self.assertEqual(len(report["measurement_cohorts"]), 2)
        self.assertEqual(sorted(c["turns"]["all"]["actor_turns"]["mean"] for c in report["measurement_cohorts"]), [1, 100])
        self.assertEqual(report["best_seed"]["run_index"], 0)
        self.assertNotIn("turns", report)
        self.assertNotIn("resources", report)
        self.assertIn("differing native", report["replay_ranking"])

    def test_capability_scope_and_boundary_changes_split_measurement_cohorts(self):
        changes = [lambda r: r.update(counter_capability="different-counter-owner"),
                   lambda r: r.update(observer_attached=0),
                   lambda r: r.update(table_catalog={"expanded": 1, "identity": "ab" * 32}),
                   lambda r: r.pop("table_catalog"),
                   lambda r: r["turns"].update(actions_definition="different-attempt-definition"),
                   lambda r: r["resources"].update(spend_definition="different-spend-definition"),
                   lambda r: r["resources"].update(recovery_scope="different-recovery-scope"),
                   lambda r: r["resources"].update(initial_party_boundary="different-resource-boundary"),
                   lambda r: r["party_policy"].update(latency_updates=2),
                   lambda r: r["party_policy"].update(knowledge="different-knowledge-scope"),
                   lambda r: r["entry_observations"]["first_protagonist_command"].update(boundary="different-native-boundary")]
        for change in changes:
            first, second = self.phase2_result(), self.phase2_result()
            change(second)
            with self.subTest(second=second):
                report = sim.aggregate(self.metric_rows([first, second]))
                self.assertEqual(len(report["measurement_cohorts"]), 2)
                self.assertEqual(sorted(c["run_indices"] for c in report["measurement_cohorts"]), [[0], [1]])

    def test_missing_historical_labels_remain_unreported(self):
        historical = {"schema": 1, "outcome": "win", "turns": {"actor_turns": 4}}
        report = sim.aggregate(self.metric_rows([historical, self.phase2_result()]))
        self.assertEqual(len(report["measurement_cohorts"]), 2)
        historical_cohort = next(c for c in report["measurement_cohorts"] if c["run_indices"] == [0])
        self.assertEqual(historical_cohort["contract"]["capabilities"]["counter_capability"], "unreported")
        self.assertEqual(historical_cohort["contract"]["turn_definitions"]["actor_turns_definition"], "unreported")

    def test_native_sparse_item_ledger_counts_zeros_in_each_outcome_group(self):
        results = [self.phase2_result() for _ in range(20)]
        for result in results:
            result["resources"].update(items_consumed=1, items_by_id=[{"id": 808, "used": 1}])
        results[-1]["outcome"] = "defeat"
        results[-1]["resources"].update(items_consumed=4,
                                       items_by_id=[{"id": 769, "used": 3}, {"id": 808, "used": 1}])
        results[-1]["resources"].pop("hp_spent")
        resources = sim.aggregate(self.metric_rows(results))["measurement_cohorts"][0]["resources"]
        all_usage = resources["all"]["items_by_id.769.used"]
        self.assertEqual((all_usage["n"], all_usage["mean"], all_usage["histogram"]),
                         (20, 0.15, {"0": 19, "3": 1}))
        win_usage = resources["win"]["items_by_id.769.used"]
        self.assertEqual((win_usage["n"], win_usage["mean"], win_usage["histogram"]),
                         (19, 0, {"0": 19}))
        self.assertEqual(resources["defeat"]["items_by_id.769.used"]["mean"], 3)
        self.assertEqual(resources["all"]["items_by_id.808.used"]["mean"], 1)
        self.assertEqual(resources["all"]["hp_spent"]["n"], 19)  # Never generic zero fill.

    def test_missing_or_incomplete_item_ledgers_do_not_invent_zero_usage(self):
        changes = [
            lambda r: r.pop("items_by_id"),
            lambda r: r.update(items_by_id=None),
            lambda r: r.update(items_by_id={}),
            lambda r: r.pop("items_consumed"),
            lambda r: r.update(items_consumed=None),
            lambda r: r.update(items_by_id=[{"id": 808, "used": 1}]),  # Total disagrees.
            lambda r: r.update(items_consumed=2, items_by_id=[{"id": 808, "used": 1}] * 2),
            lambda r: r.update(items_consumed=-1, items_by_id=[{"id": 808, "used": -1}]),
            lambda r: r.update(items_consumed=2**64, items_by_id=[{"id": 808, "used": 2**64}]),
            lambda r: r.update(items_consumed=1, items_by_id=[{"id": 808, "used": True}]),
            lambda r: r.update(items_consumed=1, items_by_id=[{"id": "808", "used": 1}]),
            lambda r: r.update(items_consumed=1, items_by_id=[{"id": 1536, "used": 1}]),
            lambda r: r.update(items_consumed=1, items_by_id=[{"id": 808, "used": 1.0}]),
            lambda r: r.update(items_consumed=1, items_by_id=[{"id": 808, "used": 1, "partial": 1}]),
        ]
        for change in changes:
            first, second = self.phase2_result(), self.phase2_result()
            first["resources"].update(items_consumed=3, items_by_id=[{"id": 769, "used": 3}])
            second["resources"].update(items_consumed=0, items_by_id=[])
            change(second["resources"])
            with self.subTest(resources=second["resources"]):
                self.assertFalse(sim.complete_native_item_ledger(second))
                cohort = sim.aggregate(self.metric_rows([first, second]))["measurement_cohorts"][0]
                usage = cohort["resources"]["all"]["items_by_id.769.used"]
                self.assertEqual((usage["n"], usage["mean"], usage["histogram"]),
                                 (1, 3, {"3": 1}))

    def test_sparse_item_zero_fill_never_crosses_native_capability_cohorts(self):
        changes = [None,
                   lambda r: r.update(counter_capability="unsupported-counter-owner"),
                   lambda r: r.pop("counter_capability"),
                   lambda r: r.update(observer_attached=0),
                   lambda r: r.pop("observer_attached"),
                   lambda r: r["resources"].pop("spend_definition"),
                   lambda r: r["resources"].update(spend_definition="different-spend-contract")]
        results = []
        for group, change in enumerate(changes):
            first, second = self.phase2_result(), self.phase2_result()
            first["resources"].update(items_consumed=3, items_by_id=[{"id": 769 + group, "used": 3}])
            second["resources"].update(items_consumed=0, items_by_id=[])
            if change is not None:
                change(first)
                change(second)
            results.extend((first, second))
        cohorts = sim.aggregate(self.metric_rows(results))["measurement_cohorts"]
        self.assertEqual(len(cohorts), len(changes))
        for cohort in cohorts:
            item_id = 769 + cohort["run_indices"][0] // 2
            usage = cohort["resources"]["all"]["items_by_id.%d.used" % item_id]
            for other_id in range(769, 769 + len(changes)):
                if other_id != item_id:
                    self.assertNotIn("items_by_id.%d.used" % other_id, cohort["resources"]["all"])
            if cohort["run_indices"] == [0, 1]:
                self.assertEqual((usage["n"], usage["mean"], usage["histogram"]),
                                 (2, 1.5, {"0": 1, "3": 1}))
            else:
                self.assertEqual((usage["n"], usage["mean"], usage["histogram"]),
                                 (1, 3, {"3": 1}))

    def test_invalid_metric_metadata_has_clear_contract_errors(self):
        changes = [lambda r: r["turns"].update(actor_turns="four"),
                   lambda r: r["turns"].update(actor_turns_definition=4),
                   lambda r: r["resources"].update(spend_definition=4),
                   lambda r: r.update(observer_attached=2),
                   lambda r: r.update(observer_attached=True)]
        for change in changes:
            result = self.phase2_result()
            change(result)
            with self.subTest(result=result), self.assertRaisesRegex(sim.ScenarioError, "worker result"):
                sim.validate_result(result)

    def test_retail_table_catalog_is_the_native_retail_identity(self):
        self.assertEqual(sim.RETAIL_TABLE_CATALOG,
                         "e6474de9ed719a177721498a3ca0fbc63f0cc1164391fe9aa856a513bb28a581")
        cohort = sim.aggregate(self.metric_rows([self.phase2_result()]))["measurement_cohorts"][0]
        self.assertEqual(cohort["contract"]["table_catalog"],
                         {"expanded": 0, "identity": sim.RETAIL_TABLE_CATALOG})

    def test_table_catalog_identity_is_validated_and_never_defaulted(self):
        expanded = self.phase2_result()
        expanded["table_catalog"] = {"expanded": 1, "identity": "ab" * 32}
        sim.validate_result(expanded)
        failed = {"schema": 1, "outcome": "engine_error",
                  "terminal_cause": "table_catalog_identity_unavailable", "table_catalog": None}
        sim.validate_result(failed)
        changes = [lambda r: r.update(table_catalog=None),  # Only a native error may lack it.
                   lambda r: r.update(table_catalog={"expanded": 0}),
                   lambda r: r.update(table_catalog={"expanded": 0, "identity": sim.RETAIL_TABLE_CATALOG, "x": 1}),
                   lambda r: r.update(table_catalog={"expanded": 2, "identity": "ab" * 32}),
                   lambda r: r.update(table_catalog={"expanded": True, "identity": "ab" * 32}),
                   lambda r: r.update(table_catalog={"expanded": 1, "identity": "AB" * 32}),
                   lambda r: r.update(table_catalog={"expanded": 1, "identity": "ab" * 31}),
                   lambda r: r.update(table_catalog={"expanded": 0, "identity": "ab" * 32}),
                   lambda r: r.update(table_catalog={"expanded": 1, "identity": sim.RETAIL_TABLE_CATALOG}),
                   lambda r: r.update(table_catalog="retail")]
        for change in changes:
            result = self.phase2_result()
            change(result)
            with self.subTest(result=result), self.assertRaisesRegex(sim.ScenarioError, "worker result.table_catalog"):
                sim.validate_result(result)
        report = sim.aggregate(self.metric_rows([self.phase2_result(), expanded]))
        self.assertEqual(sorted((c["run_indices"], c["contract"]["table_catalog"]["expanded"])
                                for c in report["measurement_cohorts"]), [([0], 0), ([1], 1)])

    def test_failed_run_without_measurements_does_not_invent_zero_metrics(self):
        rows = self.metric_rows([self.phase2_result(), {"schema": 1, "outcome": "engine_error"}])
        report = sim.aggregate(rows)
        self.assertEqual((report["requested"], report["errors"]), (2, 1))
        self.assertEqual(len(report["measurement_cohorts"]), 1)
        self.assertEqual(report["measurement_cohorts"][0]["run_indices"], [0])
        self.assertEqual(report["measurement_cohorts"][0]["turns"]["all"]["actor_turns"]["n"], 1)

    def test_absent_first_command_observation_is_null_and_not_a_zero_snapshot(self):
        first, second = self.phase2_result(), self.phase2_result()
        second["entry_observations"]["first_protagonist_command"].update(observed=0, update=None, party=None)
        report = sim.aggregate(self.metric_rows([first, second]))
        self.assertEqual(len(report["measurement_cohorts"]), 1)
        self.assertEqual(report["measurement_cohorts"][0]["contract"]["entry_observation_boundaries"],
                         {"first_protagonist_command": "native_input_ready"})
        second["entry_observations"]["first_protagonist_command"]["update"] = 0
        with self.assertRaisesRegex(sim.ScenarioError, "null update/party"):
            sim.validate_result(second)

    def test_preparation_header_actor_and_preserved_persona_layout(self):
        definition = self.definition()
        definition["context"] = {"date": 28, "time_slot": 2, "field": 240, "room": 1, "step": 7}
        definition["party"]["personas"] = [{"owner": 1, "slot": 0, "mode": "preserve_source", "source_slot": 2,
                                          "assertions": {"level": 15, "exp": 1234}}]
        source = self.image.read_bytes()
        binary = sim.compile_preparation(definition, source)
        self.assertEqual(binary[:8], b"P4BPRE01")
        self.assertEqual(struct.unpack_from("<12I", binary, 8),
                         (160, 0, len(source), 1, 1, 0, 0, 1, 0, 0,
                          sim.derive_seeds(definition["preparation"]["master_seed"], 0)["additive"],
                          sim.derive_seeds(definition["preparation"]["master_seed"], 0)["mt"]))
        seeds = sim.derive_seeds(definition["preparation"]["master_seed"], 0)
        self.assertEqual(struct.unpack_from("<I", binary, 56)[0], seeds["effect"])
        self.assertEqual(struct.unpack_from("<8I", binary, 60), (100, 100, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0, 31))
        self.assertEqual(struct.unpack_from("<5I", binary, 92), (28, 2, 240, 1, 7))
        self.assertEqual(binary[116:128], bytes(12))
        self.assertEqual(binary[128:160], hashlib.sha256(sim.canonical(definition)).digest())
        self.assertEqual(binary[160:160 + len(source)], source)
        actor = 160 + len(source)
        self.assertEqual(struct.unpack_from("<8I", binary, actor),
                         (1, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0, 0xFFFFFFFF))
        self.assertEqual(binary[actor + 32:actor + 64], bytes(32))
        persona = actor + 64
        self.assertEqual(struct.unpack_from("<8I", binary, persona), (1, 0, 0xFFFFFFFF, 0, 2, 15, 1234, 0))
        self.assertEqual(struct.unpack_from("<3I", binary, persona + 80), (0, 0, 1))
        self.assertEqual(binary[persona + 92:persona + 112], bytes(20))
        self.assertEqual(len(binary), 160 + len(source) + 64 + 112)

    def test_preparation_exact_and_growth_modes_flatten_decisions_without_save_edits(self):
        definition = self.definition("synthetic_lab")
        definition["party"]["personas"] = [
            {"owner": 1, "slot": 0, "persona_id": 10, "mode": "synthetic_exact",
             "record": {"level": 5, "exp": 1234, "skills": [1, 2, 0, 0, 0, 0, 0, 0],
                        "stats": [10, 11, 12, 13, 14], "bonus_a": [0] * 5, "bonus_b": [0] * 5}},
            {"owner": 2, "slot": 0, "persona_id": 11, "mode": "native_growth",
             "growth": {"origin": "native_init", "level": 7, "exp": 500, "starting_skills": [3, 0, 0, 0, 0, 0, 0, 0]},
             "assertions": {"skills": [3, 4, 0, 0, 0, 0, 0, 0], "stats": [6, 7, 8, 9, 10],
                            "bonus_a": [0] * 5, "bonus_b": [0] * 5},
             "decisions": [{"level": 6, "learned_skill": 4, "replacement_slot": "auto", "removed_skill": 0}]},
            {"owner": 1, "slot": 1, "mode": "preserve_source", "source_slot": 0}]
        source = self.image.read_bytes()
        binary = sim.compile_preparation(definition, source)
        start = 160 + len(source) + 64
        self.assertEqual(struct.unpack_from("<8I", binary, start), (1, 0, 10, 1, 0xFFFFFFFF, 5, 1234, 1))
        self.assertEqual(binary[start + 32:start + 48], bytes(16))
        self.assertEqual(struct.unpack_from("<8H", binary, start + 48), (1, 2, 0, 0, 0, 0, 0, 0))
        self.assertEqual(list(binary[start + 64:start + 69]), [10, 11, 12, 13, 14])
        self.assertEqual(struct.unpack_from("<I", binary, start + 88)[0], 15)
        growth = start + 112
        self.assertEqual(struct.unpack_from("<8I", binary, growth), (2, 0, 11, 2, 0xFFFFFFFF, 7, 500, 1))
        self.assertEqual(struct.unpack_from("<3I", binary, growth + 80), (0, 1, 31))
        self.assertEqual(struct.unpack_from("<3I", binary, start + 224 + 80), (0, 0, 0))
        self.assertEqual(struct.unpack_from("<6I", binary, start + 336), (2, 0, 6, 4, 0xFFFFFFFF, 0))
        self.assertEqual(self.image.read_bytes(), source)

    def test_preparation_inventory_and_explicit_actor_resources_are_typed(self):
        definition = self.definition()
        definition["party"]["hero"] = {"mode": "assert_source", "level": 15, "exp": 1234}
        actor = definition["party"]["actors"][0]
        actor.update(equipment={"weapon": 30, "armor": 40, "accessory": 50},
                     resources={"mode": "explicit", "hp": 999, "sp": 17}, tactic=4)
        definition["inventory"] = {"mode": "exact_battle_consumables", "items": [{"id": 42, "count": 3}, {"id": 43, "count": 0}]}
        source = self.image.read_bytes()
        binary = sim.compile_preparation(definition, source)
        self.assertEqual(struct.unpack_from("<3I", binary, 68), (15, 1234, 0xFFFFFFFF))
        self.assertEqual(struct.unpack_from("<I", binary, 84)[0], 2)
        self.assertEqual(struct.unpack_from("<I", binary, 112)[0], 1)
        self.assertEqual(struct.unpack_from("<8I", binary, 160 + len(source)), (1, 30, 40, 50, 1, 999, 17, 4))
        self.assertEqual(struct.unpack_from("<4I", binary, 160 + len(source) + 64), (42, 3, 43, 0))

    def test_preparation_schema_rejects_native_bounds_and_ambiguous_records(self):
        changes = [
            lambda d: d["party"]["actors"][0].update(id=5),
            lambda d: d["party"]["actors"][0].update(tactic=5),
            lambda d: d["party"]["actors"][0]["resources"].update(mode="explicit", hp=1000, sp=1),
            lambda d: d["party"]["hero"].update(level=1, exp=0),
            lambda d: d["preparation"].update(bootstrap_limit=10000001),
            lambda d: d["party"].update(selected_slot=12),
            lambda d: d["inventory"].update(mode="exact_battle_consumables", items=[{"id": 1, "count": 100}]),
            lambda d: d["inventory"].update(mode="exact_battle_consumables", items=[{"id": 1, "count": 1}] * 2),
            lambda d: d["context"].update(story_flags=1),
            lambda d: d["party"].update(personas=[{"owner": 1, "slot": 1, "mode": "preserve_source", "source_slot": 0}]),
            lambda d: d["party"].update(personas=[{"owner": 11, "slot": 0, "mode": "preserve_source", "source_slot": 0}]),
        ]
        for change in changes:
            definition = self.definition()
            change(definition)
            with self.subTest(definition=definition), self.assertRaises(sim.ScenarioError):
                sim.validate_definition(definition)
        exact = {"owner": 1, "slot": 0, "mode": "synthetic_exact", "persona_id": 10,
                 "record": {"level": 5, "exp": 1234, "skills": [0] * 8, "stats": [10] * 5}}
        for classification, mutation in (("source_attested", lambda p: None),
                                         ("synthetic_lab", lambda p: p["record"].update(skills=[0xFFFF] * 8)),
                                         ("synthetic_lab", lambda p: p["record"].update(bonus_a=[1] * 5, bonus_b=[0] * 5))):
            definition = self.definition(classification)
            persona = copy.deepcopy(exact)
            mutation(persona)
            definition["party"]["personas"] = [persona]
            with self.subTest(classification=classification, persona=persona), self.assertRaises(sim.ScenarioError):
                sim.validate_definition(definition)
        for persona_id, valid in ((0, False), (256, True), (511, True), (0xFFFF, True), (0x10000, False)):
            definition = self.definition("synthetic_lab")
            persona = copy.deepcopy(exact)
            persona["persona_id"] = persona_id
            definition["party"]["personas"] = [persona]
            with self.subTest(persona_id=persona_id):
                if valid:
                    sim.validate_definition(definition)
                else:
                    with self.assertRaisesRegex(sim.ScenarioError, "persona_id"):
                        sim.validate_definition(definition)

    def test_preparation_freezes_once_before_all_seed_workers(self):
        definition = self.declarative()
        original = self.image.read_bytes()
        report = self.batch(jobs=min(2, os.cpu_count() or 1))
        self.assertEqual(self.admissions, 5)
        self.assertEqual(report["requested"], 4)
        self.assertEqual(report["preparation"]["native_receipt"]["classification"], "source_attested")
        self.assertEqual(report["identity"]["source_entry_sha256"], sim.digest(original))
        prepared_hash = report["preparation"]["native_receipt"]["effective_image_sha256"]
        self.assertEqual(report["identity"]["entry_sha256"], prepared_hash)
        self.assertNotEqual(prepared_hash, sim.digest(original))
        self.assertEqual(report["identity"]["preparation"]["definition_sha256"], sim.digest(sim.canonical(definition)))
        for row in report["runs"]:
            directory = Path(row["attempts"][0]["directory"])
            entry = (directory / "entry.bin").read_bytes()
            size = struct.unpack_from("<I", entry, 36)[0]
            self.assertEqual(sim.digest(entry[64:64 + size]), prepared_hash)
        self.assertEqual(self.image.read_bytes(), original)

    def test_preparation_resume_reuses_frozen_image_without_regrowth(self):
        self.declarative()
        first = self.batch()
        second = self.batch(resume=True)
        self.assertEqual(first, second)
        self.assertEqual(self.admissions, 5)

    def test_preparation_replay_requires_and_reuses_original_frozen_entry(self):
        self.declarative()
        with self.assertRaisesRegex(sim.ScenarioError, "requires --prepared-from"):
            self.batch(run_index=27)
        first = self.batch()
        replay = sim.run_batch(self.scenario, self.root, self.worker, self.root / "replay",
                               run_index=27, prepared_from=self.root / "out", admission=self.admission)
        self.assertEqual(self.admissions, 6)
        self.assertEqual(replay["identity"]["entry_sha256"], first["identity"]["entry_sha256"])
        self.assertEqual(replay["runs"][0]["seeds"], sim.derive_seeds(self.scenario["master_seed"], 27))
        self.assertFalse((self.root / "replay/preparation").exists())

    def test_preparation_recovery_adopts_atomic_publication_not_a_new_process(self):
        self.declarative()
        first = self.batch()
        preparation = self.root / "out/preparation"
        (preparation / "frozen.json").unlink()
        (preparation / "attempt-1/attempt.json").unlink()
        second = self.batch(resume=True)
        self.assertEqual(first["identity"], second["identity"])
        self.assertEqual(self.admissions, 5)

    def test_changed_frozen_preparation_fails_without_regeneration(self):
        self.declarative()
        self.batch()
        image = self.root / "out/preparation/attempt-1/effective-save.bin"
        image.write_bytes(image.read_bytes() + b"changed")
        with self.assertRaisesRegex(sim.ScenarioError, "effective_image_bytes mismatch"):
            self.batch(resume=True)
        self.assertEqual(self.admissions, 5)

    def test_preparation_crash_retries_once_and_retains_both_attempts(self):
        self.mode("prep_retry")
        self.declarative()
        report = self.batch()
        self.assertEqual(self.admissions, 6)
        self.assertEqual(report["preparation"]["frozen"]["attempt"], 2)
        preparation = self.root / "out/preparation"
        self.assertEqual(sim.load_json(preparation / "attempt-1/attempt.json")["status"], "infrastructure_error")
        self.assertTrue((preparation / "attempt-1/stderr.log").read_text())

    def test_preparation_native_rejection_stops_without_retry_or_seed_jobs(self):
        self.mode("prep_reject")
        self.declarative()
        with self.assertRaisesRegex(sim.ScenarioError, "no seed workers launched"):
            self.batch()
        self.assertEqual(self.admissions, 1)
        self.assertFalse((self.root / "out/preparation/attempt-2").exists())
        self.assertEqual(list((self.root / "out").glob("run-*")), [])
        with self.assertRaisesRegex(sim.ScenarioError, "recorded native preparation failure"):
            self.batch(resume=True)
        self.assertEqual(self.admissions, 1)

    def test_preparation_success_requires_every_bound_native_receipt_identity(self):
        keys = ("definition_sha256", "request_sha256", "source_sha256", "effective_image_sha256",
                "effective_image_bytes", "effective_image_native_valid", "classification")
        for key in keys:
            self.mode("prep_bad_" + key)
            self.declarative()
            before = self.admissions
            with self.subTest(key=key), tempfile.TemporaryDirectory() as output:
                with self.assertRaisesRegex(sim.ScenarioError, "failed after two attempts"):
                    sim.run_batch(self.scenario, self.root, self.worker, output, admission=self.admission)
                self.assertEqual(self.admissions - before, 2)
                self.assertEqual(list(Path(output).glob("run-*")), [])

    def test_preparation_fill_and_actor_explicit_resources_cannot_compete(self):
        definition = self.definition()
        definition["preparation"].update(resource_mode="fill_living_hp_sp", clear_temporary_statuses=True)
        binary = sim.compile_preparation(definition, self.image.read_bytes())
        self.assertEqual(struct.unpack_from("<2I", binary, 40), (1, 1))
        definition["party"]["actors"][0]["resources"] = {"mode": "explicit", "hp": 50, "sp": 20}
        with self.assertRaisesRegex(sim.ScenarioError, "competing_resource_policies"):
            sim.validate_definition(definition)

    def test_growth_from_companion_source_keeps_source_slot_zero(self):
        definition = self.definition("synthetic_lab")
        definition["party"]["personas"] = [{"owner": 2, "slot": 0, "persona_id": 10,
                                          "mode": "native_growth", "source_slot": 0,
                                          "growth": {"origin": "source_record", "level": 6, "exp": 234}}]
        source = self.image.read_bytes()
        binary = sim.compile_preparation(definition, source)
        start = 160 + len(source) + 64
        self.assertEqual(struct.unpack_from("<8I", binary, start), (2, 0, 10, 2, 0, 6, 234, 0))
        self.assertEqual(binary[start + 32:start + 80], bytes(48))
        self.assertEqual(struct.unpack_from("<3I", binary, start + 80), (0, 0, 1))
        definition["party"]["personas"][0]["growth"]["origin"] = "native_init"
        with self.assertRaisesRegex(sim.ScenarioError, "omit for native_init"):
            sim.validate_definition(definition)

    def test_missing_publication_after_seed_jobs_never_regrows_the_party(self):
        self.declarative()
        self.batch()
        preparation = self.root / "out/preparation"
        for path in ("frozen.json", "attempt-1/attempt.json", "attempt-1/receipt.json", "attempt-1/effective-save.bin"):
            (preparation / path).unlink()
        with self.assertRaisesRegex(sim.ScenarioError, "refusing to regenerate"):
            self.batch(resume=True)
        self.assertEqual(self.admissions, 5)

    def test_prepared_from_rejects_another_definition_before_any_replay_job(self):
        definition = self.declarative()
        self.batch()
        definition["preparation"]["master_seed"] = "0" * 32
        with self.assertRaisesRegex(sim.ScenarioError, "preparation identity mismatch"):
            sim.run_batch(self.scenario, self.root, self.worker, self.root / "different-replay",
                          run_index=0, prepared_from=self.root / "out", admission=self.admission)
        self.assertEqual(self.admissions, 5)

    def test_preparation_timeout_has_two_infrastructure_receipts_and_no_cases(self):
        self.mode("prep_timeout")
        definition = self.declarative()
        definition["preparation"]["timeout_seconds"] = 0.1
        with self.assertRaisesRegex(sim.ScenarioError, "failed after two attempts"):
            self.batch()
        self.assertEqual(self.admissions, 2)
        self.assertEqual(list((self.root / "out").glob("run-*")), [])
        record = sim.load_json(self.root / "out/preparation/attempt-2/attempt.json")
        self.assertEqual(record["status"], "infrastructure_error")
        self.assertIn("TimeoutExpired", record["error"])

    def test_preparation_skill_domain_is_4095_not_the_full_u16_or_leaf_registry(self):
        definition = self.definition("synthetic_lab")
        persona = {"owner": 1, "slot": 0, "persona_id": 10, "mode": "native_growth",
                   "growth": {"origin": "native_init", "level": 7, "exp": 500,
                              "starting_skills": [0, 576, 587, 4095, 1, 2, 3, 4]},
                   "assertions": {"skills": [0, 576, 587, 4095, 1, 2, 3, 4]},
                   "decisions": [{"level": 6, "learned_skill": 4095, "replacement_slot": "auto", "removed_skill": 0}]}
        definition["party"]["personas"] = [persona]
        binary = sim.compile_preparation(definition, self.image.read_bytes())
        start = 160 + len(self.image.read_bytes()) + 64
        self.assertEqual(struct.unpack_from("<8H", binary, start + 32), (0, 576, 587, 4095, 1, 2, 3, 4))
        for section, key in (("growth", "starting_skills"), ("assertions", "skills")):
            changed = copy.deepcopy(definition)
            changed["party"]["personas"][0][section][key][0] = 4096
            with self.subTest(section=section), self.assertRaises(sim.ScenarioError):
                sim.validate_definition(changed)
        for key in ("learned_skill", "removed_skill"):
            changed = copy.deepcopy(definition)
            changed["party"]["personas"][0]["decisions"][0][key] = 4096
            with self.subTest(key=key), self.assertRaises(sim.ScenarioError):
                sim.validate_definition(changed)

    def test_invalid_unrecorded_preparation_publication_consumes_single_remaining_retry(self):
        output = self.root / "preparation-only"
        output.mkdir()
        definition = self.definition()
        identity = {"coordinator_sha256": sim.file_digest(sim.__file__), "runtime": {},
                    "port_executable": {"path": self.worker[0], "sha256": sim.file_digest(self.worker[0])},
                    "worker_arguments": [{"argument": arg, "sha256": sim.file_digest(arg) if Path(arg).is_file() else None}
                                         for arg in self.worker[1:]]}
        sim.prepare_entry(definition, self.image.read_bytes(), self.worker, output, identity, admission=self.admission)
        preparation = output / "preparation"
        (preparation / "frozen.json").unlink()
        (preparation / "attempt-1/attempt.json").unlink()
        (preparation / "attempt-1/receipt.json").write_text("{invalid publication")
        image, metadata = sim.prepare_entry(definition, self.image.read_bytes(), self.worker, output, identity,
                                             resume=True, admission=self.admission)
        self.assertEqual(self.admissions, 2)
        self.assertEqual(metadata["frozen"]["attempt"], 2)
        self.assertEqual(sim.digest(image), metadata["native_receipt"]["effective_image_sha256"])
        self.assertEqual(sim.load_json(preparation / "attempt-1/attempt.json")["status"], "infrastructure_error")

    def test_native_preparation_failure_requires_the_native_error_contract(self):
        error = {"schema": 1, "operation": "battle_prepare", "outcome": "preparation_error",
                 "terminal_cause": "unsupported_native_capability"}
        self.assertTrue(sim.native_preparation_error(error))
        for change in ({"schema": True}, {"schema": 2}, {"terminal_cause": ""}, {"terminal_cause": None}):
            with self.subTest(change=change):
                self.assertFalse(sim.native_preparation_error({**error, **change}))

    def test_replay_extremes_use_actual_final_party_hp_when_turns_are_unavailable(self):
        first, second = self.phase2_result(), self.phase2_result()
        first["turns"]["actor_turns"] = second["turns"]["actor_turns"] = None
        first["resources"]["final_party"] = [{"id": 1, "hp": 20}, {"id": 2, "hp": 40}]
        second["resources"]["final_party"] = [{"id": 1, "hp": 30}, {"id": 2, "hp": 60}]
        report = sim.aggregate(self.metric_rows([first, second]))
        self.assertEqual(report["best_seed"]["run_index"], 1)
        self.assertEqual(report["worst_seed"]["run_index"], 0)
        self.assertIn("total reported final-party HP", report["replay_ranking"])
        del second["resources"]["final_party"]
        report = sim.aggregate(self.metric_rows([first, second]))
        self.assertEqual(report["best_seed"]["run_index"], 0)

    def test_known_escape_and_scripted_outcomes_do_not_become_hypothetical_wins(self):
        outcomes = ("win", "defeat", "escape", "scripted_conclusion")
        report = sim.aggregate(self.metric_rows([{"schema": 1, "outcome": outcome} for outcome in outcomes]))
        self.assertEqual(report["win_rate"]["rate"], 0.25)
        self.assertEqual(report["win_rate"]["resolved_only"]["rate"], 0.5)
        self.assertEqual(report["win_rate"]["all_requested_bounds"], [0.25, 0.25])

    def test_oversized_source_fails_before_unbounded_file_read_or_admission(self):
        with self.image.open("wb") as source:
            source.truncate(8 * 1024 * 1024 + 1)
        with mock.patch.object(Path, "open", side_effect=AssertionError("oversized image must not be opened")):
            with self.assertRaisesRegex(sim.ScenarioError, "bounded native save image"):
                self.batch()
        self.assertEqual(self.admissions, 0)

    def test_battle_envelope_limit_is_checked_before_any_seed_worker(self):
        with self.image.open("wb") as source:
            source.write(struct.pack("<I", 1))
            source.truncate(8 * 1024 * 1024)
        self.scenario["entry"]["source"]["sha256"] = sim.file_digest(self.image)
        with self.assertRaisesRegex(sim.ScenarioError, "native 8 MiB request limit"):
            self.batch()
        self.assertEqual(self.admissions, 0)


if __name__ == "__main__":
    if "--fake-worker" in sys.argv:
        sys.exit(fake_worker())
    unittest.main()
