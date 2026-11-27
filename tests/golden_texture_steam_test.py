#!/usr/bin/env python3
"""Local Steam-only decode witness. Never commits or exports game pixels."""
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools' / 'golden_mode'))
import inventory
import census_scripts


def decoded(path):
    data = path.read_bytes()
    count, = struct.unpack_from('<I', data)
    cursor = 4
    result = []
    for _ in range(count):
        index, w, h = struct.unpack_from('<iII', data, cursor)
        cursor += 12
        n = w*h*4
        if not w or not h or n > len(data)-cursor:
            raise ValueError('invalid decoder output')
        result.append((index, w, h, data[cursor:cursor+n]))
        cursor += n
    if cursor != len(data):
        raise ValueError('trailing decoder output')
    return result


def main():
    install, inv, out = (os.environ.get(k) for k in
                        ('P4PC_GOLDEN_STEAM', 'P4PC_GOLDEN_INVENTORY', 'P4PC_GOLDEN_TEXTURE_OUT'))
    if not all((install, inv, out)):
        print('local Steam texture qualification inputs not configured; skipping')
        return 77
    from PIL import Image, ImageDraw
    root = census_scripts.verify_steam(install)
    output = inventory.check_output(out, root)
    output.mkdir(parents=True, exist_ok=True)
    rows = [json.loads(x) for x in Path(inv).open(encoding='utf-8')]
    variants = output / 'texture-variants.json'
    samples = {}
    if variants.exists():
        for variant, example in json.loads(variants.read_text(encoding='utf-8'))['examples'].items():
            if not variant.startswith('spr/'):
                samples[example['path']] = None
    else:
        for kind in ('dds', 'tmx', 'png'):
            choices = [r for r in rows if r.get('signature') == kind]
            for row in sorted(choices, key=lambda r: r['size'])[:2]:
                samples[row['path']] = None
        for row in rows:
            if row['path'].lower().endswith('.tga'):
                samples[row['path']] = None
    atlas = 'data_e.cpk::init_free.bin::init/camp.arc::c_main01x2.spr'
    selectors = [0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3b,0x3d,0x3f,0x40,0x3e,
                 0x50,0x51,0x53,0x4f,0x52,0x54,0x4d,0x3c,0x4c,0x41,0x6c,0x251,0x252,0x253]
    samples[atlas] = selectors
    by_path = {r['path']: r for r in rows}
    end = sys.argv.index('--driver-end')
    command = sys.argv[1:end]
    cards, records = [], []
    with tempfile.TemporaryDirectory(dir=output, prefix='decode-') as tmp:
        tmp = Path(tmp)
        for chain, indices in samples.items():
            row = by_path[chain]
            data = inventory.read_object(root, chain)
            digest = hashlib.sha256(data).hexdigest()
            if digest != row['sha256']:
                raise ValueError('Steam member hash mismatch: ' + chain)
            source, target = tmp/'member.bin', tmp/'decoded.rgba'
            source.write_bytes(data)
            args = command + [str(source), str(target)]
            if row.get('signature') == 'spr' and indices is None:
                args += ['0']
            elif indices is not None:
                args += [hex(i) for i in indices]
            subprocess.run(args, check=True)
            images = decoded(target)
            for index, w, h, rgba in images:
                # A second complete decode must hash identically; member identity
                # is independently pinned to the census above.
                records.append({'path': chain, 'source_sha256': digest, 'index': index,
                                'width': w, 'height': h, 'rgba_sha256': hashlib.sha256(rgba).hexdigest()})
                im = Image.frombytes('RGBA', (w, h), rgba)
                im.thumbnail((160, 128), Image.Resampling.NEAREST)
                cards.append((chain.rsplit('::',1)[-1].rsplit('/',1)[-1]+':'+hex(index), im))
            first = hashlib.sha256(target.read_bytes()).hexdigest()
            subprocess.run(args, check=True)
            if hashlib.sha256(target.read_bytes()).hexdigest() != first:
                raise ValueError('non-deterministic native decode')
    cols, cw, ch = 6, 180, 160
    sheet = Image.new('RGB', (cols*cw, ((len(cards)+cols-1)//cols)*ch), '#505050')
    draw = ImageDraw.Draw(sheet)
    for i, (title, im) in enumerate(cards):
        x, y = (i%cols)*cw, (i//cols)*ch
        for yy in range(0,128,16):
            for xx in range(0,160,16):
                draw.rectangle((x+xx,y+yy,x+xx+15,y+yy+15),fill='#999999' if (xx+yy)%32 else '#dddddd')
        sheet.paste(im,(x,y),im)
        draw.text((x,y+132),title[:27],fill='white')
    filename = output/'contact-sheet.png'
    sheet.save(filename)
    (output/'decode-receipt.json').write_text(json.dumps({'members':len(samples), 'images':records,
        'contact_sheet_sha256':hashlib.sha256(filename.read_bytes()).hexdigest()},indent=2)+'\n',encoding='utf-8')
    print('Steam native decode members=%d images=%d contact-sheet=%s' % (len(samples),len(cards),filename))
    return 0


if __name__ == '__main__':
    sys.exit(main())
