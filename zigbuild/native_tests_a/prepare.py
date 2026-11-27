#!/usr/bin/env python3
"""Prepare CTest's per-test save directory, then run the fixture runner."""
import os
from pathlib import Path
import sys

for directory in sys.argv[1:3]:
    Path(directory).mkdir(parents=True, exist_ok=True)
os.execvp(sys.argv[3], sys.argv[3:])
