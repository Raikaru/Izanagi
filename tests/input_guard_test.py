#!/usr/bin/env python3
"""Automated game gates must fail closed before retail access or process launch."""
import os
from pathlib import Path
import shutil
import subprocess
import unittest


HERE = Path(__file__).resolve().parent
GUARD = HERE / "input_guard.sh"
# Sourced helpers and standalone coroutine comparisons do not launch an SDL game/viewer.
NON_GAME_SCRIPTS = {"input_guard.sh", "artifacts.sh", "port64_task.sh"}


class InputGuardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.shell = shutil.which("sh")
        if cls.shell is None:
            raise unittest.SkipTest("POSIX shell unavailable")

    def run_script(self, script, mode=None, args=()):
        env = {key: value for key, value in os.environ.items() if not key.startswith("P4PC_")}
        if mode is not None:
            env["P4PC_INPUT"] = mode
        return subprocess.run([self.shell, str(script), *args], env=env, capture_output=True, text=True)

    def test_explicit_script_mode_is_accepted(self):
        result = self.run_script(GUARD, "script")
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_missing_empty_and_other_modes_are_rejected(self):
        for mode in (None, "", "physical", "virtual"):
            with self.subTest(mode=mode):
                result = self.run_script(GUARD, mode)
                self.assertEqual(result.returncode, 1)
                self.assertIn("P4PC_INPUT=script is required", result.stderr)

    def test_environment_arguments_cannot_undo_isolation(self):
        result = self.run_script(GUARD, "script", ("P4PC_INPUT=physical",))
        self.assertEqual(result.returncode, 1)
        self.assertIn("cannot override P4PC_INPUT=script", result.stderr)
        result = self.run_script(GUARD, "script", ("P4PC_INPUT=script",))
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_every_game_gate_checks_before_arguments_or_retail_skip(self):
        scripts = sorted(path for path in HERE.glob("*.sh") if path.name not in NON_GAME_SCRIPTS)
        self.assertTrue(scripts)
        for script in scripts:
            for mode in (None, "physical"):
                with self.subTest(script=script.name, mode=mode):
                    result = self.run_script(script, mode, ("must-not-launch",))
                    self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                    self.assertIn("P4PC_INPUT=script is required", result.stderr)


if __name__ == "__main__":
    unittest.main()
