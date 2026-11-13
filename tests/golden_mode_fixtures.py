"""Synthetic container writers for the Golden mode milestone-1 tool tests (no Golden or retail bytes).

The writers produce the structures tools/golden_mode/formats.py reads: CRI @UTF
tables (optionally masked), CPK archives, CRILAYLA streams, Atlus PAK v1/v2/v3
packs, packed-bank TBLs and a minimal MIPS ELF32.
"""
import struct

UTF_TYPES = {'u8': (0, '>B'), 'u16': (2, '>H'), 'u32': (4, '>I'), 'u64': (6, '>Q'), 'str': (10, None)}


def utf_packet(columns, rows, masked=False):
    """An @UTF packet. columns: [(name, type, 'row' | ('const', value))]; rows: [{name: value}]."""
    strings = bytearray(b'<NULL>\0')
    offsets = {}

    def string(text):
        if text not in offsets:
            offsets[text] = len(strings)
            strings.extend(text.encode('ascii') + b'\0')
        return offsets[text]

    schema = bytearray()
    row_layout = []
    for name, kind, storage in columns:
        code, fmt = UTF_TYPES[kind]
        if storage == 'row':
            schema += bytes([0x50 | code]) + struct.pack('>I', string(name))
            row_layout.append((name, code, fmt))
        else:
            value = storage[1]
            schema += bytes([0x30 | code]) + struct.pack('>I', string(name))
            schema += struct.pack('>I', string(value)) if code == 10 else struct.pack(fmt, value)
    width = sum(4 if code == 10 else struct.calcsize(fmt) for _, code, fmt in row_layout)
    body = bytearray()
    for row in rows:
        for name, code, fmt in row_layout:
            body += struct.pack('>I', string(row[name])) if code == 10 else struct.pack(fmt, row[name])
    table_name = string('table')
    rows_offset = 24 + len(schema)
    strings_offset = rows_offset + len(body)
    data_offset = strings_offset + len(strings)
    header = struct.pack('>HHIIIHHI', 1, rows_offset, strings_offset, data_offset, table_name,
                         len(columns), width, len(rows))
    table = header + schema + body + strings
    packet = b'@UTF' + struct.pack('>I', len(table)) + table
    if masked:
        out = bytearray(packet)
        m = 0x655F
        for i in range(len(out)):
            out[i] ^= m & 0xFF
            m = (m * 0x4115) & 0xFFFFFFFF
        packet = bytes(out)
    return packet


def chunk(magic, packet):
    return magic + struct.pack('<III', 0xFF, len(packet), 0) + packet


def cpk(members, masked_toc=False):
    """A CPK archive. members: [(path, stored bytes, extracted size)]; content follows the TOC."""
    toc_offset = 0x800
    columns = [('DirName', 'str', 'row'), ('FileName', 'str', 'row'), ('FileSize', 'u32', 'row'),
               ('ExtractSize', 'u32', 'row'), ('FileOffset', 'u64', 'row')]

    def toc_for(offsets):
        rows = []
        for (path, stored, extracted), offset in zip(members, offsets):
            directory, _, name = path.rpartition('/')
            rows.append({'DirName': directory, 'FileName': name, 'FileSize': len(stored),
                         'ExtractSize': extracted, 'FileOffset': offset})
        return chunk(b'TOC ', utf_packet(columns, rows, masked_toc))

    size = len(toc_for([0] * len(members)))
    content = (toc_offset + size + 0x7FF) & ~0x7FF
    offsets, cursor = [], content - toc_offset
    for _, stored, _ in members:
        offsets.append(cursor)
        cursor = (cursor + len(stored) + 0x7FF) & ~0x7FF
    toc = toc_for(offsets)
    header = chunk(b'CPK ', utf_packet([('TocOffset', 'u64', 'row'), ('ContentOffset', 'u64', 'row'),
                                        ('Files', 'u32', 'row')],
                                       [{'TocOffset': toc_offset, 'ContentOffset': content, 'Files': len(members)}]))
    out = bytearray(header.ljust(toc_offset, b'\0'))
    out += toc
    for (_, stored, _), offset in zip(members, offsets):
        out = out.ljust(toc_offset + offset, b'\0')
        out += stored
    return bytes(out)


def crilayla(plain):
    """A CRILAYLA stream for plain (at least 0x100 bytes) with literals and greedy back references."""
    if len(plain) < 0x100:
        raise ValueError('CRILAYLA needs the 0x100-byte raw prefix')
    prefix, body = plain[:0x100], plain[0x100:]
    reverse = body[::-1]  # the decoder writes the body from its end
    bits = []

    def put(value, width):
        bits.extend((value >> (width - 1 - i)) & 1 for i in range(width))

    i = 0
    while i < len(reverse):
        best_length, best_distance = 0, 0
        for distance in range(3, min(i, 8194) + 1):
            length = 0
            while i + length < len(reverse) and reverse[i + length] == reverse[i + length - distance] and length < 300:
                length += 1
            if length > best_length:
                best_length, best_distance = length, distance
        if best_length >= 3:
            put(1, 1)
            put(best_distance - 3, 13)
            remaining = best_length - 3
            for width in (2, 3, 5, 8):
                full = (1 << width) - 1
                part = min(remaining, full)
                put(part, width)
                remaining -= part
                if part != full:
                    break
            else:
                while True:
                    part = min(remaining, 255)
                    put(part, 8)
                    remaining -= part
                    if part != 255:
                        break
            i += best_length
        else:
            put(0, 1)
            put(reverse[i], 8)
            i += 1
    while len(bits) % 8:
        bits.append(0)
    emitted = bytes(int(''.join(map(str, bits[k:k + 8])), 2) for k in range(0, len(bits), 8))
    stream = emitted[::-1]  # the decoder reads from the end of the stream towards its start
    return b'CRILAYLA' + struct.pack('<II', len(body), len(stream)) + stream + prefix


def pak_v1(entries, align=64):
    out = bytearray()
    for name, payload in entries:
        out += name.encode('ascii').ljust(252, b'\0') + struct.pack('<I', len(payload)) + payload
        out += b'\0' * (-len(out) % align)
    return bytes(out)


def pak_counted(entries, width):
    out = bytearray(struct.pack('<I', len(entries)))
    for name, payload in entries:
        out += name.encode('ascii').ljust(width, b'\0') + struct.pack('<I', len(payload)) + payload
    return bytes(out)


def tbl(spans):
    """Packed-bank TBL: u32 length, payload, each record 16-aligned."""
    out = bytearray()
    for payload in spans:
        out += struct.pack('<I', len(payload)) + payload
        out += b'\0' * (-len(out) % 16)
    return bytes(out)


def mips_elf(vaddr, image):
    """A little-endian ELF32 with one PT_LOAD of image at vaddr."""
    header = bytearray(0x34)
    header[0:6] = b'\x7fELF\x01\x01'
    struct.pack_into('<HHI', header, 0x10, 2, 8, 1)
    struct.pack_into('<I', header, 0x1C, 0x34)
    struct.pack_into('<HH', header, 0x2A, 0x20, 1)
    phdr = struct.pack('<8I', 1, 0x1000, vaddr, vaddr, len(image), len(image), 7, 0x1000)
    return bytes(header + phdr).ljust(0x1000, b'\0') + image


def lui(rt, value):
    return 0x3C000000 | rt << 16 | value


def itype(op, rs, rt, imm):
    return op << 26 | rs << 21 | rt << 16 | (imm & 0xFFFF)
