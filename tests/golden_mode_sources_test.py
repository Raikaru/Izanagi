#!/usr/bin/env python3
"""tools/golden_mode/sources.py on synthetic inventories, tables, banks and disc listings."""
import hashlib
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools' / 'golden_mode'))
sys.path.insert(0, str(ROOT / 'tests'))
import golden_mode_fixtures as fx  # noqa: E402
import sources  # noqa: E402


def row(path, locale='', size=4, signature='magic:00000000', sha256='0' * 64):
    return {'path': path, 'depth': path.count('::'), 'locale': locale, 'size': size, 'signature': signature,
            'sha256': sha256}


GOLDEN_SKILL = fx.tbl([b'\x01' * 1152, b'\x02' * 440 * 44])
PS2_SKILL = fx.tbl([b'\x01' * 1152, b'\x02' * 440 * 40])
INVENTORY = [
    row('data.cpk'),
    row('data_e.cpk::init_free.bin', 'e'),
    row('data_e.cpk::init_free.bin::battle/SKILL.TBL', 'e', len(GOLDEN_SKILL)),
    row('data_fr.cpk::init_free.bin::battle/SKILL.TBL', 'fr', len(GOLDEN_SKILL)),
    row('data.cpk::init_free.bin::battle/SKILL.TBL', '', len(GOLDEN_SKILL)),
    row('data.cpk::field/pack/fd001.pac'),
    row('data.cpk::field/pack/fd001.pac::a.bf'),
    row('data.cpk::field/pack/fd001.pac::b.tmx'),
    row('data.cpk::model/other/c001.amd', size=8, signature='amd-chunk'),
]
GAME_TABLES = {
    'tables': [{'key': 'skill', 'physical_banks': [
        {'role': 'element', 'source_label': 'battle/SKILL.TBL', 'block': 0, 'payload_bytes': 1152, 'stride': 2,
         'rows': 576},
        {'role': 'active_metadata', 'source_label': 'battle/SKILL.TBL', 'block': 1, 'payload_bytes': 17600,
         'stride': 40, 'rows': 440},
        {'role': 'missing_block', 'source_label': 'battle/SKILL.TBL', 'block': 5, 'payload_bytes': 4, 'stride': 4,
         'rows': 1},
        {'role': 'names', 'source_label': 'battle/MSG.TBL', 'block': 1, 'payload_bytes': 19, 'stride': 19,
         'rows': 1}]}],
    'physical_resource_profile': {'files': [{'provider': 'BTL.CVM', 'key': 'BATTLE/SKILL.TBL'},
                                            {'provider': 'BTL.CVM', 'key': 'BATTLE/MSG.TBL'}]},
}


class SourcesTest(unittest.TestCase):
    def setUp(self):
        self.by_leaf, self.by_base = sources.golden_index(INVENTORY)

    def test_tbl_spans(self):
        self.assertEqual(sources.tbl_spans(PS2_SKILL), [1152, 17600])
        self.assertEqual(sources.tbl_spans(fx.tbl([b'abc', b'']) + bytes(16)), [3, 0])  # an empty block counts
        self.assertEqual(sources.tbl_spans(fx.tbl([b'abc']) + bytes(12)), [3])  # trailing padding ends the file
        self.assertIsNone(sources.tbl_spans(b'\xff\0\0\0abc'))

    def test_match_classes_and_locale_order(self):
        kind, rows = sources.match_path('battle/skill.tbl', self.by_leaf, self.by_base)
        # English overrides the common data.cpk, which precedes the other locales.
        self.assertEqual((kind, [r['locale'] for r in rows]), ('exact-nested', ['e', '', 'fr']))
        self.assertEqual(sources.match_path('field/pack/fd001.pac', self.by_leaf, self.by_base)[0], 'exact-top-level')
        self.assertEqual(sources.match_path('model/c001.amd', self.by_leaf, self.by_base)[0], 'basename')
        self.assertEqual(sources.match_path('movie/op.sfd', self.by_leaf, self.by_base), ('none', []))

    def test_table_inputs_and_banks(self):
        disc = {'BTL/BATTLE/SKILL.TBL': PS2_SKILL, 'BTL/BATTLE/MSG.TBL': fx.tbl([b'n' * 19, b'm' * 19])}
        golden = {'data_e.cpk::init_free.bin::battle/SKILL.TBL': GOLDEN_SKILL}
        inputs, ps2_spans, golden_spans = sources.table_input_rows(GAME_TABLES, disc.__getitem__, self.by_leaf,
                                                                   self.by_base, golden.__getitem__)
        skill, msg = inputs
        self.assertEqual((skill['match'], skill['chosen'], skill['locales']),
                         ('exact-nested', 'data_e.cpk::init_free.bin::battle/SKILL.TBL', ['', 'e', 'fr']))
        self.assertEqual((skill['ps2_spans'], skill['golden_spans'], skill['identical']),
                         ([1152, 17600], [1152, 19360], False))
        self.assertEqual((msg['match'], msg['candidates']), ('none', []))
        banks = {b['role']: b for b in sources.bank_rows(GAME_TABLES, ps2_spans, golden_spans)}
        self.assertEqual(banks['element']['golden'], 'same-size')
        self.assertEqual((banks['active_metadata']['golden'], banks['active_metadata']['golden_stride_if_same_rows']),
                         ('same-stride-other-rows', 44.0))
        self.assertEqual(banks['active_metadata']['golden_rows'], 484)
        self.assertEqual((banks['missing_block']['golden'], banks['missing_block']['golden_blocks']),
                         ('missing-block', 2))
        self.assertEqual(banks['names']['golden'], 'missing-file')

    def test_image_tables(self):
        provenance = [{'address': '0x006420a0', 'size': 184, 'table': 'fusion.gate', 'consumers': ['fclCombine']},
                      {'address': '0x005dd6e0', 'size': 396, 'table': 'player_exp_threshold'},
                      {'address': '0x00600000', 'size': 8, 'table': 'unruled'}]
        spec = {'rules': [{'table': 'fusion', 'golden': ['*::field/pack/fd001.pac::*'], 'note': 'n'},
                          {'table': 'player_exp_threshold', 'golden': [], 'note': 'none known'}]}
        strides = sources.image_strides({'tables': [{'key': 'fusion', 'gate_table': {'stride': 8}}]})
        self.assertEqual(strides, {'fusion.gate': 8, 'player_exp_threshold': 4})
        rows = {r['table']: r for r in sources.image_table_rows(provenance, spec, INVENTORY, strides)}
        gate = rows['fusion.gate']
        self.assertEqual((gate['status'], gate['stride'], gate['consumers']), ('candidate', 8, ['fclCombine']))
        self.assertEqual([c['path'] for c in gate['candidates']],
                         ['data.cpk::field/pack/fd001.pac::a.bf', 'data.cpk::field/pack/fd001.pac::b.tmx'])
        self.assertNotIn('rows_at_ps2_stride', gate['candidates'][0])  # 4 bytes do not hold an 8-byte row
        skill = {'rules': [{'table': 'skill', 'golden': ['*::battle/skill.tbl'], 'note': 'n'}]}
        found = sources.image_table_rows([{'address': '0x0', 'size': 2, 'table': 'skill.x'}], skill, INVENTORY,
                                         {'skill.x': 2})[0]
        self.assertEqual([(c['locale'], c['rows_at_ps2_stride']) for c in found['candidates']],
                         [('e', len(GOLDEN_SKILL) // 2)])  # the preferred locale only
        self.assertEqual(rows['player_exp_threshold']['status'], 'missing')
        self.assertEqual(rows['unruled']['status'], 'no-rule')

    def test_disc_rows(self):
        pack = fx.pak_v1([('a.bf', b'1'), ('c.bf', b'2')])
        model = b'x' * 8
        files = {'DATA/FIELD/PACK/FD001.PAC': pack, 'DATA/MODEL/C001.AMD': model, 'DATA/MOVIE/OP.SFD': b'@SFD'}
        inventory = INVENTORY[:-1] + [row('data.cpk::model/other/c001.amd', size=8, signature='text-ascii',
                                          sha256=hashlib.sha256(model).hexdigest())]
        by_leaf, by_base = sources.golden_index(inventory)
        children = {'data.cpk::field/pack/fd001.pac': ['data.cpk::field/pack/fd001.pac::a.bf',
                                                       'data.cpk::field/pack/fd001.pac::b.tmx']}
        rows = {r['key']: r for r in sources.disc_rows(files, files.__getitem__, by_leaf, by_base, children)}
        pac = rows['DATA/FIELD/PACK/FD001.PAC']
        self.assertEqual((pac['match'], pac['pack_members'], pac['pack_members_in_golden']), ('exact-top-level', 2, 1))
        amd = rows['DATA/MODEL/C001.AMD']
        self.assertEqual((amd['match'], amd['identical_any'], amd['same_signature']), ('basename', True, True))
        self.assertEqual(rows['DATA/MOVIE/OP.SFD']['match'], 'none')
        table = sources.coverage(rows.values(), lambda r: r['container'])
        self.assertEqual(table, {'DATA': {'exact-top-level': 1, 'basename': 1, 'none': 1}})
        pairs = sources.format_pairs(rows.values())
        self.assertEqual(pairs['amd'], {'text-ascii -> text-ascii': 1})
        self.assertEqual(set(pairs), {'pac', 'amd'})
        self.assertEqual([sources.extension(p) for p in ('a/b.tbl', 'dir.x/name', 'name')], ['tbl', '', ''])
        self.assertNotIn('xxxxxxxx', json.dumps(list(rows.values())))

    def test_request_patterns_and_named_by(self):
        strings = ['cdrom0:\\BATTLE\\SKILL.TBL;1', 'model/pack/bc%03d.pac', 'field/f%02d_%s.arc', 'Hello world',
                   'not a path', 'init/itemtbl.bin', 'some text.with space.bin', '%s/%s', '%s%03x.se']
        literals, patterns = sources.request_patterns(strings)
        self.assertEqual(literals, {'battle/skill.tbl', 'init/itemtbl.bin'})
        self.assertEqual([p for p, _ in patterns], ['model/pack/bc%03d.pac', 'field/f%02d_%s.arc'])
        self.assertEqual(sources.named_by('battle/skill.tbl', literals, patterns), 'literal')
        self.assertEqual(sources.named_by('data/init/itemtbl.bin', literals, patterns), 'literal')
        self.assertEqual(sources.named_by('model/pack/bc007.pac', literals, patterns), 'format')
        self.assertIsNone(sources.named_by('model/pack/bc07.pac', literals, patterns))
        self.assertEqual(sources.named_by('field/f01_day.arc', literals, patterns), 'format')
        files = {'BTL/BATTLE/SKILL.TBL': b'x' * 4, 'DATA/MODEL/PACK/BC012.PAC': b'y' * 4, 'DATA/X.BIN': b'z' * 4}
        rows = sources.disc_rows(files, files.__getitem__, self.by_leaf, self.by_base, {}, literals, patterns)
        self.assertEqual([r['named_by'] for r in rows], ['literal', 'format', None])

    def test_candidate_spec_is_well_formed(self):
        spec = json.loads(sources.CANDIDATES.read_text(encoding='utf-8'))
        self.assertTrue(spec['rules'])
        for rule in spec['rules']:
            self.assertEqual(set(rule) - {'table', 'golden', 'note'}, set())
            self.assertTrue(rule['note'])
            self.assertIsInstance(rule['golden'], list)

    def test_refuses_output_inside_repository(self):
        with self.assertRaises(SystemExit):
            sources.check_output(ROOT / 'build')


if __name__ == '__main__':
    unittest.main()
