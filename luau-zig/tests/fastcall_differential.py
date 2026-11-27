#!/usr/bin/env python3
"""Compare every pinned Luau FASTCALL fast and fallback path on two VMs."""
import argparse
import json
import math
import random
from pathlib import Path
import re
import struct
import subprocess
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
TRANSCENDENTAL_IDS = {3, 4, 5, 6, 8, 9, 11, 16, 17, 21, 23, 24, 26, 27}
NUMBER = re.compile(r'number:([0-9a-f]{16})')
TABLE_VALUE_NUMBER = re.compile(r'(?<==)number:([0-9a-f]{16})')
DENSE_EXPRESSIONS = {
    2: 'math.abs(x)', 3: 'math.acos(x)', 4: 'math.asin(x)',
    5: 'math.atan2(x, y)', 6: 'math.atan(x)', 7: 'math.ceil(x)',
    8: 'math.cosh(x)', 9: 'math.cos(x)', 10: 'math.deg(x)',
    11: 'math.exp(x)', 12: 'math.floor(x)', 13: 'math.fmod(x, y)',
    14: 'math.frexp(x)', 15: 'math.ldexp(x, 3)', 16: 'math.log10(y)',
    17: 'math.log(y)', 18: 'math.max(x, y)', 19: 'math.min(x, y)',
    20: 'math.modf(x)', 21: 'math.pow(y, x)', 22: 'math.rad(x)',
    23: 'math.sinh(x)', 24: 'math.sin(x)', 25: 'math.sqrt(y)',
    26: 'math.tanh(x)', 27: 'math.tan(x)',
    28: 'bit32.arshift(a, 3)', 29: 'bit32.band(a, b)',
    30: 'bit32.bnot(a)', 31: 'bit32.bor(a, b)', 32: 'bit32.bxor(a, b)',
    33: 'bit32.btest(a, b)', 34: 'bit32.extract(a, c, 3)',
    35: 'bit32.lrotate(a, 3)', 36: 'bit32.lshift(a, 3)',
    37: 'bit32.replace(a, b, 3, 4)', 38: 'bit32.rrotate(a, 3)',
    39: 'bit32.rshift(a, 3)', 46: 'math.clamp(x, -1, 1)',
    47: 'math.sign(x)', 48: 'math.round(x)',
    55: 'bit32.countlz(a)', 56: 'bit32.countrz(a)',
    59: 'bit32.extract(a, 3, 4)', 64: 'bit32.byteswap(a)',
    89: 'math.lerp(x, y, 0.25)', 91: 'math.isnan(x)',
    92: 'math.isinf(x)', 93: 'math.isfinite(x)',
}


def dense_case(id, count=10000):
    """One dynamic call site, exercised on a deterministic dense input set."""
    rng = random.Random(0x4C554155 + id)
    xs = [rng.uniform(-0.99, 0.99) for _ in range(count)]
    ys = [rng.uniform(0.01, 0.99) for _ in range(count)]
    aa = [rng.randrange(0, 1 << 32) for _ in range(count)]
    bb = [rng.randrange(0, 1 << 32) for _ in range(count)]
    cc = [rng.randrange(0, 29) for _ in range(count)]
    def array(name, values):
        return name + '={' + ','.join(repr(value) for value in values) + '}\n'
    setup = ''.join((array('xs', xs), array('ys', ys), array('aa', aa),
                     array('bb', bb), array('cc', cc)))
    source = ('local out={}\nfor i=1,#xs do\n'
              'local x,y,a,b,c=xs[i],ys[i],aa[i],bb[i],cc[i]\n'
              'out[i]=' + DENSE_EXPRESSIONS[id] + '\nend\nreturn out')
    return {'id': id, 'path': 'fast', 'name': 'dense-random',
            'setup': setup, 'source': source, 'observe': [], 'compiler_flags': [],
            'iterations': count}


def ulp_distance(a, b):
    if a == b:
        return 0
    x = struct.unpack('>d', a.to_bytes(8, 'big'))[0]
    y = struct.unpack('>d', b.to_bytes(8, 'big'))[0]
    if x == 0 and y == 0:
        return None  # Signed zero is observable through reciprocal and formatting.
    if not math.isfinite(x) or not math.isfinite(y):
        return None
    def ordered(bits):
        return (~bits & ((1 << 64) - 1)) if bits >> 63 else (bits | (1 << 63))
    return abs(ordered(a) - ordered(b))


def compare_output(left, right, tolerant, classes=None):
    """Compare all nonnumeric output exactly and classify numeric divergences."""
    if classes is None:
        classes = {'exact': 0, 'finite_ulp': 0, 'nan_divergence': 0, 'failure': 0}
    if left == right:
        classes['exact'] += 1
        return True, 0
    if not tolerant:
        classes['failure'] += 1
        return False, 0
    left_lines, right_lines = left.splitlines(), right.splitlines()
    if len(left_lines) != len(right_lines):
        classes['failure'] += 1
        return False, 0
    maximum = 0
    for left_line, right_line in zip(left_lines, right_lines):
        if left_line == right_line:
            continue
        if left_line.startswith('result=number:') and right_line.startswith('result=number:'):
            a, b = NUMBER.fullmatch(left_line[7:]), NUMBER.fullmatch(right_line[7:])
            if a is None or b is None:
                classes['failure'] += 1
                return False, maximum
            pairs = [(a.group(1), b.group(1))]
        elif left_line.startswith('result=table:{') and right_line.startswith('result=table:{'):
            if TABLE_VALUE_NUMBER.sub('number:<value>', left_line) != TABLE_VALUE_NUMBER.sub('number:<value>', right_line):
                classes['failure'] += 1
                return False, maximum
            pairs = list(zip(TABLE_VALUE_NUMBER.findall(left_line), TABLE_VALUE_NUMBER.findall(right_line)))
            if not pairs:
                classes['failure'] += 1
                return False, maximum
        else:
            classes['failure'] += 1
            return False, maximum
        for a, b in pairs:
            ai, bi = int(a, 16), int(b, 16)
            if ai == bi:
                classes['exact'] += 1
                continue
            av = struct.unpack('>d', ai.to_bytes(8, 'big'))[0]
            bv = struct.unpack('>d', bi.to_bytes(8, 'big'))[0]
            if math.isnan(av) and math.isnan(bv):
                classes['nan_divergence'] += 1
                continue
            distance = ulp_distance(ai, bi)
            if distance is None:
                classes['failure'] += 1
                return False, maximum
            maximum = max(maximum, distance)
            if distance <= 1:
                classes['finite_ulp'] += 1
            else:
                classes['failure'] += 1
    return maximum <= 1, maximum


def invoke(command, directory, label, timeout):
    try:
        completed = subprocess.run([str(item) for item in command], capture_output=True, timeout=timeout)
        result = {'command': [str(item) for item in command], 'exit': completed.returncode,
                  'stdout': completed.stdout.decode('utf-8', 'backslashreplace'),
                  'stderr': completed.stderr.decode('utf-8', 'backslashreplace')}
    except (OSError, subprocess.TimeoutExpired) as error:
        result = {'command': [str(item) for item in command], 'exit': None,
                  'stdout': '', 'stderr': '', 'error': str(error)}
    (directory / (label + '.json')).write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


def parse_probe(result):
    if result['exit'] != 0 or result['stderr']:
        raise ValueError('probe failed: ' + (result.get('error') or result['stderr'] or str(result['exit'])))
    lines = result['stdout'].splitlines()
    counts = [line for line in lines if line.startswith('interrupts=')]
    if len(counts) != 1 or not counts[0][11:].isdigit():
        raise ValueError('missing or invalid interrupt count')
    return int(counts[0][11:]), '\n'.join(line for line in lines if not line.startswith('interrupts='))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--builtin', required=True, help='numeric builtin ID or all')
    parser.add_argument('--reference', required=True, type=Path)
    parser.add_argument('--candidate', required=True, type=Path)
    parser.add_argument('--graph', required=True, type=Path)
    parser.add_argument('--output-dir', required=True, type=Path)
    parser.add_argument('--timeout', type=float, default=120)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error('--timeout must be positive')
    if args.output_dir.exists() and any(args.output_dir.iterdir()):
        parser.error('--output-dir must be empty')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    if args.builtin == 'all':
        selected = set(range(1, 135))
    else:
        try:
            selected = {int(args.builtin)}
        except ValueError:
            parser.error('--builtin must be all or an ID in 1..134')
        if not selected <= set(range(1, 135)):
            parser.error('--builtin must be all or an ID in 1..134')

    cases = []
    for family in ('math', 'misc', 'vector'):
        document = json.loads((HERE / 'fastcall-vectors' / (family + '.json')).read_text(encoding='utf-8'))
        if document.get('version') != 1 or not isinstance(document.get('cases'), list):
            raise ValueError('invalid vector document: ' + family)
        cases.extend(case for case in document['cases'] if case.get('id') in selected)
    for original in list(cases):
        if 94 <= original['id'] <= 130:
            default = dict(original)
            default['name'] += '-shipped-default'
            default['path'] = 'fallback'
            default['integer_default'] = True
            default['compiler_flags'] = [flag for flag in original['compiler_flags']
                                         if flag not in ('LuauIntegerLibrary', 'LuauIntegerFastcalls')]
            default['setup'] = ''
            cases.append(default)
    cases.extend(dense_case(id) for id in sorted(selected & DENSE_EXPRESSIONS.keys()))
    names = set()
    records = []
    coverage = {id: set() for id in selected}
    maximum_ulp = {id: 0 for id in selected if id in TRANSCENDENTAL_IDS}
    for case in cases:
        id = case.get('id')
        path = case.get('path')
        name = case.get('name')
        if path not in ('fast', 'fallback') or not isinstance(name, str) or not re.fullmatch(r'[A-Za-z0-9_.-]+', name):
            raise ValueError('invalid vector identity: ' + repr(case))
        identity = (id, name)
        if identity in names:
            raise ValueError('duplicate vector identity: ' + repr(identity))
        names.add(identity)
        setup, source = case.get('setup'), case.get('source')
        observe, flags = case.get('observe'), case.get('compiler_flags')
        if not isinstance(setup, str) or not isinstance(source, str) or not isinstance(observe, list) or not isinstance(flags, list):
            raise ValueError('invalid vector fields: ' + repr(identity))
        if not all(isinstance(item, str) and re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', item) for item in observe + flags):
            raise ValueError('invalid global or flag name: ' + repr(identity))
        coverage[id].add(path)
        directory = args.output_dir / (f'{id:03d}-' + name)
        directory.mkdir()
        setup_path = directory / 'setup.luau'
        source_path = directory / 'case.luau'
        setup_path.write_text(setup, encoding='utf-8')
        source_path.write_text(source, encoding='utf-8')
        probes = {}
        errors = []
        for side, binary in (('reference', args.reference), ('candidate', args.candidate)):
            for safe in (0, 1):
                label = f'{side}-safe{safe}'
                artifact = directory / (label + '.bytecode')
                result = invoke([binary, setup_path, source_path, artifact, safe, id,
                                 ','.join(flags), ','.join(observe)], directory, label, args.timeout)
                try:
                    probes[(side, safe)] = parse_probe(result)
                except ValueError as error:
                    errors.append(label + ': ' + str(error))
                if safe == 1:
                    if not artifact.is_file() or not artifact.stat().st_size:
                        errors.append(label + ': missing bytecode')
                    else:
                        requirement = '--forbid-fastcall-id' if case.get('integer_default') else '--require-fastcall-id'
                        graph = invoke([args.graph, artifact, requirement, id], directory,
                                       label + '-graph', args.timeout)
                        if graph['exit'] != 0 or graph['stdout'] != 'OK\n' or graph['stderr']:
                            errors.append(label + ': target FASTCALL proof failed: ' + graph['stderr'])
        max_ulp = 0
        classes = {'exact': 0, 'finite_ulp': 0, 'nan_divergence': 0, 'failure': 0}
        if len(probes) == 4:
            for side in ('reference', 'candidate'):
                unsafe_count, unsafe_output = probes[(side, 0)]
                safe_count, safe_output = probes[(side, 1)]
                if case.get('integer_default'):
                    lines = safe_output.splitlines()
                    if 'status=2' not in lines or 'results=1' not in lines or not any(line.startswith('result=string:') for line in lines):
                        errors.append(side + ': disabled integer library did not produce a runtime error')
                if side == 'candidate' and unsafe_output != safe_output:
                    errors.append('candidate: fast and fallback results/errors/side effects differ')
                expected_delta = case.get('iterations', 1) if path == 'fast' else 0
                if unsafe_count - safe_count != expected_delta:
                    errors.append(f'{side}: expected interrupt delta {expected_delta}, got {unsafe_count - safe_count}')
            for safe in (0, 1):
                left_count, left = probes[('reference', safe)]
                right_count, right = probes[('candidate', safe)]
                if left_count != right_count:
                    errors.append(f'safe={safe}: interrupt counts differ ({left_count} != {right_count})')
                equal, observed = compare_output(left, right, id in TRANSCENDENTAL_IDS, classes)
                max_ulp = max(max_ulp, observed)
                if not equal:
                    errors.append(f'safe={safe}: reference and candidate results/errors/side effects differ')
        else:
            errors.append('incomplete probe set')
        if id in maximum_ulp:
            maximum_ulp[id] = max(maximum_ulp[id], max_ulp)
        record = {'id': id, 'name': name, 'path': path, 'status': 'FAIL' if errors else 'PASS',
                  'maximum_ulp': max_ulp, 'comparison_classes': classes, 'errors': errors}
        records.append(record)
        print(f"{record['status']} {id}/{name}: " + ('; '.join(errors) if errors else 'matched'), flush=True)
    for id, paths in sorted(coverage.items()):
        for missing in {'fast', 'fallback'} - paths:
            record = {'id': id, 'name': 'missing-' + missing, 'path': missing, 'status': 'FAIL',
                      'maximum_ulp': 0, 'errors': ['required vector absent']}
            records.append(record)
            print(f'FAIL {id}/missing-{missing}: required vector absent', flush=True)
    report = {'reference_pin': '32c28089305f488f901aaf1dbf3fd1cb65fa3d11', 'records': records,
              'maximum_ulp_by_id': maximum_ulp,
              'summary': {'PASS': sum(r['status'] == 'PASS' for r in records),
                          'FAIL': sum(r['status'] == 'FAIL' for r in records), 'SKIP': 0}}
    (args.output_dir / 'results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    suite = ET.Element('testsuite', name='fastcall-differential', tests=str(len(records)),
                       failures=str(report['summary']['FAIL']), skipped='0')
    for result in records:
        test = ET.SubElement(suite, 'testcase', classname='builtin-' + str(result['id']), name=result['name'])
        if result['errors']:
            failure = ET.SubElement(test, 'failure', message='; '.join(result['errors']))
            failure.text = '\n'.join(result['errors'])
    ET.ElementTree(suite).write(args.output_dir / 'results.xml', encoding='utf-8', xml_declaration=True)
    print('SUMMARY', json.dumps(report['summary']), flush=True)
    return 1 if report['summary']['FAIL'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
