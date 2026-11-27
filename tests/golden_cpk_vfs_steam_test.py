#!/usr/bin/env python3
"""Local-only hash qualification of the Golden VFS against Steam inventory."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys


def main():
    install = os.environ.get('P4PC_GOLDEN_STEAM')
    inventory = os.environ.get('P4PC_GOLDEN_INVENTORY')
    if not install or not inventory:
        print('Steam install and inventory not configured; skipping')
        return 77
    root = Path(install).resolve()
    text = (root.parent.parent / 'appmanifest_1113000.acf').read_text(encoding='utf-8')
    match = re.search(r'"installdir"\s*"([^"]+)"', text)
    if not match or match.group(1) != root.name or not re.search(r'"appid"\s*"1113000"', text):
        raise ValueError('not the app 1113000 Steam library install')
    with open(inventory, encoding='utf-8') as f:
        rows = [json.loads(line) for line in f]
    archives = ('data_e.cpk', 'data.cpk', 'data_movie.cpk', 'sysdat.cpk')
    shadowed = set()
    samples = []
    for archive in archives:
        candidates = [r for r in rows if r.get('archive') == archive and r.get('depth') == 1
                      and r.get('size', 0) > 0 and r['path'].split('::', 1)[1].lower() not in shadowed
                      and not r.get('error')]
        if not candidates:
            raise ValueError('no unshadowed members in ' + archive)
        for compressed in (False, True):
            choices = [r for r in candidates if bool(r.get('compressed')) == compressed]
            if choices:
                samples.append(min(choices, key=lambda r: r['size']))
        shadowed.update(r['path'].split('::', 1)[1].lower() for r in rows
                        if r.get('archive') == archive and r.get('depth') == 1)
    end = sys.argv.index('--driver-end')
    command = sys.argv[1:end]
    for row in samples:
        member = row['path'].split('::', 1)[1]
        subprocess.run(command + [str(root), 'steam', member, row['sha256']], check=True)
    print('Steam VFS samples verified:', len(samples))
    return 0


if __name__ == '__main__':
    sys.exit(main())
