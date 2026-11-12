#!/usr/bin/env python3
"""tools/golden_mode/layouts.py on synthetic tables: containers, field inference, text, measurement, rendering."""
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
import layouts  # noqa: E402


def ftd(rows, row, pad=b''):
    payload = b''.join(rows)
    body = struct.pack('<IIII', len(payload), len(rows), 0, 0) + payload
    tail = layouts.align32(len(body)) - len(body)
    return body + (pad * tail)[:tail] if pad else body + bytes(tail)


def dat(rows):
    payload = b''.join(rows)
    body = struct.pack('<II', len(payload), len(rows)) + payload
    return body + bytes(layouts.align32(len(body)) - len(body))


class ContainerTest(unittest.TestCase):
    def test_ftd_padding_is_never_read(self):
        blob = ftd([b'\x01\x02\x03', b'\x04\x05\x06'], 3, pad=b'\xEE')
        self.assertEqual(layouts.ftd_table(blob), (2, b'\x01\x02\x03\x04\x05\x06'))
        self.assertIsNone(layouts.ftd_table(blob + bytes(32)))                     # wrong member size
        bad = bytearray(blob)
        bad[9] = 1                                                                  # reserved bytes
        self.assertIsNone(layouts.ftd_table(bytes(bad)))

    def test_dat_and_tbl_and_itemtbl(self):
        self.assertEqual(layouts.dat_table(dat([b'\x01\x00', b'\x02\x00'])), (2, b'\x01\x00\x02\x00'))
        self.assertIsNone(layouts.dat_table(b'\xFF\x00\x00\x00\x01\x00\x00\x00' + bytes(24)))
        spans = layouts.tbl_spans(fx.tbl([b'abc', b'defgh']))
        self.assertEqual([p for _, p in spans], [b'abc', b'defgh'])
        self.assertEqual([o for o, _ in spans], [4, 20])
        item = struct.pack('<IIII', 2, 1, 256, 1) + bytes(136) + struct.pack('<IIII', 2, 1, 256, 1) + bytes(48)
        self.assertEqual([(r, s, len(p)) for r, s, p in layouts.item_sections(item)], [(2, 68, 136), (2, 24, 48)])
        self.assertIsNone(layouts.item_sections(item + b'x'))
        self.assertIsNone(layouts.item_sections(struct.pack('<IIII', 1, 2, 256, 1) + bytes(68)))


class InferenceTest(unittest.TestCase):
    def test_widths_and_types(self):
        rows = []
        for i in range(40):
            rows.append(struct.pack('<HBBIfhH', 0x100 + i * 7, i % 3, 0, 70000 + i, 1.5 + i, -i, 0))
        fields = {f['offset']: f for f in layouts.infer_fields(b''.join(rows), 16)}
        self.assertEqual(fields[0]['type'], 'u16')       # a small high byte carrying a full-range low byte
        self.assertEqual(fields[2]['type'], 'u8')
        self.assertEqual(fields[3]['nonzero_rows'], 0)
        self.assertEqual(fields[4]['type'], 'u32')
        self.assertEqual(fields[8]['type'], 'f32')
        self.assertEqual(fields[12]['type'], 's16')
        self.assertEqual(fields[4]['bound'], 1 << 17)
        self.assertEqual(fields[2]['distinct'], '2-16')

    def test_small_neighbours_stay_bytes_and_text_tails(self):
        pairs = b''.join(bytes([i % 5, (i + 1) % 4]) for i in range(30))
        self.assertEqual([f['type'] for f in layouts.infer_fields(pairs, 2)], ['u8', 'u8'])
        rows = [struct.pack('<H', i) + name.ljust(10, b'\0') for i, name in
                enumerate([b'Agi', b'Bufu', b'Zio', b'Garu', b'', b'Hama'])]
        entry = layouts.describe('t', len(rows), b''.join(rows))
        self.assertEqual([(f['offset'], f['type']) for f in entry['fields']], [(0, 'u8'), (1, 'u8'), (2, 'text')])
        self.assertEqual(entry['fields'][-1]['width'], 10)
        numeric = b''.join(bytes([1, 2, 3, 0, 0, 0]) for _ in range(8))           # printable-free small bytes
        self.assertIsNone(layouts.text_start(numeric, 6))

    def test_render_fields(self):
        fields = [{'offset': 0, 'width': 2, 'type': 'u16', 'nonzero_rows': 3},
                  {'offset': 2, 'width': 1, 'type': 'u8', 'nonzero_rows': 0},
                  {'offset': 3, 'width': 1, 'type': 'u8', 'nonzero_rows': 0},
                  {'offset': 4, 'width': 12, 'type': 'text', 'nonzero_rows': None}]
        self.assertEqual(layouts.render_fields(fields), '0:u16 2-3:zero 4:char[12]')


class MeasureTest(unittest.TestCase):
    def test_measure_reads_every_family_and_renders(self):
        fcl = fx.pak_counted([('a.ftd', ftd([b'\x01\x00', b'\x02\x00'], 2)), ('note.bmd', b'\x07' * 40)], 32)
        cmp = fx.pak_counted([('b.ctd', ftd([b'\x05\x00\x00\x00'], 4))], 32)
        cmm = fx.pak_counted([('c.dat', dat([b'\x03\x00\x09\x00']))], 32)
        tbl = fx.tbl([b'\x01\x02', b'\x03\x04\x05\x06'])
        item = struct.pack('<IIII', 1, 1, 256, 1) + bytes(68) + struct.pack('<IIII', 1, 1, 256, 1) + bytes(24)
        objects = {'init_free.bin::init/fclTable.bin': fcl, 'init_free.bin::init/cmpTable.bin': cmp,
                   'init.bin::event/cmm.bin': cmm, 'init_free.bin::init/itemtbl.bin': item}
        for name in layouts.BATTLE:
            objects['init_free.bin::battle/%s.TBL' % name] = tbl
        read = lambda path: objects[path.split('::', 1)[1]]
        inventory = [{'archive': a, 'depth': 2, 'path': '%s::init_free.bin::init/fclTable.bin' % a, 'sha256': h}
                     for a, h in (('data.cpk', 'x'), ('data_e.cpk', 'y'), ('data_fr.cpk', 'y'))]
        result = layouts.measure(read, inventory)
        a = result['ftd']['tables'][0]
        self.assertEqual((a['name'], a['rows'], a['row_size']), ('a.ftd', 2, 2))
        self.assertEqual(result['ftd']['tables'][1]['container'], 'magic:07070707')
        self.assertEqual(result['ctd']['tables'][0]['row_size'], 4)
        self.assertEqual(result['dat']['tables'][0]['rows'], 1)
        self.assertEqual([s['bytes'] for s in result['tbl'][0]['spans']], [2, 4])
        self.assertEqual([(s['rows'], s['row_size']) for s in result['itemtbl']['sections']], [(1, 68), (1, 24)])
        self.assertEqual(layouts.locale_variance(inventory), {'init_free.bin::init/fcltable.bin': 2})
        text = json.dumps(result)
        self.assertNotIn('"payload"', text)
        markdown = layouts.render_family(result['ftd'])
        self.assertIn('| `a.ftd` | 2 | 2 |', markdown)
        self.assertIn('not this container', markdown)

    def test_refuses_output_inside_repository(self):
        with self.assertRaises(SystemExit):
            layouts.check_output(ROOT / 'docs')
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(layouts.check_output(directory), Path(directory).resolve())


if __name__ == '__main__':
    unittest.main()
