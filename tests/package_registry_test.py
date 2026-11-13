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

    def test_bundled_root_adds_shipped_ids_and_player_copies_win(self):
        shipped = self.root / "shipped"
        shipped.mkdir()
        self.package(shipped, "high", "mod.high", 10, "mod", "bundled copy")
        self.package(shipped, "extra", "mod.shipped", 0, "mod", "shipped")
        result = subprocess.run([*DRIVER, "bundled", str(self.mods), str(shipped)],
                                capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("bundled root: PASS", result.stdout)
        self.assertIn("player copy shadows bundled mod.high 1.0", result.stderr)
        result = subprocess.run([*DRIVER, "bundled-only", str(self.mods), str(shipped)],
                                capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("bundled root only: PASS", result.stdout)

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

    # Requirements: synthetic manifests only, no payload files.
    def manifest(self, ident, extra="", version="1.2", api="1.6", priority=0, kind="mod"):
        folder = (self.mods if kind == "mod" else self.packs) / ident
        folder.mkdir()
        (folder / f"{kind}.ini").write_text(
            f"[{kind}]\nid={ident}\nname=Name {ident}\nversion={version}\napi={api}\n"
            f"priority={priority}\norder=0\n{extra}", encoding="ascii")

    def base_and_dependent(self, base_version="1.2", stored="on", requirement="version=1.0\nsetting.split=on\n"):
        self.manifest("mod.base", "[setting split]\nkind=toggle\nlabel=SPLIT\n", version=base_version, priority=5)
        self.manifest("mod.dep", f"[require mod.base]\n{requirement}", priority=-5)
        store = self.root / "settings/mod-settings"
        store.mkdir(parents=True, exist_ok=True)
        (store / "mod.base.ini").write_text(f"split = {stored}\n", encoding="ascii")

    def requirements(self, mods, order="", packs="", refused="", companion=""):
        result = subprocess.run(
            [*DRIVER, "requirements", str(self.mods), str(self.packs)], capture_output=True, text=True, check=False,
            env=dict(os.environ, P4PC_TEST_MODS=mods, P4PC_TEST_ORDER=order, P4PC_TEST_PACKS=packs,
                     P4PC_TEST_REFUSED=refused, P4PC_TEST_COMPANION=companion))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("requirements: PASS", result.stdout)
        return result.stdout

    def test_requirements_load_first_whatever_the_selector_or_manifest_order(self):
        self.base_and_dependent()
        out = self.requirements("mod.dep,mod.base")
        self.assertIn("admitted=mod.base,mod.dep\n", out)
        self.assertNotIn("refused=", out)
        out = self.requirements("mod.dep,mod.base", order="mod.dep,mod.base")
        self.assertIn("admitted=mod.base,mod.dep\n", out)

    def test_missing_old_incomparable_or_mismatched_requirement_refuses_only_the_dependent(self):
        cases = (
            ({}, "mod.dep", "Name mod.dep needs mod.base 1.0+, which is not enabled", "admitted=\n"),
            ({"base_version": "0.9"}, "mod.base,mod.dep", "Name mod.dep needs Name mod.base 1.0+ (enabled: 0.9)",
             "admitted=mod.base\n"),
            ({"base_version": "2.0"}, "mod.base,mod.dep", "Name mod.dep needs Name mod.base 1.0+ (enabled: 2.0)",
             "admitted=mod.base\n"),
            ({"base_version": "beta"}, "mod.base,mod.dep",
             "Name mod.dep needs Name mod.base 1.0+, but its version beta is not comparable", "admitted=mod.base\n"),
            ({"stored": "off"}, "mod.base,mod.dep", "Name mod.dep needs Name mod.base with SPLIT on",
             "admitted=mod.base\n"),
            ({"requirement": "version=1.0\nsetting.split=maybe\n"}, "mod.base,mod.dep",
             "Name mod.dep needs Name mod.base SPLIT maybe, which is not one of its choices", "admitted=mod.base\n"),
            ({"requirement": "version=1.0\nsetting.other=on\n"}, "mod.base,mod.dep",
             "Name mod.dep needs Name mod.base setting other, which it does not declare", "admitted=mod.base\n"),
        )
        for options, enabled, reason, admitted in cases:
            with self.subTest(reason=reason):
                self.remove_case()
                self.base_and_dependent(**options)
                out = self.requirements(enabled, refused="mod.dep")
                self.assertIn(f"refused=mod.dep reason={reason}\n", out)
                self.assertIn(f"refusal mod.dep={reason}\n", out)
                self.assertIn(admitted, out)

    def remove_case(self):
        for name in ("mod.base", "mod.dep"):
            for path in sorted((self.mods / name).glob("*"), reverse=True):
                path.unlink()
            if (self.mods / name).exists():
                (self.mods / name).rmdir()

    def test_refusal_cascades_and_cycles_refuse_their_members(self):
        self.manifest("mod.a", "[require mod.b]\nversion=1.0\n")
        self.manifest("mod.b", "[require mod.a]\nversion=1.0\n")
        self.manifest("mod.c", "[require mod.a]\nversion=1.0\n")
        self.manifest("mod.d", "")
        out = self.requirements("mod.a,mod.b,mod.c,mod.d")
        self.assertIn("refused=mod.a reason=Name mod.a is in a requirement cycle through Name mod.b\n", out)
        self.assertIn("refused=mod.b reason=Name mod.b is in a requirement cycle through Name mod.a\n", out)
        self.assertIn("refused=mod.c reason=Name mod.c needs mod.a 1.0+, which was refused: "
                      "Name mod.a is in a requirement cycle through Name mod.b\n", out)
        self.assertIn("admitted=mod.d\n", out)

    def test_invalid_requirement_sections_are_refused_with_their_line(self):
        cases = (
            ("1.2", "[require mod.d]\nversion=1.0\n", "Line9: [require] sections require api=1.6"),
            ("1.6", "[require mod.x]\nversion=1.0\n", "Line9: mod.x cannot require itself"),
            ("1.6", "[require mod.d]\nsetting.a=on\n", "Line9: requirement mod.d needs version=<major>.<minor>"),
            ("1.6", "[require mod.d]\nversion=1\n", "Line9: requirement mod.d version must be <major>.<minor>"),
            ("1.6", "[require mod.d]\nversion=1.0\nwhen=always\n", "Line10: requirement mod.d has unknown field when"),
            ("1.6", "[require Mod.D]\nversion=1.0\n", "Line9: invalid requirement id [require Mod.D]"),
        )
        self.manifest("mod.d", "")
        for api, extra, reason in cases:
            with self.subTest(reason=reason):
                folder = self.mods / "mod.x"
                if folder.exists():
                    (folder / "mod.ini").unlink()
                    folder.rmdir()
                self.manifest("mod.x", extra, api=api)
                out = self.requirements("mod.d,mod.x")
                self.assertIn(f"refused=mod.x reason={reason}\n", out)
                self.assertIn("admitted=mod.d\n", out)

    def test_requirements_stay_within_their_kind(self):
        self.manifest("shared.base", "")
        self.manifest("pack.dep", "[require shared.base]\nversion=1.0\n", kind="pack")
        out = self.requirements("shared.base", packs="pack.dep")
        self.assertIn("refused=pack.dep reason=Name pack.dep needs shared.base 1.0+, which is not enabled\n", out)
        self.assertIn("admitted=shared.base\n", out)

    def remove(self, *names):
        for name in names:
            folder = self.mods / name
            for path in folder.glob("*"):
                path.unlink()
            folder.rmdir()

    def test_companions_order_when_present_and_refuse_only_incompatible_versions(self):
        self.manifest("mod.hard", "", version="1.3", priority=5)
        self.manifest("mod.arts", "[companion mod.hard]\nversion=1.2\n", api="1.6", priority=-5)
        out = self.requirements("mod.arts", companion="mod.hard")
        self.assertIn("admitted=mod.arts\n", out)
        self.assertIn("companion mod.hard of mod.arts=0\n", out)
        self.assertNotIn("refused=", out)
        out = self.requirements("mod.arts,mod.hard", companion="mod.hard")
        self.assertIn("admitted=mod.hard,mod.arts\n", out)
        self.assertIn("companion mod.hard of mod.hard=-1\ncompanion mod.hard of mod.arts=1\n", out)
        self.assertNotIn("refused=", out)
        self.remove("mod.hard")
        self.manifest("mod.hard", "", version="1.1", priority=5)
        out = self.requirements("mod.arts,mod.hard", refused="mod.arts")
        reason = "Name mod.arts needs companion Name mod.hard 1.2+ (enabled: 1.1)"
        self.assertIn(f"refused=mod.arts reason={reason}\n", out)
        self.assertIn("admitted=mod.hard\n", out)
        # A companion refused for its own reasons is simply absent.
        self.remove("mod.hard")
        self.manifest("mod.hard", "[require mod.ghost]\nversion=1.0\n", version="1.3", priority=5)
        out = self.requirements("mod.arts,mod.hard")
        self.assertIn("refused=mod.hard reason=Name mod.hard needs mod.ghost 1.0+, which is not enabled\n", out)
        self.assertIn("admitted=mod.arts\n", out)
        self.assertNotIn("refused=mod.arts", out)

    def test_invalid_companion_sections_are_refused_with_their_line(self):
        cases = (
            ("1.5", "[companion mod.d]\nversion=1.0\n", "Line9: [companion] sections require api=1.6"),
            ("1.6", "[companion mod.x]\nversion=1.0\n", "Line9: mod.x cannot be its own companion"),
            ("1.6", "[companion mod.d]\nversion=1.0\nsetting.split=on\n",
             "Line10: companion mod.d has unknown field setting.split"),
            ("1.6", "[companion mod.d]\nversion=1.0\n[require mod.d]\nversion=1.0\n",
             "Line11: mod.d is both required and a companion"),
        )
        self.manifest("mod.d", "")
        for api, extra, reason in cases:
            with self.subTest(reason=reason):
                if (self.mods / "mod.x").exists():
                    self.remove("mod.x")
                self.manifest("mod.x", extra, api=api)
                out = self.requirements("mod.d,mod.x")
                self.assertIn(f"refused=mod.x reason={reason}\n", out)
                self.assertIn("admitted=mod.d\n", out)


if __name__ == "__main__":
    unittest.main()
