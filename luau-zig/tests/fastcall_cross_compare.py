#!/usr/bin/env python3
"""Compare two complete Zig numeric traces without rounding or NaN tolerance."""
import itertools
import math
import re
import struct
import sys

PATTERN = re.compile(r'M (\d+) (\d+) (\d+) ([0-9a-fA-F]{16})')
IDS = set(range(2, 40)) | {46, 47, 48, 55, 56, 59, 64, 89, 91, 92, 93}


def records(path):
    with open(path, encoding='utf-8') as stream:
        for line in stream:
            match = PATTERN.search(line)
            if match:
                yield tuple(int(value, 16 if index == 3 else 10)
                            for index, value in enumerate(match.groups()))


def isnan(bits):
    return math.isnan(struct.unpack('>d', bits.to_bytes(8, 'big'))[0])


stats = {}
for left, right in itertools.zip_longest(records(sys.argv[1]), records(sys.argv[2])):
    if left is None or right is None or left[:3] != right[:3]:
        raise SystemExit('missing or reordered cross-platform sample')
    ident, sample, result, expected = left
    actual = right[3]
    entry = stats.setdefault(ident, {'records': 0, 'exact': 0, 'raw_nan': 0, 'failure': 0, 'examples': []})
    entry['records'] += 1
    if expected == actual:
        entry['exact'] += 1
    elif ident in (13, 89) and isnan(expected) and isnan(actual):
        # Raw fmod/lerp arithmetic is measured separately, not normalized here.
        entry['raw_nan'] += 1
        if len(entry['examples']) < 10:
            entry['examples'].append((sample, result, f'{expected:016x}', f'{actual:016x}'))
    else:
        entry['failure'] += 1
        if len(entry['examples']) < 10:
            entry['examples'].append((sample, result, f'{expected:016x}', f'{actual:016x}'))
if set(stats) != IDS or any(entry['records'] != (20018 if ident in (14, 20) else 10009)
                             for ident, entry in stats.items()):
    raise SystemExit('missing or duplicate cross-platform samples')
for ident, entry in sorted(stats.items()):
    print(f'id={ident} ' + ' '.join(f'{key}={value}' for key, value in entry.items()))
if any(entry['failure'] for entry in stats.values()):
    raise SystemExit(1)
