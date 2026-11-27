# L1 cohort 3A: setup and automatic-dungeon resources

## Decisions

Four new **PROVEN / 127 PS2 B** objects: `005effe0` (31 B), `005f0650`, `005f06c0`, and `005f0720` (32 B each). After the [typed property schema](l1-cohort-3A-field-object-metadata.md), set A totals are **18 PROVEN / 527 B**, **93 OPEN-retain / 8,246 B**. PROVEN-layout and D10a/b/c remain zero. Every admission remains `pending-seam`.

The [complete receipt](l1-cohort-3A-dungeon-preload-proof.json) resolves setup/dungeon task identities, the complete effect archive/member family, and the dungeon BF source with its copy lifecycle. No B row, structural-bank transfer row or global summary was edited.

## Structural roles

`005effe0` is the `setup field resource` task identity. PS2 `00156640` clears static 32-byte setup work; Golden inlines the constructor in root update `71001be410`, using 40-byte work. Both register priority 15 with setup/free callbacks, store the field/request context and open the primary field file. Callback `001561a0` / `71001bf210` stages the independently paired field BF and HBN event readers and scene loader `00144f60` / `71002a0d60`. Text similarity alone is not the pairing. Install the complete 21-byte name and retain ten partition-tail bytes, not a rounded 32-byte copy.

`005f0650` is the automatic-dungeon identity, including its trailing space. Both constructors (`00159e90` / `71001cd1f0`) allocate zeroed work, register priority 15 with update/free callbacks, store step/entry inputs and clear flag `0xc25`. Work expands from 72 to 128 bytes. Install 19 live name bytes and retain 13 tail bytes; do not copy the expanded work layout.

## Positive global-alias proof

The original ELF `.reginfo` records `_gp = 0x7690f0`. Therefore GP-relative `iGpffffb244` is exactly `0x764334`, the same current-step pointer named `D_00764334` by neighboring source. This resolves the apparent global mismatch between begin/copy and create/finish helpers using ABI metadata rather than guessing from behavior.

The old step-work header explicitly aliases `scriptData`, `scriptAddress` and `scriptWord` in one union. Its two preload slots end the 72-byte block. Golden's three 8-byte slots at `+68/+70/+78` end the 128-byte block. Neither a third guest slot nor a differently named script field was silently assumed.

## Complete effect archive and all member/lifecycle users

`005f06c0` is the source archive `field/fld_effect.bin`. Both begin paths guard the current step context, first preload slot and shared archive handle, then request with flag 1. The complete Golden child set is:

| Full member key | Constructor argument 3 | Golden work slot |
|---|---:|---|
| `field/ENCONT_N.EPL` | 30 | `+68` |
| `field/F_HAKEN.EPL` | 20 | `+70` |
| `field/P_HAKEN.EPL` | 20 | `+78` |

The first two keys/parameters match the complete old two-member set; the third is a positively observed Golden addition. All four name sources (archive plus three members) have exact sealed Steam spans. Code-derived parameters are **PROVEN-code** components, not a new ledger class.

Create helper `71001cf7b0` parents every child to the current step task and passes the same borrowed archive. Constructor `71001bd100` records borrowed/archive mode, the observed parameter, and the **full** `field/...EPL` key; no basename stripping is authorized. Finish helper `71001cf890` polls all three non-null children, returns not-ready while any is incomplete, then releases and clears the shared archive once.

The old request/creation/finish functions `0015c210`, `0015c280`, and `0015c360`, including native/inlined copies, are explicit required seams. A filename-only change leaving a two-slot loop is not closure. Expanded backing must be owned by the Golden-aware provider; never write three handles into the old guest array or release the borrowed archive before parsing completes. Archive/EPL payload decoding and rendering remain unqualified runtime work.

## Complete BF source and owned-copy protocol

`005f0720` supplies `field/script/dungeon.bf` (24 live bytes, eight retained tail bytes). Both request paths require current step context and an absent script buffer, and request with flag 0. Both copy the complete ready source into an owned size-based buffer, store pointer/size together and release the transient request. Golden merges the old request/copy helpers into update states 1 and 10. `0015c4b0` is explicitly included alongside the literal reader `0015c440`.

This is source/ownership correspondence, not a claim that Golden BF bytes, the VM or field behavior are PS2-compatible. Native/translated copies and Golden provider/decoder routes remain pending before runtime closure.

## Exercised evidence

`<scratch>/GoldenRE-notes/l1/l1c3A_dungeon_proof.py` reported `QUALIFIED 4 127`, three Golden EPL children and the exact GP alias `0x764334`. Source-image bounds/hashes, full names, constructor/callback protocols and ELF ABI metadata were statically inspected. The row installer preserved every unowned/B line byte-for-byte. No build, verify, objdiff, unit suite, BBV, CI, game/emulator execution or `.arch` decoding was run.
