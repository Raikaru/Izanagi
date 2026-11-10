#!/bin/sh
# Verify the selected gate reference with this checkout's manifest, including direct/manual gate runs.
here=$(cd "$(dirname "$0")" && { pwd -W 2>/dev/null || pwd; }) || exit 1
if [ -n "${P4PC_PYTHON:-}" ]; then py=$P4PC_PYTHON
elif command -v python3 >/dev/null 2>&1; then py=python3
else py=python
fi
exec "$py" "$here/ref_guard.py" "$@"
