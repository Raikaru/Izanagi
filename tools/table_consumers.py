#!/usr/bin/env python3
"""Produce source-backed expansion audit candidates, never a safety certificate.

Scan decomp, port overrides/headers and optionally the actual compiled mirror.
Every row retains its source identity, enclosing addressed function when known,
reason and line. Numeric hits are scoped by table-specific file/function names
or a direct table symbol in the same function, not attributed globally. Inline
assembly includes remain explicit unresolved evidence. No retail data is read.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

NUMBER = re.compile(r"(?<![\w.])(?:0[xX][0-9a-fA-F]+|[0-9]+)(?:[uUlL]*)(?![\w.])")
IDENTIFIER = re.compile(r"\b[A-Za-z_]\w*\b")
ADDRESS = re.compile(r"(?:FUN_|func_|0x)([0-9a-fA-F]{8})\b")
FUNCTION = re.compile(r"^\s*(?:[A-Za-z_]\w*[\s*]+)+([A-Za-z_]\w*)\s*\([^;{}]*\)\s*(?:\{\s*)?$")
NARROW = re.compile(r"\b(?:u8|s8|u16|s16|uint8_t|int8_t|uint16_t|int16_t)\b|&\s*(?:0[xX](?:ff|ffff)\b|(?:255|65535)\b)", re.I)
STORAGE = re.compile(r"\b(?:memcpy|memset|malloc|calloc|realloc|chkmemAllocSiteTag)\b|\[[^\]]*(?:0[xX][0-9a-fA-F]+|\d+)[^\]]*\]")


def clean_lines(text):
    """Keep line/column positions but remove comments and literals."""
    pattern = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
    return pattern.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text).splitlines()


def contexts(text):
    """Track top-level C bodies; marker-only/asm rows are not silently dropped."""
    original = text.splitlines()
    cleaned = clean_lines(text)
    result = []
    depth = 0
    name = None
    address = None
    pending_name = None
    pending_address = None
    for line, raw in zip(cleaned, original):
        if depth == 0:
            marker = ADDRESS.search(raw)
            if marker:
                pending_address = marker[1].lower()
            declaration = FUNCTION.match(line)
            if declaration and declaration[1] not in ('if', 'while', 'switch', 'for'):
                pending_name = declaration[1]
            if '{' in line and pending_name:
                name, address = pending_name, pending_address
                pending_name = pending_address = None
        result.append((name, address))
        depth += line.count('{') - line.count('}')
        if depth <= 0:
            depth = 0
            name = address = None
            if ';' in line:
                pending_name = None
    return cleaned, result


def scan_text(path, text, catalog, source):
    terms = {term for table in catalog['tables'] for term in table['symbols'] + table['scope']}
    if not re.search('|'.join(re.escape(term) for term in sorted(terms)), path + '\n' + text, re.I):
        return []
    cleaned, owners = contexts(text)
    original = text.splitlines()
    tokens = [set(IDENTIFIER.findall(line)) for line in cleaned]
    scope_cache = {}
    # Directly referenced symbols associate only their own enclosing function.
    direct = {}
    for table in catalog['tables']:
        symbols = set(table['symbols'])
        direct[table['key']] = {owner for identifiers, owner in zip(tokens, owners)
                                if owner[0] and symbols.intersection(identifiers)}
    rows = []
    for index, (line, owner) in enumerate(zip(cleaned, owners)):
        identifiers = tokens[index]
        numbers = {int(re.sub(r'[uUlL]+$', '', m[0]), 0) if m[0].lower().startswith('0x')
                   else int(re.sub(r'[uUlL]+$', '', m[0]), 10) for m in NUMBER.finditer(line)}
        for table in catalog['tables']:
            reasons = []
            symbols = sorted(identifiers.intersection(table['symbols']))
            scope_key = (table['key'], owner)
            if scope_key not in scope_cache:
                context = (path + ':' + (owner[0] or '')).lower()
                scope_cache[scope_key] = owner in direct[table['key']] or any(s.lower() in context for s in table['scope'])
            scoped = scope_cache[scope_key]
            if symbols:
                reasons.append({'kind': 'direct-symbol', 'values': symbols})
            if scoped:
                constants = sorted(numbers.intersection(table['constants']))
                if constants:
                    reasons.append({'kind': 'domain-constant-candidate', 'values': constants})
                if NARROW.search(line):
                    reasons.append({'kind': 'narrow-field-or-cast-candidate'})
                if STORAGE.search(line):
                    reasons.append({'kind': 'storage-or-copy-candidate'})
                if 'INCLUDE_ASM' in original[index]:
                    reasons.append({'kind': 'untranslated-body'})
            if reasons:
                rows.append({'table': table['key'], 'source': source, 'path': path, 'line': index + 1,
                             'function': owner[0], 'address': owner[1], 'reasons': reasons,
                             'text': original[index].strip(), 'status': 'unreviewed'})
    return rows


def inventory(root, catalog, mirrors=()):
    roots = [('decomp', root / 'decomp' / 'src'), ('decomp-header', root / 'decomp' / 'include'),
             ('override', root / 'overrides'), ('port', root / 'src'), ('port-header', root / 'include')]
    roots.extend((f'mirror-{i}', Path(mirror)) for i, mirror in enumerate(mirrors))
    rows = []
    inputs = []
    for source, directory in roots:
        if not directory.is_dir():
            raise ValueError(f'missing input directory: {directory}')
        for path in sorted(directory.rglob('*')):
            if path.suffix not in ('.c', '.h', '.inc') or not path.is_file():
                continue
            data = path.read_bytes()
            relative = path.relative_to(directory).as_posix()
            try:
                text = data.decode('utf-8', errors='strict')
                encoding = 'utf-8'
            except UnicodeDecodeError:
                # Some inherited comments contain legacy bytes. Preserve those
                # bytes visibly as \\xNN, not a guessed codepage or dropped text.
                text = data.decode('utf-8', errors='backslashreplace')
                encoding = 'utf-8-with-escaped-non-utf8-bytes'
            inputs.append({'source': source, 'path': relative, 'sha256': hashlib.sha256(data).hexdigest(),
                           'encoding': encoding})
            rows.extend(scan_text(relative, text, catalog, source))
    return {'schema': 1, 'catalog_sha256': hashlib.sha256(json.dumps(catalog, sort_keys=True).encode()).hexdigest(),
            'status': 'unreviewed-candidates', 'compiled_mirror_scanned': bool(mirrors),
            'limitations': ['Numeric and narrow-field rows are candidates, not proof of count dependence.',
                            'Function pointers, scripts/resources, asm and indirect aliases require semantic review.',
                            'No expansion capability may be enabled from this report alone.'],
            'inputs': inputs, 'consumers': rows,
            'summary': {table['key']: sum(row['table'] == table['key'] for row in rows) for table in catalog['tables']}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--catalog', type=Path)
    parser.add_argument('--mirror', type=Path, action='append', default=[], help='actual generated mirror directory; repeat per ABI')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    catalog = json.loads((args.catalog or args.root / 'config/game_tables.json').read_text(encoding='utf-8'))
    report = inventory(args.root, catalog, args.mirror)
    args.out.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'status': report['status'], 'compiled_mirror_scanned': report['compiled_mirror_scanned'],
                      'summary': report['summary']}, sort_keys=True))


if __name__ == '__main__':
    main()
