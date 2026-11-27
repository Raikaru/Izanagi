# R2 interception: common resource installation and native helper bindings

**SPECIFIED from Steam build10836033 ordinary `.shared` code/data, the supplied
Steam CPK, and Switch1.2 ordinary NSO code. Native binding/runtime qualification
remain pending.** Complements the [packet binding](effect-fields.md#0x800-interception-target-packet-binding-addendum).
This is the shared M7aR2/[M8f](m8f-battle-effects.md) resource contract; do not
create independent interception-resource copies or substitute a PS2 slot.

## Golden-only common interception formation

### Exact asset and handle identity

| Property | Observed value |
|---|---|
| Complete source | **`data.cpk::battle/btleff.bin::R_BLOCK.BED`** |
| Container | PAK-v2, **49 entries**, zero-based ordinal48 is R_BLOCK.BED |
| CPK member | Stored111,968B, extracted398,888B; compressed member at CPK offset12,154,880 |
| Extracted btleff.bin SHA-256 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| R_BLOCK.BED | Payload offset270,440 within extracted btleff.bin; size128,448B; header `BED400\0\0` |
| R_BLOCK.BED SHA-256 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| Steam common handle array | Battle-work `+0x1310`, stride8, 49 handles; slot48 is **+0x1490** |
| NSO common handle array | Battle-work `+0x1328`, stride8, 49 handles; slot48 is **+0x14A8** |
| Consumer | Ii constructor at Steam `0x56B3C` / NSO `0x7100015774` loads exactly that slot |

**The slot holds a formation-work handle, not the BED byte pointer.** Its data
pointer addresses R_BLOCK.BED inside the retained archive copy. Passing raw BED
bytes as an already-created formation handle is not the same operation.

The supplied PS2 projection has `BtlBattleState.formations[48]` atD04, legal
indices0..47. Its loader `btlEffectLoadTable` (`001D7760`,
`src/Battle/btlEffect.c:248-270`, pin18016afe6) loops48 times. Golden positively
extends the common table to49. **Native PS2 slot48 does not exist.** In the
Golden archive, slot9 is HANSYA.BED and slot10 KYUSYU.BED; neither is R_BLOCK.
Do not reinterpret formations[9/10], write past the48-slot native array, or
change the PS2 guest layout to simulate the new slot.

Native Golden mode must expose a **Golden-owned common-interception formation
handle** (logical common index48), installed from the exact resource above.
The original48 native slots may retain their physical layout; a separate typed
Golden sidecar owns the extra handle and its common-resource-generation lifetime.
M7aR2 consumes this handle through M8f's common-resource owner, not a local
PS2-address alias or a guessed formation fallback. Golden routing of entries0..47
remains coherent with the same loaded Golden archive.

### Loader, installation and ready publication

Native module counterparts are `btlEffectLoadTable`, `btlFormationAllocWork(2)`
and `btlFormationSetSlot`; these are operation mappings, not permission to copy
Golden pointer-bearing work structures into the PS2 image.

**NSO `0x710004A4E0..4A62C`:** the literal at `0x710061633A` is exactly
`battle/btleff.bin`; references4A51C/4A534/4A60C. If the live request at
battle+14B0 exists, poll `398080`; if absent, first query already-loaded data
through `398470(path,&size)`, otherwise start `397D20(path,0)`, save the request
at14B0 and return0. Pending polls return0. Once available, allocate/copy the
complete extracted archive into **battle+14B8** (4A548..4A578).

At4A580..4A5C8, x22 runs **0x265 through0x295 inclusive**. Entry index is
`x22-0x265`; `396FA0(archiveCopy,index,&size)` resolves its bytes. Allocate a
formation work with `328A0(2)`, store at `battle+x22*8`, and set its data with
`32FD0(handle,entryBytes)`. Thus index48 installs at `0x295*8 == 0x14A8`.
After all49 installations, release any outstanding archive request at4A5D8,
zero14B0, OR **battle flags+0x0C mask0x40000**, return1 (4A5CC..4A5F0).
The copy at14B8 is deliberately retained.

**Steam `0x9B160..9B334`:** the same literal is at RVA917FB8. Lookup `4EB170`
returns loaded bytes/size; otherwise `4EA890(path,0)` supplies the request at
**battle+1498**. Its ready state is checked against0x0C at9B1DE. Allocate/copy
into **battle+14A0** at9B20D..9B24D. The loop9B25D..9B2D5 starts at
array offset1310, increments by8 and stops after **0x31** entries. Each work
is0x848 bytes, initialized with control word0x10008 at+838 and the selected
entry pointer at+840. Index48's store is therefore1490. Release/null the request
at9B2D7..9B308, publish flag0x40000 at9B313, return1.

The native Golden installation is complete only after the common owner has
published all49 handles and the ready state. Do not install an arbitrary valid
handle merely to make Ii non-null, publish readiness while R_BLOCK is missing,
or retain a pointer into a temporary extraction buffer. M8f must use the actual
Golden BED400 resource route, not assume its byte layout is PS2 BED data.

### Lifetime and destruction

This is a **battle common-resource generation**, not per-action, per-target,
per-result or per-turn state. The normal loader's handles borrow entry bytes
from the retained whole-archive allocation. Destroying only the asynchronous
request after installation must not free that copy.

| Event | NSO | Steam | Ownership action |
|---|---|---|---|
| Common-load packet constructor | `4A440..4A478` | Native counterpart `btlEffectCreateLoadPacket` | Type306, no work; clear header flagbit0; init4A480, update4A4E0. |
| Release/reset archive ownership | `4A480..4A4DC` | `9B0F0..9B154` | When resourceFlags bit0x04000000 is clear, release non-null request and free non-null retained copy. Both request/copy pointers are nulled on either branch. |
| Common handle clear | `49310..493E8` | `9AFC0..9B0E0` | Iterate **all49**, destroy each non-null formation handle and zero its slot; clear ready flag0x40000. |
| Normal common-copy ownership, resourceFlags bit26 clear | `49334..493CC` | `9B000..9B088` | Destroy handles first, then release/null request and free/null the single retained archive copy. |
| Existing per-entry ownership mode, bit26 set | `49364..493A4` | `9B090..9B0C8` | Free each non-null handle's individual data pointer (+840), destroy that handle and null its slot. Do not free these as one borrowed archive allocation. |

ResourceFlags is NSO/Steam battle+14; the NSO byte+17 bit2 test is its bit26.
Keep the extra Golden handle in these same real common-clear/reload events and
preserve existing effect-packet lifetime/counters. Do not clear it when184 or
lead is reset, or free/reload its backing while live type301 work still uses it.
No new early-free event or independent M7a-owned archive cache is authorized.

## Native helper mappings for the interception followers

Native symbol references below describe the existing PS2/native operation at
pin18016afe6. Golden payloads, pointer widths and callbacks must retain the
paired Golden behavior; a legacy PS2 declaration is not a Golden work ABI.

### Channel cue: type902

Use **`btlSoundCreateChannelCuePacket(4,2,0x0B)`** (PS2 `001F7C20`,
`src/Battle/btlSound.c:96-111`). NSO63120..63178 and SteamBCB70..BCBD9
both allocate type902/six-byte work, storing three u16 values `(4,2,0x0B)`.
The caller enqueues with dependency kind5/Ii.uid and queue argument1.
This is not a unit-message packet or the interception formation itself.

This is a **Golden-mode projection of that native factory**, not permission to
reuse every PS2 callback unchanged. Golden's init is intentionally audio-empty:
NSO63180 returns; SteamBCAA0 only reaches its instrumentation thunk. Unlike
PS2 `btlSoundRefreshSlotChannel` (`001F7AD0`), it does not issue the old
stop-stream request when the selectors differ.

Golden update **NSO63190..63234 / SteamBCAB0..BCB63** compares the live cached
cue/variant, using Golden battle offsets **FAC/FAE + 8*channel**. On a match it
returns1 without replay. On a change NSO calls `3A36D0(1,channel,cue,variant)`,
the native **`sdkSndStartStream`** operation used by
`btlSoundSetSlotChannel` (`001F7B40`); it caches the two selectors, zeroes the
**float age** at **FB0 + 8*channel** to0.0, and returns1. Do not retain the PS2
six-byte cache stride or its init-stop side effect as the Golden ABI.

Steam performs the equivalent request through its audio backend: it computes
`(s16(cue)+200)*100+s16(variant)`, looks up that cue in the live selected source,
then plays it on channel4. For this exact tuple the backend cue is **20211**.
It publishes the same Golden cache values/zero float age. This is distinct from
the bank0x0D/channel3 voice request below. M8's real audio route must preserve
the request/cache semantics; an unconditional replay or completion-only packet
does not implement the source.

Callback span digests, end exclusive: NSO63180..63280
`84a73d2272166e6698cc572a4d08b6b3e60404b1b52a4bf4b3a3e5534a2714d2`;
SteamBCAA0..BCB70
`766aa6240541a01a1d8b569a27acd1544b4c09d9f450d56a5d9168a4173795d3`.

### Channel-cache domain, reset and expiry

The cache has **16 rows**, storage indices0..15, each
`{s16 cue, s16 variant, f32 age}` at FAC/FAE/FB0, stride8. The902 constructor
accepts an unchecked u16 channel; the16-row allocation/reset is not a new
constructor bounds check. Interception uses row4. These are cache slots, not
a claim that every backend exposes16 physical playback channels.

| Producer/event | NSO | Steam | Exact cache action |
|---|---|---|---|
| Reset | `62D20..62DC4` | `BC3D0..BC42C` | For all16 rows, cue=-1, variant=-1, **age=3.0f**. NSO stores packed `0x40400000FFFFFFFF`; Steam explicitly loops0x10 times. |
| Changed902 request | `63190..63234` | `BCAB0..BCB63` | Cache the requested selectors and set **age=0.0f** after issuing playback. An equal-selector hit does not refresh age. |
| Existing sound-queue advance | `62DD0..62EB0` | `BC430..BC4C9` | For each row with age>=0, add **0.5f * live frameStep**. If the resulting age>=3.0f, set both selectors=-1 and **age=-1.0f**. Negative ages are skipped. |

Thus reset age3.0 expires to-1 on the next normal advance; it is not an
initially active cue. Expiry is **>=3**, unlike506's strictly-greater duration
test. At step1 a newly played row needs six advances; at step2 it needs three.
Do not preserve the native signed8 age or invent a fixed three-update expiry.
NSO calls277BB0 for each active row; Steam snapshots B2B310 once for the loop.
Both read the shared frame publication specified below, not separate clocks.

The native owning lifecycle is **`btlSoundResetSlotQueue` (001F7260)** /
**`btlSoundAdvanceSlotQueue` (001F72E0)** in
`src/Battle/btlSound_inferred_001f6100.c:788-866`. NSO reset/advance calls are
at5799C/58594. Bind the Golden typed cache at those existing lifecycle events,
without changing the PS2 six-byte guest rows or adding another packet-local
cache. Preserve the separate rotating-queue fields: FA8 resets to5; its signed8
FAA resets to-1 and still advances by integer1, becoming-1 at>=3. That byte
timer is **not** the widened per-channel float age.

### The once-per-loop 0x6AC packet is a voice cue, not a text message

**NSO6F9D0..6FA08 / SteamCE860..CE8A1** allocate **type0xA02, two-byte
work**, containing only `u16(voiceId)`. They install only the update callback
**NSO6FA10 / SteamCE830**; allocator defaults otherwise remain intact.

Native binding: author this **Golden-only immediate ID-voice packet** using the
native packet allocator and the indexed-source voice operation. There is no
qualified mapping to the existing **`btlVoiceCreateIdPacket` type0xA03**:
that native packet has a multi-step state/result lifetime and clears flagbit0.
Do not silently substitute it for A02.

The callback requests source/bank **0x0D on channel3**, plays the narrowed ID,
and returns1 **immediately after issuing the request**, not after sound playback
completes. NSO6FA20..6FA34 performs `3A23F0(0x0D,3)` then
`3A27D0(3,voiceId)`; the direct helper6F730..6F75C performs the same operation.
SteamCE830..CE857 calls the direct indexed-voice helperCC570 then returns1.
The native existing operation is **`sdkSndStartIndexedSource(0x0D,3,voiceId)`**
(PS2 `0045AE10`: channel-indexed request records store the source argument and
u16 cue, then advance that channel's request state). `btlSoundPlayIndexed`
(`001F8690`, `btlSound_inferred_001f8380.c:224-234`) is the semantic native
indexed-voice helper, but its PS2 implementation reads an existing bank field:
**do not call it unchanged with an unbound PS2 bank instead of the Golden0x0D
request**. Route the explicit request through the real Golden-aware audio owner.

SteamCC570..CC603 retains source availability/current-bank/cache policy and
channel3 playback (including4FC7C0/4FC9D0/4FD470/4FC020/4FCCE0 operations).
The facade must preserve that backend policy; this mapping does not authorize
hardcoding a bank-ready/current-bank result or inventing an audio no-op.
Neither the packet's immediate completion nor the physical sound lifetime is
changed by giving its native request a real audio owner.

At the first marked target only, preserve the existing preparation order:
NSO63C30 / SteamC07D0 presentation reset, then **`btlSoundStopBank3`**
(NSO6F760 / SteamCC610; channel3/eight-frame fade request through the respective
backend), then **one `datCalcRandomBelow(2)`** operation, then construct A02
with **`u16(0x6AC + result)`**. The paired random-width helpers are
NSO71610..71634 (uniform float, multiply by unsigned width, FCVTZU) /
SteamCF1B0..CF219 (same positive-width floating calculation and truncation).
Do not replace this with an arbitrary fixed variant, consume RNG during probes,
or draw once for every intercepted target. The reset remains before fade/RNG;
its paired63C30/C07D0 path clears battle+F24 and bit0 ofF22 and performs the
existing UI/audio cancellation calls. Preserve those real operations, not only
an integer reset. Once-gate order is recorded in the packet addendum.

The exact native reset counterpart is **`btlSoundCancelPlayback` (001F62B0)**,
`btlSound_inferred_001f6100.c:172-182`: call `fclMiscPruneGlobal()`, then
`fclMiscStopAndClose()`, then set playback.phase=0 and clear playback.flags bit0.
Its native fields are **battle+A50/A4E** respectively, from
`include/btl_effect_internal.h:183-211`: the A44 embedded playback object has an
eight-byte step prefix followed by u16 channel/flags/phase. **A44 is not the
flags field.** Golden's corresponding fields are F24/F22.
NSO63C30 calls204430/2044E0 before the two stores.
SteamC07D0..C0843 performs the corresponding UI/channel cancellation, including
the channel1 stop and live presentation-object close, before those stores.
This is not `btlSoundResetPlaybackState`, a full cache reset, or a substitute
for the **separate, subsequent** bank3 fade. Use the real native cancellation
route and its Golden field projection, not just two synthetic flag writes.

A02 gets **kind5/Ii.uid**, original action UID owner and queue argument1.
It is a sibling of the type902 follower, not dependent on that sound packet.

### Per-target unit/0xBB message: type506

The native semantic counterpart is **`btlPacketCreateEffectCounter(unit,0xBB)`**
(PS2 `00202400`, `src/Battle/btlEffect.c:705-719`) and its unit-message update
`btlPacketStepEffectCounter` (`00202340`,
`btlEffect_inferred_00202340.c:88-112`). The misleading legacy argument names
`effect,limit` do not make0xBB a timer: it is the **message selector187**.

**NSO84740..84788 / SteamDD090..DD0C4** provide the default wrapper; the
Steam wrapper passes duration **0x20** into the real constructorDD0D0.
The NSO equivalent three-argument constructor starts84790. Both allocate
**type506/work0x18**, install update **847F0 / DD020**, and initialize:

| Golden work field | Binding |
|---|---|
| `+0x00`, pointer | Current target's native unit pointer, full native width |
| `+0x08`, u32 | Code0xBB |
| `+0x0C`, float | Elapsed0.0 |
| `+0x10`, u32 | Duration32 |
| Other bytes | Allocator-zero/default state |

Do not truncate a native pointer through the old PS2 `s32 effect` spelling, or
copy a twelve-byte PS2 counter-work structure into this24-byte Golden shape.
The update calls the unit-message presenter once when elapsed==0 with
`(unit,code,duration)` (NSO83DD0 / SteamDC290), then advances elapsed by
**0.5 * live frameStep** and completes only when **elapsed > duration**.
NSO847FC..84840 and SteamDD035..DD081 independently prove this; Steam factor
RVA964C38 is raw0000003F, float0.5. Preserve the existing native message-panel
route, actual target unit and Golden selector; use the Golden timer projection,
not the old PS2 callback's unconditional integer++/33-tick approximation.
Both this callback and the channel-cache expiry consume **`golden-live-frame-step`**;
neither may substitute a fixed1/2 step or maintain its own timing producer.

The caller sets **kind5/Ii.uid**, original action UID owner and queue argument
**3**, for every marked target. It does not wait on902 or A02. Text/panel
rendering remains the real native UI implementation; the packet must not become
a delay-only acknowledgement that never dispatches its unit message.

## Shared Golden-native live-clock facade

**Main-approved seam: `golden-live-frame-step`; owner: GoldenM7aR2.** Author
the new native getter **`float p4_golden_live_frame_step(void)`**, using the
existing `p4_golden_*` native-facade convention. This is a **new Golden-only
operation, not an existing PS2 getter**. R2 binds the port's real existing
frame-timing producer; later milestones must reuse this same shared publication.
No claim is made that the getter/host binding is already implemented.

The result is the current **binary32 engine-frame step in nominal60Hz ticks**,
not seconds, milliseconds, a frame ordinal, or the already-halved30Hz effect
interval. At normal60Hz the step is about1; at30Hz about2. Consumers here
multiply by0.5 exactly once. Read the actual published value; do not return a
fixed1/2, use an unrelated rendering counter, or disguise an unbound producer
as a zero-step/delay-only successful packet.

### Source state, writers and units

| State/event | Exact source evidence |
|---|---|
| NSO owner and initial publication | Application object at `0x7104A4BED8`; constructor277E10 passes that object to30CC9C, whose30CCB4 call invokes base constructor30DD58. At30DD70, object+28 (**0x7104A4BF00**) is initialized to **f64 1.0**. Object+30/+38 initialize to f64 upper3.0/lower0.01 at30DD9C. |
| NSO live writer | Application run loop **30D3AC**, store **30D4B8** to `this+28`. It samples `nn::os::GetSystemTick` through PLT530F70, converts through `nn::os::ConvertToTimeSpan(nn::os::Tick)` at530F80, converts the two timestamps to microseconds, subtracts them, multiplies by **f64 0.00006** (`0x3F0F75104D551D69`), then clamps against this+38/+30 before publication. Thus one second corresponds to60 nominal ticks, before the source clamp. |
| NSO read | **277BB0..277BBC** reads that f64 and performs FCVT to f32. Adjacent277B90 instead returns the converted value multiplied by0.5; do **not** use that already-halved helper and multiply again. |
| Steam initial publication | RVA **B2B310** is file-initialized raw`0000803F`, f32 **1.0**, in `.xpdata` (data read only). |
| Steam live writer | **4D282C..4D28B7**, stores **4D2892/4D28A2** to B2B310. Compute60 times signed counter delta (current **ECBB08** minus previous **ECBAB0**), convert that product to f32, widen it and multiply by reciprocal f64 frequency **ECBAC8**, narrow to f32, clamp to **[0.01f,3.0f]**. If the live f32 multiplier at **ECBAF0** is nonzero, multiply the clamped step by it and republish. This multiplier is not an additional consumer-side operation or a second clamp. |
| Steam frequency initialization | **4CE99C..4CE9C3** queries the frequency via IAT8874C0 into ECBAC8; source failure path stores1. This observed source fallback is not permission to invent a host clock/frequency fallback. |
| Steam reads | Cache advance BC46A and506 update DD035 read the live B2B310 field. The half factor is f32 **0.5** at964C38. |

NSO import names above were resolved through MOD0 dynamic symbols and PLT
relocations, not inferred from a function signature. Steam counter capture
uses IAT8874B8; its API name was not recovered and is not required for the
counter/frequency arithmetic proof. Source f32 constants are RVA964AA8 lower
0.01,964F58 upper3.0,964C38 half0.5; f64 reciprocal numerator964E58 is1.0.
The Steam optional multiplier can take the published value outside the
pre-multiplier clamp; consumers must not silently clamp it again.

### Start/reset and publication ordering

- **NSO application construction/reconstruction** seeds the publication to1.0
  and initializes its bounds, as above. Run-loop entry samples the initial
  previous tick at30D3E8. Each iteration samples the new tick at30D418 and
  publishes at30D4B8 **before** calling the application's vtable+20 callback
  at30D4C0. It is an engine-frame publication, not a packet-owned stopwatch.
- **Steam capture/baseline hooks** at4D2700,4CE210 and3DB278 capture into
  ECBB08. When source flag **RVA52483B8 ==0**, they rebase ECBAB0 to
  `current-frequency` at4D2734/4CE244/3DB29B. This is a one-second counter
  delta before the normal clamp, **not** a reset of B2B310 to0 or1.
  The live writer then stores current into ECBAB0 at4D28B0 for the next delta.
- The facade's state belongs to the **real host frame-timing/application
  lifecycle**. Bind its start/restart and per-frame publication there, preserving
  the source initialization/baseline semantics and the live timing policy.
  It must be available before Golden callbacks consume that frame. Reading it
  is side-effect-free and must not resample time independently for each packet.
- Battle initialization, action/target resets, cache reset, Ii allocation and
  506 allocation **do not own/reset this clock**. The cache owns its age reset;
  each506 work owns its elapsed0.0 reset. Keep those separate from engine time.

R2 owns the host-producer binding and qualification; M8f reuses it. The existing
Main-approved **`M8f-res:battle-common-14A8-1490`** refusal for actual BED400
playback is unchanged. This timing/cancellation clarification does not claim
that the resource decoder or native runtime is complete.

## Evidence receipt and qualification

All Steam code spans were checked against the PE section table and decoded only
inside `.shared`; archive/data reads do not decode protector code. Switch uses
ordinary mapped NSO code. The member SHA values above are from the supplied
**Steam CPK**; no claim is made that an unextracted Switch BED has identical bytes.

| Raw code span, end exclusive | SHA-256 |
|---|---|
| NSO49310..493EC | `38f57ce02214d953ebb8631605011507f72a478802c67377b419cd527c6b414e` |
| NSO4A440..4A630 | `4695771869843da13e43fef8ea2493d5eefdddef7e17523aa7d1bc0030cb1edc` |
| Steam9AFC0..9B335 | `f734e1378598202a3cf5d4ec3fab67387403050bbce15cabcfc5ea693ddcbd58` |
| NSO6F9D0..6FA48 | `a6d4dfb73b11c011cbdc3a71f4c9564bcae8aafdd35bed36eb929e003b178c58` |
| SteamCE830..CE8A2 | `c99f6ba66a2a563ad2f74797bd890c666fb01d67c2635995dba90024013c5ddd` |
| NSO84740..8484C | `32550410d284333421a544dfa3a1eacb1b8b8e9308e3a2915badda55ffa6dcc8` |
| SteamDD020..DD160 | `2647992aa604e73a96d3b59a2423600fe4f5e12c9a2e6752e9a834326274dbec` |
| NSO62D20..62EB4 | `c8822491b6806dc37eae6f61a36889cd69c373ff226378236f1821458cb069e5` |
| SteamBC3D0..BC4CA | `4932734fb7b382ac40864b1df1dabbb888198e389472328368be5aa0b0000c0e` |
| NSO63C30..63C64 | `f2dcbb01abafa39322a651eaa1905ed21add645e5fa2c9c7e78ab691b640b0b8` |
| SteamC07D0..C0844 | `1103757ecda239d19600c29e5c7b932f320c966decb84579ed5ad16a081a4a39` |
| NSO277B90..277BC0 | `b06925c56a3cfe7a39a1af8b5cc9865bfe9d75f7eb9a4c04c9764add7462931d` |
| NSO277E10..277E38 | `b6d046827666b4bf570037c97586dee9464c71dc161506c8f305a50fb5a54f23` |
| NSO30CCA8..30CCB8 | `5c88724a49d567f4aa023940309512142e21371e6692d4dcb48dd330cf4e03b8` |
| NSO30DD58..30DDA8 | `fc0e381851cb6ca44fe73e1262c97c243d3b8fcd478c84e7ae6345c51cbff208` |
| NSO30D3AC..30D4F4 | `e9c9649f01f1e332250f3d5b8cc66fe519a0a5282bf10dce733c5c36004cfc15` |
| Steam4D27A0..4D28B7 | `6bd3dc6e07ab2ed39bda2c723d2589b1c2e726e1165753a7a81e983265a6ff03` |
| Steam4D2700..4D273B | `2c82bccee4cd2b1997f076cf3bd356bc58d3cacd689d7366f537bd19ae2125a6` |
| Steam4CE210..4CE24B | `2d28e1af4d0577220485d633d843060435bbff0d376860b80b086870ecaaf076` |
| Steam3DB278..3DB2A2 | `754a54edf04c2b1613b6fd60d0b20951b02e2bdc0e77f7e730548d8a4e830024` |
| Steam4CE99C..4CE9C3 | `448edc156ac0cdb17204c4924bad40bd98465d51be71a04d27bc5857fc9deed0` |
| NSO530F70..530F90 | `4f581f199c390dbb12d1a97b9260e17f291856f2896128a0b6fe1164f2e931ef` |

Private receipts/readers under `<scratch>/GoldenRE-notes/l1/`:
`l1c-m7ar2-common-identity.json`, `l1c-m7ar2-resource-instructions.{json,txt}`,
`l1c_m7ar2_common.py`, `l1c_m7ar2_resource.py`; required Git Bash/fixed Python312.
Observed source facts include CPK decompression/member framing, exact member
hashes,49-iteration indexed installation/destruction, callback fields,16-row
float-age expiry, native cancellation layout, frame-step construction/publication,
MOD0 tick imports and paired helper argument/control flow.
No builds, unit/runtime tests, emulator, game,
keys, WSL or `.arch` decoding. No existing L1 cohort row is promoted by this
addendum. M7aR2/M8f own native binding and end-to-end qualification.
