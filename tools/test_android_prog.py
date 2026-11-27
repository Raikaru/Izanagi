import hashlib
import os
import tempfile
import unittest
from unittest import mock

import android_prog


class StagingTests(unittest.TestCase):
    def test_equal_length_different_content_is_replaced_and_verified(self):
        with tempfile.NamedTemporaryFile() as f:
            f.write(b"new content")
            f.flush()
            wanted = hashlib.sha256(b"new content").hexdigest()
            old = hashlib.sha256(b"old content").hexdigest()
            with mock.patch.object(android_prog, "device_digest", side_effect=[old, wanted]), \
                 mock.patch.object(android_prog, "shell"), mock.patch.object(android_prog, "adb") as adb:
                android_prog.push_if_changed(f.name, "/device/fixture")
                adb.assert_called_once_with("push", f.name, "/device/fixture", timeout=3600)

    def test_device_mismatch_after_push_fails_closed(self):
        with tempfile.NamedTemporaryFile() as f:
            f.write(b"data")
            f.flush()
            with mock.patch.object(android_prog, "device_digest", return_value=None), \
                 mock.patch.object(android_prog, "shell"), mock.patch.object(android_prog, "adb"):
                with self.assertRaises(RuntimeError):
                    android_prog.push_if_changed(f.name, "/device/fixture")

    def test_host_mutation_during_push_fails_closed(self):
        with tempfile.NamedTemporaryFile() as f:
            f.write(b"first")
            f.flush()
            def mutate(*args, **kwargs):
                with open(f.name, "wb") as changed:
                    changed.write(b"other")
            with mock.patch.object(android_prog, "device_digest", return_value=None), \
                 mock.patch.object(android_prog, "shell"), mock.patch.object(android_prog, "adb", side_effect=mutate):
                with self.assertRaises(RuntimeError):
                    android_prog.push_if_changed(f.name, "/device/fixture")

    def test_invalid_device_hash_is_not_accepted(self):
        with mock.patch.object(android_prog, "shell", return_value="not-a-digest  /device/fixture\n"):
            self.assertIsNone(android_prog.device_digest("/device/fixture"))

    def test_tree_stages_each_file_and_rejects_symlinks(self):
        with tempfile.TemporaryDirectory() as root:
            os.mkdir(os.path.join(root, "sub"))
            with open(os.path.join(root, "sub", "fixture"), "wb") as f:
                f.write(b"fixture")
            with mock.patch.object(android_prog, "shell"), \
                 mock.patch.object(android_prog, "push_if_changed") as push:
                android_prog.push_tree_verified(root, "/device/run")
                push.assert_called_once_with(os.path.join(root, "sub", "fixture"), "/device/run/sub/fixture")
                os.symlink(os.path.join(root, "sub", "fixture"), os.path.join(root, "link"))
                with self.assertRaises(ValueError):
                    android_prog.push_tree_verified(root, "/device/run")

    def test_installed_apk_must_match_host_digest(self):
        from types import SimpleNamespace
        with tempfile.NamedTemporaryFile() as apk:
            apk.write(b"debug apk")
            apk.flush()
            args = SimpleNamespace(apk=apk.name, aapt2="aapt2", iso=None, refs=None)
            with mock.patch.object(android_prog, "validate_debug_apk"), \
                 mock.patch.object(android_prog, "adb", return_value="Success"), \
                 mock.patch.object(android_prog, "shell", return_value="package:/data/app/base.apk"), \
                 mock.patch.object(android_prog, "device_digest", return_value="0" * 64):
                with self.assertRaises(RuntimeError):
                    android_prog.setup_locked(args)


class GateLockTests(unittest.TestCase):
    def test_lock_is_released_on_failure(self):
        with mock.patch.object(android_prog, "shell") as shell, \
             mock.patch.object(android_prog, "run_locked", side_effect=ValueError("failed")):
            with self.assertRaises(ValueError):
                android_prog.run(object())
            self.assertEqual(shell.call_count, 2)
            self.assertIn("mkdir", shell.call_args_list[0].args[0])
            self.assertIn("rmdir", shell.call_args_list[1].args[0])

    def test_lock_failure_does_not_start_gate(self):
        with mock.patch.object(android_prog, "shell", side_effect=SystemExit(1)), \
             mock.patch.object(android_prog, "run_locked") as run:
            with self.assertRaises(SystemExit):
                android_prog.run(object())
            run.assert_not_called()
if __name__ == "__main__":
    unittest.main()
