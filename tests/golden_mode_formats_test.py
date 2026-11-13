#!/usr/bin/env python3
"""tools/golden_mode/formats.py on synthetic fixtures: @UTF, CPK, CRILAYLA, PAK families, signatures."""
from pathlib import Path
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools' / 'golden_mode'))
sys.path.insert(0, str(ROOT / 'tests'))
import formats  # noqa: E402
import golden_mode_fixtures as fx  # noqa: E402


class UtfTest(unittest.TestCase):
    COLUMNS = [('Name', 'str', 'row'), ('Size', 'u32', 'row'), ('Big', 'u64', 'row'), ('Kind', 'u16', ('const', 7)),
               ('Label', 'str', ('const', 'fixed'))]
    ROWS = [{'Name': 'a.bin', 'Size': 5, 'Big': 1 << 40}, {'Name': 'dir/b.tbl', 'Size': 0, 'Big': 3}]

    def test_rows_constants_and_strings(self):
        rows = formats.utf_table(fx.utf_packet(self.COLUMNS, self.ROWS))
        self.assertEqual(rows, [{'Name': 'a.bin', 'Size': 5, 'Big': 1 << 40, 'Kind': 7, 'Label': 'fixed'},
                                {'Name': 'dir/b.tbl', 'Size': 0, 'Big': 3, 'Kind': 7, 'Label': 'fixed'}])

    def test_masked_packet_is_unmasked(self):
        packet = fx.utf_packet(self.COLUMNS, self.ROWS, masked=True)
        self.assertNotEqual(packet[:4], b'@UTF')
        self.assertEqual(formats.utf_table(packet)[1]['Name'], 'dir/b.tbl')

    def test_malformed_packets_refuse(self):
        packet = bytearray(fx.utf_packet(self.COLUMNS, self.ROWS))
        with self.assertRaises(formats.FormatError):
            formats.utf_table(b'NOPE' + bytes(64))
        truncated = bytes(packet[:4]) + struct.pack('>I', len(packet)) + bytes(packet[8:])
        with self.assertRaises(formats.FormatError):
            formats.utf_table(truncated)
        overrun = bytearray(packet)
        struct.pack_into('>I', overrun, 8 + 20, 1000)  # row count past the strings
        with self.assertRaises(formats.FormatError):
            formats.utf_table(bytes(overrun))


class CrilaylaTest(unittest.TestCase):
    def test_literals_and_back_references_round_trip(self):
        plain = bytes(range(256)) + b'ABCDABCDABCD' * 40 + bytes(range(7)) + b'\xff' * 600
        stream = fx.crilayla(plain)
        self.assertLess(len(stream), len(plain))
        self.assertEqual(formats.crilayla_size(stream), len(plain))
        self.assertEqual(formats.crilayla(stream), plain)

    def test_prefix_only_stream(self):
        plain = bytes(range(256))
        self.assertEqual(formats.crilayla(fx.crilayla(plain)), plain)

    def test_corrupt_streams_refuse(self):
        stream = bytearray(fx.crilayla(bytes(256) + bytes(range(200)) * 3))
        self.assertEqual(formats.crilayla_size(b'CRILAYLA' + bytes(8)), 0)
        with self.assertRaises(formats.FormatError):
            formats.crilayla(b'notlayla' + bytes(0x200))
        struct.pack_into('<I', stream, 8, 1 << 20)  # body larger than the bit stream can supply
        with self.assertRaises(formats.FormatError):
            formats.crilayla(bytes(stream))


class CpkTest(unittest.TestCase):
    def test_members_with_plain_compressed_and_masked_toc(self):
        plain = bytes(range(256)) + b'xyz' * 300
        packed = fx.crilayla(plain)
        archive = fx.cpk([('top.bin', b'hello', 5), ('dir/sub/c.tbl', packed, len(plain))], masked_toc=True)
        entries = formats.cpk_entries(formats.bytes_reader(archive))
        self.assertEqual([(p, s, e) for p, _, s, e in entries],
                         [('top.bin', 5, 5), ('dir/sub/c.tbl', len(packed), len(plain))])
        path, offset, stored, extracted = entries[1]
        self.assertEqual(formats.crilayla(archive[offset:offset + stored]), plain)
        self.assertEqual(archive[entries[0][1]:entries[0][1] + 5], b'hello')

    def test_not_a_cpk(self):
        with self.assertRaises(formats.FormatError):
            formats.cpk_entries(formats.bytes_reader(b'ZIP ' + bytes(64)))


class PakTest(unittest.TestCase):
    ENTRIES = [('a/one.tbl', b'1' * 70), ('two.bmd', b'22'), ('three', b'')]

    def test_v1_with_64_and_128_alignment(self):
        for align in (64, 128):
            variant, entries = formats.pak_entries(fx.pak_v1(self.ENTRIES, align))
            self.assertEqual(variant, 'pak-v1')
            self.assertEqual([(n, s) for n, _, s in entries], [('a/one.tbl', 70), ('two.bmd', 2), ('three', 0)])

    def test_counted_v2_and_v3(self):
        for width, label in ((32, 'pak-v2'), (24, 'pak-v3')):
            data = fx.pak_counted(self.ENTRIES, width)
            variant, entries = formats.pak_entries(data)
            self.assertEqual(variant, label)
            name, offset, size = entries[0]
            self.assertEqual((name, data[offset:offset + size]), ('a/one.tbl', b'1' * 70))

    def test_non_packs_are_not_detected(self):
        # A texture whose header reads "DDS |" + NUL with nonzero header fields after it (measured on
        # Golden's model/hrestex/dummy/*.dds) must not become a one-entry pack.
        dds_like = (b'DDS |\0\0\0\x07\x10\x08\0').ljust(252, b'\x01') + struct.pack('<I', 300) + b'\x02' * 300
        for data in (b'', b'DDS ' + bytes(300), bytes(512), b'\x05\0\0\0' + b'\x01' * 100,
                     fx.pak_v1(self.ENTRIES) + b'trailing', dds_like.ljust(576, b'\0')):
            self.assertEqual(formats.pak_entries(data), (None, None))
        self.assertIsNotNone(formats.pak_v1(b'ok'.ljust(252, b'\0') + struct.pack('<I', 300) + b'\x02' * 300 +
                                            bytes(20)))


class SignatureTest(unittest.TestCase):
    def test_labels_and_magic(self):
        self.assertEqual(formats.signature(b'CPK ' + bytes(12)), 'cri-cpk')
        self.assertEqual(formats.signature(b'\0' * 8 + b'TMX0' + bytes(4)), 'tmx')
        self.assertEqual(formats.signature(b'\x01\0\0\0\0\0\0\0MSG1\0\0\0\0'), 'message-bmd')
        self.assertEqual(formats.signature(bytes(16)), 'zero-prefix')
        self.assertEqual(formats.signature(b''), 'empty')
        self.assertEqual(formats.signature(b'plain text here!'), 'text-ascii')
        self.assertEqual(formats.signature(b'\x12\x34\x56\x78\x9a' + bytes(11)), 'magic:12345678')
        self.assertEqual(formats.magic_hex(b'\x12\x34\x56\x78\x9a'), '12345678')


if __name__ == '__main__':
    unittest.main()
