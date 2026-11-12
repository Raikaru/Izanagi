#!/usr/bin/env python3
"""Compare source and loaded P4 save-image identity/party words; emit evidence JSON."""
import datetime
import hashlib
import json
from pathlib import Path
import struct
import sys


def state(path):
    data = Path(path).read_bytes()
    if len(data) < 56:
        raise ValueError("truncated save image")
    day, period = struct.unpack_from("<hH", data, 4)
    # P4's calendar has a fixed non-leap February, even across the year boundary.
    date = datetime.date(2001, 4, 1) + datetime.timedelta(days=day)
    party = None
    offset = 56
    while offset + 8 <= len(data):
        tag, length = struct.unpack_from("<II", data, offset)
        if tag in (0x2000, 0xffffffff):
            break
        if not length or length > len(data) - offset - 8:
            raise ValueError("invalid tagged section")
        if tag == 19:
            if length != 8:
                raise ValueError("unexpected party section size")
            party = struct.unpack_from("<4h", data, offset + 8)
        offset += 8 + length
    if party is None:
        raise ValueError("missing saved party section 19")
    return {
        "date": date.strftime("%m/%d"), "day_word": "0x%04x" % (day & 65535),
        "period_word": "0x%04x" % period, "level": data[12],
        "field_word": "0x%02x" % data[52], "room_word": "0x%02x" % data[53],
        "party_section_19_words": ["0x%04x" % (word & 65535) for word in party],
        "sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data),
    }


def main():
    before, loaded = state(sys.argv[1]), state(sys.argv[2])
    keys = ("day_word", "period_word", "level", "field_word", "room_word", "party_section_19_words")
    matching = all(before[key] == loaded[key] for key in keys)
    print(json.dumps({"source": before, "loaded": loaded, "identity_and_party_match": matching}, indent=2))
    return 0 if matching else 1


if __name__ == "__main__":
    raise SystemExit(main())
