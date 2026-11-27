#!/usr/bin/env python3
"""Owner-local census-exact native qualification. Never export game payloads."""
import collections
import contextlib
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools' / 'golden_mode'))
import census_scripts
import inventory
from texture_census import ArchiveReader

@contextlib.contextmanager
def native_archives(driver):
    """Use M7c's unchanged bounded C codec; retain every census hash check."""
    original = inventory.formats.crilayla
    with subprocess.Popen(driver + ['inflate'], stdin=subprocess.PIPE, stdout=subprocess.PIPE) as process:
        def inflate(data):
            if len(data) < 16 or data[:8] != b'CRILAYLA':
                raise ValueError('not a CRILAYLA packet')
            expected = struct.unpack_from('<I', data, 8)[0] + 256
            if expected > 512 * 1024 * 1024:
                raise ValueError('CRILAYLA output budget exceeded')
            process.stdin.write(struct.pack('<I', len(data)) + data)
            process.stdin.flush()
            header = process.stdout.read(4)
            if len(header) != 4 or struct.unpack('<I', header)[0] != expected:
                raise ValueError('native CRILAYLA length mismatch')
            result = process.stdout.read(expected)
            if len(result) != expected:
                raise ValueError('native CRILAYLA short read')
            return result
        inventory.formats.crilayla = inflate
        try:
            yield
        finally:
            inventory.formats.crilayla = original
            process.stdin.close()
            if process.wait():
                raise ValueError('native CRILAYLA driver failed')



def objects(install, rows):
    """Hash every selected census occurrence, preserving duplicate paths."""
    groups = collections.defaultdict(list)
    for row in rows:
        if row.get('signature') == 'message-bmd' or row['path'].lower().endswith(('.bmd', '.msg', '.fnt')):
            groups['::'.join(row['path'].split('::')[:2])].append(row)
    with contextlib.ExitStack() as stack:
        reader = ArchiveReader(install, stack)
        for root, members in sorted(groups.items()):
            cache = {root: reader.read(root)}
            for row in members:
                chain = row['path'].split('::')
                for depth in range(3, len(chain) + 1):
                    path = '::'.join(chain[:depth])
                    if path not in cache:
                        parent = cache['::'.join(chain[:depth - 1])]
                        _, entries = inventory.children(parent)
                        matches = [(o, n) for name, o, n in entries or () if name == chain[depth - 1]]
                        if len(matches) != 1:
                            raise ValueError('nonunique census member: ' + path)
                        offset, size = matches[0]
                        data = parent[offset:offset + size]
                        if data[:8] == b'CRILAYLA':
                            data = inventory.formats.crilayla(data)
                        cache[path] = data
                data = cache[row['path']]
                if len(data) != row['size'] or hashlib.sha256(data).hexdigest() != row['sha256']:
                    raise ValueError('census mismatch: ' + row['path'])
                yield row, data


def png(path, width, height, rgba):
    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data))
    raw = b''.join(b'\0' + rgba[y * width * 4:(y + 1) * width * 4] for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
                     + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


def main():
    install, census = os.environ.get('P4PC_GOLDEN_STEAM'), os.environ.get('P4PC_GOLDEN_INVENTORY')
    out = os.environ.get('P4PC_GOLDEN_TEXT_OUT')
    if not install or not census or not out:
        print('Steam install, census and owner-local text output not configured; skipping')
        return 77
    install = census_scripts.verify_steam(install)
    out = inventory.check_output(out, install)
    if out.resolve().is_relative_to(ROOT.resolve()):
        raise ValueError('game-derived output cannot be in the repository')
    out.mkdir(parents=True, exist_ok=True)
    rows = [json.loads(line) for line in Path(census).read_text(encoding='utf-8').splitlines()]
    driver = sys.argv[1:sys.argv.index('--driver-end')]
    records, counts, controls, sources = [], collections.Counter(), collections.Counter(), {}
    with native_archives(driver), subprocess.Popen(driver + ['scan'], stdin=subprocess.PIPE, stdout=subprocess.PIPE) as process:
        try:
            for row, data in objects(install, rows):
                process.stdin.write(struct.pack('<I', len(data)) + data)
                process.stdin.flush()
                result = json.loads(process.stdout.readline())
                record = {'path': row['path'], 'sha256': row['sha256'], **result}
                records.append(record)
                family = 'font' if row['path'].lower().endswith('.fnt') else 'message'
                counts[family + '.objects'] += 1
                counts[family + ('.refused' if result['refusal'] else '.parsed')] += 1
                if result['refusal']:
                    counts['refusal.' + result['reason']] += 1
                else:
                    for key in ('dialogs', 'pages', 'speakers', 'selections', 'glyphs', 'font_glyphs'):
                        counts[key] += result.get(key, 0)
                    controls.update(result.get('control_ids', {}))
                if row['archive'] == 'data_e.cpk':
                    if row['path'].endswith(('font0.fnt', 'datSkillHelp.bmd', 'msg_FishingTutorial.bmd')):
                        sources[row['path'].split('::')[-1].split('/')[-1]] = (row, data)
            process.stdin.close()
            if process.wait():
                raise ValueError('native census driver failed')
        finally:
            if process.poll() is None:
                process.kill()
    receipt = {'counts': dict(counts), 'control_ids': dict(controls), 'objects': records}
    (out / 'parse-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'counts': counts, 'control_ids': controls}, sort_keys=True))
    if '--census-only' in sys.argv:
        return 0
    # All raw inputs and native RGBA intermediates are private and removed on exit.
    with tempfile.TemporaryDirectory(prefix='m8b-witness-', dir=out) as directory:
        temp = Path(directory)
        for name, (_, data) in sources.items():
            (temp / name).write_bytes(data)
        width, cell_height = 2048, 256
        samples = [('msg_FishingTutorial.bmd', 0, 0), ('msg_FishingTutorial.bmd', 2, 0),
                   ('datSkillHelp.bmd', 1, 0), ('datSkillHelp.bmd', 2, 0)]
        pixels = bytearray([24, 24, 24, 255]) * (width * cell_height * len(samples))
        witness = []
        for n, (name, index, page) in enumerate(samples):
            raw = temp / 'render.rgba'
            subprocess.run(driver + [str(temp / 'font0.fnt'), str(temp / name), str(index), str(page), str(raw), str(width)], check=True)
            rgba = raw.read_bytes()
            for y in range(cell_height):
                for x in range(width):
                    src = (y * width + x) * 4
                    dst = ((n * cell_height + y) * width + x) * 4
                    a = rgba[src + 3]
                    for c in range(3):
                        pixels[dst + c] = (rgba[src + c] * a + pixels[dst + c] * (255 - a)) // 255
            witness.append({'source': sources[name][0]['path'], 'sha256': sources[name][0]['sha256'], 'dialog': index, 'page': page,
                            'rgba_sha256': hashlib.sha256(rgba).hexdigest()})
        image = out / 'messages.png'
        png(image, width, cell_height * len(samples), pixels)
        receipt['witness'] = witness
        receipt['png_sha256'] = hashlib.sha256(image.read_bytes()).hexdigest()
    (out / 'parse-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    return 0


if __name__ == '__main__':
    sys.exit(main())
