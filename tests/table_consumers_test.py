#!/usr/bin/env python3
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('table_consumers', ROOT / 'tools/table_consumers.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
CATALOG = {'tables': [{'key': 'persona', 'symbols': ['personaBase'], 'constants': [256, 48], 'scope': ['persona']}]}


class ConsumerInventory(unittest.TestCase):
    def test_direct_and_narrow(self):
        text = '''// FUN_00109220
u8 *row(u16 id)
{
    return personaBase + (id & 0xffff) * 48;
}
void unrelated(void)
{
    int sameNumber = 256;
}
'''
        rows = audit.scan_text('arbitrary.c', text, CATALOG, 'mirror-0')
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]['function'], 'row')
        self.assertEqual(rows[0]['address'], '00109220')
        self.assertEqual({r['kind'] for r in rows[0]['reasons']},
                         {'direct-symbol', 'domain-constant-candidate', 'narrow-field-or-cast-candidate'})
        self.assertEqual(rows[0]['source'], 'mirror-0')
        self.assertEqual(rows[0]['status'], 'unreviewed')

    def test_comments_literals_not_code(self):
        text = '''// personaBase and 256 are not accesses
const char *text = "personaBase[256]";
/* personaBase */
void empty(void) { }
'''
        self.assertEqual(audit.scan_text('unrelated.c', text, CATALOG, 'decomp'), [])

    def test_asm_storage_candidates(self):
        text = '''INCLUDE_ASM("asm/nonmatchings/", func_00100000);
void snapshot(void)
{
    u16 records[256];
    memcpy(out, records, 48);
}
'''
        rows = audit.scan_text('persona.c', text, CATALOG, 'decomp')
        self.assertIn('untranslated-body', {r['kind'] for r in rows[0]['reasons']})
        self.assertIn('storage-or-copy-candidate', {r['kind'] for r in rows[1]['reasons']})
        self.assertEqual(rows[1]['function'], 'snapshot')

    def test_mirror_identity(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for directory in ('decomp/src', 'decomp/include', 'overrides', 'src', 'include', 'mirror'):
                (root / directory).mkdir(parents=True, exist_ok=True)
            body = 'void f(void)\n{\n personaBase[256] = 0;\n}\n'
            (root / 'decomp/src/persona.c').write_text(body)
            (root / 'mirror/persona.c').write_text(body)
            report = audit.inventory(root, CATALOG, [root / 'mirror'])
            self.assertTrue(report['compiled_mirror_scanned'])
            self.assertEqual(report['status'], 'unreviewed-candidates')
            self.assertEqual(report['summary'], {'persona': 2})
            self.assertEqual(report['inputs'][0]['sha256'], report['inputs'][1]['sha256'])
            self.assertNotEqual(report['inputs'][0]['source'], report['inputs'][1]['source'])

    def test_catalog(self):
        catalog = json.loads((ROOT / 'config/game_tables.json').read_text())
        tables = {t['key']: t for t in catalog['tables']}
        self.assertEqual(len(tables), len(catalog['tables']))
        self.assertEqual(tables['item']['pointer_slots'], ['0x007642a0'])
        self.assertEqual(tables['knowledge']['retail_id_limit'], 340)
        self.assertIsNone(tables['skill']['retail_id_limit'])
        self.assertEqual(tables['element_affinity']['retail_id_limit'], 19)


if __name__ == '__main__':
    unittest.main()
