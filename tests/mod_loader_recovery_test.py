"""Production registry/VM/native recovery, synthetic mods; no dialog-rendering claim."""
import configparser
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]
GOOD = "frame=p4.events.on('frame_start',function(e) end)\n"


class ManagedMods(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="p4-mod-loader-witness-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.mods = self.root / "mods"
        self.mods.mkdir()
        self.ini = self.root / "fixture.ini"
        self.ini.write_text("; unrelated player settings survive\n[mods]\nenabled=mod.good,mod.bad\n"
                            "[unrelated]\nkeep=untouched\n", encoding="utf-8")
        self.package("good", "mod.good", 0, GOOD)

    def package(self, folder, ident, priority, source, api="1.1", extra=""):
        root = self.mods / folder
        root.mkdir()
        (root / "mod.ini").write_text(
            f"[mod]\nid={ident}\nname=Authored {ident}\nversion=1.0\napi={api}\n"
            f"priority={priority}\norder=0\n{extra}", encoding="utf-8")
        (root / "main.luau").write_text(source, encoding="utf-8")
        (root / "files/DATA").mkdir(parents=True)
        (root / "files/DATA/TEST.TXT").write_text("good" if ident == "mod.good" else "bad", encoding="ascii")

    def invoke(self, mode, choice="quit"):
        env = dict(os.environ, P4PC_CONFIG=str(self.ini), P4PC_MODS="1", P4PC_PACKS="0",
                   P4PC_INPUT="script", P4PC_OFFSCREEN="1", SDL_AUDIODRIVER="dummy",
                   P4PC_MOD_WITNESS="1", P4PC_MOD_RECOVERY_CHOICE=choice)
        env.pop("P4PC_TEST_RESTART", None)
        result = subprocess.run([*DRIVER, mode, str(self.mods)], env=env,
                                capture_output=True, text=True, timeout=40, check=False)
        return result, result.stdout + result.stderr

    def saved(self):
        saved = configparser.ConfigParser(interpolation=None)
        saved.read(self.ini, encoding="utf-8")
        self.assertEqual(saved["unrelated"]["keep"], "untouched")
        self.assertIn("; unrelated player settings survive", self.ini.read_text(encoding="utf-8"))
        return saved

    def preflight(self, source, *, api="1.1", extra="", duplicate=False, bad_reads=1):
        self.package("bad", "mod.bad", 10, source, api, extra)
        if duplicate:
            self.package("shadow", "mod.bad", 20, source)
        result, output = self.invoke("--managed-preflight")
        self.assertEqual(result.returncode, 0, output)
        self.assertIn("Mod mod.bad was refused before any replacement files or code were exposed", output)
        self.assertIn(f"good_reads=1 bad_reads={bad_reads} mods=1 mask=0 raw=good", output)
        self.assertIn("managed pre-code refusal: PASS", output)
        self.assertIn("mod_witness=true reference_eligible=false", output)
        saved = self.saved()
        self.assertEqual(saved["mods"]["enabled"], "mod.good,mod.bad")
        self.assertNotIn("recovery_choice", saved["mods"])
        return output

    def test_compile_error_never_executes_any_chunk_or_exposes_bad_raw_files(self):
        self.preflight("local = syntax_error\n")

    def test_required_api_is_refused_before_script_read(self):
        output = self.preflight("error('MUST_NOT_RUN')", api="9.0", bad_reads=0)
        self.assertIn("Unsupported required API version", output)

    def test_only_bad_mod_is_refused_and_startup_continues_with_no_mods(self):
        self.ini.write_text("; unrelated player settings survive\n[mods]\nenabled=mod.bad\n"
                            "[unrelated]\nkeep=untouched\n", encoding="utf-8")
        self.package("bad", "mod.bad", 10, "local = syntax_error")
        result, output = self.invoke("--managed-empty")
        self.assertEqual(result.returncode, 0, output)
        self.assertIn("good_reads=0 bad_reads=1 mods=0 mask=0 raw=", output)
        self.assertIn("managed pre-code refusal: PASS", output)
        self.assertEqual(self.saved()["mods"]["enabled"], "mod.bad")

    def test_missing_native_capability_is_refused_before_script_read(self):
        output = self.preflight("error('MUST_NOT_RUN')", extra="[not_installed service]\nvalue=1\n", bad_reads=0)
        self.assertIn("unavailable native capability", output)

    def test_duplicate_id_never_selects_an_arbitrary_shadow_copy(self):
        output = self.preflight("error('MUST_NOT_RUN')", duplicate=True, bad_reads=0)
        self.assertIn("Duplicate installed packages", output)

    def runtime(self, source, diagnostic, mode="--managed-runtime", choice="quit"):
        self.package("bad", "mod.bad", 10, source)
        result, output = self.invoke(mode, choice)
        self.assertEqual(result.returncode, 0 if choice == "restart" else 1, output)
        self.assertIn("good_reads=1 bad_reads=1 mods=2 mask=0 raw=bad", output)
        self.assertIn("Mod mod.bad failed after replacement files/native work were exposed", output)
        self.assertIn(diagnostic, output)
        self.assertIn("The game must restart.", output)
        action = "restart-without-owner" if choice == "restart" else "quit"
        self.assertIn(f"mod_witness=true reference_eligible=false action={action}", output)
        self.assertNotIn("was refused before any replacement files", output)
        saved = self.saved()
        self.assertEqual(saved["mods"]["enabled"], "mod.good")
        self.assertEqual(saved["mods"]["recovery_mod"], "mod.bad")
        self.assertEqual(saved["mods"]["recovery_choice"], choice)
        return output

    def test_init_runtime_error_uses_controlled_native_quit_and_persists_disable(self):
        self.runtime("error('INIT_BAD')", "INIT_BAD")

    def test_runtime_type_error_is_not_in_game_static_analysis(self):
        self.runtime("local value:any=nil; value()", "attempt to call")

    def test_init_budget_exhaustion_uses_the_same_native_recovery_path(self):
        self.runtime("while true do end", "Interpreter safepoint budget exhausted")

    def test_event_callback_failure_does_not_free_live_subscription_handles(self):
        self.runtime("bad=p4.events.on('frame_start',function(e) error('CALLBACK_BAD') end)",
                     "CALLBACK_BAD", "--managed-callback")

    def test_callback_budget_exhaustion_does_not_fake_a_safe_unload(self):
        self.runtime("bad=p4.events.on('frame_start',function(e) while true do end end)",
                     "Interpreter safepoint budget exhausted", "--managed-callback")

    def test_hook_callback_failure_reaches_the_same_native_recovery_backend(self):
        self.runtime("bad=p4.hook(0x100100,'II','before',function(c) error('HOOK_BAD') end)",
                     "HOOK_BAD", "--managed-hook")

    def test_restart_choice_reenters_only_a_fresh_image_with_bad_mod_disabled(self):
        output = self.runtime("error('INIT_BAD')", "INIT_BAD", choice="restart")
        self.assertEqual(output.count("managed entry mod_witness=true"), 2, output)
        self.assertIn("good_reads=1 bad_reads=0 mods=1 mask=0 raw=good", output)
        self.assertIn("managed fresh-entry recovery: PASS", output)


if __name__ == "__main__":
    print("mod loader recovery witnesses: mod_witness=true reference_eligible=false", flush=True)
    unittest.main()
