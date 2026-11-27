# EFFECT0: specified native-reader contract

**R2 SPECIFIED.** Preserve the complete contiguous 472 × 12-byte payload.
The whole-image reader and pointer-escape census below supersedes the earlier
bounded scans. Behavioral branches are specified here; visual units and effect
choreography are deliberately opaque pass-through to the native M8 layer.
This is specification closure, not an implemented adapter or a lifted boot
gate. Golden addresses are PE RVAs; retail addresses are PS2 virtual addresses.

The Steam build's table descriptor at `0xB032D8` associates the EFFECT file
name with the pointer at `0xEC08F8` and a 0x1620-byte payload (472 rows of
12 bytes). Code at `0x96480` indexes that pointer by `12 * skill ID`, reads
byte 0, and returns the byte except that zero returns 1. The same
zero-to-one rule is used within larger consumers at `0x4E609` and `0x5E18C`.
**CORROBORATED:** Switch accessor `0x71000469A0` has the same unsigned
byte and zero-to-one rule as Steam `0x96480` and PS2
`btlUnitSlotValueOrOne` at `0x001EF4A0`. In the target loops at Switch
`0x710000FCC0` and `0x7100019F00`, it supplies the interval added to a
16-bit accumulator after each target; the accumulator is converted to
floating point and written to the next target effect packet's delay.
Steam `0x4E609`/`0x4E7A7`/`0x4F4A3`–`0x4F4AB` and
`0x5E18C`/`0x5E2E1`/`0x5E89E`–`0x5E8A8` exhibit the same rule.
The addition wraps modulo 65536. For synthetic row byte 3 and three targets,
the successive local delays are 0, 3, 6; byte 0 yields 0, 1, 2.
The physical time unit belongs to the native M8 presentation contract.

**SWITCH-ONLY:** Direct calls to the accessor occur at `0x7100003AB0`,
`0x7100010470`, `0x7100014F30`, and `0x710001A768`. The first uses the
fixed row ID `0x17B`; the others use the current skill ID. The
`0x7100013360` path substitutes interval 4 rather than the accessor value
when its local special-effect mode is active. This direct-call census does
not exclude inlined or indirect uses.

Consumers at `0x3ABA3`, `0x54790`, `0x563EC`, and `0xBB31F` index at the
same stride and test a **32-bit** word at row offset 4. Tests include masks
`0x400`, `0x10000`, `0x80000`, and `0x40000`, respectively. This proves that
flags are wider than the PS2's `u16` field, and that the two bytes at offsets
6–7 cannot be discarded solely because the PS2 lacks them. One observed
branch at `0x54790` forces a local selector to 1 when `0x10000` is set;
`0x563EC` adds bit 20 to a local mask when `0x80000` is set. Their full
presentation semantics remain unknown.

| Offset | Confirmed observation | Status |
| ---: | --- | --- |
| 0 | unsigned inter-target interval; zero resolves to 1 | specified; native M8 presentation input |
| 1–3 | no reader in the pinned Switch census; zero in measured Steam rows | preserve the value, no behavior |
| 4–7 | little-endian 32-bit flags, including upper-bit tests | behavioral rules and native M8 pass-through below |
| 8–9 | signed little-endian halfword used by timing-packet branches on both platforms | specified timing input; physical units delegated to M8 |
| 10–11 | no reader in the pinned Switch census; zero in measured Steam rows | preserve the value, no behavior |

### Consumer classification

The classification includes the direct readers and the pointer-escape closure
below. A presentation pass-through must retain packet lifecycle and ordering:

| Row field | Classification and handling |
| --- | --- |
| `+0` | **CORROBORATED, presentation:** pass the original byte to M8, preserving zero-to-one, the special interval-4 branch and 16-bit wrapping. |
| `+1`–`+3` | **SWITCH-ONLY no-reader result:** preserve verbatim, no behavior; not a license to delete or zero these bytes. |
| `+4`–`+7` | **Behavioural and presentation:** the word includes action-state controls such as `0x1000`/`0x20000` and presentation packet controls. Preserve the entire word; a blanket presentation-only classification would discard game-relevant state changes. |
| `+8`–`+9` | **CORROBORATED, presentation:** pass the signed halfword unchanged to M8, preserving the lead and per-target formulas below. |
| `+0x0A`–`+0x0B` | **SWITCH-ONLY no-reader result:** preserve verbatim, no behavior. |

Passing presentation data through does not permit a PS2-shaped consumer to
truncate behavioral flags or remove packet synchronization.

The following classifies the observed **set-bit** consumers; presentation
bits can be handed to the native M8 effect/model layer without inventing
exact visual semantics, provided packet dependencies and completion are
retained. Unnamed visual semantics do not leave R2 open.

| Flag masks | Class | Reason and evidence |
| --- | --- | --- |
| `0x1`, `0x200` | Behavioural / PS2-consumer relevant | The selector `0x710006B140` is propagated into the position-calculation and application path `0x71000530E0`; see below. |
| `0x2`, `0x4`, `0x8`, `0x80`, `0x100`, `0x800`, `0x2000` | Presentation-only in observed paths | Color, packet timing, UID dependency and effect choreography, as detailed below. Pass through with packet lifecycle intact. |
| `0x10`, `0x20`, `0x400`, `0x4000`, `0x10000` | Presentation-only in observed paths | Position/camera-presentation and motion-selector branches at `0x71000350D0`, `0x7100035130`, `0x7100008DB0`, `0x710003CE7C`, and `0x7100014180`. |
| `0x40`, `0x1000`, `0x8000`, `0x20000` | Behavioural / conservatively PS2-consumer relevant | These alter action/motion packet selection, action state or battle-state flags (`0x7100013D34`, `0x7100013A94`, `0x710000A850`, `0x710008BFB0`). Do not drop them as visual-only. |
| `0x40000` | **CORROBORATED, presentation-only in observed paths** | Excludes the actor from alpha-fade updates, not from the attack target list (`0x7100062280`, Steam `0xBB2E0`–`0xBB3D7`). |
| `0x80000` | Presentation-only in the observed packet path | Pass the resulting option through to M8; preserve `0x301` packet lifetime and dependencies as explained below. |
| Other flag bits | Preserve, no additional decoded branch behavior | Preserve the full word through M8; do not invent names, truncate it or synthesize effects from zero-valued Steam bits. |

PS2 EFFECT rows are 4 bytes (`u16 info`, `u16 flags`). The flag accessor
correspondence is supported by Golden's actual offset-4 reads and retail's
16-bit offset-2 flag consumers. The candidate byte projection
`Golden[0:2] + Golden[4:6]` matches 361 of 440 overlapping complete rows,
but truncates tested Golden flags and is not an approved conversion.

## Switch whole-image pointer and reader census

The pinned NSO's complete `0x532430`-byte text and exact dynamic RELA table
were enumerated, not an instruction window around known hits. Raw research
helpers `pointer_flow4.py`, `address_forms4.py`, `descriptor_layout4.py` and
their receipts are in `<scratch>/GoldenRE-notes/switch/`.
The CFG worklist follows register copies, indexed address arithmetic,
multiply-add forms, branches and stack spills until overwrite or exit.
The following 42 **SWITCH-ONLY** instructions load EFFECT GOT
`0x7100880588`; repeated loads in a function are retained:

| Sites |
| --- |
| `0x7100008DCC`, `0x710000A794`, `0x710000A8A0`, `0x710000FF88`, `0x7100010594`, `0x71000113EC`, `0x710001220C` |
| `0x7100013408`, `0x7100013668`, `0x7100013AE4`, `0x7100013D34`, `0x7100013E74`, `0x7100013F34`, `0x7100014180`, `0x71000141C0` |
| `0x7100014920`, `0x71000149F8`, `0x7100014E3C`, `0x7100014E6C`, `0x7100014F44`, `0x710001507C`, `0x71000151F8`, `0x71000152F4` |
| `0x7100015464`, `0x71000154D4`, `0x71000156A8`, `0x7100015724`, `0x7100015790`, `0x7100015830`, `0x71000159C8`, `0x71000169F4` |
| `0x710001714C`, `0x710001A1F8`, `0x7100035108`, `0x710003517C`, `0x7100035410`, `0x71000469A4`, `0x710004AE9C`, `0x7100062458`, `0x710006259C`, `0x710006B188`, `0x710008BFBC` |

There are 45 final row-memory reads: one unsigned byte at `+0`
(`0x71000469B8`), two signed-halfword reads at `+8`
(`0x7100014E54`, `0x7100014E90`), and 42 reads overlapping the flag
word only. In particular, `0x71000113F8`, `0x71000116A0` and
`0x7100011764` reuse a loaded pointer. No EFFECT table/row pointer is
stored, returned or passed as an argument by these consumers; loaded
scalar values and packet options are not row aliases. There are no
unhandled tainted-pointer operations in the receipt. Thus no indirect
row reader remains, and `+1..+3` and `+0x0A..+0x0B` have no reader.

The five relocated addresses of slot `0x7100885660` are
`0x710071C810`, `0x710071D250`, `0x710071DC90`,
`0x710071E708` and GOT `0x7100880588`. The first four are loader
destinations, not hidden field consumers. Selector `0x7100058200` chooses
one of four eight-record descriptor arrays and passes the EFFECT record
at array `+0x668` to `0x7100058AA0`. That routine loads the destination
slot at descriptor `+8`, writes the backing pointer at
`0x7100058B78`, and copies the file block at `0x7100058B94`;
its subsequent descriptor-slot loads advance by `0x10`. No descriptor
callback dispatches a row reader. Full-image absolute-pointer and
literal-load checks and the high-base MOVK census found no alternative
materialization of these roots. This closes the loader aliases as well
as the direct consumers.

**Additional behavioral `0x1` reader:** `0x710004AE50` returns
`flags & 1` only when argument 3 is nonzero, skill `+5` equals 1 and
the associated PERSONA row's bit `0x10` is clear; otherwise zero
(`0x710004AE9C`–`0x710004AEB0`). Preserve this predicate as well as
the selectors below.

**Additional `0x8000` uses:** Within the qualified path of
`0x71000113C0`, `0x71000116A4` tests the current row's bit before
the linked-unit branch; `0x7100011768` selects logical-motion argument
7 when set, versus 5 on the qualifying clear-bit alternative, for
`0x7100067F40` at `0x7100011788`. These are native motion/position
choreography, not another table or pointer escape. Retain the original
branch ordering when forwarding the action to M8.

### Additional observed flag branches — SWITCH-ONLY

These are local effects, not complete names for the bits:

| Mask | Observed rule |
| --- | --- |
| `0x1` | At `0x710000A710`–`0x710000A83C`, after earlier checks succeed and the skill's low bit `0x2` is clear, a clear EFFECT bit makes the predicate return 0; a set bit continues to genus-dependent checks. At `0x710006B140`–`0x710006B1C0`, on the effect-enabled path with bit `0x200` clear, the result is 2 exactly when this bit and the fourth argument are both nonzero, otherwise 1. |
| `0x2` | At `0x71000149F8`–`0x7100014A1C` and `0x710001A1F8`–`0x710001A21C`, the bit is passed as 0/1 to the color-transition packet constructor `0x7100060F10`, which stores it as a mode at work offset `+0x68`. In its update callback `0x7100061030`, when the enclosing battle-state and initial-frame conditions allow the update, active units in four global lists receive model bytes `+0xE8D/+0xE8E` of `0x92/4` for mode 0 and `0/6` for mode 1 (`0x7100061198`–`0x71000612D8`). Other packet behavior and display interpretation remain **OPEN**. |
| `0x4` | In the target-packet path, `0x7100014F44`–`0x7100014F64` chooses local extra delay 0 when set, 30 when clear. In later conditional target branches, `0x7100015724` and `0x7100015830` select packet dependency kind `0x0B` and the preceding effect packet's UID when set, versus kind 5 and the alternate saved UID when clear. Flag `0x800` overrides this choice on its own branch. |
| `0x8` | On the qualifying target path at `0x71000152F4`–`0x7100015314`, setting this bit writes delay 8.0 to a secondary effect packet. A separate path at `0x71000159C8`–`0x71000159E8` adds 8.0 to its packet delay. These are conditional branches, not unconditional per-skill changes. |
| `0x10`, `0x20` | `0x71000350D0` returns 0 if the associated child record has bit 0, otherwise returns the inverse of EFFECT bit `0x10`. `0x7100035130` returns 1 immediately if global battle byte `+0x0E` has `0x20`; otherwise, if that child test is clear or absent, it returns 1 exactly when `(flags & 0x30) == 0x20`. |
| `0x40` | On qualifying action paths `0x7100013D34`–`0x7100013D48`, `0x7100013F34`–`0x7100013F64`, `0x71000169F4`–`0x7100016A3C`, and `0x710001714C`–`0x7100017164`, setting this bit suppresses particular queued action or motion packets. The branches have independent surrounding prerequisites. |
| `0x80` | Several effect-packet paths including `0x710000FF88`–`0x710000FFA0` and `0x7100014EC8`–`0x7100014EDC` set packet `+0xA0` to the current action UID only when this bit is set. Other sites use the bit for packet dependency selection. |
| `0x100` | At `0x7100013668`–`0x71000136E4`, when either of two surrounding action branches creates packet type `0x504` or `0x505`, this bit sets that packet's floating-point `+0x88` to 3.0 rather than 1.0. On the later qualified path `0x7100013E74`–`0x7100013EC0`, set skips the branch that may create a motion packet and instead creates a timing packet. Branches can skip creation; units and full presentation semantics are **OPEN**. |
| `0x200` | At `0x710006B140`–`0x710006B1F0`, when the effect-enabled path is reached, this bit takes precedence over `0x1`: the result is true exactly when argument 4 is zero and global battle byte `+0x0E` has bit `0x20`; otherwise it is false. |
| `0x1000` | In the qualifying alternative at `0x7100013AE4`–`0x7100013AFC`, a set bit enters the branch that creates a packet using `0x710006ED10`, sets battle-state `+0x0C` bit `0x400000`, and ORs 6 into the halfword at battle-state `+0x1C`. It does not establish that those effects occur for every action. |
| `0x2000` | At `0x7100015464`–`0x71000154AC`, on the applicable target packet with no earlier alternate dependency, this bit selects the saved UID for the first dependency; clear instead installs two dependency UIDs and sets the second dependency kind to 4. |
| `0x4000` | `0x7100035400` returns this bit as a Boolean. At `0x710003CE7C` and `0x710003D400` its result selects between two paths constructing presentation-position vectors; the set path also distinguishes skill IDs `0x4E`, `0x38`, and `0x35` for its numeric parameters. At `0x710003B7E4` its result participates in another conditional selection with the `0x10`/`0x20` predicates. These local effects are **SWITCH-ONLY**, not proof of all downstream uses. |
| `0x8000` | At `0x710000A850`–`0x710000A8BC`, a non-enemy of genus 0 whose MODEL0 tail selector is 3 returns 1 if the third argument is zero **or** this EFFECT bit is set; selector 5 returns 1 independently. Other cases take their respective paths. |

**SWITCH-ONLY, `0x1000` branch prerequisites:** In `0x7100013360`, let
`g` be the actor's genus, replaced by −1 when action byte `+0x1D` bit 0
is set. Let `q` be 1 exactly when the skill's low flag `0x2` is clear
and action word `+0x20` has bit `0x10`; otherwise it is 0. The branch
at `0x7100013AE4` is reached after the earlier alternatives
`g != -1 && q == 1`, `g == 1 && q == 0`, and `g == 0 && q == 0`
have not been taken. In that remaining branch, set EFFECT `0x1000`
creates the packet and performs the two battle-state mutations in the
table; clear does not perform those particular mutations there. Other
branches can mutate the same state independently.

The actor-header override is byte1D mask01, equivalent to header word1C
mask0100. Its native binding is `BtlAction.flags` at PS2+18 mask0100,
**not** the DB/resultFlags byte. Producer/reset evidence and the Golden
consumer distinction are in [effect-target-state.md](effect-target-state.md).

For `0x8000`, `0x71000113C0` exits immediately when the bit is set,
before its packet-creation path. Separately, `0x710008BFB0`–`0x710008BFEC`
sets an action halfword `+0xAC` to 2 if another predicate is false or this
bit is set, and to 1 otherwise, then writes the selected skill ID at `+0xAE`.
The higher-level meanings of these actions are **OPEN**.

**CORROBORATED, `0x8000` and character kind:** The `0x710000A850`
selector branch in the table is also present at Steam
`0x47C95`–`0x47CE4`: kind 5 returns 1; kind 3 returns 1 if the
third argument is zero or the current EFFECT row has bit `0x8000`.

**SWITCH-ONLY, row-zero exception:** In the distinct packet-selection path
at `0x710001220C`–`0x7100012254`, a genus-0 actor with MODEL0 tail selector
3 and a local target predicate set checks **EFFECT row 0**, not the current
skill row: row-0 bit `0x8000` set selects argument `0x0C` for the packet
constructor at `0x7100035610`; clear selects `0x0B`. If the local predicate
is clear it selects `0x0C` regardless of that bit. This use must not be
silently rewritten as a current-skill lookup. Steam equivalence is **OPEN**.

Synthetic branch vectors: effect-enabled path with flags `0x1` and fourth
argument 7 returns 2; flags `0x201`, fourth argument 7 return 0 regardless
of global bit `0x20`; flags `0x200`, fourth argument 0 and global bit
`0x20` set return 1. For the other predicate, selector 3, third argument 1,
and flags `0x8000` return 1; clearing that bit returns 0 on that branch.
For `0x7100035130`, with the global bit clear and no child exclusion, flags
`0x20` return 1, while `0x30` return 0; with the global bit set, either
returns 1.

**SWITCH-ONLY, selector propagation:** The result of `0x710006B140` is
not merely discarded: `0x71000530E0` passes it to the position-calculation
helper `0x7100067AE0`, then applies the resulting vectors through
`0x7100067250` and `0x7100067230`. A separate caller in
`0x71000525C0` also uses the result for the position helper. Preserve this
selection as potentially behavioural for a PS2-shaped consumer until the
full movement/position consequences have been traced.

**CORROBORATED, `0x400`:** Switch `0x7100008DB0`–`0x7100008DEC`
clamps a computed floating-point distance-like quantity to at least 200 and,
for skill ID below 472, adds 150 when this flag is set. Steam `0x3ABA3`–
`0x3ABB5` tests the same mask and conditionally adds the binary32 constant
150 from RVA `0x965480`. The exact world/display units and full camera
semantics are **OPEN**. Synthetic local vector: an already-clamped value 200
becomes 350 when the bit is set and stays 200 when clear.

**CORROBORATED, `0x10000` and `0x4000`:** In the qualified action path,
the first bit forces a local motion selector to 1 when the preceding helper
returns zero; the second also forces that selector to 1 regardless of that
helper (`0x7100014180`–`0x71000141E4`; Steam `0x54790`–`0x547EB`).
Other checks can force the same selector independently.

**CORROBORATED, `0x80000`:** The target path combines this bit, shifted left
one place, into bit `0x100000` of the effect packet's options before
calling its constructor `0x7100048890` (`0x710001507C`–`0x7100015094`,
`0x7100015104`–`0x71000151B8`; Steam `0x563EC`–`0x563F6`). The constructor stores
the options in work `+0x34`. Its exact visual interpretation is delegated
to M8; the option must not be omitted merely because no local callback tests it.

**SWITCH-ONLY, packet callback narrowing:** The `0x7100048890` constructor
installs callbacks at `0x7100048920`, `0x71000489A0`, `0x7100048BB0`,
and `0x7100048B50`. In the update callback, the options word at `+0x34`
is tested for `0xC00` and its byte `+0x36` is tested at bit positions 2
and 3 (`0x7100048A28`–`0x7100048A34`,
`0x7100048A78`–`0x7100048A7C`, and
`0x7100048AF4`–`0x7100048AF8`). These tests are respectively
`0x40000` and `0x80000` in the packet word, **not** its `0x100000` bit
derived from EFFECT `0x80000`. The other three callbacks do not locally
test the options word. This is not a claim that the option is unused:
the complete work/options payload remains native M8 pass-through.

**SWITCH-ONLY, packet ownership and synchronization:** The allocator
`0x710005ADD0` makes the `0x58`-byte work area part of the packet and
stores its pointer at header `+0xC8`. Enqueue `0x710005AE80`, update
scheduler `0x710005AF30`, and dependency evaluator `0x710005B510` pass
that pointer to the installed callbacks; the traced framework does not
locally interpret work `+0x34`. The sibling constructor `0x7100048C00`
uses the same packet type and callbacks. Code at `0x710000B130` and
`0x710000F888` counts packets of type `0x301` via `0x710005AC90` and
defers action progression while any remain. Therefore the native M8
effect layer may receive the option as presentation pass-through, but its
packet creation, completion and dependencies must still be preserved:
presentation timing can gate behavioral progression. A packet-work pointer
is not an EFFECT row alias and must remain owned by that native lifecycle.

The complete installed type-`0x301` dispatch is the following. Both
constructors `0x7100048890` and `0x7100048C00` install this same set;
there is no unexplained table entry or row pointer in their work payload.

| Header field | Target | Native lifecycle contract |
| --- | --- | --- |
| `+0xA8` | `0x7100048920` | Start/reset effect time, acquire the selected unit/resource effect record, increment both unit packet counters. |
| `+0xB0` | `0x71000489A0` | Wait for the effect resource, advance native effect time and completion conditions, pass options and transforms through unchanged. |
| `+0xB8` | `0x7100048BB0` | Release the effect record/resource and decrement both unit packet counters. |
| `+0xC0` | `0x7100048B50` | Dependency predicate: after resource readiness, compare elapsed time with twice the positive native timing result, or 2 when that result is nonpositive. |


Steam 10836033 data census of the 472 rows: bytes `+0x0A` and `+0x0B`
are zero in every row; bytes `+0x08` and `+0x09` are nonzero in 38 and
24 rows respectively. This is not proof that `+0x0A` and `+0x0B` are
padding in other builds. The nonzero flag bits and row counts are:

| Mask | Rows | Mask | Rows | Mask | Rows | Mask | Rows |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `0x1` | 22 | `0x2` | 2 | `0x4` | 21 | `0x8` | 68 |
| `0x10` | 55 | `0x20` | 12 | `0x40` | 44 | `0x80` | 55 |
| `0x100` | 3 | `0x200` | 31 | `0x400` | 29 | `0x800` | 38 |
| `0x1000` | 16 | `0x2000` | 10 | `0x4000` | 5 | `0x8000` | 13 |
| `0x10000` | 11 | `0x20000` | 15 | `0x40000` | 6 | `0x80000` | 2 |

## Extra signed field and timing — CORROBORATED

The Switch EFFECT descriptor at `0x710071C808` declares the same 0x1620-byte
payload shape as Steam's `0xB032D8` (**CORROBORATED** layout). In a larger
effect-processing path, Switch `0x7100014E50`–`0x7100014E64` reads
`+0x08` as a signed little-endian 16-bit integer. Under that path's
preceding condition, a negative value is negated for a nonnegative local
offset; otherwise that local offset is zero. The same original signed value
is loaded again at `0x7100014E90`, and later added to the local offset at
`0x7100014F70`–`0x7100014F88`. Steam `0x561AF`–`0x5627D`
independently reads the same signed halfword, scans targets for nonzero
`+0x184`, computes the same lead and lead-plus-field values, and writes the
lead to the first packet's floating-point `+0x88`. Complete presentation
semantics are native M8 pass-through; the no-reader result for `+0x0A`
and `+0x0B` is established by the whole-image census, not by this path.

| Signed halfword at `+8` | Preceding condition | Local offset |
| ---: | --- | ---: |
| -12 | true | 12 |
| 12 | true | 0 |
| -12 | false | 0 |

**CORROBORATED, timing-packet use:** The preceding condition is that at
least one target entry has a nonzero 32-bit value at `+0x184`. Let `x` be
the signed halfword and `lead` be `-x` only when that condition holds and
`x < 0`, otherwise zero. The first effect packet receives `lead` as its
floating-point delay (`0x7100014E90`–`0x7100014EC0`). For a target whose
`+0x17C` is zero and `+0x184` nonzero, when flag `0x800` is set, the
later target packet's delay is `lead + x + 5 * target_index`, with the
zero-based target index in iteration order (`0x7100014F70`–`0x7100014F88`,
`0x710001574C`–`0x71000157A4`, `0x71000161D4`–`0x71000161F8`; Steam
`0x56B48`–`0x56BA5`). The latter branch takes precedence over flag `0x4`;
the dependency kind is **5 in both lead cases**, but its UID source changes.
The [target-packet binding addendum](#0x800-interception-target-packet-binding-addendum)
specifies that selection, constructor fields and ordering. The physical delay
unit and complete visual interpretation remain **OPEN**, native M8 pass-through.

These two target inputs are not aliases: Golden17C binds native
`targetBlock.specialHitFlag` at PS2+E4, while Golden184 is a result-scoped
Golden interception sidecar. Its exact admission, publication and resets
are specified in [effect-target-state.md](effect-target-state.md); do not
substitute reactionFlag E0 or message.blocked for either input.

These two target inputs are not aliases: Golden17C binds native
`targetBlock.specialHitFlag` at PS2+E4, while Golden184 is a result-scoped
Golden interception sidecar. Its exact admission, publication and resets
are specified in [effect-target-state.md](effect-target-state.md); do not
substitute reactionFlag E0 or message.blocked for either input.

| `x` | Any qualifying target | Target index | First delay | `0x800` target delay |
| ---: | --- | ---: | ---: | ---: |
| -12 | yes | 2 | 12 | 10 |
| 12 | yes | 2 | 0 | 22 |
| -12 | no | — | 0 | branch not entered for an ordinary stable target list |

**SWITCH-ONLY, flag `0x20000`:** In the effect-processing routine beginning
at `0x7100013360`, the branch at `0x7100013738` enters the path at
`0x7100013A40` when this flag is set. On that path, when its local
selector is zero and actor-work `+0x1C` bit `0x40` was clear, the flag
causes `0x7100013A94`–`0x7100013AC4` to set that actor-work bit, clear
bit `0x1000` in global battle state `+0x10`, and set a local boolean to 1.
The enclosing branch has additional prerequisites. This is an observed
conditional mutation, not a complete semantic name for the flag; an
equivalent Steam branch has not been independently located.

For the genus-0 case, the enclosing branch requires `q == 1` as defined
above, action halfword `+0xAC` unequal to 3, and skill-row byte `+0x28`
unequal to 6 (`0x710004C790`). If action word `+0x1C` bit `0x40` is
clear, `0x710004A970` scans the action's target pointers and each target's
`+0x171` count of `0x20`-byte records: it returns true if any record's
byte at target `+0x196 + 0x20*index` has bit `0x10`. On true, the
routine draws one `random_u32 % 100` via `0x71000715A0` and compares it
with 20 *before* testing global `0x1000` or EFFECT `0x20000`
(`0x7100013A54`–`0x7100013A90`). Setting EFFECT `0x20000` therefore
does not permit skipping that earlier conditional random draw. The mutation
occurs if the comparison succeeds, or the global flag is set, or EFFECT
`0x20000` is set. If action bit `0x40` was already set, the EFFECT bit
still takes the local mutation path without that draw.

Synthetic branch vector: with those prerequisites, EFFECT `0x20000`
set, action bit `0x40` clear, and a target record carrying bit `0x10`,
the code consumes exactly one random value even when it is 99 modulo 100,
then performs the mutation. With no qualifying target record, the same
flag performs the mutation without that random draw.

**CORROBORATED, flag `0x40000` — fade exclusion, not target exclusion:**
Switch `0x7100062280` walks unit lists and updates qualifying units'
packed color at `+0x30` through `0x7100067280`. The two flag tests at
`0x7100062434`–`0x710006247C` and
`0x7100062578`–`0x71000625BC` skip this **color update** when the
candidate is the actor and the bit is set. Without that exclusion, the
actor's alpha byte is set to `0xFF`; other candidates use the qualifying
target mask or calculated fade alpha. Steam `0xBB2E0`–`0xBB32D` performs
the same bit and pointer tests and `0xBB38C`–`0xBB3D7` constructs and
stores the RGBA color. This is not evidence that the bit removes a unit
from the skill's attack-target list. Forward it to the native M8 effect
layer, preserving fade timing and any synchronization.

**CORROBORATED, additional context qualification:** Both list walks also
require the current action pointer to equal the result of the context
lookup `0x7100006250` (`0x710006241C`–`0x7100062428` and
`0x7100062560`–`0x710006256C`; Steam `0xBB333`–`0xBB38A`). The
Switch lookup first requires encounter selector206h and an input action
owning a genus1 unit with ID106h. It then selects the first ID10Fh unit in
the group1 unit lookup chain and converts that exact unit pointer to the first
matching action in action-chain order. It is not a106-or10F input predicate
and never falls back to106. The exact ordering, duplicate/missing-unit and
null-unit conversion rules are in
[effect-target-state.md](effect-target-state.md#effect40000-fade-context-exact-two-stage-lookup).
It returns the found action, not the unit. Thus, for a
non-null current action, these color writes are skipped unless that
lookup returns the current action itself. Do not replace this extra
qualification with the flag test alone. The Steam branch checks the
same selector and IDs before comparing the returned pointer.

Before the list walks, the updater returns complete if battle byte `+0x10`
has bit `0x08`, the skill byte `+0x15` is `0x10` or `0x11`, or the
target-count/actor-genus gate fails. For the nonzero result of
`0x710004ACE0`, the candidate-genus mask is the actor's genus bit and
the target-genus mask is zero; otherwise the candidate mask is `3` and
the target mask is the OR of the targets' genus bits. A genus-0 actor
also requires at least one target of nonzero genus. Candidates must have
nonzero `+0xC5A`, match the candidate mask and have `+0x9C` bit 0 clear.
These are local updater gates, not attack-target filtering
(`0x7100062280`; Steam `0xBB146`–`0xBB301`).

Synthetic context vector: with all other eligibility gates true and a
non-null action, encounter selector other than `0x206` produces a null
lookup result and no candidate color write, whether flag `0x40000` is
set or clear. If the lookup returns the current action, the flag's actor
exclusion in the following table applies.

| Flag `0x40000` | Candidate equals actor unit | This fade update |
| --- | --- | --- |
| clear | yes | Other eligibility checks permitting, sets actor alpha to `0xFF` |
| set | yes | Skips actor color update; its prior alpha remains unchanged |
| set | no | Continues through the other fade eligibility checks |

## Synthetic vectors

| Row bytes relevant to test | Consumer or operation | Expected |
| --- | --- | --- |
| byte 0 = `00` | count accessor | 1 |
| byte 0 = `03` | count accessor | 3 |
| bytes 4–7 = `00 00 01 00` | flag `0x10000` | set |
| bytes 4–7 = `00 00 08 00` | flag `0x80000` | set |
| bytes 4–7 = `00 80 00 00` | flag `0x8000` | set; `0x10000` clear |

Additional synthetic preservation vector: changing only row bytes `+1`,
`+2`, `+3`, `+0x0A`, `+0x0B` to `A1 B2 C3 D4 E5` leaves all
enumerated field reads unchanged, while the retained raw row contains those
five exact bytes. Out-of-range row indexing must retain native backing and
the native reader's behavior, not an adapter-invented clamp or zero row.

## 0x800 interception target-packet binding addendum

**CORROBORATED: Steam build 10836033 ordinary `.shared` code and Switch 1.2
ordinary NSO code.** This completes the R2 timing branch above, not a new
interception-admission rule or a claim of runtime qualification. Steam addresses
below are RVAs; Switch addresses are native VAs.

### Which packet receives lead+x

**No ordinary formation or reflection packet receives an unconditional
`lead+x` delay from `0x7100014F70`.** That site only computes a local sum:
`0x7100014F70..14F88` loads lead, adds signed row field x and converts to
float; `0x710001500C` saves that float at **sp+0x48**. Its packet-delay use
is the **flag800-controlled** load/add/store at `0x71000161EC..161F8`
into **Ii**, constructed at `0x7100015774`. Do not add this sum to every
ordinary target/formation packet merely because it was computed before the loop.

Steam independently stores the integer sum at **rbp+0x5C** (`0x5623A..56249`)
and reads it for conversion/addition only on the flag800 path
(`0x56B90..56BA5`), into Ii from `0x56B3C`. This is **not** the separate
u16 hit-time accumulator at **rsp+0x5C**. The same numeric displacement with
different frame bases is not the same local.

The actual other packet sites distinguish these inputs:

- Ordinary primary/formation Pi: Switch constructor `0x7100015380` / Steam
  `0x56508`. Its local-delay branch `0x71000154B0..154CC` /
  `0x56563..56586` uses `u16(hitTimeTotal)` plus the pre-existing extra delay
  (30 when target184 is nonzero and flag4 is clear, otherwise0). It does not
  consume `lead+x`. The preceding-packet alternative
  `0x7100015384..153A0` bypasses that delay write, retaining constructor delay0.
- Special/reflected primary Pi: Switch `0x71000151B8` / Steam `0x56649`;
  delay store `0x71000151E0..151F4` / `0x5667E..5668A` uses the u16
  hit-time accumulator, not `lead+x`.
- The additional reflection effect from `0x710001571C` (Steam `0x56AAA`)
  also does not receive this sum: the following reflected branch binds its
  flag4-dependent UID/kind and enqueues it at `0x71000157CC` / `0x56AE0`,
  leaving its constructor-default delay0.
- P0 receives **lead alone**; Ii receives **5*i alone initially**, with
  `lead+x` added **only when flag800 is set**. The exact Ii binding follows.

A bounded local-use census covers Switch `14F70..16D70` and Steam
`5622A..577CC`: the sum's local definition and guarded consumer above are the
only application uses (Switch's later frame/LR restoration is not a timing
consumer). Raw-span SHA-256 respectively
`ee1c8f17d464ae92f6d9bc96c0e0b54686db8b07be496c75aa16df1262570998`
and `63bcb918e218068629b21181ce52cf36f1bfd73f515407003a24bbfa9ee96a34`.
Private receipt: `<scratch>/GoldenRE-notes/l1/l1c-m7ar2-delay-local-uses.json`.

### Packet identities and the exact dependency

These names are local semantic references, not fixed numeric UIDs:

- **P0**: the initial effect packet constructed at Switch `0x7100014E9C` /
  Steam `0x5624C`, given `float(lead)` at header `+0x88` and enqueued with
  argument **2**. Its assigned packet UID (header `+0x98`) is saved at
  Switch `0x7100014EEC..14EF0` (`sp+0x98`) / Steam `0x562A8..562AF`
  (`rbp-0x30`) **before the target loop**.
- **Pi**: this iteration's preceding primary per-target effect packet, held in
  Switch `x21` / Steam `rsi` at the interception branch. It is **not P0**.
  Constructors return it at Switch `0x71000151B8` / Steam `0x56649`,
  or Switch `0x7100015380` / Steam `0x56508`; the selected path enqueues it with argument
  **2** (Switch `0x710001521C` / `0x71000154FC`). Its resource/options
  belong to the ordinary primary-effect path.
- **Ii**: the additional marked-target effect packet described below. Its UID
  is assigned by its own enqueue, not borrowed from the actor or target.

For an entry with `17C == 0 && 184 != 0`, the constructor is called **before**
testing EFFECT flag `0x800`. Thus the marked-target packet is not itself
conditional on that flag. The flag selects its dependency/delay behavior:

| EFFECT flags / lead | Ii dependency kind (header byte `+0`) | Ii dependency UID (header u64 `+8`) | Ii delay (`float`, header `+0x88`) |
| --- | ---: | --- | --- |
| `0x800` set, `lead > 0`, regardless of `0x4` | **5** | **Pi.uid** | `float(5*i) + float(lead+x)` |
| `0x800` set, `lead <= 0`, regardless of `0x4` | **5** | **P0.uid**, the saved pre-loop UID | `float(5*i) + float(lead+x)` |
| `0x800` clear, `0x4` set | `0x0B` | Pi.uid | `float(5*i)` |
| `0x800` clear, `0x4` clear | 5 | P0.uid | `float(5*i)` |

Here `i` is the original zero-based target-list index, not a count of intercepted
targets. `x` and `lead` are exactly as defined above. In the specified producer
`lead` is nonnegative, so its nonpositive case is zero; retain the actual signed
comparison if accepting an already-computed lead. **Do not select kind 0x0B for
positive lead**, and do not use Pi.uid in both cases. The saved UID is P0.uid,
not an action UID or an arbitrary most-recent packet UID.

Switch `0x7100015778..157A4` initializes `5*i` and tests bit11 before bit2.
At `0x71000161D4..161F8`, `w22 == 5` is stored unconditionally, signed
`lead < 1` retains the saved UID, and the positive alternative loads
`[x21+0x98]`. Steam `0x56B48..56BAE` independently stores kind5 before
`jle`, chooses `[rsi+0x98]` versus `[rbp-0x30]`, and adds the saved
`lead+x`. This is a UID switch, **not a dependency-kind switch**.

### Ii constructor and fields

Use the ordinary effect constructor **Switch `0x7100048890` / Steam
`0x99AA0`**, called at **`0x7100015774` / `0x56B3C`**:

| Argument / field | Exact binding for Ii |
| --- | --- |
| Resource argument / work `+0x00`, pointer | **Golden common slot48**, `data.cpk::battle/btleff.bin::R_BLOCK.BED`, installed as a formation-work handle at Switch battle+14A8 / Steam battle+1490. Loaded at `0x7100015760` / `0x56B35`. Use the [Golden-only installation/lifetime seam](effect-interception-resources.md#golden-only-common-interception-formation), not a PS2 slot substitution, Pi's resource or a raw BED pointer. |
| Source unit argument / work `+0x08`, pointer | Acting action's unit, action `+0x38`: Switch `[x26]`, Steam `[r15+0x38]`. Switch `0x7100013380..13384`, `0x7100013700` and `0x71000144C0` establish the retained unit-slot alias. |
| Target unit argument / work `+0x10`, pointer | Current target entry's unit at `+0x38`: Switch `[x28+0x38]`, Steam `[rbx+0x38]`. |
| Mode argument / work `+0x18`, u16 | **1**, not P0's mode0. |
| Options argument / work `+0x34`, u32 | **0**. Do not inherit Pi's `0x1000` / `0xC00` or EFFECT-derived `0x100000` options. Switch explicitly passes `w4=0`; Steam's fifth stack argument is zero (`edi=0`, including reset `0x568B7`, store `0x56B31`). |
| Packet type / header `+0x80`, u32 | **0x301**; constructor requests **0x58 bytes of work**. Header `+0xC8` addresses that owned work. |
| Dependency slot0 / header `+0`, `+8` | Set from the table above. Other seven dependency slots keep allocator defaults: kind1, UID0. |
| Header `+0x88`, float | Delay from the table above; set before enqueue. |
| Header `+0xA0`, u64 | **Acting action UID**, unconditionally on this path (Switch `0x7100016210`, Steam `0x56BD0`). This owner tag is distinct from packet UID `+0x98` and dependency UID `+8`. |
| Enqueue | Switch `0x710005AE80(Ii,1)` at `0x7100016218`; Steam `0xB1090(Ii,1)` at `0x56BDA`. Preserve queue argument **1**. |

Constructor witnesses: Switch `0x71000488AC..48908`, Steam
`0x99ACD..99B49`. Both allocate type301/work58, clear header87 mask10,
and store all five arguments into the offsets above. The allocator zeroes the
allocation and then initializes all eight dependency kinds to1 (Switch
`0x710005AE08..5AE40`, Steam `0xB0F63..B0FAC`); do not replace untouched
dependency kinds with0. Other untouched work/header fields retain constructor
defaults. The installed callback tuple is:

| Header | Switch | Steam |
| --- | --- | --- |
| `+0xA8`, start | `0x7100048920` | `0x997A0` |
| `+0xB0`, update | `0x71000489A0` | `0x99840` |
| `+0xB8`, release | `0x7100048BB0` | `0x99A50` |
| `+0xC0`, dependency predicate | `0x7100048B50` | `0x999F0` |

These are the existing type301 lifecycle callbacks, not a new timing-only packet
with no effect work. Enqueue assigns the packet's own UID **before** invoking its
start callback and linking it into the selected queue (Switch
`0x710005AE94..5AEBC`, paired store to header90/98 at `5AEA4`;
Steam `0xB10B1..B1110`, UID store at `B10E8`).

### Ordering and followers

Keep constructor/enqueue order and dependency edges distinct:

1. Enqueue P0 with argument2 and save its assigned UID; construct/enqueue each
   Pi with argument2 through the existing primary-effect branches.
2. Construct Ii, bind the selected dependency, delay and owner UID, then enqueue
   it with argument1. For positive lead its kind5 edge is to Pi; for zero lead
   it is to P0 even though Pi has already been enqueued.
3. Immediately construct **Switch `0x7100063120(4,2,0x0B)` / Steam
   `0xBCB70(4,2,0x0B)`** at `0x7100016228` / `0x56BEB`. This is a distinct
   **type0x902**, six-byte work packet holding three u16 values `(4,2,0x0B)`,
   not Ii. Set its dependency **kind5 / Ii.uid** and enqueue with **1**
   (`0x710001622C..1623C`, Steam `0x56BF0..56C07`).
4. Preserve the subsequent existing message/presentation calls. On the
   once-per-loop gate, the `0x6AC + datCalcRandomBelow(2)` **immediate voice-cue**
   constructor (`0x710006F9D0` / `0xCE860`) is enqueued with **kind5 / Ii.uid**, original
   acting-action UID owner, argument **1**. Then the per-target unit/`0xBB`
   constructor (`0x7100084740` / `0xDD090`) is enqueued with **kind5 / Ii.uid**,
   the same action owner and argument **3**. Switch `0x7100016240..162B4`,
   Steam `0x56C0C..56C8F` also retain the once-gate publication. These followers
   depend on **Ii**, not on the type902 follower or each other: do not invent
   a serial chain merely from enqueue order.

Synthetic binding vectors (not game execution): with distinct P0.uid=101,
Pi.uid=202, Ii.uid=303 and target index2, `x=-12, lead=12, flags=0x804`
gives Ii `(kind5,uid202,delay10,mode1,options0)`; `x=12, lead=0` with
the same flags gives `(kind5,uid101,delay22,mode1,options0)`. Both type902
followers use `(kind5,uid303)` and work `(4,2,0x0B)`. With flag800 clear,
flag4 set selects kind0B/uid202 and delay10 instead. These deliberately distinct
UIDs expose the erroneous “same UID for both lead cases” implementation.

### Static evidence receipt and scope

Recovered directly from the supplied Steam executable's ordinary `.shared`
section (RVA1000/raw600 mapping, checked against its PE section table) and the
approved Switch1.2 mapped NSO. No `.arch` protector code or game execution.
The following SHA-256 values identify exact raw instruction spans, with
end-exclusive endpoints:

| Binary / range | SHA-256 |
| --- | --- |
| Switch `0x71000156E4..15800` | `0dc340fa86c945e36d1b922cba89e28aeb56f3eccfa5fb74ed947e10b96d8051` |
| Switch `0x71000161D4..162C8` | `30803824057b6f2b4e876d0db38939556b72ad1a6217e3a9c64f35656770d411` |
| Steam `.shared` `0x56B0F..56C94` | `48f8bb6b4c1f1e4a49c15b02b7ad33592bb404e42ec9ef5a4e0123f647a651ab` |
| Switch constructor `0x7100048890..48920` | `bc4121d6f68a4c724df5a35b7ba2c0e93423111b4b1b8b578cb4f6862f471d55` |
| Steam constructor `0x99AA0..99B53` | `7f06645a7a622d86ed66b3ff9eda864089b6011f1750664b5e502ee2657821e8` |
| Switch P0/UID source `0x7100014C80..15014` | `d5286a5d305daa330aeaf9af25447347d18cd45b8861f5131632e330a3c6657e` |
| Steam P0/UID source `0x561AF..56300` | `1223a11f941522cf4d423bbe25f19b22383bfca56759db050f0a7fe10d420960` |

Private reproducible instruction receipts:
`<scratch>/GoldenRE-notes/l1/l1c-m7ar2-binding-instructions.{json,txt}`;
source reader `l1c_m7ar2_binding.py`, run with the required Git Bash and
fixed Python312. This is structural source evidence only; native M7aR2/M8
binding, builds and runtime behavior were not exercised by this addendum.


The [resource and native-helper addendum](effect-interception-resources.md)
qualifies the R_BLOCK.BED loader/lifetime and the902/A02/506 projections shared
with [M8f](m8f-battle-effects.md). In particular A02 is an immediate voice cue,
not the native multi-step A03 voice packet or a unit text message.
