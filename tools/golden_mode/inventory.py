#!/usr/bin/env python3
"""Recursive metadata inventory of a Persona 4 Golden (Steam) install (Golden mode milestone 1).

Every loose file and every CPK member, recursing through nested CPK, Atlus PAK
v1/v2/v3 (.bin/.pac/.arc/.pak/.pack ...) and AFS2 banks, becomes one JSON line:
its chain of names, sizes, SHA-256 of the extracted bytes (and of the stored
bytes when CRILAYLA-compressed), and a format signature label plus the 4-byte
magic. No payload bytes are written. The output directory must lie outside
this repository and outside the install.

  python tools/golden_mode/inventory.py --install <Golden folder> --out <dir> [--jobs N]

Writes <dir>/inventory.jsonl (sorted, one row per object) and <dir>/summary.json.
Each finished batch is checkpointed in <dir>/parts/ (metadata rows, keyed by the
archive's size and modification time), so an interrupted run resumes.
"""
import argparse
import collections
import concurrent.futures
import hashlib
import json
import os
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import formats  # noqa: E402

REPO = Path(__file__).resolve().parents[2]
SEPARATOR = '::'
MAX_DEPTH = 8
BATCH_BYTES = 48 << 20


def locale_of(archive):
    """'data_e.cpk' -> 'e'; locale-neutral archives -> ''."""
    stem = Path(archive).stem.lower()
    if stem.startswith('data_') and stem != 'data_movie':
        return stem[5:]
    return ''


def afs2_entries(data):
    """[(cue id as name, offset, size)] of an AFS2 bank, or None."""
    if len(data) < 16 or data[:4] != b'AFS2':
        return None
    offset_size, id_size = data[5], struct.unpack_from('<H', data, 6)[0]
    count, align = struct.unpack_from('<II', data, 8)
    align &= 0xFFFF
    if offset_size not in (2, 4) or id_size not in (2, 4) or not count or not align or count > 1 << 20:
        return None
    ids_at = 16
    offsets_at = ids_at + count * id_size
    if offsets_at + (count + 1) * offset_size > len(data):
        return None
    idfmt = '<H' if id_size == 2 else '<I'
    offfmt = '<H' if offset_size == 2 else '<I'
    out = []
    for i in range(count):
        cue = struct.unpack_from(idfmt, data, ids_at + i * id_size)[0]
        start = struct.unpack_from(offfmt, data, offsets_at + i * offset_size)[0]
        end = struct.unpack_from(offfmt, data, offsets_at + (i + 1) * offset_size)[0]
        start = (start + align - 1) // align * align
        if start > end or end > len(data):
            return None
        out.append(('%05d' % cue, start, end - start))
    return out


def children(data):
    """(container kind, [(name, offset, size)]) for a recognised container, else (None, None)."""
    if data[:4] == b'CPK ':
        try:
            entries = formats.cpk_entries(formats.bytes_reader(data))
        except (formats.FormatError, struct.error):
            return None, None
        return 'cri-cpk', [(path, offset, stored) for path, offset, stored, _ in entries]
    if data[:4] == b'AFS2':
        entries = afs2_entries(data)
        return ('cri-afs2', entries) if entries else (None, None)
    return formats.pak_entries(data)


def describe(chain, stored_bytes, extract_size, depth, parent_kind):
    """One row and the extracted bytes for a single object (stored bytes as read from its container)."""
    row = {'path': SEPARATOR.join(chain), 'depth': depth, 'container': parent_kind,
           'stored': len(stored_bytes), 'compressed': False}
    data = stored_bytes
    if stored_bytes[:8] == b'CRILAYLA' and (extract_size is None or extract_size != len(stored_bytes)):
        row['compressed'] = True
        row['stored_sha256'] = hashlib.sha256(stored_bytes).hexdigest()
        try:
            data = formats.crilayla(stored_bytes)
        except formats.FormatError as error:
            row['error'] = 'crilayla: %s' % error
            data = None
    if data is not None:
        if extract_size is not None and extract_size != len(data):
            row['error'] = 'extract size %d != %d' % (extract_size, len(data))
        row['size'] = len(data)
        row['sha256'] = hashlib.sha256(data).hexdigest()
        row['signature'] = formats.signature(data[:16])
        row['magic'] = formats.magic_hex(data)
    return row, data


def walk(chain, stored_bytes, extract_size, depth, parent_kind, rows):
    row, data = describe(chain, stored_bytes, extract_size, depth, parent_kind)
    rows.append(row)
    if data is None or depth >= MAX_DEPTH:
        return
    kind, entries = children(data)
    if not kind:
        return
    row['kind'] = kind
    row['children'] = len(entries)
    for name, offset, size in entries:
        walk(chain + [name], data[offset:offset + size], None, depth + 1, kind, rows)


def scan_batch(archive_path, archive_name, members):
    """Rows for a batch of top-level CPK members [(index, path, offset, stored, extracted)]."""
    rows = []
    with open(archive_path, 'rb') as handle:
        for index, path, offset, stored, extracted in members:
            handle.seek(offset)
            raw = handle.read(stored)
            batch_rows = []
            if len(raw) != stored:
                batch_rows.append({'path': SEPARATOR.join([archive_name, path]), 'depth': 1, 'container': 'cri-cpk',
                                   'stored': stored, 'error': 'short read'})
            else:
                walk([archive_name, path], raw, extracted, 1, 'cri-cpk', batch_rows)
            for row in batch_rows:
                row['archive'] = archive_name
                row['locale'] = locale_of(archive_name)
                row['index'] = index
            rows.extend(batch_rows)
    return rows


def hash_file(path, chunk=8 << 20):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        head = handle.read(16)
        digest.update(head)
        while True:
            block = handle.read(chunk)
            if not block:
                break
            digest.update(block)
    return digest.hexdigest(), head


def read_object(install, path):
    """Extracted bytes of one inventory path ("data_e.cpk::init_free.bin::battle/SKILL.TBL"), in memory."""
    chain = path.split(SEPARATOR)
    with open(Path(install) / chain[0], 'rb') as handle:
        data = None
        if len(chain) == 1:
            return handle.read()
        for entry_path, offset, stored, extracted in formats.cpk_entries(formats.file_reader(handle)):
            if entry_path.lower() == chain[1].lower():
                handle.seek(offset)
                data = handle.read(stored)
                if data[:8] == b'CRILAYLA' and stored != extracted:
                    data = formats.crilayla(data)
                break
    if data is None:
        raise KeyError(path)
    for name in chain[2:]:
        _, entries = children(data)
        match = [(o, s) for n, o, s in entries or () if n.lower() == name.lower()]
        if not match:
            raise KeyError(path)
        data = data[match[0][0]:match[0][0] + match[0][1]]
        if data[:8] == b'CRILAYLA':
            data = formats.crilayla(data)
    return data


def batches(members, limit=BATCH_BYTES):
    batch, total = [], 0
    for member in members:
        batch.append(member)
        total += member[3]
        if total >= limit:
            yield batch
            batch, total = [], 0
    if batch:
        yield batch


def check_output(out, install):
    out = Path(out).resolve()
    for forbidden, label in ((REPO.resolve(), 'repository'), (Path(install).resolve(), 'install')):
        if out == forbidden or forbidden in out.parents:
            raise SystemExit('refusing to write inventory inside the %s (%s)' % (label, forbidden))
    return out


def part_path(parts, archive_path, archive_name, batch):
    """Checkpoint file of one batch, keyed by archive identity (size, mtime) and member range."""
    stat = Path(archive_path).stat()
    identity = hashlib.sha256(('%d:%d' % (stat.st_size, stat.st_mtime_ns)).encode()).hexdigest()[:12]
    return Path(parts) / ('%s-%s-%06d-%06d.jsonl' % (archive_name.replace('/', '_'), identity, batch[0][0],
                                                     batch[-1][0]))


def load_part(path):
    with open(path, encoding='utf-8') as handle:
        return [json.loads(line) for line in handle if line.strip()]


def save_part(path, rows):
    temporary = path.with_suffix('.tmp')
    with open(temporary, 'w', encoding='utf-8', newline='\n') as handle:
        for row in rows:
            handle.write(json.dumps(row, sort_keys=True) + '\n')
    temporary.replace(path)


def inventory(install, jobs=1, parts=None):
    """All rows for an install, sorted by (archive, index, path).

    With parts (a directory), every finished batch is checkpointed there and a rerun over the same
    install reuses it, so an interrupted inventory resumes instead of starting again."""
    install = Path(install)
    rows = []
    tasks = []
    for path in sorted(install.rglob('*')):
        if not path.is_file():
            continue
        relative = path.relative_to(install).as_posix()
        digest, head = hash_file(path)
        row = {'path': relative, 'depth': 0, 'container': 'filesystem', 'stored': path.stat().st_size,
               'size': path.stat().st_size, 'compressed': False, 'sha256': digest,
               'signature': formats.signature(head), 'magic': formats.magic_hex(head),
               'archive': relative, 'locale': locale_of(relative), 'index': -1}
        rows.append(row)
        if head[:4] == b'CPK ':
            with open(path, 'rb') as handle:
                try:
                    entries = formats.cpk_entries(formats.file_reader(handle))
                except (formats.FormatError, struct.error) as error:
                    row['error'] = 'cpk: %s' % error
                    continue
            row['kind'] = 'cri-cpk'
            row['children'] = len(entries)
            members = [(i,) + tuple(entry) for i, entry in enumerate(entries)]
            tasks.extend((str(path), relative, batch) for batch in batches(members))
    pending = []
    for task in tasks:
        checkpoint = part_path(parts, *task) if parts else None
        if checkpoint and checkpoint.is_file():
            rows.extend(load_part(checkpoint))
        else:
            pending.append((task, checkpoint))
    done = len(tasks) - len(pending)

    def finish(result, checkpoint):
        nonlocal done
        if checkpoint:
            save_part(checkpoint, result)
        rows.extend(result)
        done += 1
        if parts:
            print('batch %d/%d' % (done, len(tasks)), file=sys.stderr, flush=True)

    if jobs <= 1:
        for task, checkpoint in pending:
            finish(scan_batch(*task), checkpoint)
    elif pending:
        with concurrent.futures.ProcessPoolExecutor(max_workers=jobs) as pool:
            futures = {pool.submit(scan_batch, *task): checkpoint for task, checkpoint in pending}
            for future in concurrent.futures.as_completed(futures):
                finish(future.result(), futures[future])
    rows.sort(key=lambda r: (r['archive'], r['index'], r['path']))
    return rows


def summarize(rows):
    archives = collections.OrderedDict()
    signatures = collections.Counter()
    kinds = collections.Counter()
    errors = 0
    for row in rows:
        entry = archives.setdefault(row['archive'], {'locale': row['locale'], 'objects': 0, 'bytes': 0,
                                                     'compressed': 0, 'containers': 0, 'max_depth': 0,
                                                     'by_depth': collections.Counter()})
        entry['objects'] += 1
        entry['bytes'] += row.get('size', 0)
        entry['compressed'] += bool(row.get('compressed'))
        entry['containers'] += 'kind' in row
        entry['max_depth'] = max(entry['max_depth'], row['depth'])
        entry['by_depth'][row['depth']] += 1
        signatures[row.get('signature', '?')] += 1
        if 'kind' in row:
            kinds[row['kind']] += 1
        errors += 'error' in row
    for entry in archives.values():
        entry['by_depth'] = {str(k): v for k, v in sorted(entry['by_depth'].items())}
    return {'objects': len(rows), 'errors': errors, 'archives': archives,
            'containers_by_kind': dict(kinds.most_common()), 'signatures': dict(signatures.most_common())}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--install', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--jobs', type=int, default=1)
    args = parser.parse_args(argv)
    out = check_output(args.out, args.install)
    (out / 'parts').mkdir(parents=True, exist_ok=True)
    rows = inventory(args.install, args.jobs, out / 'parts')
    with open(out / 'inventory.jsonl', 'w', encoding='utf-8', newline='\n') as handle:
        for row in rows:
            handle.write(json.dumps(row, sort_keys=True) + '\n')
    summary = summarize(rows)
    with open(out / 'summary.json', 'w', encoding='utf-8', newline='\n') as handle:
        json.dump(summary, handle, indent=1, sort_keys=True)
        handle.write('\n')
    print('objects %d errors %d archives %d' % (summary['objects'], summary['errors'], len(summary['archives'])))
    return 0


if __name__ == '__main__':
    sys.exit(main())
