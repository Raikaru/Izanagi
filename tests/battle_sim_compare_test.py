import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("battle_sim_compare", Path(__file__).resolve().parents[1] / "tools/battle_sim_compare.py")
compare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compare)


class CorpusComparison(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.roots = [Path(self.temp.name) / name for name in ("c", "zig")]
        self.cases = []
        for root in self.roots:
            case = root / "run-00000000000000000000" / "attempt-1"
            case.mkdir(parents=True)
            self.cases.append(case)
            manifest = {"scenario": {"seed_start": 0, "n": 1},
                        "input_identity": {"port_executable": {"path": root.name, "sha256": root.name}}}
            (root / "manifest.json").write_text(json.dumps(manifest))
            result = {"outcome": "win", "updates": 123}
            (case / "entry.bin").write_bytes(b"entry")
            (case / "result.json").write_text(json.dumps(result))
            (case / "trace.jsonl").write_bytes(b'{"update":123}\n')
            row = {"run_index": 0, "seeds": {"additive": 123}, "result": result,
                   "attempts": [{"status": "recorded", "exit_code": 0}]}
            (case.parent / "run.json").write_text(json.dumps(row))

    def test_different_worker_identity_is_retained_not_compared(self):
        result = compare.compare(*self.roots)
        self.assertEqual(result["runs"], 1)
        self.assertEqual(len(result["files"]), 3)
        self.assertNotEqual(*result["workers"])

    def test_any_native_byte_delta_fails_even_equivalent_json(self):
        for file in ("entry.bin", "result.json", "trace.jsonl"):
            with self.subTest(file=file):
                path = self.cases[1] / file
                original = path.read_bytes()
                path.write_bytes(original + b" ")
                with self.assertRaisesRegex(compare.Difference, file.replace(".", r"\.")):
                    compare.compare(*self.roots)
                path.write_bytes(original)

    def test_missing_or_unexpected_case_fails(self):
        (self.roots[1] / "run-extra").mkdir()
        with self.assertRaisesRegex(compare.Difference, "unexpected runs"):
            compare.compare(*self.roots)

    def test_errors_limits_and_retries_fail(self):
        path = self.cases[1].parent / "run.json"
        original = json.loads(path.read_text())
        for outcome in ("engine_error", "policy_error", "update_limit", "action_limit"):
            with self.subTest(outcome=outcome):
                row = {**original, "result": {"outcome": outcome}}
                path.write_text(json.dumps(row))
                with self.assertRaisesRegex(compare.Difference, "unsuccessful"):
                    compare.compare(*self.roots)
        original["attempts"] *= 2
        path.write_text(json.dumps(original))
        with self.assertRaisesRegex(compare.Difference, "retried"):
            compare.compare(*self.roots)

    def test_receipt_cannot_substitute_a_different_result(self):
        path = self.cases[1].parent / "run.json"
        row = json.loads(path.read_text())
        row["result"]["updates"] += 1
        path.write_text(json.dumps(row))
        with self.assertRaisesRegex(compare.Difference, "disagrees"):
            compare.compare(*self.roots)


if __name__ == "__main__":
    unittest.main()
