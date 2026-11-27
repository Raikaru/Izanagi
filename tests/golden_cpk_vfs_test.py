#!/usr/bin/env python3
"""Run the native Golden VFS on generated CPK/PAK/CRILAYLA fixtures."""
import subprocess
import sys
import tempfile
from pathlib import Path
import golden_mode_fixtures as fx


def main():
    end = sys.argv.index('--driver-end')
    command = sys.argv[1:end]
    with tempfile.TemporaryDirectory(prefix='golden-cpk-vfs-') as directory:
        root = Path(directory)
        inner = fx.pak_counted([('deep.bin', b'deep')], 32)
        packed = fx.pak_v1([('inner.bin', b'inner'), ('inner.pack', inner)])
        plain = b'x' * 300
        compressed = fx.crilayla(plain)
        archives = {
            'data_e.cpk': [('foo.bin', b'locale', 6), ('stuff.pack', packed, len(packed)),
                           ('compressed.bin', compressed, len(plain))],
            'data.cpk': [('foo.bin', b'common', 6)],
            'data_movie.cpk': [('movie.bin', b'movie', 5)],
            'sysdat.cpk': [('system.bin', b'system', 6)],
        }
        for name, entries in archives.items():
            (root / name).write_bytes(fx.cpk(entries))
        (root / 'FOO.BIN').write_bytes(b'retail')
        subprocess.run(command + [str(root)], check=True)
        subprocess.run(command + [str(root), 'dormant'], check=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
