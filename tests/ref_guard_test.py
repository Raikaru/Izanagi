#!/usr/bin/env python3
"""Reference metadata/integrity tests, using synthetic bytes only."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import ref_guard
import mod_witness


class RefGuardTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.data = b"synthetic reference bytes"
        self.digest = hashlib.sha256(self.data).hexdigest()
        self.row = self.digest + "  old.rgb\n"
        self.pins = ref_guard.parse_manifest(self.row)
        self.ref = self.root / "old.rgb"
        self.ref.write_bytes(self.data)

    def test_matching_reference(self):
        ref_guard.verify_file(self.ref, self.pins)
        ref_guard.verify_directory(self.root, self.pins)

    def test_separate_witness_requires_exact_mode_and_script_input(self):
        self.assertFalse(mod_witness.receipt_fields({})["mod_witness"])
        for value, input_mode in (("", "script"), ("0", "script"), ("unexpected", "script"),
                                  ("1", ""), ("1", "physical")):
            with self.subTest(value=value, input_mode=input_mode):
                with self.assertRaisesRegex(ValueError, "accepts only1"):
                    mod_witness.receipt_fields({"P4PC_MOD_WITNESS": value, "P4PC_INPUT": input_mode})
        fields = mod_witness.receipt_fields({"P4PC_MOD_WITNESS": "1", "P4PC_INPUT": "script"})
        self.assertTrue(fields["mod_witness"])
        self.assertFalse(fields["reference_eligible"])

    def test_partial_witness_is_tainted_before_any_finish_receipt(self):
        mod_witness.begin(self.root, {"P4PC_MOD_WITNESS": "1", "P4PC_INPUT": "script"})
        self.assertFalse((self.root / mod_witness.RECEIPT).exists())
        with self.assertRaisesRegex(ValueError, "cannot be a retail reference"):
            ref_guard.verify_file(self.ref, self.pins)
        with self.assertRaisesRegex(ValueError, "cannot reuse"):
            mod_witness.begin(self.root, {})

    def test_separate_witness_refuses_capture_outside_its_owned_root(self):
        outside = self.root.parent / (self.root.name + "-outside.png")
        with self.assertRaisesRegex(ValueError, "owned mod-witness output root"):
            mod_witness.begin(self.root, {"P4PC_MOD_WITNESS": "1", "P4PC_INPUT": "script",
                                          "P4PC_CAPTURE_PNG": str(outside)})
        self.assertFalse((self.root / mod_witness.MARKER).exists())


    def test_fidelity_refuses_every_witness_presence_even_for_matching_pins(self):
        for value in ("", "0", "1", "unexpected"):
            with self.subTest(value=value), mock.patch.dict(os.environ, {"P4PC_MOD_WITNESS": value}):
                with self.assertRaisesRegex(ValueError, "refuses P4PC_MOD_WITNESS"):
                    ref_guard.verify_file(self.ref, self.pins)

    def test_fidelity_refuses_witness_even_for_a_matching_pinned_file(self):
        with mock.patch.dict(os.environ, {"P4PC_MOD_WITNESS": "1"}):
            with self.assertRaisesRegex(ValueError, "refuses P4PC_MOD_WITNESS"):
                ref_guard.verify_file(self.ref, self.pins)

    def test_matching_bytes_from_witness_run_are_not_retail_references(self):
        environment = {"P4PC_MOD_WITNESS": "1", "P4PC_INPUT": "script"}
        mod_witness.begin(self.root, environment)
        receipt = mod_witness.finish(self.root, 0, environment=environment)
        self.assertTrue(receipt["mod_witness"])
        self.assertFalse(receipt["reference_eligible"])
        self.assertTrue(all(row["mod_witness"] for row in receipt["outputs"]))
        with self.assertRaisesRegex(ValueError, "cannot be a retail reference"):
            ref_guard.verify_file(self.ref, self.pins)

    def test_portable_witness_sidecar_survives_copy_outside_run_root(self):
        run = self.root / "mod-run"
        run.mkdir()
        source = run / "capture.rgb"
        source.write_bytes(self.data)
        mod_witness.begin(run, {"P4PC_MOD_WITNESS": "1", "P4PC_INPUT": "script"})
        shutil.copyfile(source, self.ref)
        mod_witness.copy_output_mode(source, self.ref)
        self.assertTrue(Path(str(self.ref) + mod_witness.SIDECAR).is_file())
        with self.assertRaisesRegex(ValueError, "cannot be a retail reference"):
            ref_guard.verify_file(self.ref, self.pins)

    def test_changed_bytes_fail(self):
        self.ref.write_bytes(b"changed out of band")
        with self.assertRaisesRegex(ValueError, "changed outside its commit"):
            ref_guard.verify_file(self.ref, self.pins)

    def test_missing_reference_fails(self):
        self.ref.unlink()
        with self.assertRaises(OSError):
            ref_guard.verify_directory(self.root, self.pins)

    def test_staged_new_name_cannot_be_consumed_unpinned(self):
        staged = self.root / "new.rgb"
        staged.write_bytes(b"new baseline")
        ref_guard.verify_directory(self.root, self.pins)
        with self.assertRaisesRegex(ValueError, "not pinned"):
            ref_guard.verify_file(staged, self.pins)
        pins = ref_guard.parse_manifest(self.row + hashlib.sha256(staged.read_bytes()).hexdigest() + "  new.rgb\n")
        ref_guard.verify_file(staged, pins)

    def test_invalid_manifest(self):
        for text in ("", self.row * 2, "a" * 64 + "  ../old.rgb\n", "bad old.rgb\n"):
            with self.subTest(text=text), self.assertRaises(ValueError):
                ref_guard.parse_manifest(text)

    def git(self, *args):
        return subprocess.run(["git", "-C", str(self.root), *args], check=True, capture_output=True, text=True).stdout.strip()

    def test_history_is_append_only(self):
        self.git("init", "-q")
        self.git("config", "user.email", "ref-test@example.invalid")
        self.git("config", "user.name", "Reference guard test")
        manifest = self.root / "tests" / "refs.sha256"
        manifest.parent.mkdir()
        manifest.write_text(self.row, encoding="ascii")
        self.git("add", "tests/refs.sha256")
        self.git("-c", "commit.gpgSign=false", "commit", "-qm", "Pin synthetic baseline")
        ref_guard.check_history("HEAD", self.pins, self.root)
        ref_guard.check_history("HEAD", {**self.pins, "new.rgb": "0" * 64}, self.root)
        for pins in ({}, {"old.rgb": "0" * 64}):
            with self.subTest(pins=pins), self.assertRaisesRegex(ValueError, "changed or removed"):
                ref_guard.check_history("HEAD", pins, self.root)

    def test_unavailable_git_inspection_is_not_a_rejection_verdict(self):
        self.git("init", "-q")
        self.git("config", "user.email", "ref-test@example.invalid")
        self.git("config", "user.name", "Reference guard test")
        manifest = self.root / "tests" / "refs.sha256"
        manifest.parent.mkdir()
        manifest.write_text(self.row, encoding="ascii")
        self.git("add", "tests/refs.sha256")
        self.git("-c", "commit.gpgSign=false", "commit", "-qm", "Pin synthetic baseline")
        source = Path(ref_guard.__file__).parent
        result = subprocess.run([sys.executable, str(source / "ref_guard.py"),
                                 "--manifest", str(manifest), "--main-ref", "missing-ref"],
                                cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("inspection unavailable", result.stderr)

    def test_stale_untouched_pins_pass_but_actual_pin_changes_remain_strict(self):
        self.git("init", "-q")
        self.git("config", "user.email", "ref-test@example.invalid")
        self.git("config", "user.name", "Reference guard test")
        manifest = self.root / "tests" / "refs.sha256"
        manifest.parent.mkdir()
        manifest.write_text(self.row, encoding="ascii")
        self.git("add", "tests/refs.sha256")
        self.git("-c", "commit.gpgSign=false", "commit", "-qm", "Original reference")
        base = self.git("rev-parse", "HEAD")
        manifest.write_text(self.row + "1" * 64 + "  later.rgb\n", encoding="ascii")
        self.git("add", "tests/refs.sha256")
        self.git("-c", "commit.gpgSign=false", "commit", "-qm", "New pin on main")
        main = self.git("rev-parse", "HEAD")
        self.git("checkout", "-q", "--detach", base)
        (self.root / "ui.txt").write_text("unrelated submission\n")
        self.git("add", "ui.txt")
        self.git("-c", "commit.gpgSign=false", "commit", "-qm", "UI does not touch references")
        ref_guard.check_history(main, self.pins, self.root)
        ref_guard.check_history(main, {**self.pins, "other.rgb": "2" * 64}, self.root)
        ref_guard.check_history(main, {**self.pins, "later.rgb": "1" * 64}, self.root)
        cli = [sys.executable, str(Path(ref_guard.__file__)), "--manifest", str(manifest), "--main-ref", main]
        result = subprocess.run(cli, cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        for pins in ({}, {"old.rgb": "0" * 64}, {**self.pins, "later.rgb": "3" * 64}):
            with self.subTest(pins=pins), self.assertRaisesRegex(ValueError, "changed or removed"):
                ref_guard.check_history(main, pins, self.root)

    def test_precontract_absence_passes_but_actual_manifest_deletion_fails(self):
        self.git("init", "-q")
        self.git("config", "user.email", "ref-test@example.invalid")
        self.git("config", "user.name", "Reference guard test")
        (self.root / "ui.txt").write_text("before reference contract\n")
        self.git("add", "ui.txt")
        self.git("-c", "commit.gpgSign=false", "commit", "-qm", "Before contract")
        base = self.git("rev-parse", "HEAD")
        manifest = self.root / "tests" / "refs.sha256"
        manifest.parent.mkdir()
        manifest.write_text(self.row, encoding="ascii")
        self.git("add", "tests/refs.sha256")
        self.git("-c", "commit.gpgSign=false", "commit", "-qm", "Introduce reference contract")
        main = self.git("rev-parse", "HEAD")
        self.git("checkout", "-q", "--detach", base)
        cli = [sys.executable, str(Path(ref_guard.__file__)), "--manifest", str(manifest), "--main-ref", main]
        result = subprocess.run(cli, cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.git("checkout", "-q", "--detach", main)
        manifest.unlink()
        self.git("add", "-u")
        self.git("-c", "commit.gpgSign=false", "commit", "-qm", "Delete pinned manifest")
        result = subprocess.run(cli, cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("changed or removed", result.stderr)

    def test_bad_reference_is_a_hard_cli_failure(self):
        manifest = self.root / "refs.sha256"
        manifest.write_text(self.row, encoding="ascii")
        self.ref.write_bytes(b"wrong")
        result = subprocess.run([sys.executable, str(Path(ref_guard.__file__)), "--manifest", str(manifest), "--file", str(self.ref)],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn("ref_guard: FAIL", result.stderr)
        self.assertNotIn("FALLBACK", result.stdout)


if __name__ == "__main__":
    unittest.main()
