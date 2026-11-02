#!/usr/bin/env python3
"""Pin private gate references by filename and SHA256; never put reference bytes in git."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import sys
import mod_witness

MANIFEST = Path(__file__).resolve().parents[1] / "tests" / "refs.sha256"
ROW = re.compile(r"([0-9a-f]{64})  ([A-Za-z0-9][A-Za-z0-9_.-]*\.rgb)")


def parse_manifest(text):
    pins = {}
    for number, line in enumerate(text.splitlines(), 1):
        if not line.strip() or line.startswith("#"):
            continue
        match = ROW.fullmatch(line)
        if not match:
            raise ValueError("invalid refs.sha256 row %d (expected SHA256, two spaces, basename.rgb)" % number)
        digest, name = match.groups()
        if name in pins:
            raise ValueError("duplicate reference name in refs.sha256: %s" % name)
        pins[name] = digest
    if not pins:
        raise ValueError("refs.sha256 has no reference pins")
    return pins


def load_manifest(path=MANIFEST):
    return parse_manifest(Path(path).read_text(encoding="ascii"))


def verify_file(path, pins):
    path = Path(path)
    mod_witness.refuse_fidelity()
    mod_witness.refuse_reference(path)
    if path.name not in pins:
        raise ValueError("%s is not pinned in tests/refs.sha256; commit its new filename and hash with the gate cutover" % path.name)
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    actual = digest.hexdigest()
    expected = pins[path.name]
    if actual != expected:
        raise ValueError("%s changed outside its commit: expected %s, got %s; restore the pinned bytes, do not rehash an existing name" % (path, expected, actual))


def verify_directory(directory, pins):
    mod_witness.refuse_fidelity()
    # Extra files may be staged for a future cutover; a gate must separately verify its selected file, so an unpinned one cannot be consumed.
    for name in pins:
        verify_file(Path(directory) / name, pins)


def check_history(main_ref, pins, cwd=None):
    mod_witness.refuse_fidelity()
    # Compare only this submission's pin delta, not newer main rows absent from a stale base.
    base = subprocess.run(["git", "merge-base", "HEAD", main_ref], cwd=cwd,
                          check=True, capture_output=True, text=True).stdout.strip()

    def pins_at(ref):
        # Absence before the contract is valid; a failed Git inspection is not.
        exists = subprocess.run(["git", "ls-tree", "--name-only", ref, "--", "tests/refs.sha256"], cwd=cwd,
                                check=True, capture_output=True, text=True).stdout.strip()
        if not exists:
            return {}
        text = subprocess.run(["git", "show", ref + ":tests/refs.sha256"], cwd=cwd,
                              check=True, capture_output=True, text=True).stdout
        return parse_manifest(text)

    original, current_main = pins_at(base), pins_at(main_ref)
    for name in sorted(original.keys() | pins.keys()):
        before, after = original.get(name), pins.get(name)
        if before == after:
            continue
        if before is not None or (name in current_main and after != current_main[name]):
            raise ValueError("immutable reference pin %s was changed or removed; append a new filename/hash and switch the gate instead" % name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=MANIFEST)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--refs-dir", type=Path, help="verify every pinned file in the private directory")
    mode.add_argument("--file", type=Path, help="verify the reference this gate is about to consume")
    mode.add_argument("--main-ref", help="reject pin changes introduced since the merge base with this git ref")
    args = parser.parse_args()
    try:
        try:
            pins = load_manifest(args.manifest)
        except FileNotFoundError:
            if not args.main_ref:
                raise
            # A stale pre-contract tree may lack the file; history still rejects actual deletion.
            pins = {}
        if args.main_ref:
            check_history(args.main_ref, pins)
        elif args.file:
            verify_file(args.file, pins)
        else:
            verify_directory(args.refs_dir, pins)
    except ValueError as exc:
        print("ref_guard: FAIL: %s" % exc, file=sys.stderr)
        return 1
    except (OSError, subprocess.CalledProcessError) as exc:
        print("ref_guard: inspection unavailable: %s" % exc, file=sys.stderr)
        return 2
    if args.refs_dir:
        print("ref_guard: %d pinned references verified in %s" % (len(pins), args.refs_dir))
    return 0


if __name__ == "__main__":
    sys.exit(main())
