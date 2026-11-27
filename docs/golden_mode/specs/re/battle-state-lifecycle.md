# Golden AI battle-state lifecycle: follow-up findings

**Status: PARTIAL — writer/lifecycle closure remains open.** This follow-up
must not be read as proof that every Golden state field has a PS2 binding.
The R4 command transitions in [ai-commands.md](ai-commands.md) are specified;
this document records additional lifecycle evidence needed to implement them.
No default-zero sidecar is an equivalent replacement for a live writer.

Scope is the same pinned Switch NSO and Steam build as that specification.
Switch addresses are virtual addresses with base `0x7100000000`. Findings
are static, not an executed game/emulator receipt. No Steam `.arch` decoding
was performed. PS2 citations refer to the decomp's `src/Battle/` files.

The bounded M7a/R2 [effect target-state addendum](effect-target-state.md)
binds Golden17C to native specialHitFlag E4 and header1D mask01 to native
flags18 mask0100. Golden184 instead has a specified interception-sidecar
producer/reset projection. These bindings do not restart or close the stopped
whole-image lifecycle census below.

### Per-field lifecycle disposition

| Field/input | Trigger semantics established here | Exhaustive writer status |
| --- | --- | --- |
| 8472 battle `+0x28` | Eligible all-actor start; continuous-update entry reset; native event and early-return dependencies identified | OPEN: root aliases/callbacks not fully discharged |
| 8472 battle `+0x1E` | Opening-side precedence, write/clear/preserve branches; native openingSide binding | OPEN: root aliases/callbacks not fully discharged |
| 8479 battle `+0x1632` | Setter narrowing, ASSIST state23 dispatch, skill script/queue, action-end admission below, two SFMT draws, packet selectors and classification | OPEN: exhaustive root/actor alias and writer closure remains |
| 8481 actor `+0x170`, mask2 | Constructor, OR setter, nonzero-message snapshot/clear, zero-message preservation; the `0x187AC` clear is now a writer-recall regression | OPEN: actor copies/callbacks/returned pointers not fully discharged |
| Actor message `+0x188` | Constructor, script setters and per-turn clear; native messageCode binding | OPEN: full actor writer aliases remain |
| Battle bit29 | Exact result/continuation transition, endState producer and native `btlMainActionUpdate` / `btlCheckBattleEnd` counterparts | OPEN: full encounter-record and root alias writer closure remains |
| Enemy knowledge backing/observation | Published persistent backing, whole-buffer reset/backup/restore, tagged save transfer, bit setter/clearer | OPEN: backing aliases, observation and clearing admissions remain |
| Enemy knowledge reveal | Exactly bits0,1,2,3,4,6,7 in the two known reveal paths; bit5 is not revealed | OPEN: complete reveal-trigger and callback provenance remains |
| Entity chain, activity, targetability and IDs | Native owning state and action creation identified | OPEN: removal, replacement, disable and alias writers remain |
| HP/SP, status, guard and buffs | Native calculation/result events identified | OPEN: Golden mutations and per-turn reset census remains |
| Persona, learned skills, equipment and level | Native roster/equipment ownership identified | OPEN: complete live selection/equipment/level writer bindings remain |
| Candidate work and selected-target scratch | Command builders and selection writes specified in ai-commands.md | OPEN: complete scratch reset/replacement lifetime remains |

No row is promoted to lifecycle SPECIFIED by this continuation. This table
does not reopen or weaken the separate R4 command-reader specification.


## Allocation and destruction boundaries

| Owner | Switch evidence | Required lifetime | PS2 event counterpart |
| --- | --- | --- | --- |
| Battle runtime | `0x7100057900`, allocation/publication at `0x710005796C` | Allocate `0x1688` bytes and zero the entire allocation before initializing subsystems. This initializes state `+0x28`, `+0x1E`, `+0x1632` to zero. Initial flags OR `0x1480877C` does not set bit29. | `btlInit`, `0x00192B90`, allocates/zeros `0xE88` and applies the same initial flag mask. Do not transplant Switch offsets. |
| Battle runtime teardown | `0x71000585C0`, null publication at `0x7100058738` | Destroy battle actions/subsystems/packets before freeing the runtime and nulling its root. Extra battle-scoped state must not survive this destruction. | `btlRelease`, `0x00192CD0` |
| Action runtime | `0x710000A490` | Allocate and zero `0x528` bytes; initialize selected-target work, assign UID/entity ID, link at the battle action-chain head, then invoke initial action-state handler. Thus actor `+0x170` and `+0x188` begin zero. Allocation itself consumes an AI random draw modulo60 for another field; it is not a draw by an AI script handler. | `btlActionCreate` (`0x001B0930`, `btlAction_inferred_001b0930.c`) allocates/zeros native `0x458`, initializes target work and UID, consumes its idle-timer draw and links the action. `btlActionCreateWork` (`0x0019F5F0`) attaches the unit and queue flags. |

## 8472 state `+0x28`: new eligible-action-start counter

This is **not** the existing protagonist-turn counter. In
`0x710000ACC0`, increment at `0x710000AF5C` occurs immediately after the
conditional protagonist-counter increment (`+0x24`, `0x710000AF4C`). The
new counter increments for every actor entering the eligible turn-start
branch, not only party ID1. Arithmetic is 32-bit wrapping.

The branch is reached only after these turn-start gates:

* The current unit's recovery branch has been handled; no other active living
  action has battle-unit `+0x9C` bit4 set.
* Actor byte `+0x1D` is nonnegative (bit7 clear).
* The earlier turn-start delay/interception branches do not return early.
* Actor `+0x4C0` is zero; that field is calculated from
  `0x71000727E0` only when actor bytes `+0x30/+0x31` are both zero, otherwise
  explicitly zero. Guard helper `0x7100075CD0` also returns zero.
* The earlier snapshot of battle flags has mask `0x80000` clear.

After increment, the routine chooses strong-buff/status-cure/ordinary next
state. It does not decrement the counter upon action completion.

The turn-start dependency slice is now identified:

| Switch predicate | Observed condition | PS2 event binding |
| --- | --- | --- |
| `0x7100051670(actor)` | Return0 unless battle flags bit21 is set and the pair `(actor+8, actor+0x28)` differs from the last intercepted pair at battle `+0x1134/+0x1148`. Otherwise create/run the actor's script task, publish it at `+0x1120`, write state2 at `+0x1128`, remember the pair and return1. A negative script callback index skips the setup callback, **not** the return1/interception. | The early `btlStatusWindowShowSlot2` branch in `btlActionStartInit` (`0x001A06D0`). |
| `0x71000727E0(unit)` | Learned-skill membership builds a passive-recovery mask. IDs `0x219..0x21E` map to masks1,2,4,8,16,32; `0x25F..0x261` to `0x8000..0x20000`; `0x255/0x256/0x25B..0x25D/0x257..0x25A` to `0x40/0x80/0x100..0x400/0x800..0x4000`. Any nonzero mask suppresses this eligible-start increment. These are skill IDs, not status bits. | `datCalcGetSkillMask` in the same native start event. |
| `0x7100075CD0(unit)` | Bit9 of the calculation-unit's first halfword, as0/1. | `datCalcIsGuarding` in the same native start event. |

The earlier delay branch is specifically party genus0, stored signed16
battle `+0xF50 != -1`, same-side count excluding status `0x80000` no greater
than signed `+0xF52 >> 1`, stored ID differing from formation selection
`0x71000518A0()`, and no current status under mask `0x180001`. It selects
state4 and returns before the increment. Own recovery flag selects state
`0x1C`; actor flag bit15 selects state `0x25`; script interception selects
state `0x1A`. These must remain early returns, not post-increment corrections.

`0x710005C010` clears the counter at `0x710005C030` **at entry on every
invocation**, before checking whether active enemy actions still exist.
It resets non-enemy action states while waiting for the continuous-encounter
transition, returns zero while enemies remain, and otherwise selects the next
encounter record and returns state5. Therefore the reset is not merely
"at battle init", "once per round" or "after the first action".

PS2 mapping: `btlActionStartInit` (`0x001A06D0`,
`btlAction_inferred_001a0670.c`) has the analogous eligible `canAct` branch
and conditional `protagonistTurns` increment, but no corresponding all-actor
counter increment in that branch. Maintain Golden's additional counter at
that event, not by aliasing `BtlBattleState.protagonistTurns`.
The reset moment is `btlMainContinuousUpdate` (`0x001B1B30`,
`btlAction_inferred_001b1510.c`) at routine entry, before its enemy-presence
scan. Ordinary action-start alone never resets it.

## 8472 state `+0x1E`: encounter opening-side classification

`0x710005C1C0` creates party/enemy action work for the encounter, then checks
battle flags bit8. When set, it reads the 24-byte encounter row selected by
current encounter-record `u16 +0x10`. If row flags bit5 is clear, obtain
`0x7100046DA0()` and store its low16 at `0x710005C4A4`. If row bit5 is set,
row bit4 clear explicitly stores zero at `0x710005C4EC`; with both set the
existing value is preserved. If battle flags bit8 is clear, this opening-side
block does not overwrite the field.

Opening-side calculator `0x7100046DA0` has this ordered rule:

| Condition, in order | Value |
| --- | ---: |
| Battle flags bit29 set, or selected encounter-row flags bit3 set | 0 |
| Otherwise encounter-row flags bit4 set | 2 |
| Otherwise encounter-row flags bit11 set | 1 |
| Otherwise encounter-setup halfword at state `+0x1240`, bit2 set | 0 |
| Otherwise that halfword bit0 set | 2 |
| Otherwise | that halfword bit4, as 0/1 |

Queue ordering then consumes category0/1/2; categories1/2 set corresponding
queue bits, and row flags can also request battle flag `0x100000`.
Do not recompute the category on every BF query: the stored value can be
preserved by the encounter-row branch and 8472 gives counter `+0x28`
precedence over it.

PS2 counterpart is **`BtlBattleState.openingSide` at `+0x1A`**, not endState.
The analogous writer is `btlMainUnitCreateInit` (`0x001B1D70`) and its
`btlUnitGetMoveStateCode` calculation, with the same row-bit5/bit4
clear-versus-preserve distinction. The Switch field is initialized to zero
with the battle allocation, and its explicit reset above is an encounter
creation event, not an action-resolution event.

## 8479 state `+0x1632`: persistent assist choreography selector

Allocation zeroes it. The identified explicit writer is command8479 at
`0x710002C63C`, storing low16 of its argument. The identified consumer is
`0x7100017470`, with signed-halfword load at `0x7100017A68`:

| Stored value | Choreography selector passed to `0x710006F810` |
| ---: | ---: |
| 1 | `0x21` |
| 3 | `0x22` |
| Every other halfword pattern | `0x20` |

The consumer is an assist-action presentation routine: it consumes battle
`+0x150C` as the skill, clears that pending-skill field, prepares actor
kind2/skill, requests `battle/assist/ASSIST_ST.BED` and `ASSIST_EF.BED`,
then emits the selected packet and marks actor flags. Packet
ownership/completion must remain native, not disappear when the AI setter
returns.

### Assist dispatch and parameters

GOT `0x7100880590` resolves to the 24-byte action-state descriptor table
`0x7100709A40`. Each entry is initializer, updater, label. Entry23
(`0x17`), at `0x7100709C68`, is labelled ASSIST, with initializer
`0x7100017470` and updater `0x7100017AF0`. Thus the consumer is reached by
an action-state transition, not by the setter8479 itself.

Scheduler `0x7100063C70` tests three enabled bits at battle `+0xF1C`
in order1,2,4. The bit1/bit2 branches can schedule ESCAPE_MES/YELL and
return before the bit4 branch. Bit4 assist eligibility requires:

* scheduler argument nonzero;
* current action at battle `+0x398` nonnull, with actor `+0x20` bit0 set;
* `0x7100047CD0()` nonzero: script flag `0x38` set and the queried
  progression value (`0x710011BA50`) greater than6;
* no pending action in battle `+0x3F8`.

For bit4, the previous unsigned16 counter at `+0xF18` is saved, an SFMT
float draw is discarded, and that counter increments with halfword wrapping.
If the previous counter was nonzero, script argument `0x7E` is set to the
predicate `u16(+0xF1A) <= previous / same-side-count(1,0)`; zero count makes
that quotient zero. If the previous counter was zero and openingSide is1,
argument `0x7E` is1. Otherwise argument `0x7E` is0 and `+0xF1A` receives
the low16 of truncation of `2 * SFMT-float + 4`.

`0x7100047CA0` invokes `0x710001D140` on the helper action at battle
`+0x1B8`. That runs the script entry formatted `AI_PC_005_ASSISTSKILL`
and retrieves script result slot `0x7E` (the same slot supplied as input).
The scheduler clears pending skill `+0x150C` regardless of the result;
nonzero result is then stored there, helper actor `+0x1A` becomes `0x17`,
and `0x71000593C0` appends it to the first free pending-action slot
`+0x3F8..+0x450` (12 eight-byte slots; full returns0, not an overwrite).
It draws another SFMT float to replace `+0xF1A` as above, sets
`+0xF18` to1, and returns. Zero script result schedules nothing.
This does **not** clear `+0x1632`.

The scheduler's three direct callers are encounter-unit creation
`0x710005C1C0` at `0x710005C654` with argument1, and action-end update
`0x710001AF20` at `0x710001B124`/`0x710001B1AC` with arguments0/1.
Argument0 cannot schedule assist. Native encounter seam is
`btlMainUnitCreateInit` (`0x001B1D70`). Assist is a Golden-specific
state/script/resource mechanic; this is not proof that native PS2 AI or a
native wait-message packet already implements it. The action-end admission
is detailed below; exhaustive enabled-bit and alias writers remain open.

At presentation, both `0x7100071610(3)` calls execute unconditionally,
even for selector `0x20`; the first result is discarded. This helper uses
`0x710033B010` and SFMT advance `0x710033ADD0`, **not the additive AI
stream**. It converts the unsigned SFMT word to float32, scales by
`2^-32`, multiplies by float32(3), and truncates to unsigned32. The
consumer narrows the second result to signed16. Do not replace it with
integer modulo3: float rounding can produce3 at the upper boundary.
For selector `0x20`, the following classification replaces that result:

| Pending skill | Packet argument |
| --- | ---: |
| `0xB1`, `0xB4` | 1 |
| `0xB2`, `0xB3`, `0xB5..0xB8` | 2 |
| Every other skill | 0 |

These eight words are at `0x710062B008`. Selector1/3 special cases still
receive the second random result, not the skill classification.

**Still open:** the final alias/callee writer census proving whether other
paths write or clear the selector. Do not reset the selector at each turn
or assume only values1/3 are admitted.

Assist enabled bit4 is initially set by battle initialization:
`0x71000579B0` calls `0x7100064290`, which clears
`+0xF10..+0xF4F` and then ORs6 into `+0xF1C` at
`0x7100064308`. Thus the initial assist counter and threshold are0 and
the scheduler's bit2/bit4 branches start enabled. Another identified
enabled-bit writer, `0x7100063F50`, ORs a shifted bit; its only direct call
at `0x7100011358` supplies0 (escape enable), not assist bit index2.
The scheduler's identified clears remove masks1/2, not mask4. These
immediate writers do not substitute for the remaining alias census.

## 8481 actor `+0x170`, mask2: message-follow-up latch

Here "bit2" means **mask `0x02` (bit index1)**. Constructor zero-fill
initializes it clear. Command8481 ORs this mask at `0x710001D7A4` and stores
the message parameter at actor `+0x188`. Command8469 stores
the same parameter but does not set this latch.

The reader/clear is in `0x7100018710`, command-kind7/8 wait presentation:
when message parameter `+0x188` is nonzero, it queues the corresponding
message/effect-counter packet, snapshots `(byte170 >> 1) & 1`, then clears
only mask2 at `0x71000187AC`. The snapshot later controls the extra
`0x7100069800(..., 0x1000)` follow-up packet after ordinary presentation
packets. Kind8 suppresses a different presentation branch, so do not
collapse kinds7/8. With message parameter zero, this branch assigns a zero
snapshot but **does not clear the stored mask2 there**. Other byte bits are
preserved by the script setter and this consumer's clear.

`0x710004AEC0` is a separate selected-skill result producer: its explicit
byte170 update at `0x710004BC58` ORs mask1, not mask2. Its initial clears
start at `+0x171`, so they must not be misreported as clearing byte170.
The per-turn helper `0x710004A900(actor+0x170)` clears only the halfword
at relative `+0x18`, i.e. message parameter `+0x188`, **not the latch**.
The result-buffer zero helper `0x710004A8F0` clears 32 bytes of the supplied
buffer; all calls/aliases to an actor work region still need closure.

PS2 counterparts: `btlAICommandScriptSetActionSubword` (`0x001DF140`)
stores actor `+0xEC` / `targetBlock.message.messageCode`, then selects
kind7 through decoder `0x001DE640`. `btlActionWaitUpdate` (`0x001ACF50`)
presents the nonzero message; its PS2 kind8 does not have the same combined
kind7/8 branch as Golden. `btlTargetClearWork` (`0x001F0A40`,
`btlTarget.c`) clears the messageCode only, just like Switch's per-turn
helper. The dedicated Golden mask2 follow-up has **not** been proved to
be implemented by an existing PS2 flag. Preserve it separately until that
native route is explicitly owned; do not infer equivalence from offsets.

## Other state read by the commands: ownership inventory

These are live inputs, not arbitrary configuration defaults. Their complete
writer census is **not closed by this follow-up**.

| Input | Known owning event / native counterpart | Remaining qualification |
| --- | --- | --- |
| State flags bit29 (8472/8474) | Battle init starts clear; stores `0x710005D998`/`0x710005D9AC` in `0x710005D8E0`; native `btlMainActionUpdate` (`0x001B3A00`) | Exact continuation/result transition below; upstream endState/next-record ownership and alias census remain open |
| Entity chain/activity, unit targetability and IDs | `0x710000A490` links actions; `0x710000A220` constructs party/enemy actions, sets actor activity bits0/3 and battle-unit targetability bit3 | Removal/replacement/disable and alias writers require further census |
| HP/SP/status, guard, buffs | Calculation-unit fields consumed directly; PS2 `datCalc*` mutation and `btlTargetApplyHpSpChange` (`0x001F3010`) are the live action-result event | Golden-only mutation routes and per-turn resets not enumerated here |
| Persona selection/learned skills, equipment, level | Existing native roster/equipment/level records; dependency reads in ai-commands.md | Bind real native selection/equipment events; do not create isolated copied snapshots |
| Enemy knowledge bitmap | Persistent enemy/element observation, PS2 `datSetBitmapBit` (`0x0010F3D0`) | Switch setters, new-game/save-load and bounds lifetime still need a writer receipt |
| RNG states | Both generator initialization, advances and finalizer specified in ai-commands.md | Preserve shared engine stream ownership; no per-command reseeding |
| AI candidate scratch, selected-target pointer and flag | Builder and 8485 writes specified in ai-commands.md | Native scratch/reset/replacement lifetime remains an adapter obligation |

## Synthetic lifecycle vectors

These are invented input/event sequences, not runtime receipts.

| Sequence | Required observation |
| --- | --- |
| Allocate battle, openingSide1, counter0; query8472 | 1 |
| Same, eligible non-protagonist turn-start branch executes | Counter becomes1; query8472 is -1 despite openingSide1 |
| Same, action finishes | No counter decrement implied |
| Enter continuous-update while an active enemy remains | Counter is reset0 before update returns0 |
| Opening creation with row bit5 and bit4 both set, prior category2 | Preserve2 |
| Row bit5 set, bit4 clear | Category becomes0 |
| 8479 argument `0x10003`, then assist presentation | Stored3; packet selector `0x22` |
| 8479 argument2, then assist presentation | Selector `0x20`; not automatically selector `0x21` |
| 8481 sets mask2 and nonzero message; wait presentation | Snapshot1, clear stored mask2 only, queue the extra follow-up after ordinary presentation |
| 8481 sets mask2 but message0; same consumer branch | Snapshot0; that branch does not clear the stored latch |
| Per-turn message clear with prior actor170=`0xA7` | Message becomes0; actor170 stays `0xA7` |
| Battle release then fresh allocation | No prior battle counter, opening side or choreography selector survives |

| Additional synthetic sequence | Required observation |
| --- | --- |
| No enabled bit4, or scheduler argument0, otherwise eligible | No assist script evaluation or new pending assist skill |
| Enabled bit4; active actor valid; script flag `0x38` clear | No assist script evaluation |
| Enabled bit4; all gates pass; pending slot `+0x3F8` occupied | Do not replace that pending action |
| Assist script result0 with old pending skill `0xB1` | Clear pending skill; do not schedule helper actor |
| Assist script result `0xB4` and free pending slots | Store pending skill `0xB4`, helper pending state23, append helper; reset assist counter to1 |
| Assist presentation skill `0xB4`, selector2 | Emit selector `0x20`, argument1; still consume both SFMT draws |
| Assist presentation skill `0xB8`, selector2 | Emit selector `0x20`, argument2; still consume both SFMT draws |
| Assist presentation skill `0xB0`, selector2 | Emit selector `0x20`, argument0 |
| Assist presentation selector3; second SFMT word `0x80000000` | Float32 path gives argument1, selector `0x22` |
| Assist presentation selector1; second SFMT word `0xFFFFFFF0` | Float32 rounds word to `2^32`; argument3, selector `0x21` (not modulo3) |
| Result preparation endState1, next index2 occupied, bit7 set | Set bit29; continuation state12 on Switch /11 native |
| Same but next index3 | Clear bit29; do not enter that continuation branch |
| Result preparation endState2, prior bit29 set | That branch preserves bit29 |
| Guard bit9 set at eligible-start check | No `+0x28` increment |
| Learned skill `0x219` present, actor `+0x30/+0x31` both0 | Passive mask1 suppresses increment; do not treat this as skill slot count |
| Actor `+0x30` nonzero, same learned skill | Passive-mask work is forced0; other gates still decide increment |
| Knowledge set `(entry4, element3)` from zero words | Word1 becomes8; battle destruction alone does not imply its reset |


## Static receipt and remaining closure

External raw notes: `lifecycle_census.py/.json`,
`lifecycle_writer_inventory.txt`, `lifecycle_frontier.json`,
`lifecycle_flags.txt`, `lifecycle_meta.txt`, and per-function `*-ai.txt`.
The original traversal and its eight-table refinement were incomplete.
The corrected interprocedural expansion `life_census2.py/.json` records
2,934 seeds, 5,198 reads, 1,833 writes, 967 call events, 142 returns,
126 stored-pointer events, 119 unsupported reports, 63 indirect exits,
44 dynamic calls, 15 stack stores and one outside-text boundary.
These are **frontiers, not an exhaustive closed receipt**. The pass
propagates direct-call arguments and action-list links and seeds action
initializers/updaters, but has no general callee-return summary or
stored-pointer reload closure. Unknown-offset taints are not disjoint-range
proofs, and pointer-compare unsupported reports are not additional writers.

Eleven relative tables now have explicit successor lists. The three
additional tables are branch `0x71000032A8`, table `0x7100629E98`,
10 unsigned16 entries, target base `0x71000032AC`; branch
`0x710000354C`, table `0x7100629EAC`, 11 unsigned8 entries, target base
`0x7100003550`; branch `0x7100003654`, table `0x7100629E80`,
24 unsigned8 entries, target base `0x7100003658`. Each entry scales by4.
The initial exploratory guesses for these three tables were corrected from
their actual preceding bound checks before the reported pass.

The 42-entry action table `0x7100709A40..0x7100709E30` is enumerated in
`life-clearcalls.txt` with initializer/update pairs and labels. It is not
closed merely by seeing the ASSIST entry: action callbacks, camera virtual
tables, root returns and packet-stored pointers remain to be discharged.

The import identity is independently resolved: `0x7100530A60` dispatches
through GOT `0x710087F740`, whose JUMP_SLOT dynamic symbol is **memset**.
`0x7100338840` is its word-length forwarding wrapper;
`0x710004A8F0` passes zero and length32 to that wrapper. All 22 direct
calls to the latter are listed with argument setup in `life-clearcalls.txt`;
their existence alone does not close pointer aliases. Do not trust the old
worker's atan2f label on the PLT tail at `0x7100530A6C`.

LLVM independently decoded the guard, assist scheduling, float conversion,
message clear, memset wrapper/PLT and three added jump-table instructions
(`life-llvm.txt`, every invocation exit0). Native source was read-only.
No build, unit test, game, emulator or CI run was performed.

### Additional bit29 transition evidence

`0x710005D8E0` returns without changing bit29 when battle flags bit7 is clear.
With bit7 set, it performs completion/result preparation, then examines
signed16 battle `+0x20`. Value2 takes a later branch without the bit29
assignment, and values other than1/2/3 return without the assignment.
For value1, when unsigned16 `+0x1238` is below3 and the corresponding
eight-byte pointer in state `+0x1268` is nonnull, it sets bit29 and returns
state `0x0C`. Every other value1 case clears bit29 and continues result
preparation; value3 first clears result scratch `+0x12B8` length `0x6C`,
then also clears bit29. The PS2 counterpart is now grounded:
`btlMainActionUpdate` (`0x001B3A00`,
`btlAction_inferred_001b1510.c`) checks the same `flags & 0x80`,
`endState`1/2/3, encounter index and occupied next-record slot; its
continuation branch sets `0x20000000` and returns native state11 rather
than Switch state12. Native result scratch is `+0xCAC` length `0x58`,
not Switch `+0x12B8` length `0x6C`. This is a conditional
continuation/result transition, not a generic per-turn reset.

### Additional knowledge-bitmap writer evidence

Switch `0x71000544A0` clears `0x174` bytes through slot
`0x7100885630`; a direct caller is `0x71002760A8` in the global setup
sequence. `0x71000544D0(entry, element)` ORs the bit at
`u16(entry)*8 + u16(element)`, using 32-bit words. There is no element
bounds check in that setter. Result observation calls it at
`0x710004BC28`. Another result-observation call is `0x71000480F4`;
the reveal paths `0x710008608C..0x71000860EC` and
`0x7100086528..0x71000865A0` set **seven** element bits:
0,1,2,3,4,6,7. They do not set bit5. The earlier eight-bit description
was incorrect; the explicit clear helper below does clear eight bits.
`0x7100054510` explicitly clears eight bits for an entry; direct call
`0x7100049744` supplies a unit's ID.

Native mutation is `datSetBitmapBit` (`0x0010F3D0`, `src/g_data/g_data.c`).
In `btlTarget_inferred_001f1030.c`, result observation writes knowledge
only for enemy genus1, element less than8, and result mask6 clear, after
the outer meaningful-result test. Native full reset is
`datClearCollectionBitmap` (`0x0010F390`), 85 words rather than Switch's
93 words. The bitmap belongs to persistent data, not a freshly zeroed
battle sidecar. Switch publication/save-load aliases and the full reveal/
clear caller admission remain unclosed; none of these observations proves
that battle teardown clears persistent knowledge.

## Continuation: action-end admission

Switch `0x710001AF20` corresponds to native `btlActionEndUpdate`
(`0x001AFB50`, `btlAction_inferred_001ade90.c`). The scheduler is not
an unconditional end-of-turn hook:

1. Battle flags bit7 already set returns immediately. Retain entry actor
   flags `+0x1C` mask4 independently of later flag changes.
2. Entry actor mask8 plus successful `0x710000A920(actor)` enters the
   follow-up branch. Outstanding packets ID `0x504` or `0x506` stall the
   update before either scheduler call. `0x710005AC90` counts packet
   `u32 +0x80` matches across all four lists `+0x210/+0x220/+0x230/+0x240`,
   following packet `+0xD0`. Unit `+0x9C` bit4 then selects recovery
   state `0x1C` and returns. Otherwise append pending actor state `0x22`
   and continue. When this branch does not succeed, clear actor mask8
   and perform the separate mask `0x20000` target cleanup.
3. Clear actor masks `0x10000/0x20000`. Assist admission requires current
   actor mask `0x8000` clear and retained entry mask4 clear. That ordinary
   branch increments **actor** `+0x28`, not battle counter `+0x28`.
   `0x7100057B20()` must return0; nonzero sets battle bit7 and bypasses
   both scheduler calls.
4. If actor byte `+0x31` was nonzero, set it0 when byte `+0x30` is
   nonzero or the unit is dead; otherwise decrement it1. Recheck death
   with `0x710007A7D0(calc,0)`.
5. Death, or both actor bytes `+0x30/+0x31` now being0, reaches scheduler
   argument1 at `0x710001B1AC`. Death first clears both bytes and removes
   queued orders. Before the scheduler, enqueue a child model-release
   packet when actor activity masks1/16 are both set and child-unit
   `+0x98` bit1 is set; advance secondary buffs through `0x7100073C00`.
   After the scheduler, queue the actor through `0x710005A0C0`.
6. A living actor whose two bytes are not both0 becomes current through
   `0x710005A3E0`, then calls scheduler argument0 at `0x710001B124`.
   That branch cannot schedule assist. Special-order/mask `0x8000`
   processing likewise does not call it with1.

The initial `0x710000A920` predicate requires the helper actor at battle
`+0x1B8` to be living and free of status `0x100197`; a dead enemy actor
cannot start this branch. It clears companion count `+0x1220`, then scans
active, targetable, living actions. Every eligible enemy must have status
`0x100000`; an eligible enemy lacking it returns0 immediately. Other
eligible actions, excluding the helper and status `0x100197`, append to
battle `+0x1200` with an unchecked halfword count. Success requires both
an eligible enemy and a nonzero companion count. This is a scratch-writing
predicate, not a pure boolean.

## Continuation: endState producer

`0x7100057B20` clears battle signed16 `+0x20` each call. Battle
`+0x13` bit4 forces value2 and return1. Otherwise scan actions with
activity mask1 set and action flags mask `0x20` clear. A dead calculation
unit counts as dead only when calculation flags mask `0x20` is clear;
a counted dead party ID1 clears the leader-alive predicate. Other party
genus0 actions count as living party; enemy genus1 actions count as
living enemies only when battle-unit `+0x9C` mask8 is set.

| Condition, in precedence order | Stored endState / return |
| --- | --- |
| Leader-dead predicate, or no living party and at least one counted dead action | 2 /1 |
| No living party | 3 /1 |
| `0x7100005510()` succeeds, or no living targetable enemy remains | 1 /1 |
| Otherwise | 0 /0 |

Native event: `btlCheckBattleEnd` (`0x00193060`,
`battle_inferred_001927c0.c`), including the count precedence. Special
reaction `0x7100005510` requires battle flags bit21. Encounter `0x202`
tests living enemy ID `0x102`; encounter `0x203` tests living enemy ID
`0x103`. While that enemy lives, return0. Once it does not, enqueue
result/model packets for other active living enemies and return1.
Other encounters return0. Upstream encounter-record writers remain open.

## Continuation: persistent knowledge backing

Slot `0x7100885630` is a **pointer publication slot**, not bitmap bytes.
Identified direct publication `0x7100054494` belongs to setter
`0x7100054490`. Direct caller `0x7100273514` supplies `0x7104A21030`,
which is persistent region `0x7104A1BA28 + 0x5608`.

| Transfer | Address evidence | Effect on knowledge |
| --- | --- | --- |
| Initialize persistent region | `0x7100273460`, clear call `0x7100273480` | Clear `0xA944` bytes at `0x7104A1BA28`, including all372 knowledge bytes; then bind component pointers |
| Explicit global reset | `0x71000544A0`, caller `0x71002760A8` | Clear372 bytes through published pointer |
| Serialize section | `0x7100273E24..0x7100273E6C` | Tag12, length372; copy region `+0x5608` to save-image payload `+0x56A8` |
| Restore section | branch `0x71002745B0`, case12 at `0x71002748C0` | Copy to `0x7104A21030` only when supplied length is exactly372; mismatch skips copy |
| Backup persistent region | `0x71002754C0` | Copy `0xA944` bytes to `0x7104A41508`, including knowledge |
| Restore backup | `0x71002754E0` | Copy those bytes back, including knowledge |

Wrapper `0x7100338870` forwards low32 length to `memcpy` at
`0x7100530A40`, returning the destination when both pointers are nonnull,
otherwise0. Dynamic GOT `0x710087F730` resolves to `memcpy`;
`0x710087F740` resolves separately to `memset`.

Native events in `src/g_data/g_data.c`: `datSaveBuildImage`
(`0x0010D7C0`), `datSaveRestoreImage` (`0x0010E710`) and
`datSaveRestoreSection` (`0x0010E9E0`). Native tag24 carries340
knowledge bytes; Switch tag12 carries372. Neither tag nor size transfers
unchanged. Battle teardown is not a persistent knowledge reset.

Reveal setup `0x7100086040` tests `0x71000485E0(0xFD) != -1`.
Success sets analyze-work mask `0x40` and knowledge bits0,1,2,3,4,6,7 for
the supplied actor's battle-unit character ID. The later branch at
`0x7100086510` repeats those seven setters only when that work mask is
set. Neither sets bit5. Full entry admission to the later branch,
observation `0x71000480F4`, per-entry clear `0x7100049744` and
parent-buffer aliases still require closure.

## Continuation: dispatch and alias corrections

`0x7100036800` is a **return instruction**, not a getter entry for a
caller census. Before it, `0x71000367EC` passes stack `+0x10` to
`0x7100396240`; this is not evidence of a battle-root getter.
The widened pass still infers root-tainted returns there, demonstrating
that its return summaries need qualification.

| Interior getter entry / return | Returned battle-relative address | Direct call sites |
| --- | --- | --- |
| `0x71000606A0` / `0x71000606B0` | `+0x250` | `0x710004F500`, `0x710004FCC4` |
| `0x71000606C0` / `0x71000606D0` | `+0x280` | `0x710004EDC4` |
| `0x71000606E0` / `0x71000606F0` | `+0x2B0` | `0x710004EDCC` |
| `0x7100060700` / `0x7100060710` | `+0x2E0` | `0x710004F920`, `0x710004F9A0`, `0x710004FA54`, `0x710004FA68` |

Stored-root sites `0x7100033D78` / `0x7100033DE0` use graphics
records allocated by `0x71003934E0` (zero-filled96 bytes). Callback
field `+0x10` is respectively `0x7100033E00` / `0x7100033ED0`;
record `+0x20` stores the battle root. The inspected callbacks do not
themselves read that record payload. Generic graphics-record consumers
and all packet aliases are not thereby discharged.

`0x710005FBC0` is **battle main phase** dispatch, not an unidentified
camera vtable. GOT `0x7100880698` resolves to `0x710071F8A0`:
21 initializer/updater/label descriptors, stride24. Initialization receives
battle `+0x458`; the phase number is stored there first. Index4's updater
is counter reset `0x710005C010`; index5's initializer is opening-side
event `0x710005C1C0`; index8's updater is bit29 event `0x710005D8E0`.

Camera boundary `0x710003552C` uses55 descriptors at `0x710070A8F0`,
stride32 (two callbacks, control words, label), passing battle `+0x30`.
`0x7100002560` supplies overrides when battle `+0x10` bit4 is set:

| Encounter condition | Override table |
| --- | --- |
| `0x215`, or `0x20B` with battle `+0x11F4` nonzero | B: `0x71007001D8` |
| `0x20B`, battle `+0x11F4` zero | A: `0x71006FFAF8` |
| Other encounter | C: `0x71007008B8` |

Each override also has55 descriptors, stride32. A selected override with
both callback pointers zero falls back to the default descriptor.
Indexing uses low16 with no new clamp. All220 camera pairs,42 action
pairs and21 main-phase pairs are explicitly enumerated in external
`life2-descriptors.json` / `life2-descriptors.md`. Enumerating tables does
not close each callback's transitive writes.

### Two additional bounded relative tables

Wait branch `0x7100018750`: seven unsigned bytes at `0x710062A0C8`,
index command-kind minus7, unsigned bound6, base `0x7100018754`,
scale4. Targets, as NSO offsets, are
`18758,18754,18940,187B4,187B4,187B4,18940`; default is `187B4`.
This table is necessary to reach known latch clear `0x71000187AC`.

Save branch `0x71002745B0`:21 unsigned halfwords at `0x7100638126`,
index tag minus1, unsigned bound20, base `0x71002745B4`, scale4.
Tag12 reaches `0x71002748C0`; all21 targets are in
`life2-extra-tables.json`. Both guards/tables were independently decoded.

## Continuation receipt: not lifecycle closure

`life2_census.py` adds inferred return propagation, abstract pointer memory,
wider actor/unit/calculation roots and all six descriptor tables to the
earlier pass. Its completed exploratory run took288.074 seconds,
24,652,798 CFG steps and three memory rounds, ending with205 abstract
memory cells and1,372 inferred function-return summaries.

| Analysis event, not a proven real writer | Count |
| --- | ---: |
| Reads / writes | 42,725 /16,836 |
| Calls / stored pointers | 21,271 /3,811 |
| Returns / tail-return callers | 3,718 /2,663 |
| Unlocated pointer stores | 2,734 |
| Indirect exits / dynamic calls | 1,119 /245 |
| Unsupported / narrow-pointer reports | 35 /4 |
| Unowned returns / outside-cache boundaries | 13 /23 |

Context-insensitive pointer-cell merging greatly enlarges unknown-offset
taint:6,572 distinct store sites carry unknown STATE offsets,6,497 unknown
ACTOR offsets. This is not an absence proof. Scalar/SIMD alias confusion,
unknown-offset memory reloads, tail-return composition, nonlocated stores,
imported copies and dispatch remain limitations. The first context-distinct
exploratory run was cancelled after expansion without a useful receipt;
it has no completion or closure claim.

The three-round receipt predates the two additional tables above. Its
precise actor170 list even omits independently known clear
`0x71000187AC`. A separate single-callback replay with the seven-way table
does reach it (`life2-extra-tables.json`); that is not a whole-image rerun
or a zero-frontier certificate. **No per-field lifecycle is SPECIFIED.**

Other new external receipts: `life2-knowledge.txt/.json` (six publication
slot materializations), `life2-backing.json` (five persistent-parent
address materializations, one direct knowledge backing), `life2-xrefs.txt`,
`life2-tables.txt`, `life2-summary.txt`, `life2-llvm.txt` and
`life2-extra-llvm.txt` (nine groups each, all exit0). Worker calls for
the stated predicates, save publication, graphics callbacks and getter
entries succeeded; mid-function requests were not used as entry proof.
No Steam `.arch`, keys, game/emulator, build, tests, verify, objdiff or
CI was accessed/run.

Ordinary live-state ownership remains in scope. Native `datCalcSetHp`
(`0x00231F20`), `datCalcSetSp` (`0x00231F50`), `datCalcAddHp`
(`0x002325A0`), `datCalcAddSp` (`0x00232610`), `datCalcSetBadStatus`
(`0x00232680`) and `datCalcClearFlaggedBuffs` (`0x00235020`) in
`datCalc.c` are live native mutation owners, not a complete Golden writer
census. Roster/persona/equipment events, actor activity/targetability and
candidate/selection scratch lifetimes still need complete alias, copy and
event bindings. They have not been silently excluded from closure.

### Additional synthetic vectors

| Invented sequence, not a runtime receipt | Required observation |
| --- | --- |
| Ordinary action-end, live actor30=0, actor31=1, all scheduler gates satisfied | Decrement31 to0; advance buffs; call scheduler with1 |
| Same but actor30=1 | Clear31; call scheduler with0, not1 |
| Entry battle bit7 set, otherwise eligible | Return without either scheduler call |
| Eligible follow-up branch, packet504 outstanding | Return before scheduler and before pending-state append |
| Reveal from unknown bitmap | Set0,1,2,3,4,6,7; bit5 remains unknown |
| Restore tag12 with371 bytes | Do not replace knowledge |
| Backup, mutate knowledge, restore backup | Restore prior knowledge bytes with the containing persistent region |

## Census soundness continuation: recall is not closure

The third continuation repairs concrete analysis losses and adds a whole-text
indirect-control-flow inventory. **It does not establish a sound complete
writer census, and no lifecycle field is SPECIFIED.** The earlier event totals
cannot be used as an absence proof. A table recovered inside an incomplete CFG
does not prove that all incoming paths, callbacks or continuations are known.

Two additional losses were demonstrated while replaying known writers:

* The decoder reports the first operand of compare aliases as a register write.
  At `0x7100018730`, the instruction compares the kind-minus7 index with6; it
  does not overwrite that index. Trusting this metadata kills live values.
  The exploratory engine now treats CMP/CMN/TST/CCMP/CCMN as flag-only writes.
* The old memory-value join confused an absent result with an unknown incoming
  CFG value. Joining no result with an exact relocation discarded that
  relocation. Consequently, an ordinary load through GOT `0x7100880528`
  could fail to reach the battle pointer slot `0x7100885638`. The memory join
  now preserves an exact value when the other operand is absent. This is
  distinct from keeping a constant across a genuinely unknown CFG predecessor.

Additional exploratory repairs union every potentially overlapping cell on
unknown-offset reloads, retain unlocated pointer stores conservatively,
recognize overlapping byte/vector copies, canonicalize scalar/vector register
aliases and keep narrowed pointer provenance visible. These have synthetic
analysis-kernel regressions; they are not a complete SIMD lane/copy model.
Memset-wrapper calls report their destination ranges, rather than being
silently ignored as scalar calls.

### Known-writer replay receipt

`life3_validate_writers.py` replays the owning routines without draining their
pending callees. The following sixteen checks pass. This demonstrates recall
of these witnesses, **not** the completeness of their writer sets.

| Input | Recalled writer(s), Switch virtual address |
| --- | --- |
| Battle counter `+0x28` | `0x710000AF5C`, `0x710005C030` |
| Opening side `+0x1E` | `0x710005C4A4`, `0x710005C4EC` |
| Assist selector `+0x1632` | `0x710002C63C` |
| Actor message `+0x188` | `0x710001D790`, `0x710004A900` with work pointer actor+`0x170` |
| Actor latch `+0x170` | `0x710001D7A4`, **`0x71000187AC`** |
| Battle bit29, flag word `+0x0C` | `0x710005D998`, `0x710005D9AC` |
| Knowledge set/clear | `0x71000544FC`, `0x7100054544` |
| Whole battle zero-fill | `0x7100057970`, length `0x1688` |
| Whole actor zero-fill | `0x710000A4C8`, length `0x528` |
| Whole knowledge clear | `0x71000544B8`, length `0x174` |

The two constructor checks explicitly give allocator `0x710039D750` the
context-local battle or actor allocation identity. They do not install an
unsound global rule that every allocation is a battle or actor. Setter8481
uses the established current-actor accessor result; pending script, packet
and helper calls remain in the receipt.

All thirteen previously recorded relative tables reproduce their complete
ordered target lists, including both the wait table and the save tag table.
The wait index is unsigned `(kind-7) <= 6`, not an omitted default edge.
The save table's base is set at `0x7100274514..0x7100274518`; its callee-saved
register survives the section loop. Backward resolution must include the
loop's switch predecessors to establish that invariant. Treating every
predecessor-less case as a new function entry loses that table.

### Remaining soundness obligations

`life3_dispatch.py` inventories every decoded indirect branch/call, retains
unresolved expressions, and grows the recovered switch CFG to an edge fixed
point. Unsigned guarded byte/halfword tables use the actual index stride and
element width separately. A guardless narrow load has only a conservative
full loaded-value domain, **not** a fabricated table length. Unsupported
extension/signed-width cases remain unresolved. Fixed-point stability of the
recovered graph is not global dominance while other indirect predecessors
are unresolved.

The full writer pass has **not** been rerun or certified with these repairs.
The old materialization cache is not a newly proven complete seed inventory.
General callback/table provenance, tail-return composition, byte/SIMD-lane
copies, heap/object identity and all paths through unknown indirect exits
still require closure. Retaining a frontier is necessary; deleting reports
as apparent overtaint or merely increasing the pass size is not a proof.
In particular, constructor recall and the known `0x187AC` clear cannot alone
certify actor170; persistent backing/save recall cannot alone certify
knowledge or its reveal admissions.

Raw scripts and receipts remain under `GoldenRE-notes/switch/life3*`;
`life3-dispatch-validation.json`, `life3-alias-validation.json` and
`life3-writer-validation.json` distinguish regression success from global
closure. `life3-llvm.txt/.json` independently check nine decisive instruction
groups, all exit0. No game, emulator, Steam `.arch`, keys, build, unit suite,
objdiff/verify or CI run was used. R4/R9 reader contracts and all runtime/boot
gates remain unchanged.

### Decoder input audit

`life3_audit_cache.py` freshly decodes all `1,362,188` aligned text slots:
`1,362,125` decoded entries match the cache exactly, with zero metadata
mismatches; `63` slots remain undecoded and are listed explicitly rather than
discarded. This checks cache consistency, not the correctness of the decoder.
The fresh metadata exhibits false operand writes at `45,363` CMP, `3,176` TST
and `144` CMN aliases. These are decoder metadata counts, not game writers.
The decoded control-opcode inventory is `4,498` BLR, `2,208` BR and `18,931`
RET instructions. Thus the indirect call/branch inventory must account for
all `6,706` BR/BLR sites, not just the switches reached from the old root seeds.
The undecoded slots still require explicit handling if reachable.

### Final dispatch inventory receipt

The final run accounts for all6,706 decoded BR/BLR sites:

| Classification | Sites | Meaning |
| --- | ---: | --- |
| Locally guarded relative byte/halfword table | 805 | Recovered target lists under the recorded local CFG assumptions |
| Narrow-load full-value domain only | 220 | Conservative target interval; no established table extent |
| Relocated pointer load | 14 | Pointer provenance identified, not a blanket immutability/callback proof |
| Unresolved expression | 5,667 | Explicit control-flow frontier |

Two rounds add5,987 recovered CFG edges, then no further edges; the final
dispatch computation takes81.645 seconds. **This is not a zero frontier.**
The final regression run passes13 ordered-table comparisons,11 synthetic
alias checks and16 known-writer checks. The complete fresh decoder-cache
audit takes97.972 seconds with zero mismatches. `life3-manifest.json` seals
the analysis inputs, scripts and receipts; none of these counts licenses
promotion of an OPEN lifecycle row.
