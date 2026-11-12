#!/usr/bin/env python3
"""The debug restart witness must accept adb PTY CRLF, without weakening checks."""
import contextlib
import io
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import android_restart_test as witness


class PhoneRestartWitnessTest(unittest.TestCase):
    def test_pty_marker_and_distinct_processes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            calls = []
            def shell(command, **kwargs):
                calls.append(command)
                if command.startswith("am start"):
                    return "Starting: Debug\n"
                if command.startswith("cat ") and command.endswith(".marker"):
                    return "requested abc pid=41\r\ndrained\r\nsecond\r\n"
                if "cat files/boot.log" in command:
                    return "p4pc restart test: PASS: old pid=41 new pid=42, fresh image, same arguments, drained exit\r\n"
                if command.startswith("cat ") and command.endswith(".inbox"):
                    return "inbox-preserved\r\n"
                return ""
            argv = ["android_restart_test.py", "--serial", "fixture", "--out", str(root / "evidence"),
                    "--lock", str(root / "phone.lock")]
            with mock.patch.object(witness.device, "PKG", "org.p4pc.game.debug"), \
                 mock.patch.object(witness.device, "BASE", "/sdcard/Android/media/org.p4pc.game.debug/files"), \
                 mock.patch.object(witness.device, "shell", side_effect=shell), \
                 mock.patch.object(sys, "argv", argv), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(witness.main(), 0)
            self.assertFalse((root / "phone.lock").exists())
            result = list((root / "evidence").glob("*/result.json"))
            self.assertEqual(len(result), 1)
            self.assertIn('"old_pid": 41', result[0].read_text())
            self.assertIn('"new_pid": 42', result[0].read_text())
            self.assertTrue(any("am force-stop org.p4pc.game.debug" in c for c in calls))
            self.assertFalse(any("am force-stop org.p4pc.game " in c for c in calls))


if __name__ == "__main__":
    unittest.main()
