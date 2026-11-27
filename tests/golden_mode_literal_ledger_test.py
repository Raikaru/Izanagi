#!/usr/bin/env python3
"""Synthetic ledger closure/refusals and immutable M2 metadata coverage."""
import hashlib
import json
from pathlib import Path
import sys
import unittest
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools' / 'golden_mode'))
import literal_inventory
import literal_ledger


def obj(address='0x00530000', size=16, **extra):
    return dict(address=address, size=size, range='data1',
                supplier='decomp-literal', golden_check=True, kind='struct', **extra)


def decision(address='0x00530000', size=16, kind='D10c'):
    return dict(address=address, size=size, symbol=None, decision=kind,
                reason='Synthetic per-object deviation', evidence=['synthetic fixture'])


class InventoryTest(unittest.TestCase):
    def test_selection_does_not_promote_noncheck_variants(self):
        other = obj('0x00530100')
        other['golden_check'] = False
        other['golden_lead'] = dict(match='variant', where='P4G.exe.sdata')
        self.assertEqual(len(literal_inventory.inventory([obj(), other])), 1)

    def test_partial_lead_is_not_mapping_evidence(self):
        rows = literal_inventory.inventory([obj(golden_lead=dict(match='partial', where='P4G.exe.sdata'))])
        self.assertEqual(literal_inventory.summary(rows)['lead_classes'],
                         {'no-variant-lead': {'objects': 1, 'bytes': 16}})

    def test_duplicate_overlapping_empty_and_invalid_sizes_refuse(self):
        for rows in ([], [obj(), obj()], [obj(), obj('0x00530008')], [obj(size=0)], [obj(size=True)]):
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                literal_inventory.inventory(rows)

    def test_committed_full_set(self):
        doc = json.loads((ROOT / 'tools/golden_mode/l1-inventory.json').read_text())
        self.assertEqual(doc['source_sha256'],
                         '19a89b5d028da3fac821ef01d61f2f1fda81510cf392714fe89024a30f0c60d1')
        digest = hashlib.sha256(json.dumps(doc['objects'], sort_keys=True, separators=(',', ':'),
                                           ensure_ascii=True).encode('utf-8')).hexdigest()
        self.assertEqual(digest, '6ade459879a12d08598116ec47503c6c272a2b72993cf2bb0ab50b8c6938f44b')
        self.assertEqual(doc['summary'], literal_inventory.summary(doc['objects']))
        self.assertEqual(doc['summary']['objects'], 1043)
        self.assertEqual(doc['summary']['bytes'], 249428)
        self.assertEqual(doc['summary']['ranges'], {
            'data1': {'objects': 786, 'bytes': 241555},
            'data2': {'objects': 257, 'bytes': 7873}})
        self.assertEqual(doc['summary']['lead_classes'], {
            'exe-variant': {'objects': 585, 'bytes': 177465},
            'archive-variant': {'objects': 123, 'bytes': 63003},
            'no-variant-lead': {'objects': 335, 'bytes': 8960}})
        normalized = [dict(row, supplier='decomp-literal', golden_check=True) for row in doc['objects']]
        self.assertEqual(doc['objects'], literal_inventory.inventory(normalized))


class LedgerTest(unittest.TestCase):
    def test_all_objects_require_one_reasoned_decision(self):
        objects = [obj(), obj('0x00530100')]
        for rows in ([], [decision()], [decision(), decision()],
                     [decision(), decision('0x00530200')]):
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                literal_ledger.validate(objects, rows)
        rows = literal_ledger.validate(objects, [decision('0x00530100'), decision()])
        self.assertEqual([r['address'] for r in rows], ['0x00530000', '0x00530100'])
        self.assertEqual(literal_ledger.counts(rows), {'bound': 0, 'pending-seam': 0, 'D10a': 0, 'D10b': 0, 'D10c': 2, 'OPEN-retain': 0})

    def test_no_unreasoned_or_undecided_classification(self):
        for changes in ({'reason': ''}, {'evidence': []}, {'evidence': ['']},
                        {'decision': 'pending'}, {'decision': 'OPEN-retain'}, {'size': 15}):
            row = decision()
            row.update(changes)
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                literal_ledger.validate([obj()], [row])

    def bound(self):
        row = decision(kind='bound')
        row['golden'] = dict(container='P4G.exe', section='.sdata', offset=32, size=16, sha256='a' * 64)
        row['layout'] = dict(kind='copy', ps2_stride=4, golden_stride=4, fields=[])
        return row

    def test_bound_registry_exact_destination_and_size(self):
        rows = literal_ledger.validate([obj()], [self.bound()])
        self.assertIn('{"image/l1/00530000", "l1@1", 16u, 0x00530000u, 16u}', literal_ledger.registry(rows))
        self.assertNotIn('00530000', literal_ledger.registry([decision()]))

    def test_bound_source_identity_and_data_only_ranges(self):
        for changes in ({'sha256': ''}, {'sha256': 'A' * 64}, {'section': '.arch'},
                        {'section': None}, {'offset': -1}, {'offset': True}, {'size': 0}):
            row = self.bound()
            row['golden'].update(changes)
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                literal_ledger.validate([obj()], [row])
        row = self.bound()
        row['golden'].pop('section')
        row['golden']['member_chain'] = ['init_free.bin', 'synthetic.ftd']
        self.assertEqual(len(literal_ledger.validate([obj()], [row])), 1)

    def test_no_implicit_layout_conversion_or_truncation(self):
        for target, changes in (('layout', {'kind': 'fields'}), ('layout', {'golden_stride': 8}),
                                ('golden', {'size': 32})):
            row = self.bound()
            row[target].update(changes)
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                literal_ledger.registry([row])

    def test_proven_classes_require_current_implemented_seams(self):
        for kind in literal_ledger.PROVEN:
            row = self.bound()
            row['decision'] = kind
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                literal_ledger.validate([obj()], [row])
            row['required_seams'] = [{'owner': 'synthetic reader', 'requirement': 'exact image read'}]
            row['retention'] = {'balance_status': 'pending runtime qualification in source spec'}
            literal_ledger.validate([obj()], [row])
            digest = hashlib.sha256(json.dumps(row['required_seams'], sort_keys=True, separators=(',', ':')).encode()).hexdigest()
            binding = dict(address=row['address'], required_seams_sha256=digest,
                           tests=['golden_mode_image'], consumer_witness='none')
            pending = literal_ledger.runtime_rows([row], [])[0]
            self.assertEqual((pending['decision'], pending['runtime_status']),
                             ('pending-seam', 'pending:M10:steam-span-provider;pending:M6c-2'))
            self.assertEqual(literal_ledger.source_rows([pending]), [])
            for bindings in ([dict(binding, tests=[])], [dict(binding, required_seams_sha256='0' * 64)]):
                with self.subTest(kind=kind, bindings=bindings), self.assertRaises(ValueError):
                    literal_ledger.runtime_rows([row], bindings)
            result = literal_ledger.runtime_rows([row], [binding])[0]
            self.assertEqual((result['decision'], result['spec_decision'], result['consumer_witness']),
                             ('bound', kind, 'none'))
            self.assertNotIn('retention', result)
            self.assertEqual(result['spec_retention'], row['retention'])

    def test_weather_partitions_share_one_native_source_without_guest_writes(self):
        first = self.bound()
        first['address'] = '0x0063fca0'
        first['golden'] = dict(container='data.cpk', member_chain='data.cpk::init_free.bin::init/fclTable.bin::WeatherBonusTbl.ftd',
                               member_sha256='b' * 64, sha256='a' * 64, offset=16, size=3584,
                               source_group='L1c-fusion-weather-expanded-date-table')
        first['layout'] = dict(kind='expanded-date-keyed-table-with-duplicate-ps2-copies',
                               golden_rows=128, golden_stride=28, fields=[])
        second = dict(first, address='0x006432b0')
        sources = literal_ledger.source_rows([first, second])
        self.assertEqual(len(sources), 1)
        self.assertEqual((sources[0]['name'], sources[0]['address'], sources[0]['size']),
                         ('image/l1/fusion-weather', 0, 3584))
        self.assertEqual(literal_ledger.registry([first, second]).count('fusion-weather'), 1)
        native = literal_ledger.manifest_c([obj(first['address']), obj(second['address'])],
                                         [first, second], 'c' * 64)
        self.assertIn('\"%s\", 16u, 3584u' % ('b' * 64), native)
        second = dict(second, golden=dict(first['golden'], sha256='d' * 64))
        with self.assertRaises(ValueError):
            literal_ledger.source_rows([first, second])

    def test_inferred_requires_observable_check(self):
        row = decision(kind='D10a')
        with self.assertRaises(ValueError):
            literal_ledger.validate([obj()], [row])
        row['proposed_check'] = 'Synthetic retail-observable invariant'
        self.assertEqual(literal_ledger.counts(literal_ledger.validate([obj()], [row]))['D10a'], 1)

    def test_open_limitation_is_separate_and_requires_proof_path(self):
        row = decision(kind='OPEN-retain')
        row['proof_path'] = 'Compare the synthetic structure to a specified data source'
        rows = literal_ledger.validate([obj()], [row])
        self.assertEqual(literal_ledger.counts(rows),
                         {'bound': 0, 'pending-seam': 0, 'D10a': 0, 'D10b': 0, 'D10c': 0, 'OPEN-retain': 1})
        self.assertNotIn('00530000', literal_ledger.registry(rows))

    def test_manifest_keeps_per_object_open_provenance(self):
        row = decision(kind='OPEN-retain')
        row['symbol'] = 'synthetic_price'
        row['proof_path'] = 'Owner BBV of price'
        doc = json.loads(literal_ledger.manifest([obj()], [row], 'a' * 64))
        self.assertEqual(doc['counts']['OPEN-retain'], 1)
        self.assertEqual(doc['counts']['D10c'], 0)
        self.assertEqual(doc['objects'], [dict(row, name='synthetic_price', limitation='L1-open')])
        self.assertEqual(doc['inventory_sha256'], 'a' * 64)
        generated = literal_ledger.manifest_c([obj()], [row], 'a' * 64)
        self.assertIn('"synthetic_price", "OPEN-retain"', generated)

    def test_supplements_override_by_address_without_duplicate_objects(self):
        objects = [obj(), obj('0x00530100')]
        primary = [decision(), decision('0x00530100')]
        update = decision()
        update['reason'] = 'Enriched synthetic reader evidence'
        rows = literal_ledger.merge(objects, primary, [[update]])
        self.assertEqual(len(rows), 2)
        self.assertEqual(rows[0]['reason'], update['reason'])
        for overrides in ([update, update], [decision('0x00530200')]):
            with self.subTest(overrides=overrides), self.assertRaises(ValueError):
                literal_ledger.merge(objects, primary, [overrides])

    def test_layout_path_is_hash_pinned_bounded_and_never_implicit(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            proof_path = Path('docs/golden_mode/specs/re/synthetic-layout.json')
            proof = {'span': {'section': '.sdata', 'rva': '0x887020',
                              'size': 10, 'sha256': 'b' * 64}}
            raw = json.dumps(proof).encode()
            (root / proof_path).parent.mkdir(parents=True)
            (root / proof_path).write_bytes(raw)
            row = self.bound()
            row['supply'] = dict(source='spec-layout', proof=proof_path.as_posix(),
                                 proof_sha256=hashlib.sha256(raw).hexdigest(),
                                 span_ref='/span', destination='guest-prefix')
            with patch.object(literal_ledger, 'ROOT', root):
                result = literal_ledger.source_rows([row])[0]
                self.assertEqual((result['size'], result['read_size'], result['address']),
                                 (10, 10, 0x00530000))
                self.assertEqual(result['source'], 'spec-layout')
                self.assertEqual(result['offset'], 32)
                row['supply']['destination'] = 'native'
                self.assertEqual(literal_ledger.source_rows([row])[0]['address'], 0)
                row['supply']['proof_sha256'] = '0' * 64
                with self.assertRaises(ValueError):
                    literal_ledger.source_rows([row])
                row['supply']['proof_sha256'] = hashlib.sha256(raw).hexdigest()
                row['supply']['proof'] = '../outside.json'
                with self.assertRaises(ValueError):
                    literal_ledger.source_rows([row])

    def test_pending_labels_preserve_each_driver_route_and_owner(self):
        row = self.bound()
        row['required_seams'] = ['golden-native-driver:synthetic',
                                 'golden-resource-route:synthetic M8f M11']
        self.assertEqual(literal_ledger.pending_blockers(row), [
            'pending:M10', 'pending:M10:steam-span-provider', 'pending:M11',
            'pending:M6c-2', 'pending:M8f', 'pending:golden-native-driver:synthetic',
            'pending:golden-resource-route:synthetic'])


class CommittedLedgerTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inventory = json.loads((ROOT / 'tools/golden_mode/l1-inventory.json').read_text())
        specs = ROOT / 'docs/golden_mode/specs/re'
        primary = [json.loads(line) for line in (specs / 'l1-image-literals.jsonl').read_text(encoding='utf-8').splitlines()]
        supplement = [json.loads(line) for line in (specs / 'l1-image-literals-archive.jsonl').read_text(encoding='utf-8').splitlines()]
        cls.rows = literal_ledger.merge(cls.inventory['objects'], primary, [supplement])
        bindings = json.loads((ROOT / 'tools/golden_mode/l1-bindings.json').read_text())
        cls.rows = literal_ledger.runtime_rows(cls.rows, bindings)

    def test_complete_no_unsupplied_or_unclassified_objects(self):
        self.assertEqual(len(self.rows), 1043)
        self.assertEqual(sum(row['size'] for row in self.rows), 249428)
        self.assertEqual(sum(literal_ledger.counts(self.rows).values()), 1043)
        for row in self.rows:
            self.assertTrue(row['reason'] and row['evidence'])
            if row['decision'] == 'OPEN-retain':
                self.assertTrue(row['proof_path'])
                self.assertIsNone(row['golden'])
                self.assertEqual(row['retention']['source'], 'PS2 compiled literal')
                self.assertEqual(row['retention']['address'], row['address'])
                self.assertEqual(row['retention']['size'], row['size'])

    def test_compiled_manifest_and_cache_identity_are_current(self):
        objects, source = self.inventory['objects'], self.inventory['source_sha256']
        expected = literal_ledger.manifest_c(objects, self.rows, source)
        native = ROOT / 'src/platform/golden'
        self.assertEqual((native / 'golden_mode_literal_manifest.inc').read_text(), expected)
        digest = hashlib.sha256(literal_ledger.manifest(objects, self.rows, source).encode('utf-8')).hexdigest()
        sources = literal_ledger.source_rows(self.rows)
        byte_count = max((s['output_offset'] + max(s['size'], s['read_size']) for s in sources), default=0)
        pe_bytes = max((s['output_offset'] + s['read_size'] for s in sources if s['read_size']), default=0)
        identity = ('/* Generated metadata identity; no game bytes. */\n'
                    '#define P4_GOLDEN_LITERAL_MANIFEST_SHA256 \"%s\"\n'
                    '#define P4_GOLDEN_LITERAL_SOURCE_COUNT %du\n'
                    '#define P4_GOLDEN_LITERAL_BYTES %du\n'
                    '#define P4_GOLDEN_LITERAL_PE_BYTES %du\n') % (digest, len(sources), byte_count, pe_bytes)
        self.assertEqual((native / 'golden_mode_literal_rows.inc').read_text(), literal_ledger.registry(self.rows))
        self.assertEqual((native / 'golden_mode_literal_identity.h').read_text(), identity)

    def test_every_open_object_is_named_in_boot_manifest(self):
        doc = json.loads(literal_ledger.manifest(self.inventory['objects'], self.rows,
                                               self.inventory['source_sha256']))
        for row in doc['objects']:
            self.assertTrue(row['name'])
            if row['decision'] == 'OPEN-retain':
                self.assertEqual(row['limitation'], 'L1-open')

    def test_authored_values_equal_the_hash_pinned_specification(self):
        sources = {s['name']: s for s in literal_ledger.source_rows(self.rows)}
        for row in self.rows:
            supply = row.get('supply', {})
            if supply.get('source') != 'spec-constant':
                continue
            proof = json.loads((ROOT / supply['proof']).read_text(encoding='utf-8'))
            if row['address'] == '0x0076151c':
                expected = int(proof['value']['bits_hex'], 16).to_bytes(4, 'little')
            elif row['address'] == '0x0063c660':
                expected = b''.join(int(cell['bits'], 16).to_bytes(4, 'little')
                                    for cell in proof['schema']['fixed_cells'])
            else:
                self.fail('Missing independent specification-value equality assertion')
            actual = bytes.fromhex(sources[literal_ledger.image_name(row)]['constant_hex'])
            self.assertEqual(actual, expected)
            self.assertEqual(sources[literal_ledger.image_name(row)]['source'], 'spec-constant')

    def test_pending_and_non_supply_have_no_runtime_object(self):
        source_names = {s['name'] for s in literal_ledger.source_rows(self.rows)}
        for row in self.rows:
            if row['decision'] in ('pending-seam', 'D10c'):
                self.assertNotIn(literal_ledger.image_name(row), source_names)
            if row['decision'] == 'pending-seam':
                self.assertTrue(row['runtime_status'].startswith('pending:'))

    def test_pending_census_names_every_blocker_without_duplicate_rows(self):
        expected = literal_ledger.pending_report(self.rows)
        actual = json.loads((ROOT / 'tools/golden_mode/l1-pending.json').read_text(encoding='utf-8'))
        self.assertEqual(actual, expected)
        pending = {row['address'] for row in self.rows if row['decision'] == 'pending-seam'}
        covered = set()
        for group in actual['blockers'].values():
            self.assertEqual(group['count'], len(set(group['addresses'])))
            covered.update(group['addresses'])
        self.assertEqual(covered, pending)


if __name__ == '__main__':
    unittest.main()
