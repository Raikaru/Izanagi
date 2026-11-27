#!/usr/bin/env python3
"""Local-only AMD/GMO/PAC grammar census. Writes metadata, never asset bytes.

Reads the M7c inventory and checks every selected object's size and SHA-256.
Optional --native DLL exports the existing golden_formats.c CRILAYLA reader.
No P4G executable or implementation is read. Run under the PC CPU governor.
"""
import argparse
import collections
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'golden_mode'))
import formats
import inventory


def selected(row):
    return row.get('signature') in ('amd-chunk', 'magic:4f4d472e') or row['path'].split('::')[-1].lower().endswith('.pac')


def vertex_layout(fmt):
    """Candidate PSP attribute packing; measured separately from framing."""
    parts = []
    width = (0, 1, 2, 4)
    weight = (fmt >> 9) & 3
    count = ((fmt >> 14) & 7) + 1 if weight else 0
    if weight:
        parts.append(('weights', width[weight] * count, width[weight]))
    uv = fmt & 3
    if uv:
        parts.append(('uv', width[uv] * 2, width[uv]))
    color = (fmt >> 2) & 7
    if color:
        if color < 4:
            return None, None
        parts.append(('color', 4 if color == 7 else 2, 4 if color == 7 else 2))
    for name, shift in [('normal', 5), ('position', 7)]:
        kind = (fmt >> shift) & 3
        if kind:
            parts.append((name, width[kind] * 3, width[kind]))
    if not parts:
        return None, None
    offset, offsets = 0, {}
    for name, size, align in parts:
        offset = (offset + align - 1) // align * align
        offsets[name] = offset
        offset += size
    align = max(p[2] for p in parts)
    return (offset + align - 1) // align * align, offsets


class Meter:
    def __init__(self):
        self.counts = collections.Counter()
        self.stats = {}
        self.examples = {}
        self.errors = []

    def add(self, key, value):
        item = self.stats.setdefault(key, {'count': 0, 'min': value, 'max': value, 'values': {}})
        item['count'] += 1
        item['min'] = min(item['min'], value)
        item['max'] = max(item['max'], value)
        if item['values'] is not None:
            item['values'][str(value)] = item['values'].get(str(value), 0) + 1
            if len(item['values']) > 64:
                item['values'] = None

    def gmo(self, data, path):
        if len(data) < 32 or data[:4] != b'OMG.':
            raise ValueError('invalid GMO signature')
        self.counts['gmo.files'] += 1
        self.counts['gmo.header.' + data[:16].hex()] += 1
        self.add('gmo.bytes', len(data))
        size = struct.unpack_from('<I', data, 20)[0]
        if data[16:18] != b'\x02\x00' or size < 16 or size > len(data) - 16:
            raise ValueError('GMO root bound')
        self.add('gmo.trailer.bytes', len(data) - 16 - size)
        self.counts['gmo.trailer.nonzero.' + str(any(data[16 + size:]))] += 1
        self.chunks(data, 16, 16 + size, 'file', path, 0)

    def chunks(self, data, start, end, parent, path, depth):
        if depth > 64:
            raise ValueError('GMO nesting depth')
        p = start
        while p < end:
            if end - p < 8:
                raise ValueError('short GMO chunk header')
            typ, head, size = struct.unpack_from('<HHI', data, p)
            key = 'gmo.%04x' % typ
            if size < 8 or size > end - p:
                raise ValueError('GMO size exceeds parent at %x type %x size %x' % (p, typ, size))
            self.counts[key] += 1
            self.counts['chunk.aligned4.' + str((p | head | size) & 3 == 0)] += 1
            self.counts['edge.' + parent + '>%04x' % typ] += 1
            self.add(key + '.header', head)
            self.add(key + '.bytes', size)
            self.examples.setdefault(key, {'path': path, 'offset': p})
            if head:
                if head < 16 or head > size:
                    raise ValueError('GMO container header bound')
                child, body = struct.unpack_from('<II', data, p + 8)
                if not 16 <= head <= body <= child <= size:
                    raise ValueError('GMO offsets at %x: %x %x %x' % (p, body, child, size))
                self.add(key + '.children_offset', child)
                self.add(key + '.data_offset', body)
                self.add(key + '.data_bytes', child - body)
                self.add(key + '.name_bytes', head - 16)
                args = data[p + head:p + body]
                self.add(key + '.args_bytes', len(args))
                for word in range(min(len(args) // 4, 16)):
                    self.add(key + '.arg%02x' % (word * 4), struct.unpack_from('<I', args, word * 4)[0])
                if typ == 3:
                    self.add('gmo.model.motion_count', self.child_count(data, p + child, p + size, 11))
                    self.motion_names(data, p + child, p + size)
                if typ == 11:
                    self.add('gmo.motion.curves', self.child_count(data, p + child, p + size, 12))
                payload = data[p + body:p + child]
                if typ == 7 and len(args) >= 16:
                    fmt, vertices, morphs, reserved = struct.unpack_from('<4I', args)
                    self.add('arrays.format', fmt)
                    self.add('arrays.vertices', vertices)
                    if vertices:
                        self.add('arrays.bytes_per_vertex', len(payload) / vertices)
                    weights = ((fmt >> 14) & 7) + 1 if (fmt >> 9) & 3 else 0
                    self.add('arrays.weights', weights)
                    self.counts['arrays.layout.%08x' % fmt] += 1
                    stride, offsets = vertex_layout(fmt)
                    fits = stride is not None and ((stride * vertices + 3) // 4) * 4 == len(payload)
                    self.counts['arrays.layout_length.' + str(fits)] += 1
                    if stride is not None:
                        self.add('arrays.stride', stride)
                    if fits and weights and (fmt >> 9) & 3 == 1:
                        sums = [sum(payload[v * stride:v * stride + weights]) for v in range(vertices)]
                        if sums:
                            self.add('arrays.weight_sum_min', min(sums))
                            self.add('arrays.weight_sum_max', max(sums))
                if typ == 12 and len(args) >= 16:
                    fmt, dims, keys, reserved = struct.unpack_from('<4I', args)
                    self.add('curves.format', fmt)
                    self.add('curves.dimensions', dims)
                    self.add('curves.keys', keys)
                    mode = fmt & 15
                    width = 2 if fmt & 128 else 4
                    components = dims * (3 if mode == 2 else 5 if mode == 3 else 1) + 1
                    expected = ((keys * components * width + 3) // 4) * 4
                    self.counts['curves.length_formula.' + str(expected == len(payload))] += 1
                self.chunks(data, p + child, p + size, '%04x' % typ, path, depth + 1)
            else:
                payload = data[p + 8:p + size]
            for word in range(0 if typ in (7, 12, 0x8012, 0x8013, 0x80f1, 0x8021, 0x8022, 0x8023) else min(len(payload) // 4, 12)):
                self.add(key + '.word%02x' % (word * 4), struct.unpack_from('<I', payload, word * 4)[0])
                value = struct.unpack_from('<f', payload, word * 4)[0]
                if abs(value) < 1e20 and value == value:
                    self.add(key + '.float%02x' % (word * 4), value)
            if typ == 0x8013 and len(payload) >= 8:
                length = struct.unpack_from('<I', payload)[0]
                self.counts['image.' + payload[4:8].hex()] += 1
                self.add('image.declared_bytes', length)
            if typ == 0x80f1:
                stop = payload.find(b'\0')
                if stop >= 0:
                    name = payload[:stop].decode('ascii', errors='replace')
                    self.counts['blind.' + name] += 1
                    self.add('blind.' + name + '.bytes', len(payload))
                    at = (stop + 4) & ~3
                    if name == 'per3Helper' and len(payload) >= at + 32:
                        ident = struct.unpack_from('<i', payload, at)[0]
                        self.add('helper.id', ident)
                        q = struct.unpack_from('<4f', payload, at + 16)
                        self.add('helper.quaternion_norm2', sum(v * v for v in q))
                        self.counts['helper.trailing_zero.' + str(not any(payload[at + 32:]))] += 1
                    elif name in ('transAlgo', 'EdgeLight', 'ShadowMap') and len(payload) >= at + 4:
                        self.add('blind.' + name + '.value', struct.unpack_from('<i', payload, at)[0])
            p += size
        if p != end:
            raise ValueError('GMO unconsumed bytes')

    @staticmethod
    def child_count(data, start, end, wanted):
        count = 0
        while start + 8 <= end:
            typ, _, size = struct.unpack_from('<HHI', data, start)
            if size < 8 or size > end - start:
                break
            count += typ == wanted
            start += size
        return count

    def motion_names(self, data, start, end):
        index = 0
        while start + 8 <= end:
            typ, head, size = struct.unpack_from('<HHI', data, start)
            if size < 8 or size > end - start:
                raise ValueError('model child bound')
            if typ == 11:
                name = data[start + 16:start + head].split(b'\0', 1)[0].decode('ascii')
                match = re.search(r'-(\d+)$', name)
                if match:
                    slot = int(match.group(1))
                    self.add('motion.name_suffix', slot)
                    self.counts['motion.suffix_equals_index_minus_one.' + str(slot == index - 1)] += 1
                else:
                    self.counts['motion.unsuffixed.index%d' % index] += 1
                index += 1
            start += size

    def amd(self, data, path):
        if len(data) < 8 or data[:4] != b'CHNK':
            raise ValueError('invalid AMD')
        self.counts['amd.files'] += 1
        count = struct.unpack_from('<I', data, 4)[0]
        self.add('amd.members', count)
        p = 8
        for _ in range(count):
            if len(data) - p < 24:
                raise ValueError('short AMD member header')
            raw = data[p:p + 16]
            name = raw.split(b'\0', 1)[0].decode('ascii')
            typ, length = struct.unpack_from('<II', data, p + 16)
            p += 24
            if length > len(data) - p:
                raise ValueError('AMD member out of bounds')
            key = 'amd.%s.%x' % (name, typ)
            self.counts[key] += 1
            self.add(key + '.bytes', length)
            body = data[p:p + length]
            if body[:4] == b'OMG.':
                self.gmo(body, path + '::' + name)
            else:
                self.counts[key + '.magic.' + body[:4].hex()] += 1
                for word in range(min(len(body) // 4, 12)):
                    self.add(key + '.word%02x' % (word * 4), struct.unpack_from('<I', body, word * 4)[0])
                if typ == 720:
                    self.neck(body, path + '::' + name)
            p += length
        self.add('amd.trailer.bytes', len(data) - p)
        self.counts['amd.trailer.nonzero.' + str(any(data[p:]))] += 1
        if len(data) > p:
            self.counts['amd.trailer.magic.' + data[p:p + 4].hex()] += 1

    def neck(self, data, path):
        if len(data) < 4:
            raise ValueError('short neck archive')
        count = struct.unpack_from('<I', data)[0]
        self.add('neck.members', count)
        p = 4
        for _ in range(count):
            if len(data) - p < 36:
                raise ValueError('short neck member')
            name = data[p:p + 32].split(b'\0', 1)[0].decode('ascii')
            size = struct.unpack_from('<I', data, p + 32)[0]
            p += 36
            if size > len(data) - p:
                raise ValueError('neck member exceeds archive')
            if data[p:p + 4] == b'OMG.':
                self.gmo(data[p:p + size], path + '::' + name)
            else:
                self.counts['neck.child.' + name.rsplit('.', 1)[-1].lower()] += 1
                self.add('neck.child.bytes', size)
                for word in range(min(12, size // 4)):
                    self.add('neck.child.word%02x' % (word * 4), struct.unpack_from('<I', data, p + word * 4)[0])
            p += size
        self.add('neck.trailer.bytes', len(data) - p)
        self.counts['neck.trailer.nonzero.' + str(any(data[p:]))] += 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', default=os.environ.get('P4PC_GOLDEN_STEAM'))
    parser.add_argument('--inventory', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--native')
    args = parser.parse_args()
    install = Path(args.install).resolve()
    manifest = (install.parent.parent / 'appmanifest_1113000.acf').read_text(encoding='utf8')
    if not all(re.search(r'"%s"\s*"%s"' % (key, re.escape(value)), manifest)
               for key, value in [('appid', '1113000'), ('buildid', '10836033'), ('installdir', install.name)]):
        raise ValueError('not the pinned Steam install')
    out = Path(args.out).resolve()
    if out.is_relative_to(install):
        raise ValueError('output must not be in install')
    if args.native:
        dll = ctypes.CDLL(str(Path(args.native).resolve()))
        decode = dll.p4_golden_crilayla
        decode.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t]
        decode.restype = ctypes.c_int

        def native(data):
            length = formats.crilayla_size(data)
            dest = ctypes.create_string_buffer(length)
            if decode(data, len(data), dest, length):
                raise ValueError('native CRILAYLA refusal')
            return dest.raw
        formats.crilayla = native
    source = Path(args.inventory).read_bytes()
    rows = [json.loads(line) for line in source.splitlines()]
    targets = [r for r in rows if selected(r)]
    groups = collections.defaultdict(list)
    for row in targets:
        chain = row['path'].split('::')
        groups[(chain[0], chain[1])].append(row)
    archives = {}
    for archive, _ in groups:
        if archive not in archives:
            with (install / archive).open('rb') as handle:
                archives[archive] = {n: (o, s, e) for n, o, s, e in formats.cpk_entries(formats.file_reader(handle))}
    meter = Meter()
    records = []
    for (archive, member), members in groups.items():
        off, stored, extracted = archives[archive][member]
        with (install / archive).open('rb') as handle:
            handle.seek(off)
            base = handle.read(stored)
        if base[:8] == b'CRILAYLA' and stored != extracted:
            base = formats.crilayla(base)
        if len(base) != extracted:
            raise ValueError('top-level extract size mismatch')
        for row in members:
            data = base
            for name in row['path'].split('::')[2:]:
                _, entries = inventory.children(data)
                _, at, size = next(e for e in entries if e[0] == name)
                data = data[at:at + size]
                if data[:8] == b'CRILAYLA':
                    data = formats.crilayla(data)
            if len(data) != row['size'] or hashlib.sha256(data).hexdigest() != row['sha256']:
                raise ValueError('inventory mismatch: ' + row['path'])
            category = 'unknown'
            try:
                if data[:4] == b'CHNK':
                    category = 'AMD'
                    meter.amd(data, row['path'])
                elif data[:4] == b'OMG.':
                    category = 'GMO'
                    meter.gmo(data, row['path'])
                else:
                    category, entries = formats.pak_entries(data)
                    if not category:
                        raise ValueError('unclassified PAC')
                    meter.counts['pac.' + category] += 1
                    meter.add('pac.' + category + '.members', len(entries))
                    meter.add('pac.' + category + '.bytes', len(data))
                    for name, at, size in entries:
                        meter.counts['pac.child.' + name.rsplit('.', 1)[-1].lower()] += 1
            except (ValueError, struct.error) as error:
                meter.errors.append({'path': row['path'], 'error': str(error)})
            records.append({'path': row['path'], 'sha256': row['sha256'], 'bytes': len(data), 'class': category})
    result = {'inventory_sha256': hashlib.sha256(source).hexdigest(), 'objects': len(targets),
              'classes': dict(collections.Counter(r['class'] for r in records)),
              'counts': dict(sorted(meter.counts.items())), 'stats': dict(sorted(meter.stats.items())),
              'examples': meter.examples, 'errors': meter.errors, 'records': records}
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n', encoding='utf8')
    print(json.dumps({'objects': len(targets), 'classes': result['classes'], 'errors': meter.errors[:20],
                      'error_count': len(meter.errors), 'counts': {k: v for k, v in meter.counts.items() if k.endswith('.files')}}))
    return bool(meter.errors)


if __name__ == '__main__':
    sys.exit(main())
