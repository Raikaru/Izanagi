#!/usr/bin/env python3
"""Map every live PS2 table and disc resource to its Golden candidate source (Golden mode milestone 1).

Inputs are the metadata the other milestone-1 tools wrote (inventory.jsonl,
provenance.jsonl), the port's table catalogue (config/game_tables.json) and,
read in memory only, the user's PS2 disc and Golden install. Output is
metadata: paths, sizes, span sizes, row arithmetic, signature labels, hash
equality booleans and match classes; no payload bytes.

Sections of sources.json:

  table_inputs    the nine packed/flat inputs the port's table service reads
                  (DATA.CVM INIT/ITEMTBL.BIN and eight BTL.CVM battle TBLs):
                  Golden candidates by name in every locale, span counts/sizes
  banks           each catalogued physical bank (game_tables.json): PS2 rows x
                  stride against the Golden span at the same block index
  image_tables    each image-resident game table (provenance class game-table):
                  Golden candidates by the reviewed name patterns in
                  image_table_candidates.json, else missing
  disc            every PS2 disc file: exact path, nested path, basename or no
                  Golden match; signature equality; byte identity; for packs,
                  the share of member names Golden's same-named pack also has;
                  with --elf, whether a path string in data1/data2 names it
                  (literally or as a printf format): static request evidence
  image_requests  counts of those literal and format strings

  python tools/golden_mode/sources.py --inventory <dir> --provenance <dir> \
      --install <Golden folder> --iso <PS2 disc> [--elf SLUS_217.82] --out <dir>
"""
import argparse
import collections
import fnmatch
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / 'models'))
import formats  # noqa: E402
import inventory as inventory_tool  # noqa: E402
import p4disc  # noqa: E402
import provenance as provenance_tool  # noqa: E402

REPO = HERE.parents[1]
SEPARATOR = inventory_tool.SEPARATOR
# Locale preference for a single "chosen" candidate: a locale archive overrides the common data.cpk for
# the same path (the Golden mode VFS rule), and English is the default locale.
LOCALE_ORDER = ('e', '', 'fr', 'de', 'it', 'es', 'k', 'c', 'ck')
CANDIDATES = HERE / 'image_table_candidates.json'


def tbl_spans(data):
    """Sizes of packed-bank spans (u32 length, payload, next span 16-aligned), or None when malformed.

    The same algorithm as p4_golden_tbl_spans; PS2 and Golden battle TBLs share the container."""
    out = []
    offset = 0
    while offset + 4 <= len(data):
        length = struct.unpack_from('<I', data, offset)[0]
        if not length and offset + 16 >= len(data):
            break
        if length > len(data) - offset - 4:
            return None
        out.append(length)
        offset = (offset + 4 + length + 15) & ~15
    return out


def leaf(path):
    """The innermost name of an inventory path, normalised ("battle/skill.tbl")."""
    return path.split(SEPARATOR)[-1].replace('\\', '/').lower().lstrip('/')


def basename(path):
    return path.rsplit('/', 1)[-1]


def locale_rank(row):
    locale = row.get('locale', '')
    return LOCALE_ORDER.index(locale) if locale in LOCALE_ORDER else len(LOCALE_ORDER)


def golden_index(rows):
    """({leaf path: [rows]}, {basename: [rows]}) over every inventory object below an archive."""
    by_leaf = collections.defaultdict(list)
    by_base = collections.defaultdict(list)
    for row in rows:
        if row['depth'] < 1:
            continue
        name = leaf(row['path'])
        by_leaf[name].append(row)
        by_base[basename(name)].append(row)
    for table in (by_leaf, by_base):
        for name in table:
            table[name].sort(key=lambda r: (locale_rank(r), r['depth'], r['path']))
    return by_leaf, by_base


def candidate_summary(row):
    return {'path': row['path'], 'locale': row.get('locale', ''), 'depth': row['depth'],
            'size': row.get('size'), 'signature': row.get('signature')}


def match_path(ps2_path, by_leaf, by_base):
    """(match class, candidate rows) for a normalised PS2 path ("init/itemtbl.bin")."""
    rows = by_leaf.get(ps2_path, [])
    top = [r for r in rows if r['depth'] == 1]
    if top:
        return 'exact-top-level', top
    if rows:
        return 'exact-nested', rows
    rows = by_base.get(basename(ps2_path), [])
    if rows:
        return 'basename', rows
    return 'none', []


def bank_rows(game_tables, ps2_spans, golden_spans):
    """Per catalogued physical bank: PS2 shape against the Golden span with the same block index."""
    out = []
    for table in game_tables.get('tables', []):
        for bank in table.get('physical_banks', []):
            label = bank['source_label'].lower()
            ps2 = ps2_spans.get(label)
            golden = golden_spans.get(label)
            block = bank['block']
            row = {'table': table['key'], 'role': bank['role'], 'source': bank['source_label'], 'block': block,
                   'ps2_bytes': bank['payload_bytes'], 'stride': bank.get('stride'), 'ps2_rows': bank.get('rows')}
            if ps2 is not None and block < len(ps2):
                row['ps2_span_bytes'] = ps2[block]
            if golden is None:
                row['golden'] = 'missing-file'
            elif block >= len(golden):
                row['golden'] = 'missing-block'
                row['golden_blocks'] = len(golden)
            else:
                size = golden[block]
                row['golden_bytes'] = size
                stride = bank.get('stride')
                if size == bank['payload_bytes']:
                    row['golden'] = 'same-size'
                elif stride and size % stride == 0:
                    row['golden'] = 'same-stride-other-rows'
                    row['golden_rows'] = size // stride
                else:
                    row['golden'] = 'different-layout'
                if stride and bank.get('rows'):
                    row['golden_stride_if_same_rows'] = size / bank['rows']
            out.append(row)
    return out


def image_strides(game_tables):
    """{provenance table key: PS2 row stride} for the catalogued image-resident tables."""
    out = {}
    for table in game_tables.get('tables', []):
        key = table['key']
        for bank in table.get('static_banks', []):
            out[key + '.' + bank['role']] = bank.get('stride')
        if table.get('gate_table'):
            out[key + '.gate'] = table['gate_table']['stride']
        special = table.get('special_recipes')
        if special:
            out[key + '.special_recipe_descriptors'] = special['descriptor_stride']
            for group in special.get('groups', []):
                out['%s.special_recipes.%d' % (key, group['group'])] = group['stride']
        for alias in table.get('chart_aliases', []):
            out['%s.chart.%s' % (key, alias['role'])] = alias.get('stride')
    out['player_exp_threshold'] = 4
    return out


def image_table_rows(provenance_rows, spec, rows, strides=None):
    """Per image-resident catalogued table: Golden candidates by the reviewed path globs, or missing.

    Globs match whole lowercase inventory paths ("*::init/fcltable.bin::*"); only the preferred locale's
    candidates are listed, each with the row count its size gives at the PS2 stride when that divides."""
    strides = strides or {}
    tables = collections.OrderedDict()
    for obj in provenance_rows:
        for key in ([obj['table']] if obj.get('table') else []) + obj.get('table_aliases', []):
            entry = tables.setdefault(key, {'table': key, 'address': obj['address'], 'bytes': 0, 'consumers': set()})
            if key == obj.get('table'):
                entry['bytes'] += obj['size']
                entry['consumers'].update(obj.get('consumers', ()))
    out = []
    for key, entry in tables.items():
        rule = next((r for r in spec.get('rules', []) if key == r['table'] or key.startswith(r['table'] + '.')), None)
        matched = []
        if rule:
            globs = [g.lower() for g in rule.get('golden', [])]
            matched = [r for r in rows if r['depth'] >= 1 and any(fnmatch.fnmatchcase(r['path'].lower(), g)
                                                                   for g in globs)]
        if matched:
            best = min(locale_rank(r) for r in matched)
            matched = sorted((r for r in matched if locale_rank(r) == best), key=lambda r: r['path'])
        stride = strides.get(key)
        candidates = []
        for r in matched:
            summary = candidate_summary(r)
            if stride and r.get('size') and r['size'] % stride == 0:
                summary['rows_at_ps2_stride'] = r['size'] // stride
            candidates.append(summary)
        out.append({'table': key, 'address': entry['address'], 'bytes': entry['bytes'], 'stride': stride,
                    'consumers': sorted(entry['consumers'])[:12],
                    'status': ('candidate' if candidates else 'missing') if rule else 'no-rule',
                    'note': rule.get('note', '') if rule else 'no reviewed candidate rule',
                    'candidates': candidates})
    return out


def pack_member_share(ps2_data, golden_children):
    """(PS2 member count, how many of their names the Golden pack also has) or None for a non-pack."""
    _, entries = formats.pak_entries(ps2_data)
    if not entries:
        return None
    names = {basename(leaf(n)) for n in golden_children}
    return len(entries), sum(1 for n, _, _ in entries if basename(n.lower()) in names)


PATH_STRING = re.compile(rb'[\x20-\x7e]{4,}(?=\x00)')
FORMAT = re.compile(r'%[-0 ]?(\d*)([dixXsc])')


def image_strings(elf):
    """NUL-terminated printable runs of at least 4 bytes in the ELF's data1/data2 (held in memory only)."""
    read = provenance_tool.load_elf_image(elf)
    return [m.group().decode('ascii') for _, lo, hi in provenance_tool.RANGES
            for m in PATH_STRING.finditer(read(lo, hi - lo))]


def normalize_request(text):
    """A path-like string as a lowercase container-relative key ("cdrom0:\\BATTLE\\X.TBL;1" -> "battle/x.tbl")."""
    text = text.strip().replace('\\', '/').lower()
    text = re.sub(r'^(?:cdrom0?:|host0?:)', '', text).split(';')[0]
    return text.lstrip('/')


def request_patterns(strings):
    """(literal keys, [(format string, compiled regex)]) from path-like image strings.

    A string is path-like when it holds '/' or '\\' or ends in a 2-4 character extension. printf
    conversions become wildcards (%03d -> three digits, %d -> digits, %x -> hex, %s -> anything). A
    format with fewer than 4 literal name characters ("%s/%s") would match any path and is dropped."""
    literals, patterns = set(), []
    for raw in strings:
        if '/' not in raw and '\\' not in raw and not re.search(r'\.[A-Za-z0-9]{2,4}$', raw):
            continue
        key = normalize_request(raw)
        if not key or ' ' in key:
            continue
        if '%' not in key:
            literals.add(key)
            continue
        if len(re.sub(r'[/.]', '', FORMAT.sub('', key))) < 4:
            continue
        parts, last = [], 0
        for match in FORMAT.finditer(key):
            parts.append(re.escape(key[last:match.start()]))
            width, kind = match[1], match[2]
            if kind in 'di':
                parts.append(r'\d{%d}' % int(width) if width else r'\d+')
            elif kind in 'xX':
                parts.append(r'[0-9a-f]{%d}' % int(width) if width else r'[0-9a-f]+')
            elif kind == 'c':
                parts.append('.')
            else:
                parts.append('.+')
            last = match.end()
        parts.append(re.escape(key[last:]))
        patterns.append((key, re.compile(''.join(parts) + r'\Z')))
    return literals, patterns


def named_by(ps2_path, literals, patterns):
    """'literal' when an image string names the path (or its key without the first directory),
    'format' when a printf-style image string produces it, else None."""
    tail = ps2_path.split('/', 1)[-1]
    if ps2_path in literals or tail in literals:
        return 'literal'
    for _, regex in patterns:
        if regex.match(ps2_path) or regex.match(tail):
            return 'format'
    return None


def disc_rows(disc_files, read, by_leaf, by_base, children_of, literals=(), patterns=()):
    """Per PS2 disc file: match class, chosen candidate, signature/byte identity, pack member share and
    whether an image string names it (static request evidence; constructed paths can still be missed)."""
    out = []
    for key in sorted(disc_files):
        label, _, path = key.partition('/')
        ps2_path = path.lower()
        data = read(key)
        kind, rows = match_path(ps2_path, by_leaf, by_base)
        row = {'key': key, 'container': label, 'path': ps2_path, 'size': len(data),
               'signature': formats.signature(data[:16]), 'match': kind, 'candidates': len(rows),
               'named_by': named_by(ps2_path, literals, patterns)}
        if rows:
            chosen = rows[0]
            digest = hashlib.sha256(data).hexdigest()
            row['golden'] = candidate_summary(chosen)
            row['same_signature'] = chosen.get('signature') == row['signature']
            row['identical_any'] = any(r.get('sha256') == digest for r in rows)
            share = pack_member_share(data, children_of.get(chosen['path'], ()))
            if share:
                row['pack_members'], row['pack_members_in_golden'] = share
        out.append(row)
    return out


def coverage(rows, field):
    """{group: Counter(match class)} for disc rows grouped by a field function."""
    out = collections.defaultdict(collections.Counter)
    for row in rows:
        out[field(row)][row['match']] += 1
    return {k: dict(v) for k, v in sorted(out.items())}


def extension(path):
    return path.rsplit('.', 1)[-1] if '.' in path.rsplit('/', 1)[-1] else ''


def format_pairs(rows, top=4):
    """{extension: {"<PS2 signature> -> <Golden signature>": count}} over matched disc rows: the format
    note per resource family (the same label on both sides suggests the same container family)."""
    out = collections.defaultdict(collections.Counter)
    for row in rows:
        if 'golden' in row:
            out[extension(row['path'])]['%s -> %s' % (row['signature'], row['golden']['signature'])] += 1
    return {k: dict(v.most_common(top)) for k, v in sorted(out.items())}


def load_jsonl(path):
    with open(path, encoding='utf-8') as handle:
        return [json.loads(line) for line in handle if line.strip()]


def check_output(out):
    out = Path(out).resolve()
    if out == REPO.resolve() or REPO.resolve() in out.parents:
        raise SystemExit('refusing to write source mapping inside the repository')
    return out


def table_input_rows(game_tables, read_ps2, by_leaf, by_base, read_golden):
    """The nine table inputs with PS2 and Golden span shapes; returns (rows, ps2 spans, golden spans)."""
    rows, ps2_spans, golden_spans = [], {}, {}
    for entry in game_tables['physical_resource_profile']['files']:
        label = entry['key'].lower()
        container = entry['provider'].split('.')[0]
        ps2 = read_ps2(container + '/' + entry['key'])
        spans = tbl_spans(ps2) if label.endswith('.tbl') else None
        if spans is not None:
            ps2_spans[label] = spans
        kind, candidates = match_path(label, by_leaf, by_base)
        row = {'input': entry['key'], 'provider': entry['provider'], 'ps2_bytes': len(ps2),
               'ps2_spans': spans, 'match': kind, 'candidates': [candidate_summary(c) for c in candidates]}
        if candidates:
            chosen = candidates[0]
            golden = read_golden(chosen['path'])
            gspans = tbl_spans(golden) if label.endswith('.tbl') else None
            row['chosen'] = chosen['path']
            row['golden_bytes'] = len(golden)
            row['golden_spans'] = gspans
            row['identical'] = hashlib.sha256(golden).hexdigest() == hashlib.sha256(ps2).hexdigest()
            row['locales'] = sorted({c.get('locale', '') for c in candidates})
            if gspans is not None:
                golden_spans[label] = gspans
        rows.append(row)
    return rows, ps2_spans, golden_spans


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--inventory', required=True, help='directory holding inventory.jsonl')
    parser.add_argument('--provenance', required=True, help='directory holding provenance.jsonl')
    parser.add_argument('--install', required=True)
    parser.add_argument('--iso', required=True)
    parser.add_argument('--port', default=str(REPO))
    parser.add_argument('--elf', help="the user's SLUS_217.82: image path strings mark statically named requests")
    parser.add_argument('--out', required=True)
    args = parser.parse_args(argv)
    out = check_output(args.out)
    rows = load_jsonl(Path(args.inventory) / 'inventory.jsonl')
    provenance_rows = load_jsonl(Path(args.provenance) / 'provenance.jsonl')
    game_tables = json.loads((Path(args.port) / 'config' / 'game_tables.json').read_text(encoding='utf-8'))
    spec = json.loads(CANDIDATES.read_text(encoding='utf-8'))
    by_leaf, by_base = golden_index(rows)
    children_of = collections.defaultdict(list)
    for row in rows:
        parent = row['path'].rsplit(SEPARATOR, 1)[0]
        if parent != row['path']:
            children_of[parent].append(row['path'])
    with p4disc.Disc(args.iso) as disc:
        inputs, ps2_spans, golden_spans = table_input_rows(
            game_tables, disc.read, by_leaf, by_base, lambda p: inventory_tool.read_object(args.install, p))
        literals, patterns = request_patterns(image_strings(args.elf)) if args.elf else (set(), [])
        disc_table = disc_rows(disc.files, disc.read, by_leaf, by_base, children_of, literals, patterns)
    named = [r for r in disc_table if r['named_by']]
    result = {'table_inputs': inputs,
              'banks': bank_rows(game_tables, ps2_spans, golden_spans),
              'image_tables': image_table_rows(provenance_rows, spec, rows, image_strides(game_tables)),
              'disc_coverage': {'by_container': coverage(disc_table, lambda r: r['container']),
                                'by_top_directory': coverage(disc_table, lambda r: r['container'] + '/' +
                                                             r['path'].split('/')[0]),
                                'by_extension': coverage(disc_table, lambda r: extension(r['path'])),
                                'image_named_by_container': coverage(named, lambda r: r['container']),
                                'format_pairs_by_extension': format_pairs(disc_table),
                                'identical_files': sum(1 for r in disc_table if r.get('identical_any'))},
              'image_requests': {'literal_strings': len(literals), 'format_strings': len(patterns),
                                 'disc_files_named': len(named),
                                 'literal_strings_naming_no_disc_file': len(literals - {
                                     k for r in disc_table for k in (r['path'], r['path'].split('/', 1)[-1])})}}
    out.mkdir(parents=True, exist_ok=True)
    with open(out / 'sources.json', 'w', encoding='utf-8', newline='\n') as handle:
        json.dump(result, handle, indent=1, sort_keys=True)
        handle.write('\n')
    with open(out / 'disc.jsonl', 'w', encoding='utf-8', newline='\n') as handle:
        for row in disc_table:
            handle.write(json.dumps(row, sort_keys=True) + '\n')
    matched = collections.Counter(r['match'] for r in disc_table)
    print('inputs', len(inputs), 'banks', len(result['banks']), 'image tables', len(result['image_tables']),
          'disc', dict(matched))
    return 0


if __name__ == '__main__':
    sys.exit(main())
