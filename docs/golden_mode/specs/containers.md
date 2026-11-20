# Containers

[Specs index](README.md)

## Sources

- The port's reader `src/platform/golden/golden_formats.c` and its tests (`tests/golden_source_test.c`):
  CPK, @UTF (including masked packets), CRILAYLA, Golden's sequential `.bin`, packed-bank TBL spans.
- Milestone-1 measurement (`tools/golden_mode/formats.py`, `inventory.py`): every one of the 161,154
  objects of build 10836033 parsed with 0 errors; 288 PAK v1 and 3,960 PAK v2 packs, all with NUL-padded
  names.
- Milestone-3 measurement (`tools/golden_mode/layouts.py`): all 105 `fclTable.bin` members, 37 of 38
  `cmpTable.bin` members and 35 of 36 `cmm.bin` members fit the FTD/CTD and DAT containers below (the
  remaining two are BMD message files).
- Public: Amicitia wiki, "FTD" (header of payload size and entry count, 8 padding bytes; entry size =
  size / count) and "SKILL.TBL" (packed tables padded to 16 bytes).

## Container

All integers are little-endian unless stated.

### CRI CPK and @UTF

A CPK file starts with a `CPK ` chunk: 4-byte magic, u32 flags, u32 packet length at +8, u32 at +12,
then the packet. The packet is an @UTF table; if it does not begin with `@UTF` it is masked and is
unmasked byte by byte with `m = 0x655F; b ^= m & 0xFF; m = m * 0x4115 (mod 2^32)`. @UTF (big-endian):
`@UTF`, u32 table size, then u16 version, u16 rows offset, u32 strings offset, u32 data offset, u32 table
name, u16 columns, u16 row width, u32 rows; columns are a flags byte (storage `0x10` zero, `0x30`
constant, `0x50` per row; type `0..9` numbers of 1/1/2/2/4/4/8/8/4/8 bytes, `0x0A` string offset,
`0x0B` data offset+size) and a u32 name offset, constants inline. Offsets are relative to the byte after
the size word. The header table's `TocOffset` (and `ContentOffset`) locate the `TOC ` chunk, whose @UTF
rows carry `DirName`, `FileName`, `FileSize` (stored), `ExtractSize`, `FileOffset`; a member's absolute
offset is `min(TocOffset, ContentOffset) + FileOffset`.

### CRILAYLA

`CRILAYLA`, u32 body size, u32 compressed size, the compressed bit stream, then a raw 0x100-byte prefix
that becomes the first 0x100 output bytes. The body is written from its end towards its start, reading
bits from the stream's end: bit 1 = back reference (13-bit offset + 3; length 3 + a 2/3/5/8-bit ladder,
then 8-bit chunks while 255), bit 0 = 8-bit literal. A member is compressed when its stored size differs
from `ExtractSize` and it starts with `CRILAYLA`; the decoded size must equal `ExtractSize`.

### Atlus PAK

- **v1** (`.bin`, `.pac`, `.pack`): repeated records of a 252-byte NUL-padded name, u32 size, payload,
  padded to 64 (some packs to 128) bytes; ends at a zero name byte or the end of the file.
- **v2** (`.arc`, `.bin`, `.pac`, `.se`): u32 count, then count records of a 32-byte NUL-padded name,
  u32 size and the payload, contiguous.
- Names are printable ASCII, NUL-terminated and NUL-padded to the field end. A reader detects the
  variant by structure, never by extension, and refuses trailing non-zero bytes.

### FTD and CTD tables

```
u32 payload_bytes
u32 count          rows of the table
u32 0, u32 0
u8  payload[payload_bytes]   count rows of payload_bytes / count bytes
padding to the next multiple of 32 bytes
```

The member size is exactly `align32(16 + payload_bytes)`. Padding content is unspecified (Golden's files
often carry a stale copy of the header there) and must not be read. Every measured FTD/CTD has
`payload_bytes % count == 0`.

### cmm DAT tables

```
u32 payload_bytes
u32 count
u8  payload[payload_bytes]
padding to the next multiple of 32 bytes (content unspecified)
```

The member size is exactly `align32(8 + payload_bytes)`. `count` is the number of top-level records;
for most DAT tables that is the row count (`cmm_tables.md` lists the exceptions).

### Packed-bank TBL

Repeated blocks: u32 length, `length` payload bytes, zero padding so the next block starts on a 16-byte
boundary (relative to the file start). The block list ends at the end of the file (a zero length within
the last 16 bytes is padding). Block meaning is by index (`battle_tables.md`).

### ITEMTBL sections

Golden's `init/itemtbl.bin` is two sections, each `u32 rows, u32 1, u32 256, u32 1` and then `rows`
fixed-size rows: section 0 has 68-byte rows, section 1 has 24-byte rows; the file ends after section 1.
PS2's `INIT/ITEMTBL.BIN` is section 0 only (with its own row count).

## Records

Record layouts belong to the table specs. Containers only fix row count and row size.

## ID domains

None at this level. The CPK row index, the PAK entry order and the block index are positions, not IDs.

## Invariants and refusals

- CPK: chunk magic, packet bounds, @UTF bounds (rows within strings, columns within rows), decoded size
  equal to `ExtractSize`.
- PAK: every size within the file; names as above; nothing after the last record but zero padding.
- FTD/CTD: bytes 8..15 zero; member size `align32(16 + payload_bytes)`.
- DAT: member size `align32(8 + payload_bytes)`.
- TBL: every block length within the file.
- ITEMTBL: both section headers `rows, 1, 256, 1`; the file ends exactly after section 1.

## PS2 differences

PS2 has no CPK, FTD, CTD or DAT tables: its battle tables are packed-bank TBLs inside `BTL.CVM` and its
fusion/inheritance tables are in the executable image. PS2 packs are PAK v1 only.

## Test vectors

```golden-vector
name: ftd-three-rows-stale-padding
container: ftd
hex: 12000000030000000000000000000000 01001000414202002000430003003000 00001200000003000000000000000000 00000000000000000000000000000000
expect: {"rows": 3, "payload": "010010004142020020004300030030000000"}
```

```golden-vector
name: ftd-wrong-member-size
container: ftd
hex: 12000000030000000000000000000000 01001000414202002000430003003000 0000
expect: null
```

```golden-vector
name: ftd-nonzero-reserved
container: ftd
hex: 02000000010000000100000000000000 0102 00*14
expect: null
```

```golden-vector
name: dat-two-rows
container: dat
hex: 10000000020000000100020023010000 03000400560400000000000000000000
expect: {"rows": 2, "payload": "01000200230100000300040056040000"}
```

```golden-vector
name: tbl-two-blocks
container: tbl
hex: 06000000010203040506000000000000 12000000101112131415161718191a1b 1c1d1e1f202100000000000000000000
expect: {"spans": ["010203040506", "101112131415161718191a1b1c1d1e1f2021"]}
```

```golden-vector
name: tbl-overrun
container: tbl
hex: 20000000010203
expect: null
```

```golden-vector
name: itemtbl-two-sections
container: itemtbl
hex: 01000000010000000001000001000000 000102030405060708090a0b0c0d0e0f 101112131415161718191a1b1c1d1e1f 202122232425262728292a2b2c2d2e2f 303132333435363738393a3b3c3d3e3f 40414243010000000100000000010000 01000000808182838485868788898a8b 8c8d8e8f9091929394959697
expect: {"sections": [[1, 68, "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40414243"], [1, 24, "808182838485868788898a8b8c8d8e8f9091929394959697"]]}
```

```golden-vector
name: pak-v1-two-entries
container: pak
hex: 612e667464 00*247 03000000 010203 00*61 622e646174 00*247 01000000 04 00*63
expect: {"variant": "pak-v1", "entries": [["a.ftd", "010203"], ["b.dat", "04"]]}
```

```golden-vector
name: pak-v2-two-entries
container: pak
hex: 02000000612e66746400000000000000 00000000000000000000000000000000 0000000003000000010203622e646174 00000000000000000000000000000000 00000000000000000000000100000004
expect: {"variant": "pak-v2", "entries": [["a.ftd", "010203"], ["b.dat", "04"]]}
```

```golden-vector
name: pak-v2-name-not-padded
container: pak
hex: 01000000612e667464004141 00*24 0100000004
expect: null
```

## Status

Complete for wave 1. CPK and CRILAYLA vectors are the existing synthetic tests
(`tests/golden_source_test.c`, `tests/golden_mode_formats_test.py`); they are not repeated here because a
valid CRILAYLA stream needs a 0x100-byte prefix.
