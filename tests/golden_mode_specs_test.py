#!/usr/bin/env python3
"""docs/golden_mode/specs: required sections, index, and every synthetic test vector decoded by the readers."""
import json
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools' / 'golden_mode'))
import formats  # noqa: E402
import layouts  # noqa: E402

SPECS = ROOT / 'docs' / 'golden_mode' / 'specs'
SECTIONS = ('## Sources', '## Container', '## Records', '## ID domains', '## Invariants and refusals',
            '## PS2 differences', '## Test vectors', '## Status')
VECTOR = re.compile(r'^```golden-vector\n(.*?)^```', re.S | re.M)


def parse_hex(text):
    """Hex bytes with whitespace ignored and `XX*N` repeating byte XX N times."""
    out = bytearray()
    for token in text.split():
        if '*' in token:
            byte, count = token.split('*')
            out += bytes.fromhex(byte) * int(count)
        else:
            out += bytes.fromhex(token)
    return bytes(out)


def vectors(text):
    for block in VECTOR.findall(text):
        fields = dict(line.split(': ', 1) for line in block.strip().splitlines())
        yield fields['name'], fields['container'], parse_hex(fields['hex']), json.loads(fields['expect'])


def decode(container, blob):
    """What the spec's reader yields for a vector, in the vector's `expect` shape, or None when refused."""
    if container in ('ftd', 'dat'):
        parsed = (layouts.ftd_table if container == 'ftd' else layouts.dat_table)(blob)
        return parsed and {'rows': parsed[0], 'payload': parsed[1].hex()}
    if container == 'tbl':
        spans = layouts.tbl_spans(blob)
        return spans and {'spans': [payload.hex() for _, payload in spans]}
    if container == 'itemtbl':
        sections = layouts.item_sections(blob)
        return sections and {'sections': [[rows, size, payload.hex()] for rows, size, payload in sections]}
    if container == 'pak':
        variant, entries = formats.pak_entries(blob)
        return variant and {'variant': variant, 'entries': [[n, blob[o:o + s].hex()] for n, o, s in entries]}
    raise AssertionError('unknown container %s' % container)


def specs():
    return sorted(p for p in SPECS.glob('*.md') if p.name != 'README.md')


class SpecsTest(unittest.TestCase):
    def test_every_spec_has_the_required_sections(self):
        self.assertTrue(specs())
        for path in specs():
            text = path.read_text(encoding='utf-8')
            positions = [text.find('\n' + s + '\n') for s in SECTIONS]
            self.assertNotIn(-1, positions, '%s lacks a required section' % path.name)
            self.assertEqual(positions, sorted(positions), '%s sections out of order' % path.name)

    def test_index_lists_every_spec(self):
        index = (SPECS / 'README.md').read_text(encoding='utf-8')
        listed = set(re.findall(r'\]\(([a-z_]+\.md)\)', index))
        self.assertEqual(listed, {p.name for p in specs()})

    def test_every_vector_decodes_as_expected(self):
        names = set()
        count = 0
        for path in specs():
            text = path.read_text(encoding='utf-8')
            self.assertIn('```golden-vector', text, '%s has no test vector' % path.name)
            for name, container, blob, expect in vectors(text):
                self.assertNotIn(name, names, 'duplicate vector name %s' % name)
                names.add(name)
                count += 1
                with self.subTest(spec=path.name, vector=name):
                    self.assertEqual(decode(container, blob), expect)
        self.assertGreaterEqual(count, 10)

    def test_specs_carry_no_unlabelled_payload(self):
        # Catalogue rows hold layout facts only: no hex dumps outside test vectors.
        for path in specs():
            text = VECTOR.sub('', path.read_text(encoding='utf-8'))
            self.assertIsNone(re.search(r'\b[0-9a-f]{64,}\b', text), path.name)

    def test_hex_parser(self):
        self.assertEqual(parse_hex('0102 00*3 ff'), b'\x01\x02\x00\x00\x00\xff')


if __name__ == '__main__':
    unittest.main()
