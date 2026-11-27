#!/usr/bin/env python3
"""Synthetic texture census descriptors; no game bytes."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools' / 'golden_mode'))
import texture_census as census


class TextureCensusTest(unittest.TestCase):
    def test_dx10_formats_are_distinct(self):
        data = bytearray(148)
        data[:4] = b'DDS '
        struct.pack_into('<8I', data, 76, 32, 4, int.from_bytes(b'DX10', 'little'), 0, 0, 0, 0, 0)
        struct.pack_into('<5I', data, 128, 71, 3, 0, 1, 0)
        first = census.describe(data)
        struct.pack_into('<I', data, 128, 98)
        second = census.describe(data)
        self.assertNotEqual(first, second)
        self.assertTrue(first.endswith(',dx10=71,3,0,1,0'))
        self.assertTrue(second.endswith(',dx10=98,3,0,1,0'))
        with self.assertRaisesRegex(ValueError, 'truncated DDS DX10'):
            census.describe(data[:147])

    def test_legacy_dds_remains_distinct(self):
        data = bytearray(128)
        data[:4] = b'DDS '
        struct.pack_into('<8I', data, 76, 32, 4, int.from_bytes(b'DXT1', 'little'), 0, 0, 0, 0, 0)
        self.assertNotIn('dx10=', census.describe(data))
        with self.assertRaisesRegex(ValueError, 'truncated DDS'):
            census.describe(data[:127])


if __name__ == '__main__':
    unittest.main()
