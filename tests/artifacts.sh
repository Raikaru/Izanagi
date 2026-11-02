#!/bin/sh
# Shared scratch lifecycle: every failing gate retains its complete work tree, including on a caller timeout.
# A runner may supply P4PC_SCENE_ARTIFACTS; otherwise the work trees go below TMPDIR.
_gate_inherited_dump=${P4PC_DUMP_RAM:-}
gate_artifacts_begin() {
    gate_artifact_root=${P4PC_SCENE_ARTIFACTS:-${TMPDIR:-/tmp}}
    mkdir -p "$gate_artifact_root" || return 1
    work=$(mktemp -d "$gate_artifact_root/$1.XXXXXX") || return 1
    gate_artifact_done=0
    if [ "${2:-game}" = viewer ]; then
        export P4PC_DUMP_RAM="$work/state.bin"
    else
        export P4PC_DUMP_RAM="${_gate_inherited_dump:-$work/state_%u.bin}"
    fi
    trap 'gate_artifacts_cleanup "$?"' EXIT
}
gate_artifacts_cleanup() {
    gate_artifact_status=${1:-$?}
    if [ "${gate_artifact_done:-1}" = 0 ]; then
        gate_artifact_done=1
        if [ "$gate_artifact_status" -ne 0 ] && [ "$gate_artifact_status" -ne 77 ]; then
            echo "gate artifacts: failure evidence retained in $work"
        else
            rm -rf "$work"
        fi
    fi
}
