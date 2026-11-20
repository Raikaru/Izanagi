#!/usr/bin/env python3
"""Derive a private policy-6 ratings table from the user's retail disc.

Ratings count each Persona's native learned offensive skills per retail element;
they are a transparent catalog heuristic, not the skill-affinity mod's ratings.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))
import retail_tables

ELEMENTS = ('phys', 'fire', 'ice', 'elec', 'wind', 'almighty', 'light', 'dark')


def derive(persona, growth, skill_elements, skills):
    if len(persona) < 256 * 14 or len(growth) < 256 * 70 or len(skill_elements) < 0x1b8 * 2 or len(skills) < 0x1b8 * 40:
        raise ValueError('truncated retail catalog')
    rows = {}
    # The shipped randomizer's retail fusion eligibility, with the starter
    # admitted explicitly. C0..FF are companions, enemy or unused rows.
    for identity in range(1, 0xc0):
        meta = identity * 14
        flags = struct.unpack_from('<H', persona, meta)[0]
        if identity != 1 and (flags & 0x3d or not persona[meta + 2] or not 1 <= persona[meta + 3] <= 98):
            continue
        counts = [0] * len(ELEMENTS)
        seen = set()
        for slot in range(16):
            at = identity * 70 + 6 + slot * 4
            kind = growth[at + 1]
            if kind == 0:
                break
            skill = struct.unpack_from('<H', growth, at + 2)[0]
            if kind != 1 or skill >= 0x1b8 or skill in seen:
                continue
            seen.add(skill)
            element = skill_elements[skill * 2]
            if element < len(ELEMENTS) and skills[skill * 40 + 0x11] == 1:
                counts[element] += 1
        rows[f'0x{identity:x}'] = dict(zip(ELEMENTS, counts))
    if not rows:
        raise ValueError('catalog contains no valid Persona rows')
    digest = hashlib.sha256(persona + growth + skill_elements + skills).hexdigest()
    return {'digest': digest, 'method': 'native-offensive-learnset-count-v1', 'personas': rows}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path, help='new private output file')
    args = parser.parse_args()
    files = retail_tables.battle_files(args.iso, ['PERSONA.TBL', 'SKILL.TBL'])
    persona = retail_tables.blocks(files['PERSONA.TBL'])
    skill = retail_tables.blocks(files['SKILL.TBL'])
    table = derive(persona[0], persona[1], skill[0], skill[1])
    data = (json.dumps(table, sort_keys=True, indent=1) + '\n').encode('utf-8')
    with args.out.open('xb') as output:
        output.write(data)
    print('ratings sha256=' + hashlib.sha256(data).hexdigest())


if __name__ == '__main__':
    main()
