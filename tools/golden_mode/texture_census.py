#!/usr/bin/env python3
"""Local-only texture header census. Writes metadata, never texture payloads."""
import argparse
import contextlib
import collections
import hashlib
import json
from pathlib import Path
import struct
import inventory


def describe(data):
    kind = inventory.formats.signature(data[:16])
    if kind == 'dds':
        if len(data) < 128:
            raise ValueError('truncated DDS')
        variant = 'dds:' + ','.join(str(v) for v in struct.unpack_from('<8I', data, 76))
        if data[84:88] == b'DX10':
            if len(data) < 148:
                raise ValueError('truncated DDS DX10')
            variant += ',dx10=' + ','.join(str(v) for v in struct.unpack_from('<5I', data, 128))
        return variant
    if kind == 'tmx':
        if len(data) < 64:
            raise ValueError('truncated TMX')
        return 'tmx:palettes=%d,palette=%d,pixel=%d,mips=%d' % (data[16], data[17], data[22], data[23])
    if kind == 'spr':
        if len(data) < 32 or data[8:12] != b'SPR0':
            raise ValueError('unsupported SPR framing')
        return 'spr:SPR0'
    if kind == 'png':
        return 'png:depth=%d,color=%d' % (data[24], data[25])
    return 'tga:type=%d,depth=%d,descriptor=%d' % (data[2], data[16], data[17])


class ArchiveReader:
    def __init__(self, install, stack):
        self.install, self.stack, self.archives = Path(install), stack, {}

    def read(self, root):
        chain = root.split('::')
        if len(chain) == 1:
            return (self.install / root).read_bytes()
        archive, member = chain
        if archive not in self.archives:
            handle = self.stack.enter_context((self.install / archive).open('rb'))
            entries = {name.lower(): (o, s, e) for name, o, s, e in
                       inventory.formats.cpk_entries(inventory.formats.file_reader(handle))}
            self.archives[archive] = handle, entries
        handle, entries = self.archives[archive]
        offset, stored, extracted = entries[member.lower()]
        handle.seek(offset)
        data = handle.read(stored)
        if data[:8] == b'CRILAYLA' and stored != extracted:
            data = inventory.formats.crilayla(data)
        return data


def scan(install, rows, stack):
    selected = [r for r in rows if r.get('signature') in ('dds', 'tmx', 'spr', 'png')
                or r['path'].lower().endswith('.tga')]
    groups = collections.defaultdict(list)
    for row in selected:
        groups['::'.join(row['path'].split('::')[:2])].append(row)
    reader = ArchiveReader(install, stack)
    counts = collections.Counter()
    examples = {}
    for root, members in sorted(groups.items()):
        data = reader.read(root)
        cache = {root: data}
        for row in sorted(members, key=lambda r: r['path']):
            path = row['path']
            chain = path.split('::')
            for n in range(3, len(chain) + 1):
                current = '::'.join(chain[:n])
                if current in cache:
                    continue
                parent = cache['::'.join(chain[:n-1])]
                _, entries = inventory.children(parent)
                match = next(((o, s) for name, o, s in entries or () if name == chain[n-1]), None)
                if match is None:
                    raise ValueError('missing ' + current)
                off, size = match
                child = parent[off:off+size]
                if child[:8] == b'CRILAYLA':
                    child = inventory.formats.crilayla(child)
                cache[current] = child
            content = cache[path]
            if hashlib.sha256(content).hexdigest() != row['sha256']:
                raise ValueError('hash mismatch: ' + path)
            variant = describe(content)
            counts[variant] += 1
            examples.setdefault(variant, {'path': path, 'sha256': row['sha256'], 'size': len(content)})
            if variant == 'spr:SPR0':
                nt = struct.unpack_from('<h', content, 20)[0]
                table = struct.unpack_from('<I', content, 24)[0]
                if nt < 0 or table + nt*8 > len(content):
                    raise ValueError('SPR table: ' + path)
                for i in range(nt):
                    off = struct.unpack_from('<I', content, table+8*i+4)[0]
                    inner = describe(content[off:off+148])
                    counts['spr/' + inner] += 1
                    examples.setdefault('spr/' + inner, {'path': path, 'texture': i, 'sha256': row['sha256']})
    return {'objects': len(selected), 'variants': dict(sorted(counts.items())), 'examples': examples,
            'note': 'Counts include duplicate nested and locale objects. SPR children counted separately.'}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--install', required=True)
    ap.add_argument('--inventory', required=True)
    ap.add_argument('--out', required=True)
    args = ap.parse_args()
    out = inventory.check_output(args.out, args.install)
    out.mkdir(parents=True, exist_ok=True)
    rows = [json.loads(line) for line in Path(args.inventory).open(encoding='utf-8')]
    with contextlib.ExitStack() as stack:
        result = scan(args.install, rows, stack)
    (out / 'texture-variants.json').write_text(json.dumps(result, indent=2, sort_keys=True)+'\n', encoding='utf-8')
    print(json.dumps({'objects': result['objects'], 'variants': result['variants']}, sort_keys=True))


if __name__ == '__main__':
    main()
