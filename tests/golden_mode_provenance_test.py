#!/usr/bin/env python3
"""tools/golden_mode/provenance.py on a synthetic MIPS image, map, symbol lists and sources."""
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
import provenance  # noqa: E402

BASE = 0x00100000
END = 0x00764280


def image():
    data = bytearray(END - BASE)

    def put(address, blob):
        data[address - BASE:address - BASE + len(blob)] = blob

    code = [fx.lui(4, 0x0053), fx.itype(0x09, 4, 4, 0xDA10 - 0x10000),   # addiu a0, a0, %lo(0x0052DA10)
            fx.lui(5, 0x0053), fx.itype(0x23, 5, 2, 0xDA10 - 0x10000),   # lw v0, %lo(0x0052DA10)(a1)
            fx.itype(0x23, 28, 3, -0x5000),                             # lw v1, -0x5000(gp) = 0x007640F0
            0x03E00008, 0]
    put(0x00100000, b''.join(struct.pack('<I', w) for w in code))
    put(0x00100100, struct.pack('<I', fx.lui(6, 0x0053)) + struct.pack('<I', fx.itype(0x09, 6, 6, 0xDC00 - 0x10000)))
    put(0x0052DA10, bytes(range(1, 33)))
    put(0x0052DA40, struct.pack('<II', 0x00100000, 0x00100100))
    put(0x0052DA60, b'hello path/file.bin\0')
    put(0x0052DB00, b'\x07' * 16)
    put(0x0052DC00, b'\x09' * 16)
    put(0x007640F0, struct.pack('<I', 0x0052DA60))   # a pointer to the string
    return bytes(data)


MAP = """
 .data1         0x0052da00      0x100 /build/obj/data1_0.o
 .data
                0x0052db00       0x10 /build/obj/Main_g.c.o
 *fill*         0x0052db10       0x10
 .data1         0x0052db20   0x1deb30 /build/obj/data1_1.o
"""


class ProvenanceTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        root = Path(self.directory.name)
        self.elf = root / 'SLUS_217.82'
        self.elf.write_bytes(fx.mips_elf(BASE, image()))
        self.map = root / 'slus.map'
        self.map.write_text(MAP, encoding='utf-8')
        self.decomp = root / 'decomp'
        (self.decomp / 'config').mkdir(parents=True)
        (self.decomp / 'src').mkdir()
        (self.decomp / 'config' / 'symbol_addrs.txt').write_text(
            'datTest = 0x00100000; // type:func\nfldOther = 0x00100100; // type:func\n', encoding='utf-8')
        (self.decomp / 'config' / 'symbol_data_addrs.txt').write_text(
            'tblFuncs = 0x0052da40; // type:data evidence: test\n', encoding='utf-8')
        (self.decomp / 'src' / 'unit.c').write_text('extern char D_0052DA60[];\nint f(void) { return tblFuncs[0]; }\n',
                                                     encoding='utf-8')
        self.port = root / 'port'
        (self.port / 'config').mkdir(parents=True)
        (self.port / 'src').mkdir()
        (self.port / 'config' / 'game_tables.json').write_text(json.dumps({'tables': [
            {'key': 'fusion', 'gate_table': {'address': '0x0052dc00', 'stride': 8, 'rows': 2},
             'chart_aliases': [{'address': '0x0052dc04', 'role': 'inside', 'stride': 24},
                               {'address': '0x0052dd00', 'role': 'alone', 'stride': 24}]}]}), encoding='utf-8')
        self.out = root / 'out'

    def tearDown(self):
        self.directory.cleanup()

    def run_tool(self):
        self.assertEqual(provenance.main(['--elf', str(self.elf), '--map', str(self.map), '--decomp', str(self.decomp),
                                          '--port', str(self.port), '--out', str(self.out)]), 0)
        rows = [json.loads(line) for line in (self.out / 'provenance.jsonl').read_text(encoding='utf-8').splitlines()]
        return {r['address']: r for r in rows}, json.loads((self.out / 'provenance-summary.json').read_text())

    def test_objects_classes_and_evidence(self):
        rows, summary = self.run_tool()
        self.assertEqual(rows['0x0052da00']['class'], 'zero')
        unknown = rows['0x0052da10']
        self.assertEqual((unknown['class'], unknown['owner'], unknown['size']), ('mixed-unknown', 'incbin', 0x30))
        self.assertEqual(unknown['consumers'], ['datTest'])
        self.assertEqual(unknown['refs'], {'retail_code': 2})
        self.assertEqual(rows['0x0052da40']['class'], 'code-pointers')
        self.assertEqual(rows['0x0052da40']['names'], ['tblFuncs'])
        self.assertEqual(rows['0x0052da40']['refs'], {'decomp_c': 1})
        string = rows['0x0052da60']
        self.assertEqual(string['class'], 'strings')
        self.assertEqual(string['refs'], {'data_pointer': 1, 'decomp_c': 1})
        self.assertEqual((rows['0x0052db00']['class'], rows['0x0052db00']['object']), ('c-source', 'Main_g.c.o'))
        self.assertEqual(rows['0x0052db10']['owner'], 'fill')
        table = rows['0x0052dc00']
        self.assertEqual((table['class'], table['table'], table['size'], table['extent_basis']),
                         ('game-table', 'fusion.gate', 4, 'catalog'))
        self.assertEqual(table['consumers'], ['fldOther'])
        inside = rows['0x0052dc04']  # an index alias inside the gate extent stays an alias
        self.assertEqual((inside['table'], inside['table_aliases'], inside['size']),
                         ('fusion.gate', ['fusion.chart.inside'], 12))
        alone = rows['0x0052dd00']
        self.assertEqual((alone['table'], alone['extent_basis'], alone['class']),
                         ('fusion.chart.alone', 'catalog-start-next-boundary', 'zero'))
        self.assertIn('0x007640f0', rows)
        self.assertEqual(summary['data1']['covered'], summary['data1']['bytes'])
        self.assertEqual(summary['data2']['covered'], summary['data2']['bytes'])
        self.assertEqual(summary['data1']['mixed_unknown_bytes_by_consumer_module'], {'dat': 0x30})
        text = (self.out / 'provenance.jsonl').read_text(encoding='utf-8')
        self.assertNotIn('hello', text)

    def test_code_references_window_and_writeback(self):
        words = [fx.lui(4, 0x0053), fx.itype(0x09, 4, 4, 0x10),   # addiu a0, a0, lo: forms 0x00530010
                 fx.itype(0x23, 4, 2, 0x20),                      # lw off the full address: not a lui pair
                 fx.lui(5, 0x0060)] + [0] * provenance.LUI_WINDOW + [fx.itype(0x23, 5, 2, 0x4)]  # lui expired
        code = b''.join(struct.pack('<I', w) for w in words)
        read = lambda address, size: (code + bytes(size))[:size] if address == BASE else bytes(size)
        counts, consumers = provenance.code_references(read, provenance.GP, [BASE])
        self.assertEqual(dict(counts), {0x00530010: 1})
        self.assertEqual(consumers[0x00530010], {BASE})

    def test_register_writes_and_calls_end_a_lui(self):
        daddu_v0 = 2 << 11 | 0x2D                                   # daddu v0, zero, zero
        jal = 0x0C000000
        words = [fx.lui(2, 0x0053), daddu_v0, fx.itype(0x28, 2, 3, 0x0100),        # v0 rewritten: no address
                 fx.lui(4, 0x0054), jal, fx.itype(0x09, 4, 4, 0x0200),            # delay slot still forms it
                 fx.itype(0x23, 4, 2, 0x0300),                                   # a0 clobbered by the call
                 fx.lui(16, 0x0055), jal, 0, fx.itype(0x23, 16, 2, 0x0400)]      # s0 survives the call
        code = b''.join(struct.pack('<I', w) for w in words)
        read = lambda address, size: (code + bytes(size))[:size] if address == BASE else bytes(size)
        formed = [address for _, _, address in provenance.formed_addresses(read, provenance.GP)]
        self.assertEqual(formed, [0x00540200, 0x00550400])

    def test_refuses_output_inside_repository(self):
        with self.assertRaises(SystemExit):
            provenance.check_output(ROOT / 'build' / 'x')


if __name__ == '__main__':
    unittest.main()
