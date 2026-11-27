#!/usr/bin/env python3
"""Synthetic BF and HCA metadata census fixtures."""
import json
import os
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools' / 'golden_mode'))
import census_scripts as census
import golden_mode_fixtures as fx


class CensusTest(unittest.TestCase):
    def test_bf_commands_and_immediate(self):
        data = bytearray(160)
        data[8:12] = b'FLW0'
        struct.pack_into('<I', data, 16, 5)
        for i, (width, count, offset) in enumerate(((32, 1, 112), (32, 0, 144),
                                                     (4, 4, 144), (1, 0, 160), (1, 0, 160))):
            struct.pack_into('<4I', data, 32 + i * 16, i, width, count, offset)
        struct.pack_into('<I', data, 112 + 24, 0)
        struct.pack_into('<Hh', data, 144, 0, 0)
        struct.pack_into('<I', data, 148, 0x00080008)  # not an instruction
        struct.pack_into('<Hh', data, 152, 8, 1)
        struct.pack_into('<Hh', data, 156, 8, 2)
        actual = census.script(data, [[True, True]])
        self.assertEqual(actual['procedures'], 1)
        self.assertEqual(actual['commands'], {1: 1, 2: 1})
        self.assertEqual(actual['procedure_inventory'][0]['commands'], {1: 1, 2: 1})
        self.assertEqual(actual['missing_commands'], {'2': 1})
        with self.assertRaises(ValueError):
            census.script(data[:-1], [[]])

    def test_hca_cipher(self):
        header = bytearray(b'HCA\0\x02\x00\x00\x20' + b'fmt\0' + bytes(12) + b'ciph\x00\x38' + bytes(2))
        self.assertEqual(census.hca(header)['status'], 'keyed')
        struct.pack_into('>H', header, 28, 0)
        self.assertEqual(census.hca(header)['status'], 'clear')
        with self.assertRaises(ValueError):
            census.hca(header[:-1])

    def test_amd_member_metadata(self):
        data = b'CHNK' + struct.pack('<I', 1) + b'MODEL_DATA'.ljust(16, b'\0')
        data += struct.pack('<II', 257, 8) + b'OMG.' + bytes(4)
        members = census.amd_members(data, 'synthetic.amd')
        self.assertEqual([(m['signature'], m['size']) for m in members], [('gmo', 8)])
        trailer = census.amd_members(data + b'\x01\x02', 'synthetic.amd')
        self.assertEqual((trailer[-1]['kind'], trailer[-1]['size']), ('amd-trailer', 2))
        with self.assertRaises(ValueError):
            census.amd_members(data[:-1], 'bad.amd')

    def test_embedded_acb_bank(self):
        audio = b'HCA\0\x02\x00\x00\x20' + b'fmt\0' + bytes(12) + b'ciph\x00\x38' + bytes(2)
        bank = b'AFS2' + bytes((1, 4)) + struct.pack('<HII', 2, 1, 16)
        bank += struct.pack('<HII', 7, 26, 32 + len(audio)) + bytes(6) + audio
        strings = b'AwbFile\0table\0'
        header = struct.pack('>HHIIIHHI', 1, 29, 37, 37 + len(strings), 8, 1, 8, 1)
        table = header + b'\x5b' + bytes(4) + struct.pack('>II', 0, len(bank)) + strings + bank
        packet = b'@UTF' + struct.pack('>I', len(table)) + table
        records = census.acb_members(packet, 'synthetic.acb')
        self.assertEqual([r['kind'] for r in records], ['acb-bank', 'hca'])
        self.assertEqual(records[1]['status'], 'keyed')

    def test_embedded_itoc(self):
        audio = b'HCA\0\x02\x00\x00\x20' + b'fmt\0' + bytes(12) + b'ciph\x00\x00' + bytes(2)
        nested = fx.utf_packet([('ID', 'u16', 'row'), ('FileSize', 'u32', 'row'),
                                ('ExtractSize', 'u32', 'row')],
                               [{'ID': 7, 'FileSize': len(audio), 'ExtractSize': len(audio)}])
        names = b'DataL\0FilesL\0FilesH\0table\0'
        schema = b'\x5b' + struct.pack('>I', 0) + b'\x54' + struct.pack('>I', 6) + b'\x54' + struct.pack('>I', 13)
        rows_off = 24 + len(schema)
        strings_off = rows_off + 16
        data_off = strings_off + len(names)
        table = struct.pack('>HHIIIHHI', 1, rows_off, strings_off, data_off, 20, 3, 16, 1)
        table += schema + struct.pack('>IIII', 0, len(nested), 1, 0) + names + nested
        itoc = fx.chunk(b'ITOC', b'@UTF' + struct.pack('>I', len(table)) + table)
        header = fx.chunk(b'CPK ', fx.utf_packet([('ItocOffset', 'u64', 'row'),
                                                   ('ContentOffset', 'u64', 'row'),
                                                   ('Align', 'u32', 'row')],
                                                  [{'ItocOffset': 0x800, 'ContentOffset': 0x1000, 'Align': 32}]))
        bank = header.ljust(0x800, b'\0') + itoc
        bank = bank.ljust(0x1000, b'\0') + audio
        self.assertEqual(list(census.itoc_entries(bank)), [('#7', 0x1000, len(audio), len(audio))])

    def test_duplicate_nested_names(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            amd = b'CHNK' + struct.pack('<I', 1) + b'MODEL_DATA'.ljust(16, b'\0')
            amd += struct.pack('<II', 257, 8) + b'OMG.' + bytes(4)
            pack = fx.pak_counted([('same.amd', amd), ('same.amd', amd)], 32)
            (root / 'data.cpk').write_bytes(fx.cpk([('pack.arc', pack, len(pack))]))
            rows = census.inventory.inventory(root)
            records, errors = census.scan(root, rows, [])
            self.assertFalse(errors)
            self.assertEqual(sum(r['kind'] == 'amd-member' for r in records), 2)

    def test_archive_identity_is_required(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            hca = b'HCA\0\x02\x00\x00\x20' + b'fmt\0' + bytes(12) + b'ciph\x00\x38' + bytes(2)
            archive = root / 'data.cpk'
            archive.write_bytes(fx.cpk([('sample.hca', hca, len(hca))]))
            rows = census.inventory.inventory(root)
            records, errors = census.scan(root, rows, [])
            self.assertFalse(errors)
            self.assertEqual(records[0]['status'], 'keyed')
            archive.write_bytes(archive.read_bytes() + b'x')
            self.assertTrue(census.scan(root, rows, [])[1])

    def test_steam_manifest_validation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'steamapps' / 'common' / 'Example'
            root.mkdir(parents=True)
            manifest = root.parent.parent / 'appmanifest_1113000.acf'
            manifest.write_text('"appid" "1113000" "installdir" "Example" "buildid" "10836033"', encoding='utf-8')
            self.assertEqual(census.verify_steam(root), root.resolve())
            with self.assertRaises(ValueError):
                census.verify_steam(Path(directory))


@unittest.skipUnless(all(os.environ.get(key) for key in
                         ('P4PC_GOLDEN_STEAM', 'P4PC_GOLDEN_INVENTORY', 'P4PC_RETAIL_ELF')),
                     'local Steam census inputs not configured')
class SteamCensusTest(unittest.TestCase):
    def test_inventory_is_measurable(self):
        install = census.verify_steam(os.environ['P4PC_GOLDEN_STEAM'])
        with open(os.environ['P4PC_GOLDEN_INVENTORY'], encoding='utf-8') as f:
            rows = [json.loads(line) for line in f]
        groups = census.elf_groups(os.environ['P4PC_RETAIL_ELF'])
        records, errors = census.scan(install, rows, groups)
        output = os.environ.get('P4PC_GOLDEN_CENSUS_OUT')
        if output:
            out = census.inventory.check_output(output, install)
            out.mkdir(parents=True, exist_ok=True)
            census.write_output(out, rows, records, errors)
        self.assertFalse(errors)
        self.assertTrue(any(record['kind'] == 'bf' for record in records))
        self.assertTrue(any(record['kind'] == 'hca' for record in records))


if __name__ == '__main__':
    unittest.main()
