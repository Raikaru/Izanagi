#!/bin/sh
# Sourced by game/viewer gates: opting into scripted-only input is the runner's job.
# Never silently enable it here; an omitted isolation setting is a failed check.
if [ "${P4PC_INPUT:-}" != script ]; then
    printf '%s\n' "$0: P4PC_INPUT=script is required for automated runs (physical input forbidden)" >&2
    exit 1
fi
# Gate command arguments may contain env assignments; they must not undo isolation.
for p4pc_input_arg do
    case $p4pc_input_arg in
        P4PC_INPUT=script) ;;
        P4PC_INPUT=*)
            printf '%s\n' "$0: gate arguments cannot override P4PC_INPUT=script" >&2
            exit 1
            ;;
    esac
done
unset p4pc_input_arg
