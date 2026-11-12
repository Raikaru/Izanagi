"""Native persisted recovery choices, not native-dialog/re-exec evidence."""
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


class RecoveryConfig(unittest.TestCase):
    @staticmethod
    def environment():
        environment = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0",
                           P4PC_INPUT="script", SDL_AUDIODRIVER="dummy")
        environment.pop("P4PC_MOD_WITNESS", None)
        return environment

    def ready(self, process, marker):
        seen = []
        for line in process.stdout:
            seen.append(line)
            if line.rstrip() == marker:
                return
        self.fail(f"fixture ended before {marker}: {''.join(seen)}")

    def invoke(self, mode, path, expected=0):
        environment = self.environment()
        result = subprocess.run([*DRIVER, mode, str(path)], env=environment,
                                capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result

    @staticmethod
    def seed(path, pending=0, crashes=0):
        path.write_text(
            "; player comment survives\n[mods]\nenabled=keep.first, broken.mod, keep.last\n"
            f"boot_pending={pending}\nboot_early_crashes={crashes}\n"
            "[packs]\nenabled=pack.keep, broken.pack\n[unrelated]\nkeep=untouched\n",
            encoding="utf-8")

    @staticmethod
    def settings(path):
        saved = configparser.ConfigParser(interpolation=None)
        saved.read(path, encoding="utf-8")
        return saved

    def test_exact_owner_safe_mode_and_unknown_quit_keep_unrelated_ini(self):
        for mode in ("restart", "quit", "pack", "absent", "safe", "safe-loop",
                     "unknown-quit", "loop-quit"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "fixture.ini"
                self.seed(path)
                self.invoke(mode, path)
                self.assertIn("; player comment survives", path.read_text(encoding="utf-8"))
                saved = self.settings(path)
                self.assertEqual(saved["unrelated"]["keep"], "untouched")
                if mode.startswith("safe"):
                    self.assertEqual(saved["mods"]["enabled"], "")
                    self.assertEqual(saved["packs"]["enabled"], "")
                    self.assertEqual(saved["mods"]["recovery_kind"], "safe-mode")
                    self.assertEqual(saved["mods"]["recovery_choice"], "restart")
                    self.assertEqual(saved["mods"]["recovery_notice"], "1")
                elif mode in ("unknown-quit", "loop-quit"):
                    self.assertEqual(saved["mods"]["enabled"], "keep.first, broken.mod, keep.last")
                    self.assertEqual(saved["packs"]["enabled"], "pack.keep, broken.pack")
                    self.assertEqual(saved["mods"]["recovery_kind"], "unattributed")
                    self.assertEqual(saved["mods"]["recovery_choice"], "quit")
                elif mode == "pack":
                    self.assertEqual(saved["packs"]["enabled"], "pack.keep")
                    self.assertEqual(saved["mods"]["enabled"], "keep.first, broken.mod, keep.last")
                else:
                    self.assertEqual(saved["packs"]["enabled"], "pack.keep, broken.pack")

    def test_two_real_process_deaths_offer_safe_mode_and_notice(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.ini"
            self.seed(path)
            for count in ("0", "1"):
                result = self.invoke("health-crash", path, 23)
                self.assertIn("mod_witness=true reference_eligible=false", result.stdout)
                saved = self.settings(path)
                self.assertEqual(saved["mods"]["boot_pending"], "1")
                self.assertEqual(saved["mods"]["boot_early_crashes"], count)
            self.invoke("health-loop", path)
            self.invoke("loop-quit", path)
            self.invoke("health-loop", path)  # Quit does not erase the threshold.
            self.invoke("safe-loop", path)
            saved = self.settings(path)
            self.assertEqual(saved["mods"]["enabled"], "")
            self.assertEqual(saved["packs"]["enabled"], "")
            self.assertEqual(saved["mods"]["recovery_cause"], "startup-crash-loop")
            self.assertEqual(saved["mods"]["boot_pending"], "0")
            self.assertEqual(saved["mods"]["boot_early_crashes"], "0")
            self.invoke("health-notice", path)
            self.assertEqual(self.settings(path)["mods"]["recovery_notice"], "0")

    def test_real_state_boundaries_clear_only_their_startup(self):
        for mode in ("health-stable", "health-normal", "health-empty", "health-disabled"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "fixture.ini"
                self.seed(path, pending=1)
                self.invoke(mode, path)
                saved = self.settings(path)
                self.assertEqual(saved["mods"]["boot_pending"], "0")
                self.assertEqual(saved["mods"]["boot_early_crashes"], "0")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.ini"
            self.seed(path)
            self.invoke("health-stable-crash", path, 23)
            self.invoke("health-crash", path, 23)
            self.assertEqual(self.settings(path)["mods"]["boot_early_crashes"], "0")

    def test_live_owner_is_not_a_crash_and_cached_metadata_is_refreshed(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.ini"
            self.seed(path)
            processes = []
            try:
                holder = subprocess.Popen(
                    [*DRIVER, "health-hold", str(path)], env=self.environment(),
                    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                processes.append(holder)
                self.ready(holder, "lease-held")
                before = path.read_bytes()
                self.invoke("health-busy", f"{path.parent}/./{path.name}")
                self.assertEqual(path.read_bytes(), before)
                self.invoke("health-off-busy", path)
                self.assertEqual(path.read_bytes(), before)
                cached = subprocess.Popen(
                    [*DRIVER, "health-cached", str(path)], env=self.environment(),
                    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                processes.append(cached)
                self.ready(cached, "cache-read")
                holder.stdin.write("\n")
                holder.stdin.flush()
                out, error = holder.communicate(timeout=15)
                self.assertEqual(holder.returncode, 0, out + error)
                cached.stdin.write("\n")
                cached.stdin.flush()
                out, error = cached.communicate(timeout=15)
                self.assertEqual(cached.returncode, 0, out + error)
                saved = self.settings(path)
                self.assertEqual(saved["mods"]["boot_pending"], "0")
                self.assertEqual(saved["mods"]["boot_early_crashes"], "0")
                self.assertEqual(saved["mods"]["enabled"], "keep.first, broken.mod, keep.last")
                self.assertEqual(saved["packs"]["enabled"], "pack.keep, broken.pack")
            finally:
                for process in processes:
                    if process.poll() is None:
                        process.kill()
                        process.communicate(timeout=15)
                    for stream in (process.stdin, process.stdout, process.stderr):
                        if stream is not None:
                            stream.close()

    def test_no_optional_request_creates_no_settings_artifact(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "not-created.ini"
            self.invoke("health-noop", path)
            self.assertFalse(path.exists())

    def test_disabled_ini_refuses_partial_recovery_and_optional_preparation(self):
        self.invoke("none", "none")
        self.invoke("health-write-refusal", "none")


if __name__ == "__main__":
    unittest.main()
