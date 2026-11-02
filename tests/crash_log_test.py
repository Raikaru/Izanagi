#!/usr/bin/env python3
"""Run only the standalone fault binary, checking the production report for each signal."""
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix='p4pc-crash-') as tmp:
    for sig in (signal.SIGSEGV, signal.SIGBUS, signal.SIGILL, signal.SIGFPE, signal.SIGABRT):
        directory = Path(tmp) / sig.name
        directory.mkdir()
        result = subprocess.run([sys.argv[1], str(directory / 'boot.log'), str(sig.value)], timeout=15)
        report = (directory / 'p4pc-crash.log').read_text()
        assert result.returncode == 128 + sig.value, (sig, result.returncode, report)
        for expected in ('CRASH signal ' + str(sig.value), 'fault address', 'instruction 0:', 'frame 0:'):
            assert expected in report, (sig, expected, report)
        if sig == signal.SIGSEGV:
            assert 'guest address 0xdeadbeef' in report, report
        print(sig.name + ': exit=' + str(result.returncode) + ' report=' + str(len(report)) + ' bytes PASS')
