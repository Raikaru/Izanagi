# Golden BF commands: R4 static specification

**R4: SPECIFIED** for Switch patch 131072. This is a clean-room reader and
state-transition contract, not an implemented PS2 adapter or a lifted boot
refusal. Switch-only dependencies are deliberately not described as proved
Steam equivalents. No game or emulator was run. All hexadecimal Switch
addresses below include base `0x7100000000`; Steam addresses are PE RVAs in
build 10836033's ordinary `.shared`. No Steam `.arch` code is required by,
or decoded for, this contract.

The pinned NSO SHA-256 is
`854c7a1e33de1619abf009fa7f32b8c012960d685c8bf1bf99a92a6a4d0bda1c`,
build ID `66b2b6d53549a6364a43f69e282e3882f4d7028e000000000000000000000000`.
The executable's human-readable version is not established.

The separate [battle-state lifecycle follow-up](battle-state-lifecycle.md)
records writer/event findings and explicitly open lifecycle frontiers.
It does not replace this command-level contract or assert adapter readiness.

## Dispatch, arguments and PS2 vocabulary

The relocated AI descriptor table is `0x7100793D18` (294 entries); common
is `0x7100792C18` (192 entries). Each entry is a handler pointer and a
64-bit argument count. All counts agree with ordinary Steam's tables
`0xAA9730` and `0xAA85D0`, and public
[TGE AI declarations](https://github.com/tge-was-taken/Atlus-Script-Tools/blob/master/Source/AtlusScriptLibrary/Libraries/Persona4Golden/Modules/AI/Functions.json)
and [common declarations](https://github.com/tge-was-taken/Atlus-Script-Tools/blob/master/Source/AtlusScriptLibrary/Libraries/Persona4Golden/Modules/Common/Functions.json).
Types/names from that metadata are not evidence of hidden helper semantics.
All arguments of these commands are public `int` parameters; the VM transports
signed 64-bit integers, then each handler performs the narrowing below.

| ID (hex) | Switch handler | Steam handler | Arguments | BF return |
| --- | --- | --- | ---: | --- |
| 8468 (`2114`) | `0x7100027070` | `0x715B0` | 2 | integer |
| 8469 (`2115`, `AI_ACT_MSGYOSUKE`) | `0x710001DA00` | `0x6A6E0` | 1 | void |
| 8470 (`2116`) | `0x7100027280` | `0x71830` | 1 | integer |
| 8471 (`2117`) | `0x7100029E30` | `0x75340` | 2 | integer |
| 8472 (`2118`) | `0x710002C3B0` | `0x77E20` | 0 | integer |
| 8473 (`2119`) | `0x7100021E40` | `0x6E2F0` | 1 | integer |
| 8474 (`211A`) | `0x710002C440` | `0x77E90` | 0 | integer |
| 8479 (`211F`) | `0x710002C620` | `0x780B0` | 1 | void |
| 8481 (`2121`, `AI_ACT_MSG`) | `0x710001D770` | `0x6A570` | 1 | void |
| 8484 (`2124`) | `0x71000288D0` | `0x73500` | 1 | integer |
| 8485 (`2125`) | `0x71000289D0` | `0x73570` | 1 | integer |
| common 148 (`0094`, `RNG2`) | `0x71002DF590` | `0x466DC0` | 1 | integer |

`arg(k)` means the top-of-stack-first accessor `0x71002E11C0`, Steam
`0x46AC20`, PS2 `scrGetParamInt` (`0x0029CC00`). It does not pop the stack.
Given stack count N it reads position N-1-k: types 0 and 4 are immediate
64-bit integers, 1 is a binary32 value converted to integer, 2 indexes the
integer variable bank, and 3 indexes the float variable bank then converts.
Its five-way type table is `0x710063B349`; invalid count/type takes the
original diagnostic path (invalid type returns zero if the diagnostic returns).
Do not reinterpret a declared integer argument as a raw float bit pattern.

Every listed handler's **native** return is 1. This is not its BF result.
`0x71002E1450` clears VM return type byte `+0x53` to integer and writes the
64-bit result at `+0x1D0`. Void handlers do neither, preserving the prior BF
return slot and type. Current actor lookup `0x71002E1550` reads VM `+0x258`.
These helpers do not mutate the arguments or battle state.

The following names describe PS2 concepts, not offsets to transplant:

| Concept | Switch representation | PS2 counterpart |
| --- | --- | --- |
| Battle state | pointer obtained through GOT `0x7100880528` | `BtlBattleState`, `iGpffffb3ac` |
| Battle action/entity chain | state `+0x1C0`; entity next `+0x520` | `BtlBattleState.actions`, `BtlAction.next` |
| Entity identity/activity | ID `u32 +8`; flags `u32 +0x20` | `BtlAction` identity and active flags |
| Battle unit | entity pointer `+0x38` | `BtlAction.unit`, `BtlUnit` |
| Unit side and character ID | battle unit `u8 +0xA2`, `u16 +0xA4` | `BtlUnit.genus`, `charId` |
| Calculation unit | battle unit pointer `+0xCF0` | `BtlUnit.datUnit`, `DatUnit` |
| Calculation fields | flags `u16 +0`, ID `u16 +2`, affinity-row ID `u16 +4`, level `u8 +6`, HP `u16 +8`, SP `u16 +0xA`, status `u32 +0xC` | `DatUnit.flags/id/id2/level/hp/sp/bad` |
| Command work | actor `+0x48`, kind at actor `+0xAC` | `BtlAiCommandWork`, `btlAICommandSetActionCode` (`0x001DE640`) |
| Knowledge bitmap | direct backing at `0x7100885630` | `D_0079BCD8`, `datTestBitmapBit` (`0x0010F420`) |

PS2 sources are `src/Battle/btlAICommand.c`, `src/Main/Battle/Data/datCalc.c`,
`src/Main/Battle/Data/datPersona.c` and `src/g_data/g_data.c` in the decomp.
Do not conflate calculation-unit ID `+2`, affinity-row ID `+4`, entity ID
`+8`, and battle-unit character ID `+0xA4`.

## Per-function transitions

Unless listed, no battle-state write or random draw occurs. All list scans
preserve chain/slot order and retain duplicates. Preconditions are initialized
VM, valid actor/unit/table pointers, and correctly backed records. Assertion,
null dereference and unchecked array writes are specified edges, **not** false,
empty-list or clamped-index fallbacks. A native adapter must not manufacture a
successful BF result for those edges.

### 8468: status of identified entity

Decode `id = low28(arg(0))`, `mask = low24(arg(1))`. Lookup
`0x710001BBD0` scans every entity, without an activity/side/alive filter,
and returns the first whose complete `u32` identity equals id. Follow its
battle unit and calculation unit. `0x7100072450` returns the status word
AND mask; BF result is 1 if nonzero, otherwise 0. No mutation except the BF
return. No matching ID produces a null lookup result which the handler
**dereferences**, not a safe zero. Status mask zero still performs lookup.
Steam `0x715B0`–`0x71656` corroborates this envelope; its predicate dispatch
selects entry 11, `0x61A90`. PS2 analogue: status tests on `DatUnit.bad`.

### 8469 and 8481: schedule message actions

Read current actor, store `low16(arg(0))` at actor `+0x188` (message/action
parameter). 8481 also ORs byte `+0x170` with 2, preserving every other bit;
8469 leaves that byte untouched. Call the action decoder
`0x710001D200(actor, actor+0x48, selector)` with `0x800A` for 8469 and
`0x8002` for 8481. Its selected branches write only command-work kind 8 or
7 respectively at actor `+0xAC`; no command, target, plan, message window,
RNG state or BF return slot is changed by these calls. They schedule work,
not synchronously execute the eventual message action. The constant selected
branches have no further callees. Steam `0x6A6E0`–`0x6A72A`,
`0x6A570`–`0x6A5C1` and decoder `0x697F0` corroborate the writes.
PS2 decoder `0x001DE640` uses `0x8007`, not Golden `0x800A`, for kind 8;
map the **kind**, not the raw selector.

### 8470: choose an enemy with unknown affinity

Decode p as `low16(arg(0))`. Scan the entity chain. Require entity flags
bits 0 and 3 both set, side exactly 1, and a living calculation unit (status
`0x80000` clear and HP greater than zero). The additional status helper test
against `0x80000` is redundant but present. Only p in {0,1,2,3,4,6,7} can
append a candidate. For each such candidate, the bypass helper
`0x7100057640` always returns zero in this build; require the knowledge bit
for character ID n and element p to be clear. `0x7100054610` tests bit
`8*low16(n)+low16(p)` in the bitmap at `0x7100885630`, using a 32-bit
word index and within-word mask. It reads but never reveals/sets knowledge.
PS2 counterparts are `btlTargetAffinityKnowledgeBypassNoop` (`0x001F9CE0`)
and `datTestBitmapBit` (`0x0010F420`). The bitmap is an input of this command,
not inferred from the target's actual affinity.

With no candidate return -1, drawing nothing. Otherwise consume **one AI
additive-stream draw even for a singleton**, reduce modulo candidate count,
and return selected entity `u32 ID OR 0x80000000`, zero-extended to 64 bits.
There is also a null check on the selected pointer returning -1 (normal
appended entries are non-null). No target/selection flag write. Steam
`0x71830`–`0x7196C` corroborates the conditions, draw and encoding. There is
no bitmap bounds check; native backing must cover the indexed word. The
Switch candidate array has six pointers before saved registers, Steam eight;
insertion is unchecked. A seventh Switch insertion overwrites adjoining
stack storage, not a defined seventh-slot logical list result. No arbitrary
larger safe capacity or deterministic BF result after corruption is asserted.

### 8471: choose usable skill of requested element

Read `element = signed16(low16(arg(0)))`, `preference = signed32(low32(arg(1)))`.
Skill element `0x71000729B0` is compared as signed16. Build lists A and B:

1. Test skill zero's calculated element; if equal, append zero to A without
   membership, status, affordability, or target-kind checks.
2. Scan all eight learned/enemy slots (`0x7100072460`, `0x7100072470`).
   Only IDs 1–471 qualify. Require matching calculated element and usable
   predicate `0x7100026F60`. That predicate requires status mask `0x80008`
   clear, membership, and threshold trigger zero for these active IDs.
3. Skill-row byte `+0x0C == 0` appends to A; **any** other value appends to B.
   Do not limit the second list to one particular nonzero target-kind value.

If both empty return -1 without drawing. Otherwise draw an independent AI
random index for **each nonempty list**, A then B, including singleton lists.
Preference 1 prefers A; 0 prefers B. Every other signed32 preference consumes
one further draw modulo 100: 0–50 prefers B, 51–99 prefers A. Fall back to
the other list if the preferred one is empty. Return the selected `u16` skill
ID, zero-extended, or -1 if none. No actor target/action/skill write occurs.
Each local list has eight `u16` slots; A may attempt nine entries (zero plus
eight learned skills), with an unchecked ninth write into adjacent storage.
Do not deduplicate repeated skills or silently truncate A. The helper
contracts below specify every input to this decision. Steam `0x75340` and
`0x68EF0` corroborate narrowing and the selection envelope, but the complete
persona/eligibility internals are **Switch-only**; Steam protected helpers
were not followed.

### 8472 and 8474: battle-state categories

8472 reads battle-state signed word `+0x28`. If nonzero return -1. Otherwise
read halfword `+0x1E`: value 1 returns 1, value 2 returns 2. All other values
return 3 if state flag word `+0x0C` has bit 29 set, otherwise 0. 8474 returns
that same bit as 0 or 1 without the other checks. No arguments, no mutation
except BF result, no RNG. Corroborated by Steam `0x77E20`–`0x77ECC`.
These are exact raw state categories, not invented names such as victory,
escape, boss or preemption; adapters may expose them numerically.

### 8473: same-side aggregate SP threshold

Decode `percent = low24(arg(0))`. Read the current actor's side. Scan the
entity chain, requiring flags bits 0 and 3 and the same-side mask test used
by the original: form the low16 of `1 << (actorSide & 31)`, then test its
bit `(candidateSide & 31)`. For normal side values 0,1,2 this means equality;
retain the exact low16/masked-shift behavior for other byte values. Require
a living unit; no extra targetable-flag gate or ailment exclusion applies.

For every eligible unit, add current SP (`u16 +0xA`, `0x7100071730`) to C,
and **low16(calculated maximum SP)** (`0x7100071EC0`) to M. This is the
previously missing **second sum**, not max HP, count, or raw table SP.
Both accumulations and both products wrap to 32 bits. Return the unsigned
comparison `low32(C*100) <= low32(M*percent)` as 0/1. Empty sets and zero
maximum sums are not special-cased: zero <= zero is true. No state write
except BF result and no RNG. Steam `0x637D0`–`0x6389F` corroborates filters,
first sum, low16 addition and unsigned product comparison; Steam's protected
`0xCF790` denominator is not independently proved. PS2 counterpart for the
denominator is `datCalcMaxSp` (`0x00232290`), **not** `datCalcMaxHp`.

### 8479: battle-state message parameter

Store `low16(arg(0))` at battle state `+0x1632` (Switch), corresponding to
Steam `+0x161A` at `0x780B0`–`0x780DE`. Preserve all surrounding bytes;
no BF return write and no RNG. This field's exact numeric state transition
is the contract; a gameplay name is not needed to implement it.

### 8484: count opposite-side bit-29-affinity targets

Decode `signed16(low16(arg(0)))` as an **affinity index**, not a skill ID.
Build the opposite-side candidate list described below into local work.
For each candidate again require entity flags bits 0 and 3 and status
`0x80000` clear. Add bit 29 of `0x71000771D0(calculationUnit, index)`.
Return `low16(count)` zero-extended to BF. No RNG, no actor target/list
writes (the builder output is local). Statuses other than KO are allowed.
Steam `0x73500`–`0x7356A`, selector `0x61220` and callback `0x63960`
corroborate count/result envelope; affinity internals are Switch-only.

### 8485: select opposite-side bit-29-clear target

Decode the same affinity index. Build the same list **in current actor work
at `+0x100`** (persistent scratch side effect, even on failure). Retain a
candidate only when entity flag bit 0 is set, calculation status mask
`0x800FF` is clear, and affinity result bit 29 is clear. Candidate order is
unchanged. No survivor returns -1, preserving the prior selected target
`+0x48` and selection halfword `+0xAA`. One survivor needs no draw; more
than one consumes one AI draw modulo the surviving count. Then store
selected entity pointer at actor `+0x48`, set halfword `+0xAA` to 1 and
return its zero-extended `u32 ID OR 0x80000000`. No unit HP/SP/status or
knowledge writes occur. Steam `0x73570`–`0x73745` corroborates the extra
status mask, draw suppression for a singleton, result and selection writes.
Builder and affinity internals are Switch-only. The local survivor storage
has twelve pointers and unchecked insertion, subject to the backing rule.

### Common 148: `RNG2`

Consume the next **SFMT** word first (even when the argument will produce
zero), then read signed64 `arg(0)` and add one with 64-bit wrap. Convert the
unsigned32 random word and incremented signed64 bound separately to IEEE
binary32. Multiply the random conversion by exactly `2^-32`, then multiply
by the bound conversion, with binary32 rounding at each step. Truncate the
product toward zero to signed64 and write BF result. Switch
`0x710033B010`, `0x71002DF590`; corroborated ordinary Steam
`0x466DC0`–`0x466E43`. No modulo, rejection draw or other battle write.

With round-to-nearest, `u32 0xFFFFFFFF` rounds to `2^32`, so bound+1 can
be returned. Negative parameters are not clamped: -1 yields zero; -2 can
yield -1. Signed64 addition at `INT64_MAX` wraps to `INT64_MIN`. For
out-of-range conversion, Switch AArch64 `FCVTZS` saturates to the applicable
signed limit (NaN to zero); Steam x86-64 `CVTTSS2SI` produces indefinite
`0x8000000000000000` on invalid conversion. This architecture distinction
must not be erased. Integer-to-float and multiply rounding use the active
floating-point control state; all numeric vectors here assume nearest-even.

## Helper closure

### Living, status, bitmap and candidate builder

`0x710007A7D0(unit, delta)` is PS2 `datCalcIsDead` (`0x002428F0`): status
bit `0x80000` returns true; otherwise test whether signed32
`u16 HP + delta` is less than 1. All calls in these commands use delta zero.
`0x7100072450` reads `u32 status & suppliedMask`. Neither writes anything.
Bitmap and unconditional-zero bypass are specified in 8470 above.

`0x7100065DB0(actor, out, 2, 0, 0)` is the only candidate-builder
configuration used here. Initialize `out[0..0x5F]` to zero, the 32-bit word
at `out+0x68` to zero, and byte `out+0x6C` to 1. This sets count at `+0x68`
and its adjacent halfword to zero; bytes `+0x60..+0x67` are **not** cleared.
For non-null actor side 0, select side 1; for side 1, side 0; any other actor
side selects none. Scan all entities in chain order: require flags bits 0
and 3, the selected opposite side, battle-unit byte `+0x9C` bit 3, and a
living calculation unit. The builder's general side-2 exception to the
`+0x9C` test cannot be selected with these arguments. Append each entity
pointer at `out + 8*count`, increment count as u16. No bound check, shuffle,
RNG, self-exclusion or status-mask filter beyond living occurs. No other
builder mode is invoked by this task. PS2 concept: battle target-list work;
this is not a request to port the unrelated random-subset builder modes.

### Skill list, membership, persona and equipment

All these helpers are read-only on valid inputs. Enemy means calculation
flags bit 2 (`4`), not an inferred side value.

* `0x7100072460` always returns 8 (PS2 `datCalcGetTableLength`).
* `0x7100072470` returns enemy metadata row `60*unit.ID + 0xE`, or current
  persona record `+0xC`. Row IDs >=368 for enemy and party IDs >10 take
  assertions. PS2 counterpart `datCalcSkillList`.
* `0x7100071C70` compares all eight u16 slots, early returning true on a
  match; if none match and non-enemy, inspect equipment slot 2. A negative
  signed16 equipped item skips this extra test; otherwise compare the
  item's **32-bit** granted-skill field `+0x24` with the requested u16 ID.
  This extra grant does not add a ninth learned slot to 8471's scan.
  PS2 `datCalcHasSkill` (`0x00232730`) and its requested-skill inline.
* Party ID 1's active persona comes from the u16 selected roster index at
  `0x7100111BF0`'s returned base (base is loaded player-state pointer plus
  `0xA30`). `0x710007B800` returns record `base+4+48*index` only if its
  byte-zero bit 0 is set. Roster index is signed16 and nonnegative; limit
  is 12 with script flag `0x1202`, else 10 with `0x1201`, else 8 with
  `0x1200`, else 6. Invalid indices assert; inactive record returns null.
  Flag helper `0x71002E2FD0` reads the script flag bitmap. No flag is set.
* Other party IDs use `0x7100112B50`: load the unit-record base pointer at
  `0x71009F8AC0`, then add `132*id - 0x84`; persona subrecord is at `+0x54`.
  Its ID-1 route obtains current index via `0x7100111BE0` (u16 at the same
  selected-index location) and addresses the same 48-byte roster records.
  `0x710007B960` returns the active persona; `0x710007BD90` returns its
  skill list at `+0xC`. Missing persona reaches the original assertion,
  not an empty synthesized persona. PS2 `datPersonaGetRecord` (`0x0010A900`).
* `0x7100112E90(id, slot)` reads signed16 equipment ID from unit-record
  `+0x4C+2*slot`; slots 0,1,2 are weapon/armor/accessory conceptually.
  `0x7100112020` reads equipment-effect u16 at item-row `+0x28`;
  `0x7100112000` reads granted-skill u32 `+0x24`;
  `0x7100112060` reads element u16 `+0x2C`. Item stride is 68, base is the
  pointer at `0x71009F8A88`. Only membership's slot-2 route explicitly
  skips negative IDs; do not add that skip to other original readers.
* `0x71000729B0`: for active ID <472, non-null unit, skill flag `2` set,
  enemy uses its metadata row (indexed by **unit.ID**, not affinity ID)
  byte `+0x38`; non-enemy uses slot-0 item's element u16. Otherwise return
  **signed** byte zero of `SKILL0[low16(skillID)]`, stride 2. There is no
  lookup of SKILL1 beyond its active domain. PS2 `datCalcSkillElement`
  (`0x0023D8E0`); passive sentinel rules are in [skill-flags.md](skill-flags.md).

### Calculated maxima: both sums and costs use these

`0x71000717D0` (HP) and `0x7100071EC0` (SP) begin with unsigned16 base.
Enemies read metadata row indexed by unit.ID, HP `+4`, SP `+6`, stride 60.
Non-enemy base helpers `0x71001125C0` / `0x7100112640` read halfwords at
`0x710075ACCC + 44*(level & 255) + 4*signed16(id) - 44` (HP), plus2 (SP).
The level helper `0x7100071690` uses unit byte `+6` for enemies or ID 1,
and active persona byte `+4` otherwise (`0x710007B9F0`). Level zero asserts.
These raw address formulas preserve the original bank/level behavior; they
are not a universal row-bound claim.
The base corrects the earlier `0x710075ADCC` transcription error. The
99-row source is plaintext Steam `.sdata` RVA`0x90CAE0`, length`0x1104`;
it equals Switch and PS2's level1..99 payload byte for byte. See
[level-table-source.md](level-table-source.md) for the measured hashes,
structural Gate B locator and PS2 indexing-origin distinction.

Every percentage increment below is computed independently from the original
base using unsigned integer truncation; do not compound percentages or round
a combined percentage. Presence comes from the full membership helper,
including the accessory grant. The following table is exhaustive:

| Maximum | +10%, +20%, +30% skills | +50% | +100% | -50% | Equipment effect -> flat addition |
| --- | --- | --- | --- | --- | --- |
| HP | `210`, `211`, `212` | `24D` | `22C` | `22D` | `1A..1E` -> 10,20,30,40,50; `5B` -> 100 |
| SP | `213`, `214`, `215` | `24E` | `22D` | `22C` | `1F..23` -> 10,20,30,40,50; `8A` -> 100 |

Skill/effect IDs in this table are hexadecimal. Equipment bonuses are
non-enemy only, from slots 0 and 1 independently: identical effects count
twice. Add base, all increments and equipment, subtract the stated half-base
term, with 32-bit arithmetic. Non-enemies cap values greater than 999 at
999; enemies are uncapped. Consumers here take the **low16** of that result
before summation/cost multiplication. PS2 `datCalcMaxHp` (`0x00231F80`)
and `datCalcMaxSp` (`0x00232290`), with Golden passive IDs (do not reuse PS2
IDs without their `+0x20` translation).

### Active cost and threshold trigger

`0x71000781D0` reads active skill-row cost type `u8 +6`, A=`u16 +8`,
B=`u16 +0xA`, flags `u32 +0`. Types other than 1 and 2 return zero.
For HP/type1, enemies return zero; others use
`floor(A*low16(maxHP)/100)+B`, then halve by unsigned right shift when
skill `0x22B` is present. For SP/type2, enemy metadata byte 0 bit 1 returns
zero immediately. Otherwise flag `0x10` clear gives A+B; flag set gives
`floor(A*low16(maxSP)/100)+B`. If this initial SP value is zero, return zero
immediately. Otherwise non-enemy equipment effect `0x8C` in **either** slot
0 or 1 triples it **once**, then skill `0x22A` halves it. The final
non-early-return HP/SP value zero is replaced with one. Multiplications and
additions wrap u32; divisions and halving are unsigned. PS2 counterpart:
`datCalcSkillCost` (`0x0023D9B0`).

`0x7100078430` first tests flag `0x200`: set returns trigger zero before
status or cost. Otherwise status bit 3 returns 4. Then obtain cost above:
HP returns 1 when HP <= cost if flag1 clear, or HP < cost if flag1 set;
SP returns 2 when SP < cost; otherwise zero. No HP/SP deduction occurs.
This active-use slice is corroborated by Steam `0xD40F0` and specified in
[skill-flags.md](skill-flags.md). PS2 analogue `datCalcThresholdTrigger`
(`0x0023DDC0`) has a different special-effect shortcut.

### Affinity helper: complete observable dependency slice for 8484/8485

`0x71000771D0` takes signed16 index. Index -1 returns zero and indices
16,17,18 return 100 before any unit dereference, hence bit29 clear. Other
indices outside 0..15 take assertions; no valid affinity-row access is
promised. For 0..15 read E=`u16 row[index]`: enemy affinity bank, stride32,
indexed by **unit affinity ID +4**; non-enemy persona affinity bank, stride32,
indexed by active persona u16 `+2` (`0x710007C470`). Enemy affinity ID >=368,
persona ID >=256, missing persona or invalid index reaches assertions.
PS2 counterparts: `datCalcGetAffinity` (`0x0023A6B0`) and
`datCalcGetElementAffinity` (`0x00242800`).

E's high byte becomes the calculation's high flag byte, so E bit `0x2000`
initially determines bit29. No earlier arithmetic/modifier mask creates
bit29. All earlier modifier masks preserve it. The only clearing rewrite
is `0x7100077B80`–`0x7100077B88`, so the exact result used by these commands is:

* E `0x2000` clear -> bit29 clear.
* Otherwise bit29 is set, except it clears when **all** of: index 1..4;
  E `0x0800` set; unit flags `0x200` clear; relevant secondary nibble is
  1..7; and no applicable A/B/C/D rewrite below takes effect.
* Secondary nibble: index1 low `+0x22`, index2 high `+0x22`, index3 low
  `+0x23`, index4 high `+0x23`. Encoding 0 and 8..15 is non-positive
  (8..15 decode as nibble-15); these do not clear bit29.
* Rewrite A applies if its modifier is present and E `0x0700` is zero;
  B if present and E `0x0600` is zero; C if present and E `0x0400` is zero;
  D if present unconditionally. B also includes skill `0x1FB` for these
  four indices. Applied rewrites remove weakness bit27, preventing the
  final bit29 clearing. Primary buff nibbles can clear other flags but
  neither clear bit29 nor restore bit27; they do not alter this reduced rule.

| Index | A skill / equipment effect | B skill | C skill / equipment effect | D skill / equipment effect |
| ---: | --- | --- | --- | --- |
| 1 | `1DC` / `90` | `1DD` | `1DE` / `94` | `1DF` / `98` |
| 2 | `1E0` / `91` | `1E1` | `1E2` / `95` | `1E3` / `99` |
| 3 | `1E4` / `93` | `1E5` | `1E6` / `97` | `1E7` / `9B` |
| 4 | `1E8` / `92` | `1E9` | `1EA` / `96` | `1EB` / `9A` |

Equipment tests use either non-enemy slot 0/1, never an enemy equipment
row. All skill checks use the membership helper above. No RNG or gameplay
state writes occur anywhere in this affinity slice. The full helper also
reads primary nibbles at `+0x20/+0x21` and membership/equipment for other
indices; these influence its **other returned bits**, not bit29. For an
implementer reusing a full affinity calculator, its other-index dispatch is:

| Index | A skill / equipment | Other membership tests (hex) |
| ---: | --- | --- |
| 0 | `1D8` / none | `1D9`, `1DA`, `1DB`, `1FB` |
| 5 | none | none |
| 6 | `1EC` / none | `1ED`, `1EE`, `1FB` |
| 7 | `1EF` / none | `1F0`, `1F1`, `1FB` |
| 8 | `246` / `4B` | `1F2`, `1FA`, `1FB` |
| 9 | `245` / `4C` | `1F7`, `1FB` |
| 10 | `247` / `4D` | `1F5`, `1FA`, `1FB` |
| 11 | `24A` / `4E` | `1F6`, `1FA`, `1FB` |
| 12 | `24B` / `4F` | `1F8`, `1FB` |
| 13 | `248` / `50` | `1F3`, `1FB` |
| 14 | `249` / `51` | `1F9`, `1FA`, `1FB` |
| 15 | `24C` / `52` | `1F4`, `1FA`, `1FB` |

This is an exhaustive dependency **projection of the bit actually consumed**,
not a claim that R4 requires an unrelated full damage/ailment calculation.
Steam `0xD35B0`'s protected dependencies were not decoded. Switch mask branches
were independently inspected with `llvm-mc`, rather than trusting duplicate
labels/boolean reconstruction in decompiler output.

## RNG dependencies: two distinct streams

### AI integer draws: 31-word additive stream, not SFMT

Switch `0x71000715A0(n)` returns the unsigned remainder of
`0x710033AD60()` divided by n. It asserts for n=0; every valid call in
these handlers has positive count. `0x710033AD60` reads a four-pointer state
through GOT `0x7100881738`: base, front, rear, end. It updates the front
u32 to `low32(frontWord+rearWord)`, returns that word shifted **logically
right by one**, and advances both word pointers, wrapping each at end.
There are 31 words, initial front index3, rear index0. Thus the source
random value has **31**, not 32, significant bits. Steam `0x520960`–`0x5209D1`
independently corroborates addition, pointer wrap, shift and return.

Seeding `0x710033AC00(seed)` writes word0=low32(seed), then words1..30
using `low32(previous*0x41C64E6D+0x3039)`. Set front3/rear0 and perform
310 additive updates with pointer advances, discarding their outputs. The
next update is the first returned draw. Seed zero is not substituted with
one. Initialization `0x710033AAF0` allocates `0x7C` bytes, establishes
base/end pointers and seeds with `0x9A319039`; allocation failure skips
seeding rather than providing a substitute random generator. Allocator/debug
infrastructure is a normal initialized-engine prerequisite, not a new game
state dependency for each command. A command must consume the existing
stream, never reinitialize it locally. This seed initializer also establishes
the separate SFMT stream below.

### Common 148: SFMT-19937 and period finalizer

Switch `0x710033ADD0` consumes a 624-word state at `0x71051C48D4`, with
signed32 index at `0x71051C48D0`. If index >=624, refill all 156 128-bit
vectors, select word0 and set index1. Otherwise select word[index] and
increment index. A negative index is an unchecked before-buffer read, not
an implicit refill. Steam `0x520C20`, `0x520AE0` and 148's inline getter
corroborate the refill and signed threshold.

For vector i=0..155 let A be its old vector; B is vector i+122 for i<34,
otherwise already-updated vector i-34. C,D initially are old vectors154,155.
New vector is the XOR of A, A shifted left8 across all128 bits, C shifted
right8 across all128 bits, D shifted left18 **per u32 lane**, and B shifted
right11 per lane then masked with lane masks `001FFFEF`, `001ECB7F`,
`001AFFFF`, `001FFFF6`. Store it; advance C to prior D, D to new vector.
Truncate shifts to their widths; the state is little-endian lane order.

Seed expansion in `0x710033AB50`–`0x710033ABB4` writes word0=`0x9A319039`,
then words1..623 as
`low32(0x6C078965*(previous XOR (previous >> 30))+index)`, and index624.
The complete finalizer at `0x710033ABB8`–`0x710033ABF0` computes the parity
of `(word0 & 1) XOR (word3 & 0x13C9E684)`. Odd parity leaves all624 words
unchanged; even parity toggles word0 bit0, touching no other word or index.
This closes the previously unresolved SFMT period-certification dependency.
The generic seed formula also defines the synthetic seed vectors below;
the observed initializer uses the stated fixed seed.

Steam `0x520DC0`–`0x520E27` independently establishes that initializer's
seed, recurrence and index624, but its finalizer through `0x520CE0` remains
protected. SFMT finalizer equivalence is therefore **Switch-only**, not a
Steam proof. Static Switch enumeration is the specification authority for
R4; runtime/other-build equivalence remains a separate qualification.

## Indirect dispatch closure

Only these table-driven branches affect the command/helper configurations
above. Addresses in the target column are complete Switch virtual addresses.
The unrelated `8009` random-attack branch is not reached by either fixed
message-action selector, so its nested skill tables are not dependencies.

| Table | Input -> target suffix after `0x7100` |
| --- | --- |
| `0x710062B2AD`, 11 u8 offsets, base `0x710001D254` | action selectors `8000..800A`: `01D254,01D284,01D278,01D2A0,01D290,01D5DC,01D2B0,01D2BC,01D2C8,01D2D4,01D384` |
| `0x710062B820`, 7 u8 offsets, base `0x71000272E4` | p=`1..7`: `027364,027388,0273AC,0273D0,0272E4,0273F4,027418`; p0 has a separate branch, p5 goes to rejection |
| `0x710062DBFC`, 15 u16 offsets, base `0x7100077338` | affinity indices1..15: `077338,0773C0,0773D8,0773F0,077A80,077408,077444,077490,0774AC,0774C4,0774E0,0774FC,077514,07752C,077544` |
| `0x710063B349`, 5 u8 offsets, base `0x71002E1228` | VM types0..4: `2E1228,2E1264,2E127C,2E129C,2E1228` |

AI/common descriptor dispatch entries are enumerated in the opening table.
The bit29 slice has no callback escape; the builder configuration has no
indirect call. Error/assertion and platform allocation/zero-fill routines
are infrastructure leaves, not hidden game predicate replacements.

## Synthetic vectors

These are invented inputs for implementer tests, not extracted game payloads
or runtime receipts. Unless stated, records/pointers are valid, arrays fit,
rounding is nearest-even, statuses and unmentioned flags are zero. Random
queues below are **returned additive values**, after the right shift; each
consumed value advances only that stream. Assert both result and exact state
write/draw set. A void command preserves a sentinel BF result/type unchanged.
Skill/equipment IDs, field-offset suffixes, status/flag masks, E entries,
and byte/word bit patterns in this table are hexadecimal. Entity IDs,
counts, HP/SP, percentages, nibble values, random values and arguments are
decimal unless explicitly prefixed `0x`.


| Test | Synthetic input | Expected result / writes / draws |
| --- | --- | --- |
| 8468-mask | ID `0xF000002A`, mask `0xAB000008`, entity42 status `0x28` | BF1; no draw |
| 8468-first | two ID42 entities with statuses0,8; request42/mask8 | BF0; first wins even inactive/dead |
| 8468-none | no ID42, mask0 | null-dereference edge, not BF0 |
| 8469 | arg `0x12345`, byte170=`A5`, BF sentinel99 | parameter2345, kind8, byte170=A5, BF99 retained; zero draws |
| 8481 | same | parameter2345, kind7, byte170=A7, BF99 retained; zero draws |
| 8470-order | eligible IDs42,43; p1; knowledge clear; random3 | BF `0x8000002B`; one draw; selected-target fields unchanged |
| 8470-single | sole eligible42, p7, random123 | BF `0x8000002A`; **one** draw |
| 8470-known | sole42, n4,p1; bitmap word1 bit1 set (`8*4+1=33`) | BF-1, no draw |
| 8470-invalid | p5 or p8 | BF-1, no draw |
| 8470-dead | only candidate HP0, or status80000 | BF-1, no draw |
| 8472-priority | word28=1, field1E=1, bit29 set | BF-1 |
| 8472-category | word28=0, field1E=2, bit29 set | BF2 |
| 8472-other | word28=0, field1E=FFFF, bit29 set/clear | BF3 / BF0 |
| 8474 | state flags20000000 / 1FFFFFFF | BF1 / BF0 |
| 8479 | arg `0x12345678`; BF99 | state1632=`0x5678`; BF99 retained |
| 8473-equal | same-side living SP25,25; maxSP100,100; percent25 | BF1; no draw |
| 8473-over | same as above, second SP26 | BF0 |
| 8473-filter | extra opposite-side or inactive or dead unit SP999 | does not change the preceding results |
| 8473-empty | no eligible units; percent0 orFFFFFF | BF1 |
| 8473-mask | arg `0x1000000`, eligible currentSP1/maxSP100 | percent0, BF0 |
| 8473-wrap | C=65535, M=65535, percent65537 | RHS FFFFFFFF, BF1 (u32 comparison, not signed) |
| Max-SP | base101, skills213,214,215,24E,22C,22D; equipment1F,1F | 101+10+20+30+50-50+101+20 =282 |
| Max-HP | base101, skills210,211,212,24D,22C,22D; equipment1A,1A | 282 |
| Max-cap | base900, skill22D (SP), no equipment | party999; enemy1800 |
| Max-low16 | enemy base65535, skill22D (SP) | helper131070, contribution to 8473 is65534 |
| Cost-HP | non-enemy A10,B2,maxHP100 | 12; with22B ->6 |
| Cost-zero | HP A0,B0, no early enemy path | 1 |
| Cost-SP | non-enemy flag10,A10,B2,maxSP100, effect8C in both slots, skill22A | 18, not54 (tripling once) |
| Cost-SP-zero | A0,B0, effect8C and22A present | 0 (early return) |
| Trigger | flags200,status8,cost100,HP1 | trigger0 (but 8471's separate status80008 rejects) |
| Trigger-HP | flags0/1,status0,HP20,cost20 | trigger1 /0 |
| Trigger-SP | SP19,cost20,type2 | trigger2 |
| 8471-lists | A[10],B[20], pref1 /0, random queue[8,9] | 10 /20; two draws in either case |
| 8471-percent | same, pref2, queue[8,9,50] /[8,9,51] | 20 /10; three draws |
| 8471-fallback | A[],B[20],pref1, queue[9] | 20; one draw |
| 8471-fallback-random | A[],B[20],pref2,queue[9,99] | 20; **two** draws despite forced fallback |
| 8471-empty | A[],B[] | -1; zero draws, even pref2 |
| 8471-zero | skill0 element1; all learned inactive; status80008 set; request1,pref1 | returns0; one draw (skill0 bypasses usability) |
| 8471-duplicates | A[10,10], B[], pref1 | two positions, one index draw; no deduplication |
| Builder-failure | actor side2, prior target42 and flagAA=7 | no candidates; 8484 BF0, no persistent work write; 8485 BF-1, scratch initialization only, target/AA unchanged |
| 8484-status | eligible opposite targets with status1, affinity bit29 set | counts them; status1 is not KO |
| 8485-status | same targets | rejects all, BF-1, zero draws |
| 8485-single | one accepted target42, priorAA7 | BF8000002A, target42, AA1; zero draws |
| 8485-order | accepted42,43, random3 | select43, one draw |
| Affinity-none | index-1 or16..18, null unit | bit29=0, early return before unit access |
| Affinity-base | index0,E2000 | bit29=1, regardless of index1 modifiers |
| Affinity-clear | index1,E2800, unitflags0, secondary nibble1, no modifiers | bit29=0 |
| Affinity-nibble | preceding input with nibble0,8,15 | bit29=1 |
| Affinity-guard | clear input with unitflags200 | bit29=1 |
| Affinity-D | clear input with skill1DF or equipment98 | bit29=1 |
| Affinity-A | clear input with skill1DC | bit29=1; with E2900 instead ->0 (A blocked by E0100) |
| Affinity-B | clear input with skill1FB | bit29=1; with E2A00 instead ->0 (B blocked by E0200) |
| Affinity-C | clear input with skill1DE | bit29=1; with E2C00 instead ->0 (C blocked by E0400) |
| RNG2-zero | arg5, SFMT word0 | BF0, SFMT index+1; additive stream unchanged |
| RNG2-boundary | arg5, SFMT wordFFFFFFFF | BF6, not5 |
| RNG2-negative | arg-1 /-2, wordFFFFFFFF | BF0 /-1 |
| RNG2-index | index624, state allzero | refill allzero, return0, index1 (synthetic invalid-period state still follows recurrence) |
| SFMT-recurrence | A low lane1, B=C=D=0 | result lanes `0x101,0,0,0` |
| SFMT-cert | words0..3=0,0,0,0 | 1,0,0,0; words4..623 and index unchanged |
| SFMT-cert | 1,0,0,0 | unchanged |
| SFMT-cert | 0,0,0,4 | unchanged (odd masked parity) |
| SFMT-cert | 1,0,0,4 | 0,0,0,4 |
| Additive-wrap | all31 words0; front30=FFFFFFFF, rear27=3; pointers30/27 | front word2, returned1; pointers0/28; only that word changes |
| Stream-separation | insert one 148 call between two8470 singleton calls | additive cursor advances twice total; SFMT cursor once |

Synthetic seeded-stream vectors (no game run; four consecutive returned
AI values after the 310 discarded updates):

| Additive seed | First four returned values (decimal) |
| --- | --- |
| 0 | 1990395451, 1950343301, 404384196, 1972190027 |
| 1 | 2078917053, 143302914, 1027100827, 1953210302 |
| `9A319039` | 1373023817, 253234266, 2145093567, 898975974 |

| SFMT expansion seed | First four words after period certification (hex) |
| --- | --- |
| 0 | `00000000,00000001,6C078967,714ACB41` |
| 1 | `00000001,6C078966,DD5254A5,B9523B81` |
| `9A319039` | `9A319039,315A7A48,9C1AC66A,3795EF0B` |


Array-overflow vectors assert an unchecked write outside the named local
array, never an invented stable return: 8470 seven accepted candidates;
8471 zero plus eight A-list skills; builder/survivor list beyond its backing.
Invalid IDs/negative equipment outside the explicitly guarded membership
path must retain native assertion/backing requirements rather than silently
read a default record.

## Evidence receipt and limits

Raw notes remain outside the repository in `GoldenRE-notes/switch/`:
per-function RomWright output, `ai_cfg_receipt.json`, `ai_skill0_census.json`,
`ai_llvm_receipt.txt`, `ai_steam.txt`, `ai_vectors_receipt.txt` and
`ai_passive_receipt.txt`. Whole-text enumeration covers `0x532430` bytes,
1,362,125 decoded instructions, 35,468 ADR/ADRP seeds and the exact 25,109
relative relocations. CFG walks, not a decompiler's possibly enormous
function-size bounding box, determine helper control-flow closure. The
applicable indirect targets are enumerated, and pivotal instructions were
independently checked with LLVM. Steam corroboration reads are section-guarded.

The remaining Steam protected internals, unknown human-readable field names,
and original behavior after invalid memory access do not turn the complete
pinned-Switch contract into a cross-build/runtime proof. R4 specification
closure does not implement an adapter, approve a different RNG stream, lift
P11's runtime integration gate, or establish dynamic BF procedure reachability.
