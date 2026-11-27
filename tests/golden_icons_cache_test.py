#!/usr/bin/env python3
"""Synthetic-only M5 atlas object-set round trip; no game installation."""
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    end = sys.argv.index('--driver-end')
    with tempfile.TemporaryDirectory(prefix='golden-atlas-') as tmp:
        dirs = [Path(tmp)/name for name in ('pref','install','program')]
        for path in dirs:
            path.mkdir()
        subprocess.run(sys.argv[1:end] + ['cache'] + [str(p) for p in dirs], check=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
