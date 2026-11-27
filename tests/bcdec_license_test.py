#!/usr/bin/env python3
"""Keep the selected BC7 license present in every package notice/copy list."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class BcdecLicenseTest(unittest.TestCase):
    def test_notice_references_resolve(self):
        license_path = ROOT / 'third_party/licenses/bcdec-MIT.txt'
        text = license_path.read_text(encoding='utf-8')
        self.assertIn('Copyright (c) 2022 Sergii Kudlai', text)
        self.assertIn('Permission is hereby granted', text)
        for notice in (ROOT / 'packaging').glob('NOTICES_*.txt'):
            with self.subTest(notice=notice.name):
                self.assertIn('licenses/bcdec-MIT.txt', notice.read_text(encoding='utf-8'))

    def test_packagers_copy_the_license(self):
        scripts = ('package_win.sh', 'package_win.sh', 'package_linux.sh',
                   'package_linux.sh', 'package_macos.sh', 'android_ci_run.sh')
        copy = re.compile(r'^cp [^\n]*third_party/licenses/(?:bcdec-MIT\.txt|\*\.txt)[^\n]*$', re.MULTILINE)
        for name in scripts:
            with self.subTest(script=name):
                self.assertRegex((ROOT / 'tools' / name).read_text(encoding='utf-8'), copy)


if __name__ == '__main__':
    unittest.main()
