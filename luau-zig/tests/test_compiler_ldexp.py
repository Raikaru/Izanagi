#!/usr/bin/env python3
"""Compare signed-zero ldexp behavior in pinned and Zig interpreter hosts.

Run with the upstream_script_reference and upstream_script_candidate executables.
Both hosts compile and execute the fixture at O0/O1/O2; each VM also executes
bytecode emitted by the other compiler at each level. No native/JIT is tested.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


FIXTURE = Path(__file__).resolve().parent / 'fixtures' / 'compiler_ldexp.luau'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--candidate', type=Path, required=True)
    args = parser.parse_args()
    hosts = {'reference': args.reference.resolve(), 'candidate': args.candidate.resolve()}
    failures = []

    def run(label, host, level, option, bytecode):
        command = [str(host), str(FIXTURE), 'ldexp', 'CompilerLdexp', str(level), option, str(bytecode)]
        result = subprocess.run(command, capture_output=True, text=True, timeout=30)
        if result.returncode != 0 or result.stdout.strip() != 'OK':
            failures.append(label)
            print(f'FAIL {label}: exit={result.returncode} stdout={result.stdout!r} stderr={result.stderr!r}')
        else:
            print(f'PASS {label}')

    with tempfile.TemporaryDirectory(prefix='luau-ldexp-') as directory:
        for level in range(3):
            for compiler, host in hosts.items():
                bytecode = Path(directory) / f'{compiler}-O{level}.bc'
                run(f'{compiler} O{level} own VM', host, level, '--write-bytecode', bytecode)
                other = 'candidate' if compiler == 'reference' else 'reference'
                if bytecode.exists():
                    run(f'{compiler} O{level} on {other} VM', hosts[other], level, '--read-bytecode', bytecode)
                else:
                    failures.append(f'{compiler} O{level} bytecode unavailable')
                    print(f'FAIL {compiler} O{level}: bytecode unavailable for cross-VM run')
    print('SKIP native/JIT and non-interpreter hosts')
    return 1 if failures else 0


if __name__ == '__main__':
    raise SystemExit(main())
