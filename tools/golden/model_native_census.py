#!/usr/bin/env python3
"""Hash-checked Steam census through the production C model parser/decoders.

Only metadata is written. Input assets stream to a native driver over stdin.
Run under the PC governor. No game bytes are sent to remote test machines.
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
import subprocess
import sys
import model_measure as model

INVENTORY_SHA = '150454e1b60d50ccdd58346b6da4e18e363860bf49bb22a679db379dee3771f8'
EXPECTED = {'AMD': 4679, 'GMO': 55, 'pak-v2': 1041, 'pak-v1': 3}
CLASSES = {65536: 'AMD', 65537: 'pak-v2', 65538: 'pak-v1', 65541: 'GMO'}
# Every type occurrence, not merely the total: committed census annex §Every observed GMO type.
EXPECTED_CHUNKS = {
    '0002': 5334, '0003': 5401, '0004': 300622, '0005': 92393, '0006': 92394,
    '0007': 92393, '0008': 38628, '0009': 41151, '000a': 25750, '000b': 46796,
    '000c': 1984942, '8012': 25750, '8013': 25747, '8014': 129136, '8015': 8779,
    '8021': 7, '8022': 7, '8023': 7, '8041': 295220, '8044': 5005, '8045': 5005,
    '8048': 300598, '8049': 300575, '804b': 44, '804c': 300617, '804e': 108626,
    '8061': 92393, '8062': 56602, '8066': 493569, '806b': 1, '8081': 10396,
    '8082': 38602, '8083': 150, '8084': 25, '8085': 38591, '8086': 38555,
    '8087': 38555, '8088': 38555, '8091': 38685, '8092': 38673, '8093': 38673,
    '8094': 41151, '8097': 1985, '8098': 6, '8099': 1985, '809a': 1985,
    '80b1': 45602, '80b2': 45602, '80b3': 1984950, '80e2': 1, '80f1': 59008,
}


def objects(install, rows):
    groups = collections.defaultdict(list)
    for row in rows:
        if model.selected(row):
            chain = row['path'].split('::')
            groups[(chain[0], chain[1])].append(row)
    archives = {}
    for archive, _ in groups:
        if archive not in archives:
            with (install / archive).open('rb') as handle:
                archives[archive] = {n: (o, s, e) for n, o, s, e in model.formats.cpk_entries(model.formats.file_reader(handle))}
    for (archive, member), members in groups.items():
        off, stored, extracted = archives[archive][member]
        with (install / archive).open('rb') as handle:
            handle.seek(off)
            base = handle.read(stored)
        if base[:8] == b'CRILAYLA' and stored != extracted:
            base = model.formats.crilayla(base)
        if len(base) != extracted:
            raise ValueError('CPK extract size: ' + member)
        for row in members:
            data = base
            for name in row['path'].split('::')[2:]:
                _, entries = model.inventory.children(data)
                _, at, size = next(e for e in entries if e[0] == name)
                data = data[at:at + size]
                if data[:8] == b'CRILAYLA':
                    data = model.formats.crilayla(data)
            if len(data) != row['size'] or hashlib.sha256(data).hexdigest() != row['sha256']:
                raise ValueError('inventory identity: ' + row['path'])
            yield row, data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', default=os.environ.get('P4PC_GOLDEN_STEAM'), required=not os.environ.get('P4PC_GOLDEN_STEAM'))
    parser.add_argument('--inventory', required=True)
    parser.add_argument('--native')
    parser.add_argument('--out', required=True)
    parser.add_argument('--static', action='store_true')
    parser.add_argument('--driver', nargs='+', required=True)
    args = parser.parse_args()
    install = Path(args.install).resolve()
    manifest = (install.parent.parent / 'appmanifest_1113000.acf').read_text(encoding='utf8')
    if not all(re.search(r'"%s"\s*"%s"' % (key, re.escape(value)), manifest)
               for key, value in [('appid', '1113000'), ('buildid', '10836033'), ('installdir', install.name)]):
        raise ValueError('not the pinned Steam install')
    out = Path(args.out).resolve()
    if out.is_relative_to(install):
        raise ValueError('output inside install')
    source = Path(args.inventory).read_bytes()
    if hashlib.sha256(source).hexdigest() != INVENTORY_SHA:
        raise ValueError('not the pinned census inventory')
    rows = [json.loads(line) for line in source.splitlines()]
    if args.native:
        dll = ctypes.CDLL(str(Path(args.native).resolve()))
        decode = dll.p4_golden_crilayla
        decode.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t]
        decode.restype = ctypes.c_int

        def native(data):
            length = model.formats.crilayla_size(data)
            dest = ctypes.create_string_buffer(length)
            if decode(data, len(data), dest, length):
                raise ValueError('bounded CRILAYLA refusal')
            return dest.raw
        model.formats.crilayla = native
    argv = args.driver + (['--static'] if args.static else [])
    counts, classes, records, errors = collections.Counter(), collections.Counter(), [], []
    totals = collections.Counter()
    with subprocess.Popen(argv, stdin=subprocess.PIPE, stdout=subprocess.PIPE) as driver:
        try:
            for row, data in objects(install, rows):
                driver.stdin.write(struct.pack('<I', len(data)))
                driver.stdin.write(data)
                driver.stdin.flush()
                line = driver.stdout.readline()
                if not line:
                    raise ValueError('native driver terminated')
                result = json.loads(line)
                record = {'path': row['path'], 'sha256': row['sha256'], **result}
                records.append(record)
                if 'error' in result:
                    errors.append(record)
                    continue
                classes[CLASSES[result['class']]] += 1
                counts.update(result['counts'])
                for key in ('tim2', 'bmp', 'image_failures', 'bones', 'meshes', 'helpers',
                            'motion_clips', 'motion_channels', 'motion_sentinels', 'motion_external', 'motion_neck_channels', 'motion_time_sentinels', 'motion_keys'):
                    totals[key] += result.get(key, 0)
                if result['image_failures']:
                    errors.append(record)
            driver.stdin.close()
            if driver.wait(timeout=30):
                raise ValueError('native driver failed')
        except BaseException:
            driver.kill()
            driver.wait()
            raise
    if dict(classes) != EXPECTED or dict(counts) != EXPECTED_CHUNKS or totals['tim2'] != 25742 or totals['bmp'] != 5:
        errors.append({'error': 'census-exact aggregate mismatch'})
    if (totals['motion_clips'] != 46796 or totals['motion_channels'] != 1984950
            or totals['motion_sentinels'] != 3345 or totals['motion_external'] != 647):
        errors.append({'error': 'census-exact motion/reference mismatch'})
    formats = [sum(r.get('curve_formats', [0, 0, 0])[i] for r in records) for i in range(3)]
    dimensions = [sum(r.get('curve_dimensions', [0, 0, 0])[i] for r in records) for i in range(3)]
    if formats != [1002081, 71470, 911391] or dimensions != [38, 173347, 1811557]:
        errors.append({'error': 'census-exact curve format/dimension mismatch'})
    names = collections.Counter()
    for record in records:
        for clip in record.get('clips', []):
            suffix = re.search(r'-(\d{4})$', clip['name'])
            if clip['ordinal'] == 0:
                names['initial_unsuffixed' if suffix is None else 'initial_suffixed'] += 1
            elif suffix is None:
                names['noninitial_unsuffixed'] += 1
            else:
                names['suffix_agrees' if int(suffix.group(1)) == clip['ordinal'] - 1 else 'suffix_exception'] += 1
    if dict(names) != {'initial_unsuffixed': 4281, 'noninitial_unsuffixed': 81,
                       'suffix_agrees': 42393, 'suffix_exception': 41}:
        errors.append({'error': 'census-exact physical motion-name exception mismatch', 'names': dict(names)})
    result = {'converter': 'amd-gmo-motion@2', 'inventory_sha256': INVENTORY_SHA,
              'objects': len(records), 'classes': dict(classes), 'counts': dict(counts),
              'totals': dict(totals), 'motion_names': dict(names), 'errors': errors, 'records': records}
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n', encoding='utf8')
    print(json.dumps({k: result[k] for k in ('objects', 'classes', 'totals')}, sort_keys=True))
    print('errors=%d static_refusals=%d' % (len(errors), sum('static_refusal' in r for r in records)))
    for error in errors[:10]:
        print(json.dumps(error))
    return bool(errors)


if __name__ == '__main__':
    sys.exit(main())
