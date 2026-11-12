#!/bin/sh
# Shared scratch lifecycle: every failing gate retains its complete work tree, including on a caller timeout.
# A runner may supply P4PC_SCENE_ARTIFACTS; otherwise the work trees go below TMPDIR, or below the managed scratch of
# artifacts_local.sh where a repository provides one (the gate_local_* functions).
_gate_inherited_dump=${P4PC_DUMP_RAM:-}
gate_local=0
if [ -f "$(dirname "$0")/artifacts_local.sh" ]; then
    . "$(dirname "$0")/artifacts_local.sh"
fi
gate_artifacts_begin() {
    gate_work_managed=0
    gate_default_root=${TMPDIR:-/tmp}
    if [ "$gate_local" = 1 ] && [ -z "${P4PC_SCENE_ARTIFACTS:-}" ]; then
        gate_local_begin || return 1
        gate_work_managed=1
    fi
    gate_artifact_root=${P4PC_SCENE_ARTIFACTS:-$gate_default_root}
    mkdir -p "$gate_artifact_root" || return 1
    work=$(mktemp -d "$gate_artifact_root/$1.XXXXXX") || return 1
    if [ "$gate_work_managed" = 1 ]; then
        gate_local_register "$work" || return 1
    fi
    gate_artifact_done=0
    if [ "${2:-game}" = viewer ]; then
        export P4PC_DUMP_RAM="$work/state.bin"
    else
        gate_dump=${_gate_inherited_dump:-$work/state_%u.bin}
        if [ "$gate_work_managed" = 1 ]; then
            gate_dump=$(gate_local_dump "$gate_dump")
        fi
        export P4PC_DUMP_RAM="$gate_dump"
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
        if [ "${gate_work_managed:-0}" = 1 ]; then
            gate_local_release "$work" || true
        fi
    fi
}
