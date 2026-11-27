#!/usr/bin/env python3
"""Build and exercise the unchanged mod contract hosts against Zig Luau."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--zig', required=True)
    parser.add_argument('--python', required=True)
    parser.add_argument('--wine')
    args = parser.parse_args()
    env = os.environ.copy()
    env.pop('P4PC_ISO', None)
    env.update(P4PC_MODS='0', P4PC_PACKS='0', P4PC_INPUT='script',
               P4PC_VIRTUAL_CLOCK='1', SDL_AUDIODRIVER='dummy')
    with tempfile.TemporaryDirectory(prefix='luau-zig-contract-') as temp:
        directory = Path(temp)
        prefix = directory / 'install'
        subprocess.run([args.zig, 'build', '-Doptimize=ReleaseSafe', 'fastcall-unit-tests', 'nan-boundary-tests'],
                       cwd=ROOT / 'luau-zig', env=env, check=True)
        command = [args.zig, 'build', '-Doptimize=ReleaseSafe', '-Dpython=' + args.python,
                   '--prefix', str(prefix), 'mod-luau-tests', 'sandbox-host']
        if args.wine:
            command.insert(2, '-Dtarget=x86_64-windows-gnu')
        subprocess.run(command, cwd=ROOT / 'luau-zig', env=env, check=True)
        suffix = '.exe' if args.wine else ''
        runner = [args.wine] if args.wine else []
        mod = str(prefix / 'bin' / ('mod_luau_test' + suffix))
        save = str(prefix / 'bin' / ('mod_luau_save_test' + suffix))
        sandbox = str(prefix / 'bin' / ('luau_sandbox_test' + suffix))
        tests = ROOT / 'tests'
        commands = [
            [args.python, str(ROOT / 'luau-zig/tests/test_upstream_suite_xml.py')],
            runner + [sandbox],
            runner + [mod, str(directory / 'fixture-data')],
            runner + [save, str(directory / 'save-data')],
            [args.python, str(tests / 'mod_luau_save_failure_test.py'), *runner, save,
             '--driver-end', str(directory / 'save-failure')],
            [args.python, str(tests / 'mod_luau_stage_failure_test.py'), *runner, mod,
             '--driver-end', str(directory / 'stage-failure')],
            [args.python, str(tests / 'mod_callback_quiescence_test.py'), 'event', *runner, mod],
        ]
        for command in commands:
            print('RUN', *command, flush=True)
            subprocess.run(command, cwd=directory, env=env, check=True)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as exc:
        print(f'candidate contract command failed: {exc}', file=sys.stderr)
        sys.exit(1)
