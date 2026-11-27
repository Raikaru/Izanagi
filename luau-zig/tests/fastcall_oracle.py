#!/usr/bin/env python3
"""Classify dense FASTCALL samples against a 256-bit MPFR oracle."""
import argparse
import ctypes
import ctypes.util
import json
import math
import os
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET

IDS = {3: 'acos', 4: 'asin', 5: 'atan2', 6: 'atan', 8: 'cosh', 9: 'cos',
       11: 'exp', 16: 'log10', 17: 'log', 21: 'pow', 23: 'sinh',
       24: 'sin', 26: 'tanh', 27: 'tan'}
PAIR = re.compile(r'number:([0-9a-f]{16})=number:([0-9a-f]{16})')
MASK = (1 << 64) - 1


class MPFR(ctypes.Structure):
    _fields_ = [('precision', ctypes.c_long), ('sign', ctypes.c_int),
                ('exponent', ctypes.c_long), ('limbs', ctypes.POINTER(ctypes.c_ulong))]


def bits(value):
    return struct.unpack('>Q', struct.pack('>d', value))[0]


def number(value):
    return struct.unpack('>d', value.to_bytes(8, 'big'))[0]


def ordered(value):
    return (~value & MASK) if value >> 63 else value | (1 << 63)


def distance(a, b):
    if a == b:
        return 0
    x, y = number(a), number(b)
    if not math.isfinite(x) or not math.isfinite(y) or (x == 0 and y == 0):
        return None
    return abs(ordered(a) - ordered(b))


def values(path):
    output = json.loads(path.read_text(encoding='utf-8'))
    if output['exit'] != 0 or output['stderr']:
        raise ValueError('probe did not complete: ' + str(path))
    lines = output['stdout'].splitlines()
    if len(lines) != 4 or not lines[0].startswith('interrupts=') or lines[1:3] != ['status=0', 'results=1'] or not lines[3].startswith('result=table:{') or not lines[3].endswith('}'):
        raise ValueError('unexpected dense result structure: ' + str(path))
    body = lines[3][len('result=table:{'):-1]
    if PAIR.sub('', body).replace(';', ''):
        raise ValueError('nonnumeric dense result or side effect: ' + str(path))
    pairs = PAIR.findall(body)
    result = {int(number(int(key, 16))): int(value, 16) for key, value in pairs}
    if len(pairs) != 10000 or set(result) != set(range(1, 10001)):
        raise ValueError('missing or duplicate dense results: ' + str(path))
    return result


def inputs(path):
    setup = path.read_text(encoding='utf-8')
    arrays = []
    for name in ('xs', 'ys'):
        match = re.search(r'(?m)^' + name + r'=\{([^}]*)\}$', setup)
        if not match:
            raise ValueError('missing input array: ' + name)
        array = [float(value) for value in match.group(1).split(',')]
        if len(array) != 10000:
            raise ValueError('incomplete input array: ' + name)
        arrays.append(array)
    return arrays


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fastcall-dir', required=True, type=Path)
    parser.add_argument('--output-dir', required=True, type=Path)
    parser.add_argument('--reconcile', action='store_true', help='reclassify only dense glibc-distance failures using MPFR')
    args = parser.parse_args()
    libname = os.environ.get('P4PC_MPFR_LIBRARY') or ctypes.util.find_library('mpfr')
    if not libname:
        raise RuntimeError('MPFR shared library is required for the independent oracle')
    lib = ctypes.CDLL(libname)
    ptr = ctypes.POINTER(MPFR)
    lib.mpfr_init2.argtypes = [ptr, ctypes.c_long]
    lib.mpfr_set_d.argtypes = [ptr, ctypes.c_double, ctypes.c_int]
    lib.mpfr_get_d.argtypes = [ptr, ctypes.c_int]
    lib.mpfr_get_d.restype = ctypes.c_double
    lib.mpfr_clear.argtypes = [ptr]
    for name in set(IDS.values()):
        function = getattr(lib, 'mpfr_' + name)
        function.argtypes = [ptr, ptr, ptr, ctypes.c_int] if name in ('atan2', 'pow') else [ptr, ptr, ctypes.c_int]
    x, y, out = MPFR(), MPFR(), MPFR()
    for value in (x, y, out):
        lib.mpfr_init2(ctypes.byref(value), 256)
    report = []
    try:
        for ident, name in IDS.items():
            directory = args.fastcall_dir / (f'{ident:03d}-dense-random')
            xs, ys = inputs(directory / 'setup.luau')
            reference = {mode: values(directory / f'reference-safe{mode}.json') for mode in (0, 1)}
            candidate = {mode: values(directory / f'candidate-safe{mode}.json') for mode in (0, 1)}
            counts = {'exact_oracle': 0, 'one_ulp_oracle': 0, 'glibc_divergence': 0, 'glibc_closer': 0, 'failure': 0}
            examples = {'glibc_divergence': None, 'failure': None}
            maximum = 0
            fn = getattr(lib, 'mpfr_' + name)
            for index, (a, b) in enumerate(zip(xs, ys), 1):
                first, second = (b, a) if ident == 21 else ((b, 0) if ident in (16, 17) else (a, b))
                lib.mpfr_set_d(ctypes.byref(x), first, 0)
                lib.mpfr_set_d(ctypes.byref(y), second, 0)
                if name in ('atan2', 'pow'):
                    fn(ctypes.byref(out), ctypes.byref(x), ctypes.byref(y), 0)
                else:
                    fn(ctypes.byref(out), ctypes.byref(x), 0)
                truth = bits(lib.mpfr_get_d(ctypes.byref(out), 0))
                for mode in (0, 1):
                    ref, got = reference[mode][index], candidate[mode][index]
                    candidate_distance, reference_distance = distance(got, truth), distance(ref, truth)
                    example = {'index': index, 'safe': mode, 'x': f'{bits(first):016x}',
                               'y': f'{bits(second):016x}', 'reference': f'{ref:016x}',
                               'candidate': f'{got:016x}', 'oracle': f'{truth:016x}'}
                    if got != ref:
                        counts['glibc_divergence'] += 1
                        if examples['glibc_divergence'] is None:
                            examples['glibc_divergence'] = example
                    if candidate_distance is not None and reference_distance is not None and reference_distance < candidate_distance:
                        counts['glibc_closer'] += 1
                    if candidate_distance is None or reference_distance is None or candidate_distance > 1:
                        counts['failure'] += 1
                        if examples['failure'] is None:
                            examples['failure'] = example
                    elif candidate_distance == 0:
                        counts['exact_oracle'] += 1
                    else:
                        counts['one_ulp_oracle'] += 1
                    if candidate_distance is not None:
                        maximum = max(maximum, candidate_distance)
            report.append({'id': ident, 'function': name, 'samples_per_mode': 10000,
                           'counts': counts, 'max_oracle_ulp': maximum, 'examples': examples})
    finally:
        for value in (x, y, out):
            lib.mpfr_clear(ctypes.byref(value))
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / 'oracle.json').write_text(json.dumps({'precision_bits': 256, 'functions': report}, indent=2) + '\n', encoding='utf-8')
    lines = ['| ID | Function | Exact oracle | 1 ULP oracle | glibc divergences | glibc closer | Failures | Max ULP | Example |',
             '| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |']
    for item in report:
        c = item['counts']
        example = item['examples']['failure'] or item['examples']['glibc_divergence']
        lines.append(f"| {item['id']} | {item['function']} | {c['exact_oracle']} | {c['one_ulp_oracle']} | {c['glibc_divergence']} | {c['glibc_closer']} | {c['failure']} | {item['max_oracle_ulp']} | {json.dumps(example, sort_keys=True) if example else '-'} |")
    (args.output_dir / 'oracle.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print('\n'.join(lines))
    oracle_failed = any(item['counts']['failure'] for item in report)
    if args.reconcile:
        original = json.loads((args.fastcall_dir / 'results.json').read_text(encoding='utf-8'))
        by_id = {item['id']: item for item in report}
        records = []
        for source in original['records']:
            record = dict(source)
            record['initial_status'] = source['status']
            record['glibc_only_errors'] = []
            if record['name'] == 'dense-random' and record['id'] in by_id and not by_id[record['id']]['counts']['failure']:
                allowed = {f'safe={mode}: reference and candidate results/errors/side effects differ' for mode in (0, 1)}
                record['glibc_only_errors'] = [error for error in record['errors'] if error in allowed]
                record['errors'] = [error for error in record['errors'] if error not in allowed]
                record['status'] = 'FAIL' if record['errors'] else 'PASS'
            records.append(record)
        summary = {'PASS': sum(record['status'] == 'PASS' for record in records),
                   'FAIL': sum(record['status'] == 'FAIL' for record in records), 'SKIP': 0}
        (args.output_dir / 'reconciled.json').write_text(json.dumps({'summary': summary, 'records': records}, indent=2) + '\n', encoding='utf-8')
        suite = ET.Element('testsuite', name='fastcall-oracle-reconciled', tests=str(len(records)),
                           failures=str(summary['FAIL']), skipped='0')
        for record in records:
            case = ET.SubElement(suite, 'testcase', classname='builtin-' + str(record['id']), name=record['name'])
            if record['errors']:
                ET.SubElement(case, 'failure', message='; '.join(record['errors']))
        ET.ElementTree(suite).write(args.output_dir / 'reconciled.xml', encoding='utf-8', xml_declaration=True)
        print('RECONCILED', json.dumps(summary))
        return int(oracle_failed or summary['FAIL'] != 0)
    return int(oracle_failed)


if __name__ == '__main__':
    raise SystemExit(main())
