#!/bin/sh
. "$(dirname "$0")/input_guard.sh"
# The GPU runner uses MSYS2 sh, whose python3 is POSIX Python and cannot consume
# its Windows manifest/retail paths. Use the native Windows launcher there.
here=$(cd "$(dirname "$0")" && pwd) || exit 1
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) exec py -3 "$(cygpath -w "$here/pad_field_gate.py")" "$@" ;;
  *) exec python3 "$here/pad_field_gate.py" "$@" ;;
esac
