# cmm.bin (Social Link tables)

[Specs index](README.md) · [Containers](containers.md)

## Sources

- Measurement of build 10836033 with `tools/golden_mode/layouts.py` (container, rows, row sizes,
  inferred fields, locale variance).
- [Golden mechanics](../../modding/golden.md): `cmmRankUpSkillTable.dat` entries are
  `u16 member, u16 rank, u32 skill`, the skill word's high half naming a skill the new one replaces.

## Container

`event/cmm.bin` is a PAK v2 member of `init.bin`: 35 DAT tables ([Containers](containers.md)) and one
BMD message file (`cmmHelp.bmd`, out of this wave). 23 tables are locale-independent; 12 differ in every
locale and 1 in five.

## Records

### Specified

**`cmmRankUpSkillTable.dat`** — `count` = 10 records of 48 bytes, each 6 entries of 8 bytes:

| Offset in entry | Type | Field |
|---|---|---|
| 0 | u16 | party member |
| 2 | u16 | Social Link rank |
| 4 | u16 | skill taught (Golden skill ID) |
| 6 | u16 | skill it replaces (0 = none) |

An entry with member 0 and skill 0 is empty. The DAT header's count is the record count here, not the
entry count.

### Catalogued

| Table | Rows | Row bytes | Locale variants | Inferred fields (hex offset:type) |
|---|---:|---:|---:|---|
| `cmmArbeit.dat` | 9 | 24 | 1 | 0:u8 1-3:zero 4:u8 5:u8 6-7:zero 8:u8 9:u8 A-B:zero C:u8 D:u8 E-F:zero 10:u8 11:u8 12-13:zero 14:u8 15:u8 16-17:zero |
| `cmmArcanaSPHelp.dat` | 32 | 64 | 9 | 0:char[64] |
| `cmmBookTable.dat` | 45 | 84 | 9 | 0:u8 1-3:zero 4:u8 5:u8 6-7:zero 8:u8 9:u8 A:u16 C:u32 10:u32 14:u32 18:u32 1C:u32 20:u32 24:u32 28:u32 2C:u32 30:u32 34:u32 38:u32 3C:u32 40:u32 44:u8 45:u8 46-47:zero 48:u8 49-4B:zero 4C:u16 4E-4F:zero 50:u16 52-53:zero |
| `cmmCall.dat` | 43 | 8 | 1 | 0:u32 4:u8 5:zero 6:u8 7:zero |
| `cmmClubName.dat` | 5 | 64 | 9 | 0:char[64] |
| `cmmCommunityMaxWork.dat` | 29 | 66 | 9 | 0:u32 4:u32 8:u32 C:u32 10:u32 14:u32 18:u8 19-3F:zero 40:u8 41:zero |
| `cmmCommunityStatus.dat` | 1 | 36 | 1 | 0:f32 4:u16 6-7:zero 8:f32 C:u8 D-F:zero 10:f32 14:u8 15-17:zero 18:f32 1C:u8 1D-1F:zero 20:f32 |
| `cmmEventBGM.dat` | 280 | 12 | 1 | 0:u16 2:u16 4-7:zero 8:u8 9-B:zero |
| `cmmEventNo.dat` | 1063 | 16 | 1 | 0:u8 1:zero 2:u8 3:u8 4:u16 6:u16 8:u8 9-A:zero B:u8 C-F:zero |
| `cmmEventWEATHER.dat` | 312 | 12 | 1 | 0:u16 2-3:zero 4:u16 6:u16 8:u32 |
| `cmmFickleCounter.dat` | 49 | 8 | 1 | 0:u8 1:zero 2:u8 3-7:zero |
| `cmmFishCaptureFishBaitRate.dat` | 2 | 48 | 1 | 0:f32 4:f32 8:f32 C:f32 10:f32 14:f32 18-2F:zero |
| `cmmFishCaptureRate.dat` | 8 | 20 | 1 | 0:u16 2-3:zero 4:f32 8:u16 A-B:zero C:f32 10:u8 11:zero 12:u8 13:zero |
| `cmmFishingGauge.dat` | 12 | 12 | 1 | 0:u32 4:u8 5:u8 6:u8 7-B:zero |
| `cmmFishingOthersTable.dat` | 6 | 4 | 1 | 0:u8 1-3:zero |
| `cmmFishingTable.dat` | 12 | 72 | 1 | 0:u8 1-3:zero 4:u8 5:zero 6:u8 7:zero 8:u8 9:zero A:u8 B:zero C:f32 10:u8 11:zero 12:u8 13:zero 14:u8 15-17:zero 18:u16 1A:u8 1B:zero 1C:u8 1D:zero 1E:u16 20:u8 21-23:zero 24:f32 28:f32 2C:f32 30:u8 31-33:zero 34:u8 35:zero 36:u8 37:zero 38:f32 3C:f32 40:f32 44:f32 |
| `cmmFishTrade.dat` | 12 | 96 | 1 | 0:u32 4:u32 8:u32 C:u32 10:u32 14:u32 18:u32 1C:u32 20:u32 24:u32 28:u32 2C:u32 30:u16 32:u16 34:u8 35-37:zero 38:u16 3A-3B:zero 3C:u16 3E:u16 40:u8 41-47:zero 48:u16 4A:u16 4C:u8 4D-53:zero 54:u16 56:u16 58:u8 59-5F:zero |
| `cmmFishWeaponRate.dat` | 7 | 12 | 1 | 0:u8 1:u8 2:u8 3:u8 4:f32 8:u16 A-B:zero |
| `cmmFixString.dat` | 19 | 32 | 9 | 0:char[32] |
| `cmmFormat.dat` | 35 | 100 | 1 | 0:u8 1-3:zero 4:u8 5-7:zero 8:u8 9:u8 A-B:zero C:u16 E:u8 F-11:zero 12:u8 13:zero 14:u8 15:zero 16:u8 17:zero 18:u8 19:zero 1A:u8 1B:zero 1C:u8 1D:zero 1E:u8 1F:zero 20:u8 21:zero 22:u8 23-27:zero 28:u8 29-2B:zero 2C:u8 2D-2F:zero 30:u8 31-33:zero 34:u8 35-37:zero 38:u8 39-3B:zero 3C:u8 3D-3F:zero 40:u8 41-43:zero 44:u8 45-47:zero 48:u8 49-4B:zero 4C:u8 4D-4F:zero 50:u8 51-53:zero 54:u8 55-57:zero 58:u8 59-63:zero |
| `cmmHelp.dat` | 35 | 24 | 1 | 0:u16 2:u16 4:u16 6:u16 8:u16 A:u16 C:u16 E:u16 10:u16 12:u16 14:u16 16:u16 |
| `cmmHelpMember.dat` | 34 | 20 | 1 | 0:u16 2:u16 4:u16 6:u16 8:u16 A:u16 C:u16 E:u16 10:u16 12:u16 |
| `cmmMailOrder.dat` | 33 | 36 | 1 | 0:u32 4:u16 6:u8 7:zero 8:u8 9:u8 A:u8 B:zero C:u32 10:u8 11:zero 12:u8 13:zero 14:u16 16:u8 17:zero 18:u16 1A:u8 1B:zero 1C:u8 1D:u8 1E-1F:zero 20:u8 21:zero 22:u8 23:zero |
| `cmmMailOrder_Name.dat` | 45 | 64 | 5 | 0:char[64] |
| `cmmMailOrder_Text.dat` | 67 | 64 | 9 | 0:char[64] |
| `cmmMemberName.dat` | 35 | 64 | 9 | 0:char[64] |
| `cmmName.dat` | 35 | 64 | 9 | 0:char[64] |
| `cmmNotInvite.dat` | 3 | 8 | 1 | 0-3:zero 4:u8 5:zero 6:u8 7:u8 |
| `cmmNotReservation.dat` | 40 | 2 | 1 | 0:u8 1:u8 |
| `cmmPC_PARAM_EXP.dat` | 5 | 8 | 1 | 0:u8 1:zero 2:u8 3:zero 4:u8 5:zero 6:u8 7:zero |
| `cmmPC_PARAM_GRADE_POINT.dat` | 5 | 4 | 1 | 0:u8 1:zero 2:u8 3:zero |
| `cmmPC_PARAM_Help.dat` | 5 | 70 | 9 | 0:u32 4:u32 8-B:zero C:u32 10:u32 14:u32 18-1B:zero 1C:u32 20:u32 24:u8 25:u8 26-27:zero 28:u32 2C:u32 30:u32 34-37:zero 38:char[14] |
| `cmmPC_PARAM_name.dat` | 5 | 14 | 9 | 0:char[14] |
| `cmmPlace.dat` | 16 | 64 | 9 | 0:u32 4:u32 8:u32 C:u8 D-3F:zero |

## ID domains

Skill IDs follow the Golden skill domain ([Battle tables](battle_tables.md#id-domains)). Social Link
(community) IDs and party member IDs are small integers whose domains are not yet specified.

## Invariants and refusals

- Exactly the rows and row sizes in the catalogue; any other build is refused.
- `cmmRankUpSkillTable.dat` payload is a multiple of 48 bytes; every non-empty entry names a skill in the
  Golden skill domain.

## PS2 differences

PS2 keeps its Social Link tables in the executable image and event packs; Golden's `cmm.bin` has no
byte-identical PS2 file. Golden-only Social Links (Marie, Adachi) and the rank-up skill teaching are
Golden content (plan GATE A).

## Test vectors

```golden-vector
name: dat-rank-up-shape
container: dat
hex: 30000000010000000100020013000000 01000400140000000000000000000000 00*32
expect: {"rows": 1, "payload": "010002001300000001000400140000000000000000000000000000000000000000000000000000000000000000000000"}
```

## Status

The rank-up skill table is specified; the rest is catalogued only.
