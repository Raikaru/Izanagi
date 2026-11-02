"""Real MC adapter/codec/filesystem with synthetic bytes; never retail references."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
import_separator = sys.argv.index("--import-end", separator + 1)
IMPORT_DRIVER = sys.argv[separator + 1:import_separator]
sys.argv = [sys.argv[0], *sys.argv[import_separator + 1:]]
SLOT = "BASLUS-21782Save0"


class NativeSaveTransport(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="p4-native-save-transport-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.card = self.root / "save-root"
        self.card.mkdir()
        self.package = self.root / "mods" / "dependency"
        (self.package / "files" / "DATA").mkdir(parents=True)
        (self.package / "files" / "DATA" / "CODEC.BIN").write_bytes(b"authored fixture data")
        (self.package / "mod.ini").write_text(
            "[mod]\nid=save.dep\nname=Authored save dependency\n"
            "version=1.2.3\napi=1.1\npriority=0\norder=0\n", encoding="utf-8")
        self.slot = self.card / "mc0" / SLOT
        self.raw = self.slot / SLOT

    @staticmethod
    def environment(witness=False):
        environment = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0",
                           P4PC_INPUT="script", P4PC_CONFIG="none",
                           SDL_AUDIODRIVER="dummy")
        environment.pop("P4PC_MOD_WITNESS", None)
        if witness:
            environment["P4PC_MOD_WITNESS"] = "1"
        return environment

    def run_case(self, mode, *, witness=False, expected=0, source=None):
        environment = self.environment(witness)
        result = subprocess.run([*DRIVER, mode, str(self.card), str(source or self.package.parent)],
                                env=environment, capture_output=True, text=True,
                                check=False, timeout=40)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        if expected == 0:
            self.assertIn("synthetic filesystem/codec complete", result.stdout)
        return result

    def run_import(self, source, destination, *, slot=3, witness=False, expected=0):
        result = subprocess.run([*IMPORT_DRIVER, "--import", str(source),
                                 str(destination), str(slot)],
                                env=self.environment(witness), capture_output=True,
                                text=True, check=False, timeout=40)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result


    def assert_no_journal(self):
        leftovers = [path.name for path in (self.card / "mc0").iterdir()
                     if path.name.startswith(".p4mod-")]
        self.assertEqual(leftovers, [])

    def test_plain_bytes_no_extra_save_metadata(self):
        self.run_case("plain")
        self.assertEqual(sorted(p.name for p in self.slot.iterdir()),
                         [SLOT, "icon.ico", "icon.sys"])
        self.assertEqual(len(self.raw.read_bytes()), 4100)
        self.assert_no_journal()

    def test_new_load_intent_cancels_owned_stage_not_an_unrelated_read(self):
        self.run_case("cancel-intent")
        self.assertEqual(self.raw.read_bytes()[100], 0)
        self.assertFalse((self.slot / ".p4mods").exists())
        self.assert_no_journal()

    def test_actual_cached_process_use_survives_new_game_without_test_custody(self):
        self.run_case("history-reset")
        data = (self.slot / ".p4mods").read_bytes()
        self.assertEqual(int.from_bytes(data[56:60], "little"), 0)
        self.assertEqual(int.from_bytes(data[64:68], "little"), 1)
        self.assertEqual(int.from_bytes(data[68:72], "little"), 0)
        self.assertFalse((self.slot / "p4pc-run-mode.json").exists())
        self.assert_no_journal()

    def test_stage_restore_exact_binding_defaults_and_stale_tag_strip(self):
        self.run_case("roundtrip")
        self.assertEqual(self.raw.read_bytes()[100], 7)
        self.assertFalse((self.slot / ".p4mods").exists())
        self.assertFalse((self.slot / ".p4mods-commit").exists())
        self.assert_no_journal()

    def test_unknown_required_block_refuses_without_mutating_generation(self):
        self.run_case("required-write")
        before = {p.name: p.read_bytes() for p in self.slot.iterdir()}
        self.run_case("required-read")
        self.assertEqual(before, {p.name: p.read_bytes() for p in self.slot.iterdir()})
        self.assert_no_journal()

    def test_player_mod_history_warning_decline_then_continue_and_preserve_opaque(self):
        self.run_case("optional-write")
        before = (self.slot / ".p4mods").read_bytes()
        self.assertEqual(int.from_bytes(before[68:72], "little"), 0,
                         "normal player's modded save must NOT acquire test custody")
        self.run_case("optional-decline")
        self.assertEqual(before, (self.slot / ".p4mods").read_bytes())
        self.run_case("optional-continue")
        self.assertEqual(before, (self.slot / ".p4mods").read_bytes())
        self.assertFalse((self.slot / "p4pc-run-mode.json").exists())
        self.assert_no_journal()

    def test_missing_commit_refuses_before_provider_decode(self):
        self.run_case("required-write")
        (self.slot / ".p4mods-commit").unlink()
        self.run_case("broken-read")

    def test_changed_sidecar_refuses_before_provider_decode(self):
        self.run_case("required-write")
        path = self.slot / ".p4mods"
        data = bytearray(path.read_bytes()); data[-1] ^= 1; path.write_bytes(data)
        self.run_case("broken-read")

    def test_changed_retained_raw_padding_still_breaks_full_image_binding(self):
        self.run_case("required-write")
        data = bytearray(self.raw.read_bytes()); data[100] ^= 1; self.raw.write_bytes(data)
        self.run_case("broken-read")

    def test_witness_only_save_is_intrinsic_and_cannot_enter_player_or_retail_run(self):
        self.run_case("witness", witness=True)
        sidecar = (self.slot / ".p4mods").read_bytes()
        self.assertEqual(int.from_bytes(sidecar[68:72], "little") & 1, 1)
        self.run_case("witness-refuse")
        # Even deleting only the companion marker cannot declassify the capsule.
        (self.slot / "p4pc-run-mode.json").unlink()
        self.run_case("witness-refuse")
        self.assert_no_journal()

    def test_actual_process_death_before_publication_keeps_original_generation(self):
        self.run_case("crash-pending", expected=23)
        self.assertEqual(self.raw.read_bytes()[100], 0)
        self.assertTrue(any(p.name.startswith(".p4mod-stage-")
                            for p in (self.card / "mc0").iterdir()))
        self.run_case("recovered")
        self.assertEqual(self.raw.read_bytes()[100], 0)
        self.assert_no_journal()

    def test_resolved_alias_retains_marked_ancestor_scope(self):
        source = self.root / "tainted" / "nested"
        source.mkdir(parents=True)
        data = source / "save.psu"
        data.write_bytes(b"custody-only; no container parsing or gameplay claim")
        (source.parent / "p4pc-run-mode.json").write_text(json.dumps(
            {"schema": 1, "run_mode": "mod-witness", "mod_witness": True,
             "reference_eligible": False}), encoding="utf-8")
        alias = self.root / "alias"
        if os.name == "nt":
            # Real unprivileged NT junction: no synthetic canonical path mock.
            result = subprocess.run(["cmd", "/d", "/c", "mklink", "/J",
                                     str(alias), str(source)], capture_output=True,
                                    text=True, check=False, timeout=20)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            alias.symlink_to(source, target_is_directory=True)
        self.run_case("custody-file", source=alias / data.name)
        self.run_case("custody-directory", source=alias)

    def test_portable_file_marker_and_invalid_marker_type(self):
        source = self.root / "source"
        source.mkdir()
        data = source / "save.psu"
        data.write_bytes(b"custody-only")
        self.run_case("custody-clean", source=data)
        marker = Path(str(data) + ".p4pc-mode.json")
        marker.write_text(json.dumps({"schema": 1, "run_mode": "mod-witness",
                                      "mod_witness": True, "reference_eligible": False}),
                          encoding="utf-8")
        self.run_case("custody-file", source=data)
        marker.unlink()
        (source / "p4pc-run-mode.json").mkdir()
        self.run_case("custody-bad", source=data)

    @unittest.skipIf(os.name == "nt", "POSIX execute-only directories")
    def test_custody_in_execute_only_ancestor_stays_fail_closed(self):
        locked = self.root / "execute-only"
        source = locked / "source"
        source.mkdir(parents=True)
        data = source / "save.psu"
        data.write_bytes(b"custody-only")
        locked.chmod(0o111)
        try:
            if os.access(locked, os.R_OK):
                self.skipTest("process can read execute-only directory")
            self.run_case("custody-clean", source=data)
            locked.chmod(0o700)
            marker = locked / "p4pc-run-mode.json"
            marker.write_text("witness")
            locked.chmod(0o111)
            self.run_case("custody-file", source=data)
            locked.chmod(0o700)
            marker.unlink()
            marker.symlink_to("missing-target")
            locked.chmod(0o111)
            self.run_case("custody-bad", source=data)
        finally:
            locked.chmod(0o700)


    def test_native_folder_pair_survives_slot_remap_without_retail_rewrite(self):
        self.run_case("required-write")
        destination = self.root / "import-card"
        self.run_import(self.slot, destination)
        target = destination / "BASLUS-21782Save3"
        self.assertEqual(self.raw.read_bytes(), (target / target.name).read_bytes())
        self.assertEqual((self.slot / ".p4mods").read_bytes(), (target / ".p4mods").read_bytes())
        self.assertEqual((self.slot / ".p4mods-commit").read_bytes(),
                         (target / ".p4mods-commit").read_bytes())
        self.assertFalse((target / "p4pc-run-mode.json").exists())

    def test_clean_native_import_replacement_strips_stale_pair(self):
        self.run_case("required-write")
        source = self.root / "clean" / SLOT
        source.mkdir(parents=True)
        for name in (SLOT, "icon.ico", "icon.sys"):
            (source / name).write_bytes((self.slot / name).read_bytes())
        self.run_import(source, self.slot.parent, slot=0)
        self.assertEqual(self.raw.read_bytes(), (source / SLOT).read_bytes())
        self.assertFalse((self.slot / ".p4mods").exists())
        self.assertFalse((self.slot / ".p4mods-commit").exists())
        self.assertFalse((self.slot / "p4pc-run-mode.json").exists())
        self.assert_no_journal()

    def test_incomplete_native_import_pair_refuses_before_destination_creation(self):
        self.run_case("required-write")
        (self.slot / ".p4mods-commit").unlink()
        destination = self.root / "refused-card"
        self.run_import(self.slot, destination, expected=1)
        self.assertFalse(destination.exists())

    def test_witness_import_retains_custody_outside_witness_process(self):
        self.run_case("witness", witness=True)
        destination = self.root / "witness-copy"
        result = self.run_import(self.slot, destination)
        self.assertIn("[mod-witness; NOT a retail reference]", result.stderr)
        target = destination / "BASLUS-21782Save3"
        self.assertTrue((target / "p4pc-run-mode.json").is_file())
        self.assertEqual(self.raw.read_bytes(), (target / target.name).read_bytes())
        self.assertEqual((self.slot / ".p4mods").read_bytes(), (target / ".p4mods").read_bytes())

    def test_witness_import_early_refusal_receipt_is_labelled(self):
        destination = self.root / "refused-witness-card"
        result = self.run_import(self.root / "missing.psu", destination,
                                 witness=True, expected=1)
        self.assertIn("[mod-witness; NOT a retail reference]", result.stderr)
        self.assertFalse(destination.exists())

    def test_new_clean_game_overwrites_old_generation_not_its_test_custody(self):
        self.run_case("witness", witness=True)
        # This is a fresh process/new synthetic game, NOT a witness save load.
        self.run_case("plain")
        self.assertFalse((self.slot / ".p4mods").exists())
        self.assertFalse((self.slot / ".p4mods-commit").exists())
        self.assertFalse((self.slot / "p4pc-run-mode.json").exists())
        self.assert_no_journal()

    def test_import_into_marked_destination_keeps_scope_and_labels_receipt(self):
        self.run_case("plain")
        destination = self.root / "marked-copy"
        destination.mkdir()
        (destination / "p4pc-run-mode.json").write_text(json.dumps(
            {"schema": 1, "run_mode": "mod-witness", "mod_witness": True,
             "reference_eligible": False}), encoding="utf-8")
        result = self.run_import(self.slot, destination)
        target = destination / "BASLUS-21782Save3"
        self.assertIn("[mod-witness; NOT a retail reference]", result.stderr)
        self.assertTrue((target / "p4pc-run-mode.json").is_file())
        self.assertFalse((target / ".p4mods").exists())
        self.assertEqual(self.raw.read_bytes(), (target / target.name).read_bytes())

if __name__ == "__main__":
    unittest.main()
