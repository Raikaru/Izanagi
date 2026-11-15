#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import zipfile

spec = importlib.util.spec_from_file_location('guard', Path(__file__).resolve().parents[1] / 'tools/check_package_golden_cache.py')
guard = importlib.util.module_from_spec(spec)
spec.loader.exec_module(guard)


class GuardTest(unittest.TestCase):
    def test_tree_and_zip(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'normal').mkdir()
            (root / 'normal' / 'data').write_text('synthetic')
            guard.check(root)
            (root / 'Golden-Cache').mkdir()
            with self.assertRaises(ValueError):
                guard.check(root)
            with self.assertRaises(ValueError):
                guard.check(root / 'Golden-Cache')
            archive = root / 'fixture.zip'
            with zipfile.ZipFile(archive, 'w') as output:
                output.writestr('assets/Golden-Cache/key/manifest.json', '{}')
            with self.assertRaises(ValueError):
                guard.check(archive)

    def test_external_symlink_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            stage = root / 'stage'
            stage.mkdir()
            target = root / 'outside'
            target.mkdir()
            try:
                (stage / 'link').symlink_to(target, target_is_directory=True)
            except OSError as error:
                self.skipTest(f'symlinks unavailable: {error}')
            with self.assertRaises(ValueError):
                guard.check(stage)

    def test_unreadable_tree_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            with mock.patch.object(guard.os, 'walk', return_value=[]) as walk:
                guard.check(tmp)
                self.assertIs(walk.call_args.kwargs['onerror'], guard.walk_error)
            with self.assertRaises(PermissionError):
                guard.walk_error(PermissionError('denied'))


if __name__ == '__main__':
    unittest.main()
