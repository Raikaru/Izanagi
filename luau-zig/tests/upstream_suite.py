#!/usr/bin/env python3
"""Run and account for the pinned upstream Luau.Conformance target and Zig adapters."""
import ast
import argparse
import json
import os
import re
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

from generate_upstream_api import (API_COMPILER_OPTIMIZATION, CASES as API_CASES,
    CLASS_API_CASE, FLAGGED_CASES, LIBRARY_FLAGGED_CASES, PARTIAL_CASES,
    PARTIAL_COMPILER_OPTIMIZATION)
from upstream_inventory import UPSTREAM, inventory

ROOT = Path(__file__).resolve().parent.parent.parent
# These cases only exercise codegen/JIT or return immediately unless native
# codegen is enabled and supported. JitInliner explicitly enables the JIT.
NATIVE_ONLY_CASES = frozenset({
    'CodegenSupported', 'JitInliner', 'Native', 'NativeTypeAnnotations', 'NativeAttribute', 'IrInstructionLimit',
    'CodegenNopPaddingDeterministicOff', 'CodegenRandomizeCodeSizeNonDecreasing',
    'CodegenRandomizeFunctionalCorrectness',
})

def native_only(case):
    # Every IrLowering test inspects CodeGen getCodegen* output; the
    # SharedCodeAllocator suite tests executable native-code allocation.
    return (case['suite'] in {'IrLowering', 'SharedCodeAllocator'} or
            (case['suite'] == 'Conformance' and case['name'] in NATIVE_ONLY_CASES))


MULTI_OPT_API_CASES = ('ApiCalls', 'Debugger', 'InterruptInspection', 'TagMethodError',
                       'UserdataDirectAccess', 'HugeFunction', 'ExportEdgeCase')
DEFAULT_COMPILER_CASES = frozenset({'InterruptErrorInspection', 'HugeFunction', 'HugeFunctionLoadFailure',
    'HugeConstantTable', 'LargeNestedClosure', 'ExportEdgeCase', 'ClassInheritanceRepeatedCallMemberOffsetCorruption'})
SCRIPT_HOST_CASES = frozenset({'Buffers', 'BuffersWithCage', 'GC', 'Tables', 'PCall', 'Coroutine',
    'CoroutineHost', 'CYield', 'Iter', 'Userdata', 'Vector', 'VectorLibrary', 'Bitwise', 'Coverage', 'IfLocal', 'Classes', 'Integers', 'NativeIntegerSpills', 'Types'})
# Audited pinned cases whose interpreter-specific setup and flags are present
# in upstream_script_host; native hook/CodeGen variants remain separate N/A.
COMPLETE_SCRIPT_CASES = frozenset({'Buffers', 'BuffersWithCage', 'Bitwise', 'Coroutine',
                                   'CoroutineHost', 'CYield', 'GC', 'PCall', 'Tables',
                                   'Userdata', 'Vector', 'VectorLibrary', 'NativeIntegerSpills',
                                   'Coverage', 'IfLocal', 'Classes', 'Iter', 'Types', 'Integers'})
API_GRAPH_CASES = PARTIAL_CASES - {'ExceptionObject', 'LargeModuleA64', 'UserdataAlignment'}
SCRIPT_FLAGS = {
    'BuffersWithCage': {'LuauBufferCage'},
    'Tables': {'LuauTableRobustOom'},
    'Coroutine': {'LuauCoroutineFinally'},
    'CoroutineHost': {'LuauCoroutineFinally'},
    'CYield': {'LuauPcallErrorContinuationReentry'},
    'IfLocal': {'LuauExperimentalIfLocalSyntax'},
    'Vector': {'LuauCompileNoFoldVectorEqW'},
    'Integers': {'LuauIntegerType2', 'LuauIntegerLibrary'},
    'NativeIntegerSpills': {'LuauIntegerType2'},
    'Types': {'LuauIntegerType2', 'LuauCoroutineFinallyAnalysis', 'LuauCoroutineFinally',
              'DebugLuauForceOldSolver'},
    'Classes': {'DebugLuauUserDefinedClasses', 'DebugLuauUserDefinedClassesRuntime',
                'LuauEmitCallFeedback', 'LuauBytecodeCostModel'},
}

def api_host_optimization(case):
    """The generated host executes exactly this compiler configuration, or no compiler."""
    if case['suite'] == 'DirectFieldAccess':
        return 1
    if case['name'] in PARTIAL_CASES:
        return PARTIAL_COMPILER_OPTIMIZATION.get(case['name'])
    if case['name'] in API_CASES:
        return API_COMPILER_OPTIMIZATION.get(case['name'])
    raise ValueError('not an adapted API case')

def fixture_result(left, right):
    """Do not attribute a failed reference setup to the candidate."""
    if left['exit'] == 77:
        return 'SKIP', 'reference host reported an unsupported prerequisite'
    if left['exit'] != 0 or left['stdout'] != 'OK\n' or left['stderr']:
        return 'FAIL', 'reference fixture host failed: ' + (left['stderr'].splitlines() or [f"exit {left['exit']}"])[0]
    if right['exit'] == 77:
        return 'SKIP', 'candidate host reported an unsupported prerequisite'
    if right['exit'] != 0 or right['stdout'] != 'OK\n' or right['stderr']:
        return 'FAIL', 'candidate fixture execution failed: ' + (right['stderr'].splitlines() or [f"exit {right['exit']}"])[0]
    return 'PASS', 'fixture body only; original host assertions are accounted separately'

def gc_semantic_result(reference, left, right):
    """A skipped witness cannot hide a failure or establish semantic parity."""
    if (reference is not None and not reference['pass']) or any(x not in (None, 0, 77) for x in (left, right)):
        return 'FAIL'
    if reference is None or left in (None, 77) or right in (None, 77):
        return 'SKIP'
    return 'PASS'

def alignment_result(reference, left, right):
    """Promoted alignment assertions must all run on both real allocators."""
    if any(result is not None and not result['pass'] for result in (reference, left, right)):
        return 'FAIL'
    if any(result is None for result in (reference, left, right)):
        return 'SKIP'
    count = left['assert_successes'] + left['assert_failures']
    candidate_count = right['assert_successes'] + right['assert_failures']
    return 'PASS' if count > 0 and count == candidate_count else 'FAIL'

def combined_status(statuses):
    """Aggregate only observed passes, retaining failures over missing outcomes."""
    return ('FAIL' if 'FAIL' in statuses else
            'PASS' if statuses and all(status == 'PASS' for status in statuses) else 'SKIP')

def partial_case_result(reference, left, right, left_graph, right_graph, gc_status):
    """All adapted assertions, compiler graphs and semantic GC checks are required."""
    if (any(result is not None and not result['pass'] for result in (reference, left, right))
            or any(graph is not None and graph[0] == 'FAIL' for graph in (left_graph, right_graph))
            or gc_status == 'FAIL'):
        return 'FAIL'
    if any(result is None for result in (reference, left, right, left_graph, right_graph)):
        return combined_status([gc_status, 'SKIP'])
    left_count = left['assert_successes'] + left['assert_failures']
    right_count = right['assert_successes'] + right['assert_failures']
    if not left_count or left_count != right_count or left_graph[1] != right_graph[1]:
        return 'FAIL'
    return combined_status([left_graph[0], right_graph[0], gc_status])

def subcase_observed(result, path):
    """Only a reported path in an executed doctest case counts as observed."""
    if result is None or not result.get('xml'):
        return False
    try:
        nodes = [ET.fromstring(result['xml'])]
    except ET.ParseError:
        return False
    for name in path:
        nodes = [child for node in nodes for child in node.findall('SubCase')
                 if child.get('name', '') == name]
        if not nodes:
            return False
    return bool(nodes)

def inliner_fixture(source):
    """Reconstruct only the pinned upstream source-builder statements."""
    start = source.index('    std::string source;') + len('    std::string source;')
    end = source.index('    bool wasEnabled = jitInliner;', start)
    body = source[start:end]
    statement = re.compile(r'\s*(?:for \(int i = 0; i < (\d+); i\+\+\)\s*)?'
                           r'source \+= ("(?:[^"\\]|\\.)*");')
    cursor = 0
    parts = []
    for match in statement.finditer(body):
        if body[cursor:match.start()].strip():
            raise ValueError('unexpected upstream inliner source-builder statement')
        count = int(match.group(1) or '1')
        parts.append(ast.literal_eval(match.group(2)) * count)
        cursor = match.end()
    if body[cursor:].strip() or not parts:
        raise ValueError('incomplete upstream inliner source-builder')
    return ''.join(parts)






def invoke(command, timeout, output, label, optimization=None, bytecode_prefix=None):
    command = [str(x) for x in command]
    env = os.environ.copy()
    env.pop('P4PC_ISO', None)
    env.pop('LUAU_TEST_OPTIMIZATION', None)
    env.pop('LUAU_API_BYTECODE_PREFIX', None)
    if bytecode_prefix is not None:
        env['LUAU_API_BYTECODE_PREFIX'] = str(bytecode_prefix)
    env['LUAU_TEST_FIXTURES_DIR'] = str(UPSTREAM / 'tests' / 'conformance')
    if optimization is not None:
        env['LUAU_TEST_OPTIMIZATION'] = str(optimization)
    try:
        run = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, timeout=timeout)
        result = {'command': command, 'exit': run.returncode,
                  'stdout': run.stdout.decode('utf-8', 'backslashreplace'),
                  'stderr': run.stderr.decode('utf-8', 'backslashreplace')}
    except subprocess.TimeoutExpired as exc:
        def decode(s):
            return s.decode('utf-8', 'backslashreplace') if isinstance(s, bytes) else (s or '')
        result = {'command': command, 'exit': None, 'stdout': decode(exc.stdout),
                  'stderr': decode(exc.stderr), 'error': f'timed out after {timeout}s'}
    except OSError as exc:
        result = {'command': command, 'exit': None, 'stdout': '', 'stderr': '', 'error': str(exc)}
    (output / (label + '.process.json')).write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


def xml_results(path):
    root = ET.parse(path).getroot()
    results = {}
    seen = {}
    skipped_count = 0
    for suite in root.findall('.//TestSuite'):
        for case in suite.findall('TestCase'):
            filename = case.get('filename')
            if not filename:
                raise ValueError(f'missing doctest filename for {case.attrib}')
            # XML can originate on a different OS from this driver. pathlib's
            # host-specific basename would not split the other OS's separator.
            basename = filename.replace('\\', '/').rsplit('/', 1)[-1]
            if not basename:
                raise ValueError(f'invalid doctest filename: {filename!r}')
            suite_name, case_name = suite.get('name'), case.get('name')
            if suite_name is None or case_name is None:
                raise ValueError(f'missing doctest suite or case name for {case.attrib}')
            key = (basename, int(case.get('line', '')), suite_name, case_name)
            if key in seen:
                raise ValueError(f'duplicate doctest identity after filename normalization: {key}; '
                                 f'paths {seen[key]!r} and {filename!r}')
            seen[key] = filename
            skipped = case.get('skipped')
            if skipped not in (None, 'true', 'false'):
                raise ValueError(f'invalid doctest skipped attribute for {key}: {skipped!r}')
            if skipped == 'true':
                if len(case):
                    raise ValueError(f'skipped doctest case has result content for {key}')
                skipped_count += 1
                continue
            end = case.find('OverallResultsAsserts')
            if end is None:
                raise ValueError(f'missing result for {case.attrib}')
            success = end.get('test_case_success')
            if success not in ('true', 'false'):
                raise ValueError(f'missing or invalid test-case outcome for {key}')
            results[key] = {'pass': success == 'true',
                            'filename': filename,
                            'assert_successes': int(end.get('successes', '0')),
                            'assert_failures': int(end.get('failures', '0')),
                            'xml': ET.tostring(case, encoding='unicode')}
    if not results:
        raise ValueError(f'no test cases in {path}')
    summary = root.find('OverallResultsTestCases')
    if summary is None:
        raise ValueError(f'missing doctest test-case summary in {path}')
    successes = int(summary.get('successes', ''))
    failures = int(summary.get('failures', ''))
    skipped_summary = summary.get('skipped')
    if skipped_summary is not None and int(skipped_summary) != skipped_count:
        raise ValueError(f'doctest skipped summary does not match reported cases in {path}')
    if successes + failures != len(results) or failures != sum(not result['pass'] for result in results.values()):
        raise ValueError(f'doctest summary does not match reported cases in {path}')
    return results


def scoped_xml_results(path, expected):
    results = xml_results(path)
    if set(results) != set(expected):
        raise ValueError(f'case identities differ: expected={expected}, actual={list(results)}')
    return results


def reason(case):
    if case['name'] == 'ExceptionObject':
        return 'requires !LUA_USE_LONGJMP and uncaught C++ VM exceptions; Zig VM uses longjmp'
    body = case['body']
    if case['suite'] == 'IrLowering': return 'native IR lowering/codegen prerequisite'
    if case['suite'] == 'FeedbackVector': return 'JIT feedback-vector/inliner host and instrumentation not adapted'
    if case['suite'] == 'SharedCodeAllocator': return 'native code allocator/codegen prerequisite'
    if 'Jit' in case['name']: return 'JIT inliner/codegen prerequisite'
    if 'codegen' in body or 'CodeGen::' in body or 'is_native' in body:
        return 'native codegen or test-specific host/codegen assertions'
    if 'Frontend' in body or 'TypeInfer' in body: return 'typechecker/analysis host prerequisite'
    if case['flags']: return 'test-specific scoped flag configuration or host setup not adapted: ' + ', '.join(case['flags'])
    if case['calls']: return 'test-specific setup, callback, yield, compiler options, or post-run assertions not adapted'
    return 'upstream C++ host/internal VM/compiler assertions not adapted'


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reference-binary', help='unchanged pinned Luau.Conformance; runs -O0/-O1/-O2 first')
    for opt in (0, 1, 2):
        p.add_argument(f'--reference-xml-o{opt}', type=Path, help='use existing pinned reference doctest XML instead')
    p.add_argument('--reference-script-host', help='test-only upstream_script_reference executable')
    p.add_argument('--candidate-script-host', help='test-only Zig-linked upstream_script_candidate executable')
    p.add_argument('--reference-api-host', help='generated upstream_api_reference executable')
    p.add_argument('--candidate-api-host', help='generated Zig-linked upstream_api_candidate executable')
    p.add_argument('--graph-host', help='pinned upstream bytecode graph parser/serializer witness')
    p.add_argument('--gc-semantic-reference', type=int, help='exit code of C++ sandbox GC semantic witness')
    p.add_argument('--gc-semantic-candidate', type=int, help='exit code of Zig sandbox GC semantic witness')
    p.add_argument('--output-dir', required=True, type=Path, help='fresh directory for full XML/process evidence')
    p.add_argument('--timeout', type=float, default=600)
    a = p.parse_args()
    if a.timeout <= 0: p.error('--timeout must be positive')
    if a.output_dir.exists() and any(a.output_dir.iterdir()):
        p.error('--output-dir must be empty; existing evidence is never overwritten')
    a.output_dir.mkdir(parents=True, exist_ok=True)
    cases = inventory()  # also verifies all source blobs, including non-UTF8 fixtures
    records = []
    def record(kind, ident, opt, status, detail='', evidence=None):
        row = {'kind': kind, 'id': ident, 'optimization': opt, 'status': status, 'detail': detail}
        if evidence is not None: row['evidence'] = evidence
        records.append(row)
        print(f'{status} {kind}/{ident}' + (f'/O{opt}' if opt is not None else '') + (': ' + detail if detail else ''), flush=True)

    # The genuine, unchanged target always runs before any candidate host.
    refs = {}
    for opt in (0, 1, 2):
        source = getattr(a, f'reference_xml_o{opt}')
        xmlpath = a.output_dir / f'upstream-reference-O{opt}.xml'
        process = None
        if source:
            try: xmlpath.write_bytes(source.read_bytes())
            except OSError as exc:
                record('reference', 'full-target', opt, 'FAIL', str(exc)); continue
        elif a.reference_binary:
            process = invoke([a.reference_binary, '--reporters=xml', '--out=' + str(xmlpath.resolve()), f'-O{opt}'],
                             a.timeout, a.output_dir, f'upstream-reference-O{opt}')
        else:
            record('reference', 'full-target', opt, 'SKIP', 'missing unchanged reference binary or reference XML')
            continue
        try:
            refs[opt] = xml_results(xmlpath)
            failed_cases = [key for key, result in refs[opt].items() if not result['pass']]
            if (process and process['exit'] != 0) or failed_cases:
                record('reference', 'full-target', opt, 'FAIL',
                       f'upstream executable or {len(failed_cases)} reported test case(s) failed', process)
            else:
                record('reference', 'full-target', opt, 'PASS', f'{len(refs[opt])} executed doctest cases')
        except (OSError, ET.ParseError, ValueError) as exc:
            record('reference', 'full-target', opt, 'FAIL', f'invalid or incomplete XML: {exc}', process)

    # Isolate API test cases so a crash or infinite loop cannot hide subsequent
    # assertions. The two upstream tests with the same name are one group;
    # their original file/line identities are retained in the XML.
    api_groups = {}
    for case in cases:
        if ((case['suite'] == 'Conformance' and case['name'] in API_CASES | PARTIAL_CASES)
                or case['suite'] == 'DirectFieldAccess'):
            api_groups.setdefault((case['suite'], case['name']), []).append(
                (case['file'], case['line'], case['suite'], case['name']))
    api = {}
    api_variants = {}
    api_graph = {}
    def capture_api_graph(side, name, key, opt, label, process):
        if name not in API_GRAPH_CASES:
            return
        files = sorted(a.output_dir.glob(label + '-bc-*.bin'))
        if not a.graph_host:
            status = 'SKIP'
        elif not files:
            status = 'FAIL' if process['exit'] == 0 else 'SKIP'
        else:
            statuses = []
            for index, artifact in enumerate(files):
                result = invoke([a.graph_host, artifact.resolve()], a.timeout, a.output_dir,
                                f'{label}-graph-{index}')
                statuses.append('PASS' if result['exit'] == 0 and result['stdout'] == 'OK\n'
                                and not result['stderr'] else 'FAIL')
            status = combined_status(statuses)
        api_graph[(side, key, opt)] = (status, len(files))
        record('api-bytecode-graph', side + '/' + name, opt, status,
               f'{len(files)} captured compiler artifact(s) checked with pinned upstream graph')
    for side, binary in (('reference', a.reference_api_host), ('candidate', a.candidate_api_host)):
        if not binary:
            record('api-host', side, None, 'SKIP', 'missing API host executable')
            continue
        api[side] = {}
        for index, ((suite, name), expected) in enumerate(sorted(api_groups.items())):
            label = f'api-{side}-{index:03d}'
            xmlpath = a.output_dir / (label + '.xml')
            process = invoke([binary, '--reporters=xml', '--out=' + str(xmlpath.resolve()),
                              '--test-suite=' + suite, '--test-case=' + name],
                             a.timeout, a.output_dir, label,
                             bytecode_prefix=(a.output_dir / (label + '-bc')).resolve()
                             if name in API_GRAPH_CASES else None)
            try:
                results = scoped_xml_results(xmlpath, expected)
                api[side].update(results)
                for key in results:
                    capture_api_graph(side, name, key, api_host_optimization(
                        next(case for case in cases if (case['file'], case['line'], case['suite'], case['name']) == key)),
                        label, process)
                failures = sum(not result['pass'] for result in results.values())
                if process['exit'] != 0 or failures:
                    record('api-host', side + '/' + suite + '/' + name, None, 'FAIL',
                           f'host exited unsuccessfully or {failures} reported test case(s) failed', process)
            except (OSError, ET.ParseError, ValueError) as exc:
                record('api-host', side + '/' + suite + '/' + name, None, 'FAIL',
                       f'invalid or incomplete XML: {exc}', process)
        for name in MULTI_OPT_API_CASES:
            suite = 'Conformance'
            expected = api_groups[(suite, name)]
            for opt in (0, 2):
                label = f'api-{side}-{name}-O{opt}'
                xmlpath = a.output_dir / (label + '.xml')
                process = invoke([binary, '--reporters=xml', '--out=' + str(xmlpath.resolve()),
                                  '--test-suite=' + suite, '--test-case=' + name],
                                 a.timeout, a.output_dir, label, optimization=opt,
                                 bytecode_prefix=(a.output_dir / (label + '-bc')).resolve())
                try:
                    results = scoped_xml_results(xmlpath, expected)
                    for key, result in results.items():
                        if process['exit'] != 0:
                            result['pass'] = False
                        api_variants[(side, key, opt)] = result
                        capture_api_graph(side, name, key, opt, label, process)
                except (OSError, ET.ParseError, ValueError) as exc:
                    record('api-host', side + '/' + suite + '/' + name, opt, 'FAIL',
                           f'invalid or incomplete XML: {exc}', process)

    fixtures = {}
    for case in cases:
        key = (case['file'], case['line'], case['suite'], case['name'])
        ident = f'{case["suite"]}/{case["name"]}@{case["file"]}:{case["line"]}'
        supported_api = ((case['suite'] == 'Conformance' and case['name'] in API_CASES)
                         or case['suite'] == 'DirectFieldAccess')
        partial_api = case['suite'] == 'Conformance' and case['name'] in PARTIAL_CASES
        supported_script = (case['suite'] == 'Conformance' and
                            (len(case['calls']) == 1 or
                             (case['name'] == 'Integers' and [c['file'] for c in case['calls']] ==
                              ['integers.luau', 'integers_regspill.luau'])) and
                            (case['simple'] or case['name'] in SCRIPT_HOST_CASES))
        for call in case['calls']:
            fixtures.setdefault(call['file'], []).append(ident)
        cage_exercised = False
        script_outcomes = {}
        if case['suite'] == 'Conformance' and case['name'] == 'JitInlinerLongJump':
            for opt in (0, 1, 2):
                ref = refs.get(opt, {}).get(key)
                if ref is None:
                    record('reference-case', ident, opt, 'SKIP', 'not observed in reference XML')
                else:
                    record('reference-case', ident, opt, 'PASS' if ref['pass'] else 'FAIL',
                           f"assertions={ref['assert_successes']} passed/{ref['assert_failures']} failed")
            fixture = a.output_dir / 'jit-inliner-long-jump.luau'
            fixture.write_text(inliner_fixture(case['source']), encoding='utf-8')
            outcomes = []
            for mode in ('InlinerDefault', 'InlinerEnabled'):
                processes, artifacts, graphs = [], [], []
                for side, binary in (('reference', a.reference_script_host), ('candidate', a.candidate_script_host)):
                    label = 'inliner-long-jump-' + mode + '-' + side
                    artifact = (a.output_dir / (label + '.bytecode')).resolve()
                    artifacts.append(artifact)
                    if not binary:
                        processes.append(None)
                        graphs.append('SKIP')
                        continue
                    process = invoke([binary, fixture.resolve(), '=JitInlinerLongJump', mode, '1',
                                      '--write-bytecode', artifact], a.timeout, a.output_dir, label)
                    processes.append(process)
                    if not a.graph_host or process['exit'] == 77:
                        graphs.append('SKIP')
                    elif not artifact.is_file() or not artifact.stat().st_size:
                        graphs.append('FAIL')
                    else:
                        args = [a.graph_host, artifact]
                        if mode == 'InlinerEnabled':
                            args.append('--require-callfb')
                        graph = invoke(args, a.timeout, a.output_dir, label + '-graph')
                        graphs.append('PASS' if graph['exit'] == 0 and graph['stdout'] == 'OK\n'
                                      and not graph['stderr'] else 'FAIL')
                left, right = processes
                if any(p is not None and p['exit'] not in (0, 77) for p in processes):
                    status = 'FAIL'
                elif left is None or right is None or 77 in (left['exit'], right['exit']):
                    status = combined_status([*graphs, 'SKIP'])
                elif ((left['exit'], left['stdout'], left['stderr']) != (right['exit'], right['stdout'], right['stderr'])
                      or left['stdout'] != 'result=501500\n' or left['stderr']):
                    status = 'FAIL'
                elif 'FAIL' in graphs:
                    status = 'FAIL'
                elif 'SKIP' in graphs:
                    status = 'SKIP'
                else:
                    status = 'PASS' if artifacts[0].read_bytes() == artifacts[1].read_bytes() else 'FAIL'
                outcomes.append(status)
                record('inliner-semantic', ident + '/' + mode, 1, status,
                       'original generated script, matching result and byte-identical compiled protos and debug data')
            ref = refs.get(1, {}).get(key)
            reference_status = 'SKIP' if ref is None else 'PASS' if ref['pass'] else 'FAIL'
            record('candidate-case', ident, 1, combined_status([reference_status, *outcomes]),
                   'interpreter long-jump behavior with default and feedback-enabled flags')
            record('inliner-internals', ident, None, 'N/A',
                   'native JIT inliner is not shipped in the mod runtime')
            continue
        if case['suite'] == 'Conformance' and case['name'] in {
            'BytecodeDistributionPerFunctionTest', 'BytecodeDumpLinked'
        }:
            for opt in (0, 1, 2):
                ref = refs.get(opt, {}).get(key)
                if ref is None:
                    record('reference-case', ident, opt, 'SKIP', 'not observed in reference XML')
                else:
                    record('reference-case', ident, opt, 'PASS' if ref['pass'] else 'FAIL',
                           f"assertions={ref['assert_successes']} passed/{ref['assert_failures']} failed")
            if case['name'] == 'BytecodeDumpLinked':
                record('candidate-case', ident, None, 'N/A',
                       'C++ graph-dump linkage utility is not shipped in the mod runtime; the case has no script')
                continue
            matches = re.findall(r'const char\* source = R"\((.*?)\)";', case['source'], re.DOTALL)
            if len(matches) != 1 or re.search(r'\bprint\s*\(', matches[0]):
                record('candidate-case', ident, 1, 'FAIL', 'unexpected bytecode distribution script')
                continue
            fixture = a.output_dir / 'bytecode-distribution.luau'
            fixture.write_text(matches[0], encoding='utf-8')
            processes = []
            artifacts = []
            for side, binary in (('reference', a.reference_script_host), ('candidate', a.candidate_script_host)):
                artifact = (a.output_dir / f'bytecode-distribution-{side}.bytecode').resolve()
                artifacts.append(artifact)
                if not binary:
                    processes.append(None)
                    continue
                processes.append(invoke([binary, fixture.resolve(), '=BytecodeDistribution',
                                         'BytecodeDistribution', '1', '--write-bytecode', artifact],
                                        a.timeout, a.output_dir, 'bytecode-distribution-' + side))
            left, right = processes
            ref = refs.get(1, {}).get(key)
            if ((ref is not None and not ref['pass']) or
                    any(p is not None and p['exit'] not in (0, 77) for p in processes)):
                status = 'FAIL'
            elif ref is None or left is None or right is None or 77 in (left['exit'], right['exit']):
                status = 'SKIP'
            elif ((left['exit'], left['stdout'], left['stderr']) != (right['exit'], right['stdout'], right['stderr'])
                    or left['stdout'] != 'results=0\n' or left['stderr']):
                status = 'FAIL'
            elif not all(path.is_file() and path.stat().st_size for path in artifacts):
                status = 'FAIL'
            else:
                status = 'PASS' if artifacts[0].read_bytes() == artifacts[1].read_bytes() else 'FAIL'
            record('candidate-case', ident, 1, status,
                   'original script execution and byte-identical complete bytecode including every proto and debug section')
            record('utility-assertions', ident, None, 'N/A',
                   'C++ CodeGen bytecode-summary utility is not shipped in the mod runtime')
            continue
        if case['suite'] == 'FeedbackVector':
            for opt in (0, 1, 2):
                ref = refs.get(opt, {}).get(key)
                if ref is None:
                    record('reference-case', ident, opt, 'SKIP', 'not observed in reference XML')
                else:
                    record('reference-case', ident, opt, 'PASS' if ref['pass'] else 'FAIL',
                           f"assertions={ref['assert_successes']} passed/{ref['assert_failures']} failed")
            source_matches = re.findall(r'compile\(R"\((.*?)\)"\);', case['source'], re.DOTALL)
            if len(source_matches) != 1 or re.search(r'\bprint\s*\(', source_matches[0]):
                record('candidate-case', ident, 0, 'FAIL', 'unexpected upstream feedback script or output')
                continue
            fixture = a.output_dir / ('feedback-' + case['name'] + '.luau')
            fixture.write_text(source_matches[0], encoding='utf-8')
            mode_statuses = []
            for mode in ('FeedbackDefault', 'FeedbackEnabled'):
                side_results = []
                graph_results = []
                for side, binary in (('reference', a.reference_script_host), ('candidate', a.candidate_script_host)):
                    label = f'feedback-{case["name"]}-{mode}-{side}'
                    artifact = (a.output_dir / (label + '.bytecode')).resolve()
                    if not binary:
                        side_results.append(None)
                        graph_results.append('SKIP')
                        continue
                    result = invoke([binary, fixture.resolve(), '=FeedbackVectorTest', mode, '0',
                                     '--write-bytecode', artifact], a.timeout, a.output_dir, label)
                    side_results.append(result)
                    if not a.graph_host or result['exit'] == 77:
                        graph_results.append('SKIP')
                    elif not artifact.is_file() or artifact.stat().st_size == 0:
                        graph_results.append('FAIL')
                    else:
                        args = [a.graph_host, artifact]
                        if mode == 'FeedbackEnabled':
                            args.append('--require-callfb')
                        graph = invoke(args, a.timeout, a.output_dir, label + '-graph')
                        graph_results.append('PASS' if graph['exit'] == 0 and graph['stdout'] == 'OK\n'
                                             and not graph['stderr'] else 'FAIL')
                left, right = side_results
                if left is None or right is None or (left['exit'] == 77 or right['exit'] == 77):
                    status = combined_status([*graph_results, 'SKIP'])
                elif (left['exit'], left['stdout'], left['stderr']) != (right['exit'], right['stdout'], right['stderr']):
                    status = 'FAIL'
                elif left['exit'] != 0 or left['stdout'] != 'results=0\n' or left['stderr']:
                    status = 'FAIL'
                else:
                    status = combined_status(graph_results)
                mode_statuses.append(status)
                record('feedback-semantic', ident + '/' + mode, 0, status,
                       'unchanged script; compare result, errors and output on both engines' +
                       ('; both compiled outputs require CALLFB' if mode == 'FeedbackEnabled' else ''))
            ref = refs.get(1, {}).get(key)
            reference_status = 'SKIP' if ref is None else 'PASS' if ref['pass'] else 'FAIL'
            record('candidate-case', ident, 0, combined_status([reference_status, *mode_statuses]),
                   'original script under shipped defaults and feedback-enabled compiler flags')
            record('feedback-internals', ident, None, 'N/A',
                   'slot updates and inline callbacks are unshipped JIT internals; compiler feedback flags default false')
            continue
        # O0/O1/O2 represent actual upstream main.cpp interpreter variants.
        for opt in (0, 1, 2):
            ref = refs.get(opt, {}).get(key)
            if ref is None:
                record('reference-case', ident, opt, 'SKIP', 'not observed: missing XML or upstream compile-time conditional')
            else:
                record('reference-case', ident, opt, 'PASS' if ref['pass'] else 'FAIL',
                       f"assertions={ref['assert_successes']} passed/{ref['assert_failures']} failed")
            if native_only(case):
                record('candidate-case', ident, opt, 'N/A',
                       'upstream case only tests native codegen, which is not shipped')
                continue
            if case['suite'] == 'Conformance' and case['name'] == 'GCDump':
                if opt != 1:
                    continue  # The semantic witness is optimization-independent.
                left, right = a.gc_semantic_reference, a.gc_semantic_candidate
                status = gc_semantic_result(ref, left, right)
                record('candidate-case', ident, None, status,
                       f'sandbox GC semantic replacement: reference exit={left}, candidate exit={right}; '
                       'weak reachability and live object graphs under repeated host collection')
                continue
            if supported_api or partial_api:
                actual_opt = api_host_optimization(case)
                variant = case['suite'] == 'Conformance' and case['name'] in MULTI_OPT_API_CASES and opt != 1
                if variant:
                    actual_opt = opt
                if actual_opt is None and opt != 1:
                    continue  # One aggregate result, compared with the O1 reference.
                if actual_opt is not None and opt != actual_opt:
                    if case['name'] in {'ExceptionObject', 'LargeModuleA64', 'Interrupt', 'NDebugGetUpValue'}:
                        continue  # Unshipped exception mode or an optimization fixed by the upstream case.
                    if case['suite'] == 'DirectFieldAccess' or (case['suite'] == 'Conformance' and case['name'] in DEFAULT_COMPILER_CASES):
                        continue  # Original interpreter assertions use fixed default compiler options.
                    if partial_api:
                        record('api-assertions', ident, opt, 'SKIP', f'API host executed compiler O{actual_opt} only')
                    record('candidate-case', ident, opt, 'SKIP', f'API host executed compiler O{actual_opt} only')
                    continue
                if variant:
                    left = api_variants.get(('reference', key, opt))
                    right = api_variants.get(('candidate', key, opt))
                else:
                    left, right = api.get('reference', {}).get(key), api.get('candidate', {}).get(key)
                category = 'api-assertions' if partial_api else 'candidate-case'
                independent = ('; executes O0/O1/O2 interpreter subcases' if case['name'] == 'NativeUserdata'
                               else '; optimization-independent (no compiler)' if actual_opt is None else '')
                if left is None or right is None or (ref is None and case['name'] != 'ExceptionObject'):
                    record(category, ident, actual_opt, 'SKIP', 'missing reference case or generated API host result' + independent)
                elif (case['name'] != 'ExceptionObject' and not ref['pass']) or not left['pass']:
                    record(category, ident, actual_opt, 'FAIL', 'reference upstream/API test failed' + independent)
                elif not partial_api and (ref['assert_successes'] + ref['assert_failures'] != left['assert_successes'] + left['assert_failures']
                      or left['assert_successes'] + left['assert_failures'] != right['assert_successes'] + right['assert_failures']):
                    record(category, ident, actual_opt, 'FAIL', 'API assertion count differs from reference' + independent)
                else:
                    detail = ('upstream DirectAccess and ValidateMetatable assertions; graph/GC validation omitted'
                              if case['name'] == 'UserdataDirectAccess'
                              else 'upstream alignment conditions promoted to doctest checks' if case['name'] == 'UserdataAlignment'
                              else 'protected-call exception fixture semantics' if case['name'] == 'ExceptionObject'
                              else 'large-module interpreter assertions; A64 lowering omitted' if case['name'] == 'LargeModuleA64'
                              else 'upstream post-run API assertions; graph/GC validation omitted' if partial_api
                              else 'upstream interpreter assertions, Zig VM; compiler where used')
                    record(category, ident, actual_opt, 'PASS' if right['pass'] else 'FAIL', detail + independent)
                if partial_api:
                    if case['name'] == 'ExceptionObject':
                        record('candidate-case', ident, actual_opt, 'N/A',
                               'C++ exception mode not shipped: tools/luau/CMakeLists.txt uses LUA_USE_LONGJMP=1 for every profile; Zig vm/jmp.zig uses longjmp-style unwinding')
                    elif case['name'] == 'UserdataAlignment':
                        record('candidate-case', ident, actual_opt, alignment_result(ref, left, right),
                               'all upstream userdata alignment assertions promoted to checks on both allocators')
                    elif case['name'] in API_GRAPH_CASES:
                        status = partial_case_result(
                            ref, left, right, api_graph.get(('reference', key, actual_opt)),
                            api_graph.get(('candidate', key, actual_opt)),
                            gc_semantic_result(ref, a.gc_semantic_reference, a.gc_semantic_candidate))
                        record('candidate-case', ident, actual_opt, status,
                               'adapted upstream API assertions, captured compiler graphs and sandbox GC semantics')
                    elif case['name'] == 'LargeModuleA64':
                        record('candidate-case', ident, actual_opt, 'N/A',
                               'A64 native lowering checks are not applicable because codegen is not shipped; interpreter result is reported separately')
                    else:
                        omitted = ('original non-doctest assertions promoted to checks'
                                   if case['name'] == 'UserdataAlignment'
                                   else 'upstream bytecode-graph/GC validation not adapted')
                        record('candidate-case', ident, actual_opt, 'SKIP', omitted + independent)
            elif supported_script:
                simple_status = 'SKIP'
                if case['name'] == 'Coverage' and opt != 1:
                    continue  # The upstream case always compiles O1.
                elif not (a.reference_script_host and a.candidate_script_host) or ref is None:
                    record('script-body', ident, opt, 'SKIP', 'missing source host(s) or upstream reference result')
                    for direction in ('upstream-to-zig', 'zig-to-upstream'):
                        record('bytecode-cross', ident + '/' + direction, opt, 'SKIP', 'missing source host(s) or upstream reference result')
                else:
                    fixture = UPSTREAM / 'tests/conformance' / case['calls'][0]['file']
                    name = '=' + fixture.name
                    label = f'script-{case["line"]}-O{opt}'
                    left_bytecode = (a.output_dir / (label + '-reference.bytecode')).resolve()
                    right_bytecode = (a.output_dir / (label + '-candidate.bytecode')).resolve()
                    left_bytecode.unlink(missing_ok=True)
                    right_bytecode.unlink(missing_ok=True)
                    left_args = [a.reference_script_host, fixture, name, case['name'], str(opt)]
                    right_args = [a.candidate_script_host, fixture, name, case['name'], str(opt)]
                    left = invoke(left_args + ['--write-bytecode', left_bytecode], a.timeout, a.output_dir, label + '-reference')
                    right = invoke(right_args + ['--write-bytecode', right_bytecode], a.timeout, a.output_dir, label + '-candidate')
                    left_ok = ref['pass'] and left['exit'] == 0 and left['stdout'] == 'OK\n' and not left['stderr']
                    right_ok = right['exit'] == 0 and right['stdout'] == 'OK\n' and not right['stderr']
                    if left['exit'] == 77 and ref['pass']:
                        status, detail = 'SKIP', 'reference explicitly reports missing compiler/flag prerequisite'
                    elif not left_ok:
                        status, detail = 'FAIL', 'official reference or reference script adapter failed'
                    elif right['exit'] == 77:
                        status, detail = 'SKIP', 'candidate explicitly reports missing compiler/flag prerequisite'
                    elif not right_ok:
                        status, detail = 'FAIL', 'Zig script execution differs from reference'
                    else:
                        status, detail = 'PASS', 'script-body only; see candidate-case for remaining upstream assertions'
                        if case['name'] == 'BuffersWithCage':
                            cage_exercised = True
                    if case['name'] == 'Integers':
                        detail += '; integer flags explicitly enabled, not proof the default reference case executed the fixture'
                    record('script-body', ident, opt, status, detail, {'reference': left, 'candidate': right})
                    graph_statuses = []
                    for side, producer_ok, artifact in (('reference', left_ok, left_bytecode),
                                                        ('candidate', right_ok, right_bytecode)):
                        if not producer_ok or not a.graph_host:
                            graph_status = 'SKIP'
                            graph_result = None
                        elif not artifact.is_file() or artifact.stat().st_size == 0:
                            graph_status = 'FAIL'
                            graph_result = None
                        else:
                            graph_result = invoke([a.graph_host, artifact], a.timeout, a.output_dir,
                                                  label + '-graph-' + side)
                            graph_status = ('PASS' if graph_result['exit'] == 0 and
                                            graph_result['stdout'] == 'OK\n' and not graph_result['stderr']
                                            else 'FAIL')
                        graph_statuses.append(graph_status)
                        record('bytecode-graph', ident + '/' + side, opt, graph_status,
                               'pinned upstream function graph parse and nonempty serialization', graph_result)
                    gc_status = gc_semantic_result(ref, a.gc_semantic_reference, a.gc_semantic_candidate)
                    statuses = [status, gc_status, *graph_statuses]
                    simple_status = ('FAIL' if 'FAIL' in statuses else
                                     'SKIP' if 'SKIP' in statuses else 'PASS')
                    for direction, producer_ok, artifact, consumer_args in (
                        ('upstream-to-zig', left_ok, left_bytecode, right_args),
                        ('zig-to-upstream', left_ok and right_ok, right_bytecode, left_args),
                    ):
                        cross_id = ident + '/' + direction
                        if not producer_ok:
                            record('bytecode-cross', cross_id, opt, 'SKIP', 'successful producer execution required')
                            continue
                        if not artifact.is_file() or artifact.stat().st_size == 0:
                            record('bytecode-cross', cross_id, opt, 'FAIL',
                                   'producer reported success without a nonempty bytecode artifact')
                            continue
                        result = invoke(consumer_args + ['--read-bytecode', artifact], a.timeout, a.output_dir,
                                        label + '-' + direction)
                        if result['exit'] == 77:
                            status, detail = 'SKIP', 'consumer explicitly reports missing host/flag prerequisite'
                        elif result['exit'] != 0 or result['stdout'] != 'OK\n' or result['stderr']:
                            status, detail = 'FAIL', 'consumer failed to load and execute producer bytecode'
                        else:
                            status, detail = 'PASS', 'producer bytecode loaded and executed by other VM; not graph validation'
                        record('bytecode-cross', cross_id, opt, status, detail, result)
                script_outcomes[opt] = simple_status
                if case['simple'] or case['name'] in COMPLETE_SCRIPT_CASES:
                    detail = ('fixture body, upstream bytecode graph on both compilers, and sandbox GC semantics; '
                              'native codegen not shipped')
                    if case['name'] == 'Integers':
                        detail += '; integer type and library flags explicitly enabled on both engines'
                    record('candidate-case', ident, opt, simple_status, detail)
                else:
                    record('candidate-case', ident, opt, 'FAIL' if simple_status == 'FAIL' else 'SKIP',
                           'additional original host assertions or compiler variants not yet accounted')
            else:
                record('candidate-case', ident, opt, 'SKIP', reason(case))
        if native_only(case):
            for sub in case['subcases']:
                record('subcase', f'{ident}/{"/".join(sub["path"])}@{sub["line"]}', None, 'N/A',
                       'native-only upstream subcase; codegen is not shipped')
            for flag in case['flags']:
                record('feature-flag', ident + '/' + flag, None, 'N/A',
                       'native-only upstream case; codegen is not shipped')
            for call in case['calls']:
                record('script-call', f'{ident}/{call["file"]}@{call["line"]}', None, 'N/A',
                       'native-only upstream case; codegen is not shipped')
            continue
        # Every explicitly named doctest subcase is retained even when unavailable.
        for sub in case['subcases']:
            path = sub['path']
            script_subcase = (case['name'] in {'VectorLibrary', 'NativeIntegerSpills'} and len(path) == 1 or
                              case['name'] == 'Vector' and path[0] in {'NoIrHooks', 'IrHooks'})
            if script_subcase:
                opts = (0, 1, 2) if path in (['NoIrHooks'], ['IrHooks']) else (
                    (int(path[-1][1]),) if path[-1] in ('O0', 'O1', 'O2') else ())
                outcomes = [script_outcomes.get(opt, 'SKIP') for opt in opts]
                record('subcase', f'{ident}/{"/".join(path)}@{sub["line"]}', None,
                       combined_status(outcomes),
                       'interpreter fixture, compiler graph and sandbox GC witness; native codegen excluded')
                continue
            if case['name'] == 'NativeUserdata' and sub['path'][0] == 'IrHooks':
                record('subcase', f'{ident}/{"/".join(sub["path"])}@{sub["line"]}', None, 'N/A',
                       'native IR hooks are not shipped')
                continue
            if case['name'] in {'Debugger', 'UserdataDirectAccess', 'NativeUserdata'}:
                opts = (None,) if case['name'] == 'NativeUserdata' else (0, 1, 2)
                outcomes = []
                for subopt in opts:
                    if subopt in (0, 2):
                        left = api_variants.get(('reference', key, subopt))
                        right = api_variants.get(('candidate', key, subopt))
                    else:
                        left = api.get('reference', {}).get(key)
                        right = api.get('candidate', {}).get(key)
                    ref = refs.get(1 if subopt is None else subopt, {}).get(key)
                    status = partial_case_result(
                        ref, left, right, api_graph.get(('reference', key, subopt)),
                        api_graph.get(('candidate', key, subopt)),
                        gc_semantic_result(ref, a.gc_semantic_reference, a.gc_semantic_candidate))
                    if status == 'PASS' and not (subcase_observed(left, path) and subcase_observed(right, path)):
                        status = 'FAIL'
                    outcomes.append(status)
                record('subcase', f'{ident}/{"/".join(path)}@{sub["line"]}', None,
                       combined_status(outcomes),
                       'subcase observed in both API XML reports with matching graph and GC witnesses')
                continue
            record('subcase', f'{ident}/{"/".join(path)}@{sub["line"]}', None, 'SKIP',
                   'subcase-specific upstream host/codegen assertions not adapted')
        for variant in ('native-codegen', 'jit-inliner'):
            record('candidate-variant', ident + '/' + variant, None, 'N/A',
                   'codegen not shipped: tools/luau/CMakeLists.txt excludes CodeGen and Inliner; the runtime initializes the interpreter')
        for flag in case['flags']:
            if flag in {'LuauCodegenBufferInteger', 'LuauCodegenA64BitlrotateSelf'}:
                record('feature-flag', ident + '/' + flag, None, 'N/A',
                       'flag is used only in upstream CodeGen, which is not shipped')
            elif case['name'] == 'GCDump' and flag in {
                'DebugLuauUserDefinedClasses', 'DebugLuauUserDefinedClassesRuntime'
            }:
                record('feature-flag', ident + '/' + flag, None, 'N/A',
                       'heap-enumeration-only class setup defaults off in compiler/flags.zig:20 and vm/flags.zig:6; GC semantics tested separately')
            elif case['name'] == 'BuffersWithCage' and flag == 'LuauBufferCage' and not cage_exercised:
                record('feature-flag', ident + '/' + flag, None, 'SKIP',
                       'cage-enabled reference and candidate script execution not both observed')
            elif not ((supported_script and flag in SCRIPT_FLAGS.get(case['name'], set())) or
                    (supported_api and case['name'] in FLAGGED_CASES and flag in {'LuauGcTraceUdata', 'LuauGcHeapShrinkFix'}) or
                    (supported_api and case['name'] == CLASS_API_CASE and flag in SCRIPT_FLAGS['Classes']) or
                    (supported_api and case['name'] in LIBRARY_FLAGGED_CASES and flag == 'LuauSandboxFreezesVectorMetatable') or
                    (partial_api and case['name'] == 'Debugger' and flag == 'LuauCoroutineFinally') or
                    (partial_api and case['name'] == 'ExportEdgeCase' and flag == 'LuauExportValueSyntax')):
                record('feature-flag', ident + '/' + flag, None, 'SKIP',
                       'flag-specific test variant or native lowering not adapted')
        if supported_script and case['name'] == 'Integers':
            call = case['calls'][1]
            record('script-call', f'{ident}/{call["file"]}@{call["line"]}', None, 'N/A',
                   'upstream gates this fixture on native codegen and the fixture asserts is_native(); codegen is not shipped')
        if not supported_script and not partial_api:
            for call in case['calls']:
                record('script-call', f'{ident}/{call["file"]}@{call["line"]}', None, 'SKIP',
                       'unadapted upstream call variant: ' + reason(case))

    all_fixtures = sorted(p.name for p in (UPSTREAM / 'tests/conformance').glob('*.luau'))
    for name in all_fixtures:
        if name not in fixtures:
            record('fixture', name, None, 'SKIP', 'not associated with a literal runConformance call; requires upstream-specific driver')
    # Independently enumerate the preserved fixture directory: cases without a
    # literal runConformance call must not disappear from the qualification.
    # A reference adapter failure is not a Zig failure, but it is not a pass:
    # its host setup or the fixture's upstream invocation still needs adapting.
    for name in all_fixtures:
        fixture = UPSTREAM / 'tests/conformance' / name
        owners = [case for case in cases if any(call['file'] == name for call in case['calls'])]
        case_name = owners[0]['name'] if owners else fixture.stem
        for opt in (0, 1, 2):
            ident = f'{name}/{case_name}'
            if name == 'coverage.luau' and opt != 1:
                continue  # The upstream case forces O1 for every outer variant.
            if name in ('integers_regspill.luau', 'native.luau', 'native_types.luau', 'jit_inliner.luau'):
                detail = ('upstream forcibly enables JIT inliner; codegen is not shipped' if name == 'jit_inliner.luau'
                          else 'upstream gates this fixture on native codegen and it asserts is_native(); codegen is not shipped')
                record('fixture-parity', ident, opt, 'N/A', detail)
                continue
            if name in ('debugger.luau', 'ndebug_upvalues.luau', 'native_userdata.luau', 'udata_direct.luau'):
                # These fixtures require upstream host callbacks. The generated
                # API host executes the actual case (including its fixture),
                # rather than the generic host that lacks those callbacks.
                owner = owners[0]
                key = (owner['file'], owner['line'], owner['suite'], owner['name'])
                if name in ('debugger.luau', 'udata_direct.luau') and opt != 1:
                    left = api_variants.get(('reference', key, opt))
                    right = api_variants.get(('candidate', key, opt))
                else:
                    left = api.get('reference', {}).get(key)
                    right = api.get('candidate', {}).get(key)
                official = refs.get(opt, {}).get(key)
                if left is None or right is None or official is None:
                    record('fixture-parity', ident, opt, 'SKIP', 'missing adapted API or upstream reference result')
                else:
                    passed = official['pass'] and left['pass'] and right['pass']
                    record('fixture-parity', ident, opt, 'PASS' if passed else 'FAIL',
                           'adapted upstream host and fixture; graph/GC validation accounted separately')
                continue
            if not (a.reference_script_host and a.candidate_script_host):
                record('fixture-parity', ident, opt, 'SKIP', 'missing reference or candidate script host')
                continue
            args = [fixture, '=' + name, case_name, str(opt)]
            label = f'fixture-{name}-O{opt}'
            left = invoke([a.reference_script_host, *args], a.timeout, a.output_dir, label + '-reference')
            right = invoke([a.candidate_script_host, *args], a.timeout, a.output_dir, label + '-candidate')
            status, detail = fixture_result(left, right)
            record('fixture-parity', ident, opt, status, detail, {'reference': left, 'candidate': right})
    counts = {s: sum(row['status'] == s for row in records) for s in ('PASS', 'FAIL', 'SKIP', 'N/A')}
    report = {'upstream_commit': '32c28089305f488f901aaf1dbf3fd1cb65fa3d11',
              'summary': counts, 'fixture_cases': fixtures, 'records': records,
              'case_metadata': [{k: v for k, v in case.items() if k not in ('source', 'body')} for case in cases],
              'note': 'Script-body PASS is partial and never implies an upstream candidate-case PASS.'}
    (a.output_dir / 'results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    table = ['| Fixture / host case | Optimization | Result | Detail |',
             '| --- | --- | --- | --- |']
    for row in records:
        if row['kind'] == 'fixture-parity':
            detail = row['detail'].replace('|', '\\|').replace('\n', ' ')
            table.append(f"| {row['id']} | O{row['optimization']} | {row['status']} | {detail} |")
    (a.output_dir / 'fixture-results.md').write_text('\n'.join(table) + '\n', encoding='utf-8')
    print('SUMMARY ' + ' '.join(f'{key}={value}' for key, value in counts.items()))
    return 1 if counts['FAIL'] else 77 if counts['SKIP'] else 0


if __name__ == '__main__':
    sys.exit(main())
