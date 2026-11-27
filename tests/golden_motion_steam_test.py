#!/usr/bin/env python3
"""Owner-local animated native GPU witnesses. Run under the PC governor.

Every source is frozen-census checked. Frames and strips stay on the owner PC.
The operator MUST view the strips: numeric diversity is not visual acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from local_gates import Machine

# Labels are presentation candidates, not a retail gameplay/slot-name proof (U08).
SAMPLES = [
    ('character-gesture', 'data.cpk::model/npc/n1001_0.AMD', 2, 60),
    ('character-run', 'data.cpk::model/npc/n1001_0.AMD', 1, 20),
    ('enemy-attack', 'data.cpk::model/pack/em001.pac::em001.AMD', 1, 44),
    ('extended-slot27', 'data.cpk::model/npc/n1030_0.AMD', 27, 55),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('viewer', 'elf', 'install', 'inventory', 'out'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    install = Path(args.install).resolve()
    manifest = (install.parent.parent / 'appmanifest_1113000.acf').read_text(encoding='utf8')
    if not all(re.search(r'"%s"\s*"%s"' % (k, re.escape(v)), manifest)
               for k, v in [('appid', '1113000'), ('buildid', '10836033'), ('installdir', install.name)]):
        raise ValueError('not the pinned Steam install')
    source = Path(args.inventory).read_bytes()
    if hashlib.sha256(source).hexdigest() != '150454e1b60d50ccdd58346b6da4e18e363860bf49bb22a679db379dee3771f8':
        raise ValueError('not the frozen inventory')
    rows = {r['path']: r for r in map(json.loads, source.splitlines())}
    out = Path(args.out).resolve()
    if out.is_relative_to(install):
        raise ValueError('output inside install')
    out.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, P4PC_MODE='golden', P4PC_GOLDEN=str(install), P4PC_GOLDEN_HYBRID='0',
               P4PC_INPUT='script', P4PC_MODS='0', P4PC_PACKS='0', P4PC_CONFIG='none',
               SDL_AUDIODRIVER='dummy', P4PC_GL_API='gl', P4PC_OFFSCREEN='1', P4PC_HIDDEN_WINDOW='1')
    for key in ('P4PC_OVERRIDE', 'P4PC_GOLDEN_VFS_DORMANT', 'P4PC_VIEWER_MODEL', 'P4PC_TEXTURE_DUMP', 'P4PC_CAPTURE_WINDOW', 'P4PC_ISO'):
        env.pop(key, None)
    from PIL import Image, ImageDraw
    records = []
    for label, identity, slot, end in SAMPLES:
        frames = []
        strip = Image.new('RGB', (640 * 4, 448 * 2))
        draw = ImageDraw.Draw(strip)
        for index in range(8):
            # Fractional frames and both endpoints, including the final nonlooped key.
            frame = end * index / 7
            png, log = out / f'{label}-{index}.png', out / f'{label}-{index}.log'
            command = [str(Path(args.viewer).resolve()), str(Path(args.elf).resolve()),
                       'golden:' + '/'.join(identity.split('::')[1:]), str(png)]
            env['P4PC_GOLDEN_MODEL_SLOT'] = str(slot)
            env['P4PC_GOLDEN_MODEL_SECONDS'] = format(frame / 30, '.17g')
            env['P4PC_SAVE_DIR'] = str(out / ('save-' + label))
            with Machine(priority='study').case():
                run = subprocess.run(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
            log.write_bytes(run.stdout)
            print(f'{label} frame={frame:.6g}: rc={run.returncode}', flush=True)
            if run.returncode:
                print(run.stdout.decode('utf8', errors='replace'))
                return run.returncode
            if rows[identity]['sha256'].encode() not in run.stdout or b'Golden motion slot=' not in run.stdout:
                raise ValueError('source identity/playback proof absent: ' + identity)
            with Image.open(png) as image:
                rgba = image.convert('RGBA')
                colors = rgba.getcolors(rgba.width * rgba.height)
                foreground = rgba.width * rgba.height - max(n for n, _ in colors)
                if rgba.size != (640, 448) or foreground < 1000 or len(colors) < 64:
                    raise ValueError('empty/degenerate render: ' + label)
                strip.paste(rgba.convert('RGB'), (index % 4 * 640, index // 4 * 448))
                frames.append({'frame': frame, 'seconds': frame / 30,
                               'png_sha256': hashlib.sha256(png.read_bytes()).hexdigest(),
                               'rgba_sha256': hashlib.sha256(rgba.tobytes()).hexdigest(), 'command': command})
            draw.text((index % 4 * 640 + 8, index // 4 * 448 + 8), f'{label} slot={slot} frame={frame:.4f}', fill='white')
        if len({r['rgba_sha256'] for r in frames}) < 4:
            raise ValueError('pose did not visibly change: ' + label)
        path = out / (label + '-strip.png')
        strip.save(path)
        records.append({'sample': label, 'source': identity, 'source_sha256': rows[identity]['sha256'],
                        'slot': slot, 'frames': frames, 'strip_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                        'visual_review': 'REQUIRED; numeric checks are not visual acceptance or retail BBV'})
    (out / 'render-receipt.json').write_text(json.dumps({'records': records}, indent=2) + '\n', encoding='utf8')
    return 0


if __name__ == '__main__':
    sys.exit(main())
