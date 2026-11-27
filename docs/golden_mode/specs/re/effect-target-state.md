# EFFECT0 timing and packet actor-state bindings

**Field/event correspondence specified for the pinned Switch build.** This
addendum resolves the two target-state timing inputs and the actor-header
genus override used by [effect-fields.md](effect-fields.md).
It is a bounded producer/binding specification under the lifecycle reframe,
not a claim that the still-open whole-image battle alias census has closed.
All Switch addresses use base `0x7100000000`. No Steam `.arch` was inspected.

## Binding decision

| Golden action/target field | Meaning and PS2 binding |
| --- | --- |
| Signed32 `+0x17C` | Reflected/special-hit marker. Bind to **`BtlAction.targetBlock.specialHitFlag`, PS2 `+0xE4`**, not reactionFlag at `+0xE0`. |
| Signed32 `+0x184` | **Golden-only party-interception result marker.** Preserve as result-scoped Golden sidecar state; no corresponding PS2 target member is established. Do not alias specialHitFlag, reactionFlag or message.blocked. |
| Byte `+0x1D` mask01 (word `+0x1C` mask0100) | Actor-header presentation override. Bind to **native `BtlAction.flags`, PS2 `+0x18`, mask0100**. This is not native DB/resultFlags. |

The adjacent layout is decisive, rather than a constant offset translation:

| Switch | PS2 `BtlAction` | Role |
| --- | --- | --- |
| `+0x178` | `+0xE0` reactionFlag | Reaction/result classification |
| `+0x17C` | `+0xE4` specialHitFlag | Reflection/special-hit path |
| `+0x180` | `+0xE8` message.blocked | Skill-admission/message-block flag |
| `+0x184` | No PS2 target member established | Golden interception marker |
| `+0x188`, `+0x18A` | `+0xEC`, `+0xEE` | Message code and signed auxiliary message |

The native declaration is `BtlActionTargetBlock` in
`decomp/include/btl_action_internal.h`: reactionFlag E0, specialHitFlag E4,
then the eight-byte message union E8..EF. The producer correspondence below,
not those names alone, establishes the binding. In particular, Golden184 is
not the native message.blocked word: Golden180 already has that producer.

## Golden17C: writers and native correspondence

The principal producer is selected-skill resolution `0x710004AEC0`.
Its PS2 counterpart is `btlTargetResolveSelectedSkill` (`0x001F14F0`,
`src/Battle/btlTarget_inferred_001f1030.c`).

| Event | Switch writer | Required value/semantics |
| --- | --- | --- |
| Action allocation | `0x710000A4C8` | Zero the `0x528`-byte allocation, including17C and184. Native `btlActionCreate` (`0x001B0930`) likewise zeroes its action allocation. |
| Begin selected-skill result preparation | `0x710004B038` | The wide zero initializes the acting action's178..187, including17C and184. |
| Begin each selected target's result preparation | `0x710004B28C` | The same wide zero initializes that target's178..187. The native resolver separately clears E0/E4 and the blocked/message work for actor and targets. |
| Original hit resolver returns low16 `0x200` | `0x710004B39C` | Store1 at the **original selected target's**17C, even though subsequent calculation is redirected to the attacker. Re-resolve attacker against itself with the additional flag2. Normalize the second result200 to100, and2/4 to1; those normalizations do not undo the marker. |
| Golden interception rewrite below | `0x710004BF8C` | Clear17C and180 together;184 remains1. |
| Existing follow-up substitution rewrite | `0x710004C23C` | Clear17C and180 of the original target as its hit result is replaced. Native resolver's follow-up substitution similarly resets specialHitFlag E4 and message.blocked. |

In the native `btlTargetResolveSelectedSkill`, the first result200 redirects
the calculation to the attacker, performs the same second-result normalization,
and writes the original target's **specialHitFlag E4 = 1**. It also forces the
special-hit repetition counts to one. Golden's17C has that same reflection
producer and repetition behavior (`0x710004B2BC..0x710004B3A0` and the following
special-hit count branches). ReactionFlag E0/Golden178 instead has the separate
status-mask and terminal-hit producers; it is not the field to read for17C.

Native consumer evidence agrees: `btlActionAttackUpdate` (`0x001A59A0`,
`src/Battle/btlAction_inferred_001a1450.c`) reads target specialHitFlag E4 when
scanning targets, choosing reflected focus, and adding reflected-hit packet
timing. The same native function reads reactionFlag E0 separately. Therefore
using E0 for Golden17C would conflate two live results.

## Golden184: complete admission of the identified producer

`0x710004AEC0` evaluates the interception admission after preparing the
ordinary target results (`0x710004BDC4..0x710004BE64`). This is not an EFFECT0
flag setter and is not a script-command default. Admission requires **all**:

1. Predicate `0x7100047C40()` succeeds. It rejects battle flag-word `+0x0C`
   bit31 already set; otherwise script flag `0x38` must be set, progression
   selector5 is requested through `0x710011EA60`, and the unsigned16 result of
   `0x710011BA50` must be **greater than8**. A value8 does not pass.
2. The acting action's battle unit genus is1 (enemy).
3. The selected skill's SKILL1 byte `+0x0C` at stride `0x2C` is1 or2.
4. The selected-target list is nonempty and **at least one** target has
   battle-unit genus0 and reaction word `+0x178 == 1`.

The raw flag/progression conditions above are the contract; no social-rank
name, English skill label or broader interpretation of reaction178 is inferred.
The producer is a Golden party-result interception, not an alternative name
for an ordinary reflected hit.

### Native primitives for the flag/community admission

The native binding for the gate is **`datGetFlag(0x38)`**, followed, when
set, by **`cmmCommunityForArcana(5)`** and then
**`cmmCommunityRecordValue(returnedId)`**, comparing the latter unsigned16
result with8. These are live persistent-data services, not sidecar defaults.

| Golden primitive | Native PS2 primitive and evidence |
| --- | --- |
| `0x710002E2FD0(0x38)` | `datGetFlag` (`0x00106330`, `src/g_data/g_data.c`): query the persistent global flag bitmap at flag index38h. |
| `0x710011EA60(5)` | `cmmCommunityForArcana` (`0x00247CB0`, `src/cmmMisc.c`): selector5 first chooses community ID3, falls back to4 if3 has no record, and returns0 if the chosen ID has no record. |
| `0x710011AE00(id)` | `cmmCommunityHasRecord` (`0x001077F0`, `src/cmmCommunity.c`): record-presence test, not a nonzero-value test. |
| `0x710011BA50(id)` | `cmmCommunityRecordValue` (`0x00107AC0`, `src/cmmCommunity.c`): return record unsigned16 at+6, or0 for no record. |

The call at `0x7100047C6C` leaves the selected community ID in argument/result
register X0; `0x7100047C70` forwards that ID directly to the value getter.
The getter's worker output omits its argument, but its instruction call to
record lookup `0x710011AAB0` preserves this incoming X0. Do not query community
ID5 directly or discard the selector's fallback result.

For this admitted domain0/3/4 the native and Golden selection/value semantics
agree. This does not assert equal global community-ID bounds: Golden's record
lookup checks against23h, whereas the PS2 routines check against1Fh.
No social-rank English name or alternative meaning of the value is inferred.

| Record presence/value | Required gate result, assuming flag38h set and battle bit31 clear |
| --- | --- |
| ID3 exists with value8; ID4 exists with value10 | False: choose3 and compare8; do not prefer4's higher value |
| ID3 absent; ID4 exists with value9 | True |
| Both absent | False: selected ID0 yields value0 |

Once admitted, the loop at `0x710004BF10..0x710004BF94` rewrites **every genus0
entry in that selected-target list**, not just the entry that satisfied the
reaction test. Other genera are skipped. For each rewritten target:

| Writer | Result |
| --- | --- |
| `0x710004BF5C` | Wide publication at184 sets **184=1**, message188=0, auxiliary18A=-1. |
| `0x710004BF68` | OR first-hit row flags `+0x1AA` with `0x400`. |
| `0x710004BF70` | Set hit-row count byte171 to1. |
| `0x710004BF74`, `0x710004BF78` | Clear first row's word18C and word19C. This is **not** a whole-row clear; do not infer that SP delta190 was cleared. |
| `0x710004BF7C`, `0x710004BF80` | Set first reaction byte1A8 toFF and result flags byte173 to0. |
| `0x710004BF84` | Set hitOutcome174 to0100, targetFlags176 to1, and reaction178 to0. |
| `0x710004BF8C` | Clear17C and180. |

After the loop, `0x710004C0A0` ORs `0x80000000` into the battle flag word.
The admission predicate then rejects another interception while that bit stays
set. A new battle allocation starts with that bit clear. Preserve the one-shot
admission while this latch remains set; do not recreate184 per query or invent
another latch-clearing event.

### Reset and persistence

Both184 and17C start0 with action allocation. The ordinary actor/target result
preparation wide stores B038/B28C clear them for the next selected-skill
resolution that processes that action. The interception rewrite sets184 and
clears17C in the same result lifetime. The per-turn message helper
`0x710004A900` clears only the halfword188, so it **does not clear either
marker**. Keep the sidecar associated with the live target result and preserve
it through the native packet/timing consumption; do not initialize it to zero
inside the EFFECT0 reader. General result copies must carry the corresponding
Golden marker, rather than rederive it from a PS2 message or status field.

The native resolver's ordinary E0/E4/message initialization, reflection and
follow-up substitution are present, but its result block has no inserted
interception word between message.blocked and messageCode and no matching
Golden admission/rewrite branch. This establishes the sidecar decision for
this projection; it does not assert a whole-game negative alias census.

## EFFECT0 timing integration

The existing timing formulas remain unchanged. The any-target scan for184 is
at `0x7100014E14..0x7100014E30`; target-specific184 testing is at
`0x710001574C..0x7100015750`. For the later target path, interpret
`17C == 0 && 184 != 0` as **not reflected and marked by Golden interception**.
Do not read native E4 for both conditions. Native E4 supplies17C; a live
Golden result sidecar supplies184. Native reactionFlag E0 is a different input.

Synthetic vectors (invented, not gameplay receipts):

| Event/input | 17C | 184 | Required observation |
| --- | ---: | ---: | --- |
| Fresh allocation or ordinary result setup | 0 | 0 | No qualifying184 target from this entry |
| Reflection result200, no interception | 1 | 0 | Native specialHitFlag E4 is the correct17C binding |
| Interception gates pass; rewritten genus0 target | 0 | 1 | Qualifies for184 lead and non-reflected target path |
| Same gates, but progression result8 | Ordinary result | 0 after setup | Do not admit interception |
| Same gates, battle bit31 already set | Ordinary result | 0 after setup | Do not repeat interception |
| Rewrite list contains two party targets and one enemy | 0 on rewritten entries | 1 on both party entries | Do not mark only the original triggering party entry or the enemy |
| Message-only clear after a marked result | Unchanged | Unchanged | Message clearing is not marker clearing |
| Marker1,17C0,EFFECT8=-12,target index2,flag800 | 0 | 1 | First lead12; qualifying target delay10 |

## Actor-header genus override: Golden1D mask01

The correct input is **byte `+0x1D`, mask01**, equivalently bit0100 of
the little-endian action-header word at1C. It is not a resultFlags bit.
Switch `0x71000134DC..0x71000134EC` loads that byte and forces the local
signed genus to-1 when the bit is set; otherwise the real signed genus is
retained. This does not modify the unit's stored genus.

**The native header correspondence is confirmed:** use `BtlAction.flags`
at PS2 `+0x18`, mask0100, not `BtlSkillAction.stateBits` atDB.
The native header declaration locates flags at18. More importantly, its
producer and reset event match the Golden header's lifetime:

| Event | Golden writer and trigger | PS2 evidence / binding |
| --- | --- | --- |
| Allocation | `0x710000A4C8` whole-action zero | Native `btlActionCreate` (`0x001B0930`) zeroes flags as part of allocation. |
| ASSIST completion | `0x7100017B3C` ORs0100 when no packets506/800 remain **and** action-UID packet lookup `0x710005AB40(uid,3FFFFFFFFFFFFFFF)` returns null; then enter action state11h | `btlActionAssistUpdate` (`0x001AC5B0`, `btlAction_inferred_001a1450.c`) ORs native flags0100 after packet506/800 checks and selects native state0Fh. Golden additionally waits on the UID lookup; do not omit that gate. |
| YELL completion | `0x7100017FC0` ORs0100 on the same packet506/800 and UID-null conditions, after its dialog work reaches completion state2 | Golden's action descriptor25 names this YELL (`0x7100017F10`). Route this additional Golden event to the same live native header bit; it is not a resultFlags write. |
| End initialization | `0x710001AE2C` or `0x710001AEB8` clears0100 on both sides of the pending4000 branch, before the status-window callback can defer processing | `btlActionEndInit` (`0x001AFA50`, `btlAction_inferred_001ade90.c`) clears native flags with FEFF at the same event after pending4000 handling and before the callback. |

YELL's exact dialog sequencing is visible in `0x7100017F10`: battle work1508
starts at0, successful dialog readiness advances it to1, successful dialog
completion advances it to2, and only that later state reaches the packet/UID
gate above. The updater's state1 polling branch does not set0100 by itself.
These are packet-completion/header-state events, not critical-hit results.

The residual EFFECT1000 branch uses this header bit as an additional Golden
consumer. Let `g` be the real signed genus unless the override is set, in
which case `g=-1`. Let `q` be the existing predicate: skill low flag2 clear
and action word20 mask10 set. The earlier alternatives are
`g!=-1 && q==1`, `g==1 && q==0`, and `g==0 && q==0`. After those alternatives
fail, EFFECT1000 selects the residual packet branch. Thus setting the override
can admit that residual branch for a party or enemy actor without changing its
real genus. Bind the live native flags0100 input, and retain the Golden branch
logic; do not assume the unmodified native consumer already performs it.

The separate native `BtlSkillAction.stateBits` declaration atDB in
`btlActionSkillUpdate` (`0x001A7720`) aliases `targetBlock.resultFlags`.
Its Golden result-byte counterpart is173, not header1D. The native resolver
`btlTargetResolveSelectedSkill` (`0x001F14F0`) initializes and publishes that
result byte after its critical/noncritical result aggregation. Native skill
presentation passes stateBits to the result/action packet and tests its low
two bits for status-result presentation. These accesses are independent of
the ASSIST/EndInit header0100 producer/reset pair. Never use DB bit1 as the
genus override.

Additional synthetic branch vectors:

| Header0100 | Real genus | q | Effective genus / consequence |
| ---: | ---: | ---: | --- |
| 0 | 0 | 0 | 0; ordinary genus0 alternative |
| 1 | 0 | 0 | -1; residual branch, packet requires EFFECT1000 |
| 1 | 1 | 1 | -1; bypass the ordinary q1 alternative, residual packet requires EFFECT1000 |
| 0 after EndInit | 1 | 0 | 1; ordinary genus1 alternative restored |
| Unchanged header, DB/resultFlags bit1 changes | Any | Any | No genus override change from the result byte |

## Residual EFFECT1000 branch: dependency and timing outputs

This closes the shared-output seam for the residual branch after the three
ordinary `g/q` alternatives fail. These are explicit Golden assignments, not
invented safe defaults for missing native initialization.

| Shared value / native consumer-local role | EFFECT1000 clear | EFFECT1000 set | Switch evidence |
| --- | ---: | ---: | --- |
| `sceneUid` | 0 | 0 | X27 zeroed at `0x7100013B04` / `0x7100013C1C`; shared colour packet stores this scalar as dependency UID |
| `chainUid` | 0 | 0 | X28 zeroed at `0x7100013B08` / `0x7100013C20` |
| Source `animPacket` | null | null | X23 zeroed at `0x7100013B10` / `0x7100013C28`; no source animation packet is constructed here |
| `timingFrames` | 0 | 0 | SP+148 zeroed at `0x7100013B1C` / `0x7100013C30`; shared follow-up delay reads it at `0x7100014AA4..0x7100014AB8` |
| Animation-end dependency UID | 0 | 0 | SP+8 zeroed at `0x7100013B14` / `0x7100013C24` |
| Damage-frame work | 0 | 0 | SP+10 zeroed at `0x7100013B18` / `0x7100013C2C` |
| Default windup-frame work | 6 | 6 | Common `0x7100013C64..0x7100013C68` writes SP+2C=6 |

The clear path is `0x7100013B00..0x7100013B34`. The set path first creates
the **real look-at-deactivation packet**, `0x710006ED10(NULL,3)`, writes the
acting action UID into that packet, and enqueues it on list1
(`0x7100013BE4..0x7100013C08`). It then initializes the same shared outputs
at `0x7100013C0C..0x7100013C40`, ORs battle flags+0C with400000 and
action-end halfword+1C with6, and joins at `0x710001499C`.
The set path additionally records its branch latch at SP+120 as1; the clear
path records0 there. It is **not** an immediate/timing placeholder packet.

Constructor correspondence: Golden `0x710006ED10` allocates packet11Bh with
work containing a unit pointer and flags, then installs its init/update/destroy
callbacks. Native `btlUnitCreateLookAtDeactivatePacket` (`0x0019E7C0`,
`src/Battle/btlUnit_functions.c`) creates the equivalent unit/flags work with
native packet119h and the corresponding hold/update/release callbacks.
Use the native constructor, not the Golden numeric packet ID or an immediate
packet. **The residual path does not copy this newly created packet's UID into
sceneUid, chainUid or animPacket.**

After either path, shared skill-colour lerp/work, tint and unit-tint packets
retain dependency **kind4, UID0**, with the actual acting action UID still
attached (`0x710001499C..0x7100014A80`). Their constructor parameters remain
unchanged:10h for the colour/tint durations and8 for unit tint. The caller does
not zero their constructor-owned delay/duration fields merely because
`timingFrames` is0.

When a follow-up action exists, its first shared movement/camera/action packets
use **kind4, chainUid0, delay0**, as shown by the SP+148 load and X28 stores.
The normal follow-up chain then replaces chainUid with its newly created
movement packet UID; do not keep zero after that normal publication event.
The corresponding native shared consumers are in `btlActionSkillUpdate`
(`0x001A7720`, `btlAction_inferred_001a1450.c`): sceneUid feeds the colour/tint
packets; chainUid and timingFrames feed the follow-up packet group.

An adapter admitting these new residual cases must carry the explicit scalar
UIDs into those shared consumers. A legacy native expression that fetches
`animPacket->uid` only because older genus branches always created a packet
must not be used as the residual UID source. Null here means no source
animation packet, not permission to create a replacement, remove dependency
kind4, skip shared packets, or attach the look-at packet's UID instead.

Synthetic join vectors:

| Residual case | Extra look-at packet | Scene/chain UID at shared join | Source animation | Timing/windup |
| --- | --- | --- | --- | --- |
| g=-1,q=0,EFFECT1000 clear | None from this branch | 0/0 | Null | 0/6 |
| g=-1,q=1,EFFECT1000 set | Native look-at deactivate(NULL,3), list1 | 0/0, not extra packet UID | Null | 0/6 |
| Either, followed by ordinary follow-up action group | As above | Initial0; later chain becomes movement UID | No fabricated source packet | Initial delay0, normal subsequent chaining preserved |

## EFFECT40000 fade context: exact two-stage lookup

This is a **Switch-specified bounded lookup**, not a new native alpha-fade
eligibility rule. Define H(A) to be `0x7100006250(A)`, where A is the current
action passed by the fade updater. There is **no106-versus10F priority list**:
106 qualifies the input action;10F is the sole unit-search key.

The complete lookup sequence is:

1. Read encounter selector through `0x7100046D80`. If it is not206h, return
   null. Otherwise, the supplied action's unit pointer at38 must name a unit
   whose genus byteA2 is1 and ID halfwordA4 is106h; either mismatch returns
   null (`0x7100006260..0x7100006284`). The helper does not add an input-null
   guard before these accesses.
2. Call `0x710006F200(1,10Fh)` (`0x7100006288..0x7100006290`). This selects
   the unit-group lookup head at battle `1D0 + (u16(1)<<4)`, i.e.1E0, and
   follows each unit's CF8 link. Return the **first** node whose ID halfwordA4
   equals10Fh, or null at the end (`0x710006F200..0x710006F22C`).
   Selection of group1 is the genus selection; the loop does not add another
   test of each node's genus byte, activity, targetability, HP or status.
3. Pass that exact returned **unit pointer**, including null, to the
   action-for-unit lookup `0x710001BBA0` (tail transfer at `0x710000629C`).
   Start at battle action head1C0, follow action links520, and return the
   **first** action whose unit pointer38 is pointer-identical to the argument,
   or null when the action chain ends. This is not an ID/UID comparison.
   There is no action-activity/death filter and no search for an action106.

Consequently, duplicate10F units are resolved by unit-chain order first.
Only that selected unit is converted to an action; the code does not try a
later10F unit because the first has no associated action. Multiple actions
for the selected unit are resolved by action-chain order. Missing10F does
not fall back to106. The null unit result is **not** short-circuited: if an
action with null unit exists, step3 can return that action. Do not add a safe
null fallback that changes this behavior.

In both fade candidate loops, after their other candidate checks and the
EFFECT40000 actor-self exclusion, the updater evaluates **A == H(A)**.
This is an action-pointer comparison, not candidate-unit equality or a
broader test for an actor whose ID is106 or10F. The call/comparison sites
are `0x710006241C..0x7100062428` and `0x7100062560..0x710006256C`.
Only equality reaches the candidate colour writes. Preserve this comparison
even when ordinary distinct106/10F records make it fail.

| Synthetic live-record arrangement | H(A) | Equality / colour-write gate |
| --- | --- | --- |
| Encounter206; input A owns genus1/106; distinct unit10F has action B | B, not A | False, even when both actions are alive/active |
| Same, but A owns10F rather than106 | Null from input qualification | False for non-null A;10F is not an alternative accepted input ID |
| Input106; first10F unit U has no action, later10F unit V has action B | Null from action search for U | No retry with V or fallback to A |
| Input106; no10F unit; action Z has a null unit | Z from null-pointer identity search | Preserve that returned action, not an invented null result |
| Input106; encounter is not206 | Null before unit/action search | False for non-null A |

These lookup rules do not broaden the attack target list, change the existing
actor-self fade exclusion, or remove the packet's timer/completion work when
no candidate colour write occurs. The native alpha-fade implementation has
no equivalent context lookup; its adapter must implement this bounded Golden
selection and retain the exact returned-action comparison, rather than infer
an ID-based native eligibility shortcut.

## Static receipt and scope

Fixed RomWright analyse/worker calls succeeded for `0x710004AEC0`,
`0x7100047C40`, `0x710004A8F0`, `0x710004A900`, `0x7100013360`,
`0x7100017AF0`, `0x7100017F10` and `0x710001AE10`. Sixteen independent LLVM
instruction groups in external `m7a-target-llvm.txt/.json` all exit0 and cover
the target stores/reads and actor-header override and producer/reset instructions.
Native declarations and the corresponding native producer/consumer functions
were read directly. No build, game/emulator, Steam `.arch`, key access or CI run
was used.

Raw notes remain in `GoldenRE-notes/switch/m7a_target*` and the corresponding
worker outputs. A literal displacement scan also found camera objects with a
member184 (`0x7100007E50`, `0x710003E6D0`); those use camera/unit-pointer layout,
not BtlAction+38, and are **not** writers of the target marker. The first request
at `0x710003DEBC` decoded an undefined instruction and was discarded, not used
as an entry or writer proof. This bounded addendum does not reopen the stopped
whole-image lifecycle census.

Community-binding follow-up receipt: fixed-worker analyse/decompile calls for
`0x710011EA60`, `0x710011BA50` and `0x710011AE00` succeeded. Five additional
LLVM groups in external `m7a-community-llvm.txt/.json` all exit0. The seven-entry
relative selector table at `0x7100631720` independently routes input5 to
`0x710011EAB8`, the ID3/fallback4 path. The flag-query worker at
`0x710002E2FD0` returned exit1 with no output after successful analysis; its
bitmap operation was established from instructions and independently checked
with LLVM instead. Native `datGetFlag`, `cmmCommunityForArcana`,
`cmmCommunityHasRecord` and `cmmCommunityRecordValue` sources were read directly.

Residual-output follow-up receipt: the fixed worker for `0x710006ED10`
succeeded; its void-return annotation was not trusted, since instructions
preserve the allocated packet pointer in X0 through return. Five additional
LLVM groups in external `m7a-residual-llvm.txt/.json` all exit0, independently
checking both residual initialization paths, the shared kind4/UID stores,
the follow-up timing load and the look-at packet constructor. Native
`btlUnitCreateLookAtDeactivatePacket` and the native shared consumers were
read directly. No game/emulator or build verification was performed.

Fade-lookup follow-up receipt: fixed-worker analyse/decompile calls for
`0x7100006250` and `0x7100062280` succeeded. Seven additional LLVM groups
in external `m7a-fade-llvm.txt/.json` all exit0, checking the input106 gate,
unit10F search, unit-pointer-to-action conversion, both exact action-pointer
comparisons, and candidate colour-write blocks. The unusual null-unit
conversion and the lack of activity/HP filters are retained from the
instructions; no native eligibility expansion was inferred.
