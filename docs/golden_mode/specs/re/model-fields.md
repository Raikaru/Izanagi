# MODEL0: motion extension and observed tail access

**R3 and P12 SPECIFIED.** R3 uses the raw-backing preservation rule
below; P12 uses the total type/ID forwarding, constructor, registered
dispatch and asynchronous-load contracts below. Exact names and physical
units of presentation fields belong to the native M8 layer. This does not
implement an adapter, establish PS2 asset compatibility or lift a boot gate.
Golden addresses are PE RVAs; retail addresses are PS2 virtual addresses.

The Golden table descriptor at `0xB03420` names MODEL.TBL and associates a
0x106A-byte block with the pointer at `0xEC0948`: 11 records of 382 bytes.
The adjacent descriptor also associates a 0x14D80-byte block with the pointer
at `0xEC0918`: 368 records of 232 bytes, matching MODEL1's measured shape.

At `0xCBD67`–`0xCBDAF`, the party-unit initializer selects its MODEL0 row
with stride `0x17E`, reads the `u16` at +0x14 as scale in hundredths, assigns
the motion-record pointer to row +0x3C, and assigns motion count `0x20` (32).
This independently establishes the five added 10-byte motion slots after the
PS2's 27 slots. The PS2 `BtlUnitPartyRecord` and initializer at
`0x0019E9F0` use the same bounds, scale and motion-array start, but the PS2
row is 332 bytes and has only 27 slots. Golden rows therefore contain:

| Offset | Extent | Observation |
| ---: | ---: | --- |
| `0x00` | 20 | two ten-byte bounds, consistent with PS2 |
| `0x14` | 2 | unsigned scale percent in initializer |
| `0x16` | 4 | includes bytes used by consumers; do not call all padding |
| `0x1A` | 34 | PS2-correlated signed hit intervals |
| `0x3C` | 320 | 32 ten-byte motion records; first 27 correspond structurally to PS2 |
| `0x17C` | 2 | trailing bytes after all 32 motion slots |

The PS2 record type refines the structural comparison, but is not by itself
evidence of every Golden consumer (**OPEN** where indicated):

| Offsets | Retail interpretation / comparison |
| --- | --- |
| `0x00`–`0x13` | Two ten-byte bounds records. Each has signed 16-bit X, Y, Z followed by unsigned 16-bit height and radius. PS2 `0x0019E9F0`, Switch `0x710006F000`, and Steam `0xCBDCD`–`0xCBE2A` initialize the first record with these widths (**CORROBORATED**); Switch `0x7100068330` and Steam `0xC35D4` use signed XYZ in scaled transformed positions. Units are **OPEN**. |
| `0x14`–`0x15` | Unsigned 16-bit scale, hundredths; Steam initializer `0xCBD95` and Switch `0x710006F04C`–`0x710006F088` both divide by 100 (**CORROBORATED**). Representable raw range 0–65535, not a claim that every value is valid content. |
| `0x16`–`0x17` | No conventional direct field read was observed; an unchecked interval read from a preceding row can reach these bytes. Preserve them verbatim in the contiguous backing. Steam 10836033 has little-endian halfword value 9 in all 11 rows. |
| `0x18` | Unsigned byte in the observed Golden consumers; range 0–255. Branch-dependent rule below; units **OPEN**. |
| `0x19` | No conventional direct field read was observed; an unchecked interval read from a preceding row can include it. Preserve verbatim. |
| `0x1A`–`0x3B` | Seventeen signed 16-bit entries, representable range −32768–32767. PS2 party timing accesses them by motion class and hit ordinal. Switch `0x710006B0C4`–`0x710006B120` uses the corresponding entries as detailed below (**SWITCH-ONLY**); validity of content ordinals and physical units remains **OPEN**. |
| `0x3C`–`0x17B` | Thirty-two ten-byte motion records; the observed halfword-level uses are detailed below. |
| `0x17C`–`0x17D` | First byte is accessed as kind; the unchecked interval index can also read both bytes as one signed halfword, as described below. |

### Consumer classification and raw backing

The classification concerns observed Switch readers. A preserved byte
need not have a semantic name if the adapter retains the original backing
and the native reader accesses it unchanged.

| Field | Classification and handling |
| --- | --- |
| `+0x00`–`+0x13` | **CORROBORATED, behavioural and presentation:** bounds are copied into unit state and also used in transformed positions. Preserve both complete signed-XYZ/unsigned-dimension records and pass model presentation through to M8. |
| `+0x14`–`+0x15` | **CORROBORATED, behavioural and presentation:** scale initializes unit state and participates in motion position calculations; preserve the unsigned scale and conversion. |
| `+0x16`–`+0x17` | **SWITCH-ONLY, behavioural through unchecked cross-row indexing:** no conventional direct consumer observed; preserve the bytes and row adjacency verbatim. |
| `+0x18` | **CORROBORATED, behavioural:** supplies the conditional base skill value and can propagate into battle result count. It cannot be dropped as presentation data. |
| `+0x19` | **SWITCH-ONLY, behavioural through unchecked cross-row indexing:** no conventional direct consumer observed; preserve the byte and row adjacency verbatim. |
| `+0x1A`–`+0x3B` | **SWITCH-ONLY, behavioural and presentation:** signed hit-interval inputs feed the motion hit-timing calculation. Preserve all entries and implement the unchecked indexing below. |
| `+0x3C`–`+0x17B` | **CORROBORATED, behavioural and presentation:** motion data supplies both timing calculations and model/animation setup. Preserve all 32 complete records; known choreography can be passed unchanged to the native M8 model layer, but this does not replace the PS2-shaped timing consumer. |
| `+0x17C` | **CORROBORATED, behavioural and presentation:** kind affects the support-skill predicate and presentation resource selection. Preserve and implement the behavioral predicate. |
| `+0x17D` | **SWITCH-ONLY, behavioural through unchecked interval indexing:** preserve verbatim; `0x710006AFA0` can read it as the high byte of the timing numerator. Whether normal game content reaches it is **OPEN**. |

**OPEN, Steam data census only:** byte `+0x18` ranges from 1 to 4 across
the eleven rows; `+0x19` and `+0x17D` are zero in all eleven, while
`+0x17C` ranges from 0 to 6 and is nonzero in six rows. Observed values do
not prove padding, legal ranges in other builds, or consumer semantics.

### Switch direct-pointer and derived-reader census

The whole-image CFG worklist in raw helper `pointer_flow4.py` supersedes
the earlier eleven-instruction scan. It enumerates all text, the exact
dynamic RELA table, register copies, indexed/multiply-add addresses and
stack spills, with no instruction-distance cutoff. MODEL0 GOT
`0x71008806C8` has ten load sites:
`0x71000683A0`, `0x7100068570`, `0x7100068674`, `0x71000687E4`,
`0x710006B0C4`, `0x710006F04C`, `0x71000787AC`, `0x7100078884`,
`0x710007A424`, and `0x710007AC28`. MODEL1 GOT `0x71008805A8`
has 27. The final row reads number 20 for MODEL0 and 38 for MODEL1.
Both have exactly two kinds of pointer escape: bounds returned by
`0x71000684A0`, and motion-array pointers stored by `0x710006F000`
in unit `+0xC50`. There are no other row-pointer stores or argument
escapes, and no unhandled tainted-pointer operation in that receipt.
The loader and address-form closure is shared with
[`effect-fields.md`](effect-fields.md#switch-whole-image-pointer-and-reader-census).

The relocated root slots and descriptor destinations are:

| Table | Pointer slot | Four descriptor destination fields |
| --- | --- | --- |
| MODEL0 | `0x7100885668` | `0x710071C958`, `0x710071D398`, `0x710071DDD8`, `0x710071E850` |
| MODEL1 | `0x7100885680` | `0x710071C968`, `0x710071D3A8`, `0x710071DDE8`, `0x710071E860` |

The complete MODEL1 GOT-load census is:

| Sites |
| --- |
| `0x710000A7C0`, `0x710000A7F8`, `0x710000A8CC`, `0x710000B5FC`, `0x710000F15C`, `0x7100010508`, `0x71000117E4` |
| `0x7100011B10`, `0x7100012224`, `0x7100012678`, `0x7100013E3C`, `0x7100015120`, `0x71000193B8`, `0x710001B48C` |
| `0x710004B1A8`, `0x710004C3F0`, `0x71000683E0`, `0x71000685AC`, `0x71000686B4`, `0x7100068814`, `0x710006AF84` |
| `0x710006B084`, `0x710006C464`, `0x710006F104`, `0x71000788C8`, `0x710007B27C`, `0x7100086480` |

The twelve unit-motion pointer loads listed below were each followed to
their final record-field loads by `field_flow4.py`: none stores the
motion-array address elsewhere or passes it onward. This closes the saved
`+0xC50` alias for both MODEL0 and MODEL1, rather than treating the GOT
census as the complete reader set.


The ten direct GOT sites resolve to these row accesses on the genus-0 /
non-enemy paths (**SWITCH-ONLY**; other genera use their own tables):

| Function | MODEL0 row access or pointer escape |
| --- | --- |
| `0x7100068330`, `0x71000685F0` | Signed XYZ from the first bounds record (`+0`, `+2`, `+4`); values are scaled and transformed. |
| `0x71000684A0` | Returns a pointer to first bounds `+0` or second bounds `+0x0A`; the two direct recipients are traced below. |
| `0x7100068770` | Signed XYZ from the second bounds record (`+0x0A`, `+0x0C`, `+0x0E`), scaled and transformed. |
| `0x710006AFA0` | Signed hit interval `+0x1A + 0x12*class + 2*ordinal`; it also reads the saved motion pointer. |
| `0x710006F000` | Reads first bounds and scale, stores the motion-array pointer; flow below. |
| `0x7100078770`, `0x710007A350` | Read the kind byte at `+0x17C`. |
| `0x7100078840`, `0x710007AB40` | Read the skill base-value byte at `+0x18`. |

These direct non-indexed paths do not conventionally read `+0x16/+0x17`,
`+0x19`, or `+0x17D`. The unchecked interval calculation can nevertheless
reach all of them, including across row boundaries, as shown below.

**SWITCH-ONLY, initializer pointer flow:** On the genus-0 branch,
`0x710006F04C`–`0x710006F088` loads the unsigned scale halfword,
stores `row+0x3C` as unit `+0xC50`, and stores count 32 at `+0xC34`.
The common continuation `0x710006F154`–`0x710006F1A0` reads the five
first-bounds halfwords and copies their converted values to the unit; it
does not pass the row pointer onward. Its tail call receives the unit and
unit-relative `+0xC80`, not the row pointer. Thus this initializer's
lasting row alias is the motion-array pointer.

The direct `+0xC50` load census in battle code is
`0x7100067FE8`, `0x710006A084`, `0x710006A4B0`, `0x710006A648`,
`0x710006A704`, `0x710006ACC4`, `0x710006ADE4`, `0x710006AF18`,
`0x710006B0AC`, `0x710006B0EC`, `0x710006DD70`, and `0x710006DE50`.
The `0x710006B0AC` load belongs to the enemy branch and points to MODEL1,
not MODEL0. The other sites are the direct accesses discussed in the
motion section below. The full-text split-base worklist
`formatter_flow4.py` follows every immediate ADD seed to overwrite/exit,
not just nearby loads. Its six extra `+0xC50` candidates are non-unit
global-storage accesses (`0x710034392C`, `0x71003439DC`,
`0x7100346C08`, `0x71003E0B8C`, `0x7100456708`,
`0x7100456960`), not additional motion aliases; the first two are based
at `0x71051C5C50`, the third in `0x71051C6F30` storage, and the last
three in SDK global storage. The register-immediate `0xC50` census
finds no separate indexed-offset materialization.

The superficially similar loads `0x71002263C4` and `0x71002263D0` are
not unit-motion readers: `0x7100220E50`–`0x7100220EB4` allocates a
separate `0x1260`-byte task-work object and registers it with the callback;
`0x710039F2D8`, `0x710039F088`, and `0x710039FBB0` carry that allocation
through task `+0x48` to the cleanup routine. The two loads access and free
that task-work member, not the unit's MODEL0 motion pointer.

**SWITCH-ONLY, indirect bounds access:** `0x71000684A0` returns a pointer
to either the first bounds record or the second at `+0x0A`. It selects the
second for argument 2 equal to 10, or equal to 2 with the unit's associated
object passing a `0x100000` predicate; enemy-specific exceptions also exist.
The returned pointer is consumed only by the two direct recipients below;
there is no static address-taken/relocated bounds-accessor entry.

**SWITCH-ONLY, two direct pointer recipients:** An ARM64 direct-call scan
finds calls to the bounds accessor at `0x710005246C` and
`0x710006A5E0`. The first recipient `0x71000523F0` requests selector 10,
reads signed Z at bounds-relative `+4`, scales it by unit scale and uses
it in a displacement applied to unit positions. The second recipient
`0x710006A3F0` copies signed XYZ and unsigned height/radius from the
selected bounds into unit motion state. The loads are
`0x7100052470` and `0x710006A5E4`/`0x710006A5F8`/
`0x710006A608`/`0x710006A618`/`0x710006A624`.
`return_closure4.py` also records the residual bounds address left in
the integer return register on a void motion-setup exit. This is not a
new stored row alias: the conservative forwarding reaches unused integer
argument registers of no-argument helpers (`0x710002BB20`,
`0x7100064320`, `0x7100277BB0`) or the floating-point math thunk
`0x7100530A70`, rather than a new bounds-field reader.

**CORROBORATED, second `+0x18` reader:** Switch `0x7100078840` and
Steam `0xD4400`–`0xD4477` return that byte for a non-enemy if the second
argument overlaps `0x6`, otherwise 1; for an enemy they read a byte from
the corresponding MODEL1 row at `+0x1A`.
This matches PS2 `datCalcSkillValueBase` at `0x0023E2F0`: a character
base skill value selected by the caller's result/slot flags. The PS2 battle
resolution caller `0x001F1030` stores it as a hit-result count-like value;
do not conflate it with the EFFECT0 inter-target delay.

**SWITCH-ONLY, caller propagation:** The battle resolution routine
`0x710004AEC0` calls the base-value accessor at `0x710004B424` for its
weapon-property skill branch, with the hit-result selector computed by
`0x7100079BA0`; it eventually stores the resulting byte in the target
record at `+0x172` (`0x710004B260`). A reflection/special-target path and
outcomes `0x100`, `0x200`, `0x400`, 2 or 4 can replace that value with 1.
This corresponds structurally to the PS2 result-count storage, not a
blanket instruction to use MODEL0 `+0x18` for every hit.

The tail byte `+0x17C` is the character-table **kind** selected by the
accessor: Steam `0xD4370` and Switch `0x7100078770` return it for
non-enemies and return zero for enemies (**CORROBORATED**). It corresponds
to PS2 `datCalcGetCharacterTableKind` (`0x0023E1F0`), whose retail byte is
at row `+0x14A`. The exact semantic names for the individual kind values
and Golden byte `+0x17D` remain **OPEN**.

**CORROBORATED, kind-based asset selection:** In one qualifying
weapon-property presentation path of Switch `0x7100011CA0`, kind 0, 1,
or 6 selects effect-resource indices `(4, 1)`; kind 5 selects `(5, 2)`;
other kinds select `(3, 0)`. Steam `0x5220D`–`0x52270` has the same
kind cases and selector pairs. Switch subsequently indexes a resource
array in battle state at `+0x1328` and queues effect packets using those
indices. These cases describe one path, not universal category names.

### Switch patch 131072 additional consumers

**CORROBORATED:** Switch `0x710007AB40` and Steam `0xD58F0`–`0xD59A1`
use MODEL0 byte `+0x18` in the skill-value selector. When the skill row's
low flag bit 1 is set, the source is a non-enemy, and the third argument
overlaps mask `6`, that byte is returned; otherwise this branch returns 1.
When the skill bit is clear, it reads minimum and maximum from skill-row
bytes `+0x13` and `+0x14`. If minimum is less than maximum it adds
`random_u32 % (maximum - minimum + 1)` to the minimum; otherwise it returns
the minimum. Switch asserts if either endpoint is zero; the Steam ordinary
code shown does not perform that assertion. The third argument corresponds
to the PS2 result/slot flags above.

**CORROBORATED, support-skill predicate:** Switch `0x710007A350` and
Steam `0xD5320`–`0xD5429` reject kind 3 or 5, then reject outcome values
other than 2 or 4. If source byte `+0x0C` has bit `0x80`, a qualifying
case returns 1 immediately. Otherwise the initial chance is 100 for an
enemy and 10 for a non-enemy; the three equipment/passive checks `0x71`,
`0x72`, `0x73` replace it with 80, 50, 0 in that order, so later matches
take precedence. The final test is unsigned random value modulo 100 less
than the resulting chance. Both routines first reject a clear skill flag
`0x2`, source flag `0x10`, target byte `+0x0E` bit `0x10`, or target byte
`+1` bit `0x20`; see Steam `0xD534E`–`0xD5385`. Steam `0xCFD50`
checks the two equipped item values and returns false for enemy sources.

PS2 `datCalcRollSupportSkillHit` (`0x00241F00`) has the same observed
kind-3/5 rejection but starts a non-enemy at 5 rather than Golden's 10.
Category names beyond those branches remain **OPEN**; they are not needed
for the specified predicate or the raw-backing rule.

| Synthetic branch input | Switch observation |
| --- | --- |
| Non-enemy, skill bit 1 set, third argument `2`, MODEL0 `+0x18 = 4` | returns 4 |
| Same, third argument `0` | returns 1 |
| Qualifying predicate, non-enemy, MODEL0 `+0x17C = 3` | predicate returns 0 before its random check |
| Qualifying non-enemy, kind 2, outcome 2, neither item value 0x71/0x72/0x73, random modulo 100 = 9 | returns 1 |
| Same but random modulo 100 = 10 | returns 0 |
| Qualifying non-enemy with both equipment values 0x71 and 0x73 | chance 0; returns 0 for every random value |

Offsets +120/+121 are the first signed 16-bit field in motion slot 6:
`0x3C + 6 * 10 = 120`. In the PS2
`btlUnitMotionHitFrames` (`0x001999F0`), the
party numerator is instead selected from the signed halfwords at `+0x1A`,
with motion-class stride `0x12` and hit-ordinal stride 2; the mapped
motion's second halfword is the playback-rate denominator in percent.
The enemy branch, not the party branch, adds a motion record's first
halfword to its hit interval. Switch `0x710006B0C4`–`0x710006B120`
independently reads a signed party interval from `+0x1A` and a signed
motion-record rate at `+2` in its floating-point timing calculation
(**SWITCH-ONLY** for this Golden calculation). The added slots 27–31 start
at offsets 330, 340, 350, 360, and 370. Retail's final two bytes correlate
with Golden +380/+381, but truncating the five extra motions is unsafe.

**SWITCH-ONLY, interval class and indexing:** For a genus-0 unit,
`0x710006AFA0` accepts low-16-bit logical motion IDs 5, 6, and 7 on
this path, returning zero for other IDs. IDs 5, 6, and 7 select interval
classes 0, 1, and 1, respectively
(`0x710006B000`–`0x710006B02C`). The signed numerator is read at
`row + 0x1A + 0x12 * class + 2 * unsigned_16(hit_ordinal)`; the
denominator is `(signed motion-record +2 / 100) * caller_rate`, and the
quotient is converted to an integer (`0x710006B0C0`–`0x710006B124`).
The mapped motion slot supplies the denominator. The function itself does
not bound the ordinal before reading. Physically, class 0 ordinals 0–8
and class 1 ordinals 0–7 fit in the 17-halfword interval region; these
are layout boundaries, **not** proof of valid caller input or a clamp.
Preserve the binary32 divisions and AArch64 `FCVTZS` conversion shown at
`0x710006B0F0`–`0x710006B124`, including their non-finite input behavior;
the physical time unit remains **OPEN**.

Synthetic local vectors with caller rate 1 and the mapped rate halfword
100: ID 5 and ordinal 2 read row `+0x1E`; IDs 6 and 7 with ordinal 2
read row `+0x30`. A signed value −4 at the selected location produces −4.

**SWITCH-ONLY, unchecked tail alias:** The table at `0x710062D790` maps
genus-0 logical IDs 5, 6, and 7 to slots 4, 5, and 6. Because the ordinal
is not clamped, ID 5 with ordinal 177, or ID 6/7 with ordinal 168, reads
the signed halfword at row `+0x17C`, including the normally separate
`+0x17D` byte. For example, tail bytes `02 FF`, mapped rate 100 and
caller rate 1 yield −254. This is a local accessor vector, not a claim
that ordinary game content reaches those ordinals. The caller
`0x7100011CA0` derives a loop count from target byte `+0x172` and can
pass the next ordinal at `0x71000127F8`; no ordinal clamp is visible at
the accessor itself. Do not classify `+0x17D` as proven unread merely
because its measured Steam values are all zero.

**REQUIRED raw-backing rule:** Retain the complete 11 × `0x17E` MODEL0
payload as one contiguous, byte-identical block in the layout used by the
native reader; do not project it to PS2 rows, split rows into unrelated
allocations, zero padding, clamp the ordinal, or substitute a synthetic
value for an out-of-row read. For row ID `r`, class `c`, and unsigned
16-bit ordinal `n`, fetch the two bytes at block-relative
`0x17E*r + 0x1A + 0x12*c + 2*n` and the following address, then interpret
them as a signed little-endian halfword. This retains reads into motion
records, tail bytes, and subsequent rows. For an access outside the
`0x106A`-byte payload, exact bytes depend on native surrounding memory;
the adapter must preserve the actual backing used by the native reader,
not invent zeros or assert an asset-derived result. The loaded payload
alone does not specify those outside bytes.

Synthetic cross-row vectors, with source row 0, logical ID 5, rate 100,
caller rate 1, and byte-identical contiguous backing: ordinal 189 reads
row 1 `+0x16/+0x17`; bytes `09 00` yield 9. Ordinal 190 reads row 1
`+0x18/+0x19`; bytes `02 80` yield −32766. These are reader vectors, not
claims that normal game content reaches the ordinals.

### Motion fields and added-slot mapping

All offsets below are relative to the start of a ten-byte motion record;
their physical row location is `0x3C + 10 * slot + offset`. The unit
initializer stores the motion-array pointer and count; later readers need
not reference the MODEL0 GOT.

| Offset | Observed use |
| --- | --- |
| `+0` | **CORROBORATED:** Signed halfword. Switch `0x710006AC20` and Steam `0xC6B9F`–`0xC6BD5` divide this value by `(signed +2 / 100) * caller rate` and convert to integer for an in-range mapped slot. Motion setup requires it nonnegative and `+4` positive before copying both into state (Switch `0x710006A3F0`, Steam `0xC67ED`–`0xC6829`). Exact units and all roles remain **OPEN**. |
| `+2` | **CORROBORATED:** Signed halfword interpreted as percentage in the timing divisors above and in motion setup (Switch `0x710006A4B0`, Steam `0xC6676`–`0xC6691`). |
| `+4` | **CORROBORATED:** Signed halfword. Switch `0x710006AD40` and Steam `0xC6C3F`–`0xC6C7C` return 0 if negative; otherwise divide it by `(signed +2 / 100) * caller rate` and convert to integer. Motion setup tests it for positivity as above. Units remain **OPEN**. |
| `+6` | **CORROBORATED:** Signed halfword multiplied by unit scale as part of a positional calculation (Switch `0x7100067FE8`–`0x7100068014`, Steam `0xC4AD2`–`0xC4AF3`). Units and all uses remain **OPEN**. |
| `+8` | **CORROBORATED:** An unsigned halfword read by Switch `0x710006AF18` and Steam `0xC5FE1`. Conditional motion setup copies it to unit `+0xC46` (Switch `0x710006A704`, Steam `0xC686C`). Interpretation remains **OPEN**. |

For a PS2-shaped consumer, `+0`, `+2`, `+4` and `+6` are
**behavioural/consumer-relevant** as well as model presentation data:
the first three enter timing or motion-state calculations and `+6` enters
the position calculation `0x7100067F40`/`0x7100067AE0`. The observed
`+8` consumers are **presentation-only** and may be passed through to
M8, subject to the packet-lifecycle qualification below. This distinction
does not certify unexamined aliases as absent.

**SWITCH-ONLY, unit-update indirect readers:** In update mode 3 at unit
`+0xC2C`, when helper `0x710038A8D0` returns zero, the routine obtains
the current animation position when the unit has a loaded model (otherwise
uses zero), maps the current logical motion, and for an in-range slot
compares that integer position with
`int(signed(record+0) / (signed(record+2) / 100))`
(`0x710006DD64`–`0x710006DDAC`). At or beyond the threshold it sets the
model animation rate to zero. An out-of-range slot uses threshold zero.

In update mode 0, when model byte `+0x42A` is 1 and the queued logical
motion at unit `+0xC44` is eligible for setup, a positive signed value
already in unit `+0xC46` is passed through. Otherwise the setup argument
starts at zero; for logical motion IDs 3, 14, 18, or 24, the routine maps
the motion and, if the slot is in range, reads the unsigned `record+8`
and passes it as the setup argument. If that mapped slot is out of range it
passes 6 instead (`0x710006DE10`–`0x710006DE64`). `0x710006A3F0`
then forwards that argument to the native model-motion operation. This is
presentation data passed through to the native M8 model layer, although its
exact name and physical units remain **OPEN**.

The direct accessor `0x710006AE70` returns unsigned `record+8` for a
loaded unit and an in-range mapped slot, and returns 6 if the unit is not
loaded or the slot is out of range. Callers at `0x7100001D40` and
`0x7100014694` pass the result into type-`0x104` motion packets
`0x710006B540` or `0x710006B410`. Their callback `0x710006B4A0`
compares the current logical motion with queued unit `+0xC44`; if setup
is needed, it supplies the stored halfword as the model-motion setup
argument to `0x710006A3F0`. Packet start and cleanup
`0x710006B480`/`0x710006B520` increment/decrement the unit's packet
counter. The mode is presentation pass-through, but packet ordering and
completion must be preserved. Synthetic local vector: unloaded unit
returns 6; loaded unit with valid mapped slot whose `+8` is 12 returns 12.

For genus 0, the logical-motion lookup table at `0x710062D790` maps motion
IDs 30, 31, 32 and 33 to slots 28, 29, 30 and 31 respectively. Its 34
entries contain no slot-27 mapping. In addition, `0x7100067F40`,
`0x710006AC20`, `0x710006AD40`, and `0x710006AFA0` override the mapping
to slot 28 for logical motion 1 when unit state bit `0x100` is set and
party ID is 1. At `0x710006A59C`–`0x710006A5C4`, motion setup
overrides the mapped index to slot 27 when unit motion-state `+0xC28`
has bit `0x40`, then passes that index to the model-motion routine.
`0x710002C650` and `0x710005D700` conditionally set this bit while walking
the unit list at global battle state `+0x1D0`, and then request logical
motion 0; the first path skips that request for units rejected by a
predicate. Other writers and the slot's semantic name remain **OPEN**.

Synthetic local vectors: slot 30 starts at row offset 360; record `+0 = 12`,
`+2 = 50`, caller rate 1 yields integer 24 in `0x710006AC20`. With `+4 = -1`,
`0x710006AD40` returns 0 before dividing.

## Enemy model path formatting

For a valid nonnegative enemy ID, the unit initializer at `0xCBC50` stores
the ID at unit `+0xA4` and, when the unit's genus byte `+0xA2` is 1, indexes
MODEL1 by that same ID with stride 232 (`0xCBCEB`–`0xCBD45`). A standard
model-load call at `0xB39A0` passes unit `+0xA4` to the packet constructor.
The constructor at `0xC8CC0` ordinarily assigns resource type `genus + 1`
and the passed ID, so the enemy resource type is 2 and its ID is the MODEL1
row index on this standard path. It has an observed exception: when global
battle flags include `0x200000`, the unit genus is 1, and its stored ID is
`0x111`, it instead selects resource type 2 and model ID `0x110`
(`0xC8D44`–`0xC8D7A`). Other calls can pass explicit IDs, so this rule does
not replace those call sites or prove every boss transformation.

The statically readable resolver at `0x252C5CB0` (entry thunk `0x3EA8C0`)
uses a type-dispatch table at `0x3EAA34`. For resource type 2 it formats the
low 16-bit numeric identifier as uppercase hexadecimal with at least three
digits, using the prefix `model/` and format `enemy/em%03X`; it then appends
`.AMD` (`0x252C5D63`–`0x252C5D7C`, `0x252C5DEB`–`0x252C5E23`).
Thus an explicitly supplied type-2 identifier `0x12` formats as
`model/enemy/em012.AMD`. This is the logical model name; the archive-loading
branch described below and actual PS2 asset compatibility must still be
handled. Equal MODEL1 parameter rows alone do not establish asset equality.

An enclosing wrapper at `0x3EAA90` may instead request a pack path: when its
callback selects the pack branch, it formats the AMD path, replaces the last
three extension characters with lowercase `pac`, takes the basename including
its preceding separator, and prefixes it with `model/pack`
(`0x3EAAE6`–`0x3EAB64`). For type 2 and identifier `0x12`, that branch
produces `model/pack/em012.pac`. The registered Switch branch and
section selection are specified below; equal parameter rows do not alias IDs.

**SWITCH-ONLY, registered pack selection:** Initialization
`0x7100280D10`, called during initialization at `0x710027781C`,
registers the logical-name formatter `0x7100280D60`, pack predicate
`0x7100280F20`, and pack-name formatter `0x7100280F30` through callback
setter `0x7100385E60`. With these callbacks installed,
the predicate is true exactly for unsigned 16-bit types below 3, so type 2
takes the pack branch. For type 2, `0x7100280D60` formats
`model/enemy/em%03X.AMD` using the low 16 bits of the ID; the pack
formatter replaces `AMD` with lowercase `pac` and prefixes the basename
with `model/pack`. Consequently synthetic ID `0x12` maps to
`model/pack/em012.pac` on this registered Switch path. The full
seven-slot callback inventory and the only setter call are closed below;
the fallback branches are specified rather than assumed absent.

### Switch constructor and explicit boss load

**CORROBORATED:** Switch `0x710006BEE0` and Steam `0xC8CC0` use the
passed model ID with resource type `genus + 1` unless the boss override
applies. Switch override `0x7100005BD0` checks battle-state flag
`0x200000`, genus 1, and stored unit ID `0x111`; only if all three are true
does it override the pair to type 2, ID `0x110`, regardless of the caller's
explicit ID. The third argument is a separate 16-bit **load-flags word**,
not a motion ID: Switch `0x710006BF90` and Steam `0xC8342`–`0xC83B1`
test its bits. Do not treat values such as `0x216` or `0x7E` as animations.

**CORROBORATED:** Switch `0x7100001608`–`0x7100001618` and Steam
`0x3E174`–`0x3E182` explicitly request model ID `0x10E` with load flags
`0x216` through their respective constructors. Thus a genus-1 actor that
does not meet the override requests type 2 ID `0x10E`, not its stored MODEL1
row. The corresponding retail PS2 sequence also passes these two values
(`btlBoss` sequence around `0x0022B0B0`).

**CORROBORATED, load-flag control flow:** The constructor stores the word
unchanged in packet work `+0x0C` (`0x710006BEE0`, Steam `0xC8D36`–
`0xC8D40`). In initial processing, bit `0x200` bypasses the existing-model
identity/reuse lookup; bit `0x10` suppresses the initial formatted-path
request. These two decisions are independent (`0x710006BF90`, Steam
`0xC8333`–`0xC83B1`). The later update can still perform a lookup or request
if the unit is not marked as already loading; neither bit proves that no
asset will ever be requested. After a model is ready, bit `0x4` enables
genus-specific setup (`0x710006C070`, Steam `0xC865A`–`0xC866A`).
The boss word `0x216` sets all three bits; `0x7E` sets `0x10` and
`0x4`, but not `0x200`. The remaining flag rules follow.

**SWITCH-ONLY, additional post-ready load flags:** On the ready path,
`0x710006C1AC`–`0x710006C1B8` tests bits `0x100`, `0x40`, and
`0x20` independently. `0x100` clears model bytes `+0xE8C`–`+0xE8E`
(`0x710006C3DC`–`0x710006C3F0`). Bit `0x40` invokes unit helper
`0x7100054810` with argument zero; this computes grid-aligned extents from
the unit position and scaled radius, writes unit spatial fields, and links
its spatial entry into battle state `+0x478`. It is therefore
**behavioural / PS2-consumer relevant**, not just model presentation.
Bit `0x20` invokes `0x710006A990`, motion setup `0x710006A3F0` with
the unit's saved motion values, and `0x7100069F00`
(`0x710006C3F4`–`0x710006C438`). Both bits may act in the same update.
The latter motion work may be delegated to the native M8 model layer with
its packet ordering intact.

**SWITCH-ONLY, remaining observed ready-path flags:** Bit `0x8` clears
the alpha byte of the unit's packed color at `+0x33`
(`0x710006C108`–`0x710006C114`). The fade updater `0x7100062670`
compares this byte against a calculated alpha and writes the packed color
through `0x7100067280`; `0x7100035390` also tests whether alpha is
nonzero before its motion-position predicate. Thus alpha is presentation
data, but its dependent predicate and packet timing must remain intact.
Bit `0x1`, on the loaded-unit branch, sets model `+0x408` bit `0x20`.
Bit `0x2`, subject to the genus-specific battle state exclusions at
`0x710006C148`–`0x710006C164`, sets model `+0x408` bit 0, stores a
model resource pointer at `+0xFA0`, and writes binary32 values
approximately `0.65`, `0.65`, `1.0` to model `+0xE80/+0xE84/+0xE88`
(`0x710006C118`–`0x710006C1A8`). These observed color and model
appearance values pass through to M8, with synchronization preserved.
For the enemy/type-2 path the complete tested load-word mask is `0x037F`;
bits in `0xFC80` are retained in the work word but introduce no additional
branch behavior in these callbacks. In particular `0x80` is not a motion ID.

**SWITCH-ONLY, two additional direct arguments:** Both calls
`0x710000EBC8` and `0x710000EC20` load the selected unit's stored
halfword ID at unit `+0xA4`, and pass load flags `0x0D`
(`0x710000EBAC`–`0x710000EC20`). They remain subject to the same boss
override. Synthetic example: with genus 1, stored ID `0x111`, and battle
override flag set, either call requests type 2 ID `0x110` with load flags
still `0x0D`.

A direct ARM64 `b`/`bl` scan found Switch constructor call sites at
`0x7100001618`, `0x710000EBC8`, `0x710000EC20`, `0x7100014524`,
`0x71000145C8`, `0x71000171B0`, `0x710005C330`, and `0x710005FD58`.
The whole-image address-form and relocation census finds no address-taken
constructor entry, so there is no additional indirect constructor target.

**SWITCH-ONLY, observed direct call arguments:** `0x710005C1C0` walks up
to four party records. When its direct model lookup fails it passes that
party record's 16-bit ID with load flags `0x36` at `0x710005C330`; when lookup
succeeds it uses the returned model instead of issuing this packet.
`0x710005FCF0` walks the unit list and passes each unit's stored ID with
load flags `0x3E` at `0x710005FD58`. `0x7100014524` and
`0x71000145C8` pass the linked unit's stored ID with load flags `0x1D`
and `0x0D`, respectively. These argument choices are **CORROBORATED** at
Steam `0xB2086`–`0xB2092`, `0xB39A0`–`0xB39B0`, and
`0x54D1B`–`0x54D61`, respectively. All remain subject to the constructor's
override.

**SWITCH-ONLY, explicit spawned IDs:** `0x7100016D70` takes up to three
consecutive nonzero signed halfwords from action offsets `+0xB6`, `+0xB8`,
and `+0xBA`, stopping at the first zero. For each it creates an enemy unit,
initializes the MODEL1 record with that ID, and requests that same explicit
ID with load flags `0x7E` through `0x710006BEE0` at `0x71000171B0`.
The corresponding Steam call at `0x58899`–`0x588A5` confirms that a
caller-selected ID and load flags `0x7E` are supplied (**CORROBORATED** for
the call arguments, not yet for the source offsets or loop termination).

**SWITCH-ONLY, dependent resource packet:** The summon path next issues
`0x710006CEB0` at `0x71000171D8` with the same unit ID and flags `0x10`,
dependent on the model-load packet. This constructor stores genus+1 and
the supplied ID without invoking the boss override. However, its observed
callbacks `0x710006CF40` and `0x710006CF90` do **not** use those stored
fields to choose an asset: they operate on the unit's already-selected
model at `+0xC60`, starting or waiting for its resource at `+0xEA0`.
With `0x10` set, initial processing defers the resource check to the update
callback. Thus the dependent packet's stored explicit ID does not undo the
model constructor's override. The only direct call is the one above;
the constructor has no static address-taken or relocated entry.

### Other type-2 consumers outside the battle packet — SWITCH-ONLY

The model manager is not exclusive to the battle constructor. Its general
dispatcher `0x7100387D50` takes a type and ID and either reuses an existing
pair or requests the corresponding formatted resource. Attachment helper
`0x710038AEB0` forwards caller-supplied type and ID to that dispatcher
(`0x710038AED0`), so these generic interfaces cannot be excluded from a
type-2 census without tracing their callers.

The field-resource reader `0x71001D78A0` forwards two 16-bit fields from
each input record as type and ID at `0x71001D7AC8`, with flags 0 or
`0x30`; it also has replacement-record paths at `0x71001D7B94` and
`0x71001D7D34`. Similarly, `0x71001F4070` conditionally forwards stored
type/ID pairs at `0x71001F40AC` and `0x71001F40E0`. Thus type 2 is
possible when supplied by those records; neither path establishes a new
MODEL1 index-to-ID transformation. These are total forwarding contracts:
input records supply arbitrary unsigned 16-bit pairs, not an inferred
enemy-only domain. No content-range or upstream-provenance assumption is
needed to apply the manager to such a pair.

For the first field path, the loaded resource provides a 32-bit record
count at `+8`, record stride at `+0x0C`, and records beginning at `+0x18`.
Each ordinary record supplies unsigned type and ID halfwords at record
`+4/+6` (`0x71001D7AA0`–`0x71001D7AC8`). Type 4 with ID `0xFFFF` is
the special editor placeholder rather than an ordinary manager request.
When the later replacement-record branches qualify, they similarly pass
their selected two halfwords as type and ID; they do not run the battle
constructor's boss override. For synthetic field input `(type=2,
ID=0x111)`, the manager receives `(2, 0x111)` even if the battle override
flag is set; if the registered pack formatter is active and no existing
model is reused, the requested path is `model/pack/em111.pac`.

The two replacement sources are distinct. The first walks 32-byte
entries terminated by first halfword `0xFFFF` and passes their halfwords
at `+2/+4` as type/ID. The second passes halfwords at `+0x68/+0x6A`
of a selected record. In both paths the corresponding FBN placeholder
must have type 4, ID `0xFFFF`, and a matching marker
`(source_halfword_0 & 0x3FF) | 0xC00`; a replacement pair `(0, 0)` is
not requested. Further local filters can prevent either request
(`0x71001D78A0`, including `0x71001D7B94` and `0x71001D7D34`).
These input fields are forwarded, not inferred from MODEL1.

The manager indexes a list by the low 16-bit type and compares low 16-bit
IDs (`0x7100387D50`). On a match it creates a new model object with the
same pair via `0x7100387130`; it does not return the existing list entry
unchanged. Its model-state word gains `0x2000`, plus `0x4000` for
manager flag `1`, `0x800000` for flag `4`, `0x1000000` for flag `0x10`,
`0x2000000` for flag `8`, and `0x4000000` for flag `0x20`. On a miss,
the manager chooses the formatted or pack creation path using the
registered predicate; both constructors install the pair in the new
object and propagate the corresponding manager bits
(`0x7100387010`, `0x7100387A50`, `0x7100387130`). These manager flags
are distinct from the battle packet's load-flags word.

Synthetic manager vector: a matching entry `(2, 0x111)` and manager
flags `0x30` produce a new object retaining `(2, 0x111)` with both
state bits `0x1000000` and `0x4000000` set; no battle boss override is
applied by this manager operation.

For the two reserve slots used by `0x71001F4070`, the observed writer
`0x71001F3ED0` clears both 16-byte records, then writes type 9 and a
computed ID when it marks a slot active
(`0x71001F3F38`–`0x71001F3F8C`). Initialization `0x71001EFEF0` also
clears the slots, and `0x71001F4120` writes their separate selection
indices at offsets 8 and `0x18`, not their type/ID halfwords
(`0x71001F41DC`–`0x71001F4214`). A page-relative scan found four
references to the reserve-storage GOT entry `0x7100880C40`, at
`0x71001EFF48`, `0x71001F3EF0`, `0x71001F4084`, and
`0x71001F41DC`; its relocation points to `0x7100A1A794`. A scan for
direct address construction within those 32 bytes and a relocation scan
found no additional exact references. Thus this observed producer reloads
type 9, not type 2, at `0x71001F40AC`/`0x71001F40E0`.
The reader contract is nevertheless the actual stored unsigned pair, not
a hardcoded type-9 substitution: any changed pair follows the same generic
manager rule. Synthetic reserve record: active=1, type=9, ID=`0x123`
causes manager request `(9, 0x123)`; a stored type 2 requests type 2.

Direct `b`/`bl` targets of the manager in the scanned Switch text are
`0x710014BE74`, `0x7100196BF4`, `0x71001D7AC8`, `0x71001D7B94`,
`0x71001D7D34`, `0x71001F0968`, `0x71001F0D84`, `0x71001F3F4C`,
`0x71001F40AC`, `0x71001F40E0`, `0x71001F8E9C`, `0x7100361CF8`,
`0x7100361F88`, `0x7100362650`, `0x71003682FC`, and `0x710038AED0`.
The full text/address-materialization, absolute-word and RELA censuses
find no address-taken manager or attachment-helper entry. The complete
attachment-helper call set is `0x7100148954` with type 5 and
`0x71002810B0`/`0x7100281198` with type 7. Thus those three do not
create type-2 requests; the manager's arbitrary-pair contract still covers
its field/script/copy callers.

`0x71001F3F4C` passes constant type 9. The type reaching
`0x71001F0D84` in `0x71001F07B0` is 1 or 9 on its local branches, not 2;
the other manager call in that function passes type 1. The calls at
`0x7100361CF8`, `0x7100361F88`, `0x7100362650` and `0x71003682FC`
copy type and ID from an existing model object's unsigned halfwords
`+0x404/+0x406`, without remapping. The first takes the model directly;
the others obtain it from source-object fields `+0x98`, `+0xB8`,
and `+0xA8`, respectively. The copy paths preserve any source pair,
including type 2; a chain of copies cannot introduce a new ID mapping.
Source model identity is therefore an input, not an unresolved P12
special-case transformation.

Separately, `0x7100281240` handles a loaded model whose stored type is 2
and, unless its earlier ready checks return, allocates or reuses its
dependent resource, chooses the three-entry cache using type 2 and the
model's stored ID (`0x71002812F0`–`0x710028130C`), and requests
`model/enemy/em%03X.se` with that ID. The name is a separate dependent
`.se` resource, not an alternate `.AMD` model ID or evidence that this
request was dynamically reached. For synthetic type 2, ID `0x12`, this
format yields `model/enemy/em012.se`. The generic helper
`0x710038F220` also accepts types 1 and 2 and uses the three-entry cache
for type 2 (`0x710038F2F0`–`0x710038F320`). Its contract accepts
the supplied pair unchanged; it does not derive a replacement MODEL1 ID.

There is also an embedded-resource path: when the model-format predicate
at `0x7100387C80` is true, the constructor takes sections from the
already-supplied model resource and forwards section 0 together with the
same type and ID to `0x710038F220` (`0x7100387D14`). Thus the `.se`
file request above is not the only way this dependent resource is supplied.
The model update at `0x7100387428` likewise forwards the model's stored
type and ID to the same helper. These paths do not imply a different model
identity.

### P12 closure: dynamic inputs and every installed dispatch entry

The remaining manager callers have explicit total contracts. At
`0x710014BE74` the type is 5; at `0x7100196BF4` it is 3. The
script-facing `0x71001F8E70` obtains argument 0 as type and argument 1
as ID through `0x71002E11C0`, forwards them with manager flags `0x20`
at `0x71001F8E9C`, and hands the result back through
`0x71002E1450`. Thus a script may request `(2, n)` for any low-16-bit
`n`; there is no MODEL1-row translation or battle boss override on
that path. This, the field/replacement records, the reserve slots and
the pair-copy invariant above close every manager input family without
claiming that static analysis proves which content executes a request.

The only static call to callback setter `0x7100385E60` is
`0x7100280D48`. The setter stores all seven pointers, replacing a null
argument with the corresponding relocated default. Its indirect wrappers
at `0x7100385F30` through `0x7100385F80` read the slots shown below.
Full-text slot-reference enumeration includes paired stores, not just
single-pointer loads. No alternate writer or callback table entry
supplies a model-ID transformation.

| Global slot | Registered target | Null-argument default | Type-2 contract |
| --- | --- | --- | --- |
| `0x71051C8F40` | `0x7100280D60` | `0x7100385DF0` | Format `model/enemy/em%03X.AMD` from unsigned 16-bit ID. Default returns 1 without formatting. |
| `0x71051C8F48` | `0x7100280F20` | `0x7100385E00` | Pack predicate is unsigned 16-bit type < 3. Default returns 1. |
| `0x71051C8F50` | `0x7100280F30` | `0x7100385E10` | Pack-name conversion yields `model/pack/em%03X.pac`; default returns 1 without formatting. |
| `0x71051C8F58` | `0x7100281040` | `0x7100385E20` | Rejects non-type-1 models with result 0, so cannot attach a type-2 replacement. Default returns 1. |
| `0x71051C8F60` | `0x7100281240` | `0x7100385E30` | Ready/cache check, then same-ID enemy `.se` request as specified above. Default returns 1. |
| `0x71051C8F68` | null argument installs `0x7100385E40` | `0x7100385E40` | Target returns 1; no static slot reader. Preserve the installed pointer, no additional behavior. |
| `0x71051C8F70` | null argument installs `0x7100385E50` | `0x7100385E50` | Target returns 0; no type/ID mutation. |

The first five default targets are exhaustively accounted for even though
this initializer supplies five non-null overrides. They are literal
constant-result functions, not undocumented alternative formatters.
Separately, setter `0x710038B570` installs `0x71002813F0` in
`0x71051C8F88`. That target tail-calls `0x71002755C0`, which returns
the binary32 product of global `0x7104A4BE50`, frame-step helper
`0x7100277BB0` and `0.5`. It is a native presentation update interval,
not a resource-identity dispatch.

The battle packet's dynamic targets are likewise finite:

| Packet / header callback field | Target | Required handling |
| --- | --- | --- |
| Model load `0x111`, `+0xA8` | `0x710006BF90` | Increment unit packet counter; perform initial identity/reuse and request decisions under load bits `0x200`/`0x10`. |
| Model load `0x111`, `+0xB0` | `0x710006C070` | Wait for model and dependent resource, perform ready setup/flag effects, return completion only afterward. |
| Model load `0x111`, `+0xB8` | `0x710006C5A0` | Decrement the unit packet counter. The allocator-cleared `+0xC0` is not a fourth installed load target. |
| Dependent-resource packet, `+0xA8` | `0x710006CF40` | Increment counter; unless `0x10` defers it, start/check the selected model's resource. |
| Dependent-resource packet, `+0xB0` | `0x710006CF90` | Wait on selected model `+0xEA0`, not the stored explicit ID. |
| Dependent-resource packet, `+0xB8` | `0x710006D000` | Decrement counter; no ID remapping. |

On load completion, the enemy branch of `0x710006C070` reads MODEL1
`+0x18` and independently ORs model-state `0x200`, `0x400`, `0x800`
for row bits `0x2`, `0x8`, `0x10`
(`0x710006C478`, `0x710006C534`–`0x710006C56C`).
These are model presentation flags, not alternate asset IDs.
Steam ordinary `.shared` corroborates the same three independent tests
at `0xC88DE`–`0xC8919`, packet identity override at
`0xC8D44`–`0xC8D7A`, initial flags at `0xC8342`–`0xC83E4`,
and summon arguments at `0x58899`–`0x588A5`. Calls from this ordinary
code into protected code were not followed or decoded.

**Synthetic closure vectors:**

| Input / state | Expected |
| --- | --- |
| Script arguments `(2, 0x10111)` | Manager receives low-16 pair `(2, 0x111)`, flags `0x20`; no battle override. |
| Clone chain with source pair `(2, 0x111)`, battle override flag set | Every clone retains `(2, 0x111)`; the override belongs only to the battle-load constructor. |
| Registered type 2, ID `0x12`, pack has sections 0 and 1 | Section 1 supplies the model and section 0 the dependent resource, both retaining `(2, 0x12)`. |
| Enemy ready path, load bit `0x4` set, MODEL1 `+0x18 = 0x1A` | Model state gains all of `0x200`, `0x400`, `0x800`; selected asset ID does not change. |
| Identical model-load inputs differing only in load bit `0x80` | Work words remain distinct; no extra callback branch or animation selection follows from that bit. |

The MODEL1 raw-reader census covers bounds and scale, presentation selectors
`+0x18/+0x19/+0x22/+0x24/+0x26`, base value `+0x1A`, enemy interval
`+0x1C`, and the saved motion array beginning `+0x2A`. Preserve all
232 bytes per row and the contiguous native backing; presentation values
pass through to M8, while the specified timing/base-value readers remain
behavioral. Unchecked indexing retains raw backing, including cross-row
accesses; there is no PS2 row projection, ID deduplication, bounds clamp
or inferred asset alias. This closes P12 as a static reader/load contract,
not a claim of executed assets or completed runtime integration.


| Synthetic constructor inputs | Expected resource pair |
| --- | --- |
| genus 1, stored ID `0x100`, explicit ID `0x10E`, load flags `0x216` | type 2, ID `0x10E`, flags `0x216` |
| genus 1, stored ID `0x111`, flag `0x200000` set, explicit ID `0x10E` | type 2, ID `0x110` |
| genus 1, stored ID `0x111`, flag clear, explicit ID `0x10E` | type 2, ID `0x10E` |

## Synthetic vectors

| Input | Expected |
| --- | --- |
| motion index 6 | byte offset 120 |
| motion index 27 | byte offset 330 |
| motion index 31 | byte offset 370; ten-byte record ends at offset 379 |
| tail | first byte at offset 380, second at 381 |
| tail first byte `0x7F`, non-enemy | accessor returns `0x7F` |
| tail first byte `0x7F`, enemy | accessor returns 0 |
| resource type 2, supplied identifier `0x12` | path `model/enemy/em012.AMD` |
| standard enemy row `0x12`, no exception | type 2, model ID `0x12` |
| standard enemy row `0x111`, battle flag `0x200000` set | type 2, model ID `0x110` |
