# L1 archive candidates and no-lead literals

## Scope and status

**Structural source specification; not a runtime binding or Golden-balance closure receipt.**
The companion [decision supplement](l1-image-literals-archive.jsonl) covers **456 objects / 69,527 B**,
100% of this reconciled assignment. Every address has a decision, reason, retained PS2 span,
PS2 type evidence/consumer leads, Golden candidate or explicit null, layout status and cheapest proof path.
No Golden payload or instruction listing is included.

| Reconciled bucket | Objects | PS2 span bytes |
|---|---:|---:|
| Archive variant | 123 | 63,003 |
| Archive partial (not a variant source) | 6 | 352 |
| No surviving candidate | 327 | 6,172 |
| Total | 456 | 69,527 |

The roadmap's historical **128 archive variants** is not the canonical object selection. The M2
`golden_check == true` inventory has 1,043 objects / 249,428 B. GoldenRETables4 owns the other
587 executable-candidate objects, including **two partial executable leads / 2,436 B**. Together,
6,172 + 352 + 2,436 = **8,960 B** in the no-variant-lead bucket. No object is dropped to make the
historical count agree. `no lead` and `no variant lead` are different categories.

| Decision class | Objects | Bytes |
|---|---:|---:|
| PROVEN | 149 | 23,795 |
| PROVEN-code | 24 | 6,061 |
| PROVEN-layout (Main-authorized local proof) | 3 | 56 |
| D10a INFERRED-from-BBV with check | 0 | 0 |
| D10b published facts | 0 | 0 |
| D10c positively no counterpart | 129 | 33,554 |
| OPEN-retain | 151 | 6,061 |

**Merged decision census:**149 data mappings are PROVEN,24 objects have
PROVEN-code constants and three objects are PROVEN-layout;151 rows remain OPEN.
The129 D10c rows have positive source-owned replacement/absence contracts,
including22 directory representations; they are not dead-asset permissions.
Source mappings and replacement-route obligations remain pending-seam;
GoldenM6b/M10 must implement and qualify their contracts before runtime closure.
No BBV or published-fact promotion is claimed. Global totals include peer work;
source ownership and task transfers do not duplicate the canonical inventory.

## Table contract and integration

This JSONL is a **supplement keyed by PS2 address**, not a second object inventory. The primary
`l1-image-literals.jsonl` is now owned by GoldenRE-L1c (continuing GoldenRE-L1a2/GoldenRETables4). Apply a supplement record as the enriched decision
for that address; never concatenate duplicate addresses into a 1,499-object supplier ledger.
All 456 records keep the primary schema (`address`, `size`, `symbol`, `decision`, `reason`, `evidence`,
`golden`, `candidate`, `layout`, `proof_path`, `consumers`, `retention`). Additions:

- `scope`: archive-variant or no-variant-lead; partial archive candidates belong to the latter.
- `structural_evidence`: observed member loader/typed reader where known, explicitly separate from
  `object_correspondence_proven` and `layout_delta_proven`: claims for149 PROVEN,24 PROVEN-code and three PROVEN-layout rows; D10c has separate positive replacement proof.
- `candidate.census`: observed member framing/size/hash/signature. This is source identification,
  **not** evidence that the member supplies the PS2 object.
- `candidate.offset_basis`: offset in the decoded leaf member, not a CPK offset. `probe_size` is the
  PS2 heuristic window size, not an established Golden object extent.
- Optional `candidate.ftd`: header-derived payload extent/count/stride, excluding padding;
  `inside_declared_payload` checks that exact extent, unlike whole-member bounds.
- `unnamed@ADDRESS` is an explicit unnamed inventory object identifier, not a discovered PS2 symbol.
  Existing symbols and consumer names are preserved; consumers are M2 static leads, not a complete
  computed-address reference proof.

The151 OPEN rows have no approved Golden source. The176 source-qualified
rows supply data mappings or embedded-code constants, logical shape and
`required_seams`;129 D10c rows instead require
their explicit replacement routes. Old M2 candidates remain historical
provenance. No decision permits copying an unqualified heuristic byte window.

## Evidence and boundaries

- M2 `typed/typed.jsonl` SHA-256:
  `19a89b5d028da3fac821ef01d61f2f1fda81510cf392714fe89024a30f0c60d1`.
  Canonical selection is `golden_check == true`, reconciled by `tools/golden_mode/l1-inventory.json`.
- M7c 2026-11-12 `inventory.jsonl`: all **54 distinct candidate archive members** in this assignment
  exist; each record carries its full nested member path, size and SHA-256. Existence/size/hash and
  M2 equal-byte/vote counts are metadata only. Container interpretation follows
  [containers](../containers.md), [battle tables](../battle_tables.md).
- Switch 1.2 ordinary static reader witnesses below are corroboration of named source structure,
  not proof that Steam has identical code or that a PS2 span is a matching field. They were read
  from the supplied private static-analysis notes. No protector `.arch` code was decoded, and no
  game was executed, unpacked or memory-dumped for this work.
- The M2 matcher misses small/changed/inlined constants and uses heuristic boundaries. An unidentified
  consumer or empty symbol list is retained explicitly, never treated as unused or safe to delete.

### Nine PROVEN facility source mappings

The nine FTD sources have independently measured Steam headers and member hashes recorded per object.
All nine corresponding PS2 and Golden typed readers are now recorded. Their on-disk member size
includes padding; a previous measurement called all bytes after +16
`payload_size`, but **only the header's first u32 is live payload length**. This supplement deliberately
uses that header length, not the padded remainder.

| Golden member in `data.cpk::init_free.bin::init/fclTable.bin` | Payload layout | Switch reader | Required implementation seam |
|---|---|---|---|
| `FlameSkillTbl.ftd`, `WindSkillTbl.ftd`, `FreezeSkillTbl.ftd`, `ElectricSkillTbl.ftd`, `RecoverySkillTbl.ftd`, `SupportSkillTbl.ftd` | +16, 10 x raw halfword, 20 B; 28 B padding | `0x71001737e0`, category1..6 filename pointers at `0x7100764048..4070` | Direct003042f0 AND inlined0030c3c0 reads;20-byte prefix only, preserve PS2 tail; Golden producer uses `(level-1)/10`, not PS2 threshold loop |
| `CmbCmmCheckTbl.ftd` | +16, 12 x 11 B, 132 B;12 B padding | `0x71001735c0` |0030b7b0 month/score case must map selector through5 x u32 `fclCmmRankMsgNo.ftd`, not fixed PS2+2 |
| `WeatherCommuBonus.ftd` | +16,11 x f32,44 B;4 B padding | `0x71001737a0`; float multiplier caller `0x710015b918` |00311930 optional EXP multiplier read;44-byte prefix, retain4-byte PS2 tail |
| `fclComuLvExp.ftd` | +16,11 x f32,44 B;4 B padding |174430 ->170300 -> global9fb578 -> reader `0x710016d0f0` |00311930 whole/fractional scale reads;44-byte prefix, retain4-byte tail; Golden EXP arithmetic separately qualified |

`0x7100174430` loads `init/fclTable.bin` and `init/datMsg.pak`, waits for both, resolves named
members through `0x7100397040`, and registers FTD payload/count descriptor pairs. The same loader
registers named help BMD banks through `0x710018b250`. Those help-member candidates therefore have
an observed Golden loader envelope, but **no PS2 address-to-message-ID/entry mapping** yet.
The nine FTD records plus nine help-BMD records carry these stronger structural witnesses.

### A concrete false-correspondence warning

PS2 `D_0063ED80`, 48 B, has an M2 candidate in `linex2.dat`. The actual decomp reader at
`src/Kernel/mc.c:1914-1953` (decomp pin `18016afe6`) copies six 8-byte pairs into a 12-float local
array, translates two groups of three 2D vertices and draws upper/lower card-status triangles.
That establishes the **PS2 geometry role**, not a line-archive source. The cheapest next proof is the
matching Golden card-status renderer. Shared floats or zeros in `linex2.dat` do not establish it.
This object stays OPEN-retain, not D10c.

## Family-specific proof obligations

Each record specializes these obligations with its address, consumers and candidate path/offset:

- **BMD:** identify PS2 literal role and stable message ID/label; trace Golden caller selecting the
  same message and decode entry boundaries/relocations/encoding. A byte offset is not a message ID.
  A retail display can cross-check text meaning, not prove pointer layout.
- **AICALC:** establish which packed-bank block and complete BF procedure contains the candidate;
  correlate opcode/operand/label role and caller, using `ai-commands.md` and `ai-script-uses.md`.
  A byte window in a VM program is not a balance-table projection.
- **ITEMTBL:** parse the two sections, then prove item ID and name/record boundaries. English
  `data_e.cpk` overrides base data; a base-locale lead is not an approved English string source.
- **FBN/CMR/PLG and generic DAT/FTD/CTD:** trace typed Golden member readers and PS2 owner role.
  Container existence or advertised rows alone does not prove the object. Exclude container padding.
- **No lead:** establish operation, operand meaning, width and live span from the listed PS2 owner,
  then follow the equivalent ordinary `.shared`/Switch caller. Constants can be compiled inline;
  numeric search alone cannot prove source correspondence or absence.

## Candidate-family census

These are heuristic destinations, not approved suppliers. All counts are PS2 inventory spans.

| Candidate member | Objects | Bytes |
|---|---:|---:|
| `No located source` | 327 | 6,172 |
| `data.cpk::battle/result/shuffle.arc::btlShuffleMsg.bmd` | 1 | 223 |
| `data.cpk::facility/cmbroot.arc::msg_combine.bmd` | 1 | 24 |
| `data.cpk::facility/market.arc::market.plg` | 1 | 24 |
| `data.cpk::facility/weapon.arc::weaponx2.plg` | 1 | 160 |
| `data.cpk::field/pack/fd001_001.arc::f001_001.CMR` | 7 | 368 |
| `data.cpk::field/pack/fd006_006.arc::f006_006_009.FBN` | 1 | 24 |
| `data.cpk::field/pack/fd006_006.arc::f006_006_019.FBN` | 1 | 16 |
| `data.cpk::field/pack/fd008_001.arc::f008_001_008.FBN` | 1 | 24 |
| `data.cpk::field/pack/fd008_002.arc::f008_002_004.FBN` | 1 | 16 |
| `data.cpk::field/pack/fd011_001.arc::f011_001_121.FBN` | 1 | 24 |
| `data.cpk::field/pack/fd012_004.arc::f012_004.CMR` | 1 | 40 |
| `data.cpk::field/pack/fd016_005.arc::f016_005_001.FBN` | 1 | 32 |
| `data.cpk::field/pack/fd028_001.arc::f028_001_001.FBN` | 2 | 56 |
| `data.cpk::field/pack/fd028_002.arc::f028_002_001.FBN` | 1 | 32 |
| `data.cpk::init.bin::event/cmm.bin::cmmFishCaptureFishBaitRate.dat` | 1 | 64 |
| `data.cpk::init.bin::event/cmm.bin::cmmHelp.bmd` | 1 | 224 |
| `data.cpk::init.bin::init/datMsg.pak::datArmorHelp.bmd` | 3 | 2,436 |
| `data.cpk::init.bin::init/datMsg.pak::datEventItemHelp.bmd` | 2 | 1,376 |
| `data.cpk::init.bin::init/datMsg.pak::datMaterialHelp.bmd` | 1 | 731 |
| `data.cpk::init.bin::init/datMsg.pak::datSkillHelp.bmd` | 2 | 1,744 |
| `data.cpk::init_free.bin::battle/AICALC.TBL` | 16 | 26,567 |
| `data.cpk::init_free.bin::init/fclTable.bin::CmbCmmCheckTbl.ftd` | 1 | 144 |
| `data.cpk::init_free.bin::init/fclTable.bin::ElectricSkillTbl.ftd` | 1 | 32 |
| `data.cpk::init_free.bin::init/fclTable.bin::FlameSkillTbl.ftd` | 1 | 32 |
| `data.cpk::init_free.bin::init/fclTable.bin::FreezeSkillTbl.ftd` | 1 | 32 |
| `data.cpk::init_free.bin::init/fclTable.bin::RecoverySkillTbl.ftd` | 1 | 32 |
| `data.cpk::init_free.bin::init/fclTable.bin::SupportSkillTbl.ftd` | 1 | 24 |
| `data.cpk::init_free.bin::init/fclTable.bin::WeatherBonusTbl.ftd` | 6 | 5,269 |
| `data.cpk::init_free.bin::init/fclTable.bin::WeatherCommuBonus.ftd` | 1 | 48 |
| `data.cpk::init_free.bin::init/fclTable.bin::WindSkillTbl.ftd` | 1 | 32 |
| `data.cpk::init_free.bin::init/fclTable.bin::fclCombineTable_Normal.ftd` | 1 | 43 |
| `data.cpk::init_free.bin::init/fclTable.bin::fclComuLvExp.ftd` | 1 | 48 |
| `data.cpk::init_free.bin::init/fclTable.bin::fclWeaponTalk.ftd` | 1 | 448 |
| `data.cpk::init_free.bin::init/fclTable.bin::fclWeaponTalkMat.ftd` | 1 | 48 |
| `data.cpk::init_free.bin::init/line.arc::linex2.dat` | 7 | 6,513 |
| `data.cpk::model/pack/bc008_c1.pac::bc008_c1.se::bc008_c1.bin` | 2 | 2,047 |
| `data.cpk::title/quiz.arc::msg_quiz.bmd` | 2 | 2,304 |
| `data.cpk::title/quiz.arc::msg_quiz_answer.bmd` | 3 | 1,208 |
| `data_e.cpk::facility/cmbroot.arc::msg_combine.bmd` | 5 | 152 |
| `data_e.cpk::facility/weapon.arc::msg_weapon.bmd` | 10 | 8,650 |
| `data_e.cpk::init.bin::event/cmm.bin::cmmBookTable.dat` | 5 | 136 |
| `data_e.cpk::init.bin::event/cmm.bin::cmmPC_PARAM_name.dat` | 3 | 48 |
| `data_e.cpk::init.bin::init/datMsg.pak::datItemHelp.bmd` | 1 | 308 |
| `data_e.cpk::init_free.bin::battle/MSG.TBL` | 2 | 32 |
| `data_e.cpk::init_free.bin::init/cmpTable.bin::camp_msg.bmd` | 1 | 848 |
| `data_e.cpk::init_free.bin::init/cmpTable.bin::cmpConfigHelp.ctd` | 1 | 48 |
| `data_e.cpk::init_free.bin::init/cmpTable.bin::cmpQuestItem.ctd` | 1 | 48 |
| `data_e.cpk::init_free.bin::init/cmpTable.bin::cmpSystemHelp.ctd` | 1 | 40 |
| `data_e.cpk::init_free.bin::init/fclTable.bin::fclHelpTable_COMBINE.ftd` | 1 | 24 |
| `data_e.cpk::init_free.bin::init/fclTable.bin::fclHelpTable_COMBINE_ROOT.ftd` | 1 | 24 |
| `data_e.cpk::init_free.bin::init/fclTable.bin::fclLogFormatText.ftd` | 1 | 24 |
| `data_e.cpk::init_free.bin::init/fclTable.bin::pWeatherBonus.ftd` | 1 | 24 |
| `data_e.cpk::init_free.bin::init/fclTable.bin::pWeatherCondition.ftd` | 1 | 16 |
| `data_e.cpk::init_free.bin::init/itemtbl.bin` | 15 | 424 |

## Per-object index

The machine-readable record is authoritative for the full candidate offset, member hash, unproven
layout delta, decision, required seams and exact proof path. Seventeen entries
are PROVEN, one PROVEN-layout and twelve D10c; 426 remain OPEN-retain.
`none` below means no original M2 candidate, not no proven source. Known symbols
are preserved in the JSONL.

| PS2 address | Size B | Kind | Candidate offset (leaf-relative) |
|---|---:|---|---|
| `0x0056d430` | 224 | struct | `0x29c` |
| `0x005702bc` | 308 | ascii-text | `0x450` |
| `0x00579370` | 731 | struct-array | `0x3c` |
| `0x00584341` | 223 | ascii-text | `0x3a5` |
| `0x0058a760` | 224 | struct | `0x24` |
| `0x0059524f` | 515 | struct-array | `0x26a63` |
| `0x00595452` | 910 | ascii-text | `0x26b76` |
| `0x005957e0` | 1184 | ascii-text | `0x13324` |
| `0x005968d0` | 512 | struct | `0x39904` |
| `0x005a0023` | 70 | struct | `0x31697` |
| `0x005a0069` | 41 | ascii-text | `0x3167d` |
| `0x005a02bc` | 315 | struct-array | `0x315d0` |
| `0x005a4450` | 1424 | struct-array | `0xd84` |
| `0x005a49e0` | 3960 | struct-array | `0x24b44` |
| `0x005bbcd0` | 320 | struct | `0x9ec` |
| `0x005bc5d0` | 272 | ascii-text | `0x67c` |
| `0x005c0c20` | 1040 | struct-array | `0x34e14` |
| `0x005c1030` | 12574 | struct-array | `0x14a74` |
| `0x005c414e` | 514 | struct-array | `0x2b222` |
| `0x005c454c` | 1268 | struct-array | `0x2f6b0` |
| `0x005c554b` | 3589 | ascii-text | `0x6c8` |
| `0x005c67b0` | 896 | struct-array | `0x54c` |
| `0x005dc974` | 84 | int32-array | none |
| `0x005dcc64` | 4 | struct | none |
| `0x005dcc68` | 4 | struct | none |
| `0x005df308` | 16 | ascii-text | `0xaf9` |
| `0x005dfaa0` | 16 | ascii-text | `0x2d9ea` |
| `0x005dfe30` | 32 | ascii-text | `0x2dfb8` |
| `0x005e0138` | 24 | ascii-text | `0x2e439` |
| `0x005e0c90` | 16 | ascii-text | `0x30a90` |
| `0x005e0d50` | 32 | ascii-text | `0x29ce` |
| `0x005e0d70` | 32 | ascii-text | `0x29ce` |
| `0x005e0e60` | 32 | ascii-text | `0x31858` |
| `0x005e0eb0` | 32 | ascii-text | `0x318d0` |
| `0x005e0ee0` | 32 | ascii-text | `0x31ae0` |
| `0x005e0f80` | 32 | ascii-text | `0x31bb8` |
| `0x005e1018` | 24 | ascii-text | `0x110` |
| `0x005e1060` | 32 | ascii-text | `0x2b4` |
| `0x005e1080` | 32 | ascii-text | `0x308` |
| `0x005e10a0` | 24 | ascii-text | `0x35c` |
| `0x005e10f8` | 24 | ascii-text | `0x500` |
| `0x005e1180` | 32 | ascii-text | `0x32050` |
| `0x005e1380` | 32 | ascii-text | `0x32338` |
| `0x005e15f8` | 16 | ascii-text | `0x304c1` |
| `0x005e17b0` | 32 | ascii-text | `0x32938` |
| `0x005e1890` | 32 | ascii-text | `0x32a70` |
| `0x005e1d80` | 32 | ascii-text | `0x331a8` |
| `0x005e1f00` | 32 | ascii-text | `0x333b8` |
| `0x005e45c0` | 32 | ascii-text | none |
| `0x005e50d0` | 160 | struct | `0x2998` |
| `0x005e5220` | 16 | struct | none |
| `0x005e5548` | 24 | ascii-text | none |
| `0x005e55b0` | 8 | scalar | none |
| `0x005e55c0` | 8 | scalar | none |
| `0x005e55d8` | 8 | scalar | none |
| `0x005e55e0` | 48 | int32-array | `0xd8e0` |
| `0x005e56f0` | 16 | struct-array | none |
| `0x005e5700` | 8 | struct-array | none |
| `0x005e5708` | 8 | scalar | none |
| `0x005e5740` | 16 | struct | none |
| `0x005e5750` | 16 | struct | none |
| `0x005e5810` | 32 | ascii-text | none |
| `0x005e5850` | 32 | ascii-text | none |
| `0x005e84a0` | 1152 | struct | `0x4da` |
| `0x005e8920` | 1152 | struct-array | `0x4da` |
| `0x005ed9f0` | 16 | ascii-text | none |
| `0x005ee320` | 1152 | struct-array | `0x4c6` |
| `0x005ee7a0` | 1152 | struct-array | `0x236` |
| `0x005eec20` | 1152 | struct-array | `0x236` |
| `0x005ef870` | 32 | ascii-text | none |
| `0x005efa10` | 16 | f32-array | none |
| `0x005efa20` | 16 | f32-array | none |
| `0x005efa30` | 16 | f32-array | none |
| `0x005efc80` | 16 | ascii-text | none |
| `0x005efcb0` | 16 | ascii-text | none |
| `0x005efce0` | 32 | ascii-text | none |
| `0x005efd00` | 32 | ascii-text | none |
| `0x005efd20` | 32 | ascii-text | none |
| `0x005efd40` | 32 | ascii-text | none |
| `0x005efd60` | 32 | ascii-text | none |
| `0x005efd80` | 24 | ascii-text | none |
| `0x005efda8` | 16 | ascii-text | none |
| `0x005efdb8` | 16 | ascii-text | none |
| `0x005efdd8` | 16 | ascii-text | none |
| `0x005efde8` | 24 | ascii-text | none |
| `0x005efe10` | 24 | ascii-text | none |
| `0x005efe28` | 8 | int64-array | none |
| `0x005efe38` | 8 | int64-array | none |
| `0x005f0590` | 1 | struct | none |
| `0x005f05b8` | 22 | struct | none |
| `0x005f05e8` | 24 | ascii-text | none |
| `0x005f0670` | 32 | ascii-text | none |
| `0x005f0790` | 16 | ascii-text | none |
| `0x005f07c0` | 16 | ascii-text | none |
| `0x005f15f8` | 24 | ascii-text | `0xfdc` |
| `0x005f18b0` | 8 | int64-array | none |
| `0x005f18b8` | 8 | f32-array | none |
| `0x005f18f8` | 4 | struct | none |
| `0x005f18fc` | 4 | struct | none |
| `0x005f1908` | 4 | struct | none |
| `0x005f190c` | 4 | struct | none |
| `0x005f1e20` | 32 | f32-array | none |
| `0x005f1e40` | 32 | ascii-text | none |
| `0x005f1e60` | 32 | ascii-text | none |
| `0x005f1e80` | 32 | ascii-text | none |
| `0x005f1ea0` | 32 | ascii-text | none |
| `0x005f1ec0` | 32 | ascii-text | none |
| `0x005f1ee0` | 24 | ascii-text | none |
| `0x005f1ef8` | 24 | ascii-text | none |
| `0x005f1fd0` | 32 | ascii-text | none |
| `0x005f2010` | 32 | ascii-text | none |
| `0x005f2030` | 32 | ascii-text | none |
| `0x005f2050` | 32 | ascii-text | none |
| `0x005f2070` | 32 | ascii-text | none |
| `0x005f2090` | 32 | ascii-text | none |
| `0x005f20b0` | 4 | f32-array | none |
| `0x005f20b4` | 4 | f32-array | none |
| `0x005f20b8` | 4 | f32-array | none |
| `0x005f20bc` | 4 | f32-array | none |
| `0x005f20c0` | 4 | f32-array | none |
| `0x005f20c4` | 108 | f32-array | none |
| `0x005f2130` | 4 | f32-array | none |
| `0x005f2134` | 4 | f32-array | none |
| `0x005f2138` | 4 | f32-array | none |
| `0x005f213c` | 68 | f32-array | none |
| `0x005f2190` | 4 | f32-array | none |
| `0x005f2194` | 4 | f32-array | none |
| `0x005f2198` | 4 | f32-array | none |
| `0x005f219c` | 4 | f32-array | none |
| `0x005f5360` | 16 | ascii-text | none |
| `0x005f5370` | 16 | ascii-text | none |
| `0x005f55b0` | 320 | struct-array | none |
| `0x005f56f0` | 16 | ascii-text | none |
| `0x005f5718` | 24 | ascii-text | none |
| `0x005f6168` | 16 | ascii-text | none |
| `0x005f6178` | 24 | ascii-text | none |
| `0x005f6190` | 32 | ascii-text | none |
| `0x005f61d0` | 40 | ascii-text | `0x20a` |
| `0x005f6210` | 32 | ascii-text | none |
| `0x005f6230` | 32 | ascii-text | none |
| `0x005f6250` | 32 | ascii-text | none |
| `0x005f6270` | 48 | ascii-text | `0x248` |
| `0x005f62a0` | 16 | ascii-text | none |
| `0x005f62b0` | 16 | ascii-text | none |
| `0x005f62c0` | 16 | ascii-text | none |
| `0x005f62d0` | 64 | ascii-text | `0x248` |
| `0x005f6310` | 64 | ascii-text | `0x248` |
| `0x005f6350` | 32 | ascii-text | none |
| `0x005f6370` | 64 | ascii-text | `0x24a` |
| `0x005f63b0` | 48 | ascii-text | `0x24a` |
| `0x005f63e0` | 16 | ascii-text | none |
| `0x005f63f0` | 64 | ascii-text | `0x248` |
| `0x005f6430` | 32 | ascii-text | none |
| `0x005f6450` | 32 | ascii-text | none |
| `0x005f6470` | 32 | ascii-text | none |
| `0x005f6490` | 24 | ascii-text | none |
| `0x005f64a8` | 24 | ascii-text | none |
| `0x005f6e20` | 456 | struct-array | none |
| `0x006095e0` | 16 | ascii-text | none |
| `0x0060a0d0` | 16 | struct | none |
| `0x0061646f` | 754 | struct-array | `0x58f` |
| `0x00617461` | 1293 | struct-array | `0x801` |
| `0x00626530` | 16 | ascii-text | `0x1978` |
| `0x00635938` | 24 | ascii-text | none |
| `0x00636480` | 80 | struct | none |
| `0x006364d0` | 32 | f32-array | none |
| `0x006367d8` | 16 | ascii-text | `0x16` |
| `0x006367e8` | 16 | ascii-text | `0x24` |
| `0x006367f8` | 16 | ascii-text | `0x32` |
| `0x00636808` | 24 | ascii-text | `0x730` |
| `0x00636838` | 24 | ascii-text | none |
| `0x00637248` | 24 | ascii-text | `0x70c` |
| `0x00637320` | 40 | sjis-text | none |
| `0x0063b090` | 16 | ascii-text | none |
| `0x0063b0d0` | 32 | ascii-text | none |
| `0x0063b580` | 32 | ascii-text | `0xbdc` |
| `0x0063bab0` | 56 | int8-array | `0x3b4` |
| `0x0063be10` | 16 | ascii-text | none |
| `0x0063c700` | 16 | struct | none |
| `0x0063c710` | 16 | struct | none |
| `0x0063c790` | 16 | struct | none |
| `0x0063ed80` | 48 | int32-array | `0x5ff0` |
| `0x0063ee50` | 16 | ascii-text | none |
| `0x0063f2b0` | 4 | int32-array | none |
| `0x0063f2b4` | 4 | scalar | none |
| `0x0063f2b8` | 8 | scalar | none |
| `0x0063f2c0` | 4 | int32-array | none |
| `0x0063f2c4` | 4 | scalar | none |
| `0x0063f2cc` | 36 | int32-array | none |
| `0x0063f2f0` | 32 | int32-array | none |
| `0x0063f310` | 48 | int32-array | none |
| `0x0063f3c8` | 24 | ascii-text | none |
| `0x0063f3e0` | 32 | ascii-text | none |
| `0x0063f520` | 48 | int32-array | none |
| `0x0063f560` | 64 | f32-array | none |
| `0x0063f5b8` | 24 | f32-array | none |
| `0x0063f620` | 8 | f32-array | none |
| `0x0063f628` | 8 | f32-array | none |
| `0x0063f630` | 8 | f32-array | none |
| `0x0063f638` | 8 | f32-array | none |
| `0x0063f640` | 8 | f32-array | none |
| `0x0063f648` | 8 | f32-array | none |
| `0x0063f660` | 8 | f32-array | none |
| `0x0063f668` | 32 | f32-array | none |
| `0x0063f888` | 8 | f32-array | none |
| `0x0063f9b0` | 16 | f32-array | none |
| `0x0063faa0` | 32 | ascii-text | none |
| `0x0063fb50` | 32 | ascii-text | none |
| `0x0063fbb0` | 24 | ascii-text | `0xbdd` |
| `0x0063fca0` | 854 | struct | `0x10` |
| `0x0063fff6` | 109 | struct | `0x47e` |
| `0x0064006e` | 195 | struct | `0x4f6` |
| `0x00640131` | 1471 | struct | `0x661` |
| `0x00640e70` | 2032 | f32-array | `0x61c` |
| `0x006417e0` | 144 | struct-array | `0x10` |
| `0x0064253a` | 43 | struct | `0x1ee` |
| `0x00643030` | 24 | ascii-text | `0x11f` |
| `0x00643070` | 16 | ascii-text | `0x9a23` |
| `0x00643128` | 24 | ascii-text | `0x7e` |
| `0x00643170` | 16 | ascii-text | `0x94` |
| `0x006431e0` | 16 | ascii-text | `0x3e7b` |
| `0x006432b0` | 1397 | struct | `0x128` |
| `0x00643825` | 1243 | struct | `0x745` |
| `0x00644e80` | 24 | ascii-text | none |
| `0x00644ed0` | 448 | struct-array | `0x20` |
| `0x006450b0` | 48 | struct-array | `0x10` |
| `0x00645240` | 3877 | struct-array | `0x998` |
| `0x0064616f` | 1021 | struct-array | `0x18c7` |
| `0x00646673` | 685 | struct-array | `0x1d97` |
| `0x00646961` | 768 | struct-array | `0x2085` |
| `0x00646c61` | 257 | struct-array | `0x2385` |
| `0x00646d62` | 208 | struct-array | `0x2486` |
| `0x00646e32` | 47 | struct | `0x2556` |
| `0x00646f47` | 794 | struct-array | `0x266b` |
| `0x0064726f` | 191 | struct-array | `0x2993` |
| `0x0064732e` | 802 | struct-array | `0x2a52` |
| `0x0064a078` | 4 | int32-array | none |
| `0x0064a07c` | 4 | int32-array | none |
| `0x0064a080` | 16 | int32-array | none |
| `0x0064a090` | 4 | f32-array | none |
| `0x0064a0e8` | 240 | f32-array | none |
| `0x0064a1e0` | 8 | f32-array | none |
| `0x0064a1e8` | 8 | f32-array | none |
| `0x0064a618` | 24 | ascii-text | none |
| `0x0064a630` | 24 | ascii-text | none |
| `0x0064a648` | 8 | scalar | none |
| `0x0064a650` | 8 | scalar | none |
| `0x0064a658` | 8 | scalar | none |
| `0x0064a668` | 8 | scalar | none |
| `0x0064a690` | 16 | int128-array | none |
| `0x0064a6b8` | 8 | scalar | none |
| `0x0064a6c0` | 8 | scalar | none |
| `0x0064a6d0` | 16 | ascii-text | none |
| `0x0064a790` | 848 | struct | `0xc8` |
| `0x0064b090` | 48 | ascii-text | `0xe86` |
| `0x0064b420` | 1216 | struct-array | `0x2d392` |
| `0x0064b8e0` | 1216 | struct-array | `0x2d392` |
| `0x0064bda0` | 1216 | struct-array | `0x2d392` |
| `0x0064d2c0` | 48 | ascii-text | `0x257` |
| `0x0064d3e8` | 24 | ascii-text | `0xe0` |
| `0x0064d420` | 40 | ascii-text | `0x190` |
| `0x0064e3b0` | 144 | int32-array | none |
| `0x0064e440` | 32 | int32-array | none |
| `0x0064eeb0` | 32 | ascii-text | none |
| `0x0064f0e0` | 16 | ascii-text | none |
| `0x00710500` | 16 | ascii-text | `0x2dc` |
| `0x007124c0` | 16 | ascii-text | `0x7cc` |
| `0x00713138` | 16 | ascii-text | `0xadc` |
| `0x00713180` | 32 | ascii-text | `0xdc8` |
| `0x007131a0` | 32 | ascii-text | `0x1310` |
| `0x007132f0` | 16 | ascii-text | none |
| `0x00713448` | 24 | ascii-text | `0xbbc` |
| `0x00713470` | 16 | ascii-text | none |
| `0x00713480` | 8 | struct | none |
| `0x00713490` | 4 | struct | none |
| `0x00713cc0` | 16 | ascii-text | none |
| `0x00713ce0` | 16 | struct-array | none |
| `0x00713d00` | 16 | struct | none |
| `0x00713d18` | 8 | f32-array | none |
| `0x00713d20` | 4 | f32-array | none |
| `0x00713d24` | 4 | f32-array | none |
| `0x00713d28` | 8 | f32-array | none |
| `0x00713d5c` | 180 | int32-array | none |
| `0x00713e10` | 16 | ascii-text | none |
| `0x00713e20` | 16 | ascii-text | none |
| `0x00713e30` | 32 | ascii-text | none |
| `0x00713e50` | 32 | ascii-text | none |
| `0x00713e84` | 108 | int32-array | none |
| `0x00713ef0` | 32 | ascii-text | none |
| `0x00713f24` | 76 | int32-array | none |
| `0x00713ff0` | 32 | ascii-text | none |
| `0x00714028` | 232 | struct-array | none |
| `0x00714110` | 32 | ascii-text | none |
| `0x00714148` | 88 | struct-array | none |
| `0x00714368` | 24 | struct | `0xde2` |
| `0x007144a8` | 16 | ascii-text | none |
| `0x007144b8` | 16 | ascii-text | none |
| `0x00714508` | 24 | ascii-text | none |
| `0x007146b0` | 16 | ascii-text | none |
| `0x007146d0` | 16 | ascii-text | none |
| `0x00748340` | 16 | ascii-text | none |
| `0x007488f8` | 16 | int8-array | none |
| `0x00748908` | 1 | int8-array | none |
| `0x00749040` | 32 | int16-array | `0x10` |
| `0x00749060` | 32 | int16-array | `0x10` |
| `0x00749080` | 32 | int16-array | `0x10` |
| `0x007490a0` | 32 | int16-array | `0x10` |
| `0x007490c0` | 32 | int16-array | `0x10` |
| `0x007490e0` | 24 | int16-array | `0x10` |
| `0x007494d0` | 48 | f32-array | `0x10` |
| `0x00749500` | 48 | f32-array | `0x10` |
| `0x007495c0` | 64 | f32-array | `0x0` |
| `0x00749840` | 640 | struct-array | `0x10` |
| `0x00749ac0` | 112 | struct-array | `0x5e8` |
| `0x00750008` | 132 | struct-array | `0x65e` |
| `0x007525d4` | 44 | struct | `0xdcb0` |
| `0x00753b20` | 24 | ascii-text | `0x955` |
| `0x007540c0` | 16 | struct | `0xbc18` |
| `0x00755050` | 40 | int64-array | `0x396` |
| `0x00761120` | 4 | f32-array | none |
| `0x0076112c` | 4 | f32-array | none |
| `0x00761130` | 4 | f32-array | none |
| `0x00761134` | 4 | f32-array | none |
| `0x00761138` | 4 | f32-array | none |
| `0x0076113c` | 4 | f32-array | none |
| `0x00761144` | 4 | f32-array | none |
| `0x00761148` | 4 | f32-array | none |
| `0x0076114c` | 4 | f32-array | none |
| `0x00761150` | 4 | f32-array | none |
| `0x0076116c` | 4 | f32-array | none |
| `0x00761170` | 4 | f32-array | none |
| `0x00761174` | 4 | f32-array | none |
| `0x00761178` | 4 | f32-array | none |
| `0x0076117c` | 4 | f32-array | none |
| `0x00761180` | 4 | f32-array | none |
| `0x00761184` | 4 | f32-array | none |
| `0x00761188` | 4 | f32-array | none |
| `0x0076118c` | 4 | f32-array | none |
| `0x00761190` | 4 | f32-array | none |
| `0x00761194` | 4 | f32-array | none |
| `0x00761198` | 4 | f32-array | none |
| `0x0076119c` | 4 | f32-array | none |
| `0x007611a0` | 4 | f32-array | none |
| `0x007611a4` | 4 | f32-array | none |
| `0x007611a8` | 4 | f32-array | none |
| `0x007611ac` | 4 | f32-array | none |
| `0x007611b0` | 4 | f32-array | none |
| `0x007611b4` | 4 | scalar | none |
| `0x007611b8` | 4 | f32-array | none |
| `0x007611bc` | 4 | f32-array | none |
| `0x007611c0` | 4 | f32-array | none |
| `0x007611c4` | 4 | f32-array | none |
| `0x007611c8` | 4 | f32-array | none |
| `0x007611cc` | 4 | f32-array | none |
| `0x007611d0` | 4 | f32-array | none |
| `0x007611d4` | 4 | f32-array | none |
| `0x007611d8` | 4 | f32-array | none |
| `0x007611dc` | 4 | f32-array | none |
| `0x007611e0` | 4 | f32-array | none |
| `0x007611e4` | 4 | f32-array | none |
| `0x007611f4` | 4 | f32-array | none |
| `0x007611f8` | 4 | f32-array | none |
| `0x00761200` | 4 | f32-array | none |
| `0x00761204` | 4 | f32-array | none |
| `0x00761208` | 4 | f32-array | none |
| `0x00761218` | 4 | f32-array | none |
| `0x0076122c` | 4 | f32-array | none |
| `0x0076124c` | 4 | f32-array | none |
| `0x00761250` | 4 | scalar | none |
| `0x00761260` | 4 | f32-array | none |
| `0x00761270` | 4 | f32-array | none |
| `0x00761274` | 4 | f32-array | none |
| `0x00761288` | 4 | f32-array | none |
| `0x00761290` | 4 | f32-array | none |
| `0x007612cc` | 4 | f32-array | none |
| `0x007612d0` | 4 | f32-array | none |
| `0x007612e4` | 4 | f32-array | none |
| `0x007612f4` | 4 | f32-array | none |
| `0x00761300` | 4 | f32-array | none |
| `0x00761304` | 4 | f32-array | none |
| `0x00761308` | 4 | f32-array | none |
| `0x0076135c` | 4 | f32-array | none |
| `0x00761398` | 4 | scalar | none |
| `0x007613ac` | 4 | f32-array | none |
| `0x007613bc` | 4 | f32-array | none |
| `0x007613c4` | 4 | f32-array | none |
| `0x007613d8` | 4 | f32-array | none |
| `0x007613dc` | 4 | f32-array | none |
| `0x007613ec` | 4 | f32-array | none |
| `0x007613f4` | 4 | f32-array | none |
| `0x007613f8` | 4 | f32-array | none |
| `0x00761444` | 4 | f32-array | none |
| `0x00761450` | 4 | f32-array | none |
| `0x0076146c` | 4 | f32-array | none |
| `0x00761470` | 4 | f32-array | none |
| `0x0076147c` | 4 | f32-array | none |
| `0x00761490` | 4 | f32-array | none |
| `0x00761494` | 4 | f32-array | none |
| `0x00761498` | 4 | f32-array | none |
| `0x0076149c` | 4 | f32-array | none |
| `0x007614a0` | 4 | f32-array | none |
| `0x007614a4` | 4 | f32-array | none |
| `0x007614a8` | 4 | f32-array | none |
| `0x007614b8` | 4 | scalar | none |
| `0x007614c0` | 4 | f32-array | none |
| `0x007614c8` | 4 | f32-array | none |
| `0x007614cc` | 4 | scalar | none |
| `0x007614d4` | 4 | f32-array | none |
| `0x0076151c` | 4 | f32-array | none |
| `0x00761594` | 4 | f32-array | none |
| `0x00761608` | 4 | f32-array | none |
| `0x00761610` | 4 | f32-array | none |
| `0x00761614` | 4 | f32-array | none |
| `0x00761618` | 4 | f32-array | none |
| `0x00761620` | 4 | f32-array | none |
| `0x00761624` | 4 | f32-array | none |
| `0x00761628` | 4 | f32-array | none |
| `0x00762ca0` | 8 | int32-array | none |
| `0x00762d80` | 8 | ascii-text | none |
| `0x00762d90` | 4 | f32-array | none |
| `0x00762d94` | 4 | f32-array | none |
| `0x00762ea0` | 8 | int32-array | none |
| `0x00762ee0` | 8 | ascii-text | none |
| `0x00762f00` | 8 | ascii-text | none |
| `0x00762f20` | 8 | ascii-text | none |
| `0x00763038` | 1 | struct | none |
| `0x00763039` | 1 | scalar | none |
| `0x0076303a` | 1 | scalar | none |
| `0x0076303b` | 1 | scalar | none |
| `0x0076303c` | 4 | int32-array | none |
| `0x00763040` | 4 | int32-array | none |
| `0x00763058` | 8 | ascii-text | none |
| `0x00763060` | 8 | ascii-text | none |
| `0x00763078` | 8 | ascii-text | none |
| `0x00763098` | 8 | ascii-text | none |
| `0x00763128` | 8 | struct | none |
| `0x00763130` | 8 | ascii-text | none |
| `0x00763138` | 4 | struct | none |
| `0x0076313c` | 4 | struct | none |
| `0x00763140` | 8 | ascii-text | none |
| `0x00763148` | 8 | ascii-text | none |
| `0x00763150` | 8 | ascii-text | none |
| `0x00763158` | 4 | struct | none |
| `0x007635c0` | 8 | struct | none |
| `0x007637a0` | 8 | int32-array | none |
| `0x007637a8` | 8 | int8-array | none |
| `0x00763824` | 4 | struct | none |
| `0x00763880` | 8 | ascii-text | none |
| `0x007638d8` | 8 | int32-array | none |
| `0x00763960` | 8 | ascii-text | none |
| `0x00763cd8` | 8 | int64-array | none |
| `0x007641fc` | 4 | f32-array | none |
| `0x00764200` | 4 | scalar | none |
| `0x00764204` | 4 | scalar | none |
| `0x00764208` | 8 | ascii-text | none |
| `0x00764210` | 112 | ascii-text | none |

## Qualification and remaining gate

This is a docs-only classification batch. The main agent should check the union of primary/supplement
by address against the 1,043-object canonical inventory and ensure only456 addresses receive this
supplement. Synthetic binder tests must reject candidate-only records as approved source mappings and exclude
FTD padding. Each PROVEN row must cover all required seams before its runtime status changes from
pending-seam to bound; preserve the specified PS2 tail. Source-reader proofs remain obligations for
445 OPEN records. Thirty-one records were materially investigated in the facility/shop batch: nine promotions
and22 shop records with concrete PS2 reader layouts and redesigned Golden-module blockers. No build, game run, BBV, unit-test,
objdiff or image-binder qualification is claimed by this specification.

## Batch 2: source proofs and implementer vectors

The nine PROVEN rows total424 PS2 inventory bytes. `required_seams` is a list of every known direct
or inlined read site plus the prefix/tail rule. `runtime_status=pending-seam` is not a source-proof
failure: it means the implementer must still bind and test those seams. No bound row is claimed here.

### Stage-kind lists

PS2 `y_fclCombine.c:6492-6506` selects these six signed-halfword arrays for kinds1..6:
`00749040`, `00749060`, `00749080`, `007490a0`, `007490c0`, `007490e0`.
Golden's filename selector preserves that exact category ordering. Independent NSO instruction
inspection at1737e0..173820 confirms signed category*8, signed index*2 and an unsigned halfword
load at member+16. Preserve the halfword bits; apply the original caller's signed/narrowing
interpretation at its boundary, rather than assuming every result is a signed32-bit scalar.

The direct PS2 lookup003042f0 is not sufficient:0030c3c0 inlines the same selection at source
lines9367/9383 for neighboring reward choices. The old PS2 roll producer003040d0 can reach
index10 at level99. This producer seam is now resolved from ordinary Golden instructions,
not guessed: at Switch15e95c..15ea60 the adjusted level is clamped1..99, **subtracts1**, and
then uses signed integer division by10. Its index is0..9. Replace the PS2 threshold loop with
that observed Golden calculation; do not consume a retained PS2 tail element.

Synthetic vector: six arrays of10 halfwords, each entry `1000 + 100*category + index`.
Both direct and inlined seams must select the same bits for every category1..6/index0..9.
Fill preserved PS2 tail and FTD padding with different sentinels: neither is a source entry.
Adjusted levels1,10,11,98,99 yield indices0,0,1,9,9 respectively. First five PS2 spans preserve
12 B after the20-byte prefix; the sixth preserves4 B.

### Monthly message selector

PS2 menu case0030b7b0 uses month row `(month-1)*11`, column `trunc(10*average_score)`,
then adds2 to the selected signed byte. Golden1735c0 uses the same calendar/score row structure,
but reads a byte selector and obtains the final u32 message from `fclCmmRankMsgNo.ftd`.
The additional source is20 live bytes/5 rows at+16, full-member SHA-256
`f26211d22b937945e0d2e0563da3871d31f227fcc0e9e4b54d1370e24a312e1e`.
Instruction inspection confirms byte selection followed by a4-byte mapped load.

Synthetic vector:132 selector bytes arranged12x11, selector `(row+column)%5`, and five
synthetic message IDs `1000+17*selector`. Month3/column7 yields selector4 and message1068,
**not** selector+2. Preserve12 trailing PS2 span bytes if installing the selector prefix.
The auxiliary map needs its own registered source even though it has no PS2 inventory address.

### Community EXP sources are floats

The raw decompiler signature for1737a0 reports an integer return; that is wrong. Independent
instruction inspection shows a scalar float load at+16+4*rank and return in the float register.
Caller15b918 multiplies that float into EXP and truncates to integer. PS2's corresponding source
is `D_007494D0[rank]` in00311930's optional-multiplier branch.

`fclComuLvExp.ftd` is registered as pointer/count by174430 ->170300; its payload pointer lands
in global9fb578. Reader16d0f0 loads f32 at `pointer + 4*rank` twice, for whole and decimal-fraction
extra-level calculations. This corresponds structurally to `D_00749500[rank]` in00311930.
The observed Golden arithmetic is not asserted equivalent to the PS2 incremental loop:

- Rank getter result0 returns0.
- Let base level be `L`, source scale be `S`, and `E(persona, level)` the cumulative EXP accessor
  at07bc30. Compute `T=trunc(float(L)+S)` and `U=min(T,99)`.
- Start with `E(persona,U)-E(persona,L)`.
- If `T<99`, compute `F=trunc((S-float(U-L))*10)`. When `F>0`, add
  `((E(persona,U+1)-E(persona,U))/10)*F`, preserving integer division before multiplication.
- Negative final result is cleared to0. The optional weather multiplier is a separate reader.

Synthetic vector: `E(level)=100*level*level`, nonzero rank, `L=10`, `S=2.5` gives5650;
`L=98`, `S=2.5` gives19700 after the level99 cap. With base EXP100 and synthetic multiplier1.75,
the multiplier seam yields175; the no-multiplier branch stays100. These are synthetic algorithm
vectors, not captured retail outputs. Both11-float sources bind44 bytes only, retaining4 PS2 bytes.

### Twenty-two shop rows: concrete reasons to remain OPEN

The batch also investigated22 shop/message-bank objects, rather than promoting heuristic floats:

-0063f2b0..0063f2cc are fragments/aliases of an indexed message-bank selector array;0063f2c0
  is also used as the paired count-text bank. The generic reader accepts a signed-byte table
  selector. Independent fragment writes cannot establish the whole array's handle namespace.
-0063f2f0 and0063f310 copy only6 and9 s32 values, respectively, despite32/48-byte inventory
  spans.0063f520 copies9 disc request IDs from a48-byte span. Archive byte offsets or FTD text
  rows cannot be substituted for these IDs.
-0063f3c8 is a diagnostic allocation/resource label;0063f3e0 is a task name. Golden reorganized
  the PS2 shop controller into shop-base and weapon tasks; their labels cannot be selected by
  spelling alone.
-0063f560 is a shared8-byte-per-entry anchor-position table base. Runtime anchor indices reach
  far past its64-byte inventory fragment; neighboring vector objects alias this table.
  The ten directly investigated vector fragments have panel/scroll roles recorded per row,
  including partial-span warnings for0063f5b8/0063f668.

Concrete Golden footholds, not yet object mappings:
172510 reads `fclWeaponCategoryName.ftd`,1725d0 reads `fclWeaponSellList.ftd`,172680 reads
`fclWeaponTopMenu.ftd`. Their row stride is32,34 or44 bytes depending on the locale selector.
19c710 constructs20-byte shop-list rows and invokes shop-base logic.1b15d0 allocates the weapon
task, calls1987f0 and uses a distinct work layout. The cheapest remaining proof follows these
callers into bank registration/renderer ownership, reconstructing logical message and anchor IDs.
All22 records name their actual PS2 reader span and the specific Golden-namespace/layout blocker.

## Batch 3: resolved stage producer and concrete archive blockers

This batch materially updates33 records: all nine PROVEN source descriptors are normalized to
logical payload bounds, six of those receive the resolved producer algorithm, and24 OPEN records
receive actual packed-bank or calendar-record structure. Counts stay9 PROVEN/447 OPEN.

### Exact Golden stage producer

Ordinary Switch instructions15e95c..15eabc establish the following, independently of the
decompiler's return-type guesses:

1. Read base persona level through07b9f0.
2. With one RNG result `R`, form random percent by converting `R & 0x7fffffff` to binary32,
   multiplying by binary32 `2^-31`, then binary32 `100`, then truncating toward zero.
   Stage is0 below40,1 below65,2 below85,3 below95, otherwise4. These are cumulative thresholds.
3. Form a second random percent in the same way from a separate RNG result. Percent<=29
   subtracts stage; otherwise add stage. Preserve the observed signed-byte narrowing in the
   comparison path; valid base levels1..99 and stages0..4 do not overflow that signed byte.
4. Subtraction floors the adjusted level at1; addition caps it at99.
5. **Index is `trunc((adjusted_level-1)/10)`**, not `adjusted_level/10`.
   Explicit subtract1 is at15ea3c, followed by signed constant division at15ea40..15ea54;
  15ea60 passes the index to the halfword source reader1737e0.
6. If the returned skill is already present, the next source read uses index+1 only when index<9
   (15ea8c). The lower neighboring read uses index-1 only when index>=1 (15eabc).

Thus level99 maps to9, not10, without a fabricated array clamp. Keep the binary32 RNG scaling
operation order; rounding can produce percent100, which belongs to stage4/addition, not an
invented reroll. Synthetic threshold vectors39/40/64/65/84/85/94/95 select0/1/1/2/2/3/3/4;
direction29 subtracts and30 adds. These vectors describe synthetic inputs, not captured retail RNG.

### Eighteen battle-bank candidate windows

The sixteen AICALC candidates have now been resolved to their real packed payloads:
two are in program block9, thirteen in program block10, and one in enemy-AI block8.
Block9 starts at file61220 with82718 live bytes; base `data.cpk` block10 starts at143956
with281267 live bytes. The English bank's different program size is not interchangeable with
this base-member candidate. Every record carries its exact block-relative offset and block list.

In particular,0064b420/0064b8e0/0064bda0 all choose the **same**1216-byte window at block10
payload+41278. That is three heuristic votes for one program window, not proof of three object
identities.007540c0's candidate is enemy-AI row288 at field+68, not an independently identified
16-byte literal. Missing PS2 procedure/field ownership is the concrete blocker.

The two English MSG candidates are interior substrings of block1's23-byte skill names, not full
rows.005df308 points into skill-name row92 at character+1;00626530 points into row253 at
character+9. Without a PS2 owner/skill ID and passive-domain mapping, neither can be promoted.

### Six WeatherBonus fragment candidates

The Steam `WeatherBonusTbl.ftd` header declares3584 live bytes =128x28 rows at+16; its3616-byte
member has16 trailing padding bytes. Switch173720 returns payload/count, and169790 scans28-byte
records matching calendar month/day bytes+0/+1. Bytes+2..7 form a constraint header; bytes+8..27
hold five4-byte effects. This is structural source evidence, not proof of each M2 fragment window.

PS2 logic base0063fca0 and display base006432b0 have corresponding28-byte typed rule records,
but M2 splits both into fragments, including midrecord boundaries. Golden adds calendar rows;
the candidate offsets do not preserve row-relative positions. The remaining proof is to establish
the full PS2 table extents and join by date/constraint/effect role, covering both logic and display
consumers. A concatenation of the six heuristic windows is not an authorized projection.

## Batch 4: no-lead renderer-context source

This30-row batch promotes005dc974/84 B to PROVEN and records29 additional concrete function-correlation
leads from the exercised `reader_match.py` v3 receipt. Those29 remain OPEN: a matched function does
not by itself identify an object's field, complete extent or representation. Current totals are
**10 PROVEN/508 B;446 OPEN/69,019 B;D10a/b/c0**.

### Correspondence established without a byte-match premise

PS2 `D_005DC974` is a renderer-context reference table, not a balance-rate array:
`tnEffect.c` declares three banks of seven32-bit entries, with remapped getters at offsets0/28/56.
The effect wrappers004b11d0/004b1210 optionally select that table before flushing child effects.
Rank-up blur creation passes this same table (`cmmRankUp.c:3283`).

The reader matcher supplied Steam rank-up function1c14b0 as a lead (score0.86087, margin0.21444).
Independent ordinary `.shared` inspection then proves the actual object:

-1c1863 loads the address of `.xpdata` RVA**ab7560**, passed as the sixth stack argument at
  the1c18cb call to46f7e0.
-46f7e0 stores that argument at its work+0x18; callback46f630 saves/selects/restores the table
  around the effect pass using global62488c8.
-Three ordinary getters50fd70/50fdb0/50fdf0 access seven remapped8-byte entries in banks
 +0x38/+0x70/+0. The source is therefore168 B, not84 B.

Source: Steam10836033 `.xpdata`, RVAab7560, raw file offsetab5d60,168 B.
SHA-256 `dc368b7354afc92ee932f50ec1da1125a9eeec1371c026e476844ec764d20a8c`.
These are preferred-image queue references whose target objects are virtual-only `.xpdata`;
the proof reads the file-backed references, **not** target queue payload or protector code.

### Exact lossless normalization into existing port queues

All three banks alias **the same seven queues**, on both platforms. For logical queue `j=0..6`:

| Property | Golden source | Existing guest/native-port representation |
|---|---|---|
| Queue identity | preferred VA minus imagebase140000000 = `1e41b2c0 + j*0x60` | `00796520 + j*0x30` |
| Table entry width |8 B |4 B |
| Bank offsets |0/56/112 |0/28/56 |
| Bank aliasing |Three identical copies of the seven references |Same three identical copies |

The eight source remap bytes at `.sdata` RVA912a24 are identical to the existing
`tnSlotRemap[8]` declaration (`tnEffect.c:116`); the first seven form a permutation of0..6.
The native port explicitly recognizes guest queues796520..796640/stride0x30 as generic cut-in
screen-model queues (`src/render/p4gl_im2d.c:1257-1260`).

Converter contract:
1. Read the168-byte file-backed source and the8-byte remap; pin the source executable/build.
2. Normalize every preferred VA by subtracting the preferred imagebase140000000.
3. Require source entry `(bank,j)` to identify `1e41b2c0+j*0x60`, for every bank0..2 and j0..6;
   require the observed three-bank aliasing and remap identity.
4. Emit32-bit guest queue identity `00796520+j*0x30` in each corresponding bank.
   The result is84 B and equals the existing PS2 table: equality is a consequence of the proven
   queue-role normalization, not the original heuristic matcher.

This specific object needs **no new native queue namespace or renderer milestone prerequisite**.
Reuse the renderer-owned queue records and their existing initialization, per-frame clearing and
append lifecycle. The binder writes/validates the reference table before consumers; it must not
allocate, initialize, reset, clear or free queue objects. No Golden VA or host pointer enters guest RAM.
Existing32-bit getters may remain unchanged after normalization. Native static table copies are
acceptable only with a witness that their21 entries equal the normalized Golden output.

Required witnesses cover **all four** PS2 consumers: cutinDrawTaskUpdate00120bc0,
rank-up effects00257900, rank-up sequence0025b240, and shuffle reward effects0038d330;
also cover all three bank getters and both effect flush wrappers. Testing only the matched blur
call site would miss the other reads.

Synthetic vector: source imagebase `0x100000000`, queue-RVA base `0x2000`, source stride96,
guest queue base `0x4000`, guest stride48. Three banks each contain seven preferred addresses.
Normalize `(bank2,queue6)` to guest `0x4120`, with every bank producing the same queue identity.
Reject a mismatched bank entry, non-affine reference, wrong bank alias or changed slot remap;
never truncate a preferred VA. These addresses are synthetic, not retail payload.

## Batch 5: complete local-layout review and bounded Y-axis source

Main explicitly authorized **PROVEN-layout** for005efa20 on2026-11-13 after a live-prefix
hand-check. This is separate from a matched-reader PROVEN class: the ordinary Golden reader
has not been identified. The source placement is fixed by a unique local structural interval,
not by candidate similarity or global compiler/archive ordering.

The exercised `layout_anchor.py` v3 method was reused against all446 remaining OPEN rows using
the full9,415-object PS2 inventory and cached full-span anchors. Source-image hashes were checked
against the cache. One unique placement survives; the other445 receive per-object
`local_layout_review` records with actual left/right anchors, gap object counts and failure
reasons for `.sdata`, `.xpdata` and Switch mapped data. Those failures are **not** proof of absence
and do not evaluate ordering inside archive members. No archive-layout promotion is made here.

### 005efa20: singleton interval and live-width evidence

| Property | Left anchor | Admitted object | Right anchor |
|---|---|---|---|
| PS2 address |005efa10 |005efa20 |005efa30 |
| PS2 inventory extent |16 B |16 B |16 B |
| Steam `.sdata` RVA |917580 |917590 |9175a0 |
| Golden bounded extent |16 B |16 B |16 B |
| Basis |Full16-byte identity |Sole16-byte gap,4-byte aligned |Full16-byte identity |

There is exactly one contiguous object between the anchors, with no undocumented widening.
Other complete source placements do not survive. Source object raw file offset is916190
(decimal9527696); its16-byte SHA-256 is
`6855960ee428276148fc8745625c44bde7ae835a9e2fa181cc917af9bcad0d6a`.

Typed PS2 reader evidence:

-00146f50 `MT_Scene_BuildResMatrix` passes005efa20 as the Y-axis `RwV3d` to `RwMatrixRotate`,
  followed by the X/Z rotations and translation (`Scene/mt_scene.c:212-240`).
-0028c580 passes the same axis for model, character and light transforms
  (`k_sceneDraw_inferred_0028c3f0.c:204/227/242`).
-`RwV3d` is exactly three f32 fields (`rw_types_internal.h:12-18`); `SceneLightAxis` aliases it.
  Only12 bytes are live axis components, even though M2's aligned inventory span is16 bytes.

Independent hand-check observed: both full16-byte flanks match, source/PS2 live12 bytes match,
the final4 bytes also match, and the PS2 live vector is the unit Y-axis. The binding rule remains
**copy12 source bytes; retain4 PS2 tail bytes**, not a speculative four-component vector.
Both consumer groups and native static copies require live-prefix witnesses. No math or rotation
order change is authorized.

Synthetic fixture: a source12-byte three-component axis followed by a synthetic padding sentinel,
and a guest16-byte span ending in a different sentinel. After binding, only the first12 bytes
change; the guest tail survives exactly. Exercise both resource-matrix and scene-light readers.

Final supplement totals at this handoff: **10 PROVEN/508 B;1 PROVEN-layout/16 B;
445 OPEN-retain/69,003 B;D10a/b/c0**. GoldenRE-L1c owns subsequent module-level triage/deep RE
and primary-ledger merging. The445 remaining rows are unresolved sources, not demonstrated
intractable cases; the concrete local-probe failures and earlier reader/blocker evidence are
starting points for that continuation.

## L1c: complete dated fusion-table correspondence

The six WeatherBonus fragments previously OPEN in Batch 3 are now PROVEN.
See [the module proof](l1-fusion-weather-proof.json) and
[canonical specification](l1-image-literals.md#fusion-weather-module-level-structural-proof).
The former unresolved PS2 extent is now established by the actual sentinel
walk: 94 records of 28 bytes, including dummy and sentinel, in both duplicate
logic/display arrays. Their full 2,632-byte hashes are identical.
Golden supplies 128 records, including 34 added dates; 26 existing records
change. All PS2 date keys map uniquely, with the complete row-index join
recorded in the proof.

These six partitions account for 5,269 PS2 bytes, not six separate Golden
sources: bind the single 3,584-byte expanded FTD payload to all dated
constraint, outcome, skill-argument, today/tomorrow and display consumers.
Do not copy M2 windows, use old row indices, or overwrite adjacent
alignment/unknown partitions. The phase-indexed fallback table is not
qualified by this source proof. All six remain runtime-pending.

The complete [triage](l1-image-literals-triage.md) classifies every baseline
OPEN row, preserving unresolved consumer evidence without calling it dead.

## Cohort 4: directory representation replacement

The [initial directory proof](l1-directory-migration-proof.json),
[followup proof](l1-directory-migration-followup-proof.json), and
[M10 route contract](l1-directory-resource-routes.md) supersede the earlier
zero-D10c disposition **only for the cohort-4 address-keyed rows**.
This supplement contains **22 D10c /31,114 B; zero OPEN-retain**.
The disjoint current directory scope in the primary ledger is
**56 D10c /72,730 B; zero OPEN-retain**; other proof classes are zero.
Four non-directory objects /2,476 B from the original60-object assignment
were transferred to cohort6, not changed by this followup.
Do not add these supplement counts to the primary.

D10c drops PS2 static CVM directory metadata only after M10 binds each row's
`golden-resource-route:<class>` seam to its specified Golden archive/cache or
bank/cue namespace and actual producers. All22 are runtime pending-seam, not
current asset-loader qualification. No old-name fallback or guessed cue-to-wave
index is allowed. Prior heuristic archive-window candidates remain historical
metadata and must never supply directory bytes/sectors. E820's no-E826-assets
decision additionally carries the M11 script-request/reopen obligation.
Only this cohort subsection and its owned rows are changed here; shared
global summaries are maintained by the global-count owner.

## Cohort 6: manager diagnostic source argument

`0063BE10` / 16 B is now source-PROVEN by the structurally paired MSG1
validator, manager installer and object constructor. The
[panel-bank receipt](l1-ui-panel-bank-proof.json) pins the complete Golden
Switch source path at `71005F4F74`, 69 bytes including NUL, and actual
assertion/allocation-site argument-zero consumers.

This is a semantic diagnostic argument seam, not a prefix copy: resolve every
direct/inlined/native filename use to the full Golden path, preserve PS2 mode,
and never overwrite 69 bytes into the 16-byte guest literal. No corresponding
Steam path or runtime binding is claimed. Only this owned supplement row is
updated by this manager batch; global supplement totals belong to the global owner.

## Cohort 6: typed source-file diagnostic namespace

The [diagnostic proof](l1-diagnostic-source-proof.json) additionally qualifies
64 owned source-file objects / 1,448 PS2 B; **28** of these are present in
this supplement and are synchronized by address, not appended as duplicates.
Each has complete typed filename uses, one unambiguous full Golden path and
an actual argument-zero diagnostic/allocation-site sink. The semantic seam
supplies the entire Switch 1.2 path, not a retained PS2 basename or an
unproved Steam path. No long in-place guest overwrite is permitted.

Release assertion/allocation-site no-ops remain no-ops; the pool recorder's
actual file/line stores are distinct. The 19 reviewed incomplete/ambiguous
source cases remain OPEN, with their concrete blockers recorded. These
literal proofs do not qualify other data in the same module or runtime binding.

