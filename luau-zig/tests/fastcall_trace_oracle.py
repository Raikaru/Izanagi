#!/usr/bin/env python3
"""Classify full raw-bit numeric traces against MPFR and pinned Luau."""
import argparse
import ctypes
import ctypes.util
import itertools
import json
import math
import os
from pathlib import Path
import re
from fastcall_oracle import MPFR, IDS, bits, distance, number

PATTERN = re.compile(r'M (\d+) (\d+) (\d+) ([0-9a-fA-F]{16})')
ALL_IDS = sorted(set(range(2, 40)) | {46, 47, 48, 55, 56, 59, 64, 89, 91, 92, 93})
EDGES = [0, 0x8000000000000000, 1, 0x8000000000000001, 0x7ff0000000000000,
         0xfff0000000000000, 0x7ff8000000001234, 0xfff8000000005678,
         0x41e0000000000000, 0xc1e0000000000000, 0x41f0000000000000, 0x43e0000000000000]
MASK = (1 << 64) - 1


def records(path):
    with open(path, encoding='utf-8') as stream:
        for line in stream:
            match = PATTERN.search(line)
            if match:
                yield tuple(int(value, 16 if i == 3 else 10) for i, value in enumerate(match.groups()))


def sample_inputs():
    dense = 0x2345abcd
    result = {}
    for ident in ALL_IDS:
        seed = 0x8d26c5ab91d37e4f
        values = []
        for index in range(10009):
            if index == 0:
                value = 4.0
            elif index <= 8:
                dense = (dense * 1664525 + 1013904223) & 0xffffffff
                bitwise = 28 <= ident <= 39 or ident in (55, 56, 59, 64)
                value = float(dense % 2049 - 1024 if bitwise else dense % 19001 - 9500)
                if not bitwise:
                    value /= 10000
            else:
                seed = (seed * 6364136223846793005 + 1442695040888963407) & MASK
                value = number(EDGES[index - 9] if index - 9 < len(EDGES) else seed)
            values.append(value)
        result[ident] = values
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    name = os.environ.get('P4PC_MPFR_LIBRARY') or ctypes.util.find_library('mpfr')
    if not name:
        raise RuntimeError('MPFR shared library is required')
    lib = ctypes.CDLL(name)
    ptr = ctypes.POINTER(MPFR)
    lib.mpfr_init2.argtypes = [ptr, ctypes.c_long]
    lib.mpfr_set_d.argtypes = [ptr, ctypes.c_double, ctypes.c_int]
    lib.mpfr_get_d.argtypes = [ptr, ctypes.c_int]
    lib.mpfr_get_d.restype = ctypes.c_double
    lib.mpfr_clear.argtypes = [ptr]
    for name in set(IDS.values()):
        getattr(lib, 'mpfr_' + name).argtypes = [ptr, ptr, ptr, ctypes.c_int] if name in ('atan2', 'pow') else [ptr, ptr, ctypes.c_int]
    x, y, out = MPFR(), MPFR(), MPFR()
    for value in (x, y, out):
        lib.mpfr_init2(ctypes.byref(value), 256)
    inputs = sample_inputs()
    stats = {}
    try:
        for left, right in itertools.zip_longest(records(args.reference), records(args.candidate)):
            if left is None or right is None or left[:3] != right[:3]:
                raise ValueError('missing or reordered trace record')
            ident, sample, result, ref = left
            got = right[3]
            entry = stats.setdefault(ident, {'records': 0, 'exact_reference': 0, 'finite_divergence': 0,
                                             'nan_divergence': 0, 'failure': 0, 'max_oracle_ulp': 0, 'examples': []})
            entry['records'] += 1
            if ident not in IDS:
                if ref != got:
                    entry['failure'] += 1
                else:
                    entry['exact_reference'] += 1
                continue
            value = inputs[ident][sample]
            lib.mpfr_set_d(ctypes.byref(x), value, 0)
            lib.mpfr_set_d(ctypes.byref(y), 2.0, 0)
            fn = getattr(lib, 'mpfr_' + IDS[ident])
            if ident in (5, 21):
                fn(ctypes.byref(out), ctypes.byref(x), ctypes.byref(y), 0)
            else:
                fn(ctypes.byref(out), ctypes.byref(x), 0)
            truth = bits(lib.mpfr_get_d(ctypes.byref(out), 0))
            real, observed, expected = number(ref), number(got), number(truth)
            if ref == got:
                entry['exact_reference'] += 1
            elif math.isnan(real) and math.isnan(observed):
                entry['nan_divergence'] += 1
            elif math.isfinite(real) and math.isfinite(observed):
                entry['finite_divergence'] += 1
            else:
                entry['failure'] += 1
            if math.isnan(expected):
                accurate = math.isnan(observed)
            elif math.isinf(expected):
                accurate = got == truth
            else:
                delta = distance(got, truth)
                accurate = delta is not None and delta <= 1
                if delta is not None:
                    entry['max_oracle_ulp'] = max(entry['max_oracle_ulp'], delta)
            if not accurate:
                entry['failure'] += 1
                if len(entry['examples']) < 10:
                    entry['examples'].append({'sample': sample, 'input': f'{bits(value):016x}',
                                              'reference': f'{ref:016x}', 'candidate': f'{got:016x}',
                                              'oracle': f'{truth:016x}'})
    finally:
        for value in (x, y, out):
            lib.mpfr_clear(ctypes.byref(value))
    if set(stats) != set(ALL_IDS) or any(entry['records'] != (20018 if ident in (14, 20) else 10009)
                                          for ident, entry in stats.items()):
        raise ValueError('incomplete trace coverage')
    args.output.write_text(json.dumps({'precision_bits': 256, 'functions': stats}, indent=2) + '\n', encoding='utf-8')
    for ident, entry in sorted(stats.items()):
        print(f'id={ident} ' + ' '.join(f'{key}={value}' for key, value in entry.items()))
    return int(any(entry['failure'] for entry in stats.values()))


if __name__ == '__main__':
    raise SystemExit(main())
