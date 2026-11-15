#!/usr/bin/env python3
"""tools/golden_mode/typing_pass.py on a synthetic MIPS image, declarations, provenance rows and haystacks."""
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
import typing_pass  # noqa: E402

BASE = 0x00100000
END = 0x00764280
PARTIAL = bytes(range(100, 164))
FLOATS = struct.pack('<8f', 1.0, 2.5, -3.0, 0.125, 100.0, 7.75, -0.5, 9.0)
TABLE = bytes(range(1, 61))                                   # 60 distinct bytes: a lead candidate
STRUCTS = b''.join(struct.pack('<HBBI', 0x200 + i, 3, 0, 0x01000000 + i) for i in range(8))


def image():
    data = bytearray(END - BASE)

    def put(address, blob):
        data[address - BASE:address - BASE + len(blob)] = blob

    code = [fx.lui(4, 0x0053), fx.itype(0x31, 4, 0, 0xDA00 - 0x10000),   # lwc1 f0, %lo(0x0052DA00)(a0)
            fx.lui(5, 0x0053), fx.itype(0x21, 5, 2, 0xDA40 - 0x10000),   # lh v0, %lo(0x0052DA40)(a1)
            fx.lui(6, 0x0053), fx.itype(0x09, 6, 6, 0xDA80 - 0x10000),   # addiu a2, a2, %lo(0x0052DA80)
            0x03E00008, 0, 0, 0]
    put(0x00100000, b''.join(struct.pack('<I', w) for w in code))
    put(0x0052DA00, FLOATS)
    put(0x0052DA40, TABLE)
    put(0x0052DA80, STRUCTS)
    put(0x0052DB00, struct.pack('<4I', 0x00100000, 0x00100008, 0x00100010, 0x00100018))   # case labels
    put(0x0052DB40, struct.pack('<2I', 0x00100000, 0x00100000))                          # function pointers
    put(0x0052DB80, b'path/file.bin\0\0\0')
    put(0x0052DBC0, bytes([0x83, 0x41, 0x83, 0x43, 0x83, 0x45, 0x83, 0x47]))             # Shift-JIS
    put(0x0052DD00, PARTIAL)
    return bytes(data)


def obj(address, size, cls='mixed-unknown', **extra):
    row = {'address': '0x%08x' % address, 'size': size, 'range': 'data1', 'class': cls, 'refs': {'retail_code': 1},
           'names': []}
    row.update(extra)
    return row


OBJECTS = [obj(0x0052DA00, 32), obj(0x0052DA40, 60, consumers=['datTest']), obj(0x0052DA80, 64),
           obj(0x0052DB00, 16, 'code-pointers'), obj(0x0052DB40, 8, 'code-pointers'), obj(0x0052DB80, 16, 'strings'),
           obj(0x0052DBC0, 8), obj(0x0052DC00, 16, 'zero'), obj(0x0052DC10, 16, 'c-source'),
           obj(0x0052DC20, 32, names=['D_0052DC20'], refs={'decomp_c': 2}),
           obj(0x0052DC40, 8, table='fusion.gate'), obj(0x0052DD00, 64)]


def pe(sections):
    """A minimal PE32 with the given [(name, raw bytes)] sections, file-aligned at 0x200."""
    header = bytearray(0x400)
    header[0:2] = b'MZ'
    struct.pack_into('<I', header, 0x3C, 0x80)
    header[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HH', header, 0x84, 0x14C, len(sections))
    struct.pack_into('<H', header, 0x94, 0xE0)
    table, raw, offset = 0x80 + 24 + 0xE0, bytearray(), 0x400
    for i, (name, blob) in enumerate(sections):
        struct.pack_into('<8sIIII', header, table + 40 * i, name.encode(), len(blob), 0x1000 * (i + 1), len(blob),
                         offset + len(raw))
        raw += blob
    return bytes(header) + bytes(raw)


class TypingTest(unittest.TestCase):
    def setUp(self):
        self.image = image()
        self.read = lambda address, size: self.image[address - BASE:address - BASE + size]
        self.functions = {0x00100000: 'datTest'}

    def test_code_access_widths(self):
        accesses, users = typing_pass.code_accesses(self.read, provenance.GP, self.functions)
        self.assertEqual(dict(accesses[0x0052DA00]), {('loadf', 4): 1})
        self.assertEqual(dict(accesses[0x0052DA40]), {('load', 2): 1})
        self.assertEqual(dict(accesses[0x0052DA80]), {('address', 0): 1})
        self.assertEqual(users[0x0052DA40], {0x00100000})

    def test_code_range_reads(self):
        words = [fx.lui(4, 0x0010), fx.itype(0x23, 4, 2, 0x0100),     # lw v0, 0x00100100: reads code bytes
                 fx.lui(5, 0x0010), fx.itype(0x09, 5, 5, 0x0200),     # addiu: a function pointer, not a read
                 fx.itype(0x2B, 4, 3, 0x0104)]                        # sw v1, 0x00100104 (lui still live)
        code = b''.join(struct.pack('<I', w) for w in words)
        read = lambda address, size: (code + bytes(size))[:size] if address == BASE else bytes(size)
        reads = typing_pass.code_range_reads(read, provenance.GP, {BASE: 'f'})
        self.assertEqual(reads, [{'address': '0x00100100', 'access': {'load4': 1}, 'functions': ['0x00100000']},
                                 {'address': '0x00100104', 'access': {'store4': 1}, 'functions': ['0x00100000']}])

    def test_declarations(self):
        with tempfile.TemporaryDirectory() as directory:
            src = Path(directory) / 'a.c'
            src.write_text('extern s16 D_0052DC20[][4];\nextern u8 D_0052DA40[];\nextern f32 *gPtr;\n'
                           'static const u16 table[8] = {1};\n', encoding='utf-8')
            decls = typing_pass.parse_declarations([directory])
        self.assertEqual(decls['D_0052DC20'][:3], ('s16', 0, ['', '4']))
        self.assertFalse(decls['D_0052DA40'][3])                  # u8 x[] is no evidence
        self.assertEqual(decls['gPtr'][:2], ('f32', 1))
        self.assertEqual(decls['table'][:3], ('u16', 0, ['8']))
        self.assertIsNone(typing_pass.declared([], 0x0052DA40, decls))
        self.assertEqual(typing_pass.declared([], 0x0052DC20, decls)[0], 'D_0052DC20')

    def test_stride_guess(self):
        self.assertEqual(typing_pass.stride_guess(STRUCTS, 1)[0], 8)
        self.assertEqual(typing_pass.stride_guess(bytes(64), 1), (None, 0.0))
        # A mask without zeros cannot show a period beyond the element: the guess is the unit itself.
        self.assertEqual(typing_pass.stride_guess(bytes(range(1, 200)), 4)[0], 4)

    def test_kinds_and_suppliers(self):
        decls = {'D_0052DC20': ('s16', 0, ['', '4'], True)}
        exe = pe([('.sdata', b'\x11' * 32 + TABLE + b'\x22' * 32), ('.text', b'\x90' * 64)])
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'P4G.exe'
            path.write_bytes(exe)
            haystacks = typing_pass.pe_data_sections(path)
        self.assertEqual([label for label, _ in haystacks], ['P4G.exe.sdata'])
        archive = ('data_e.cpk::init_free.bin::x.ftd', bytes(16) + STRUCTS[:32] + bytes(16) + STRUCTS[40:] +
                   bytes(16) + PARTIAL[:20])
        rows = {r['address']: r for r in typing_pass.run(OBJECTS, self.read, self.functions, decls,
                                                          haystacks + [archive])}
        self.assertEqual((rows['0x0052da00']['kind'], rows['0x0052da00']['evidence']), ('f32-array', 'access'))
        table = rows['0x0052da40']
        self.assertEqual((table['kind'], table['element_width']), ('int16-array', 2))
        self.assertEqual(table['golden_lead']['match'], 'identical')
        self.assertEqual((table['supplier'], table['golden_lead']['offset']), ('golden-exe', 32))
        structs = rows['0x0052da80']
        self.assertEqual((structs['kind'], structs['stride'], structs['rows']), ('struct-array', 8, 8))
        self.assertEqual(structs['golden_lead']['match'], 'variant')
        self.assertEqual((structs['supplier'], structs.get('golden_check')), ('decomp-literal', True))
        self.assertEqual((rows['0x0052db00']['kind'], rows['0x0052db00']['supplier']), ('jump-table', 'symbols'))
        self.assertEqual(rows['0x0052db40']['kind'], 'function-pointer-table')
        self.assertEqual(rows['0x0052db80']['kind'], 'ascii-text')
        self.assertEqual(rows['0x0052dbc0']['kind'], 'sjis-text')
        self.assertEqual(rows['0x0052dc00']['supplier'], 'zero')
        self.assertEqual(rows['0x0052dc10']['supplier'], 'decomp-c')
        declared = rows['0x0052dc20']
        self.assertEqual((declared['kind'], declared['stride'], declared['evidence']), ('int16-array', 8, 'decl'))
        self.assertEqual(rows['0x0052dc40']['supplier'], 'golden-convert')
        partial = rows['0x0052dd00']
        self.assertEqual(partial['golden_lead']['match'], 'partial')
        self.assertNotIn('golden_check', partial)          # a partial lead is no evidence and nothing reads it
        summary = typing_pass.summarize(list(rows.values()))['data1']
        self.assertEqual(summary['code_referenced_without_supplier'], 0)
        self.assertEqual(summary['golden_leads'], {'identical': 1, 'variant': 1, 'partial': 1})
        self.assertEqual(summary['bytes_by_supplier']['golden-exe'], 60)
        text = json.dumps(list(rows.values()))
        self.assertNotIn(TABLE.hex()[:16], text)

    def test_archive_haystacks_take_small_table_objects_only(self):
        import inventory
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'install'
            root.mkdir()
            pack = fx.pak_v1([('init/a.ftd', TABLE), ('model/b.tmx', bytes(8) + b'TMX0' + bytes(20)),
                              ('event/c.pm1', TABLE)])
            (root / 'data_e.cpk').write_bytes(fx.cpk([('init_free.bin', pack, len(pack))]))
            (Path(directory) / 'inv').mkdir()
            (Path(directory) / 'inv' / 'inventory.jsonl').write_text(
                ''.join(json.dumps(r) + '\n' for r in inventory.inventory(root)), encoding='utf-8')
            stacks = typing_pass.archive_haystacks(Path(directory) / 'inv', root)
        self.assertEqual(stacks, [('data_e.cpk::init_free.bin::init/a.ftd', TABLE)])

    def test_main_writes_metadata_outside_the_repository(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'prov').mkdir()
            (root / 'prov' / 'provenance.jsonl').write_text(''.join(json.dumps(o) + '\n' for o in OBJECTS[:3]))
            (root / 'SLUS').write_bytes(fx.mips_elf(BASE, self.image))
            (root / 'decomp' / 'config').mkdir(parents=True)
            (root / 'decomp' / 'config' / 'symbol_addrs.txt').write_text('datTest = 0x00100000; // type:func\n')
            (root / 'port').mkdir()
            self.assertEqual(typing_pass.main(['--provenance', str(root / 'prov'), '--elf', str(root / 'SLUS'),
                                               '--decomp', str(root / 'decomp'), '--port', str(root / 'port'),
                                               '--out', str(root / 'out')]), 0)
            rows = [json.loads(line) for line in (root / 'out' / 'typed.jsonl').read_text().splitlines()]
            self.assertEqual([r['kind'] for r in rows], ['f32-array', 'int16-array', 'struct-array'])
            self.assertTrue((root / 'out' / 'typed-summary.json').is_file())
        with self.assertRaises(SystemExit):
            typing_pass.check_output(ROOT / 'build')


if __name__ == '__main__':
    unittest.main()
