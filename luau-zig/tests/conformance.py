#!/usr/bin/env python3
"""Differential consumer checks for the pinned Luau reference and a Zig candidate."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
FIX = HERE / 'fixtures'
# Explicit witnesses prevent two equally broken implementations from passing.
CASES = (
    ('language', ('language.luau',), (), 'language:16:16:6\n'),
    ('userdata-coroutine', ('coroutine_userdata.luau',), (), 'userdata-coroutine:16\n'),
    ('libraries', ('libraries.luau',), (), 'libraries:abcd:-12345:5a\n'),
    ('sandbox', ('sandbox.luau',), (), 'sandbox:restricted:readonly\n'),
    ('control-flow', ('control_flow.luau',), (), 'control:10:5,4,2,1:12\n'),
    ('chunk-carry', ('carry_first.luau', 'carry_second.luau'), ('alpha', 'beta'),
     'carry:first\ncarry:second:23\n'),
    ('gc-sandbox', ('gc_sandbox.luau',), (), 'gc-sandbox:live\n'),
)
BYTECODE_CASES = CASES[:5]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for option in ('reference-runner', 'candidate-runner', 'reference-bytecode', 'candidate-bytecode',
                   'reference-sandbox', 'candidate-sandbox', 'reference-mod', 'candidate-mod',
                   'reference-mod-save', 'candidate-mod-save'):
        p.add_argument('--' + option, help='path to executable; missing prerequisites are SKIP, not PASS')
    p.add_argument('--timeout', type=float, default=30, help='per-process timeout in seconds')
    p.add_argument('--mod-timeout', type=float, default=3600, help='timeout per bundled-mod qualification')
    p.add_argument('--report', type=Path, help='write complete observations and skips as JSON')
    args = p.parse_args()
    if args.timeout <= 0 or args.mod_timeout <= 0:
        p.error('timeouts must be positive')
    records = []
    env = os.environ.copy()
    # Never discover or read the caller's private game assets or saves.
    env.pop('P4PC_ISO', None)
    env.update(P4PC_INPUT='script', P4PC_MODS='0', P4PC_PACKS='0')

    def expected_lines(output):
        # Native Windows C text streams translate each LF to CRLF. Preserve the
        # raw observation and undo only that translation for expected fixtures.
        return output.replace('\r\n', '\n') if sys.platform == 'win32' else output

    def record(name, status, detail='', observations=None):
        item = {'name': name, 'status': status, 'detail': detail}
        if observations is not None:
            item['observations'] = observations
        records.append(item)
        print(f'{status} {name}' + (f': {detail}' if detail else ''), flush=True)
        if status == 'FAIL' and observations is not None:
            print(json.dumps(observations, indent=2), file=sys.stderr, flush=True)

    def invoke(command, timeout):
        command = [str(x) for x in command]
        try:
            result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True,
                                    timeout=timeout)
            return {'command': command, 'exit': result.returncode,
                    'stdout': result.stdout.decode('utf-8', errors='backslashreplace'),
                    'stderr': result.stderr.decode('utf-8', errors='backslashreplace')}
        except subprocess.TimeoutExpired as exc:
            def decode(value):
                if value is None: return ''
                return value.decode('utf-8', errors='backslashreplace') if isinstance(value, bytes) else value
            return {'command': command, 'exit': None, 'stdout': decode(exc.stdout),
                    'stderr': decode(exc.stderr), 'error': f'timed out after {timeout}s'}
        except OSError as exc:
            return {'command': command, 'exit': None, 'stdout': '', 'stderr': '', 'error': str(exc)}

    def missing(name, *paths):
        absent = [label for label, value in paths if not value]
        if absent:
            record(name, 'SKIP', 'missing ' + ', '.join(absent))
            return True
        return False

    runners = (('reference', args.reference_runner), ('candidate', args.candidate_runner))
    for name, files, inputs, expected in CASES:
        if missing('source/' + name, ('--reference-runner', args.reference_runner),
                   ('--candidate-runner', args.candidate_runner)):
            continue
        observations = {}
        problems = []
        for side, runner in runners:
            command = [runner, *(FIX / file for file in files)]
            if inputs: command += ['--', *inputs]
            result = observations[side] = invoke(command, args.timeout)
            if result['exit'] != 0 or expected_lines(result['stdout']) != expected or result['stderr']:
                problems.append(f'{side} did not produce the expected successful observation')
        if observations['reference']['stdout'] != observations['candidate']['stdout']:
            problems.append('stdout differs')
        record('source/' + name, 'FAIL' if problems else 'PASS', '; '.join(problems), observations)

    for name, filename, line, prefix, marker in (
        ('syntax', 'syntax_error.luau', 2, '', None),
        ('runtime', 'runtime_error.luau', 2, 'runtime:before\n', 'conformance-runtime-marker'),
    ):
        if missing('error/' + name, ('--reference-runner', args.reference_runner),
                   ('--candidate-runner', args.candidate_runner)):
            continue
        observations = {}
        problems = []
        for side, runner in runners:
            result = observations[side] = invoke([runner, FIX / filename], args.timeout)
            diag = result['stderr']
            if (result['exit'] != 1 or expected_lines(result['stdout']) != prefix or
                not re.search(re.escape(filename) + ':' + str(line) + r'\b', diag) or
                (marker and marker not in diag)):
                problems.append(f'{side} did not report the expected error and source line')
        record('error/' + name, 'FAIL' if problems else 'PASS', '; '.join(problems), observations)

    adapters = (('reference', args.reference_bytecode), ('candidate', args.candidate_bytecode))
    if not missing('bytecode/prerequisites', ('--reference-bytecode', args.reference_bytecode),
                   ('--candidate-bytecode', args.candidate_bytecode)):
        with tempfile.TemporaryDirectory(prefix='luau-differential-') as directory:
            directory = Path(directory)
            for name, files, _, expected in BYTECODE_CASES:
                artifacts = {}
                for side, adapter in adapters:
                    out = directory / (side + '-' + name + '.luau-bc')
                    result = invoke([adapter, 'compile', FIX / files[0], out], args.timeout)
                    problem = ''
                    if result['exit'] != 0 or result['stdout'] or result['stderr']:
                        problem = 'compilation failed or wrote unexpected output'
                    elif not out.is_file():
                        problem = 'successful compiler did not create bytecode'
                    else:
                        data = out.read_bytes()
                        if not data or data[0] != 13:
                            problem = 'expected default Luau bytecode version 13'
                        else:
                            artifacts[side] = out
                    record(f'bytecode/{name}/compile-{side}', 'FAIL' if problem else 'PASS',
                           problem, {side: result})
                for producer, _ in adapters:
                    for consumer, adapter in adapters:
                        case = f'bytecode/{name}/{producer}-to-{consumer}'
                        if producer not in artifacts:
                            record(case, 'SKIP', f'{producer} compilation failed')
                            continue
                        result = invoke([adapter, 'run', artifacts[producer]], args.timeout)
                        problem = '' if result['exit'] == 0 and expected_lines(result['stdout']) == expected and not result['stderr'] else 'incorrect bytecode execution'
                        record(case, 'FAIL' if problem else 'PASS', problem, {consumer: result})
                if name == 'language':
                    for bad in (2, 15):
                        for side, adapter in adapters:
                            case = f'bytecode/version-{bad}/{side}'
                            if 'reference' not in artifacts:
                                record(case, 'SKIP', 'reference compilation failed')
                                continue
                            data = artifacts['reference'].read_bytes()
                            invalid = directory / f'version-{bad}.bc'
                            invalid.write_bytes(bytes([bad]) + data[1:])
                            result = invoke([adapter, 'run', invalid], args.timeout)
                            problem = '' if result['exit'] == 1 and not result['stdout'] and 'version' in result['stderr'].lower() else 'unsupported bytecode version not rejected'
                            record(case, 'FAIL' if problem else 'PASS', problem, {side: result})
            for side, adapter in adapters:
                out = directory / (side + '-bad.bc')
                result = invoke([adapter, 'compile', FIX / 'syntax_error.luau', out], args.timeout)
                problem = '' if result['exit'] == 1 and not result['stdout'] and result['stderr'] and not out.exists() else 'syntax error accepted or no diagnostic'
                record(f'bytecode/syntax/{side}', 'FAIL' if problem else 'PASS', problem, {side: result})

    for suite, ref, candidate in (
        ('sandbox-host', args.reference_sandbox, args.candidate_sandbox),
        ('mod-host', args.reference_mod, args.candidate_mod),
        ('mod-save-host', args.reference_mod_save, args.candidate_mod_save),
    ):
        if missing(suite, ('reference executable', ref), ('candidate executable', candidate)):
            continue
        with tempfile.TemporaryDirectory(prefix='luau-host-witness-') as directory:
            observations = {}
            bad = []
            for side, program in (('reference', ref), ('candidate', candidate)):
                base = Path(directory) / side
                command = [program] if suite == 'sandbox-host' else [program, base]
                result = observations[side] = invoke(command, args.mod_timeout)
                if result['exit'] != 0:
                    bad.append(side)
                if suite == 'mod-save-host':
                    result = observations[side + '-callback-failure'] = invoke(
                        [program, '--callback-failure', Path(directory) / (side + '-failure')], args.mod_timeout)
                    if result['exit'] != 42:
                        bad.append(side + '-callback-failure')
            record(suite, 'FAIL' if bad else 'PASS', ('failed: ' + ', '.join(bad)) if bad else '', observations)

    for suite, script in (('randomizer', 'mod_randomizer_engine_test.py'),
                          ('skill-affinity', 'mod_skill_affinity_test.py')):
        name = 'real-mod/' + suite
        driver = ROOT / 'tests' / script
        mod = ROOT / 'mods' / ('p4pc.' + suite) / 'main.luau'
        if missing(name, ('--reference-runner', args.reference_runner), ('--candidate-runner', args.candidate_runner),
                   (str(driver), driver.is_file()), (str(mod), mod.is_file())):
            continue
        observations = {side: invoke([sys.executable, driver, '--', runner], args.mod_timeout)
                        for side, runner in runners}
        bad = [side for side, result in observations.items() if result['exit'] != 0]
        record(name, 'FAIL' if bad else 'PASS', ('qualification failed: ' + ', '.join(bad)) if bad else '', observations)

    counts = {status: sum(r['status'] == status for r in records) for status in ('PASS', 'FAIL', 'SKIP')}
    print('SUMMARY ' + ' '.join(f'{k}={v}' for k, v in counts.items()), flush=True)
    if args.report:
        args.report.write_text(json.dumps({'summary': counts, 'records': records}, indent=2) + '\n', encoding='utf-8')
    return 1 if counts['FAIL'] else 77 if counts['SKIP'] else 0


if __name__ == '__main__':
    sys.exit(main())
