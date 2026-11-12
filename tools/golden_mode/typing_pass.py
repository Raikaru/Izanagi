#!/usr/bin/env python3
"""Type every PS2 data1/data2 object and assign its Golden-mode supplier (Golden mode milestone 2).

Input is milestone 1's provenance.jsonl. Each object gets an element kind, an element width, a stride and a
row count where they can be measured, the evidence behind each, any Golden leads, and the supplier a
Golden-mode image builder would use. Nothing but metadata is written, outside the repository.

Evidence, strongest first:

  decl      decomp `extern`/definition types (`extern s16 D_00641C40[][19];`); `u8 x[]` placeholders are
            treated as no evidence
  access    widths of the retail code's loads and stores into the object (lb/lh/lw/ld/lq, lwc1/swc1 as
            f32, addiu/ori as address-taken), found with the same lui/%lo and gp-relative scan as M1
  content   word classes measured on the user's ELF: zero, plausible f32, small integer, code pointer
            (function start or a label inside a function: jump table), data pointer, ASCII and
            Shift-JIS text; the stride by periodicity of the nonzero-byte mask

Kinds: zero, scalar, f32-array, int8-array, int16-array, int32-array, int64-array, int128-array,
pointer-table, jump-table, function-pointer-table, ascii-text, sjis-text, struct-array (stride known),
struct (size only), c-source.

Golden leads: each object's 8-byte windows are looked up in the Golden haystacks (the Steam P4G.exe's
file-backed data sections and, optionally, small Golden archive objects); the best-voted placement is
verified byte by byte. identical = every byte equal; variant = at least half equal; partial = fewer.

Suppliers (every object gets exactly one):

  zero                 zeroed guest RAM
  decomp-c             already compiled from decomp C (M1 map ownership)
  symbols              pointer, function-pointer and jump tables: regenerated from the symbol lists
  golden-exe           identical bytes in the Steam P4G.exe's plaintext data sections (no balance risk)
  golden-archive       identical bytes in a Golden archive object
  golden-convert       a catalogued game table whose Golden source exists (M5 converter)
  decomp-literal       typed object to be carried as decomp C data (GATE C); `golden_check` is set when
                       game-logic code reads it and a Golden variant was found or may exist

  python tools/golden_mode/typing_pass.py --provenance <dir> --elf SLUS_217.82 --decomp <checkout> \
      [--golden-exe <Steam P4G.exe>] [--inventory <dir> --install <Golden folder>] --out <dir>
"""
import argparse
import bisect
import collections
import json
import math
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import provenance  # noqa: E402

REPO = HERE.parents[1]
WIDTH = {0x20: ('load', 1), 0x24: ('load', 1), 0x21: ('load', 2), 0x25: ('load', 2), 0x23: ('load', 4),
         0x27: ('load', 4), 0x22: ('load', 4), 0x26: ('load', 4), 0x37: ('load', 8), 0x1A: ('load', 8),
         0x1B: ('load', 8), 0x1E: ('load', 16), 0x36: ('load', 16), 0x31: ('loadf', 4), 0x35: ('loadf', 8),
         0x28: ('store', 1), 0x29: ('store', 2), 0x2B: ('store', 4), 0x2A: ('store', 4), 0x2E: ('store', 4),
         0x3F: ('store', 8), 0x1F: ('store', 16), 0x3E: ('store', 16), 0x39: ('storef', 4),
         0x3D: ('storef', 8), 0x09: ('address', 0), 0x0D: ('address', 0)}
BASIC = {'s8': 1, 'u8': 1, 'char': 1, 'signed char': 1, 'unsigned char': 1, 'bool': 1,
         's16': 2, 'u16': 2, 'short': 2, 'unsigned short': 2,
         's32': 4, 'u32': 4, 'int': 4, 'unsigned int': 4, 'long': 4, 'unsigned long': 4, 'f32': 4,
         'float': 4, 's64': 8, 'u64': 8, 'f64': 8, 'double': 8, 'long long': 8, 'u128': 16}
DECL = re.compile(r'(?:^|[;{}\n])\s*(?:extern\s+)?(?:static\s+)?(?:const\s+)?(?:volatile\s+)?'
                  r'((?:unsigned\s+|signed\s+)?[A-Za-z_]\w*)\s*(\*+)?\s*(?:const\s+)?([A-Za-z_]\w*)\s*'
                  r'((?:\[[^\]\n]*\])*)\s*(?:=|;)')
ADDRESS_NAME = re.compile(r'^(?:D|DAT|PTR_DAT|PTR)_(00[0-9A-Fa-f]{6})$')
GAME_MODULES = {'dat', 'btl', 'fcl', 'cmm', 'cmp', 'shd', 'fld', 'evt', 'cld', 'wshop', 'sequence', 'title',
                'y', 'sfl', 'scr', 'k', 'mt', 'fr', 'eff', 'mdl', 'camp', 'shop', 'dng', 'msg', 'itf'}


# ---- evidence -------------------------------------------------------------------------------------

def in_code(address):
    return any(lo <= address < hi for lo, hi in provenance.CODE)


def code_accesses(read, gp, functions, wanted=provenance.in_data):
    """{address: Counter((kind, width))} and {address: set(function start)} for code-formed addresses
    that satisfy `wanted` (data1/data2 by default; `in_code` audits reads of the code ranges)."""
    out = collections.defaultdict(collections.Counter)
    users = collections.defaultdict(set)
    starts = sorted(functions)
    for pc, op, address in provenance.formed_addresses(read, gp):
        if op in WIDTH and wanted(address):
            out[address][WIDTH[op]] += 1
            position = bisect.bisect_right(starts, pc) - 1
            if position >= 0:
                users[address].add(starts[position])
    return out, users


def code_range_reads(read, gp, functions):
    """Loads and stores the retail code forms into the code ranges (not address-taken function pointers).

    Golden mode leaves the code ranges zero (native code never executes MIPS bytes), so every such access
    is a data read of code bytes that the image builder would have to supply."""
    accesses, users = code_accesses(read, gp, functions, in_code)
    out = []
    for address in sorted(accesses):
        kinds = {k: v for k, v in accesses[address].items() if k[0] != 'address'}
        if kinds:
            out.append({'address': '0x%08x' % address,
                        'access': {'%s%d' % k: v for k, v in sorted(kinds.items())},
                        'functions': sorted('0x%08x' % f for f in users[address])})
    return out


def parse_declarations(roots):
    """{name: (base type, pointer depth, [dims])} from C sources: the first informative declaration wins."""
    out = {}
    for root in roots:
        root = Path(root)
        if not root.exists():
            continue
        for path in sorted(p for p in root.rglob('*') if p.suffix in ('.c', '.h') and p.is_file()):
            text = path.read_text(encoding='utf-8', errors='replace')
            for match in DECL.finditer(text):
                base, stars, name, dims = match[1], match[2] or '', match[3], match[4]
                dims = [d.strip() for d in re.findall(r'\[([^\]]*)\]', dims)]
                informative = stars or base not in ('u8', 'char', 'unsigned char') or (dims and dims[0])
                if base in ('return', 'goto', 'else', 'case', 'struct', 'enum', 'union', 'typedef'):
                    continue
                if name not in out or (informative and not out[name][3]):
                    out[name] = (base, len(stars), dims, bool(informative))
    return out


def declared(names, address, decls):
    """The informative declaration of an object's names or its D_<address> spelling, else None."""
    for name in list(names) + ['D_%08X' % address, 'D_%08x' % address, 'DAT_%08x' % address]:
        found = decls.get(name)
        if found and found[3]:
            return name, found
    return None


def word_classes(blob, start, function_starts):
    """Counter of aligned word classes, plus text measures."""
    c = collections.Counter()
    starts = function_starts
    for offset in range(-start % 4, len(blob) - 3, 4):
        v = struct.unpack_from('<I', blob, offset)[0]
        if v == 0:
            c['zero'] += 1
        elif provenance.code_pointer(v):
            c['function' if v in starts else 'label'] += 1
        elif provenance.in_data(v) or 0x00764280 <= v < 0x02000000:
            c['data_pointer'] += 1
        elif v < 0x10000:
            c['small'] += 1
        elif 107 <= (v >> 23) & 0xFF <= 147:
            c['float'] += 1
        else:
            c['other'] += 1
    c['ascii'] = sum(m.end() - m.start() for m in re.finditer(rb'[\x20-\x7e\t\n\r]{4,}\x00', blob))
    c['sjis'] = 2 * len(re.findall(rb'[\x81-\x9f\xe0-\xef][\x40-\x7e\x80-\xfc]', blob))
    return c


NONZERO_BIT = bytes([0x30] + [0x31] * 255)  # byte -> '0' / '1'


def stride_guess(blob, unit):
    """Smallest period (multiple of unit) whose nonzero-byte masks overlap with Jaccard >= 0.85.

    The mask is one big integer (bit n-1-i = byte i nonzero), so each period costs a few big-int ops."""
    n = len(blob)
    if n < 4:
        return None, 0.0
    mask = int(blob.translate(NONZERO_BIT), 2)
    ones = mask.bit_count()
    if ones < 8 or ones < 0.05 * n:
        return None, 0.0
    full = (1 << n) - 1
    for p in range(max(unit, 2), min(n // 2, 1024) + 1, max(unit, 1)):
        low = ~((1 << p) - 1)
        x = mask & low                      # byte i for i < n - p
        y = (mask << p) & full & low        # byte i + p, aligned with byte i
        union = (x | y).bit_count()
        if union and (x & y).bit_count() / union >= 0.85:
            return p, round((x & y).bit_count() / union, 3)
    return None, 0.0


def infer(obj, blob, accesses, decl, function_starts):
    """(kind, element width, stride, evidence) for one object."""
    size = obj['size']
    start = int(obj['address'], 16)
    classes = word_classes(blob, start, function_starts)
    nonzero_words = sum(v for k, v in classes.items() if k not in ('zero', 'ascii', 'sjis'))
    if obj['class'] == 'zero':
        return 'zero', None, None, 'content', classes
    if obj['class'] == 'c-source':
        return 'c-source', None, None, 'map', classes
    if decl:
        _, (base, stars, dims, _) = decl
        if stars:
            return 'pointer-table', 4, 4, 'decl', classes
        if base in BASIC:
            width = BASIC[base]
            kind = 'f32-array' if base in ('f32', 'float') else 'int%d-array' % (width * 8)
            stride = width
            inner = [d for d in dims[1:] if d.isdigit()]
            if inner:
                stride = width * math.prod(int(d) for d in inner)
            return kind, width, stride, 'decl', classes
        stride, _ = stride_guess(blob, 4)
        return ('struct-array' if stride else 'struct'), None, stride, 'decl:' + base, classes
    if nonzero_words and classes['function'] + classes['label'] == nonzero_words:
        return ('jump-table' if classes['label'] else 'function-pointer-table'), 4, 4, 'content', classes
    if nonzero_words and classes['function'] + classes['label'] + classes['data_pointer'] == nonzero_words:
        return 'pointer-table', 4, 4, 'content', classes
    loads = collections.Counter()
    for (kind, width), count in accesses.items():
        if kind != 'address':
            loads[('f' if kind.endswith('f') else 'i', width)] += count
    if size <= 8 and loads:
        return 'scalar', max(w for _, w in loads), size, 'access', classes
    if classes['ascii'] and classes['ascii'] + blob.count(0) >= size * 0.9:
        return 'ascii-text', 1, None, 'content', classes
    if classes['sjis'] >= size * 0.5:
        return 'sjis-text', 2, None, 'content', classes
    if loads and all(f == 'f' for f, _ in loads):
        return 'f32-array', 4, 4, 'access', classes
    if len({w for _, w in loads}) == 1 and not any(f == 'f' for f, _ in loads):
        width = next(iter(loads))[1]
        stride, _ = stride_guess(blob, width)
        kind = 'int%d-array' % (width * 8)
        return (kind if not stride or stride == width else 'struct-array'), width, stride or width, 'access', classes
    if nonzero_words and classes['float'] >= 0.9 * nonzero_words:
        return 'f32-array', 4, 4, 'content', classes
    stride, _ = stride_guess(blob, 1)
    if stride:
        return 'struct-array', None, stride, 'access+content' if loads else 'content', classes
    return 'struct', None, None, 'access' if loads else 'content', classes


# ---- Golden leads -------------------------------------------------------------------------------

def kmers(blob, k=8):
    """[(window as int, offset)] of every k-byte window holding at least 3 distinct byte values."""
    out = []
    for offset in range(len(blob) - k + 1):
        window = blob[offset:offset + k]
        if len(set(window)) >= 3:
            out.append((int.from_bytes(window, 'little'), offset))
    return out


def needle_index(objects, read, min_size=16, max_owners=16):
    """{8-byte window: [(object index, offset)]}; windows shared by more than max_owners places are dropped."""
    index = {}
    for number, obj in enumerate(objects):
        if obj['size'] < min_size:
            continue
        for value, offset in kmers(read(int(obj['address'], 16), obj['size'])):
            index.setdefault(value, []).append((number, offset))
    return {value: places for value, places in index.items() if len(places) <= max_owners}


def golden_leads(objects, read, haystacks, min_votes=2):
    """{object index: lead} for objects whose 8-byte windows appear in a haystack, best placement verified.

    haystacks: [(label, bytes)]. Haystack windows are taken at 4-byte alignment."""
    index = needle_index(objects, read)
    if not index:
        return {}
    votes = collections.Counter()
    for label, hay in haystacks:
        for phase in (0, 4):
            body = hay[phase:]
            body = body[:len(body) // 8 * 8]
            for number, (value,) in enumerate(struct.iter_unpack('<Q', body)):
                places = index.get(value)
                if places:
                    position = phase + number * 8
                    for owner, offset in places:
                        votes[(owner, label, position - offset)] += 1
    best = {}
    for (owner, label, base), count in votes.items():
        if count >= min_votes and count > best.get(owner, (0,))[0]:
            best[owner] = (count, label, base)
    hay_by_label = dict(haystacks)
    leads = {}
    for index, (count, label, base) in best.items():
        obj = objects[index]
        blob = read(int(obj['address'], 16), obj['size'])
        hay = hay_by_label[label]
        lo, hi = max(base, 0), min(base + len(blob), len(hay))
        segment = hay[lo:hi]
        equal = sum(1 for a, b in zip(segment, blob[lo - base:hi - base]) if a == b)
        fraction = equal / len(blob)
        leads[index] = {'where': label, 'offset': base, 'votes': count, 'equal_bytes': equal,
                        'match': 'identical' if equal == len(blob) else 'variant' if fraction >= 0.5 else 'partial'}
    return leads


def pe_data_sections(path, names=('.sdata', '.rdata', '.xpdata', '.shared')):
    """[(label, raw bytes)] of a PE's file-backed sections (no decryption, no unpacking)."""
    buf = Path(path).read_bytes()
    pe = struct.unpack_from('<I', buf, 0x3C)[0]
    count, optsize = struct.unpack_from('<H', buf, pe + 6)[0], struct.unpack_from('<H', buf, pe + 20)[0]
    out = []
    for i in range(count):
        name, _, _, rs, ro = struct.unpack_from('<8sIIII', buf, pe + 24 + optsize + 40 * i)
        name = name.rstrip(b'\0').decode('latin1')
        if name in names:
            out.append(('P4G.exe' + name, buf[ro:ro + rs]))
    return out


# ---- suppliers ----------------------------------------------------------------------------------

def module_of(consumers):
    modules = {(provenance.MODULE.match(n) or [n, n])[1] for n in consumers if not n.startswith('func_')}
    return modules


def supplier(obj, kind, lead):
    if obj['class'] == 'zero' or kind == 'zero':
        return 'zero', False
    if obj['class'] == 'c-source':
        return 'decomp-c', False
    if kind in ('pointer-table', 'jump-table', 'function-pointer-table'):
        return 'symbols', False
    if lead and lead['match'] == 'identical':
        return ('golden-exe' if lead['where'].startswith('P4G.exe') else 'golden-archive'), False
    if obj.get('table'):
        return 'golden-convert', False
    game = bool(module_of(obj.get('consumers', ())) & GAME_MODULES)
    # A partial lead (under half the bytes equal) is not evidence that Golden holds this object.
    return 'decomp-literal', game or bool(lead and lead['match'] == 'variant')


def run(objects, read, functions, decls, haystacks=()):
    function_starts = set(functions)
    accesses, _ = code_accesses(read, provenance.GP, functions)
    leads = golden_leads(objects, read, list(haystacks)) if haystacks else {}
    access_addresses = sorted(accesses)
    out = []
    for index, obj in enumerate(objects):
        start = int(obj['address'], 16)
        blob = read(start, obj['size'])
        counted = collections.Counter()
        lo = bisect.bisect_left(access_addresses, start)
        hi = bisect.bisect_left(access_addresses, start + obj['size'])
        for address in access_addresses[lo:hi]:
            counted.update(accesses[address])
        decl = declared(obj.get('names', ()), start, decls)
        kind, width, stride, evidence, classes = infer(obj, blob, counted, decl, function_starts)
        lead = leads.get(index)
        source, check = supplier(obj, kind, lead)
        row = {'address': obj['address'], 'size': obj['size'], 'range': obj['range'], 'm1_class': obj['class'],
               'kind': kind, 'evidence': evidence, 'supplier': source,
               'code_referenced': provenance.code_referenced(obj),
               'access': {'%s%d' % k: v for k, v in sorted(counted.items())},
               'words': {k: v for k, v in classes.items() if v}}
        if width:
            row['element_width'] = width
        if stride:
            row['stride'] = stride
            row['rows'] = obj['size'] // stride
        if decl:
            row['declared'] = {'name': decl[0], 'type': decl[1][0] + '*' * decl[1][1], 'dims': decl[1][2]}
        if lead:
            row['golden_lead'] = lead
        if check:
            row['golden_check'] = True
        for key in ('names', 'consumers', 'table'):
            if obj.get(key):
                row[key] = obj[key]
        out.append(row)
    return out


def summarize(rows):
    summary = {}
    for name, lo, hi in provenance.RANGES:
        part = [r for r in rows if r['range'] == name]
        by = lambda key, pick=lambda r: True: dict(collections.Counter(
            {k: sum(r['size'] for r in part if r[key] == k and pick(r)) for k in {r[key] for r in part}}).most_common())
        live = lambda r: r['code_referenced']
        summary[name] = {
            'bytes': hi - lo, 'objects': len(part),
            'bytes_by_kind': by('kind'), 'code_referenced_bytes_by_kind': by('kind', live),
            'bytes_by_supplier': by('supplier'), 'code_referenced_bytes_by_supplier': by('supplier', live),
            'golden_check_bytes': sum(r['size'] for r in part if r.get('golden_check')),
            'golden_check_code_referenced_bytes': sum(r['size'] for r in part if r.get('golden_check') and live(r)),
            'golden_leads': dict(collections.Counter(r['golden_lead']['match'] for r in part if 'golden_lead' in r)),
            'golden_lead_bytes': dict(collections.Counter({m: sum(r['size'] for r in part if r.get('golden_lead', {})
                                                                  .get('match') == m)
                                                           for m in ('identical', 'variant', 'partial')})),
            'untyped_code_referenced_bytes': sum(r['size'] for r in part if live(r) and r['kind'] == 'struct'
                                                 and r['evidence'] == 'content'),
            'code_referenced_without_supplier': sum(1 for r in part if live(r) and not r['supplier']),
        }
    return summary


def load_jsonl(path):
    with open(path, encoding='utf-8') as handle:
        return [json.loads(line) for line in handle if line.strip()]


def archive_haystacks(inventory_dir, install, max_size=4 << 20):
    """Small Golden archive objects (tables, scripts' data; no textures, models, audio or movies), in memory."""
    import inventory as inventory_tool
    skip = {'dds', 'tmx', 'spr', 'amd-chunk', 'cri-hca', 'cri-usm', 'flow-bf', 'bed', 'cri-afs2', 'cri-cpk',
            'magic:47585000', 'magic:80000024', 'gxt', 'png', 'pe', 'riff'}
    bulky = ('.epl', '.se', '.pm1', '.pm2', '.pm3', '.acb', '.ep')
    rows = load_jsonl(Path(inventory_dir) / 'inventory.jsonl')
    wanted = [r['path'] for r in rows if 'kind' not in r and r['depth'] >= 2 and r['archive'] in ('data.cpk', 'data_e.cpk')
              and r.get('signature') not in skip and r.get('size', 0) <= max_size
              and not r['path'].lower().endswith(bulky)]
    by_top = collections.defaultdict(list)
    for path in wanted:
        chain = path.split(inventory_tool.SEPARATOR)
        by_top[inventory_tool.SEPARATOR.join(chain[:2])].append(chain[2:])
    out = []
    for top, tails in sorted(by_top.items()):
        data = inventory_tool.read_object(install, top)
        for tail in tails:
            blob = data
            for name in tail:
                _, entries = inventory_tool.children(blob)
                match = [(o, s) for n, o, s in entries or () if n == name]
                if not match:
                    blob = None
                    break
                blob = blob[match[0][0]:match[0][0] + match[0][1]]
            if blob:
                out.append((inventory_tool.SEPARATOR.join([top] + tail), blob))
    return out


def check_output(out):
    out = Path(out).resolve()
    if out == REPO.resolve() or REPO.resolve() in out.parents:
        raise SystemExit('refusing to write typing output inside the repository')
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--provenance', required=True)
    parser.add_argument('--elf', required=True)
    parser.add_argument('--decomp', required=True)
    parser.add_argument('--port', default=str(REPO))
    parser.add_argument('--golden-exe', help="the Steam install's P4G.exe (file-backed data sections only)")
    parser.add_argument('--inventory', help='milestone-1 inventory directory (with --install)')
    parser.add_argument('--install', help='Golden install (with --inventory)')
    parser.add_argument('--out', required=True)
    args = parser.parse_args(argv)
    out = check_output(args.out)
    objects = load_jsonl(Path(args.provenance) / 'provenance.jsonl')
    read = provenance.load_elf_image(args.elf)
    functions = provenance.parse_functions(Path(args.decomp) / 'config' / 'symbol_addrs.txt')
    decls = parse_declarations([Path(args.decomp) / 'src', Path(args.decomp) / 'include',
                                Path(args.port) / 'src', Path(args.port) / 'overrides', Path(args.port) / 'include'])
    haystacks = []
    if args.golden_exe:
        haystacks += pe_data_sections(args.golden_exe)
    if args.inventory and args.install:
        haystacks += archive_haystacks(args.inventory, args.install)
    rows = run(objects, read, functions, decls, haystacks)
    out.mkdir(parents=True, exist_ok=True)
    with open(out / 'typed.jsonl', 'w', encoding='utf-8', newline='\n') as handle:
        for row in rows:
            handle.write(json.dumps(row, sort_keys=True) + '\n')
    summary = summarize(rows)
    reads = code_range_reads(read, provenance.GP, functions)
    summary['code_range_reads'] = {'addresses': len(reads), 'functions': len({f for r in reads for f in r['functions']}),
                                   'loads': sum(v for r in reads for k, v in r['access'].items() if k.startswith('load')),
                                   'stores': sum(v for r in reads for k, v in r['access'].items()
                                                 if k.startswith('store'))}
    with open(out / 'code-range-reads.jsonl', 'w', encoding='utf-8', newline='\n') as handle:
        for row in reads:
            handle.write(json.dumps(row, sort_keys=True) + '\n')
    with open(out / 'typed-summary.json', 'w', encoding='utf-8', newline='\n') as handle:
        json.dump(summary, handle, indent=1, sort_keys=True)
        handle.write('\n')
    for name, entry in summary.items():
        if name in ('data1', 'data2'):
            print(name, entry['bytes_by_supplier'], 'untyped code-ref', entry['untyped_code_referenced_bytes'])
    print('code-range reads', summary['code_range_reads'])
    return 0


if __name__ == '__main__':
    sys.exit(main())
