#!/usr/bin/env python3
"""Validate complete per-object L1 decisions and generate same-shape binder rows.

Neither M2 heuristic leads nor missing decision rows authorize a supplier. This
utility reads metadata only. It never searches the executable or game archives.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re

DECISIONS = ('bound', 'pending-seam', 'D10a', 'D10b', 'D10c', 'OPEN-retain')
PROVEN = ('PROVEN', 'PROVEN-code', 'PROVEN-layout')
ROOT = Path(__file__).resolve().parents[2]
ADDRESS = re.compile(r'0x[0-9a-f]{8}\Z')
SHA256 = re.compile(r'[0-9a-f]{64}\Z')


def nonempty(value):
    return isinstance(value, str) and bool(value.strip())


def evidence_valid(value):
    return nonempty(value) or (isinstance(value, dict) and
                               nonempty(value.get('class', value.get('classification'))) and
                               nonempty(value.get('source', value.get('ps2_source',
                                        value.get('authority', value.get('proof'))))))


def validate(objects, decisions):
    expected = {}
    for obj in objects:
        address = obj.get('address')
        if not isinstance(address, str) or not ADDRESS.fullmatch(address) or address in expected:
            raise ValueError('invalid or duplicate inventory address')
        expected[address] = obj
    if not expected:
        raise ValueError('empty literal inventory')
    found = {}
    for row in decisions:
        address = row.get('address')
        if not isinstance(address, str) or address not in expected or address in found:
            raise ValueError('unknown or duplicate decision address: %r' % address)
        if type(row.get('size')) is not int or row['size'] != expected[address]['size']:
            raise ValueError('decision size differs from inventory: ' + address)
        decision = row.get('decision')
        if decision not in DECISIONS + PROVEN:
            raise ValueError('undecided object: ' + address)
        if not nonempty(row.get('reason')) or not isinstance(row.get('evidence'), list) or not row['evidence'] or not all(evidence_valid(e) for e in row['evidence']):
            raise ValueError('missing per-object reason/evidence: ' + address)
        if decision in PROVEN:
            if not row.get('required_seams') or not isinstance(row.get('golden'), dict):
                raise ValueError('proven source lacks source/seam contract: ' + address)
        elif decision == 'pending-seam':
            if (row.get('spec_decision') not in PROVEN or not row.get('required_seams') or
                    not row.get('runtime_status', '').startswith('pending:') or row.get('supplier')):
                raise ValueError('unnamed or supplied pending seam: ' + address)
        elif decision == 'bound' and row.get('supply'):
            supply_source(row)
        elif decision == 'bound':
            golden, layout = row.get('golden', {}), row.get('layout', {})
            if not isinstance(golden, dict) or not isinstance(layout, dict):
                raise ValueError('missing source/layout: ' + address)
            source_hash = golden.get('sha256', golden.get('member_sha256'))
            if layout.get('kind') in ('widened-renderer-pointer-table', 'bounded-vec3-prefix-with-retained-tail'):
                source_hash = golden.get('probe_sha256')
            if not nonempty(golden.get('container')) or not isinstance(source_hash, str) or not SHA256.fullmatch(source_hash):
                raise ValueError('missing source identity: ' + address)
            if golden.get('section') not in ('.sdata', '.xpdata', None):
                raise ValueError('source is not an allowed data section: ' + address)
            if golden.get('section') is None and not golden.get('member_chain'):
                raise ValueError('source lacks data section or archive member: ' + address)
            offset, size = golden.get('offset'), golden.get('size')
            if offset is None and golden.get('section') in ('.sdata', '.xpdata') and golden.get('rva'):
                offset = int(golden['rva'], 16) - {'.sdata': 0x887000, '.xpdata': 0xa20000}[golden['section']]
            if type(offset) is not int or offset < 0 or type(size) is not int or size <= 0:
                raise ValueError('invalid source range: ' + address)
            if not nonempty(layout.get('kind')) or not isinstance(layout.get('fields'), list):
                raise ValueError('missing specified layout: ' + address)
        elif decision == 'D10a' and not nonempty(row.get('proposed_check')):
            raise ValueError('D10a lacks observable check: ' + address)
        elif decision == 'OPEN-retain' and not nonempty(row.get('proof_path')):
            raise ValueError('open limitation lacks per-object proof path: ' + address)
        found[address] = row
    missing = sorted(set(expected) - set(found))
    if missing:
        raise ValueError('unsupplied/unclassified objects (%d): %s' % (len(missing), ', '.join(missing)))
    return [found[address] for address in sorted(found)]


def merge(objects, primary, supplements):
    merged = {row['address']: row for row in validate(objects, primary)}
    for supplement in supplements:
        addresses = {row['address'] for row in supplement}
        subset = [obj for obj in objects if obj['address'] in addresses]
        for row in validate(subset, supplement):
            merged[row['address']] = row
    return validate(objects, list(merged.values()))


def counts(rows):
    tally = collections.Counter(row['decision'] for row in rows)
    return {key: tally[key] for key in DECISIONS}


def pending_blockers(row):
    seams = json.dumps(row.get('required_seams', []))
    labels = {'pending:' + marker for marker in
              re.findall(r'golden-(?:native-driver|resource-route):[a-z0-9-]+', seams)}
    labels.update('pending:' + owner for owner in re.findall(r'\bM(?:7[a-d]|8[a-g]|9|10|11)\b', seams))
    if 'golden-resource-route:' in seams:
        labels.update(('pending:M10', 'pending:M11'))
    if any(label.startswith('pending:golden-native-driver:') for label in labels) or not labels:
        labels.add('pending:M6c-2')
    # A span dependency alone does not close any consumer/lifetime obligation.
    def steam_span(value):
        if isinstance(value, dict):
            if value.get('section') in ('.sdata', '.xpdata') and ('rva' in value or 'offset' in value):
                return True
            return any(steam_span(v) for v in value.values())
        return isinstance(value, list) and any(steam_span(v) for v in value)
    if steam_span(row.get('golden')):
        labels.add('pending:M10:steam-span-provider')
    return sorted(labels)


def pending_report(rows):
    unmarked = {}
    groups, retired_groups = {}, {}
    for row in rows:
        target = groups if row['decision'] == 'pending-seam' else retired_groups if row['decision'] == 'D10c' else None
        if target is not None:
            for blocker in row['pending_blockers']:
                target.setdefault(blocker, []).append(row['address'])
        if row['decision'] == 'pending-seam' and not any(
                label.startswith('pending:golden-') for label in row['pending_blockers']):
            kind = row.get('layout', {}).get('kind', 'unspecified')
            unmarked.setdefault(kind, []).append(row['address'])
    return dict(counts=counts(rows),
                unmarked_consumer_contracts={key: dict(count=len(value), addresses=value)
                                            for key, value in sorted(unmarked.items())},
                counting='Non-disjoint blocker sets; each address counted once per label.',
                blockers={key: dict(count=len(value), addresses=value) for key, value in sorted(groups.items())},
                non_supply_blockers={key: dict(count=len(value), addresses=value)
                                     for key, value in sorted(retired_groups.items())})


def runtime_rows(rows, bindings):
    implemented = {}
    for binding in bindings:
        address = binding['address']
        if address in implemented:
            raise ValueError('duplicate runtime binding: ' + address)
        implemented[address] = binding
    result = []
    for original in rows:
        row = dict(original)
        if row['decision'] in PROVEN:
            binding = implemented.pop(row['address'], None)
            if binding is None:
                blockers = pending_blockers(row)
                row.update(decision='pending-seam', spec_decision=original['decision'],
                           runtime_status=';'.join(blockers), supplier='', pending_blockers=blockers)
            else:
                digest = hashlib.sha256(json.dumps(row['required_seams'], sort_keys=True,
                                                    separators=(',', ':')).encode()).hexdigest()
                if binding.get('required_seams_sha256') != digest or not binding.get('tests'):
                    raise ValueError('stale or unqualified runtime seam implementation: ' + row['address'])
                row.update(decision='bound', spec_decision=original['decision'], runtime_status='bound',
                           implementation=binding, supplier=image_name(row))
                if binding.get('supply'):
                    row['supply'] = binding['supply']
                    supply_source(row)
                if 'retention' in row:
                    row['spec_retention'] = row.pop('retention')
                if 'consumer_witness' in binding:
                    row['consumer_witness'] = binding['consumer_witness']
        elif row['decision'] == 'D10c':
            row.update(runtime_status='non-supply', supplier='', pending_blockers=pending_blockers(row))
        result.append(row)
    if implemented:
        raise ValueError('runtime binding has no proven source: ' + ', '.join(implemented))
    return result


def proof_value(document, pointer):
    if not isinstance(pointer, str) or not pointer.startswith('/'):
        raise ValueError('invalid specification evidence pointer')
    value = document
    for part in pointer[1:].split('/'):
        key = part.replace('~1', '/').replace('~0', '~')
        value = value[int(key)] if isinstance(value, list) else value[key]
    return value


def supply_source(row):
    """Read only a hash-pinned public specification, never an image/code file."""
    supply = row['supply']
    path = (ROOT / supply['proof']).resolve()
    specs = (ROOT / 'docs/golden_mode/specs/re').resolve()
    if not path.is_relative_to(specs) or path.suffix != '.json':
        raise ValueError('supplier evidence is not a public specification')
    raw = path.read_bytes().replace(b'\r\n', b'\n')
    if hashlib.sha256(raw).hexdigest() != supply.get('proof_sha256'):
        raise ValueError('supplier specification identity differs: ' + row['address'])
    proof = json.loads(raw)
    if supply['source'] == 'spec-constant':
        data = bytearray()
        for field in supply['fields']:
            value = proof_value(proof, field['ref'])
            bits = int(value, 16) if isinstance(value, str) else value
            width = field['width']
            if type(bits) is not int or width not in (1, 2, 4, 8) or not 0 <= bits < 1 << (8 * width):
                raise ValueError('invalid authored constant field')
            data.extend(bits.to_bytes(width, 'little'))
        if not data:
            raise ValueError('empty authored constant')
        result = dict(section='', member_chain='', offset=0, size=len(data),
                      constant_hex=data.hex(), sha256=hashlib.sha256(data).hexdigest())
    elif supply['source'] == 'spec-layout':
        span = proof_value(proof, supply['span_ref'])
        if span['section'] not in ('.sdata', '.xpdata') or not SHA256.fullmatch(span['sha256']):
            raise ValueError('layout source is not a qualified data span')
        size = span['size']
        offset = int(span['rva'], 16) - {'.sdata': 0x887000, '.xpdata': 0xa20000}[span['section']]
        if type(size) is not int or size <= 0 or offset < 0:
            raise ValueError('invalid layout source span')
        result = dict(section=span['section'], member_chain='', offset=offset,
                      size=size, sha256=span['sha256'])
    else:
        raise ValueError('unknown specification supplier')
    address = int(row['address'], 16) if supply['destination'] == 'guest-prefix' else 0
    if supply['destination'] not in ('guest-prefix', 'native') or address and result['size'] > row['size']:
        raise ValueError('invalid specification supplier destination')
    return dict(result, source=supply['source'], name=image_name(row), address=address,
                rows=0, stride=0, converter=0,
                read_size=result['size'] if result['section'] else 0)


def image_name(row):
    if row.get('layout', {}).get('kind') == 'expanded-date-keyed-table-with-duplicate-ps2-copies':
        return 'image/l1/fusion-weather'
    return 'image/l1/' + row['address'][2:]


def source_rows(rows):
    sources = []
    offset = 0
    weather_source = None
    for row in rows:
        if row['decision'] != 'bound':
            continue
        if row.get('supply'):
            source = supply_source(row)
            source['output_offset'] = offset
            sources.append(source)
            offset += source['size']
            continue
        golden, layout = row['golden'], row['layout']
        kind = layout['kind']
        supported = ('copy', 'same-shape-copy', 'prefix-copy', 'keyed-expansion',
                     'mapped-prefix-with-consumer-domain-change', 'two-table-semantic-lookup',
                     'mapped-prefix-f32', 'widened-renderer-pointer-table', 'bounded-vec3-prefix-with-retained-tail',
                     'expanded-date-keyed-table-with-duplicate-ps2-copies')
        if kind not in supported:
            raise ValueError('bound object needs a specified converter: ' + row['address'] + ': ' + kind)
        if kind in ('copy', 'same-shape-copy') and (
                golden['size'] != row['size'] or
                (layout.get('ps2_stride') is not None and layout['ps2_stride'] != layout.get('golden_stride'))):
            raise ValueError('same-shape source changes extent or stride: ' + row['address'])
        expanded_weather = kind == 'expanded-date-keyed-table-with-duplicate-ps2-copies'
        if expanded_weather:
            if (golden.get('source_group'), golden['size'], layout.get('golden_rows'),
                layout.get('golden_stride')) != ('L1c-fusion-weather-expanded-date-table', 3584, 128, 28):
                raise ValueError('unsupported weather descriptor: ' + row['address'])
            if weather_source is not None:
                if golden != weather_source:
                    raise ValueError('shared weather source differs: ' + row['address'])
                continue
            weather_source = golden
        address = 0 if kind == 'keyed-expansion' or expanded_weather else int(row['address'], 16)
        size = golden['size']
        queue = kind == 'widened-renderer-pointer-table'
        vector_prefix = kind == 'bounded-vec3-prefix-with-retained-tail'
        if vector_prefix:
            binding = row.get('binding', {})
            if (row['size'], golden['size'], golden.get('logical_payload_bytes'),
                binding.get('copy_prefix_bytes'), binding.get('retain_ps2_tail_bytes'),
                binding.get('record_bytes'), binding.get('guest_address')) != (16, 16, 12, 12, 4, 16, row['address']):
                raise ValueError('unsupported bounded vector prefix: ' + row['address'])
            size = 12
        if queue:
            norm = golden['normalization']
            if (int(norm['source_queue_rva_base'], 16), norm['source_queue_stride'],
                int(norm['guest_queue_base'], 16), norm['guest_queue_stride'],
                norm['queue_count'], norm['banks'], norm['output_bytes']) != (0x1e41b2c0, 96, 0x796520, 48, 7, 3, 84):
                raise ValueError('queue family requires a different normalization: ' + row['address'])
            size = norm['output_bytes']
        if address and size > row['size']:
            raise ValueError('oversized fixed image write: ' + row['address'])
        source = dict(golden, name=image_name(row), address=address, size=size,
                      output_offset=offset, rows=layout.get('golden_rows', 0),
                      stride=layout.get('golden_stride', 0), converter=int(queue),
                      read_size=golden['size'] if golden.get('section') else 0)
        if queue:
            source['offset'] = int(golden['rva'], 16) - 0xa20000
            source['sha256'] = golden['probe_sha256']
        if vector_prefix:
            source['sha256'] = golden['probe_sha256']
        sources.append(source)
        # Reserve the widened input in-place; normalize only after all reads.
        offset += max(size, golden['size']) if golden.get('section') else size
        auxiliaries = list(golden.get('auxiliary_sources', []))
        if queue:
            remap = golden['slot_remap_source']
            # Existing PS2 tnSlotRemap bytes, not a copied Golden payload.
            auxiliaries.append(dict(section=remap['section'],
                offset=int(remap['rva'], 16) - 0x887000, size=8, rows=8, stride=1,
                sha256=hashlib.sha256(bytes((0, 1, 4, 6, 2, 3, 5, 0))).hexdigest()))
        for aux in auxiliaries:
            pe = bool(aux.get('section'))
            source = dict(aux, name='image/l1/' + row['address'][2:] + ('-remap' if pe else '-map'),
                          address=0, offset=aux['offset'] if pe else aux['payload_offset'],
                          size=aux['size'] if pe else aux['payload_bytes'], output_offset=offset, converter=0)
            sources.append(source)
            offset += source['size']
    for source in sources:
        pe = bool(source.get('section'))
        source['read_size'] = source.get('read_size', source['size']) if pe else 0
    return sources


def registry(rows):
    lines = ['/* Generated by literal_ledger.py; do not edit. */']
    for source in source_rows(rows):
        lines.append('    {%s, \"l1@1\", %du, 0x%08xu, %du},' % (
            json.dumps(source['name']), source['size'], source['address'],
            source['size'] if source['address'] else 0))
    return '\n'.join(lines) + '\n'


def manifest(objects, decisions, source_sha256):
    rows = validate(objects, decisions)
    rows = [dict(row, name=row.get('symbol') or 'image/l1/' + row['address'][2:],
                 **({'limitation': 'L1-open'} if row['decision'] == 'OPEN-retain' else {})) for row in rows]
    return json.dumps({'format': 1, 'inventory_sha256': source_sha256,
                       'counts': counts(rows), 'objects': rows},
                      sort_keys=True, separators=(',', ':'), ensure_ascii=True) + '\n'


def manifest_c(objects, decisions, source_sha256):
    rows = validate(objects, decisions)
    text = manifest(objects, rows, source_sha256)
    # One JSON object per literal keeps C string tokens well below compiler
    # limits, even with long per-object proof paths and provenance.
    parts = text.split('},{')
    lines = ['/* Generated by literal_ledger.py; metadata only. */',
             'static const char literal_manifest[] =']
    for i, part in enumerate(parts):
        if i:
            part = '},{' + part
        lines.append('    ' + json.dumps(part, ensure_ascii=True))
    lines[-1] += ';'
    lines.append('static const P4GoldenLiteralObject literal_registry[] = {')
    for row in rows:
        name = row.get('symbol') or 'image/l1/' + row['address'][2:]
        lines.append('    {%su, %du, %s, %s, %s, %s, %s, %s},' % (
            row['address'], row['size'], json.dumps(name, ensure_ascii=True),
            json.dumps(row['decision']), json.dumps(row['reason'], ensure_ascii=True),
            json.dumps(row.get('proof_path', ''), ensure_ascii=True),
            json.dumps(image_name(row) if row['decision'] == 'bound' else ''),
            json.dumps(row.get('runtime_status', 'L1-open'))))
    lines.append('};')
    sources = source_rows(rows)
    for i, source in enumerate(sources):
        if 'constant_hex' in source:
            values = ', '.join('0x%02x' % value for value in bytes.fromhex(source['constant_hex']))
            lines.append('static const uint8_t literal_constant_%d[] = {%s};' % (i, values))
    lines.append('static const P4GoldenLiteralSource literal_sources[] = {')
    for i, source in enumerate(sources):
        section = source.get('section', '')
        member = source.get('member_chain', '')
        source_hash = source['sha256'] if section or source.get('constant_hex') else source['member_sha256']
        constant = 'literal_constant_%d' % i if source.get('constant_hex') else 'NULL'
        lines.append('    {%s, %s, %s, %s, %du, %du, %du, %du, %du, %du, %du, %s},' % (
            json.dumps(source['name']), json.dumps(section), json.dumps(member), json.dumps(source_hash),
            source['offset'], source['size'], source['output_offset'], source['rows'], source['stride'],
            source['read_size'], source['converter'], constant))
    lines.append('};')
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--decisions', type=Path, required=True)
    parser.add_argument('--supplement', type=Path, action='append', default=[])
    parser.add_argument('--bindings', type=Path)
    parser.add_argument('--registry-out', type=Path)
    parser.add_argument('--manifest-out', type=Path)
    parser.add_argument('--pending-out', type=Path)
    parser.add_argument('--manifest-c-out', type=Path)
    parser.add_argument('--identity-out', type=Path)
    args = parser.parse_args()
    inventory = json.loads(args.inventory.read_text(encoding='utf-8'))
    raw = args.decisions.read_bytes()
    sources = [raw] + [path.read_bytes() for path in args.supplement]
    batches = [[json.loads(line) for line in source.decode('utf-8').splitlines() if line.strip()] for source in sources]
    rows = merge(inventory['objects'], batches[0], batches[1:])
    bindings = json.loads(args.bindings.read_text(encoding='utf-8')) if args.bindings else []
    rows = runtime_rows(rows, bindings)
    if args.pending_out:
        args.pending_out.write_text(json.dumps(pending_report(rows), indent=2) + '\n', encoding='utf-8')
    if args.registry_out:
        args.registry_out.write_text(registry(rows), encoding='utf-8')
    if args.manifest_out:
        args.manifest_out.write_text(manifest(inventory['objects'], rows, inventory['source_sha256']), encoding='utf-8')
    if args.manifest_c_out:
        args.manifest_c_out.write_text(manifest_c(inventory['objects'], rows, inventory['source_sha256']), encoding='utf-8')
    if args.identity_out:
        digest = hashlib.sha256(manifest(inventory['objects'], rows, inventory['source_sha256']).encode('utf-8')).hexdigest()
        source_specs = source_rows(rows)
        byte_count = max((s['output_offset'] + max(s['size'], s['read_size']) for s in source_specs), default=0)
        args.identity_out.write_text('/* Generated metadata identity; no game bytes. */\n'
            '#define P4_GOLDEN_LITERAL_MANIFEST_SHA256 \"%s\"\n'
            '#define P4_GOLDEN_LITERAL_SOURCE_COUNT %du\n'
            '#define P4_GOLDEN_LITERAL_BYTES %du\n'
            '#define P4_GOLDEN_LITERAL_PE_BYTES %du\n' %
            (digest, len(source_specs), byte_count,
             max((s['output_offset'] + s['read_size'] for s in source_specs if s['read_size']), default=0)), encoding='utf-8')
    print(json.dumps({'counts': counts(rows), 'objects': len(rows),
                      'bytes': sum(row['size'] for row in rows),
                      'decision_sources_sha256': [hashlib.sha256(source).hexdigest() for source in sources]}, sort_keys=True))


if __name__ == '__main__':
    main()
