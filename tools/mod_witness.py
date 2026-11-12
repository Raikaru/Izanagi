#!/usr/bin/env python3
"""Mode/provenance for the separate mod-test lane; never a retail reference."""
import argparse
import hashlib
import json
import os
from pathlib import Path

VARIABLE = "P4PC_MOD_WITNESS"
MARKER = "p4pc-run-mode.json"
SIDECAR = ".p4pc-mode.json"
RECEIPT = "mod-witness-receipt.json"
WITNESS_FIELDS = {"run_mode": "mod-witness", "mod_witness": True, "reference_eligible": False}


def receipt_fields(environment=None, sources=()):
    environment = os.environ if environment is None else environment
    value = environment.get(VARIABLE)
    if value is None:
        if any(witness_output(path) for path in sources):
            return dict(WITNESS_FIELDS)
        return {"run_mode": "ordinary", "mod_witness": False}
    if value != "1" or environment.get("P4PC_INPUT") != "script":
        raise ValueError("P4PC_MOD_WITNESS accepts only1 with P4PC_INPUT=script")
    return dict(WITNESS_FIELDS)


def refuse_fidelity(environment=None, overrides=()):
    environment = os.environ if environment is None else environment
    if VARIABLE in environment or any(value.partition("=")[0] == VARIABLE for value in overrides):
        raise ValueError("fidelity/retail execution refuses P4PC_MOD_WITNESS, even with both selectors disabled")


def _write(path, value):
    Path(path).write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _read(path):
    value = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(value, dict) or value.get("schema") != 1 or value.get("mod_witness") is not True or value.get("reference_eligible") is not False:
        raise ValueError("invalid mod-witness provenance: %s" % path)
    return value


def witness_output(path):
    path = Path(path)
    for candidate in {path.absolute(), path.resolve()}:
        sidecar = Path(str(candidate) + SIDECAR)
        if sidecar.exists():
            _read(sidecar)
            return True
        metadata = Path(str(candidate) + ".json")
        if metadata.is_file():
            record = json.loads(metadata.read_text(encoding="utf-8"))
            if isinstance(record, dict) and record.get("mod_witness") is True:
                return True
        parents = (candidate, *candidate.parents) if candidate.is_dir() else candidate.parents
        for parent in parents:
            marker = parent / MARKER
            if marker.exists():
                _read(marker)
                return True
    return False


def refuse_reference(path):
    if witness_output(path):
        raise ValueError("mod-witness output cannot be a retail reference: %s" % path)


def _digest(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def tag_output(path):
    path = Path(path)
    if path.is_symlink() or not path.is_file():
        raise ValueError("witness output is not an owned regular file: %s" % path)
    record = {"schema": 1, **WITNESS_FIELDS, "sha256": _digest(path)}
    _write(str(path) + SIDECAR, record)
    return record


def copy_output_mode(source, destination):
    """Preserve taint through copies/conversions, with the destination's real hash."""
    if witness_output(source):
        tag_output(destination)


def begin(root, environment=None):
    fields = receipt_fields(environment)
    root = Path(root)
    if not fields["mod_witness"]:
        if (root / MARKER).exists():
            raise ValueError("cannot reuse a mod-witness output root for an ordinary run")
        return False
    if root.is_symlink():
        raise ValueError("witness output root is a symlink")
    root.mkdir(parents=True, exist_ok=True)
    for name in ("P4PC_CAPTURE_PNG", "P4PC_CAPTURE_RGB"):
        value = (os.environ if environment is None else environment).get(name)
        if not value:
            continue
        output = Path(value)
        output = output if output.is_absolute() else root / output
        if not output.resolve().is_relative_to(root.resolve()):
            raise ValueError("%s must remain in the owned mod-witness output root" % name)
    _write(root / MARKER, {"schema": 1, **fields})
    return True


def finish(root, exit_code, outputs=None, environment=None, outcome=None):
    fields = receipt_fields(environment)
    if not fields["mod_witness"]:
        return None
    root = Path(root)
    _read(root / MARKER)  # a killed/partial run remains tainted from begin onward
    candidates = root.rglob("*") if outputs is None else (Path(p) for p in outputs)
    records = []
    for path in sorted(candidates):
        if path.is_symlink():
            raise ValueError("witness output contains a symlink: %s" % path)
        if not path.is_file() or path.name in (MARKER, RECEIPT) or path.name.endswith(SIDECAR):
            continue
        records.append({"path": str(path), **tag_output(path)})
    receipt = {"schema": 1, "kind": "mod-test", **fields,
               "observed_exit_code": exit_code, "outcome": outcome, "outputs": records}
    _write(root / RECEIPT, receipt)
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("begin", "finish", "fidelity", "validate"))
    parser.add_argument("--root", type=Path)
    parser.add_argument("--exit-code")
    parser.add_argument("--outcome")
    args = parser.parse_args()
    try:
        if args.action == "fidelity":
            refuse_fidelity()
        elif args.action == "validate":
            receipt_fields()
        elif args.root is None:
            parser.error("--root is required for witness output ownership")
        elif args.action == "begin":
            begin(args.root)
        else:
            finish(args.root, args.exit_code, outcome=args.outcome)
    except (OSError, ValueError) as exc:
        parser.exit(1, "mod_witness: %s\n" % exc)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
