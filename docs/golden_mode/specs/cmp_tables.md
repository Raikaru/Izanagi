# cmpTable.bin (camp menu tables)

[Specs index](README.md) · [Containers](containers.md)

## Sources

- Measurement of build 10836033 with `tools/golden_mode/layouts.py` (container, rows, row sizes,
  inferred fields, locale variance).
- Public: Amicitia wiki "FTD" (the `.ctd` extension is the same container).

## Container

`init/cmpTable.bin` is a PAK v2 member of `init_free.bin`: 37 CTD tables (the FTD container,
[Containers](containers.md)) and one BMD message file (`camp_msg.bmd`, out of this wave). 23 tables are
locale-independent; 15 differ in every locale.

## Records

No camp table is read by a PS2 counterpart that the port has specified yet: the PS2 camp menus keep
their equivalents in the executable image (milestone 2 types those objects). Every table here is
catalogued and may only be copied whole until specified. `cmpPsGradTable.ctd` (256 rows, one per
persona ID) and the `cmpTypeID_*` lists (4-byte rows, item-type ID lists) are the likely first
specifications when the camp converters start.

| Table | Rows | Row bytes | Locale variants | Inferred fields (hex offset:type) |
|---|---:|---:|---:|---|
| `cmpBookHelp.ctd` | 3 | 64 | 9 | 0:char[64] |
| `cmpCalendarItem.ctd` | 39 | 64 | 9 | 0:char[64] |
| `cmpCalTable.ctd` | 74 | 12 | 1 | 0:u8 1:zero 2:u8 3:zero 4:u8 5:u8 6-7:zero 8:u8 9-B:zero |
| `cmpCmuDressTable.ctd` | 13 | 2 | 1 | 0:u8 1:zero |
| `cmpConfigHelp.ctd` | 92 | 64 | 9 | 0:u32 4:u32 8:u32 C:u32 10:u32 14:u32 18:u32 1C:u32 20:u32 24:u32 28:u32 2C:u32 30:u32 34:u32 38:u32 3C-3F:zero |
| `cmpConfigItem.ctd` | 92 | 64 | 9 | 0:char[64] |
| `cmpConfigKeyHelp.ctd` | 6 | 64 | 9 | 0:char[64] |
| `cmpConfigPCItem.ctd` | 46 | 64 | 9 | 0:char[64] |
| `cmpEquipDress.ctd` | 4 | 64 | 9 | 0:char[64] |
| `cmpEquipParam.ctd` | 5 | 64 | 9 | 0:char[64] |
| `cmpEquipSortTable.ctd` | 4 | 24 | 1 | 0:u32 4:char[20] |
| `cmpItemList.ctd` | 1 | 64 | 9 | 0:char[64] |
| `cmpItemSortTable.ctd` | 12 | 24 | 1 | 0:u32 4:char[20] |
| `cmpItemType.ctd` | 8 | 64 | 9 | 0:char[64] |
| `cmpPersonaParam.ctd` | 5 | 64 | 9 | 0:u8 1:u8 2-3F:zero |
| `cmpPsGradTable.ctd` | 256 | 10 | 1 | 0:u8 1:zero 2:u8 3:zero 4:u32 8:s16 |
| `cmpPsSkillAddTable.ctd` | 7 | 20 | 1 | 0:u16 2:u8 3:zero 4:u8 5:zero 6:u16 8:u16 A-13:zero |
| `cmpQuestItem.ctd` | 69 | 64 | 9 | 0:char[64] |
| `cmpQuestTable.ctd` | 69 | 16 | 1 | 0:u8 1:u8 2-3:zero 4:u8 5:u8 6-7:zero 8:u8 9:u8 A-B:zero C:u8 D-F:zero |
| `cmpSystemHelp.ctd` | 9 | 64 | 9 | 0:char[64] |
| `cmpSystemMenu.ctd` | 8 | 64 | 9 | 0:char[64] |
| `cmpTypeID_ACC.ctd` | 1 | 4 | 1 | 0:zero 1:u8 2:u16 |
| `cmpTypeID_ARMOR.ctd` | 1 | 4 | 1 | 0:zero 1:u8 2:u16 |
| `cmpTypeID_BEETLE.ctd` | 1 | 4 | 1 | 0:zero 1:u8 2:u16 |
| `cmpTypeID_BOOK.ctd` | 21 | 4 | 1 | 0:u16 2:u16 |
| `cmpTypeID_BUTTERFLY.ctd` | 1 | 4 | 1 | 0:zero 1:u8 2:u16 |
| `cmpTypeID_DRESS.ctd` | 11 | 4 | 1 | 0:u16 2:u16 |
| `cmpTypeID_EV_ITEM.ctd` | 2 | 4 | 1 | 0:zero 1:u8 2:u16 |
| `cmpTypeID_FISH.ctd` | 12 | 4 | 1 | 0:u16 2:u16 |
| `cmpTypeID_FOOD.ctd` | 2 | 4 | 1 | 0:zero 1:u8 2:u16 |
| `cmpTypeID_GEM.ctd` | 2 | 4 | 1 | 0:zero 1:u8 2:u16 |
| `cmpTypeID_ITEM.ctd` | 260 | 4 | 1 | 0:u16 2:u16 |
| `cmpTypeID_LEAF.ctd` | 1 | 4 | 1 | 0:zero 1:u8 2:u16 |
| `cmpTypeID_MATERIAL.ctd` | 1 | 4 | 1 | 0:zero 1:u8 2:u16 |
| `cmpTypeID_SKL_CARD.ctd` | 1 | 4 | 1 | 0:zero 1:u8 2:u16 |
| `cmpTypeID_VEGETABLE.ctd` | 15 | 4 | 1 | 0:u8 1:u8 2:u16 |
| `cmpTypeID_WEAPON.ctd` | 2 | 4 | 1 | 0:zero 1:u8 2:u16 |

## ID domains

`cmpPsGradTable.ctd` rows follow the persona domain (256). `cmpTypeID_*` values are assumed item IDs of
the Golden item domain (0..2559) [inferred: row size 4, bucket bound].

## Invariants and refusals

Exactly the rows and row sizes in the catalogue; any other build is refused.

## PS2 differences

PS2 has no `cmpTable.bin`; camp menu tables are in the PS2 executable image or its camp packs.

## Test vectors

```golden-vector
name: ctd-two-u32-rows
container: ftd
hex: 08000000020000000000000000000000 0100000002000000 00*8
expect: {"rows": 2, "payload": "0100000002000000"}
```

## Status

Catalogued only.
