#!/usr/bin/env python3
"""Owner-local Golden static models through the production RW/native viewer path.

Run under the PC governor. Each GPU case uses study-priority admission. The script
writes owner-local PNGs/metadata, never extracted assets. PNGs MUST be viewed by
the operator; numeric image checks alone are not visual acceptance or retail BBV.
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

SAMPLES = {
    'character': 'data.cpk::model/npc/n1001_0.AMD',
    'enemy': 'data.cpk::model/pack/em001.pac::em001.AMD',
    'field-prop': 'data.cpk::field/farm_set.AMD',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--viewer', required=True)
    parser.add_argument('--elf', required=True)
    parser.add_argument('--install', required=True)
    parser.add_argument('--inventory', required=True)
    parser.add_argument('--out', required=True)
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
    # A model witness is not a gameplay/retail gate and cannot silently inherit a
    # developer VFS override or a dormant/hybrid source.
    for key in ('P4PC_OVERRIDE', 'P4PC_GOLDEN_VFS_DORMANT', 'P4PC_VIEWER_MODEL', 'P4PC_TEXTURE_DUMP', 'P4PC_CAPTURE_WINDOW'):
        env.pop(key, None)
    records = []
    for label, identity in SAMPLES.items():
        row = rows[identity]
        path = '/'.join(identity.split('::')[1:])
        png, log = out / (label + '.png'), out / (label + '.log')
        command = [str(Path(args.viewer).resolve()), str(Path(args.elf).resolve()), 'golden:' + path, str(png)]
        env['P4PC_SAVE_DIR'] = str(out / ('save-' + label))
        with Machine(priority='study').case():
            run = subprocess.run(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
        log.write_bytes(run.stdout)
        print(label + ': rc=' + str(run.returncode), flush=True)
        if run.returncode:
            print(run.stdout.decode('utf8', errors='replace'))
            return run.returncode
        if row['sha256'].encode() not in run.stdout:
            raise ValueError('selected source hash absent from real VFS log: ' + identity)
        from PIL import Image
        with Image.open(png) as image:
            rgba = image.convert('RGBA')
            colors = rgba.getcolors(rgba.width * rgba.height)
            foreground = rgba.width * rgba.height - max(n for n, _ in colors)
            if foreground < 1000 or len(colors) < 64:
                raise ValueError('empty/degenerate render: ' + label)
            record = {'sample': label, 'source': identity, 'source_sha256': row['sha256'],
                      'png_sha256': hashlib.sha256(png.read_bytes()).hexdigest(),
                      'rgba_sha256': hashlib.sha256(rgba.tobytes()).hexdigest(),
                      'dimensions': list(rgba.size), 'foreground_pixels': foreground, 'colors': len(colors),
                      'command': command, 'visual_review': 'REQUIRED; numeric checks are not visual acceptance'}
        records.append(record)
    (out / 'render-receipt.json').write_text(json.dumps({'records': records}, indent=2) + '\n', encoding='utf8')
    return 0


if __name__ == '__main__':
    sys.exit(main())
