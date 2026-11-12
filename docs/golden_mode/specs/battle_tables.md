# Battle tables and ITEMTBL

[Specs index](README.md) · [Containers](containers.md)

## Sources

- Measurement of build 10836033 (`tools/golden_mode/layouts.py`, `tools/golden_mode/sources.py`):
  block sizes, row counts and per-block equality with PS2 (metadata only).
- PS2 layouts and roles: `config/game_tables.json` (physical banks), `include/p4pc/game_encounters.h`,
  `include/p4pc/game_items.h` (`P4ItemRow`), `include/p4pc/game_personas.h`, `include/p4pc/game_enemies.h`.
- PS2-to-Golden row rules already proven in the port's Golden mechanics package:
  `src/platform/golden/golden_tables.c` and [Golden mechanics](../../modding/golden.md).
- Public: Amicitia wiki "Persona 4 Golden/data.cpk/battle/SKILL.TBL" (44-byte active rows, element IDs).

## Container

All tables are members of `init_free.bin` (PAK v1) in `data.cpk` and every locale CPK; Golden mode reads
the selected locale's copy (`data_e.cpk` for English), which overrides `data.cpk`. The eight `.TBL` files
are packed-bank TBLs; `init/itemtbl.bin` is the two-section ITEMTBL ([Containers](containers.md)).

| File | Golden bytes | Blocks | Locale variants |
|---|---:|---:|---:|
| `battle/SKILL.TBL` | 22,048 | 2 | 1 |
| `battle/UNIT.TBL` | 42,096 | 3 | 1 |
| `battle/MSG.TBL` | 48,832 | 5 | 9 (text) |
| `battle/PERSONA.TBL` | 41,504 | 6 | 1 |
| `battle/ENCOUNT.TBL` | 71,088 | 5 | 2 |
| `battle/EFFECT.TBL` | 5,680 | 1 | 1 |
| `battle/MODEL.TBL` | 112,144 | 3 | 1 |
| `battle/AICALC.TBL` | 425,248 | 11 | 9 |
| `init/itemtbl.bin` | 235,552 | 2 sections | 9 |

## Records

Each row below is `PS2 rows x bytes -> Golden rows x bytes`. "Same layout" means the Golden row has the
PS2 row's size and field positions; Golden adds rows past PS2's domain.

### SKILL.TBL

| Block | Role | PS2 | Golden | Layout |
|---|---|---|---|---|
| 0 | element by skill ID | 576 x 2 | 624 x 2 | `u8 element` (0 Phys, 1 Fire, 2 Ice, 3 Elec, 4 Wind, 5 Almighty, 6 Light, 7 Dark, 8..15 ailment/status classes, 16 Healing, 17 Support, 18 Analysis/special, 0xFF passive or blank), `u8` second byte (meaning unspecified; copy) |
| 1 | active skill metadata by active ID | 440 x 40 | 472 x 44 | Golden row below |

Golden active row (44 bytes). PS2's 40-byte row is Golden bytes `0`, `4..6`, `8..43` in that order
(PS2 byte 0 = Golden 0; PS2 1..3 = Golden 4..6; PS2 4..39 = Golden 8..43). Golden bytes 1..3 widen the
flag byte at 0 to a word and byte 7 is padding; they are non-zero only on Golden-only skills.

| Golden offset | Type | Field |
|---|---|---|
| 0x00 | u32 | flags (low byte is PS2's byte 0) |
| 0x04 | u8 | use area bits (1 field, 2 battle) |
| 0x05 | u8 | power class (1 physical, 2 magic/default) |
| 0x06 | u8 | unspecified (copy) |
| 0x07 | u8 | padding |
| 0x08 | u16 | cost |
| 0x0A | u16 | cost is HP (0/1) |
| 0x0C | u8 | target kind (0 single, 1 group, 2 all) |
| 0x0D | u8 | side bits (1 ally, 2 enemy) |
| 0x0E..0x11 | u8 x 4 | unspecified (copy) |
| 0x12 | u8 | hit chance (percent) |
| 0x13, 0x14 | u8, u8 | minimum / maximum hits |
| 0x15 | u8 | effect kind |
| 0x16 | u16 | power |
| 0x18 | u16 | SP/HP recovery flags |
| 0x1A | u16 | recovery amount or percent |
| 0x1C | u8 | ailment mode (0 none, 1 inflicts, 2 cures, 3 other) |
| 0x1D | u8 | ailment chance (percent) |
| 0x1E | u16 | padding |
| 0x20 | u16 | ailment bits |
| 0x22 | u16 | position status bits (0x08 KO, 0x10 down) |
| 0x24 | u16 | buff/debuff bits |
| 0x26 | u16 | resist-change bits |
| 0x28 | u8 | special effect ID |
| 0x29 | u8 | chance against downed targets |
| 0x2A | u8 | critical chance |
| 0x2B | u8 | padding |

### UNIT.TBL

| Block | Role | PS2 | Golden | Layout |
|---|---|---|---|---|
| 0 | enemy metadata by enemy ID | 336 x 60 | 368 x 60 | same layout (HP, stats, rewards, drops at the `P4_ENEMY_ROW_SIZE` row; skill list as PS2) |
| 1 | enemy affinities by enemy ID | 336 x 32 | 368 x 32 | same layout: 16 x u16 affinity words by element |
| 2 | persona affinities by persona ID | 256 x 32 | 256 x 32 | same layout |

### MSG.TBL (text)

| Block | Role | PS2 | Golden | Layout |
|---|---|---|---|---|
| 0 | arcana names | 32 x 21 | 32 x 21 | NUL-padded `char[21]` |
| 1 | skill names by skill ID | 576 x 19 | 624 x 23 | NUL-padded `char[19]` -> `char[23]` |
| 2 | enemy names by enemy ID | 336 x 21 | 368 x 21 | NUL-padded `char[21]` |
| 3 | persona names by persona ID | 256 x 17 | 256 x 21 | NUL-padded `char[17]` -> `char[21]` |
| 4 | other battle strings | 15,141 B | 20,627 B | opaque: copied whole (not row-addressed by a spec yet) |

Text bytes are the locale's encoding; a name longer than the PS2 width must be refused or the width
relocated with its readers (table service), never truncated silently.

### PERSONA.TBL

| Block | Role | PS2 | Golden | Layout |
|---|---|---|---|---|
| 0 | persona metadata | 256 x 14 | 256 x 14 | same layout (`DatPersonaRecord`; u16 ID at +2, inheritance class at +0x0A) |
| 1 | growth and learnsets | 256 x 70 | 256 x 70 | same layout |
| 2 | party growth and learnsets | 24 x 622 | 24 x 622 | same layout; learn events name skill IDs (Golden IDs) |
| 3 | party EXP curves | 9 x 392 | 9 x 392 | 98 x u32 per curve |
| 4 | skill replacement pairs | 41 x 4 | 108 x 4 | `u16 original, u16 replacement` (Golden skill IDs); the list ends at the first pair with a zero half |
| 5 | persona replacement by ID | 256 x 4 | 256 x 4 | same layout (identical bytes in this build) |

### ENCOUNT.TBL

| Block | Role | PS2 | Golden | Layout |
|---|---|---|---|---|
| 0 | formations by formation ID | 912 x 24 | 944 x 24 | same layout: five u16 enemy IDs at +8 (0 = none) |
| 1 | selectors by field step | 160 x 10 | 180 x 10 | same layout: u16 weighted group at +0, u16 record group at +6, treasure count at +4 |
| 2 | unread block | 200 B | 200 B | identical bytes; copy |
| 3 | weighted groups | 56 x 248 | 68 x 252 | Golden row below |
| 4 | record (treasure) groups | 75 x 348 | 84 x 348 | same layout: 29 x 12-byte records, u16 weight, u16 item ID at +2, kind bit 0 of byte +7 |

Golden weighted group (252 bytes): a 12-byte rate header, then two lists (normal day, alternate day) of
30 `u16 formation, u16 weight` entries each (entries 0..19 normal, 20..24 special, 25..29 rare). The
header holds, for each category c (0 normal, 1 special, 2 rare), a `u8[3]` at offset `3c`: the rate on a
normal day, the rate on an alternate day, and a third Golden column; bytes 9..11 are padding. PS2's
248-byte row has an 8-byte header with a `u8[2]` at `2c` (the first two of each Golden triple) and
bytes 6..7 padding, then the same two 30-entry lists.

### EFFECT.TBL, MODEL.TBL, AICALC.TBL

| Table/block | Role | PS2 | Golden | Layout |
|---|---|---|---|---|
| EFFECT 0 | active skill presentation | 440 x 4 | 5,664 B | Golden is 472 x 12 [inferred from Golden's 472 actives]; field split unspecified: copy whole |
| MODEL 0 | party battle models | 11 x 332 | 11 x 382 | row widened; field split unspecified: copy whole |
| MODEL 1 | enemy battle parameters by enemy ID | 336 x 232 | 368 x 232 | same layout |
| MODEL 2 | genus-2 child models | 256 x 88 | 256 x 88 | same layout |
| AICALC 0..7 | AI constants | 504/52/60/20/40/12/84/6 B | same sizes | same sizes (blocks 1, 3, 4, 5, 7 identical bytes); copy |
| AICALC 8 | enemy AI rows by enemy ID | 336 x 164 | 368 x 164 | same layout |
| AICALC 9, 10 | AI programs | 58,430 / 164,907 B | 82,718 / 281,279 B | opaque: copied whole; compatibility of Golden AI programs with the PS2 AI interpreter is not established |

### ITEMTBL

| Section | Role | PS2 | Golden | Layout |
|---|---|---|---|---|
| 0 | item metadata by item ID | 1,536 x 68 | 2,560 x 68 | same layout (`P4ItemRow`: kind, flags, values, stat bonuses at +0x14, granted skill at +0x24, element at +0x2C, category mask at +0x30, price at +0x34, availability at +0x38, moon gates at +0x3C/+0x40) |
| 1 | item names by item ID | - | 2,560 x 24 | NUL-padded `char[24]` in the locale's encoding (9 locale variants). PS2 keeps its item names as strings in the executable image; milestone 2 matched those strings to these rows byte for byte |

1,182 of PS2's 1,536 section-0 rows are byte-identical in Golden.

## ID domains

| Domain | PS2 | Golden | Rule |
|---|---|---|---|
| skill (names, elements) | 0..575 | 0..623 | Actives 0..0x1B7 keep their IDs; Golden actives 0x1B8..0x1D7 are new; a PS2 passive `p` is Golden `p + 0x20` |
| active skill rows | 0..439 | 0..471 | as above |
| enemy | 0..335 | 0..367 | 336..367 are Golden-only |
| persona | 0..255 | 0..255 | same domain; Golden fills PS2 blank IDs and third-tier party rows |
| formation | 0..911 | 0..943 | Golden renumbered random formations (see Golden mechanics) |
| selector / weighted / record group | 160 / 56 / 75 | 180 / 68 / 84 | extra rows serve Golden-only floors |
| item | 0..1535 | 0..2559 | 1536..2559 are Golden-only |

Every carrier of these IDs in the PS2 code is at least u16; the table service's relocation (Gate A, full
Golden content) moves every bank of a widened domain together.

## Invariants and refusals

- Block counts and sizes exactly as in the Golden column; any other build is refused until measured.
- SKILL: block 0 is `624 x 2`, block 1 is `472 x 44`; MSG block 1 is `624 x 23`; their domains agree.
- Every ID field (formation enemies, learn events, replacement pairs, selector groups, record items,
  granted skills) is within its Golden domain; zero means none where noted.
- ENCOUNT weighted rows: entry formations within 0..943; a category whose rate is non-zero has at least
  one non-zero entry.
- PERSONA block 4 holds a pair with a zero half before its end.

## PS2 differences

- Widened domains: skill 576 -> 624 (passive shift +0x20), enemy 336 -> 368, item 1,536 -> 2,560,
  formations 912 -> 944, selectors 160 -> 180, weighted groups 56 -> 68, record groups 75 -> 84,
  replacement pairs 41 -> 108.
- Widened rows: SKILL 1 40 -> 44, MSG 1 19 -> 23, MSG 3 17 -> 21, ENCOUNT 3 248 -> 252, EFFECT 0 4 -> 12
  [inferred], MODEL 0 332 -> 382; ITEMTBL gains section 1 (item names, which PS2 keeps in its image).
- Same sizes, different values: PERSONA 0..3, UNIT 2, MODEL 2, AICALC 0/2/6.

## Test vectors

The containers' vectors cover these files' framing ([Containers](containers.md)); the conversion rules
are exercised by `tests/golden_source_test.c` (skill rows, passive shift, weighted-group repack) on
synthetic tables.

```golden-vector
name: skill-tbl-shape
container: tbl
hex: 04000000ff0001000000000000000000 08000000010000000000000000000000
expect: {"spans": ["ff000100", "0100000000000000"]}
```

## Status

Every block a converter reads has its size, row count and layout; Golden-only row semantics beyond the
PS2 fields (SKILL flags word, EFFECT 0, MODEL 0, MSG 4, AICALC 9/10) are copied whole
and need native support or further specs before Golden-only behaviour can use them.
