#!/usr/bin/env python3
"""Build pinned upstream and Zig adapters, then retain differential evidence."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PIN = '32c28089305f488f901aaf1dbf3fd1cb65fa3d11'


def run(command, **kwargs):
    print('RUN', *map(str, command), flush=True)
    return subprocess.run(command, check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--zig', required=True)
    parser.add_argument('--python', required=True)
    parser.add_argument('--scratch-root', type=Path, default=Path.cwd())
    args = parser.parse_args()
    # The evidence root is outside the checkout and retains all output.
    args.scratch_root.mkdir(parents=True, exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='luau-upstream-', dir=args.scratch_root)).resolve()
    upstream = Path(os.environ.get('P4PC_LUAU_REFERENCE_DIR', '/lane/build/luau-upstream-32c28089')).resolve()
    if shutil.disk_usage(scratch).free < 12 * 1024**3:
        raise RuntimeError('insufficient free space for reference build while preserving the 10 GiB floor')
    ref_build = scratch / 'upstream-build'
    adapter_build = scratch / 'adapter-build'
    install = scratch / 'candidate'
    print('EVIDENCE', scratch, 'UPSTREAM', upstream, 'PIN', PIN, flush=True)
    git = ['git', '-c', 'safe.directory=' + str(upstream), '-C', str(upstream)]
    git_env = {**os.environ, 'GIT_OPTIONAL_LOCKS': '0'}
    head = subprocess.check_output([*git, 'rev-parse', 'HEAD'], text=True, env=git_env).strip()
    dirty = subprocess.check_output([*git, 'status', '--porcelain'], text=True, env=git_env)
    if head != PIN or dirty:
        raise RuntimeError(f'upstream checkout mismatch: HEAD={head} dirty={dirty!r}')
    zig_env = {**os.environ, 'ZIG_GLOBAL_CACHE_DIR': str(scratch / 'zig-global-cache')}
    run([args.zig, 'build', '-Doptimize=ReleaseSafe', '-Dpython=' + args.python,
         '--cache-dir', str(scratch / 'zig-cache'),
         '--prefix', str(install), 'upstream-script-candidate', 'upstream-api-candidate',
         'fastcall-candidate'], cwd=ROOT / 'luau-zig', env=zig_env)
    conformance = (upstream / 'tests/Conformance.test.cpp').read_text(encoding='utf-8')
    helper = conformance[conformance.index('static void populateRTTI('):conformance.index('TEST_CASE("Types")')]
    (scratch / 'upstream_populate_rtti.inc').write_text(helper, encoding='utf-8')
    extra_cmake = scratch / 'rtti-target.cmake'
    extra_cmake.write_text(
        f'add_executable(p4pc_types_rtti "{ROOT / "luau-zig/tests/upstream_rtti.cpp"}")\n'
        f'target_include_directories(p4pc_types_rtti PRIVATE "{scratch}")\n'
        'target_compile_features(p4pc_types_rtti PRIVATE cxx_std_17)\n'
        'target_link_libraries(p4pc_types_rtti PRIVATE Luau.Analysis Luau.VM)\n',
        encoding='utf-8')
    run(['cmake', '-S', str(upstream), '-B', str(ref_build), '-DLUAU_BUILD_CLI=OFF',
         '-DLUAU_BUILD_TESTS=ON', '-DCMAKE_BUILD_TYPE=Release',
         '-DCMAKE_PROJECT_Luau_INCLUDE=' + str(extra_cmake)])
    run(['cmake', '--build', str(ref_build), '--config', 'Release', '--target',
         'Luau.Conformance', 'p4pc_types_rtti', '-j', '2'])
    rtti = scratch / 'types-rtti.luau'
    with rtti.open('w', encoding='utf-8') as out:
        run([str(ref_build / 'p4pc_types_rtti')], stdout=out)
    fixture_env = {**os.environ, 'LUAU_TEST_RTTI_SOURCE': str(rtti)}
    run(['cmake', '-S', str(ROOT / 'luau-zig/tests'), '-B', str(adapter_build), '-DCMAKE_BUILD_TYPE=Release'])
    run(['cmake', '--build', str(adapter_build), '--config', 'Release', '--target',
         'upstream_script_reference', 'upstream_api_reference', 'upstream_graph_reference',
         'fastcall_reference', 'fastcall_math_reference', '-j', '2'])
    semantic_build = scratch / 'semantic-build'
    run(['cmake', '-S', str(ROOT / 'luau-zig/sandbox-witness'), '-B', str(semantic_build),
         '-DCMAKE_BUILD_TYPE=Release'])
    run(['cmake', '--build', str(semantic_build), '--config', 'Release',
         '--target', 'sandbox_witness_heap_reference', '-j', '2'])
    semantic_reference = subprocess.run([str(semantic_build / 'sandbox_witness_heap_reference')]).returncode
    semantic_candidate = subprocess.run([
        args.zig, 'build', '-Doptimize=ReleaseSafe', '--cache-dir', str(scratch / 'zig-cache'),
        'sandbox-witness-heap',
    ], cwd=ROOT / 'luau-zig', env=zig_env).returncode
    print(f'WEAK_GC_REFERENCE={semantic_reference} WEAK_GC_CANDIDATE={semantic_candidate}', flush=True)
    suite = subprocess.run([args.python, str(ROOT / 'luau-zig/tests/upstream_suite.py'),
         '--reference-binary', str(ref_build / 'Luau.Conformance'),
         '--reference-script-host', str(adapter_build / 'upstream_script_reference'),
         '--candidate-script-host', str(install / 'bin/upstream_script_candidate'),
         '--reference-api-host', str(adapter_build / 'upstream_api_reference'),
         '--candidate-api-host', str(install / 'bin/upstream_api_candidate'),
         '--graph-host', str(adapter_build / 'upstream_graph_reference'),
         '--gc-semantic-reference', str(semantic_reference),
         '--gc-semantic-candidate', str(semantic_candidate),
         '--output-dir', str(scratch / 'results')], env=fixture_env).returncode
    math_results = scratch / 'results' / 'math-traces'
    math_results.mkdir()
    with (math_results / 'reference.txt').open('w', encoding='utf-8') as out:
        reference_math = subprocess.run([str(adapter_build / 'fastcall_math_reference')], stdout=out).returncode
    with (math_results / 'candidate.txt').open('w', encoding='utf-8') as out:
        candidate_math = subprocess.run([
            args.zig, 'test', 'src/fastmath_trace_test.zig', '-fllvm', '-OReleaseSafe',
            '--cache-dir', str(scratch / 'zig-cache'),
        ], cwd=ROOT / 'luau-zig', env=zig_env, stderr=out, stdout=out).returncode
    with (math_results / 'comparison.txt').open('w', encoding='utf-8') as out:
        math_comparison = subprocess.run([
            args.python, str(ROOT / 'luau-zig/tests/fastcall_trace_oracle.py'),
            '--reference', str(math_results / 'reference.txt'),
            '--candidate', str(math_results / 'candidate.txt'),
            '--output', str(math_results / 'trace-oracle.json'),
        ], stdout=out, stderr=out).returncode
    cross_status = []
    qemu = os.environ.get('P4PC_QEMU_AARCH64', '/usr/bin/qemu-aarch64-static')
    with (math_results / 'raw-x64.txt').open('w', encoding='utf-8') as out:
        cross_status.append(subprocess.run([
            args.zig, 'test', 'src/raw_nan_probe.zig', '-fllvm', '-OReleaseSafe',
            '--cache-dir', str(scratch / 'zig-cache'),
        ], cwd=ROOT / 'luau-zig', env=zig_env, stdout=out, stderr=out).returncode)
    for label, target in (('a64', 'aarch64-linux-musl'), ('android', 'aarch64-linux-android')):
        for purpose, source in (('trace', 'src/fastmath_trace_test.zig'),
                                ('raw', 'src/raw_nan_probe.zig')):
            executable = scratch / (label + '-' + purpose)
            with (math_results / f'{label}-{purpose}-build.txt').open('w', encoding='utf-8') as out:
                compiled = subprocess.run([
                    args.zig, 'test', source, '-fllvm', '-OReleaseSafe', '-static',
                    '-target', target, '--test-no-exec', '-femit-bin=' + str(executable),
                    '--cache-dir', str(scratch / 'zig-cache'),
                ], cwd=ROOT / 'luau-zig', env=zig_env, stdout=out, stderr=out).returncode
            cross_status.append(compiled)
            if compiled:
                continue
            with (math_results / f'{label}-{purpose}.txt').open('w', encoding='utf-8') as out:
                cross_status.append(subprocess.run([qemu, str(executable)], stdout=out, stderr=out).returncode)
        trace = math_results / f'{label}-trace.txt'
        if trace.is_file():
            with (math_results / f'{label}-comparison.txt').open('w', encoding='utf-8') as out:
                cross_status.append(subprocess.run([
                    args.python, str(ROOT / 'luau-zig/tests/fastcall_cross_compare.py'),
                    str(math_results / 'candidate.txt'), str(trace),
                ], stdout=out, stderr=out).returncode)
    fastcalls = subprocess.run([
        args.python, str(ROOT / 'luau-zig/tests/fastcall_differential.py'),
        '--builtin', 'all',
        '--reference', str(adapter_build / 'fastcall_reference'),
        '--candidate', str(install / 'bin/fastcall_candidate'),
        '--graph', str(adapter_build / 'upstream_graph_reference'),
        '--output-dir', str(scratch / 'results' / 'fastcall'),
    ], env=fixture_env).returncode
    oracle = subprocess.run([
        args.python, str(ROOT / 'luau-zig/tests/fastcall_oracle.py'),
        '--fastcall-dir', str(scratch / 'results' / 'fastcall'),
        '--output-dir', str(scratch / 'results' / 'fastcall-oracle'), '--reconcile',
    ], env=fixture_env).returncode
    print(f'UPSTREAM_SUITE={suite} FASTCALL_DIFFERENTIAL={fastcalls} ORACLE={oracle} MATH_REFERENCE={reference_math} MATH_CANDIDATE={candidate_math} MATH_COMPARE={math_comparison} CROSS={cross_status}', flush=True)
    return 0 if fastcalls in (0, 1) and all(status == 0 for status in (semantic_reference, semantic_candidate, suite, oracle,
                                                                        reference_math, candidate_math, math_comparison, *cross_status)) else 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f'upstream conformance lane failed: {exc}', file=sys.stderr)
        sys.exit(1)
