"""Native package metadata/provider evidence; no retail bytes or production codecs."""
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


class NativePackages(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="p4-package-registry-")
        self.addCleanup(self.temporary.cleanup)
        self.root = pathlib.Path(self.temporary.name)
        self.mods = self.root / "mods"
        self.packs = self.root / "packs"
        self.mods.mkdir()
        self.packs.mkdir()
        self.package(self.mods, "high", "mod.high", 10, "mod", "high")
        self.package(self.packs, "low", "pack.low", -10, "pack", "low")

    def package(self, root, folder, ident, priority, kind, text, api="1.1"):
        package = root / folder
        package.mkdir()
        (package / f"{kind}.ini").write_text(
            f"[{kind}]\nid={ident}\nname=Authored {ident}\nversion=1.0\napi={api}\n"
            f"priority={priority}\norder=0\n[content]\nclass=mixed\n"
            "[texture authored-key]\nfile=Textures/p4tex-v1/A1.BIN\n",
            encoding="utf-8",
        )
        (package / "files/DATA").mkdir(parents=True)
        (package / "files/DATA/TEST.TXT").write_text(text, encoding="ascii")
        (package / "Textures/p4tex-v1").mkdir(parents=True)
        (package / "Textures/p4tex-v1/A1.BIN").write_bytes(b"image\0native")
        return package

    def run_mode(self, mode):
        result = subprocess.run(
            [*DRIVER, mode, str(self.mods), str(self.packs)],
            capture_output=True, text=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout, result.stderr

    def test_explicit_mod_order_keeps_pack_slots_and_default_unlisted_order(self):
        self.package(self.mods, "lowest", "mod.low", -20, "mod", "low")
        self.package(self.mods, "middle", "mod.mid", 0, "mod", "mid")
        for path in (*self.mods.glob("*/mod.ini"), *self.packs.glob("*/pack.ini")):
            path.write_text(path.read_text().split("[content]", 1)[0], encoding="utf-8")
        out, _ = self.run_mode("selector-order")
        self.assertIn("persisted selector order: PASS", out)

    def test_metadata_capability_refusal_removes_only_that_mod_and_its_raw_files(self):
        out, _ = self.run_mode("admit-capability")
        self.assertIn("refused=mod.high", out)
        self.assertIn("provider admission admit-capability: PASS", out)

    def test_bad_required_version_is_reported_without_blocking_valid_siblings(self):
        path = self.mods / "high/mod.ini"
        path.write_text(path.read_text().replace("api=1.1", "api=9.0"), encoding="utf-8")
        out, _ = self.run_mode("admit-version")
        self.assertIn("Unsupported required API version", out)

    def test_duplicate_installed_id_is_refused_instead_of_choosing_a_shadow_copy(self):
        self.package(self.mods, "duplicate", "mod.high", 20, "mod", "shadow")
        out, _ = self.run_mode("admit-duplicate")
        self.assertIn("Duplicate installed packages", out)

    def test_unknown_native_section_is_a_pre_code_capability_refusal(self):
        path = self.mods / "high/mod.ini"
        path.write_text(path.read_text() + "[not_installed feature]\nvalue=1\n", encoding="utf-8")
        out, _ = self.run_mode("admit-unknown")
        self.assertIn("unavailable native capability", out)

    def test_settings_are_mod_only_and_admitted_from_the_store(self):
        for path in (self.mods / "high/mod.ini", self.packs / "low/pack.ini"):
            header = path.read_text().split("[content]", 1)[0].replace("api=1.1", "api=1.2")
            path.write_text(header + "[setting flag]\nkind=toggle\nlabel=FLAG\napply=restart\n", encoding="utf-8")
        store = self.root / "settings/mod-settings"
        store.mkdir(parents=True)
        (store / "mod.high.ini").write_text("flag = on\n", encoding="ascii")
        out, _ = self.run_mode("settings-kinds")
        self.assertIn("refused=pack.low reason=[setting] sections are for mods, not engine packs", out)
        self.assertIn("setting kinds: PASS", out)

    def test_mandatory_prepare_io_refusal_renews_snapshot_and_keeps_old_pinned_work_immutable(self):
        out, _ = self.run_mode("admit-prepare")
        self.assertIn("Fixture format header is not P4HD", out)
        self.assertIn("provider admission admit-prepare: PASS", out)

    def test_disabled_installations_are_scanned_only_on_demand_after_empty_startup(self):
        # A payload executable must not be opened/activated by a metadata-only UI
        # listing. It WOULD be refused by real activation's complete index audit.
        (self.mods / "high/files/DATA/UNSAFE.EXE").write_bytes(b"MZnot-executed")
        bad = self.mods / "bad"
        bad.mkdir()
        (bad / "mod.ini").write_text("[mod]\nid=broken\napi=2.0\n", encoding="ascii")
        absent = self.packs / "no-manifest"
        absent.mkdir()
        out, _ = self.run_mode("catalog")
        self.assertIn("installed=mod.high api=1.1 fields=2", out)
        self.assertIn("installed=pack.low api=1.1 fields=2", out)
        self.assertIn("invalid=bad line=3", out)
        self.assertIn("Unsupported required API version", out)
        self.assertIn("invalid=no-manifest", out)
        self.assertIn("catalog=4 invalid=2 activation=0", out)

    def test_shared_start_accepts_ordinary_empty_both_selectors(self):
        out, _ = self.run_mode("start-mode")
        self.assertIn("ordinary harness start: PASS", out)

    def test_fidelity_guard_refuses_either_staged_selector_and_published_catalog(self):
        for mode, selector in (("guard-mod", "mods"), ("guard-pack", "packs"),
                               ("guard-published", "mods")):
            with self.subTest(mode=mode):
                result = subprocess.run(
                    [*DRIVER, mode, str(self.mods), str(self.packs)],
                    capture_output=True, text=True, check=False,
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("fidelity requires P4PC_MODS=0 and P4PC_PACKS=0", result.stderr)
                self.assertIn(f"{selector} selector still contains", result.stderr)

    def test_shared_metadata_reservation_policy_and_atomic_staging_refusal(self):
        out, _ = self.run_mode("sort")
        self.assertIn("allocation-free metadata sort: PASS", out)
        out, _ = self.run_mode("budget-policy")
        self.assertIn("metadata policy: PASS limit=4096 peak=4096", out)
        out, err = self.run_mode("budget-refuse")
        self.assertIn("metadata staging refusal: PASS", out)
        self.assertIn("package/native metadata budget exhausted", err)
        self.assertIn("limit=1", err)

    def test_source_tracking_is_explicit_and_open_through_script_prepare(self):
        out, err = self.run_mode("source-intent")
        self.assertIn("explicit source intent: PASS", out)
        self.assertIn("requested after activation/retirement", err)

    def test_shared_start_allows_tainted_scripted_witness_but_reference_guard_stays_strict(self):
        environment = dict(os.environ, P4PC_MOD_WITNESS="1", P4PC_INPUT="script",
                           P4PC_MODS="0", P4PC_PACKS="0")
        allowed = subprocess.run([*DRIVER, "witness-mode", str(self.mods), str(self.packs)],
                                 env=environment, capture_output=True, text=True, check=False)
        self.assertEqual(allowed.returncode, 0, allowed.stdout + allowed.stderr)
        self.assertIn("explicit mod witness boundary: PASS", allowed.stdout)
        refused = subprocess.run([*DRIVER, "guard-witness", str(self.mods), str(self.packs)],
                                 env=environment, capture_output=True, text=True, check=False)
        self.assertNotEqual(refused.returncode, 0)
        self.assertIn("fidelity harness refuses P4PC_MOD_WITNESS", refused.stderr)
        environment["P4PC_INPUT"] = "player"
        physical = subprocess.run([*DRIVER, "witness-mode", str(self.mods), str(self.packs)],
                                  env=environment, capture_output=True, text=True, check=False)
        self.assertNotEqual(physical.returncode, 0)
        self.assertIn("only1 with P4PC_INPUT=script", physical.stderr)
        environment["P4PC_INPUT"] = "script"
        for value in ("", "0", "true"):
            environment["P4PC_MOD_WITNESS"] = value
            for mode in ("witness-mode", "guard-invalid-witness-dump"):
                invalid = subprocess.run([*DRIVER, mode, str(self.mods), str(self.packs)],
                                         env=environment, capture_output=True, text=True, check=False)
                self.assertNotEqual(invalid.returncode, 0)
                self.assertIn("only1 with P4PC_INPUT=script", invalid.stderr)

    def test_issued_file_plan_reopens_after_retirement_without_fresh_provider_authority(self):
        out, _ = self.run_mode("plan-delayed")
        self.assertIn("issued file lineage plan-delayed: PASS observed=1", out)

    def test_file_plan_metadata_budget_refusal_does_not_leak_or_change_the_open_file(self):
        out, _ = self.run_mode("plan-budget")
        self.assertIn("issued file lineage plan-budget: PASS observed=1", out)

    def test_issued_file_plan_refuses_same_sized_replacement_object(self):
        out, _ = self.run_mode("plan-replaced")
        self.assertIn("issued file lineage plan-replaced: PASS observed=0", out)

    def test_issued_file_plan_refuses_changed_revision_even_below_original_bound(self):
        out, _ = self.run_mode("plan-grown")
        self.assertIn("issued file lineage plan-grown: PASS observed=0", out)

    def test_global_order_raw_conflicts_and_old_generation_read_handles(self):
        out, err = self.run_mode("order")
        self.assertIn("PASS mode=order prepare=1 publish=1 rollback=0 observed=2", out)
        self.assertIn("pack.low < mod.high (winner)", err)
        self.assertLess(err.index("packs: enabled pack.low"), err.index("mods: enabled mod.high"))

    def test_prepare_failure_publishes_nothing_and_keeps_issued_reader_identity(self):
        out, _ = self.run_mode("prepare-fail")
        self.assertIn("PASS mode=prepare-fail prepare=1 publish=0 rollback=1 observed=2", out)

    def test_cosmetic_requirement_uses_actual_claims_not_kind_or_label(self):
        out, _ = self.run_mode("cosmetic")
        self.assertIn("PASS mode=cosmetic prepare=1 publish=1 rollback=0 observed=2", out)
        out, err = self.run_mode("cosmetic-mixed")
        self.assertIn("PASS mode=cosmetic-mixed prepare=1 publish=0 rollback=1 observed=2", out)
        self.assertIn("Cosmetic-only declaration conflicts with actual gameplay claims", err)

    def test_catalog_diagnoses_duplicate_extension_field_without_preparing(self):
        bad = self.packs / "duplicate-field"
        bad.mkdir()
        (bad / "pack.ini").write_text(
            "[pack]\nid=duplicate\nname=Duplicate\nversion=1\napi=1.1\npriority=0\norder=0\n"
            "[texture x]\nfile=one.bin\nfile=two.bin\n", encoding="ascii",
        )
        broken = self.mods / "bad"
        broken.mkdir()
        (broken / "mod.ini").write_text("[mod]\nid=bad\napi=1.2\n", encoding="ascii")
        out, _ = self.run_mode("catalog")
        self.assertIn("invalid=duplicate-field line=10 reason=Duplicate extension section/argument/key", out)
        self.assertIn("activation=0", out)
        # Equal-key heapsort entries must retain source order for diagnostics,
        # including groups larger than the first repeated pair.
        path = bad / "pack.ini"
        path.write_text(path.read_text(encoding="ascii") + "file=three.bin\n", encoding="ascii")
        out, _ = self.run_mode("catalog")
        self.assertIn("invalid=duplicate-field line=10 reason=Duplicate extension section/argument/key", out)
        self.assertIn("activation=0", out)


    def test_disabled_legacy_catalog_uses_same_compendium_metadata(self):
        legacy = self.mods / "legacy"
        (legacy / "BGM").mkdir(parents=True)
        (legacy / "BGM/04.ADX").write_bytes(b"authored-fixture-not-a-retail-stream")
        bad = self.mods / "bad"
        bad.mkdir()
        (bad / "mod.ini").write_text("[mod]\nid=bad\napi=2.0\n", encoding="ascii")
        (self.packs / "no-manifest").mkdir()
        out, _ = self.run_mode("catalog")
        self.assertIn("installed=compendium.", out)
        self.assertIn("api=1.0 fields=0", out)
        self.assertIn("catalog=5 invalid=2 activation=0", out)


    def test_empty_extension_is_diagnosed_not_silently_ignored(self):
        empty = self.mods / "empty"
        empty.mkdir()
        (empty / "mod.ini").write_text(
            "[mod]\nid=empty\nname=Empty\nversion=1\napi=1.1\npriority=0\norder=0\n"
            "[texture missing-fields]\n", encoding="ascii",
        )
        (self.packs / "no-manifest").mkdir()
        out, _ = self.run_mode("catalog")
        self.assertIn("invalid=empty", out)
        self.assertIn("Empty extension section refused", out)
        self.assertIn("catalog=4 invalid=2 activation=0", out)


if __name__ == "__main__":
    unittest.main()
