#!/usr/bin/env python3
"""Measure embedded GMO TIM2/BMP headers and hrestex name coverage, metadata only.

Same CLI, Steam/hash checks and archive traversal as model_measure.py. This is a
measurement reader, not a renderer. It does not infer swizzle from image size.
"""
import collections
import json
from pathlib import Path
import struct
import sys

import model_measure as model

DDS = {}


def name_text(raw):
    # CP932 is a measured candidate for non-ASCII artist names, not a loader claim.
    return raw.decode('ascii' if raw.isascii() else 'cp932')


def raw_names(packet):
    """Read TOC string cells losslessly; census UTF-8 replacement loses nine DDS names."""
    if packet[:4] != b'@UTF':
        packet = model.formats.utf_unmask(packet)
    model.formats.utf_table(packet)  # Existing bounded framing validation.
    data = packet[8:]
    rows, strings, _ = struct.unpack_from('>HII', data, 2)
    columns, width, count = struct.unpack_from('>HHI', data, 16)

    def string(offset):
        start = strings + offset
        end = data.find(b'\0', start)
        if not strings <= start < len(data) or end < 0:
            raise ValueError('TOC string bound')
        return data[start:end]

    cursor, row_offset, fields = 24, 0, {}
    for _ in range(columns):
        flags = data[cursor]
        key = string(struct.unpack_from('>I', data, cursor + 1)[0])
        kind, storage = flags & 15, flags & 0xf0
        cursor += 5
        if storage == 0x30:
            where = cursor
            cursor += model.formats.UTF_SIZES[kind]
        elif storage == 0x50:
            where = row_offset
            row_offset += model.formats.UTF_SIZES[kind]
        elif storage == 0x10:
            where = None
        else:
            raise ValueError('TOC storage variant')
        if kind == 10:
            fields[key] = (storage, where)
    result = []
    for index in range(count):
        row = {}
        for key, (storage, where) in fields.items():
            at = where if storage == 0x30 else rows + index * width + where if where is not None else None
            row[key] = string(struct.unpack_from('>I', data, at)[0]) if at is not None else b''
        directory, name = row.get(b'DirName', b''), row.get(b'FileName', b'')
        result.append(directory + b'/' + name if directory else name)
    return result


def stem(name):
    return name.replace('\\', '/').rsplit('/', 1)[-1].rsplit('.', 1)[0].casefold()


class TextureMeter(model.Meter):
    def __init__(self):
        super().__init__()
        self.matched = set()
        self.owner_models = {}
        self.counts['dds.inventory_objects'] = sum(len(v) for v in DDS.values())
        self.counts['dds.distinct_stems'] = len(DDS)
        self.counts['dds.conflicting_stems'] = sum(len(v) > 1 for v in DDS.values())
        self.counts['dds.non_ascii_names'] = sum(not key.isascii() for key in DDS)

    def chunks(self, data, start, end, parent, path, depth, scopes=()):
        if depth > 64:
            raise ValueError('GMO depth')
        p = start
        while p < end:
            if end - p < 8:
                raise ValueError('short GMO header')
            typ, head, size = struct.unpack_from('<HHI', data, p)
            if size < 8 or size > end - p:
                raise ValueError('GMO parent bound')
            if head:
                child, body = struct.unpack_from('<II', data, p + 8)
                if not 16 <= head <= body <= child <= size:
                    raise ValueError('GMO block offsets')
                if typ == 10:
                    name = name_text(data[p + 16:p + head].split(b'\0', 1)[0])
                    self.texture(data, p + child, p + size, name, path)
                if typ == 7 and body - head >= 16:
                    fmt, vertices, one, extra = struct.unpack_from('<4I', data, p + head)
                    stride, offsets = model.vertex_layout(fmt)
                    uv_width = (0, 1, 2, 4)[fmt & 3]
                    expected = ((vertices * (stride + extra * 2 * uv_width) + 3) // 4) * 4 if stride is not None else -1
                    self.add('arrays.extra_uv_sets', extra)
                    self.counts['arrays.extra_uv_length.' + str(expected == child - body)] += 1
                children, at = collections.Counter(), p + child
                while at < p + size:
                    ct, ch, cs = struct.unpack_from('<HHI', data, at)
                    if cs < 8 or cs > p + size - at:
                        raise ValueError('GMO child bound')
                    if ct < 0x8000:
                        index = children[ct]
                        children[ct] += 1
                        if typ == 3 and ct == 11:
                            name = data[at + 16:at + ch].split(b'\0', 1)[0].decode('ascii')
                            match = model.re.search(r'-(\d+)$', name)
                            if index and (not match or int(match.group(1)) != index - 1):
                                self.examples.setdefault('motion.mapping_exceptions', []).append(
                                    {'path': path, 'offset': at, 'index': index,
                                     'name': name, 'suffix': int(match.group(1)) if match else None})
                    at += cs
                if typ == 3 and path.endswith('::MODEL_DATA'):
                    self.owner_models.setdefault(path.rsplit('::', 1)[0], []).append(children)
                self.chunks(data, p + child, p + size, '%04x' % typ, path, depth + 1, scopes + (children,))
            else:
                payload = data[p + 8:p + size]
                refs = []
                if typ in (0x8041, 0x804e, 0x8061, 0x8066, 0x8091) and len(payload) >= 4:
                    refs = [struct.unpack_from('<I', payload)[0]]
                elif typ == 0x80b3 and len(payload) == 16:
                    refs = [struct.unpack_from('<I', payload)[0], struct.unpack_from('<I', payload, 12)[0]]
                elif typ == 0x8044 and len(payload) >= 4:
                    n = struct.unpack_from('<I', payload)[0]
                    if len(payload) == 4 + n * 4:
                        refs = struct.unpack_from('<%dI' % n, payload, 4)
                for ref in refs:
                    kind, level, index = ref >> 16, (ref >> 12) & 15, ref & 4095
                    valid = level < len(scopes) and index < scopes[-1 - level].get(kind, 0)
                    self.counts['reference.%04x.' % typ + str(valid)] += 1
                    if not valid:
                        self.examples.setdefault('reference.exceptions', []).append(
                            {'path': path, 'offset': p, 'command': typ, 'reference': ref})
                        if '::NECK_BENDER::' in path and kind in (5, 8):
                            owners = self.owner_models.get(path.split('::NECK_BENDER::', 1)[0], [])
                            fits = len(owners) == 1 and index < owners[0].get(kind, 0)
                            self.counts['reference.owner_context.%04x.' % typ + str(fits)] += 1
                        if ref == 0xffffffff and typ == 0x80b3:
                            self.counts['reference.sentinel.target_command.%04x' % struct.unpack_from('<I', payload, 4)[0]] += 1
                if typ in (0x8044, 0x8045, 0x8062) and len(payload) >= 4:
                    n = struct.unpack_from('<I', payload)[0]
                    unit = 64 if typ == 0x8045 else 4
                    self.counts['counted.%04x.' % typ + str(len(payload) == 4 + n * unit)] += 1
                if typ == 0x8066 and len(payload) >= 16:
                    ref, mode, vertices, primitives = struct.unpack_from('<4I', payload)
                    expected = 20 if mode & 0x100 else ((16 + 2 * vertices * primitives + 3) // 4) * 4
                    self.counts['draw.u16_or_sequential_length.' + str(expected == len(payload))] += 1
                if typ == 0x804b and len(payload) == 16:
                    q = struct.unpack('<4f', payload)
                    self.add('bone804b.quaternion_norm2', sum(v * v for v in q))
            p += size

    def texture(self, data, start, end, name, path):
        self.counts['texture.blocks'] += 1
        filename, images = '', []
        p = start
        while p < end:
            typ, head, size = struct.unpack_from('<HHI', data, p)
            if head or size < 8 or size > end - p:
                raise ValueError('unexpected texture child')
            payload = data[p + 8:p + size]
            if typ == 0x8012:
                filename = name_text(payload.split(b'\0', 1)[0])
            elif typ == 0x8013:
                if len(payload) < 4:
                    raise ValueError('short file image')
                length = struct.unpack_from('<I', payload)[0]
                if length > len(payload) - 4:
                    raise ValueError('file image exceeds command')
                self.add('image.command_padding', len(payload) - 4 - length)
                images.append(payload[4:4 + length])
            p += size
        name_key = name.replace('\\', '/').rsplit('/', 1)[-1].casefold()
        if name_key.endswith(('.tm2', '.gim', '.bmp', '.dds')):
            name_key = stem(name)
        file_key = stem(filename) if filename else ''
        by_name, by_file = name_key in DDS, file_key in DDS
        self.counts['binding.name_ascii.' + str(name.isascii())] += 1
        self.counts['binding.filename_ascii.' + str(filename.isascii())] += 1
        self.counts['binding.name_found.' + str(by_name)] += 1
        self.counts['binding.filename_found.' + str(by_file)] += 1
        self.counts['binding.name_equals_filename.' + str(name_key == file_key)] += 1
        self.counts['binding.both_different.' + str(by_name and by_file and name_key != file_key)] += 1
        self.counts['binding.' + ('embedded' if images else 'external') + '.any_dds.' + str(by_name or by_file)] += 1
        for key in {name_key, file_key}:
            if key in DDS:
                self.matched.add(key)
        self.counts['dds.matched_distinct_stems'] = len(self.matched)
        for image in images:
            if image[:4] == b'TIM2':
                self.tim2(image, path)
            elif image[:2] == b'BM':
                self.bmp(image, path)
            else:
                raise ValueError('unclassified embedded image')

    def tim2(self, image, path):
        if len(image) < 16:
            raise ValueError('short TIM2')
        self.counts['image.TIM2'] += 1
        version, style, count = struct.unpack_from('<BBH', image, 4)
        self.add('tim2.version', version)
        self.add('tim2.style', style)
        self.add('tim2.pictures', count)
        self.counts['tim2.file_reserved_zero.' + str(not any(image[8:16]))] += 1
        if style not in (0, 1):
            raise ValueError('unclassified TIM2 style')
        p = 16 if style == 0 else 128
        for _ in range(count):
            if len(image) - p < 48:
                raise ValueError('short TIM2 picture')
            total, clut, pixels, head, colors, fmt, mips, clut_type, image_type, width, height = struct.unpack_from('<IIIHHBBBBHH', image, p)
            if not 48 <= head <= total <= len(image) - p or pixels + clut > total - head:
                raise ValueError('TIM2 picture bound')
            fields = {'total': total, 'clut_bytes': clut, 'image_bytes': pixels, 'header_bytes': head,
                      'colors': colors, 'picture_format': fmt, 'mips': mips, 'clut_type': clut_type,
                      'image_type': image_type, 'width': width, 'height': height,
                      'picture_padding': total - head - pixels - clut}
            tex0, tex1, gsreg, gsclut = struct.unpack_from('<QQII', image, p + 24)
            fields.update({'tex0_psm': (tex0 >> 20) & 63, 'tex0_cpsm': (tex0 >> 51) & 15,
                           'tex0_csm': (tex0 >> 55) & 1, 'tex0_csa': (tex0 >> 56) & 31,
                           'tex0': tex0, 'tex1': tex1, 'gsreg': gsreg, 'gsclut': gsclut})
            for key, value in fields.items():
                self.add('tim2.' + key, value)
            self.counts['tim2.variant.%d.%d.%d.%d.%d.%d' % (version, style, fmt, mips, clut_type, image_type)] += 1
            bpp = {1: 16, 2: 24, 3: 32, 4: 4, 5: 8}.get(image_type)
            if bpp:
                self.counts['tim2.base_image_size.' + str(pixels == (width * height * bpp + 7) // 8)] += 1
            palette = image[p + head + pixels:p + head + pixels + clut]
            if clut_type & 63 == 3 and palette:
                alpha = palette[3::4]
                self.add('tim2.palette_alpha_min', min(alpha))
                self.add('tim2.palette_alpha_max', max(alpha))
                self.counts['tim2.palette_alpha_over128.' + str(max(alpha) > 128)] += 1
                partial = any(0 < a < 255 for a in alpha)
                self.counts['tim2.palette_partial.' + str(partial)] += 1
                if partial:
                    self.examples.setdefault('tim2.partial_alpha', {'path': path})
            self.examples.setdefault('tim2.variant.%d.%d.%d.%d.%d.%d' % (version, style, fmt, mips, clut_type, image_type), {'path': path})
            p += total
        self.add('tim2.trailing_bytes', len(image) - p)

    def bmp(self, image, path):
        if len(image) < 54:
            raise ValueError('short BMP')
        self.counts['image.BMP'] += 1
        size, reserved, offset, dib = struct.unpack_from('<4I', image, 2)
        width, height, planes, bpp, compression, image_bytes, xppm, yppm, colors, important = struct.unpack_from('<iiHHIIiiII', image, 18)
        if size > len(image) or offset > len(image) or dib < 40 or 14 + dib > len(image):
            raise ValueError('BMP bounds')
        for key, value in {'file_bytes': size, 'reserved': reserved, 'pixel_offset': offset, 'dib_bytes': dib,
                           'width': width, 'height': height, 'planes': planes, 'bpp': bpp,
                           'compression': compression, 'image_bytes': image_bytes, 'xppm': xppm,
                           'yppm': yppm, 'colors': colors, 'important': important,
                           'trailing_bytes': len(image) - size}.items():
            self.add('bmp.' + key, value)
        self.examples.setdefault('bmp', {'path': path})


if __name__ == '__main__':
    inventory_path = sys.argv[sys.argv.index('--inventory') + 1]
    install = Path(sys.argv[sys.argv.index('--install') + 1] if '--install' in sys.argv else model.os.environ['P4PC_GOLDEN_STEAM'])
    candidates, raw_tocs = collections.defaultdict(list), {}
    for line in Path(inventory_path).read_text(encoding='utf8').splitlines():
        row = json.loads(line)
        if '::model/hrestex/' in row['path'].casefold() and row.get('signature') == 'dds':
            archive = row['archive']
            if archive not in raw_tocs:
                with (install / archive).open('rb') as handle:
                    reader = model.formats.file_reader(handle)
                    header = model.formats.utf_table(model.formats._chunk(reader, 0, b'CPK '))[0]
                    raw_tocs[archive] = raw_names(model.formats._chunk(reader, header['TocOffset'], b'TOC '))
            raw = raw_tocs[archive][row['index']]
            if raw.decode('utf8', 'replace') != row['path'].split('::', 1)[1]:
                raise ValueError('TOC index identity mismatch')
            candidates[stem(name_text(raw))].append(row['path'])
    DDS.update(candidates)
    model.Meter = TextureMeter
    sys.exit(model.main())
