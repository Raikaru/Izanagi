# SKILL1 widened flags: data observations

This specifies the widened flag-word readers in Switch patch 131072;
the individual consumers and selector are corroborated against Steam
10836033 where stated. Retain all four bytes. This specification does not
itself implement or register the Golden-mode adapter.

For Steam build 10836033, `battle/SKILL.TBL` block 1 contains 472 rows of
44 bytes. The flag word occupies bytes 0–3 in little-endian order. Direct
measurement of bytes 1–3 found:

| Byte offset | Observed values and counts |
| --- | --- |
| 1 | 0: 443; 1: 2; 2: 8; 3: 8; 4: 5; 6: 1; 8: 1; 12: 2; 14: 1; 16: 1 |
| 2 | 0: 472 |
| 3 | 0: 472 |

All 29 nonzero byte-1 rows are within indices 0–439, the overlapping PS2
active-skill range. Consequently the earlier claim that the widening is
nonzero only for Golden-only skills is incorrect. In particular, dropping the
upper flag bytes on shared IDs silently loses data. Zero values in this build
are not a guarantee for other builds. Apart from the specific behavior below,
the observed combinations alone do not establish the meaning of the bits.

## Threshold trigger: bit `0x200`

The Golden consumer at RVA `0xD40F0` tests bit `0x200` of the 32-bit flag
word first. If set, it immediately returns trigger 0, even when the unit's
status word has bit 3 set. Otherwise it checks that status bit and returns 4
when set. It then calls the skill-cost helper and examines byte 6 of the
skill row: value 1 selects HP, value 2 selects SP, and other values return
0. For HP, low flag bit 0 clear returns trigger 1 when current HP is less
than or equal to cost; bit 0 set returns 1 only when HP is strictly less than
cost. For SP, return 2 when current SP is strictly less than cost; otherwise
return 0. The PS2 analogue is `datCalcThresholdTrigger` at `0x0023DDC0`,
whose special-effect-byte shortcut is not this Golden flag test.

The other widened branches and combined `0x200`–`0x1000` selector are
detailed below.

**CORROBORATED, Switch patch 131072 vs Steam 10836033:** Switch
`0x7100078430` tests the same `0x200` bypass, status bit, cost type and
HP/SP comparisons as Steam `0xD40F0`. Switch `0x7100026F60` uses this
trigger when deciding whether an active skill can enter command 8471's
candidate lists. The selector and remaining widened bits are specified below.

**CORROBORATED, bit `0x100`:** In the non-category-3 branch of a larger
effect-processing routine, Switch `0x710001369C`–`0x71000136E8` tests
the skill row's bit `0x100`. If set, it bypasses creation and setup of
an intermediate effect object; if clear, that object is created and
configured. Steam `0x54528`–`0x54598` has the same branch and bypass.
This establishes the control-flow effect, not the object's full visual
meaning.

**CORROBORATED, bit `0x800`:** Within a battle-result accumulator, Switch
`0x710004BA38`–`0x710004BA8C` reaches the counter branch only when the
halfword category equals 1, two preceding per-result values are not 1,
and the associated result condition is nonzero. In that branch it adds
`1 - ((skill_flags >> 11) & 1)` to its local counter. Steam
`0x9D091`–`0x9D0C1` applies the same suppression: the counter increments
only when the flag is clear. The surrounding counter's complete role is
not established.

| Skill flags | Other counter-branch conditions | Increment |
| ---: | --- | ---: |
| `0` | true | 1 |
| `0x800` | true | 0 |

**CORROBORATED, bit `0x400`:** Switch `0x7100078B20`–`0x7100078BDC`
checks a skill row's `0x400` flag in a target-evaluation routine. After
earlier type and cross-side checks, when the target unit word has bit 0
set and byte 0 of its associated 60-byte enemy row has bit 2 set, the
routine immediately returns `0x100`. Steam `0xD46EE`–`0xD4711` makes
the same tests and reaches the `0x100` return at `0xD4926`. This records
the numeric outcome; a gameplay name for the enemy-row bit and outcome
is not yet established.

| Skill flags | Target word bit 0 | Enemy row byte 0 bit 2 | Branch |
| ---: | ---: | ---: | --- |
| `0x400` | 1 | 1 | returns `0x100` |
| `0` | 1 | 1 | continues evaluation |
| `0x400` | 0 | 1 | continues evaluation |

## Combined selector in bits 9–12

**CORROBORATED:** This is an additional use of `0x200`, `0x400`, and
`0x800` and a use of `0x1000`: these four bits are also interpreted
*together* as a four-bit value `selector = (flags >> 9) & 15`, not as
four independent booleans. In two calculations, Switch
`0x7100073F84`–`0x7100073F94` and `0x71000760B0`–`0x71000760C4`
replace the caller's second-argument unit pointer when this value is
nonzero; when zero, they retain that argument. Switch helper
`0x7100112500` selects `(selector - 1)` from a unit-record array with
stride `0x84`. Steam `0xD1989`–`0xD19AD` and
`0xD2C68`–`0xD2C8C` perform the same extraction, zero test and pointer
selection with the same stride. The selected record is used by the
subsequent calculation, so preserving the word but ignoring this field
changes behavior. Do not treat `0x1000` as unused.

| Synthetic flags | Selector | Unit argument |
| ---: | ---: | --- |
| `0` | 0 | caller's original |
| `0x200` | 1 | array element 0 |
| `0x400` | 2 | array element 1 |
| `0x800` | 4 | array element 3 |
| `0x1000` | 8 | array element 7 |
| `0x1E00` | 15 | array element 14 |
| `0x100` | 0 | caller's original |

## Coverage of all widened bits

**SWITCH-ONLY:** In this Switch NSO the flag-word consumers above are the
only observed influences of bits 8–31. Bits `0x2000`, `0x4000`, `0x8000`,
and each bit `0x00010000` through `0x80000000` have no independent reader
or effect in these consumers. Preserve them unchanged; do not invent behavior
or treat this result as a guarantee for another build. Full-word and halfword
loads exist, so “no influence” does not mean those bytes are never loaded.
For the measured Steam table, the selector values occur as follows:
0:445, 1:16, 2:5, 3:1, 4:1, 6:2, 7:1, 8:1.

The Switch audit followed the 91 direct text references to the SKILL1
pointer GOT `0x71008805A0` through derived table and row addresses, flag
loads, branches, the saved flag word at `0x71000133D4` and its stack
reloads, and the target-evaluation jump table at `0x7100078C00`.
The large calculation at `0x7100073EE0` was checked by disassembly when
the decompiler timed out. Four other relocations to the same pointer slot
occur in language-specific `battle/SKILL.TBL` load descriptors at
`0x710071C1B8`, `0x710071CBF8`, `0x710071D638`, and `0x710071E0B0`:
`0x7100058200` passes those descriptors to the table loader
`0x7100058AA0`, which assigns the table pointer and copies each block.
They are not additional flag tests. Nearby Switch `0x7100078C20` and
`0x7100078C30` test the target unit's first halfword; `0x71000764D4`
tests SKILL1 row `+0x20`, not the flag word.

Steam's ordinary `.shared` slot `0xEC0910` has 263 direct RIP-relative
references in a linear scan; the positive widened consumers are independently
corroborated above. This is not a claim about protected Steam `.arch`.
R10 and P6's flag-reader specification is complete for the audited Switch
build; their runtime refusals remain until the corresponding adapter is
implemented. R9's separate passive semantic-element contract is specified below.

| Synthetic flag word | Expected widened behavior |
| ---: | --- |
| `0x00002000` | no widened effect; selector 0, original unit argument |
| `0x80000000` | no widened effect; selector 0, original unit argument |
| `0x80001000` | same widened effects as `0x1000`: selector 8, element 7 |

Synthetic threshold vectors (cost is the value produced by the cost helper):

| Flags | Unit status | Byte 6 | Current HP/SP | Cost | Trigger |
| ---: | ---: | ---: | ---: | ---: | ---: |
| `0x200` | `8` | 1 | 1 | 100 | 0 |
| `0` | `8` | 1 | 100 | 1 | 4 |
| `0` | `0` | 1 | 20 HP | 20 | 1 |
| `1` | `0` | 1 | 20 HP | 20 | 0 |
| `0` | `0` | 2 | 19 SP | 20 | 2 |

## Synthetic vectors

These vectors specify only little-endian flag-word extraction and preservation,
not gameplay behavior:

| Bytes at row offsets 0–3 | Numeric word | Preserved high bytes |
| --- | ---: | --- |
| `00 00 00 00` | `0x00000000` | `00 00 00` |
| `42 06 00 00` | `0x00000642` | `06 00 00` |
| `80 00 01 02` | `0x02010080` | `00 01 02` |

The adapter must route the described consumers while preserving all bits;
behavior in other builds is outside this audit.

## R9: passive semantic-element registration — SPECIFIED

**Scope:** the pinned Switch patch 131072 from [ai-commands.md](ai-commands.md),
with ordinary Steam 10836033 code/data corroboration. This is an admission
and reader contract, not an implemented adapter, an elemental passive-effect
catalogue, or permission to lift a boot refusal before routing its consumers.

### Sentinel semantics, not a guessed elemental affiliation

`SKILL.TBL` block 0 has 624 two-byte rows. Its first byte is a **signed**
element discriminator. For every passive-domain ID 472..623 in the measured
Steam bank, the row is `FF 01`: 152 first bytes equal 255 and 152 second
bytes equal 1. Those are aggregate measurements, not shipped payloads.
They say neither that every slot has a real named skill nor that each has
an implemented passive effect. The explicit active/passive split remains
472; do not classify a filled/blank slot solely by the element byte.

The calculated element accessor `0x71000729B0` first handles a weapon-derived
element only for a **non-null unit, ID below 472 and SKILL1 flag 2**. All
other cases read a signed byte at `SKILL0 + 2*low16(id)`, ending at
`0x7100072A18`. Therefore a passive `FF` means **-1**, not physical 0,
special/analysis 18, or unsigned element 255. No SKILL1 row exists for these
passive IDs and its flags must not be read speculatively. The result is
independent of actor, enemy/persona type and equipment.

**CORROBORATED:** Steam's ordinary element accessor `0xD3E80`–`0xD3F30`
has the same non-null/active-ID-below-472/flag2 override and signed fallback
at `0xD3F27`, reading the element pointer at `0xEC0940`. Its enemy branch
reads metadata `+0x38`; its party branch reads weapon item `+0x2C`. Steam
`0xD3D20`–`0xD3D44` performs the same signed-index-plus-one bit conversion.
These ranges were read with explicit `.shared` section guards; protected
entry instrumentation callees were not followed. No protected Steam helper
is needed to establish the passive signed-sentinel return.

PS2 `datCalcSkillElement` at `0x0023D8E0`
(`src/Main/Battle/Data/datCalc.c`) has the same signed fallback, with active
cutoff 440. PS2 passive ID p is Golden p+32 when the mapping is valid;
this renumbers the **skill**, not its -1 element. Golden-only passive skills
are not assigned an element by subtracting 32 or borrowing another skill's
active row.

Switch `0x7100072A50` interprets the signed discriminator as an element-set
bit: `1 << (signed16(element)+1)`, so -1 produces sentinel bit 0, **1**,
whereas physical 0 produces bit 1, **2**. The corresponding PS2 function is
`datCalcElementBit` (`0x0023D6E0`). This difference is observable by callers;
“no element” must not be rewritten as physical. Affinity
`0x71000771D0` returns zero for index -1 before dereferencing a unit; indices
16..18 return 100 instead. Thus the UI's previously documented inheritance/
group bucket 18 for `FF` is **not** evidence for registering element 18.
A passive whose name refers to fire, ice, healing or an ailment still has
the no-element discriminator here; its actual effects are separately keyed
skill-presence/equipment consumers, not inferred from its name.

### Native registration contract

The semantic value for a passive row whose raw byte is `FF` is the distinguished
**NONE / -1** discriminator. At the existing unsigned32 `element_id` transport
seam this has bit pattern **`0xFFFFFFFF`**; the legacy return is signed -1
and the serialized legacy first byte is `FF`. It is a sentinel, not an
ordinary affinity leaf and not an unregistered element ID 255. Admission must
recognize it explicitly for this passive case rather than require a fabricated
registered damage/status leaf. If a registry uses a tagged representation,
its NONE tag must preserve these same legacy observations. That representation
choice is adapter work, not an assertion that Golden itself has such a registry.

For this cohort register `active=false`, preserve the second byte unchanged,
retain the real source-backed skill/name/help identity, and route all semantic
element reads to NONE. Do not fabricate an active metadata row, effect asset,
attack delivery, damage/status channel binding, affinity row/column or elemental
knowledge entry merely to satisfy declaration validation. In particular,
`game_skills.c` currently requires `p4_game_tables_find_element` for every
authored descriptor; this spec requires explicit passive-sentinel admission,
not a silent fallback through that lookup. The adapter is not implemented here.
An unrecognized byte or another build remains subject to its own source/profile
qualification; do not broaden NONE admission to arbitrary unknown u32 IDs or
automatically assign -1 to every authored inactive skill.

An implementer must distinguish:

| Quantity | Required value for this measured `FF` passive case |
| --- | --- |
| Native semantic discriminator | NONE, signed -1; unsigned transport `FFFFFFFF` |
| Raw parallel-table element byte | `FF` |
| Parallel-table second byte | original value (1 in this measured cohort) |
| Element-set conversion | sentinel mask `00000001`, not physical mask `00000002` |
| Native affinity query for this discriminator | 0, including bit29 clear |
| Active/cast eligibility in 8471 | excluded from learned-skill scan (IDs >=472), even if owned |
| Membership, max-stat and other passive effects | retain actual passive ID and use their explicit effect consumers |
| Presentation/inheritance group | separate namespace; group18 does not change the discriminator |

### Exhaustive Switch reader enumeration

The table-pointer slot is `0x7100885650`, GOT alias `0x71008806D0`.
Whole-text ADR/ADRP/CFG propagation and all 25,109 relative relocations
identify nine code-reader chains. Every final table read is `LDRSB` of byte
zero with a two-byte index; no table-row pointer is returned, stored or passed
to a hidden reader. No unsupported tainted-pointer operation or unresolved
indirect boundary remains in this root receipt.

| Final signed-byte read | Immediate role in its containing calculation |
| --- | --- |
| `0x7100072A18` | calculated skill element fallback |
| `0x71000743C0` | element and element-set handling in skill calculation |
| `0x7100076478` | element-6 comparison in status/skill result calculation |
| `0x7100076974` | secondary element/modifier calculation |
| `0x710007809C` | element-6 special-case calculation |
| `0x7100078CBC` | target evaluation element/element-set handling |
| `0x7100079C94` | target-side element/element-set handling |
| `0x710007A1B8` | secondary target element/element-set handling |
| `0x710007A960` | small-element classifier |

These direct inlines must preserve signed -1 just like the named accessor;
changing only that accessor is insufficient. Their complete unrelated damage
formulas are not new passive effects: passive IDs do not acquire active rows.
Four other aliases of the slot at `0x710071C1A8`, `0x710071CBE8`,
`0x710071D628`, `0x710071E0A0` are language-specific battle-table loader
destinations. The same descriptor loader `0x7100058200` -> `0x7100058AA0`
specified for SKILL1 assigns/copies them; they are not semantic readers.
No byte-1 semantic reader occurs in these table-root chains. Preserve that
byte without inventing an effect from its constant value.

Raw census and measurement receipts are `ai_skill0_census.json` and
`ai_passive_receipt.txt` under the external Switch notes directory. They cover
the complete `0x532430`-byte text, not a bounded cross-reference window. This
closes R9's static semantic ambiguity, not passive effect implementation,
UI/help conversion, or any other build's behavior.

### Synthetic registration vectors

Inputs here are invented; the raw bytes illustrate the contract, not extracted
skill rows. Existing source/name/help admission is assumed valid where needed.

| Input | Expected observation |
| --- | --- |
| ID600, inactive, parallel bytes `FF 01`, non-null enemy with arbitrary weapon element3 | semantic -1 / transport `FFFFFFFF`; raw `FF 01`; element-set mask1; no active metadata read |
| Same ID/bytes, null unit | same -1; weapon branch not entered |
| ID600, bytes `FF A5` | same -1 and preserve auxiliary `A5`; no inferred auxiliary effect |
| ID600 with semantic registration18 but raw `FF` | mismatch: special/analysis is not NONE; do not accept as an equivalent reconstruction |
| ID600 with semantic registration0 but raw `FF` | mismatch: physical mask2 differs from sentinel mask1 |
| ID600 with transport255 but raw `FF` | mismatch: unsigned255 is not signed-byte -1 |
| Valid original PS2 passive p, Golden p+32, both raw `FF` | distinct skill IDs; element remains -1 on both sides |
| Enemy with passive600 only and 8471 element -1 | passive does not enter either active candidate list |
| Index -1 affinity request, even with null unit | zero before unit dereference |
| Active ID10, flags2, valid unit weapon element3, raw parallel `FF` | calculated element3; active override has precedence |
| Active ID10, flags0, raw `FF`, null or non-null unit | fallback -1; this read alone does not turn an active ID into a passive |
| Authored unknown full-width element ID, inactive flag alone | no permission to coerce to NONE or alias to a retail element |

