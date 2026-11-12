#!/usr/bin/env python3
"""tools/golden_mode/inventory.py on a synthetic install: recursion, hashes, signatures, no payloads."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools' / 'golden_mode'))
sys.path.insert(0, str(ROOT / 'tests'))
import golden_mode_fixtures as fx  # noqa: E402
import inventory  # noqa: E402

MARKER = b'PAYLOAD-MARKER-' * 4


def afs2(payloads, align=32):
    count = len(payloads)
    header = bytearray(b'AFS2' + bytes([1, 4]) + struct.pack('<HII', 2, count, align))
    header += b''.join(struct.pack('<H', i) for i in range(count))
    table_at = len(header)
    header += bytes(4 * (count + 1))
    offsets, cursor = [], table_at + 4 * (count + 1)
    body = bytearray(header)
    for payload in payloads:
        offsets.append(cursor)
        start = (cursor + align - 1) // align * align
        body = body.ljust(start, b'\0') + payload
        cursor = len(body)
    offsets.append(cursor)
    for i, value in enumerate(offsets):
        struct.pack_into('<I', body, table_at + 4 * i, value)
    return bytes(body)


def synthetic_install(root):
    inner_plain = bytes(256) + MARKER * 8
    nested_v2 = fx.pak_counted([('deep/x.tmx', bytes(8) + b'TMX0' + bytes(20)), ('deep/y.bin', MARKER)], 32)
    pack = fx.pak_v1([('battle/SKILL.TBL', fx.tbl([b'\x01' * 40, b'\x02' * 8])), ('inner.pac', nested_v2)])
    compressed_pack = fx.crilayla(pack.ljust(max(len(pack), 0x100), b'\0'))
    archive = fx.cpk([('init_free.bin', compressed_pack, max(len(pack), 0x100)),
                      ('sound/bank.awb', afs2([MARKER, b'abc']), len(afs2([MARKER, b'abc']))),
                      ('plain.bin', inner_plain, len(inner_plain))])
    (root / 'data_e.cpk').write_bytes(archive)
    (root / 'sub').mkdir()
    (root / 'sub' / 'loose.txt').write_bytes(b'not a game file\n')
    return pack


class InventoryTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name) / 'install'
        self.root.mkdir()
        self.pack = synthetic_install(self.root)

    def tearDown(self):
        self.directory.cleanup()

    def test_recursive_rows(self):
        for jobs in (1, 2):
            rows = inventory.inventory(self.root, jobs=jobs)
            paths = [r['path'] for r in rows]
            self.assertEqual(paths, [
                'data_e.cpk',
                'data_e.cpk::init_free.bin',
                'data_e.cpk::init_free.bin::battle/SKILL.TBL',
                'data_e.cpk::init_free.bin::inner.pac',
                'data_e.cpk::init_free.bin::inner.pac::deep/x.tmx',
                'data_e.cpk::init_free.bin::inner.pac::deep/y.bin',
                'data_e.cpk::sound/bank.awb',
                'data_e.cpk::sound/bank.awb::00000',
                'data_e.cpk::sound/bank.awb::00001',
                'data_e.cpk::plain.bin',
                'sub/loose.txt'])
            by = {r['path']: r for r in rows}
            top = by['data_e.cpk::init_free.bin']
            self.assertTrue(top['compressed'])
            self.assertEqual((top['kind'], top['children'], top['locale']), ('pak-v1', 2, 'e'))
            self.assertEqual(top['sha256'], hashlib.sha256(self.pack.ljust(max(len(self.pack), 0x100), b'\0')).hexdigest())
            self.assertIn('stored_sha256', top)
            self.assertEqual(by['data_e.cpk::init_free.bin::inner.pac']['kind'], 'pak-v2')
            self.assertEqual(by['data_e.cpk::init_free.bin::inner.pac::deep/x.tmx']['signature'], 'tmx')
            self.assertEqual(by['data_e.cpk::init_free.bin::inner.pac::deep/y.bin']['depth'], 3)
            self.assertEqual(by['data_e.cpk::sound/bank.awb']['kind'], 'cri-afs2')
            self.assertEqual(by['data_e.cpk::sound/bank.awb::00000']['sha256'], hashlib.sha256(MARKER).hexdigest())
            self.assertEqual(by['data_e.cpk']['children'], 3)
            self.assertEqual(by['sub/loose.txt']['signature'], 'text-ascii')
            self.assertFalse(any('error' in r for r in rows))

    def test_read_object_follows_the_chain(self):
        data = inventory.read_object(self.root, 'data_e.cpk::init_free.bin::inner.pac::deep/y.bin')
        self.assertEqual(data, MARKER)

    def test_outputs_hold_metadata_only(self):
        out = Path(self.directory.name) / 'out'
        self.assertEqual(inventory.main(['--install', str(self.root), '--out', str(out)]), 0)
        text = (out / 'inventory.jsonl').read_text(encoding='utf-8') + (out / 'summary.json').read_text(encoding='utf-8')
        self.assertNotIn('PAYLOAD-MARKER', text)
        self.assertNotIn('not a game file', text)
        summary = json.loads((out / 'summary.json').read_text(encoding='utf-8'))
        self.assertEqual(summary['objects'], 11)
        self.assertEqual(summary['archives']['data_e.cpk']['max_depth'], 3)
        parts = list((out / 'parts').iterdir())
        self.assertEqual(len(parts), 1)
        self.assertNotIn('PAYLOAD-MARKER', parts[0].read_text(encoding='utf-8'))

    def test_checkpoints_resume(self):
        parts = Path(self.directory.name) / 'parts'
        parts.mkdir()
        first = inventory.inventory(self.root, parts=parts)
        (checkpoint,) = parts.iterdir()
        rows = inventory.load_part(checkpoint)
        rows[0]['signature'] = 'from-checkpoint'
        inventory.save_part(checkpoint, rows)
        again = inventory.inventory(self.root, parts=parts)  # the batch is read back, not scanned
        self.assertEqual(len(again), len(first))
        self.assertEqual(again[1]['signature'], 'from-checkpoint')
        (self.root / 'data_e.cpk').write_bytes((self.root / 'data_e.cpk').read_bytes() + bytes(2048))
        fresh = inventory.inventory(self.root, parts=parts)  # a changed archive is a new identity
        self.assertEqual(fresh[1]['signature'], first[1]['signature'])
        self.assertEqual(len(list(parts.glob('*.jsonl'))), 2)

    def test_refuses_output_inside_repository_or_install(self):
        with self.assertRaises(SystemExit):
            inventory.check_output(ROOT / 'build' / 'golden', self.root)
        with self.assertRaises(SystemExit):
            inventory.check_output(self.root / 'out', self.root)

    def test_locale(self):
        self.assertEqual([inventory.locale_of(n) for n in ('data.cpk', 'data_ck.cpk', 'data_movie.cpk', 'sysdat.cpk')],
                         ['', 'ck', '', ''])


if __name__ == '__main__':
    unittest.main()
