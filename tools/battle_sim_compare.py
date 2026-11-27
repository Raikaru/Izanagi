#!/usr/bin/env python3
"""Compare C and Zig battle batches without normalizing any native output bytes."""
import argparse
import hashlib
import json
from pathlib import Path
import sys


class Difference(ValueError):
    pass


def load(path):
    return json.loads(path.read_bytes())


def compare(left, right):
    left, right = Path(left), Path(right)
    manifests = [load(root / "manifest.json") for root in (left, right)]
    if manifests[0]["scenario"] != manifests[1]["scenario"]:
        raise Difference("scenario inputs differ")
    scenario = manifests[0]["scenario"]
    expected = {"run-%020d" % index for index in range(scenario["seed_start"], scenario["seed_start"] + scenario["n"])}
    if not expected:
        raise Difference("empty scenario corpus")
    for root in (left, right):
        actual = {path.name for path in root.glob("run-*") if path.is_dir()}
        if actual != expected:
            raise Difference("%s: incomplete or unexpected runs" % root)
    hashes = []
    for name in sorted(expected):
        cases = [root / name for root in (left, right)]
        rows = [load(case / "run.json") for case in cases]
        if rows[0]["run_index"] != rows[1]["run_index"] or rows[0]["seeds"] != rows[1]["seeds"]:
            raise Difference(name + ": run index or seeds differ")
        for row in rows:
            attempts = row["attempts"]
            if (len(attempts) != 1 or attempts[0].get("status") != "recorded" or
                    attempts[0].get("exit_code") != 0 or
                    row["result"].get("outcome") not in {"win", "defeat", "escape", "scripted_conclusion"}):
                raise Difference(name + ": unsuccessful, censored or retried worker")
        for file in ("entry.bin", "result.json", "trace.jsonl"):
            paths = [case / "attempt-1" / file for case in cases]
            payloads = [path.read_bytes() for path in paths]
            if not payloads[0] or payloads[0] != payloads[1]:
                raise Difference(name + "/" + file + ": native bytes differ or are empty")
            if file == "result.json" and any(json.loads(payloads[0]) != row["result"] for row in rows):
                raise Difference(name + ": run receipt disagrees with native result")
            hashes.append({"path": name + "/attempt-1/" + file,
                           "sha256": hashlib.sha256(payloads[0]).hexdigest()})
    return {"schema": 1, "runs": len(expected), "files": hashes,
            "workers": [manifest["input_identity"]["port_executable"] for manifest in manifests]}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("c_batch", type=Path)
    parser.add_argument("zig_batch", type=Path)
    args = parser.parse_args(argv)
    try:
        receipt = compare(args.c_batch, args.zig_batch)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print("battle_sim_compare: FAIL: " + str(exc), file=sys.stderr)
        return 1
    print(json.dumps(receipt, sort_keys=True))
    print("battle_sim_compare: PASS: %d runs; entry, result and trace bytes identical" % receipt["runs"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
