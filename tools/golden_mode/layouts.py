#!/usr/bin/env python3
"""Measure the record layouts of Golden's table families for the format specs (Golden mode milestone 3).

Families (docs/golden_mode/specs/):

  ftd      init_free.bin init/fclTable.bin members (*.ftd)    FTD container: u32 payload bytes, u32 rows,
  ctd      init_free.bin init/cmpTable.bin members (*.ctd)    8 zero bytes, payload, zero pad to 32
  dat      init.bin event/cmm.bin members (*.dat)             DAT container: u32 payload bytes, u32 rows,
                                                              payload, zero pad to 32
  tbl      init_free.bin battle/*.TBL                         packed banks: u32 length, payload, pad to 16
  itemtbl  init_free.bin init/itemtbl.bin                     sections: u32 rows, u32 1, u32 256, u32 1, rows

Per table: container facts, rows, row size, and inferred fields. Field inference is a heuristic over the
measured rows (f32 when at least 90% of non-zero aligned words are plausible floats; u32/u16 when a higher
byte varies; u8 otherwise; signed when the top bit is set in a value whose higher bytes are all 0xFF) and
is labelled `inferred` in the specs. Value facts are reduced to buckets (non-zero rows, distinct-value
bucket, power-of-two bound); no row is ever copied out. Locale variance comes from the M1 inventory
hashes.

  python tools/golden_mode/layouts.py --inventory <M1 inventory dir> --install <Golden folder> --out <dir>
  python tools/golden_mode/layouts.py --render <dir>/layouts.json      # markdown tables for the specs
"""
import argparse
import collections
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import formats  # noqa: E402

REPO = HERE.parents[1]
FAMILIES = {
    'ftd': ('init_free.bin', 'init/fclTable.bin', '.ftd'),
    'ctd': ('init_free.bin', 'init/cmpTable.bin', '.ctd'),
    'dat': ('init.bin', 'event/cmm.bin', '.dat'),
}
BATTLE = ('SKILL', 'UNIT', 'MSG', 'PERSONA', 'ENCOUNT', 'EFFECT', 'MODEL', 'AICALC')
ITEM_SECTIONS = (68, 24)
LOCALE = 'data_e.cpk'


# ---- containers -------------------------------------------------------------------------------------

def align32(value):
    return (value + 31) & ~31


def ftd_table(blob):
    """(rows, payload) of an FTD/CTD table, or None when the container does not hold.

    The member is exactly the 16-byte header plus payload rounded up to 32 bytes. Padding content is
    unspecified: Golden's files often carry a stale copy of the header there, so it is never read."""
    if len(blob) < 16:
        return None
    size, rows, pad0, pad1 = struct.unpack_from('<IIII', blob, 0)
    if pad0 or pad1 or len(blob) != align32(16 + size):
        return None
    return rows, blob[16:16 + size]


def dat_table(blob):
    """(rows, payload) of a cmm DAT table: 8-byte header, payload, rounded up to 32 (padding unspecified)."""
    if len(blob) < 8:
        return None
    size, rows = struct.unpack_from('<II', blob, 0)
    if len(blob) != align32(8 + size):
        return None
    return rows, blob[8:8 + size]


def tbl_spans(blob):
    """[(offset, payload)] of a packed-bank TBL, or None when malformed."""
    out, offset = [], 0
    while offset + 4 <= len(blob):
        length = struct.unpack_from('<I', blob, offset)[0]
        if not length and offset + 16 >= len(blob):
            break
        if length > len(blob) - offset - 4:
            return None
        out.append((offset + 4, blob[offset + 4:offset + 4 + length]))
        offset = (offset + 4 + length + 15) & ~15
    return out


def item_sections(blob, row_sizes=ITEM_SECTIONS):
    """[(rows, row size, payload)] of an ITEMTBL with the given section row sizes, or None."""
    out, offset = [], 0
    for row_size in row_sizes:
        if offset + 16 > len(blob):
            return None
        rows, one, wide, one2 = struct.unpack_from('<IIII', blob, offset)
        if (one, wide, one2) != (1, 256, 1) or offset + 16 + rows * row_size > len(blob):
            return None
        out.append((rows, row_size, blob[offset + 16:offset + 16 + rows * row_size]))
        offset += 16 + rows * row_size
    return out if offset == len(blob) else None


# ---- field inference ----------------------------------------------------------------------------------

def bucket_distinct(count):
    return '1' if count <= 1 else '2-16' if count <= 16 else '17-256' if count <= 256 else '>256'


def bound(value):
    """Smallest power of two above value (a range bucket, not the value)."""
    return 1 << max(value, 0).bit_length()


def plausible_float(word):
    exponent = (word >> 23) & 0xFF
    return word & 0x7FFFFFFF and 107 <= exponent <= 147


def column(rows, offset, width):
    return [int.from_bytes(r[offset:offset + width], 'little') for r in rows]


def infer_fields(payload, stride):
    """[{offset, width, type, nonzero_rows, distinct, bound}] for rows of `stride` bytes."""
    rows = [payload[i:i + stride] for i in range(0, len(payload) - stride + 1, stride)]
    fields, offset = [], 0
    while offset < stride:
        width, kind = 1, 'u8'
        if offset % 4 == 0 and offset + 4 <= stride:
            words = column(rows, offset, 4)
            nonzero = [w for w in words if w]
            if nonzero and sum(1 for w in nonzero if plausible_float(w)) >= 0.9 * len(nonzero) and \
                    any(w >> 16 for w in nonzero):
                width, kind = 4, 'f32'
            elif any(w >> 16 for w in nonzero) and not all(w >> 16 == 0xFFFF for w in nonzero if w >> 16) and \
                    not (max(w & 0xFFFF for w in words) < 0x1000 and max(w >> 16 for w in words) < 0x1000):
                width, kind = 4, 'u32'   # two small halves read as two u16 fields instead
        if width == 1 and offset % 2 == 0 and offset + 2 <= stride:
            halves = column(rows, offset, 2)
            low = {h & 0xFF for h in halves}
            high = [h >> 8 for h in halves]
            # A u16 has a small high byte carrying a low byte that uses its range; two small bytes stay u8.
            if any(high) and (max(high) < 0x10 or max(high) == 0xFF) and (len(low) > 32 or max(low) >= 0x80):
                width, kind = 2, 'u16'
        values = column(rows, offset, width)
        top = 1 << (8 * width - 1)
        if kind in ('u16', 'u32') and any(v & top for v in values):
            high = (1 << (8 * width)) - 1
            if all(v < 0x100 or v > high - 0x100 for v in values if v & top):
                kind = 's' + kind[1:]
        magnitude = [((1 << (8 * width)) - v) if kind.startswith('s') and v & top else v for v in values] \
            if kind != 'f32' else [0]
        fields.append({'offset': offset, 'width': width, 'type': kind,
                       'nonzero_rows': sum(1 for v in values if v), 'distinct': bucket_distinct(len(set(values))),
                       'bound': bound(max(magnitude)) if kind != 'f32' else None})
        offset += width
    return fields


TEXT = re.compile(rb'([\x20-\x7e\x80-\xfc]{3,})\x00+\Z')
LETTERS = re.compile(rb'[A-Za-z \x80-\xfc]')


def text_start(payload, stride):
    """Smallest row offset from which rows hold NUL-terminated, NUL-padded text.

    At least half the rows must be non-empty there, at least 80% of those must be a run of 3 or more
    printable bytes followed only by NULs, and at least 60% of the text bytes must be letters, spaces or
    non-ASCII (locale) bytes, so small numeric columns are not mistaken for text."""
    rows = [payload[i:i + stride] for i in range(0, len(payload) - stride + 1, stride)]
    for start in range(0, stride - 3):
        tails = [r[start:] for r in rows if any(r[start:])]
        if len(tails) * 2 < len(rows):
            continue
        texts = [m.group(1) for m in map(TEXT.match, tails) if m]
        letters = sum(len(LETTERS.findall(t)) for t in texts)
        if len(texts) >= 0.8 * len(tails) and letters >= 0.6 * sum(len(t) for t in texts):
            return start
    return None


def describe(name, rows, payload, extra=None):
    entry = {'name': name, 'rows': rows, 'payload_bytes': len(payload)}
    if rows and len(payload) % rows == 0:
        size = entry['row_size'] = len(payload) // rows
        start = text_start(payload, size)
        if start is None:
            entry['fields'] = infer_fields(payload, size)
        else:
            head = b''.join(payload[i:i + start] for i in range(0, len(payload), size))
            entry['fields'] = (infer_fields(head, start) if start else []) + [
                {'offset': start, 'width': size - start, 'type': 'text', 'nonzero_rows': None,
                 'distinct': None, 'bound': None}]
    else:
        entry['row_size'] = None
        entry['note'] = 'payload is not rows x fixed size (count is not a row count)'
    entry.update(extra or {})
    return entry


# ---- measurement ----------------------------------------------------------------------------------------

def locale_variance(inventory_rows):
    """{leaf path: number of distinct SHA-256 across data.cpk and the locale CPKs}."""
    hashes = collections.defaultdict(set)
    for row in inventory_rows:
        if row['depth'] >= 2 and row['archive'].startswith('data') and row['archive'] != 'data_movie.cpk':
            hashes[row['path'].split('::', 1)[1].lower()].add(row.get('sha256'))
    return {path: len(values) for path, values in hashes.items()}


def measure(read_object, inventory_rows):
    variance = locale_variance(inventory_rows)
    out = {'locale_archive': LOCALE}
    for family, (pack, member, suffix) in FAMILIES.items():
        parent = read_object('%s::%s::%s' % (LOCALE, pack, member))
        variant, entries = formats.pak_entries(parent)
        tables = []
        for name, offset, size in entries:
            blob = parent[offset:offset + size]
            parsed = (dat_table if family == 'dat' else ftd_table)(blob) if name.lower().endswith(suffix) else None
            key = ('%s::%s::%s' % (pack, member, name)).lower()
            if parsed is None:
                tables.append({'name': name, 'container': formats.signature(blob[:16]), 'size': size,
                               'locale_variants': variance.get(key)})
                continue
            rows, payload = parsed
            tables.append(describe(name, rows, payload, {'size': size, 'locale_variants': variance.get(key)}))
        out[family] = {'pack': '%s::%s' % (pack, member), 'pack_variant': variant, 'tables': tables}
    battle = []
    for table in BATTLE:
        name = 'battle/%s.TBL' % table
        blob = read_object('%s::init_free.bin::%s' % (LOCALE, name))
        spans = tbl_spans(blob)
        battle.append({'name': name, 'size': len(blob),
                       'spans': [{'block': i, 'offset': off, 'bytes': len(p)} for i, (off, p) in enumerate(spans or [])],
                       'locale_variants': variance.get(('init_free.bin::' + name).lower())})
    out['tbl'] = battle
    item = read_object('%s::init_free.bin::init/itemtbl.bin' % LOCALE)
    sections = item_sections(item)
    out['itemtbl'] = {'size': len(item), 'sections': [describe('section%d' % i, rows, payload, {'row_size_spec': size})
                                                      for i, (rows, size, payload) in enumerate(sections or [])],
                      'locale_variants': variance.get('init_free.bin::init/itemtbl.bin')}
    return out


# ---- rendering ----------------------------------------------------------------------------------------

def render_fields(fields):
    """'0:u16 2-3:zero 4:u32 8:char[26]': always-zero runs collapse to one range."""
    parts, zero = [], None
    for f in fields:
        if f['type'] != 'text' and f['nonzero_rows'] == 0:
            zero = (zero[0] if zero else f['offset'], f['offset'] + f['width'] - 1)
            continue
        if zero:
            parts.append('%X-%X:zero' % zero if zero[1] > zero[0] else '%X:zero' % zero[0])
            zero = None
        parts.append('%X:char[%d]' % (f['offset'], f['width']) if f['type'] == 'text' else
                     '%X:%s' % (f['offset'], f['type']))
    if zero:
        parts.append('%X-%X:zero' % zero if zero[1] > zero[0] else '%X:zero' % zero[0])
    return ' '.join(parts)


def render_family(family):
    lines = ['| Table | Rows | Row bytes | Locale variants | Inferred fields (hex offset:type) |',
             '|---|---:|---:|---:|---|']
    for t in family['tables']:
        if 'rows' not in t:
            lines.append('| `%s` | - | - | %s | not this container (%s) |' % (t['name'], t.get('locale_variants'),
                                                                          t['container']))
            continue
        fields = render_fields(t.get('fields', [])) or t.get('note', '')
        lines.append('| `%s` | %d | %s | %s | %s |' % (t['name'], t['rows'], t['row_size'] or '-',
                                                       t.get('locale_variants'), fields))
    return '\n'.join(lines)


def check_output(out):
    out = Path(out).resolve()
    if out == REPO.resolve() or REPO.resolve() in out.parents:
        raise SystemExit('refusing to write layouts inside the repository')
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--inventory')
    parser.add_argument('--install')
    parser.add_argument('--out')
    parser.add_argument('--render', help='layouts.json to render as markdown')
    args = parser.parse_args(argv)
    if args.render:
        data = json.loads(Path(args.render).read_text(encoding='utf-8'))
        for family in FAMILIES:
            print('### %s\n\n%s\n' % (family, render_family(data[family])))
        return 0
    if not (args.inventory and args.install and args.out):
        parser.error('--inventory, --install and --out are required to measure')
    import inventory
    out = check_output(args.out)
    rows = [json.loads(line) for line in open(Path(args.inventory) / 'inventory.jsonl', encoding='utf-8')]
    result = measure(lambda path: inventory.read_object(args.install, path), rows)
    out.mkdir(parents=True, exist_ok=True)
    (out / 'layouts.json').write_text(json.dumps(result, indent=1, sort_keys=True) + '\n', encoding='utf-8')
    print('ftd %d ctd %d dat %d tables' % tuple(len(result[f]['tables']) for f in FAMILIES))
    return 0


if __name__ == '__main__':
    sys.exit(main())
