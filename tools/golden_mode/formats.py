"""Read-only readers for the containers of a Persona 4 Golden (Steam) install.

Golden mode milestone 1 measures the user's own install: names, sizes, hashes
and format signatures. These readers parse in memory and return metadata; the
callers never write payload bytes anywhere. The formats are the ones the C
reader already implements (include/p4pc/golden_source.h, docs/modding/golden.md):

- CRI CPK: a "CPK " chunk whose @UTF header names the TOC; the "TOC " chunk's
  @UTF table has one row per member (DirName, FileName, FileSize, ExtractSize,
  FileOffset). Masked @UTF packets are unmasked first.
- CRILAYLA: CRI's backwards LZ stream with a raw 0x100-byte prefix.
- Atlus PAK families: v1 (252-byte name, u32 size, 64-byte alignment; Golden's
  .bin/.pac/.arc), v2 (u32 count, 32-byte name, u32 size, contiguous) and v3
  (u32 count, 24-byte name, u32 size, contiguous). Detection is structural,
  never by extension.
"""
import struct

UTF_SIZES = (1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 4, 8)
UTF_FORMATS = ('>B', '>b', '>H', '>h', '>I', '>i', '>Q', '>q', '>f', '>d')


class FormatError(ValueError):
    """A structure is malformed or out of bounds."""


# ---- @UTF ---------------------------------------------------------------------

def utf_unmask(packet):
    out = bytearray(packet)
    m = 0x655F
    for i in range(len(out)):
        out[i] ^= m & 0xFF
        m = (m * 0x4115) & 0xFFFFFFFF
    return bytes(out)


def utf_table(packet):
    """Rows of an @UTF packet as a list of {column: value}; strings decoded, data cells (offset, size)."""
    if packet[:4] != b'@UTF':
        packet = utf_unmask(packet)
        if packet[:4] != b'@UTF':
            raise FormatError('not an @UTF packet')
    if len(packet) < 32:
        raise FormatError('truncated @UTF packet')
    size = struct.unpack_from('>I', packet, 4)[0]
    if size < 24 or size > len(packet) - 8:
        raise FormatError('@UTF size out of range')
    d = packet[8:8 + size]
    rows_off, strings_off, data_off = struct.unpack_from('>HII', d, 2)
    columns, row_width, row_count = struct.unpack_from('>HHI', d, 16)
    if strings_off > size or data_off > size or rows_off > strings_off or row_width * row_count > strings_off - rows_off:
        raise FormatError('@UTF layout out of range')

    def string(offset):
        start = strings_off + offset
        if start >= size:
            raise FormatError('@UTF string out of range')
        end = d.find(b'\0', start)
        if end < 0:
            raise FormatError('@UTF string unterminated')
        return d[start:end].decode('utf-8', 'replace')

    def value(kind, offset):
        if kind < 10:
            return struct.unpack_from(UTF_FORMATS[kind], d, offset)[0]
        if kind == 10:
            return string(struct.unpack_from('>I', d, offset)[0])
        start, length = struct.unpack_from('>II', d, offset)
        return (data_off + start, length)

    spec = []
    cursor = 24
    row_cursor = 0
    for _ in range(columns):
        if cursor + 5 > rows_off:
            raise FormatError('@UTF columns overrun')
        flags = d[cursor]
        storage, kind = flags & 0xF0, flags & 0x0F
        if kind >= len(UTF_SIZES):
            raise FormatError('@UTF column type')
        name = string(struct.unpack_from('>I', d, cursor + 1)[0])
        cursor += 5
        width = UTF_SIZES[kind]
        if storage & 0x20:
            if cursor + width > rows_off:
                raise FormatError('@UTF constant overrun')
            spec.append((name, 'const', value(kind, cursor)))
            cursor += width
        elif storage & 0x40:
            spec.append((name, 'row', (kind, row_cursor)))
            row_cursor += width
            if row_cursor > row_width:
                raise FormatError('@UTF row overrun')
        else:
            spec.append((name, 'zero', None))
    rows = []
    for r in range(row_count):
        base = rows_off + r * row_width
        row = {}
        for name, storage, info in spec:
            if storage == 'const':
                row[name] = info
            elif storage == 'row':
                row[name] = value(info[0], base + info[1])
            else:
                row[name] = None
        rows.append(row)
    return rows


# ---- CRILAYLA -----------------------------------------------------------------

def crilayla_size(src):
    """Uncompressed size of a CRILAYLA stream, or 0 when src is not one."""
    if len(src) < 0x110 or src[:8] != b'CRILAYLA':
        return 0
    body, prefix = struct.unpack_from('<II', src, 8)
    if prefix + 0x110 > len(src):
        return 0
    return body + 0x100


def crilayla(src):
    total = crilayla_size(src)
    if not total:
        raise FormatError('not a CRILAYLA stream')
    body = total - 0x100
    prefix = struct.unpack_from('<I', src, 12)[0]
    out = bytearray(total)
    out[0:0x100] = src[prefix + 0x10:prefix + 0x110]
    if not body:
        return bytes(out)
    position = len(src) - 0x100 - 1
    pool = 0
    left = 0
    end = 0x100 + body - 1
    done = 0

    def bits(count):
        nonlocal position, pool, left
        value = 0
        while count:
            if not left:
                if position < 0x10:
                    raise FormatError('CRILAYLA bit stream truncated')
                pool = src[position]
                position -= 1
                left = 8
            take = left if left < count else count
            value = (value << take) | ((pool >> (left - take)) & ((1 << take) - 1))
            left -= take
            count -= take
        return value

    while done < body:
        if bits(1):
            reference = end - done + bits(13) + 3
            length = 3
            for width in (2, 3, 5, 8):
                part = bits(width)
                length += part
                if part != (1 << width) - 1:
                    break
            else:
                while True:
                    part = bits(8)
                    length += part
                    if part != 255:
                        break
            if reference > end or length > body - done:
                raise FormatError('CRILAYLA back reference out of range')
            for _ in range(length):
                out[end - done] = out[reference]
                reference -= 1
                done += 1
        else:
            out[end - done] = bits(8)
            done += 1
    return bytes(out)


# ---- CPK ------------------------------------------------------------------------

def _chunk(read, offset, magic):
    header = read(offset, 16)
    if len(header) < 16 or header[:4] != magic:
        raise FormatError('missing %r chunk' % magic)
    length = struct.unpack_from('<I', header, 8)[0]
    if length < 32 or length > (256 << 20):
        raise FormatError('chunk length out of range')
    packet = read(offset + 16, length)
    if len(packet) != length:
        raise FormatError('chunk truncated')
    return packet


def cpk_entries(read):
    """Members of a CPK given read(offset, size) -> bytes.

    Returns [(path, offset, stored, extracted)] with absolute offsets, in TOC order.
    """
    header = utf_table(_chunk(read, 0, b'CPK '))
    if not header:
        raise FormatError('empty CPK header')
    toc = header[0].get('TocOffset')
    if not toc:
        raise FormatError('CPK has no TOC')
    content = header[0].get('ContentOffset') or toc
    base = min(toc, content)
    out = []
    for row in utf_table(_chunk(read, toc, b'TOC ')):
        directory = row.get('DirName') or ''
        name = row.get('FileName') or ''
        path = (directory + '/' + name) if directory else name
        stored = row.get('FileSize') or 0
        extracted = row.get('ExtractSize')
        if extracted is None:
            extracted = stored
        out.append((path, base + (row.get('FileOffset') or 0), stored, extracted))
    return out


def bytes_reader(data):
    return lambda offset, size: data[offset:offset + size]


def file_reader(handle):
    def read(offset, size):
        handle.seek(offset)
        return handle.read(size)
    return read


# ---- Atlus PAK families -----------------------------------------------------------

def _name(field):
    """A pack entry name: printable ASCII, NUL-terminated and NUL-padded to the field's end.

    Every measured Golden pack pads with zeros; requiring it keeps files that merely start with a
    short printable magic ("DDS |", "GXP") from reading as one-entry packs."""
    end = field.find(b'\0')
    if end <= 0 or any(field[end:]):
        return None
    raw = field[:end]
    if any(c < 0x20 or c > 0x7E for c in raw):
        return None
    return raw.decode('ascii')


def pak_v1(data):
    """[(name, offset, size)] of a 252-byte-name pack, or None when the structure does not hold."""
    out = []
    offset = 0
    n = len(data)
    while offset + 256 <= n and data[offset]:
        name = _name(data[offset:offset + 252])
        if name is None:
            return None
        size = struct.unpack_from('<I', data, offset + 252)[0]
        offset += 256
        if size > n - offset:
            return None
        out.append((name, offset, size))
        offset = (offset + size + 63) & ~63
        # Some packs align to 128: skip zero padding up to the next 128-byte boundary.
        while offset + 256 <= n and not data[offset] and offset % 128:
            offset += 64
    if not any(size for _, _, size in out) or any(data[offset:]):
        return None
    return out


def _pak_counted(data, width, order):
    if len(data) < 4:
        return None
    count = struct.unpack_from(order + 'I', data, 0)[0]
    if not 0 < count <= 100000:
        return None
    out = []
    offset = 4
    n = len(data)
    for _ in range(count):
        if offset + width + 4 > n:
            return None
        name = _name(data[offset:offset + width])
        if name is None:
            return None
        size = struct.unpack_from(order + 'I', data, offset + width)[0]
        offset += width + 4
        if size > n - offset:
            return None
        out.append((name, offset, size))
        offset += size
    if not any(size for _, _, size in out) or any(data[offset:]):
        return None
    return out


def pak_entries(data):
    """(variant, [(name, offset, size)]) for a recognised Atlus pack, else (None, None)."""
    for variant, parse in (('pak-v1', pak_v1),
                           ('pak-v2', lambda d: _pak_counted(d, 32, '<')),
                           ('pak-v3', lambda d: _pak_counted(d, 24, '<')),
                           ('pak-v2be', lambda d: _pak_counted(d, 32, '>')),
                           ('pak-v3be', lambda d: _pak_counted(d, 24, '>'))):
        entries = parse(data)
        if entries:
            return variant, entries
    return None, None


# ---- Signatures -----------------------------------------------------------------

MAGIC0 = (
    (b'CPK ', 'cri-cpk'), (b'CRILAYLA', 'crilayla'), (b'@UTF', 'cri-utf'), (b'AFS2', 'cri-afs2'),
    (b'AFS\0', 'afs'), (b'CRID', 'cri-usm'), (b'@SFD', 'cri-sfd'), (b'HCA\0', 'cri-hca'),
    (b'RIFF', 'riff'), (b'OggS', 'ogg'), (b'MZ', 'pe'), (b'\x89PNG', 'png'), (b'DDS ', 'dds'),
    (b'GXT\0', 'gxt'), (b'BM', 'bmp'), (b'\x7fELF', 'elf'), (b'PK\x03\x04', 'zip'), (b'TXP0', 'txp'),
    (b'SPR0', 'spr'), (b'SPD0', 'spd'), (b'DSCS', 'dscs'), (b'EPL\0', 'epl'), (b'FTD\0', 'ftd'),
    (b'GFS0', 'gfs'), (b'IFFX', 'iff'), (b'VAGp', 'vag'), (b'BED4', 'bed'), (b'CHNK', 'amd-chunk'),
    (b'\xef\xbb\xbf', 'text-utf8-bom'),
)
MAGIC8 = ((b'TMX0', 'tmx'), (b'FLW0', 'flow-bf'), (b'MSG1', 'message-bmd'), (b'SPR0', 'spr'),
          (b'TXP0', 'txp'), (b'FNT0', 'font'), (b'PAK0', 'pak0'))


def signature(head):
    """A format label from the first bytes (up to 16 suffice); never more than a magic number."""
    for magic, label in MAGIC0:
        if head.startswith(magic):
            return label
    for magic, label in MAGIC8:
        if head[8:12] == magic:
            return label
    if len(head) >= 4 and head[:4] in (b'\x10\0\0\0', b'\x16\0\0\0'):
        return 'rw-chunk-%02x' % head[0]
    if not head:
        return 'empty'
    if not any(head):
        return 'zero-prefix'
    if all(0x20 <= c < 0x7F or c in (9, 10, 13) for c in head):
        return 'text-ascii'
    return 'magic:' + head[:4].hex()


def magic_hex(head):
    """The first four bytes as hex: a format magic, recorded instead of any payload."""
    return head[:4].hex()
