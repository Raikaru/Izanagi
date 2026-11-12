#!/usr/bin/env python3
"""Verify a Windows player imports only system or adjacent shipped DLLs."""
import argparse
from pathlib import Path
import sys
from pe_imports import imports

SYSTEM_DLLS = frozenset(('kernel32.dll', 'user32.dll', 'advapi32.dll', 'shell32.dll',
                         'gdi32.dll', 'msvcrt.dll', 'ntdll.dll', 'ws2_32.dll', 'ole32.dll',
                         'ucrtbase.dll', 'shcore.dll', 'version.dll', 'winmm.dll', 'imm32.dll',
                         'setupapi.dll', 'oleaut32.dll', 'comdlg32.dll', 'dbghelp.dll',
                         'opengl32.dll', 'bcrypt.dll'))


def missing(executable, directory):
    present = {path.name.lower() for path in Path(directory).iterdir() if path.is_file()}
    return [name for name in imports(executable)
            if name not in SYSTEM_DLLS and not name.startswith('api-ms-win-') and name not in present]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    absent = missing(args.executable, args.directory)
    if absent:
        print('package: missing imported DLLs: ' + ', '.join(absent), file=sys.stderr)
        return 1
    print('package: Windows imports are system or shipped DLLs')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
