#!/usr/bin/env python3
"""Optional census-exact production-parser test. No retail bytes in fixtures."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    install = os.environ.get('P4PC_GOLDEN_STEAM')
    inventory = os.environ.get('P4PC_GOLDEN_INVENTORY')
    if not install or not inventory:
        print('SKIP: owner Steam install and frozen census inventory required')
        return 77
    argv = sys.argv[1:sys.argv.index('--driver-end')]
    root = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix='golden-model-census-') as directory:
        command = [sys.executable, str(root / 'tools/golden/model_native_census.py'),
                   '--install', install, '--inventory', inventory, '--out', str(Path(directory) / 'receipt.json'), '--static']
        native = os.environ.get('P4PC_GOLDEN_NATIVE')
        if native:
            command += ['--native', native]
        return subprocess.call(command + ['--driver'] + argv)


if __name__ == '__main__':
    sys.exit(main())
