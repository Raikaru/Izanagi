#!/usr/bin/env python3
"""Owner-local numerical witnesses for the M8c spec, not a runtime decoder.

Checks inventory hashes, compares packed-position bounds and two CLUT-order
candidates. Outputs metadata only. Installed Pillow is a measurement dependency,
not a new shipped decoder. Its version is recorded with the numerical result.
"""
import argparse
import ctypes
import hashlib
import io
import json
from pathlib import Path
import re
import struct

import model_measure as model


def chunks(data, start=16, end=None, parent=0):
    end = 16 + struct.unpack_from('<I', data, 20)[0] if end is None else end
    p = start
    while p < end:
        typ, head, size = struct.unpack_from('<HHI', data, p)
        if size < 8 or size > end - p:
            raise ValueError('GMO chunk bound')
        if head:
            child, body = struct.unpack_from('<II', data, p + 8)
            if not 16 <= head <= body <= child <= size:
                raise ValueError('GMO offsets')
            yield typ, p, parent, data[p + 16:p + head].split(b'\0', 1)[0], data[p + body:p + child], data[p + head:p + body]
            yield from chunks(data, p + child, p + size, typ)
        else:
            yield typ, p, parent, b'', data[p + 8:p + size], b''
        p += size


def gmo(amd):
    if amd[:4] != b'CHNK':
        raise ValueError('AMD signature')
    p = 8
    for _ in range(struct.unpack_from('<I', amd, 4)[0]):
        name = amd[p:p + 16].split(b'\0', 1)[0]
        kind, size = struct.unpack_from('<II', amd, p + 16)
        p += 24
        if size > len(amd) - p:
            raise ValueError('AMD member bound')
        if name == b'MODEL_DATA' and kind == 257:
            return amd[p:p + size]
        p += size
    raise ValueError('MODEL_DATA absent')


def bounds(data, extra_wanted):
    flat = list(chunks(data))
    quant = {}
    for typ, _, parent, _, payload, _ in flat:
        if typ == 0x8015 and parent == 3:
            mask = struct.unpack_from('<I', payload)[0]
            quant[mask] = struct.unpack_from('<' + 'f' * ((len(payload) - 4) // 4), payload, 4)
    q = quant[384]
    box, part = None, b''
    for typ, offset, parent, name, payload, args in flat:
        if typ == 5:
            part, box = name, None
        elif typ == 0x8014 and parent == 5:
            box = struct.unpack('<6f', payload)
        elif typ == 7:
            fmt, count, _, extra = struct.unpack_from('<4I', args)
            if extra != extra_wanted or not count or box is None or ((fmt >> 7) & 3) != 2:
                continue
            stride, offsets = model.vertex_layout(fmt)
            uv_bytes = extra * 2 * (0, 1, 2, 4)[fmt & 3]
            full_stride = stride + uv_bytes
            result = {}
            for shift in sorted({0, uv_bytes}):
                low, high = [float('inf')] * 3, [-float('inf')] * 3
                for vertex in range(count):
                    xyz = struct.unpack_from('<3h', payload, vertex * full_stride + offsets['position'] + shift)
                    for j in range(3):
                        value = q[j] + q[j + 3] * xyz[j] / 32768
                        low[j], high[j] = min(low[j], value), max(high[j], value)
                error = max(abs(a - b) for a, b in zip(low + high, box))
                result['position_shift_%d_max_error' % shift] = error
            return {'part': part.decode('ascii'), 'array_offset': offset, 'vertices': count,
                    'additional_uv_sets': extra, 'stride': full_stride, 'errors': result}
    raise ValueError('witness array absent')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', required=True)
    parser.add_argument('--inventory', required=True)
    parser.add_argument('--native')
    parser.add_argument('--out', required=True)
    args = parser.parse_args()
    install = Path(args.install).resolve()
    manifest = (install.parent.parent / 'appmanifest_1113000.acf').read_text(encoding='utf8')
    if not all(re.search(r'"%s"\s*"%s"' % (k, re.escape(v)), manifest)
               for k, v in [('appid', '1113000'), ('buildid', '10836033'), ('installdir', install.name)]):
        raise ValueError('not the pinned Steam install')
    out = Path(args.out).resolve()
    if out.is_relative_to(install):
        raise ValueError('output inside install')
    if args.native:
        dll = ctypes.CDLL(str(Path(args.native).resolve()))
        decode = dll.p4_golden_crilayla
        decode.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t]
        decode.restype = ctypes.c_int

        def native(data):
            dest = ctypes.create_string_buffer(model.formats.crilayla_size(data))
            if decode(data, len(data), dest, len(dest)):
                raise ValueError('CRILAYLA refusal')
            return dest.raw
        model.formats.crilayla = native
    inventory_bytes = Path(args.inventory).read_bytes()
    rows = {r['path']: r for r in map(json.loads, inventory_bytes.splitlines())}
    sources = {}

    def read(path):
        row = rows[path]
        data = model.inventory.read_object(install, path)
        if len(data) != row['size'] or hashlib.sha256(data).hexdigest() != row['sha256']:
            raise ValueError('inventory mismatch: ' + path)
        sources[path] = row['sha256']
        return data

    character = gmo(read('data.cpk::model/pack/bc001.pac::bc001.AMD'))
    field = gmo(read('data.cpk::field/pack/f006_016.arc::f006_016.AMD'))
    enemy = gmo(read('data.cpk::model/pack/em017.pac::em017.AMD'))
    dds = read('data.cpk::model/hrestex/dummy/em017_t01.dds')
    texture, embedded = b'', None
    for typ, _, _, name, payload, _ in chunks(enemy):
        if typ == 10:
            texture = name
        elif typ == 0x8013 and texture == b'em017_t01':
            size = struct.unpack_from('<I', payload)[0]
            embedded = payload[4:4 + size]
            break
    if embedded is None or embedded[:4] != b'TIM2':
        raise ValueError('TIM2 witness absent')
    total, clut, pixels, head, colors, fmt, mips, ct, it, width, height = struct.unpack_from('<IIIHHBBBBHH', embedded, 16)
    if (ct, it, colors) != (3, 5, 256):
        raise ValueError('unexpected witness image class')
    indices = embedded[16 + head:16 + head + pixels]
    palette = embedded[16 + head + pixels:16 + head + pixels + clut]
    from PIL import Image
    reference = Image.open(io.BytesIO(dds)).convert('RGBA').resize((width, height), Image.Resampling.BOX).tobytes()
    errors = {}
    for swapped in (False, True):
        error = 0
        for p, index in enumerate(indices):
            mapped = (index & ~24) | ((index & 8) << 1) | ((index & 16) >> 1) if swapped else index
            for component in range(3):
                error += abs(palette[mapped * 4 + component] - reference[p * 4 + component])
        errors['csm1' if swapped else 'identity'] = error / (width * height * 3)
    result = {'inventory_sha256': hashlib.sha256(inventory_bytes).hexdigest(), 'sources': sources,
              'quantized_position': bounds(character, 0), 'additional_uv_position': bounds(field, 1),
              'clut': {'pillow': Image.__version__, 'resize': 'BOX', 'rgb_mae': errors,
                       'embedded_sha256': hashlib.sha256(embedded).hexdigest(),
                       'width': width, 'height': height, 'palette_alpha_min': min(palette[3::4]),
                       'palette_alpha_max': max(palette[3::4])}}
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n', encoding='utf8')
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
