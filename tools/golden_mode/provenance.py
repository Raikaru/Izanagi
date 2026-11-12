#!/usr/bin/env python3
"""Per-object provenance map of the PS2 executable's data1/data2 ranges (Golden mode milestone 1).

Golden mode boots without the PS2 ELF, so every object the image supplies must
be zero, generated from source, converted from the user's Golden files, or
reported missing. This tool partitions data1 and data2 into objects and says,
for each, where the bytes come from today and what Golden mode would need.

Boundaries are the union of: linker-map input sections (C object, incbin piece
or fill), decomp symbols (symbol_addrs, symbol_data_addrs, symbols_recovered),
the port's own placements (config/port_symbols*.txt), address-named
identifiers in decomp/port sources (D_/DAT_<addr>), addresses the retail code
forms (lui/%lo pairs and gp-relative accesses: the port runs a translation of
that code), data-range words stored inside data1/data2, and catalogued table
extents (config/game_tables.json, the decomp's g_data EXP table). An object
runs to the next boundary unless a catalogue gives its exact extent.

Per object it records the owner (map), names, reference counts (retail code,
data pointers, decomp C, port C and overrides), the retail functions forming
its addresses, and byte statistics measured on the user's ELF: zero bytes,
NUL-terminated ASCII bytes, aligned code- and data-pointer words. It never
records the bytes themselves. Classes (first that holds):

  zero            every byte zero: supplied by zeroed guest RAM
  game-table      a catalogued game-content table: needs a Golden source
  c-source        owned by a compiled C object in the map: generated from source
  code-pointers   only zero and code-address words: generated from symbols
  pointers        only zero, code- and data-address words: generated from symbols
  strings         only NUL-terminated ASCII (and zero padding)
  mixed-unknown   anything else: needs analysis before Golden mode can supply it

  python tools/golden_mode/provenance.py --elf SLUS_217.82 --map slus21782.map \
      --decomp <decomp checkout> --out <dir>
"""
import argparse
import bisect
import collections
import json
import re
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
RANGES = (('data1', 0x0052DA00, 0x0070C850), ('data2', 0x0070E140, 0x00764280))
GP = 0x007690F0  # decomp config/symbols_recovered.txt `_gp`
CODE = ((0x00100000, 0x0052DA00), (0x0070C850, 0x0070E140))
SYMBOL_LINE = re.compile(r'^\s*([A-Za-z_.$][\w.$]*)\s*=\s*(0x[0-9a-fA-F]+)\s*;(.*)$')
PORT_SYMBOL = re.compile(r'^\s*([A-Za-z_][\w]*)\s+(0x[0-9a-fA-F]+)\s+(data|func)\b')
MAP_SECTION = re.compile(r'^\s*(\.[\w.]+|\*fill\*)?\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)(?:\s+(\S.*))?$')
ADDRESS_NAME = re.compile(r'\b(?:D|DAT|PTR_DAT|PTR)_(00[0-9a-fA-F]{6})\b')
HEX_LITERAL = re.compile(r'\b0x(00[0-9a-fA-F]{6})\b')
IDENTIFIER = re.compile(r'\b[A-Za-z_]\w*\b')
MODULE = re.compile(r'_*([A-Z]+(?![a-z])|[A-Z]?[a-z]+)')
SOURCE_SUFFIXES = ('.c', '.h', '.cpp', '.inc', '.s')


def in_data(address):
    return any(lo <= address < hi for _, lo, hi in RANGES)


def range_of(address):
    for name, lo, hi in RANGES:
        if lo <= address < hi:
            return name, lo, hi
    return None


def parse_symbol_files(paths):
    """({address: set(names)}, {address: set(file names)}) for symbol lines inside the data ranges."""
    by_address = collections.defaultdict(set)
    sources = collections.defaultdict(set)
    for path in paths:
        path = Path(path)
        if not path.is_file():
            continue
        for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
            match = SYMBOL_LINE.match(line) or PORT_SYMBOL.match(line)
            if not match:
                continue
            name, address = match[1], int(match[2], 16)
            if in_data(address):
                by_address[address].add(name)
                sources[address].add(path.name)
    return by_address, sources


def parse_functions(path):
    """{start: name} of `name = 0xADDR; // type:func` lines (the decomp's function list)."""
    out = {}
    path = Path(path)
    if not path.is_file():
        return out
    for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
        match = SYMBOL_LINE.match(line)
        if match and 'type:func' in match[3]:
            out.setdefault(int(match[2], 16), match[1])
    return out


def parse_map(lines):
    """[(start, size, owner, object)] for input sections and fill inside the data ranges.

    owner: 'incbin' for data1_N.o/data2_N.o pieces, 'c-object' for compiled objects, 'fill' for *fill*.
    GNU ld may print the section name on its own line and the address on the next; both forms are read.
    """
    out = []
    pending = None
    for line in lines:
        stripped = line.rstrip('\n')
        match = MAP_SECTION.match(stripped)
        if not match or (match[1] is None and pending is None):
            head = stripped.strip()
            pending = head if head.startswith('.') and len(head.split()) == 1 else None
            continue
        section = match[1] or pending
        pending = None
        start, size = int(match[2], 16), int(match[3], 16)
        if not size or not in_data(start):
            continue
        obj = (match[4] or '').strip()
        if section == '*fill*':
            owner = 'fill'
        elif re.search(r'(?:^|/)data[12]_\d+\.o$', obj):
            owner = 'incbin'
        elif obj.endswith('.o'):
            owner = 'c-object'
        else:
            continue
        out.append((start, size, owner, obj.rsplit('/', 1)[-1]))
    return out


def catalog_extents(game_tables, g_data_exp=True):
    """[(start, size, key)] of catalogued image-resident game tables."""
    out = []
    if g_data_exp:
        out.append((0x005DD6E0, 396, 'player_exp_threshold'))
    for table in game_tables.get('tables', []):
        key = table['key']
        for bank in table.get('static_banks', []):
            if bank.get('rows') and bank.get('stride'):
                out.append((int(bank['address'], 16), bank['rows'] * bank['stride'], key + '.' + bank['role']))
            else:
                out.append((int(bank['address'], 16), None, key + '.' + bank['role']))
        gate = table.get('gate_table')
        if gate:
            out.append((int(gate['address'], 16), gate['rows'] * gate['stride'], key + '.gate'))
        special = table.get('special_recipes')
        if special:
            for group in special.get('groups', []):
                out.append((int(group['rows_address'], 16), group['payload_bytes'],
                            '%s.special_recipes.%d' % (key, group['group'])))
            out.append((int(special['descriptor'], 16), special['descriptor_stride'] * len(special.get('groups', [])),
                        key + '.special_recipe_descriptors'))
        for alias in table.get('chart_aliases', []):
            out.append((int(alias['address'], 16), None, '%s.chart.%s' % (key, alias['role'])))
    return [entry for entry in out if in_data(entry[0])]


def scan_references(roots, known_names):
    """(refs, named): refs {address: Counter(root label)} from address-named identifiers, data-range
    hex literals and known symbol names; named is the set of addresses spelled as D_/DAT_ identifiers.
    Only named addresses and symbols become object boundaries: a hex literal may be any constant."""
    refs = collections.defaultdict(collections.Counter)
    named = set()
    name_to_addresses = collections.defaultdict(set)
    for address, names in known_names.items():
        for name in names:
            name_to_addresses[name].add(address)
    for label, root in roots:
        root = Path(root)
        if not root.exists():
            continue
        files = [root] if root.is_file() else [p for p in root.rglob('*') if p.suffix in SOURCE_SUFFIXES and p.is_file()]
        for path in files:
            text = path.read_text(encoding='utf-8', errors='replace')
            seen = collections.Counter()
            for match in ADDRESS_NAME.finditer(text):
                address = int(match[1], 16)
                seen[address] += 1
                if in_data(address):
                    named.add(address)
            for match in HEX_LITERAL.finditer(text):
                seen[int(match[1], 16)] += 1
            for token in IDENTIFIER.findall(text):
                for address in name_to_addresses.get(token, ()):
                    seen[address] += 1
            for address, count in seen.items():
                if in_data(address):
                    refs[address][label] += count
    return refs, named


# I-type MIPS/EE opcodes whose immediate is a signed offset from rs (loads, stores, addiu); ori is unsigned.
MEMORY_OPS = {0x09, 0x1A, 0x1B, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,
              0x2B, 0x2E, 0x31, 0x35, 0x36, 0x37, 0x39, 0x3D, 0x3E, 0x3F}
# Opcodes writing GPR rt: immediate ALU ops and integer loads (lwc1/ldc1/lqc2 write coprocessor registers).
WRITES_RT = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x1A, 0x1B, 0x1E, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
             0x26, 0x27, 0x37}
# A call (jal, jalr) clobbers at, v0-v1, a0-a3, t0-t9 and ra; s0-s7, gp, sp and fp survive it.
CALLER_SAVED = tuple(range(1, 16)) + (24, 25, 31)
ORI = 0x0D
LUI = 0x0F
GP_REGISTER = 28
LUI_WINDOW = 32


def formed_addresses(read, gp):
    """Yield (pc, opcode, address) for every lui/%lo pair and gp-relative access in the code ranges.

    A lui's value lives until LUI_WINDOW instructions pass, any instruction writes its register
    (SPECIAL/MMI rd, immediate ALU ops and loads rt, mfc1/cfc1 rt) or a call clobbers it (caller-saved
    registers), so a register reused for other values never forms a stale address."""
    for lo, hi in CODE:
        code = read(lo, hi - lo)
        pending = {}
        call_slot = None
        for index in range(0, len(code) - 3, 4):
            word = struct.unpack_from('<I', code, index)[0]
            op, rs, rt, imm = word >> 26, (word >> 21) & 31, (word >> 16) & 31, word & 0xFFFF
            if op == LUI:
                pending[rt] = (imm << 16, index)
            elif op in MEMORY_OPS or op == ORI:
                offset = imm if op == ORI else (imm - 0x10000 if imm & 0x8000 else imm)
                base = gp if rs == GP_REGISTER else (pending[rs][0] if rs in pending and
                                                       index - pending[rs][1] <= LUI_WINDOW * 4 else None)
                if base is not None:
                    yield lo + index, op, (base + offset) & 0xFFFFFFFF
            if op in (0x00, 0x1C):
                pending.pop((word >> 11) & 31, None)
            elif op in WRITES_RT or (op == 0x11 and rs in (0, 1, 2)):
                pending.pop(rt, None)
            if call_slot == index:          # the call's delay slot has run: the callee clobbers now
                for register in CALLER_SAVED:
                    pending.pop(register, None)
            if op == 0x03 or (op == 0x00 and word & 0x3F == 0x09):
                call_slot = index + 4


def code_references(read, gp, functions=()):
    """(counts, consumers): Counter of data-range addresses the retail code forms (lui/%lo pairs and
    gp-relative accesses) and {address: set(function start)} for the sorted function starts given.

    The port runs a translation of this code, so an address it forms is one the port can read."""
    out = collections.Counter()
    consumers = collections.defaultdict(set)
    for pc, _, address in formed_addresses(read, gp):
        if in_data(address):
            out[address] += 1
            position = bisect.bisect_right(functions, pc) - 1
            if position >= 0:
                consumers[address].add(functions[position])
    return out, consumers


def data_pointer_targets(read):
    """Counter of data-range addresses stored as aligned words inside data1/data2 (pointer-like words)."""
    out = collections.Counter()
    for _, lo, hi in RANGES:
        blob = read(lo, hi - lo)
        for offset in range(0, len(blob) - 3, 4):
            value = struct.unpack_from('<I', blob, offset)[0]
            if in_data(value):
                out[value] += 1
    return out


def code_pointer(value):
    return value & 3 == 0 and any(lo <= value < hi for lo, hi in CODE)


def byte_stats(data, start=0):
    """Counts only: zero bytes, ASCII-string bytes (runs >= 4 + NUL) and, over guest-aligned words,
    nonzero words, code-pointer words and data-pointer words."""
    zero = data.count(0)
    ascii_bytes = sum(m.end() - m.start() for m in re.finditer(rb'[\x20-\x7e\t\n\r]{4,}\x00', data))
    code = pointers = nonzero = words = 0
    for offset in range(-start % 4, len(data) - 3, 4):
        value = struct.unpack_from('<I', data, offset)[0]
        words += 1
        if value:
            nonzero += 1
            if code_pointer(value):
                code += 1
            elif in_data(value):
                pointers += 1
    return {'zero': zero, 'ascii': ascii_bytes, 'code_pointer_words': code, 'data_pointer_words': pointers,
            'words': words, 'nonzero_words': nonzero}


def classify(obj):
    stats = obj['stats']
    size = obj['size']
    if stats['zero'] == size:
        return 'zero'
    if obj.get('table'):
        return 'game-table'
    if obj['owner'] == 'c-object':
        return 'c-source'
    if stats['code_pointer_words'] and stats['code_pointer_words'] == stats['nonzero_words']:
        return 'code-pointers'
    if stats['nonzero_words'] and stats['code_pointer_words'] + stats['data_pointer_words'] == stats['nonzero_words']:
        return 'pointers'
    if stats['ascii'] and stats['ascii'] + stats['zero'] >= size:
        return 'strings'
    return 'mixed-unknown'


def load_elf_image(elf):
    """{guest address -> byte} accessor over every PT_LOAD of a MIPS ELF32."""
    data = Path(elf).read_bytes()
    if data[:4] != b'\x7fELF' or data[4] != 1 or data[5] != 1:
        raise SystemExit('not a little-endian ELF32')
    phoff, = struct.unpack_from('<I', data, 0x1C)
    phentsize, phnum = struct.unpack_from('<HH', data, 0x2A)
    segments = []
    for i in range(phnum):
        kind, offset, vaddr, _, filesz, _, _, _ = struct.unpack_from('<8I', data, phoff + i * phentsize)
        if kind == 1 and filesz:
            segments.append((vaddr, data[offset:offset + filesz]))

    def read(address, size):
        for vaddr, blob in segments:
            if vaddr <= address and address + size <= vaddr + len(blob):
                return blob[address - vaddr:address - vaddr + size]
        raise ValueError('address 0x%08x+%d outside the image' % (address, size))
    return read


def build(read, sections, symbols, symbol_sources, tables, refs, starts=(), consumers=None):
    """Partition data1/data2 into objects with provenance and classes.

    refs: {address: Counter(label)}, summed over each object's [start, end). starts: further evidenced
    object starts (named identifiers, retail-code targets, data-pointer targets). consumers:
    {address: set(function name)} of retail functions forming that address."""
    consumers = consumers or {}
    boundaries = set()
    for name, lo, hi in RANGES:
        boundaries.update((lo, hi))
    for start, size, _, _ in sections:
        boundaries.update((start, start + size))
    boundaries.update(symbols)
    boundaries.update(starts)
    for start, size, _ in tables:
        boundaries.add(start)
        if size:
            boundaries.add(start + size)
    ordered = sorted(b for b in boundaries if any(lo <= b <= hi for _, lo, hi in RANGES))
    section_starts = [s[0] for s in sections]
    ref_addresses = sorted(refs)
    objects = []
    for start, end in zip(ordered, ordered[1:]):
        located = range_of(start)
        if not located or end > located[2]:
            continue
        owner, obj_name = 'unmapped', None
        index = bisect.bisect_right(section_starts, start) - 1
        if index >= 0:
            s_start, s_size, s_owner, s_obj = sections[index]
            if s_start <= start < s_start + s_size:
                owner, obj_name = s_owner, s_obj
        size = end - start
        counted = collections.Counter()
        users = set()
        for address in ref_addresses[bisect.bisect_left(ref_addresses, start):bisect.bisect_left(ref_addresses, end)]:
            counted.update(refs[address])
            users.update(consumers.get(address, ()))
        obj = {'range': located[0], 'address': '0x%08x' % start, 'size': size, 'owner': owner,
               'names': sorted(symbols.get(start, ())), 'symbol_files': sorted(symbol_sources.get(start, ())),
               'refs': dict(sorted(counted.items())), 'stats': byte_stats(read(start, size), start)}
        if users:
            obj['consumers'] = sorted(users)
        if obj_name:
            obj['object'] = obj_name
        # A catalogued extent owns its bytes; a start-only entry (an index alias such as a fusion chart)
        # names the object only when no extent covers it, and is otherwise kept as an alias.
        sized = [key for t_start, t_size, key in tables if t_size and t_start <= start < t_start + t_size]
        aliases = [key for t_start, t_size, key in tables if not t_size and t_start == start]
        if sized:
            obj['table'], obj['extent_basis'] = sized[-1], 'catalog'
        elif aliases:
            obj['table'], obj['extent_basis'] = aliases.pop(0), 'catalog-start-next-boundary'
        if aliases:
            obj['table_aliases'] = aliases
        obj.setdefault('extent_basis', 'next-boundary')
        obj['class'] = classify(obj)
        objects.append(obj)
    return objects


def code_referenced(obj):
    """Read by code: the retail code (which the port runs translated) or decomp/port source names it."""
    return any(label != 'data_pointer' for label in obj['refs'])


def consumer_module(obj):
    """The leading word of the retail functions forming its addresses ('dat' for datPersona, 'Rp' for
    RpSkyTexCacheRasterLock, 'ACSSND'), 'unnamed' for func_<addr> only, 'multiple' when they differ,
    'none' without code use."""
    names = obj.get('consumers', ())
    if not names:
        return 'none'
    modules = {(MODULE.match(n) or [n, n])[1] for n in names if not n.startswith('func_')}
    if not modules:
        return 'unnamed'
    return modules.pop() if len(modules) == 1 else 'multiple'


def summarize(objects):
    out = {}
    for name, lo, hi in RANGES:
        rows = [o for o in objects if o['range'] == name]
        by_class = collections.Counter()
        live_by_class = collections.Counter()
        by_owner = collections.Counter()
        unknown_by_module = collections.Counter()
        for o in rows:
            by_class[o['class']] += o['size']
            by_owner[o['owner']] += o['size']
            if code_referenced(o):
                live_by_class[o['class']] += o['size']
            if o['class'] == 'mixed-unknown':
                unknown_by_module[consumer_module(o)] += o['size']
        out[name] = {'bytes': hi - lo, 'covered': sum(o['size'] for o in rows), 'objects': len(rows),
                     'named_objects': sum(1 for o in rows if o['names']),
                     'code_referenced_objects': sum(1 for o in rows if code_referenced(o)),
                     'objects_by_class': dict(collections.Counter(o['class'] for o in rows).most_common()),
                     'bytes_by_class': dict(by_class.most_common()),
                     'code_referenced_bytes_by_class': dict(live_by_class.most_common()),
                     'bytes_by_owner': dict(by_owner.most_common()),
                     'mixed_unknown_bytes_by_consumer_module': dict(unknown_by_module.most_common())}
    return out


def check_output(out):
    out = Path(out).resolve()
    if out == REPO.resolve() or REPO.resolve() in out.parents:
        raise SystemExit('refusing to write provenance inside the repository')
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--elf', required=True, help="the user's SLUS_217.82")
    parser.add_argument('--map', required=True, help='a GNU ld map of the decomp build')
    parser.add_argument('--decomp', required=True, help='decomp checkout (config/ symbol lists, src/, include/)')
    parser.add_argument('--port', default=str(REPO), help='port checkout (config/, src/, overrides/, include/)')
    parser.add_argument('--out', required=True)
    args = parser.parse_args(argv)
    out = check_output(args.out)
    decomp, port = Path(args.decomp), Path(args.port)
    symbols, symbol_sources = parse_symbol_files(
        [decomp / 'config' / n for n in ('symbol_addrs.txt', 'symbol_data_addrs.txt', 'symbols_recovered.txt')] +
        [port / 'config' / n for n in ('port_symbols.txt', 'port_symbols_manual.txt')])
    sections = sorted(parse_map(Path(args.map).read_text(encoding='utf-8', errors='replace').splitlines()))
    tables = catalog_extents(json.loads((port / 'config' / 'game_tables.json').read_text(encoding='utf-8')))
    refs, named = scan_references([('decomp_c', decomp / 'src'), ('decomp_include', decomp / 'include'),
                                   ('port_src', port / 'src'), ('port_overrides', port / 'overrides'),
                                   ('port_include', port / 'include')], symbols)
    read = load_elf_image(args.elf)
    functions = parse_functions(decomp / 'config' / 'symbol_addrs.txt')
    retail_code, consumers = code_references(read, GP, sorted(functions))
    pointers = data_pointer_targets(read)
    for address, count in retail_code.items():
        refs[address]['retail_code'] += count
    for address, count in pointers.items():
        refs[address]['data_pointer'] += count
    objects = build(read, sections, symbols, symbol_sources, tables, refs,
                    starts=named | set(retail_code) | set(pointers),
                    consumers={a: {functions[f] for f in fs} for a, fs in consumers.items()})
    out.mkdir(parents=True, exist_ok=True)
    with open(out / 'provenance.jsonl', 'w', encoding='utf-8', newline='\n') as handle:
        for obj in objects:
            handle.write(json.dumps(obj, sort_keys=True) + '\n')
    summary = summarize(objects)
    with open(out / 'provenance-summary.json', 'w', encoding='utf-8', newline='\n') as handle:
        json.dump(summary, handle, indent=1, sort_keys=True)
        handle.write('\n')
    for name, entry in summary.items():
        print(name, entry['objects'], 'objects', entry['bytes_by_class'])
    return 0


if __name__ == '__main__':
    sys.exit(main())
