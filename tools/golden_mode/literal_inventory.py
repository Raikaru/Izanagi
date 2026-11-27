#!/usr/bin/env python3
"""Freeze M2's exact golden_check set without reading or emitting game bytes.

A lead is a heuristic, not binding authority. In particular partial leads belong
in the no-proven-source bucket, and variants outside golden_check are not L1.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path

FIELDS = ('address', 'size', 'range', 'code_referenced', 'kind', 'evidence',
          'element_width', 'stride', 'rows', 'declared', 'names', 'consumers', 'golden_lead')


def inventory(rows):
    objects = []
    previous_end = 0
    for row in sorted(rows, key=lambda r: int(r['address'], 16)):
        if row.get('golden_check') is not True:
            continue
        address, size = int(row['address'], 16), row['size']
        if not isinstance(size, int) or isinstance(size, bool) or size <= 0:
            raise ValueError('nonpositive object size')
        if address < previous_end or address + size > 0x100000000:
            raise ValueError('duplicate, overlapping or overflowing object')
        if row.get('supplier') != 'decomp-literal':
            raise ValueError('golden_check must be a decomp literal')
        if row['range'] not in ('data1', 'data2'):
            raise ValueError('unknown image range')
        previous_end = address + size
        objects.append({key: row[key] for key in FIELDS if key in row})
    if not objects:
        raise ValueError('empty golden_check inventory')
    return objects


def summary(objects):
    result = {'objects': len(objects), 'bytes': sum(r['size'] for r in objects),
              'ranges': {}, 'lead_classes': {}}
    for label in ('data1', 'data2'):
        part = [r for r in objects if r['range'] == label]
        result['ranges'][label] = {'objects': len(part), 'bytes': sum(r['size'] for r in part)}
    groups = collections.defaultdict(list)
    for row in objects:
        lead = row.get('golden_lead', {})
        label = 'no-variant-lead'
        if lead.get('match') == 'variant':
            label = 'exe-variant' if lead['where'].startswith('P4G.exe.') else 'archive-variant'
        groups[label].append(row)
    for label, part in sorted(groups.items()):
        result['lead_classes'][label] = {'objects': len(part), 'bytes': sum(r['size'] for r in part)}
    return result


def document(raw):
    objects = inventory(json.loads(line) for line in raw.decode('utf-8').splitlines() if line.strip())
    return {'format': 1, 'source_sha256': hashlib.sha256(raw).hexdigest(),
            'selection': 'M2 typed.jsonl: golden_check == true',
            'summary': summary(objects), 'objects': objects}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--typed', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    result = document(args.typed.read_bytes())
    args.out.write_text(json.dumps(result, indent=2, ensure_ascii=True) + '\n', encoding='utf-8')
    print(json.dumps(result['summary'], sort_keys=True))


if __name__ == '__main__':
    main()
