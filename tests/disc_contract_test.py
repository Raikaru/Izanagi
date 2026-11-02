#!/usr/bin/env python3
"""Keep the PC-speed disc implementation bodies identical to accepted e2a429f1.

Only shared API declarations have moved into adxf_host.h. Runtime state words,
RNG, walk and scene behavior are checked separately with unchanged inputs.
"""
import hashlib
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
FILES = ROOT / "src/platform/files"
ACCEPTED = {
    "sce_adxf.c": "5126d18c5edb0be1ddd2b8aeca5a0a92b44eb5f981a2aab944d3d02732d503f1",
    "rofs_host.c": "2bbb6b5dc172ba82d7b10c7296c03b13ac15a93fb7c324204cd49436dc69b5e0",
    "adxf_partition.c": "e4bb8f3797fe84171a37649a8071cf57c713df4bb7c772fecd628ff89f3f6dea",
    "adxf_fsvr.c": "22dca4f75fa51226c408327fff112ff7618f245410bf8e6d11a22f2542e3f69d",
}
API_INCLUDE = '#include "adxf_host.h"\n'


class DiscContract(unittest.TestCase):
    def test_accepted_implementations_are_unchanged(self):
        for name, digest in ACCEPTED.items():
            with self.subTest(file=name):
                text = (FILES / name).read_text(encoding="utf-8")
                if name == "adxf_fsvr.c":
                    self.assertTrue(text.startswith(API_INCLUDE))
                    text = text[len(API_INCLUDE):]
                if name == "adxf_fsvr.c":
                    # Getter prototypes now come from the shared header with
                    # their actual pointer result; neither function body changed.
                    marker = 'extern int func_004e1a00(const char *name);\n'
                    self.assertEqual(text.count(marker), 1)
                    text = text.replace(marker, marker +
                                        'extern void func_004f8518(void);\n'
                                        'extern void func_004f8548(void);\n', 1)
                self.assertEqual(hashlib.sha256(text.encode()).hexdigest(), digest)

    def test_guest_callback_addresses_keep_original_symbols(self):
        text = (FILES / "adxf_fsvr.c").read_text(encoding="utf-8")
        self.assertIn('func_004e17a0(p4_rofs_name, (void *)func_004f8518, 0)', text)
        self.assertIn('func_004e17a0(name, (void *)func_004f8548, 0)', text)

    def test_host_handle_keeps_accepted_layout(self):
        text = (FILES / "adxf_host.h").read_text(encoding="utf-8")
        declaration = re.search(r"typedef struct \{(.*?)\} P4AdxfHandle;", text, re.S)
        self.assertIsNotNone(declaration)
        self.assertEqual(declaration.group(1).split(),
                         'signed char used; signed char stat; short pad; int fd; '
                         'long long base; long long size; long long pos; '
                         'int completion_alarm;'.split())


if __name__ == "__main__":
    unittest.main()
