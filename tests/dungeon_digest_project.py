#!/usr/bin/env python3
"""Project dungeon digest records onto their public, one-way hash fields."""
import sys

FIELDS = ("flip", "field", "step", "seed", "grid", "rng", "mt", "player", "units", "battle", "roll")
HASH_FIELDS = ("grid", "rng", "mt", "player", "units", "roll")


def project(line):
    words = line.split()
    if words[:2] != ["dungeon", "digest"] or tuple(words[2::2]) != FIELDS or len(words) != 24:
        raise ValueError("invalid dungeon digest row")
    values = dict(zip(words[2::2], words[3::2]))
    count, separator, unit_hash = values["units"].partition(":")
    if not separator or not count:
        raise ValueError("invalid dungeon unit digest")
    values["units"] = unit_hash
    for field in HASH_FIELDS:
        value = values[field]
        if len(value) != 8 or any(c not in "0123456789abcdefABCDEF" for c in value):
            raise ValueError(f"invalid {field} hash")
    return "dungeon digest " + " ".join(f"{field} {values[field]}" for field in HASH_FIELDS)


if __name__ == "__main__":
    try:
        for line in sys.stdin:
            print(project(line))
    except ValueError as exc:
        sys.exit(str(exc))
