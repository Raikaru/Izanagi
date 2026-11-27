# Golden ACB sequence, track and synth playback

## Status and clean-room boundary

Research specification, 2026-11-13. This extends `../../audio.md`; it is not
an implementation, a listening test, or a claim that every CRI feature has
been reproduced. **OPEN items below remain refusals, not invented defaults.**
The scheduling and installed type/opcode census are closed for the measured
Steam install. The final DSP sections specify the full210-byte spatial path,
non-neutral authored filtering, registered wet effects and post-pan routing.
The remaining OPEN ledger is explicit. A flat waveform list is not a plan.

Inputs were the owner's Steam build10836033 ACB/AWB and authorized ACF files,
`.shared` player instructions, Switch1.2's main NSO, and local research notes.
No game execution, emulation, `.arch` decoding, third-party Atom implementation,
or retail bytes in this document. Only prose, addresses and measured facts
cross the specification/implementation boundary. Do not copy decompiler output.

Source identities:

- Steam `P4G.exe` SHA256
  `0f327c7669e6cac72aaf355181b1e51edd12fee4d71709fc1802dbf0da70546e`.
  Addresses below are **module RVAs**, not process addresses; reads stayed
  inside `.shared` (RVA0x1000, size0x885600, file offset0x600).
- Switch `main.nso` SHA256
  `854c7a1e33de1619abf009fa7f32b8c012960d685c8bf1bf99a92a6a4d0bda1c`.
  NSO VAs use base0x7100000000. Extracted earlier from the authorized update.
- Install root `C:/Program Files (x86)/Steam/steamapps/common/Persona 4 Golden`.
  Inventory SHA256
  `150454e1b60d50ccdd58346b6da4e18e363860bf49bb22a679db379dee3771f8`.
  Each of630 ACB payloads was extracted through its inventory container chain,
  SHA256 checked, then all cue roots traversed. All1,100 external AWB inventory
  objects are retained in the framing census of `audio.md`; this extension
  does not decode audio again or claim a new AWB-payload verification.

Owner-local reproducibility inputs/results (not implementation dependencies):
`<scratch>/GoldenRE-notes/acb/{research.py,census.py,census.json,nonpoly.json}`.
Final `census.json` SHA256
`e5b57e7d81f2a165aec7f64cda3e47ee4a2f22b8d7761a047da857c6755757b6`.
It contains every cue's ordered track-event occurrences, repeated waveform
edges, scalar-column distributions and every opcode payload distribution.
No global visited-set deduplication is used in the final graph traversal;
only recursion-stack cycle detection. There were **3,631 roots, zero errors,
zero graph cycles**. These are static graph counts, not simultaneous voices.

## Paired player evidence index

These correspondences were established by caller flow, command dispatch,
payload widths, endian conversions and state updates, not function-name guesses.
Raw instructions and fallible NSO decompilations remain in the local directory
above; branch conclusions were checked against instructions. A protected thunk
is a boundary, not permission to follow it.

| Contract | Steam `.shared` RVA | Switch VA |
|---|---|---|
| Track event interpreter | 0x654A50–0x657164 | 0x71003D6064–0x71003D8764 |
| Sequence track selection/construction | 0x66EE4C–0x66FE01 | 0x71003FF69C–0x7100400154 |
| Synth selection/child construction | 0x670858–0x67148C | 0x7100401740–0x7100402138 |
| Weighted random threshold | 0x66CEC4–0x66D059 | 0x71004007B8–0x710040090C |
| Sequential/shuffle selection state | 0x66DE3C–0x66E0F9 | 0x710040090C–0x7100400B9C |
| Parameter command interpreter | 0x659428–0x65B8B0 | 0x71003F9FA8–0x71003FBBBC |
| Parent/track lifecycle update | 0x652C24–0x65385C | 0x71003D4B6C–0x71003D5D58 |

Pivotal paired sites:

- Relative millisecond wait: Steam0x655A43–0x655A93; NSO's opcode0x7D1
  branch in0x71003D6064 (retained instructions in
  `switch-71003d6064.asm`). Both consume BE32 and subtract value*1000
  from the track's time-credit accumulator.
- Fine wait: Steam0x655859–0x655877 joins0x655A73; NSO opcode0x7D5 branch.
  Both sign-extend BE16 and subtract it **without** multiplying by1000.
- Reference launch: Steam's opcode2000 branch reaches0x656D18;
  NSO's corresponding branch invokes0x71003D8AFC after decoding two BE16s.
  The second value is a row index.
- Event end state: Steam0x656297 writes track state3; NSO opcode0 branch
  writes state3 and clears the interpreter's continue flag.
- Actual completion/restart: Steam0x65359E–0x6536EB; NSO0x71003D4B6C
  final child-list cleanup and empty-list test. Ended tracks with live children
  remain; parent completion/restart follows only after the counts drain.
- Parameter jump tables: Steam0x65B68C (137 RVA32 entries);
  NSO0x710063E03A (137 u16 entries, target=0x71003FA0BC+entry*4).
  Every installed parameter opcode has a dispatch target in both players.

The Steam parent-link callee0x639640 is a `.shared` jump into protected code.
It was **not followed**. NSO0x71003C9674 explicitly links a parameter object's
parent at+0x48; Steam call sites corroborate the hierarchy's construction but
not every internal merge rule. Do not advertise full paired inheritance proof.

## Graph identity, order and multiplicity

All serialized numbers in ACB nested UTF tables and command payloads are BE.
Cue IDs, table-row indices and AWB wave IDs are different namespaces.
A cue reference `(type,index)` uses1=waveform row,2=synth row,3=sequence row
for the measured graph. Do not treat `2000` itself as a waveform ID.
Synth `ReferenceItems` is an ordered list of BE16(type),BE16(index) pairs.
Sequence `TrackIndex` is an ordered BE16 track-row list, selected using
`NumTracks`; track `EventIndex` selects a `TrackEventTable` row. Modern command
indices select their own Seq/Track/Synth command tables, not TrackEventTable.
Older ACBs use the common `CommandTable` namespace.

A reference occurrence creates a child occurrence: two equal synth references,
two note-ons to the same synth, or shared track rows selected twice are **not**
one voice. Cache decoded data if desired, but never cache away playback state.
Validate every traversed index/blob span and detect actual recursive cycles;
a shared DAG is not a cycle. A flattened leaf list is only a resource inventory.

### Types and concurrency

Type belongs to the selecting container, not to the waveform. For installed
Type0, start **all** listed tracks/children with a common parent origin and
independent event cursors/time accumulators. Track array position is not a
serial delay. Each note-on is nonblocking: later events on that track do not
wait for the previously created child to finish. Synth Type0 similarly starts
all reference occurrences. A nested sequence establishes its children relative
to that nested start, not the top-level cue's zero.

| Type | Selection rule in both players | Installed synth / sequence rows |
|---|---|---:|
| 0 | All entries (polyphonic), authored order | 3605 /3578 |
| 1 | One entry, advance persistent ordered selector | 0 /0 |
| 2 | One entry from persistent shuffled index/reference work list | 0 /0 |
| 3 | One weighted-random entry, first cumulative weight >= random threshold | 0 /5 |
| 4 | One weighted-random entry excluding previous selection when eligible | 0 /9 |
| 5 | Cumulative BE16 TrackValues/1000 compared with caller control value | 0 /0 |
| 6 | Ordered selection with time-conditioned reset | 0 /0 |
| 7,8 | Selector/transition-dependent sequence cases; not ordinary layering | 0 /0 |

Types1,2,5,6,7,8 are **not approved implementation contracts** here: the branch
shape is observed, but unused-type initialization/reset/control semantics are
OPEN. Reject them rather than silently using Type0. Synth's branch coverage is
not identical to sequence's for7/8; do not invent a shared universal enum.

For Type3/4 in this install every active track has BE16 weight100. A zero-track
container returns before selection, even if `TrackValues` contains a leftover
weight. Type3/4 start one track, **not** every wave in all alternatives.
Type4 stores the chosen track position in the cue-sheet selection work state,
not in the immutable authored bytes. State persists across starts through the
row's work-area lookup/update path; retain previous choice at row/sheet scope,
not one global last-wave variable. Exact reset on unload/reload and concurrent
starts is OPEN (refuse fidelity claims for those boundaries).

Threshold details matter: the default NSO RNG is an inclusive integer interval
callback, not a floating-point draw. Default callback table0x7100873020's fourth
pointer resolves via relocation to0x71003D079C; it uses remainder modulo
`upper-lower+1`. Type3 passes[0,sum(weights)] when weights exist, otherwise
[0,count-1]. Scan in list order using cumulative >= threshold. Thus replacing
this with a uniform alternative index is not bit-equivalent, even for equal
weights. Steam0x66CEC4 calls its RNG wrapper0x62FF98 with the same bounds and
uses the same comparison; exact Steam default RNG algorithm/seed is OPEN.
Do not claim matching random-choice sequences across platforms.

For installed Type4 positive equal weights with a valid previous choice and
more than one entry: omit the previous entry's weight from the random total
and scan; skip previous choice when the positive-alternative count is nonzero.
For the first draw (signed previous index<0), use the complete weight total.
The threshold remains inclusive. Preserve the first-cumulative boundary rule;
do not substitute rejection sampling or deduplicate equal waveform outcomes.
The general Type4 low-total/zero-weight and one-entry corner cases remain
OPEN; they are not needed by any active installed Type4 row.

## Track events: time, units and termination

TrackEvent bytecode consists of BE16 opcode, u8 payload length, then payload.
Lengths exclude the three-byte header. Preserve byte order, command order and
all occurrences; advance by3+length only after bounding the entire command.
For measured assets the complete event language is:

| Opcode (decimal) | Payload | Meaning | Occurrences |
|---|---|---|---:|
| 2001 | BE32 unsigned | Relative delay in milliseconds |2979|
| 2005 | BE16 signed | Additional relative delay in microseconds |2979|
| 2000 | BE16 reference type, BE16 row index | Start referenced child at current track time |3527|
| 0 | empty | End this event program; no further event launches |3462|

All2000 references are type2 (synth). All2005 payloads are0. Of2001's2,979
payloads,2,517 are0;462 are nonzero, with99 distinct values overall.
A delay is **relative**, not an absolute timestamp. Repeated waits accumulate.
A note-on consumes no duration from the event clock and does not change the
next wait's origin. End is not note-off, not a silence command, and not a
request to stop already playing waveforms. All3,462 programs have exactly one
terminal0, with no trailing command bytes.

The player accumulates elapsed microseconds as credit, executes commands while
credit>=0, and subtracts delays. A negative balance suspends interpretation
until a later update contributes enough credit; zero balance executes now.
Carry residual credit across updates rather than rounding each wait to a game
frame. A late update carries lateness into the child-start offset; do not move
an overdue note to the next fresh origin. PlaybackRatio scales the sequence
clock; all installed values are100 (ratio1). Zero is decoded as ratio1 by the
NSO sequence reader0x71003F21EC, not as a stopped clock.

For a fixed-rate, no-control-change plan, an event's mathematical start is the
sum of its preceding2001*1000+2005 microseconds, relative to its parent's start.
Same-time events retain program order. Type0 track starts share an origin;
resource/voice-limit side effects still see authored track order. Never wait
for one track's sample EOF before interpreting a sibling track.

Do not generalize that every Atom command is supported. The interpreter has
additional branches for999/1000-series delays,2004 duration-bearing notes,
2006 tempo-relative waits, loop-control1200-series and other action commands;
none occur in the measured TrackEvent tables. Their semantics are OPEN and
must cause an unsupported-program refusal if encountered. Never skip an
unknown event and report successful playback. A malformed/truncated event or
missing terminal marker is not an implicit successful EOF.

## Parameter tables, defaults and inheritance

**Event bytecode and parameter bytecode are separate interpreters.** Parameter
streams are bounded by the UTF `Command` blob length; they do not require an
event-style opcode0 terminator. There are no parameter opcode0 occurrences
in this install. A numeric command or parameter-pallet index0 is valid;
0xFFFF means absent, not row0. Absent commands do not create silent voices.
Do not read an absent command table just because an unused column defaults0.

Authoring/reader defaults are different from test-fixture omissions:

| Item | Observed authored data / reader behavior | Missing-column policy |
|---|---|---|
| Synth.Type | Present in630 sheets; all3605 rows0 | No retail proof for omission; a synthetic fixture may explicitly declare neutral Type0 as a port-only extension |
| Sequence.Type | Present for all3592 rows; distribution above | Reader initializes0 before version-gated trailing columns; old schema omission is0, not inference that every missing modern column is legal |
| PlaybackRatio |3592 values100; reader treats0 as1.0, otherwise value/100 | Not a licence to shift ordinal columns or accept malformed schema |
| CommandIndex |0xFFFF absent; otherwise row in owning table | Named-field port fixture extension may default absent, never0; not a measured retail schema |
| ParameterPallet | Sequence3592/Track3462/Synth3527 present values all0xFFFF;78 old synth rows omit it | Reader default absent (-1) for older schema |
| TrackValues | BE16 selector data, not gains or delays for Type0 | Old reader defaults null/zero length; no access for Type0 |
| Action-track start/count | Reader old-schema defaults0xFFFF/0 | No action-track semantics inferred from absence |
| Local/global AISAC refs | Preserve the row's own refs/counts | Missing metadata is not proof that referenced AISAC curves can be discarded |

The modern readers' optional-field thresholds are column-count/version gated,
not arbitrary name omission: Synth reader0x71003F1B2C and Track reader
0x71003F2F14; sequence reader0x71003F21EC. These exact default-reader internals
are NSO evidence; their full Steam internal counterparts are not all paired.
For existing synthetic tests lacking Type/command indices, the implementer may
retain a clearly documented **port fixture profile** of Type0/absent commands.
It must not be presented as the game accepting that schema. Required graph
links/counts still must be present and bounded. Do not weaken retail parsing to
make an underspecified fixture appear authoritative.

Construction order: root parameter context -> sequence parameter commands ->
new track context inheriting the sequence -> track parameter commands ->
new synth context inheriting the note's track -> synth parameter commands ->
child waveform/nested container. Each sibling has its own context. The NSO
links parent contexts rather than copying final scalar values into every
child. Parameter pallets, where present, are applied in distinct calls; they
are absent throughout the installed modern rows, not aliases of command rows.
Track contexts with no authored command explicitly initialize runtime slots
0x0B/0x0C to0, rather than inheriting the sequence's nonzero values blindly.
Synth/no-command legacy compatibility includes a version-dependent path;
its full effect remains OPEN.

Repeated writes within one parameter stream execute in serialized order. Keep
an unset value distinct from explicit0 and from explicit unity. NSO's internal
unset scalar sentinel is bit pattern0x7FFFFFFF; treating it as numerical zero
loses inheritance. NSO default-value getter0x71003C8F20 establishes unity for
slots0,6,7,0x50,0x52,0x53,0x54,0x67,0x68,0x7F,0x83,0x84,0x87,0x92,0x99,
0xA0,0xB2; zero for ordinary additive slots including3,4,5,0x0B,0x0C;
0xFFFF for0x5D..0x61. Defaults of0x55..0x5C,0x62/0x63 use the spatial
configuration object; do not hardcode all of them as0 or1.

The hierarchy is **not** a universal last-writer-wins map: NSO's downstream
combiner0x71003CA324 adds some values, multiplies others and replaces others,
with unset detection. Parent traversal0x71003C98B0 also incorporates player,
category and AISAC contexts. The final DSP sections establish the scalar/
bus/envelope-field fold, non-neutral filter laws and registered wet kernels;
the dry profile is a narrower implementation seam, not a DSP absence claim.
A layered renderer must not silently ignore non-neutral authored pitch or gain.
Parameter transfers alone do not certify a complete CRI DSP clone.

### Parameter transfer facts

Here `U`/`S` mean BE unsigned/signed16; `F` is BE IEEE754 float32; `R(b,r,l,h)`
means clamp a randomized base/range to[l,h]. NSO0x71003CE2DC uses no RNG draw
when range0; otherwise clamps base±range to bounds, interpolates using a
31-bit random value, then clamps. Paired Steam case bodies have the same
serialized conversions for the rows below unless explicitly qualified.
Runtime slot numbers are identifiers, **not** source struct offsets.

| Opcodes | Transfer at parameter interpreter boundary |
|---|---|
|16|U/10000 -> slot7|
|32|Signed first U/10000; negative resets this filter parameter family; otherwise first/10000 and second/10000 passed to two filter setters|
|33|u8 -> slot0xB1; related boolean ->0xB2. **Version difference:** Steam uses context mask; NSO tests zero/nonzero. Installed0 only|
|34|two u8s: first context+0xB9; second runtime slot0x91|
|36|U ->0x7D, u8 ->0x7E, final u8/100 ->0x7F|
|37|second U ->0x80; do not infer first U's use from its physical presence|
|38|U ->0x81, u8 ->0x82, final u8/100 ->0x83|
|39|U/10000 ->0x84|
|40|U milliseconds ->0x85, next u8 curve kind ->0x86, final u8/100 strength ->0x87; see envelope follow-up|
|43|u8 probability gate, random comparison to0..100; installed100 at each standard block|
|60|R(S,S,-2400,2400) -> slot3 (pitch parameter)|
|61|R(S/10,S/10,-360,360) -> slot5|
|63|R(U/10000,U/10000,0,1) -> slot6|
|64|11-byte compound filter definition; leading0 or255 resets family; nonzero form has four U fields and trailing U (OPEN full filter)|
|65|BE32 category IDs (one or two records, lengths4/8), resolved against registered ACF; not an event reference or timed wait|
|66|U/10000 ->0x62|
|67|two U/10000*360 values -> angular conversion ->0x55/0x56|
|68,69|F ->0x58,0x59 respectively|
|70|u8 -> slot100|
|71|R(U,S,0,60000) added to existing authored slot0x90|
|74|U/100 ->0x5C|
|75,78,108,127,128|U -> slots0x5D,0x5E,0x60,0x5F,0x61 respectively;0xFFFF skips write|
|79|two U plus u8, cue-limit configuration passed through limit handling; **not a repeat opcode**|
|80|two u8s: first stores per-start control byte, second adds to context priority accumulator|
|82|u8 nonzero ->0x93; zero leaves unset|
|83/84|U/10000 ->0x0B; sequence/synth use83, track84|
|85/86|U/10000 ->0x0C; sequence/synth use85, track86|
|87|U/100 -> slot0 (gain parameter)|
|89|R(U/100,U/100,0,max(base,1)) -> slot0 (gain parameter)|
|90|U/100 ->0xA0|
|93|u8 -> track flag+0xD3; controls child replacement/stop behavior; installed0|
|110|u8 -> start-context+0xB8; value1 enables the sequence's whole-program replay flag; installed0|
|111|U StringValue index resolves named bus, second U/10000 is send value; not an event reference|
|112|explicit skip-length branch in both observed parameter interpreters; all installed payloads zero4-byte words|
|114|u8 controls start state/reset behavior; installed0; nonzero remains OPEN|
|117|u8==1 writes0 to runtime0x98; installed0, otherwise no write|
|118|one-byte consumed compatibility branch, no observed scalar write|
|120|U time plus mutable BE64 last-time work value; retrigger interval test in microsecond clock; installed all ten bytes0|
|121|S -> slot4 (signed parameter, including authored negative values)|
|122|u8==1 writes4 to slot0x66; installed0|
|124|u8==1 sets context flag+0xBB; installed0|

36/38/40 have a special short form in another interpreter mode; measured blobs
have the widths listed in the census. Do not claim these transfers establish
full envelope shapes, filters, bus routing or category arbitration. Compound
reset32/64, scalar gain87/89 and pitch60/121 are not all no-ops just because
many authoring defaults are neutral. For example, synth command121 has values
150,300,90 and-300 as well as0; a decoder that drops it is observably incomplete.

## Repeat, loop and completion

Three different mechanisms must remain separate:

1. **Waveform loop:** ACB LoopFlag/extension and bounded codec loop coordinates
   govern one voice's sample loop as specified in `audio.md`. A looping leaf
   keeps that child alive. Do not loop every waveform because LoopFlag is
   merely nonzero (this install has1 and2, not a Boolean0/1 convention).
2. **Event program:** terminal0 stops future launches on that track only.
   Type0 siblings continue; every child created before end must drain or be
   explicitly stopped. Multiple note-ons can overlap on one track.
3. **Whole-sequence replay:** start-context flag set by command110==1 causes
   sequence reconstruction after live tracks/children drain, unless stopped.
   Both players' final empty-count branch reconstructs tracks using the
   selector again. It is not an automatic repeat at CueTable.Length, and
   Type4 re-selection must retain selection state. All installed110 payloads0;
   externally requested repeat/loop override is a distinct caller policy.

The lifecycle keeps ended tracks while their child list is nonempty. A parent
finishes only when its track list and active-child count are both empty;
stopping prevents replay. This is stronger than “last event byte consumed”.
For zero-track cues the event-plan is empty: do not invent a silent waveform or
borrow an unrelated wave. Resource/voice-limit failure is not evidence of a
successful audible cue; report the appropriate refused/failed start.

**Duration policy for the existing port (not a retail rounding claim).**
GoldenM8d2 reports the existing CLI bounds each finite wave as
`ceil(NumSamples*48000/SamplingRate)` output frames, with the existing mixer
resampling at source_rate/48000. Retain that convention for finite fixed-rate
48kHz group witnesses with integer-millisecond starts, no unresolved envelopes,
rate modulation or repeats. Integer milliseconds map exactly to48 frames.
For that qualified profile the output bound is the maximum over all selected
note occurrences of `start_ms*48 + ceil(NumSamples*48000/SamplingRate)`.
Preserve intervening silence, don't sum simultaneous durations, don't count
unselected random alternatives, and wait for queued mixer frames to drain
before reporting done. If a zero-sample child is scheduled after silence,
retain its scheduled start when computing the bound. Empty graph bounds0.

This port convention makes timed multi-wave WAV witnesses deterministic
without asserting Atom's exact fractional-sample rounding. CueTable.Length is
metadata, **not a proved hard cutoff or substitute for graph completion**.
Do not truncate at it, add codec guard samples, or use the maximum file length
instead of waveform NumSamples. Retail fractional start conversion, rate/pitch
modulation, release tails, loop-stop release and repeat override interactions
remain OPEN. Refuse their unsupported variants rather than labeling this
qualified duration formula universal.

## Install-wide census

630 ACBs,3,631 cues and3,631 names:594 sheets in data.cpk,12 each in data_c,
data_e,data_k. Locale and nested duplicates are intentionally retained.
Cue root types:3(sequence)=3592;2(synth)=39. Rows:Waveform3425, Synth3605,
Sequence3592, Track3462, TrackEvent3462. All3605 synths have one reference pair:
3566 reference a waveform,39 another synth. The39 old root synths resolve
through their child synth to RIFF waveforms; no HCA claim is made for them.

Sequence track-count distribution:

| Tracks | Rows |
|---:|---:|
|0|707|
|1|2528|
|2|189|
|3|122|
|4|40|
|5|6|

Flattened **occurrence** counts per cue, before selecting Type3/4 alternatives:

| Leaf occurrences | Cues |
|---:|---:|
|0|707|
|1|2563|
|2|183|
|3|123|
|4|25|
|5|23|
|6|2|
|7|2|
|8|2|
|11|1|

A first census using a global visited set undercounted repeated edges; the
final recursion-stack version above agrees independently with GoldenM8d2's
native graph histogram. No deduplicated counts are used as voice counts.

All active nonpoly rows (sequence row numbers are not cue IDs):

| ACB under sound/adx2 | Sequence row:type(track count) |
|---|---|
|bgm/snd00_bgm.acb|649:3(2),650:3(2),670:4(3),726:4(5)|
|bgm_en/snd00_bgm.acb|649:3(0),650:3(0),670:4(0),726:4(0)|
|comse/snd04_comse.acb|115:4(4),118:3(2),119:4(2),122:4(2),124:4(3),126:4(3)|

All nonempty active weight arrays contain100 for every track. The zero-track
bgm_en rows retain a single100 weight, which is not an extra track.

Event-program shape census (`W`=2001,`u`=2005,`N`=2000,`E`=0):

| Shape | Rows |
|---|---:|
|WuNE|2879|
|WuNWuNE|31|
|NE|534|
|NWuNE|10|
|WuNWuNWuNE|3|
|NWuNWuNE|2|
|NWuNWuNWuNWuNE|1|
|NWuNWuNWuNWuNWuNWuNWuNE|1|
|WuNWuNWuNWuNE|1|

### Every installed parameter opcode, by table and payload width

Counts below count serialized occurrences, including repeated bus-send111
records. `S`=SeqCommandTable (3592 rows),`T`=TrackCommandTable (3424 rows),
`Y`=SynthCommandTable (3164 rows). The old common CommandTable has zero rows.
Rows contain parameter streams, not event programs. Widths are payload bytes.

| Opcode | Width(s) | S | T | Y |
|---:|---|---:|---:|---:|
|16|2|3209|2579|2587|
|32|4|0|2579|2587|
|33|1|0|2579|0|
|34|2|3209|2579|2587|
|36|4|0|0|2716|
|37|4|0|0|2756|
|38|4|0|0|2761|
|39|2|0|0|2761|
|40|4|0|0|2587|
|43|1|3209|2579|2587|
|60|4|3209|2579|2587|
|61|4|3209|2579|2587|
|63|4|3209|2579|2587|
|64|11|0|2579|2587|
|65|4,8|3474|0|0|
|66|2|3209|2579|2587|
|67|4|3209|0|0|
|68|4|3209|0|0|
|69|4|3209|0|0|
|70|1|3209|0|0|
|71|4|0|0|2587|
|74|2|3209|0|0|
|75|2|3209|0|0|
|78|2|3209|0|0|
|79|5|7|0|0|
|80|2|3209|0|0|
|82|1|3209|0|0|
|83|2|3209|0|2587|
|84|2|0|2579|0|
|85|2|3209|0|2587|
|86|2|0|2579|0|
|87|2|1|794|0|
|89|4|3209|2579|2587|
|90|2|3209|0|0|
|93|1|0|2579|0|
|108|2|3209|0|0|
|110|1|3209|0|0|
|111|4|27910|26448|24368|
|112|4|826|944|944|
|114|1|3209|0|0|
|117|1|0|0|2587|
|118|1|0|0|2587|
|120|10|3209|0|0|
|121|2|0|0|2587|
|122|1|0|0|2587|
|124|1|0|0|2587|
|127|2|3209|0|0|
|128|2|3209|0|0|

No other TrackEvent or parameter opcode occurs in these630 ACBs. Presence of
an opcode in an unreferenced parameter row still counts; this is deliberately
not a runtime-frequency claim. Opcode65 widths4/8 count662/2812 respectively
in sequence commands. All four event opcodes and every parameter opcode have
bounded payloads; zero malformed command tails were observed.

## Explicit OPEN/refusal ledger

These are missing proof, not assumed neutral behavior. The implementation may
support a closed qualified profile and report precise refusals for the rest;
it must not advertise all3,631 cues as reproduced just because traversal works.

| ID | OPEN item | Required disposition/evidence to close |
|---|---|---|
|A1|Default RNG algorithm, seed expansion and two-stream ownership resolved below; concurrent mutable selector-work arbitration remains OPEN|Use the paired recurrence and preserve draw order; do not substitute independent per-cue RNGs or claim deterministic concurrent interleavings|
|A2|Closed for the measured install: active Types0/3/4 and empty selectors covered; other types and zero-weight programs absent|Explicit unsupported-type/selector refusal outside that measured profile, not invented semantics for unused types|
|A3|Resolved paired scalar/bus/envelope folding and all measured local time-AISAC graph types below; dynamic category limit/steal arbitration remains OPEN|Use ordered per-context folding and the measured graph laws; arbitrary external/controller combinations are outside the qualified install profile|
|A4|Closed for the three requested DSP gaps: full common210-byte spatial law, authored non-neutral32 filtering and registered BUS1 Reverb/BUS2 Bandpass→Biquad including post-pan links; see the final DSP follow-ups|Preserve exact coefficients/defaults,43 RNG gates, render-chunk interpolation and tails; proof policy is nso-body + steam-dispatch-pair, never protected Steam decoding|
|A5|External repeat override, wave-loop stop/release and finite repeat counts remain OPEN; action tracks positively absent|Don't equate command79 with repeats or event0 with note-off; explicit unsupported policy where not already specified by the existing single-wave seam|
|A6|Retail fractional start/resampling, modulated clock conversion and exact combined voice-tail completion remain OPEN; registered Reverb ended-block clock and envelope release minimum resolved above/below|Qualified integer-ms fixed-rate port formula is allowed, not a universal retail rounding or tail claim|
|A7|Closed install boundary: two measured modern schemas plus the explicitly identified39 old sheets; arbitrary synthetic omissions unproved and unsupported|Named-field fixture defaults remain port-only; do not accept arbitrary missing retail columns|
|A8|Closed install boundary: no recursive graphs, malformed measured programs or other event opcodes; outside that boundary unsupported|Explicit refusal; never silently skip unknown behavior|

## Implementation acceptance witnesses

These are required follow-through for the implementer, **not tests run by this
spec writer**: two simultaneous tracks; repeated same-wave references retaining
two voices; sequential note-ons with overlap and a nonzero relative wait;
end marker with a live child; empty cue; nested synth; Type3/4 selecting only
one track with stable injected thresholds at cumulative boundaries; Type4
previous-choice exclusion; integer-ms max-end WAV frame bound and final mixer
drain; explicit refusal of unsupported parameter/effect/type cases. Existing
single-wave HCA/ADX, loop bounds, category controls and PS2 refusal remain
unchanged unless a separately proved contract requires otherwise.


## A3/A4 follow-up: registered ACF and the closed dry profile

This section supersedes the earlier blanket A3/A4 restriction **only for the
explicit profile below**. It closes paired scalar/bus/envelope-field folding
and authorizes real multi-wave dry cue witnesses without ignoring parameters.
The final DSP sections additionally supply the non-neutral and wet laws;
this narrow profile itself is not an all-cues or bit-identical-DSP approval.

Main explicitly authorized static ACF parsing during the follow-up. The two
installed ACF assets were independently extracted/read and SHA256 checked:

| Install object | Bytes | SHA256 |
|---|---:|---|
|data.cpk::sound/adx2/common.acf|65536|30a344ecfa24e747411ab08712ba768769f0bdbc6ee0bd4c0fc312b5c497ce48|
|data_cri/sound/adx2/common.acf|6656|572a36a2161b7249696fd2f6f44c28346fa9b1ffe30bd6c70ecbaa5bd8779f99|

Both are nested UTF, with8 BusTable rows,3 DspFxTable rows,2 BusLinkTable rows,
one DspSettingTable (`DspBusSetting_0`),9 BusNameTable strings (including empty
entry0),16 AISAC control names but **zero** global AISAC/graph/React rows.
Packed ACF:16 categories/names/command rows; loose ACF:13 each. The three
additional packed categories are BGM_VOL, SE_VOL, VOICE_VOL (IDs13,14,15).
Packed version19398656/format1.28.0; loose19070976/format1.23.00. The common
dry proof below is valid for either measured asset, not for arbitrary ACFs.
Owner-local full metadata: `acf-packed.json`, `acf-loose.json`,
`acf-inventory.json` beside the original census evidence.

### Registered ACF loading, not ACB-owned DSP defaults

NSO0x71003A1608 resolves `sound/adx2/common.acf` through its binder and calls
0x71003ED4E8 ->0x71003ED4EC to register it before creating the player pool.
The registration body validates library initialization, unregisters/replaces
previous ACF state, reads ACF information, sizes category/cue-limit/bus work,
initializes it and publishes registered state; failures do not fabricate an
empty valid configuration. Paired Steam registration body is0x625834–0x625AF6
(the same diagnostic strings and allocation/validation branches). A separate
initialization configuration contains the loose `data_cri/sound/adx2/common.acf`
path: Steam0x4E9AC0, NSO0x71003974A0. Do not conflate that config string with
proof that the game cue player never subsequently registers the binder asset.
The precise Steam game-wrapper choice between those assets is still OPEN;
no protected wrapper was decoded. The relevant categories and dry buses below
have equal behavior in both assets, so this ambiguity does not block that
qualified witness.

NSO0x71003A1800 updates named category volumes BGM_VOL/SE_VOL/VOICE_VOL from
integer settings divided by100. Steam0x4FD7A0/0x4FE7A0 contain the corresponding
BGM_VOL setting call and percent conversion. Those run-time controls remain
state, not immutable ACB metadata; a unity-control isolated witness must state
that precondition. Do not apply the user's master/category gain twice.

### Paired parameter fold, including sends and envelope precedence

The formerly missing Steam fold is readable in `.shared`:

| Rule | Steam RVA | NSO VA |
|---|---|---|
|Scalar/context fold|0x633750–0x635018|0x71003CA918–0x71003CC7DC|
|Authored linear gain product|0x6337F8–0x63382F|0x71003CA918 gain block at+0x130|
|Final context gain times authored gain|0x6387E8–0x638823|0x71003CD7E0–0x71003CD818|
|Named send merge|0x63A130–0x63A4B5|0x71003CA918 named-bus block|
|Envelope precedence|0x6342E5–0x634740|0x71003CA918 envelope block at+0x140|
|Fold linked-context order|0x635BC0–0x635BF2|0x71003CA918 linked+0x38 iteration|

Unset is bitwise0x7FFFFFFF, not floating-point zero. Linear authored gains
(runtime slot0, op87=U16/100 or op89's randomized percent) **multiply** across
contexts; the final getter also multiplies the context/control gain at+0x104.
Pitch60's slot3 adds cents; slot4 (op121) replaces when explicitly supplied.
NSO0x71003CD940/0x71003CD9C0 and paired Steam0x638158/0x6382D8 use
`exp2f(cents/1200)`, multiplying frequency ratios. NSO import relocation at
0x5CA560 binds GOT0x87FDF0, symbol236, to `exp2f`; this is not a guessed math
helper. Existing output-rate rounding still needs the qualifications above.

The exact ACB schema is Header field **`StringValueTable`** (nested UTF data
table), row indexed by the first unsigned BE16, string column
**`StringValue`**. This is a zero-based row index, not a CueNameTable index or
an ACF BusName index. The authored skill07D table rows0..7 are `MasterOut`,
`BUS1`, `BUS2`, `BUS3`, `BUS4`, `BUS5`, `BUS6`, `BUS7`; resolve by string
because the ACF's MasterOut name index is8, not0.

Named bus111 is **not** a direct addition to the master signal. First resolve
its StringValue index to a bus **name**, then the registered bus number; second
BE16/10000 is a linear send multiplier. For each matching bus, independently
multiply explicit parent/child send values. An absent entry inherits the
existing entry; explicit0 remains0. Lists are merged by bus identity, not by
string-index equality across different tables. The paired merge instructions
multiply send scalars; they do not let a child's unity write override a
parent's zero. Relative-offset send fields (not authored by111 here) have a
separate flag and are not interchangeable with these gains.

Envelope fields are not gain products or time sums: for a non-player context,
the first explicit field in the ordered fold wins; a later ordinary context
only fills unset fields. A context of runtime type6 (player) replaces explicit
fields, overriding that rule. This is checked field-by-field, including curve
kind/strength, not a wholesale copy of a presumed ADSR object. Steam0x634347
branches on type6;0x634522 is the fill-unset branch. Retain this distinction
instead of a universal nearest-child or last-writer-wins rule.

Scalar gain/pitch and bus-send folding is now paired proof despite the
parent-link wrapper itself remaining protected. Category and AISAC contexts
can contribute to the ordered chain; do not treat category assignment as only
a public mixer label.

### Exact installed bus topology and effect identity

All buses have Volume1, Pan3dVolume1, angle0 and distance1 in both assets.
There are8 actual buses, not9: empty BusNameTable entry0 is not a ninth bus.
Bus0 uses BusNameTable index8 (`MasterOut`); buses1..7 use BUS1..BUS7.

| Bus | Effect chain (author order) | Destination |
|---|---|---|
|0 MasterOut|none|master output|
|1 BUS1|CRIWARE/Reverb|link Type2, gain1, to MasterOut|
|2 BUS2|CRIWARE/Bandpass then CRIWARE/Biquad|link Type2, gain1, to MasterOut|
|3..7|none|no links; not parallel copies automatically added to MasterOut|

All three effects have PluginType1, FxType255, SrType0 and Bypass0. Their
serialized parameters/configuration are BE float32, in slot order:

| Effect | Parameter values | Configuration values |
|---|---|---|
|CRIWARE/Reverb|2000,10,20,0,8000|1000,50,0|
|CRIWARE/Bandpass|24,24000|none|
|CRIWARE/Biquad|0,24000,1,1|none|

These measured vectors are interpreted by the final DSP algorithm sections,
including their units, coefficients, tails and post-pan Type2 links.
A renderer that has not implemented those laws must not replace a wet return
with dry passthrough, ignore the bus or count it as another dry layer.
The explicit-zero wet sends in the closed profile below eliminate wet input;
require initially cleared DSP state (no old tails) for its finite WAV bound.

### Category65: binding, gain and eligibility

Correction/clarification: each four-byte65 record is a **BE32 category ID**,
not an event-style BE16(reference type,row) pair. Thus00000009 means category
ID9, not reference pair(0,9). AcfReferenceTable labels corroborate the binding
but do not supply gain or admission state themselves.

The paired parameter interpreters resolve the ID to registered-category state;
Steam category resolver0x643E64–0x643EF5 / NSO0x7100403BA8 return0xFFFF and
issue a diagnostic when ACF/category is absent. No-ACF skipping is observable
fallback behavior, **not** permission to call an unconfigured render the
installed cue. This profile requires one of the measured ACFs registered.

Relevant category rows:

| ID/name | Packed gain | Loose gain | Group | Category73(max cue instances,work index) |
|---|---:|---:|---:|---|
|2 /02_footstep_companion|1|1|1 packed /0 loose|4,2|
|8 /cut2|1|1|4 packed /3 loose|1,11|
|9 /cut3|1|1|4 packed /3 loose|2,12|

The gain is ACF command89 percent100 (packed), or unity default when absent
(loose). ACF category commands73 carry two BE16s (count, work index), not gain;
129 is0 in packed rows,120 is all-zero retrigger work/time. Names such as
`cut2` are not a count oracle: category8 actually has limit1 in these files.
Both variants have no category AISAC/parameter pallet/React contribution for
these rows. Other packed category6/stream_bgm has authored gain0.5; it is NOT
covered by a global claim that every category is unity.

Category membership is established at cue/container start before child
launches. The limit concerns active cue instances, not each waveform note in
one cue. An isolated idle category permits a first eligible cue; its later
notes do not independently consume new category cue slots. The decoded
category gain participates in the same multiplicative scalar path as the
cue/track/synth gains; run-time category controls/mutes/reacts remain additional
state. For a bounded implementation, admit only when every bound category has
available capacity and no unresolved arbitration/react state. Refuse the
contended/stealing case until its policy is implemented; never silently erase
category65. NSO0x71003E6184 checks category retrigger timestamps, and the
parameter65 path inserts registered category references before that check.

### Closed generic dry profile (A3/A4-D)

The following is an approved **parameter-aware** real-cue profile, not a cue-ID
exception or permission to sum exported stems:

- Resolve and execute the actual cue graph and all track events; preserve
  repeated references. Type0 or a fully resolved supported selector must be
  honored; never expand every random alternative.
- Every reachable command path contains only65 category records,87 linear
  percent gain,89 percent gain with zero random range,111 named sends, or
  explicitly decoded skip-only112. Index0xFFFF is absent. No AISAC, parameter
  pallet, action track, pitch/envelope/filter or unresolved command may be
  silently discarded to obtain eligibility.
- Resolve each category against one of the measured ACFs; require available
  cue-instance capacity, zero retrigger interval, no AISAC/React and known
  category gain. Apply that gain, track gain and all caller controls exactly
  once. Unity caller controls and no competing active cues are explicit real
  witness preconditions, not invented authoring defaults.
- Fold named sends multiplicatively. Require effective MasterOut send1 and
  all other sends0 (including buses without links), MasterOut volume1/no FX,
  and no retained prior wet-bus tail. This is genuinely dry routing even when
  every child has eight unity111 records.
- Use the bounded waveform NumSamples/loop metadata. A finite WAV witness
  requires finite leaves and no external repeat; do not call looped input EOF.

Under these predicates, per-note PCM is multiplied by the product of authored
and known category/caller gains before the existing mixer adds voices. No
reverb/filter/envelope implementation is needed because none is reached, not
because an unsupported command was ignored. Existing mono/stereo resampling,
pan/output convention and final clipping remain the existing port's policy;
this specification does not claim a newly proved CRI channel-pan law.

#### Actual four-note witness: skill07D cue ID1

Source `data.cpk::skill/07D.pac::07D.acb`, SHA256
`edf05b6e8bb967e0e53fba666be9c9c918e3b9395c8e5fe76b4dce16f8221d03`.
Cue ID1 selects sequence row0 Type0, three track rows. It does **not** have the
210-byte default parameter block: its sequence commands are65(category9) and
eight111 records, its tracks have87 plus eight111, and its four synth rows
have only eight111. MasterOut is1/BUS1..7=0 at the sequence; all track/synth
sends are1. No envelope/filter/pitch/AISAC path is present. Do not confuse this
with a newer standard-block cue that happens to have a similar name.

| Start ms | Track | Synth | Waveform row/AWB ID | Gain before caller controls |
|---:|---:|---:|---:|---:|
|0|0|0|0 /0|0.78|
|67|1|2|1 /1|0.76|
|200|2|3|0 /0|0.78|
|284|0|1|1 /1|0.78|

Both waves are mono HCA,44100Hz, LoopFlag1/no extension. Row0 NumSamples12238;
row1 NumSamples15219. They remain **four occurrences**, including two uses of
each wave. With the existing48kHz port ceil convention their durations are
13321/16565 frames; start frames0/3216/9600/13632. End frames
13321/19781/22921/30197: final bound **30197 frames**, plus no fabricated tail.
The overlapping four-note result, after actual graph scheduling/gain/routing,
is the real cue witness; separately decoded-and-added stems without these
contracts are not.

Other measured minimal paths: skill06F,072,075 cue0 each uses65/87/111 only;
06F has two notes at0 and2503ms, track gain0.86, category8. An exhaustive
follow-up over322 ACB sheets containing multi-leaf graphs found **zero**
multi-leaf cue paths with only absent/112/111 commands. Category and gain
support cannot be avoided honestly by choosing a different real multi cue.

### The210-byte common sequence block is not an empty stream

Measured example: comse `snd04_comse.acb` sequence115 (cue10302, Type4) has
exactly210 command bytes. It contains120,80,70,65,82,90,110,114,89,43,34,60,
16,61,63,83,85,66,67,68,69,74,75,78,108,127,128 and eight111 records.
The existing opcode table specifies their serialized transfers. At measured
values its category ID2, gain1, pitch60=0,111 MasterOut1/others0, controls
70/82/110/114=0, probability43=100, and retrigger120=0 are meaningful decoded
state. Other fields are normalized spatial/filter defaults or absent indices,
not bytes that may be skipped merely because the total length is210.

Its child synths additionally declare36/37/38/39/40 envelope settings (time
fields0, sustain1, curve kind0/strength1), filter reset64/32 and121 pitch0.
Track filter64/32 and0x0B/0x0C values also exist. Paired precedence above closes
how those settings inherit. The later DSP sections give their envelope,
filter, spatial and bus algorithms. This cue does not fit the narrow
A3/A4-D command allowlist; a full parameter interpreter must use those
specified laws and retain43's RNG gate. Additionally Type4 selects one
track: it is not a simultaneous
four-wave witness. Poly short comse examples include cue20012/20013/20014;
apply the same explicit parameter/DSP eligibility checks to them.

The narrow dry approval supplies a simple multi-wave mixer witness. Wider
standard-block and wet playback use the complete DSP sections below, rather
than treating unsupported authored commands as neutral.

## Exact serialized controller fields for dry-profile admission

These are literal UTF column names measured across all630 ACB schemas, not
aliases. `LocalAisacNumRefs` does **not** occur in these table schemas.
`LocalAisacs` is the local-reference **data blob**; require zero byte length,
not a missing invented count. `GlobalAisacNumRefs` is the global count, with
`GlobalAisacStartIndex` the starting row in `GlobalAisacReferenceTable`.
For A3/A4-D require count0; measured start values are0xFFFF.

| Node table | Local/global AISAC columns | Other authored-controller columns |
|---|---|---|
|SequenceTable|`LocalAisacs`, `GlobalAisacStartIndex`, `GlobalAisacNumRefs`|`ParameterPallet`; `ActionTrackStartIndex`, `NumActionTracks`; `InstPluginTrackStartIndex`, `NumInstPluginTracks`|
|TrackTable|`LocalAisacs`, `GlobalAisacStartIndex`, `GlobalAisacNumRefs`|`ParameterPallet`; `TargetType`, `TargetName`, `TargetId`, `TargetAcbName`, `Scope`, `TargetTrackNo` describe action targeting, not additional count aliases|
|SynthTable|`LocalAisacs`, `GlobalAisacStartIndex`, `GlobalAisacNumRefs`|`ParameterPallet`; `ActionTrackStartIndex`, `NumActionTracks`|

Modern schemas occur in378 sheets without `Guid` and213 with it. The39 old
schema sheets have SequenceTable(`Tempo`, `NumTracks`, `TrackIndex`,
`CommandIndex`); TrackTable retains the five event/command/local/global AISAC
columns; SynthTable lacks the modern pallet/action columns. Do not invent
missing control counts as retail defaults. The original old-schema OPEN
qualification remains; a strict modern dry profile may refuse that schema.
Neither modern TrackTable has `NumActionTracks` or `NumInstPluginTracks`;
neither SynthTable has instrument-plugin-track columns. Those features are
owned by the tables that actually carry their counts.

For admitted modern nodes require `ParameterPallet`0xFFFF, action counts0,
plugin-track counts0 where the actual schema has those fields. Measured
corresponding start indices are0xFFFF. Check every reachable node, including
all possible selected alternatives. If a recognized authored controller is
nonempty/nonzero, or an unknown controller/schema appears, refuse by name;
do not turn an unrecognized column into an assumed absent controller.

## Paired envelope and neutral-filter backend follow-up

This section closes two previously OPEN edges, without approving the entire
common210-byte parameter block or arbitrary filtering.

### Envelope times, stages and the zero-time edge

Paired software envelope initialization is Steam0x6A9E8C–0x6A9F2C /
NSO0x71004096CC–0x7100409784. Stage advance is
Steam0x6AA004–0x6AA1C1 / NSO0x710040946C–0x710040965C.
Stage progress is Steam0x6A9CE4 (called by stage advance) /
NSO0x710040965C–0x71004096CC. Parameter delivery reaches NSO effect14 through
0x7100407A0C and0x71004093D4: parameter IDs0..10 are attack time/curve/
strength, hold time, decay time/curve/strength, sustain level, release time/
curve/strength. These are the typed fields from36/37/38/39/40, not opcodes
passed through as effect IDs.

Times are **milliseconds**, converted at the envelope's sample rate:
integer frame count is trunc(time_ms*rate/1000) for attack/hold/decay stages
in the observed state machine. Start delay additionally carries millisecond
and microsecond parts. Gain moves0→1 in attack, stays1 for hold, moves
1→sustain in decay, then holds sustain until release; release moves from
its captured release-start gain toward0. A zero-length attack/hold/decay
segment consumes no frames and falls through in the same state update.
**Release is different:** Steam0x6A9E34–0x6A9E89 /
NSO0x7100409784–0x71004097D8 clamps its time to **at least5 milliseconds**
before converting to frames. Steam's directly referenced constant0x965038
(`.sdata`, not `.arch`) is5.0. An authored release0 is therefore not proof
of a zero-length stop fade.

All installed36/38/40 curve-kind bytes are0 and all their strength bytes are
100 (=1). NSO curve0 at0x710041D350 leaves normalized progress unchanged.
Do not treat the decompiler's float renderings of integer state codes1..7
as denormal envelope amplitudes. State5 is sustain and state7 is finished.

**Closed unity-before-release case:** authored attack=hold=decay=release=0,
sustain=1, curve kind0/strength1. Initialization performs a zero-frame
state update, falling through delay0/attack0/hold0/decay0 to sustain1 before
the first rendered frame. It remains1 until release. On release entry the
authored0 is clamped to5ms, so a port must not replace that transition with
an instantaneous stop. This is the actual default-looking comse synth
envelope. Model the pre-release part as unity only after decoding/inheriting
the fields; the stop/release lifecycle and any tail still need the minimum
release rule. Nonzero attack/hold/decay values retain authored timing.
Sample/block interpolation equivalence of a new non-neutral renderer remains
outside the dry approval.

The envelope factor is delivered to the source-mixer gain after voice
filter processing: Steam0x699564–0x699967 (step call0x699790, gain handoff
0x69995A to0x68E164) / NSO0x7100406F84–0x7100407428
(handoff to0x710040A9B0). Both setters write source gain+0x224 and its dirty
flag; they do not change the category volume or authored bus-send list.
The factor therefore belongs before source-to-bus accumulation, not as a
second category multiplier applied to the final master bus. Exact non-neutral
block interpolation/rounding remains OPEN; unity is unaffected.

### Neutral filters are explicit bypasses, not guessed flat coefficients

Paired bandpass getter Steam0x637C94–0x637D65 /
NSO0x71003CDADC–0x71003CDCA0 resolves normalized lower/upper knobs to cutoff
frequencies. Endpoints0 and1 map to24Hz and24000Hz respectively; the returned
enable flag is0 when those are the resolved pair. NSO's interior conversion
is clamped exp2((x*2.1735804080963135+1)*4.5849624); endpoint thresholds
are2^-16 and1-2^-16. Do not interpret opcode32's knobs as Hz directly.
Interior values are not approved as no-ops.

Paired biquad getter Steam0x637B78–0x637C92 /
NSO0x71003CDD20–0x71003CDEC4 defaults to type0 (disabled). An explicit type0
also clears its enable indication. Command64's leading0/255 is the observed
reset path. Command32's negative first signed value resets its family;
otherwise its two normalized fields feed the bandpass knobs.

Actual enable/disable is paired at Steam0x63DEB6–0x63DF12 and0x63DF38 onward
inside0x63D6A8–0x63E0F6 / NSO0x71003DDB5C's filter update:
bandpass endpoint pair disables effect2; biquad type0 disables effect1.
Thus inherited lower0/upper1 plus reset/type0 is a proven bypass in the
common track/synth profile. It does not run a lossy “almost flat” filter.
The final DSP sections specify non-neutral opcode32 coefficients and history,
center/LFE0x0B/0x0C, the remaining spatial fields and registered wet effects.
These laws extend beyond the narrow A3/A4-D profile; they do not permit its
command allowlist to be bypassed or DSP commands to be discarded.

## Complete installed hierarchy column-name allowlists

For strict retail dry admission compare the **complete column-name set**,
not a heuristic substring such as “Aisac”, against a measured modern variant
below. Column order is shown for identification, not required as a semantic
ordering. These are factual schemas, not permission to skip a field's
specified semantics. Each modern base occurs in378 ACB sheets; each `+Guid`
variant occurs in213. Add exactly `Guid` to the corresponding base, with no
other added columns. The old variants each occur in39 sheets and remain
outside the modern dry profile.

**Modern SequenceTable base (16 columns):**

`PlaybackRatio`, `NumTracks`, `TrackIndex`, `CommandIndex`, `LocalAisacs`,
`GlobalAisacStartIndex`, `GlobalAisacNumRefs`, `ParameterPallet`,
`ActionTrackStartIndex`, `NumActionTracks`, `TrackValues`, `Type`,
`ControlWorkArea1`, `ControlWorkArea2`, `InstPluginTrackStartIndex`,
`NumInstPluginTracks`.

**Modern TrackTable base (12 columns):**

`EventIndex`, `CommandIndex`, `LocalAisacs`, `GlobalAisacStartIndex`,
`GlobalAisacNumRefs`, `ParameterPallet`, `TargetType`, `TargetName`,
`TargetId`, `TargetAcbName`, `Scope`, `TargetTrackNo`.

**Modern SynthTable base (13 columns):**

`Type`, `VoiceLimitGroupName`, `CommandIndex`, `ReferenceItems`,
`LocalAisacs`, `GlobalAisacStartIndex`, `GlobalAisacNumRefs`,
`ControlWorkArea1`, `ControlWorkArea2`, `TrackValues`, `ParameterPallet`,
`ActionTrackStartIndex`, `NumActionTracks`.

**Old SequenceTable (4 columns):**
`Tempo`, `NumTracks`, `TrackIndex`, `CommandIndex`.

**Old TrackTable (5 columns):**
`EventIndex`, `CommandIndex`, `LocalAisacs`, `GlobalAisacStartIndex`,
`GlobalAisacNumRefs`.

**Old SynthTable (9 columns):**
`Type`, `VoiceLimitGroupName`, `CommandIndex`, `ReferenceItems`,
`LocalAisacs`, `GlobalAisacStartIndex`, `GlobalAisacNumRefs`,
`ControlWorkArea1`, `ControlWorkArea2`.

All3462 measured modern TrackTable rows have `TargetType`0, `TargetName`"",
`TargetId`0xFFFFFFFF, `TargetAcbName`"", `Scope`0 and `TargetTrackNo`0xFFFF.
Require those non-action target values for the strict profile; do not
interpret them as a reference to cue0. All3605 SynthTable rows have empty
`VoiceLimitGroupName`; a nonempty group is not approved by that measurement.
`ControlWorkArea1`/`ControlWorkArea2` are serialized runtime work indices,
not booleans saying whether an AISAC exists; they have many nonzero values
in installed nodes and must follow the selector/work behavior already
specified rather than an invented “nonzero means authored DSP” rule.

`Guid` is the extra identity column in the newer measured variants, not an
additional controller-count spelling. `TrackValues` keeps its existing
type-specific selection meaning; it is not globally required empty.
Port-only fixtures must have their own explicit allowed schema/default
contract; accepting one must not weaken these measured retail allowlists.

## DSP implementation proof: registered effect callbacks and BUS2

**Proof policy authorized by Main:** `proof: nso-body + steam-dispatch-pair`.
The NSO supplies the algorithm body; Steam supplies the same registered
effect identity, callback ABI, input vector and readable dispatch/process
path. This is PROVEN-code under that policy, not an assertion that protected
Steam bodies were compared instruction by instruction.

### Callback binding; protected entries deliberately not followed

Each registered effect interface has a version word1 and a pointer to ten
callbacks. Observed callback indices0/1/2/3/4/5/8 are name, work size,
create, destroy, reset, process and parameter update, respectively. The
remaining accessor/query callbacks are not required to bind the process.

| Effect | NSO interface / callback array | Steam callback array | Process callback NSO / Steam | Parameter update NSO / Steam |
|---|---|---|---|---|
|CRIWARE/Reverb|0x7100873FC0 /0x7100873FD0|0x8D2FF0|0x7100416A50 /0x69D1E4|0x71004167CC /0x69D2F8|
|CRIWARE/Bandpass|0x71008739B8 /0x7100873A28|0x8D2BD0|0x710040D8F0 /0x69A7E4|0x710040D87C /0x69AB80|
|CRIWARE/Biquad|0x71008739C8 /0x71008739D8|0x8D2B80|0x710040D8F0 /0x69A7E4|0x710040D6C8 /0x69AA04|

Steam callback pointers are static `.sdata` pointers resolved against the PE
image base. The executable-name getters are0x69CD00/0x69A61C/0x69A614,
paired with NSO0x71004163CC/0x710040D5A4/0x710040D598. These establish the
binding rather than guessing from an adjacent function.
Steam Bandpass update0x69AB80 immediately jumps to0x32252300 in `.arch`;
Biquad type0's coefficient helper0x6A6DC0 immediately jumps to0x322C6E20.
Those targets were recorded, **not followed or decoded**. Steam's readable
biquad dispatch0x69AA04–0x69AB80 supplies type/frequency/Q/gain arguments,
and its process0x69A7E4–0x69A9D0 pairs the NSO's state/tail behavior.

### BUS2 is two active filters, not “neutral because24000”

Process the registered BUS2 effects in authored order:
Bandpass[24,24000] then Biquad[0,24000,1,1].
Unlike voice biquad type0 (disabled at the voice-control boundary), the
**plugin Biquad type0 is lowpass**. Do not conflate those enum namespaces.
At48000Hz both upper cutoffs clamp to23900Hz, not24000Hz. Consequently the
BUS2 chain is not a no-op and must not be skipped.

#### Biquad plugin type0: exact lowpass law

`proof: nso-body + steam-dispatch-pair`.
NSO0x710040D6C8 selects0x71003E9E80 for type0; Steam0x69AA04 selects
0x6A6DC0 with the same serialized fields. The vector is
`[type, cutoff_Hz, Q, gain]`; the lowpass branch does not use gain.
Clamp Q to[0,10] at update, then raise it to0.001 if smaller in the
coefficient helper. Clamp cutoff to[10, Fs/2-100].
Compute in float32: w=(cutoff*float32(2*pi))/Fs,
s=sinf(w), c=cosf(w), alpha=s/(Q+Q).
The raw coefficient arrays, in their actual order, are:

- denominator A=[1+alpha, -2*c, 1-alpha];
- numerator B=[(1-c)*0.5, 1-c, (1-c)*0.5].

Normalize all coefficients by A0, preserving the signs of A1/A2.
The sample recurrence is
`y[n]=B0*x[n]+B1*x[n-1]+B2*x[n-2]-A1*y[n-1]-A2*y[n-2]`
after normalization. Keep independent state per channel, initially zero.
The float32 2*pi constant is bits0x40C90FDB.

#### Bandpass plugin: exact prewarped second-order law

`proof: nso-body + steam-dispatch-pair`.
NSO parameter update0x710040D87C calls0x710040E224; its generator chain is
0x710040E090→0x710040DEB8→0x710040DC90→0x710040DB7C.
The two parameter values are lower_Hz and upper_Hz. Set
lo=max(lower,10), hi=min(upper,Fs/2-100).
If lo>hi-1, emit A=[1,0,0], B=[0,0,0]: **zero response**, not identity.
Otherwise design in float64, using pi bits0x400921FB54442D18:

1. u=2*Fs*tan(pi*lo/Fs), v=2*Fs*tan(pi*hi/Fs).
2. D=v-u, C=u*v. The analog transfer is D*s/(s*s+D*s+C).
3. Map with s=2*Fs*(1-z^-1)/(1+z^-1). Equivalently, with K=2*Fs,
   raw A=[K*K+D*K+C, 2*(C-K*K), K*K-D*K+C],
   raw B=[D*K,0,-D*K], then divide both by raw A0.

For the observed generator's evaluation/rounding path, do not replace the
root calculation with a different float32 direct-form design. Its prewarp
order is t=(1.0/Fs)*pi, qlo=(1/t)*tan(t*lo), qhi=(1/t)*tan(t*hi);
tau is double2*pi. It forms D=qhi*tau-qlo*tau and
C=((qlo*tau)*qhi)*tau, then r=sqrt(C), delta=D*D+(r*r)*(-4).
For delta>0, poles are(-D+sqrt(delta))/2 and(-D-sqrt(delta))/2.
Otherwise they are -D/2 +/- i*sqrt(-delta)/2. There is one analog zero0
and gain D. Map each root
r to (2+r/Fs)/(2-r/Fs). The pole gain factors are
(1/Fs)/(2-r/Fs); the zero gain factor is (2-0/Fs)/(1/Fs).
Append the extra digital zero-1, expand the real polynomials in double,
take their real components and cast each A/B element to float32. If the
resulting A0 is0, replace A0 with1. Float32 coefficient normalization then
feeds the recurrence above. This records the actual complex-root design
path as well as its mathematically equivalent closed transfer.

Both bus filters use the same coefficient normalizer and processing kernel:
NSO0x71003E9B4C/0x71003E9C5C, Steam0x6A687C/0x6A6520.
The optimized kernel expands eight samples at a time. Do not assert
bit-identical scalar-versus-expanded evaluation merely from the equivalent
transfer equation; the expansion rounds at different intermediate sums.
Per channel its state comprises x[-2],x[-1],y[-2],y[-1].
On an input-ended call the plugin reports drained only when all four state
magnitudes in every channel are <=2^-15; otherwise it continues processing.
Once drained, a later live-input call clears the full state bank. After each
processed block, y-state magnitudes below1e-24 are flushed to0.
There is no authored finite tail duration to substitute for that state test.

### Reverb binding and parameter laws now established

This is **CRIWARE/Reverb**, not I3DL2Reverb or IRReverb.
The live vector is `[RT60_ms, room-size, pre-delay_ms, HP_Hz, LP_Hz]`;
the registered vector is `[2000,10,20,0,8000]`. Configuration[1000,50,0]
allocates maximum pre-delay1000ms, maximum room-size50 and routing mode0.
NSO create0x710041650C and update0x71004167CC are paired with readable
Steam0x69CE30 and0x69D2F8. The processing outer loop limits chunks to256
frames (Steam0x69D1E4 / NSO0x7100416A50).

Clamp RT60 to[0,20000]ms; room-size>=100 uses299.40118ms, otherwise its
base delay scale is room-size/334*1000ms. Each of nine comb delays is the
next prime at or above trunc(base_ms*delay_scale[i]*Fs/1000), then bounded
by its allocated ring and with a minimum16 samples.
Feedback gain is pow(0.001, comb_delay_ms/RT60), clamped to[-0.99,0.99].
HP is clamped[0,24000], LP at most24000, and each is multiplied by the
per-comb frequency scale before the same bandpass generator above constructs
the feedback filter. The registered0HP still becomes the generator's10Hz
lower clamp; it is not a bypass flag.

Pre-delay is at most1000ms and at least16 frames. For mode0, sum all input
channels with scalar bits0x3DA1907F (0.07888888567686081) into mono.
Process pre-delay, then **five serial allpasses**, then **nine parallel
damped combs**, mixing comb outputs into left/right by separate tap vectors.
Steam0x69D57C / NSO0x7100416E7C establishes that topology. Allpass feedback
is bits0x3F1E353F. For delayed state d and input x, write ring=x+g*d and
output=(1-g*g)*d-g*x. Each comb outputs its raw delayed sample d; its ring
write is x+feedback*bandpass(d), with private bandpass history.

The following sections supply exact tap tables, delay initialization,
accumulation order and bus tail/link integration for this registered kernel.

## Reverb exact constant network and state progression

`proof: nso-body + steam-dispatch-pair`.
This completes the registered mode0 Reverb **effect kernel**; bus-link tap
placement is a separate graph-integration question, not part of its transfer
function. The authoritative NSO tables are0x710063E854/0x710063E878/
0x710063E88C/0x710063E8B0/0x710063E8D4. The readable Steam core
0x69D57C–0x69D708 has the same five-allpass/nine-comb loops and separate
left/right coefficient arrays. NSO helpers0x7100410D2C/0x7100410FB0/
0x7100410EB8 pair with Steam0x6AAF58/0x6AB694/0x6ABAA8.

The following entries are IEEE754 float32 **bit patterns**, in processing
order, not integer amplitudes:

| Table | Ordered values |
|---|---|
|Nine comb delay scales|3F8315F0,3F24E67C,3F334A1F,3F4895E7,3F69F7F1,3F953752,3FB2DB9C,3FA39AF5,3F8BDCF4|
|Five allpass times in ms|40F9999A,3FA7AE14,40D0F5C3,411851EC,40700000|
|Nine comb cutoff scales|3F40BAE4,3F836827,3F5B02EC,3FAF3589,3F7E0DA1,3F908C2B,3F6C8846,3F497D91,3F94ED82|
|Nine left mix taps|3FCCCCCD,BFA66666,3F800000,BF44EC4F,3F200000,3FB33333,3F36DB6E,3FE66666,3F0E38E4|
|Nine right mix taps|3F200000,3F44EC4F,3F800000,3FA66666,3FCCCCCD,3F36DB6E,3FB33333,3F0E38E4,3FE66666|

The allpass times are7.8,1.31,6.53,9.52,3.75ms as their exact stored floats.
For each, delay is trunc(time_ms*Fs/1000), clamped to[16,allocated_capacity].
Every allpass has g bits0x3F1E353F. Apply all five **serially** in listed
order before any comb; do not run them as parallel early-reflection taps.

Ring creation NSO0x71004104C0 initializes all sample storage to zero. If
maximum delay capacity is M, physical ring length is round_up(M+256,16).
Creation initially sets write index=floor(M/2), read index0 and active delay
floor(M/2). Setting active delay D moves read index to(write-D) mod physical
length without changing write index or clearing the ring. D is clamped as
above. Thus an ongoing parameter change must not recreate the delay state.
The nine comb capacities at creation use the configured maximum room-size
and the same prime-rounded delay-scale rule; their actual active delay and
feedback coefficients are then updated from the live parameters.

For each comb, preserve both input and output histories of its private
feedback bandpass. Given the raw delayed ring value d[n], calculate
z[n]=normalized_bandpass(d[n]) using that history; write
x[n]+g*z[n] to the ring; the comb's **output is d[n]**, not z[n].
All nine receive the same five-allpass result x. Accumulate L/R in comb
index order0..8; tap0 initializes each output, subsequent taps add to it.
There is no extra dry term, dry/wet percentage or normalization after these
tap sums inside this effect.

The kernel preserves its denormal-avoidance writes in their actual order.
At each <=256-frame chunk start, add2^-20 to mono input scratch sample0
and subtract2^-20 from pre-delay output scratch sample1, then execute the
pre-delay ring read/write copies, allpasses and combs. Record those writes
as ordered scratch operations; do not relocate them to the final output or
invent a random dither source.

For mode0 and >=2 output channels, output0=L and output1=R. When there are
>=6 outputs, output4=-R and output5=-L. When there are>=4 outputs,
outputs2 and3 are zeroed. The effect does not write additional channels6/7
in this mode; its caller's buffer policy remains separate. With fewer than
two outputs, the callback copies matching live channels and reports
completion immediately for ended input rather than running the network.
Only mode0 is present in either registered ACF; other routing modes are not
required by this install census.

### Reverb tail is an explicit clock, not the filter-state threshold

The effect retains a tail budget T and an ended-input counter C in frames.
Parameter update computes pre=trunc(min(pre_delay_ms,1000)*Fs/1000) and
rt=trunc(clamped_RT60_ms*Fs/1000); update T=max(T,pre+rt).
The pre-delay's minimum16 active frames does not replace the raw `pre`
term in that budget. The installed live vector contributes96960 frames at
48000Hz, but that is **not necessarily the retained budget**: create first
sets default RT60=3000ms, room=config_room/2=25, pre-delay=config_pre/2=500ms,
HP=0 and LP=8000, and calls update. For configuration[1000,50,0] at48000Hz
that initializes T=168000 frames (3500ms). Applying the authored2000/20
does not shrink T, so168000 remains the budget absent a later increase.

- Live input: if C>T, reset delay/comb/filter history first; then set C=0,
  report not-complete and process the block.
- Ended input: if C>T, report complete before processing; otherwise add the
  current block size to C, report not-complete and process that block.

The comparison is strictly `>`; the callback need not stop exactly on T.
Reverb reset clears pre-delay, all five allpass and all nine comb ring
storage, comb biquad histories and input scratch. It is not a cue-repeat
restart and does not manufacture new note-ons. These rules are paired in
Steam0x69D1E4/0x69D128 and NSO0x7100416A50/0x71004169B4.

## Authored non-neutral opcode32 filtering is now specified

`proof: nso-body + steam-dispatch-pair`.
The voice filter wrapper0x7100408938 and double design0x71004087A4 have
the same operations as registered-bus wrappers0x710040E224/0x710040E090,
with separate internal helper addresses. Their address-normalized retained
decompilations are identical; the bounded instructions establish the calls.
Voice coefficient normalization0x7100408AAC is the corresponding expanded
biquad coefficient construction. The Steam authored dispatch and effect2
application are the parameter and0x63DEB6 update paths already cited.

After opcode32 inheritance, lower knob combines as1-(1-a)*(1-b), upper knob
as a*b. Unset remains distinct from explicit0. Convert each resolved knob k
to cutoff: k<=2^-16 gives24Hz; k>=1-2^-16 gives24000Hz; otherwise
clamp(exp2((k*2.1735804080963135+1)*4.5849624),24,24000).
Use the bandpass equations and state rule above, including its subsequent
10Hz/Nyquist-minus100 clamp and invalid-band zero response. A resolved
24/24000 endpoint pair disables the **voice effect** instead of running
this filter. Every other enabled pair is a real bandpass; preserve its
history over blocks and parameter changes.

The install has17 opcode32 payload values; negative first signed16 resets
the family, not a negative cutoff. Among opcode64 payloads only the two
leading0/255 reset forms occur, so a non-neutral authored voice-biquad type
is positively absent from this install. This absence does not disable the
ACF's separate plugin Biquad type0 lowpass.

This authored32 coefficient/state specification supersedes the earlier
blanket non-neutral-filter refusal. Its absence proof for active64 is an
install fact, not permission to skip future unknown filter commands.

For exact Reverb updates, keep the unbounded prime result u separately from
the ring-clamped active delay: feedback's pow exponent uses u/Fs*1000/RT60,
while the delay setter independently clamps active length to capacity and
at least16 samples. With the installed room-size10 these lengths exceed16
and fit the configured room-size50 capacity; the distinction still belongs
in the general update rule.

## Registered wet-bus links and exact mix-stage order

`proof: nso-body + steam-dispatch-pair`.
The former link-Type2 OPEN is closed by the ACF installer and source-mixer
coefficient builder, not by an SDK enum name:

- NSO0x71003E7BC0 reads each link through0x71004059DC, maps authored Type
  through0x71006325C0 and calls0x710040B3C4→0x710040AA70.
- Steam0x6802D8 installs buses and calls0x6828A0. Its link loop at
  0x682950–0x682999 reads0x647648, maps through0x8CE368 (`.sdata`) and calls
  0x68DC38→0x68E06C with destination bus, send level and translated mode.
- Both translation tables are `[2,1,0]`: ACF Type2 maps to internal mode0.
  The recorded Steam constructor0x68D010 jumps to0x31F75100 in `.arch`;
  that target was not followed.

NSO0x710040ABA8 gives the **actual** mode law. For authored send s, source
gain g and channel-pan matrix P:

| ACF Type | Internal mode | Send coefficient matrix |
|---|---|---|
|0|2|s on the channel diagonal: before gain/pan|
|1|1|s*g on the channel diagonal: after gain, before pan|
|2|0|(s*g)*P: after gain and pan|

Thus installed BUS1/BUS2→MasterOut links are unity **post-volume,
post-pan** links. They carry processed bus output, not an additional copy
of the voice's dry input. BUS3..7 have no installed outgoing links.

The bus processor0x710040AFCC executes the effect slots in order0..7,
ping-ponging its channel buffers. An active effect's output becomes the
next effect's input; completion propagates through the callback's actual
out-argument (0x710040BBC4 copies its updated local, despite a misleading
constant0 in decompiler output). After effects, it calls the source mixer
0x710040A4EC to apply the bus gain/pan/send coefficients and accumulate in
destination bus buffers. Consequently BUS2's Bandpass precedes its Biquad,
and both precede its post-pan link. Reverb precedes BUS1's link. Apply
MasterOut's own gain/pan only when MasterOut is processed, not once for
each incoming edge.

Bus installation computes downstream rank recursively at0x710040B2FC and
sorts by descending rank at0x710040C480/0x710040C738 before processing
0x710040C5A0. This puts BUS1/BUS2 before MasterOut. Equal ranks compare0;
no cue-level priority between those independent buses is specified by that
comparator. The actual imported sorter is `qsort` (ELF GOT0x87F790), not
the stale `atoi` label in decompiler output.

MasterOut has no authored outgoing links, but backend bus0 receives a unity
internal-mode0 self-output route during0x710040C480. The root-bus special
handling in0x710040AFCC clears/copies its staging buffer before accumulation;
that is the final output collector, **not a recursive audio-feedback loop**.
Do not make MasterOut silent merely because BusLinkTable lacks a row for it.

### Gain/envelope/category order and dirty-state interpolation

Authored node gains and category/caller gains fold first as previously
specified. The folded gain is passed through0x71003B3468; negative gain is
clamped to0, and gain<=2^-16 is made0. It becomes part of the voice's
channel matrix through0x71003B7384, together with its per-channel gain/pan.
Voice filtering processes decoded/resampled samples before that matrix is
applied. Envelope state advances over the available voice render chunk;
its resulting factor reaches the source gain through0x710040A9B0. The
source mixer then forms the send/gain/pan coefficients. Therefore the
semantic path is:

decoded/resampled voice → voice filters → authored/category/caller channel
gain and pan × envelope × named send → bus effect chain → bus volume/pan
→ post-pan bus links → MasterOut controls/output.

Do not apply category volume again to the wet return. Category volume is
part of the voice contribution before buses, not a second master-bus gain.

For dirty coefficients0x710040A4EC constructs **old and new complete
matrices**, using old/new send, source gain and pan, respectively. It
interpolates each final matrix coefficient, not the separate factors.
For a chunk of N frames and old/new coefficient a/b, the scalar tail at
0x710040ACD4 uses current=a, step=(b-a)/N: accumulate current*x for the
current sample, then increment current. Thus samples use a+k*(b-a)/N,
k=0..N-1; b becomes the next chunk's old state. Unchanged coefficients
use direct multiply-add accumulation. Expanded block kernels implement
the same coefficient progression. Preserve native chunk boundaries when
reproducing non-neutral envelope changes; do not replace this with a
sample-at-a-time envelope that crosses stage boundaries differently.

## Common210-byte block: spatial controls and the100 gate

The following closes the previously unresolved0x0B/0x0C interpretation and
the standard block's resolved two-dimensional spatial path.
`proof: nso-body + steam-dispatch-pair`.
NSO scalar mapping0x71003C7BDC puts slots5/6/7 and0x0B/0x0C in the
pan family at context+0x158. The ordered fold0x71003CA918 / Steam0x633750
adds angle (slot5), multiplies distance/spread (slot6), pan volume (slot7)
and center/LFE sends (slots0x0B/0x0C). It does **not** let unity child writes
override a zero inherited send.

Projection NSO0x71003CE5E4 / Steam0x639CB8–0x639DD9 defaults angle0,
distance/spread1, pan volume1, center0 and LFE0. Therefore the sequence's
83/85=1, track84/86=0 and synth83/85=1 resolve center/LFE to0, exactly the
default sends. Those track zeros are not master-volume mutes.
In0x71003D17A4, a nonzero center send adds to the center output, or to
both outputs when rendering stereo. Mono→stereo center gain uses bits
0x3F353BEF per output; stereo→stereo center gain uses0x3F004DCE for each
source/output pair. LFE is added to output3 when present; for mono it uses
its full send, for stereo each source uses0x3F353BEF. Zero sends add nothing.

The effective pan mode is slot100, authored by70. Standard blocks store0.
Getter0x71003CC9F8 returns0 for that resolved value;0x71003CE4F0 then uses
the authored 2D pan family, not source/listener cone-distance calculations.
For mode2, runtime flag0x711D228408==1 requires a source at context+0x98;
otherwise both source+0x98 and listener+0xA0 are required. A resolved mode0
does not become3D merely because a source is attached.
Caller overrides still participate in the ordered scalar fold; evaluate the
resolved mode, not only the sequence's original byte.

For normal mode0 panning0x71003D0A28 wraps angle to(-180,180], locates the
two enclosing speakers and sets theta=(angle-low)/(high-low)*(pi/2).
The enclosing speakers receive cosApprox(theta) and sinApprox(theta), scaled
by pan volume, abs(distance) clamped to1 and their speaker gains. The
remaining1-abs(distance) contribution is equal over the layout's speakers
with gain sqrt(1/speaker_count). A negative distance reverses angle180deg.
The actual helpers are polynomials, **not generic libm sin/cos**:

- sinApprox(x)=x*(1-z/6*(1-z/20*(1-z/42*(1-z/72)))), z=x*x.
- cosApprox(x)=1-z/2*(1-z/12*(1-z/30*(1-z/56))).

Use float32 stage operations in NSO0x710041D030/0x710041D090. Default
front speaker angles are-30/+30 degrees; surround defaults-120/+120,
with newer8-channel layouts additionally-110/+110 and-150/+150 as
initialized at0x71003D1EA0. Default mono uses center angle0; stereo source
indices use left/right. Source-channel count census is2653 mono /772 stereo
waveform rows, with no higher-channel authored rows. The earlier port's
mono-duplication convention is port policy, not this newly specified
retail panning law.

Opcode90 is **event-clock rate**, not another audio gain: U/100 writes
slot0xA0 (default1), multiplied through its family.0x71003CD3A0 supplies
playback-context+0x88 at0x71003D4B6C; the event interpreter uses it when
advancing/compensating event time. All3209 authored values are100 (=1).
Opcode34's second byte is base voice priority (context+0x10C), combined
with the spatial priority offset by0x71003CDA88; installed values are0/100.
It is not an amplitude. Its first byte is0 in every observed payload.

**Opcode43=100 is not a license to skip the gate or RNG draw.**
Paired Steam0x659F74–0x659FA9 / NSO0x71003FA7EC onward draws through the
shared randomized-value helper with range[0,100], truncates the result
and accepts only if the authored u8 is **strictly greater** than it.
The underlying integer RNG domain includes0..0x7FFFFFFF and float rounding
can reach100. At that endpoint even an authored100 takes the rejection
path, clearing start admission. Sequence, track and synth parameter programs
each execute their own gate in their normal order; don't coalesce them.
A deterministic witness must supply/control those draws, including this
rare rejection edge, before claiming complete standard-block semantics.

## Remaining common-block spatial fields: complete parameter meaning

`proof: nso-body + steam-dispatch-pair`.
This section specifies the cone/distance/Doppler fields even when a caller
selects a3D mode. Their inactivity in resolved mode0 is a proved control
flow, not a parser omission. The Steam serialization dispatch is the67/68/
69/74/66 branch family in0x659428–0x65B8B0; NSO's scalar fold and spatial
consumers below supply the full body proof under the authorized policy.

| Command | Runtime field / meaning | Installed values |
|---|---|---|
|66|slot0x62, 3D gain multiplier|all8375 values1|
|67|slots0x55/0x56, inner/outer **half-cone angles in radians**|all3209 pairs360/360 authored degrees, yielding pi/pi|
|68|slot0x58, minimum distance|3091 zero,118 one|
|69|slot0x59, maximum distance|3091 zero,118 five hundred|
|74|slot0x5C, Doppler exponent|all3209 zero|

Command67 first converts each BE16/10000 to full degrees (times360).
NSO0x71003CE7C8→0x71003CEBA0 then sets inner=min(A/2,B/2) and
outer=min(B/2,180), dividing each by360 and multiplying by float32
2*pi. Do not feed full degrees directly into the cosine comparison.
The two angles add along the authored context fold;3D gain and Doppler
exponent multiply. Min/max distance replace when explicitly provided.
Other pan-family center/LFE and filter rules remain as specified above.

Before spatial calculation0x71003CCB0C starts from the global defaults,
overlays the cue's resolved context+0x160, then the registered source's
parameter override block, followed by its explicit AISAC-index overrides.
At each overlay only values other than bitwise0x7FFFFFFF are applied.
Thus source overrides have their real defined precedence; do not multiply
min/max distances or confuse an explicit zero with unset.

Defaults initialized at0x71003CEA20 and copied by0x71003CF8F0 are:
gain1, inner/outer pi/pi, outside-cone gain0, min/max distance0/0,
source-radius0, interior-distance0, Doppler exponent0, attenuation-curve
mode2 and five AISAC control indices-1. These are runtime defaults,
distinct from a missing UTF column.

### Cone and distance laws

For effective3D use the current source/listener state as inputs; positions,
orientation vectors, velocities and focus weights are runtime state, **not
extra ACB table columns**.0x71003CEC2C forms separate vectors:

- Rv=source_position - lerp(listener_position,listener_focus,volume_focus);
- Rp=source_position - lerp(listener_position,listener_focus,direction_focus).

Distances used for attenuation and pan spread are norm(Rv) and norm(Rp),
respectively, multiplied by the source distance factor. Distances are in
those scaled runtime world units; the ACB does not declare a meters
conversion. Cone direction uses Rp and source forward.

The cone helper0x71003CEF28 starts gain1. If attenuation is disabled, either
direction vector has zero length, or inner>=float32(pi-approximately4e-7),
gain stays1. The exact threshold is float32 bits0x40490FD9.
Otherwise let c=-dot(Rp,source_forward)/(norm(Rp)*norm(source_forward)),
ci=cosApprox(inner), co=cosApprox(outer):

- c<=co: cone gain=outside;
- co<c<ci: theta=acosf(c), t=min((outer-theta)/(outer-inner),1),
  cone gain=outside+(1-outside)*t;
- c>=ci: cone gain=1.

The current install's360/360 cones are pi/pi and take the full-cone
gain1 path. This proves their neutral angular contribution, rather than
assuming that an unknown pair of angles is harmless.

Distance helper0x71003CF078 returns normalized distance u:
0 if d<=min,1 if d>=max, otherwise(d-min)/(max-min).
It starts distance gain1. If attenuation is enabled and curve mode2,
an interior distance gets
expf(u*float32(0xC0ADF854)+float32(2.7182817))/float32(15.154263);
d>=max gets gain0. Disabled attenuation or other curve modes leave that
gain1 here; normalized distance can still feed registered AISAC controls.
The interior exponential's edge value is not silently substituted for
the explicit gain0 at/above max. The final spatial gain is
3D_gain*cone_gain*distance_gain.

Pan spread helper0x71003CF3CC uses radius r and interior width w:
spread0 at d<=r; (d-r)/w for r<d<=r+w; otherwise1.
It reports negative r/w as errors rather than normalizing them away.
With default r=w=0 it gives1 for positive distance and0 at coincidence.
The spatial projection inserts this spread and angle into the same
channel-pan matrix law already specified.

### Orientation, Doppler and priority

For nonzero direction R, forward F and up U, helper0x71003CF13C computes
h=dot(R,F), q=dot(R,cross(U,F)), then
azimuth=atan2f(q,h)*360/float32(2*pi), reversing its sign when runtime
coordinate flag0x711D228600 is1. Elevation is
atan2f(dot(R,U),sqrt(h*h+q*q))*360/float32(2*pi), folded into[-90,90].
A zero R gives both angles0. An unoriented source uses0x71003CF2D4's
acosf(normalized dot) angle with elevation0; oriented source azimuth is
made absolute for its cone-direction control.

Doppler helper0x71003CF468 starts ratio1 and leaves it1 if distance,
runtime velocity-unit factor or exponent is0. Otherwise let
c=340*velocity_unit_factor, denominator=c+dot(Rv,source_velocity)/d,
numerator=c+dot(Rv,listener_velocity)/d, where d is the scaled volume
distance supplied by0x71003CEC2C. If both terms are positive, ratio is
powf(numerator/denominator,Doppler_exponent); otherwise it stays1.
The authored exponent0 is a proven disabled contribution unless replaced
by a later explicit source override. expf/powf/acosf are actual ELF
imports at GOT0x87FDF8/0x87FE00/0x87F870, not guessed math identities.

When the source's distance-priority flag is enabled, its spatial priority
offset is trunc(normalized_distance*(-255)); otherwise0. Add that to
command34's base priority as described above. This is admission/stealing
priority, not extra attenuation.

### Timing and audio-frequency separation

Command60's summed cents determine audible frequency ratio through
exp2f(cents/1200), including its declared random range. Command121's slot4
sets the **maximum frequency-ratio reservation**, not an additional
transposition to multiply into the sounding pitch. In0x71003DD558 it
becomes the player's max-ratio field+0x290, capped by library max_pitch
and floored to1. The actual ratio is separately updated from slot3 into
player+0x294. Preserve resource bounds without accidentally double-shifting
the sound. Command90's event-clock rate is separate from both.

The complete210-byte program is therefore decoded as scheduling/admission,
category/routing, event-clock, gain/pitch and spatial-control instructions
with the equations above; it is never treated as a210-byte empty marker.
Every35-command stream still executes in serialized order, including43's
strict gate and its RNG draw. All standard zero/one/0xFFFF values have
their own specified effects or absence conditions rather than a blanket
“all defaults are no-op” exemption.

Transcendental precision is part of these laws: filter0x7100530AA0/
0x7100530AB0 resolve to `sinf`/`cosf`; bandpass prewarp0x7100531850
resolves to double `tan`; spatial projection0x7100530A70 is `atan2f`.
The pan polynomial helpers are separate and must not be substituted for
those imports in the filter designers.

Registered Reverb has no extra implicit dry or allpass-chain latency term:
each allpass has a direct path, while each comb outputs its delayed state.
For zero initial history, nonzero input's first wet response is after the
active pre-delay plus the minimum active comb delay, not plus the sum of
all five allpass times. Its ended-input block clock can extend beyond T:
for fixed positive block size N it processes(floor(T/N)+1)*N ended frames
before the next call reports complete. At T168000 and N256 this is168192
processed ended frames. This is derived from the specified strict->
callback, not a newly exercised audio-output receipt.

## Non-DSP closure census: presence and positive absence

An additional static pass hashes and parses all630 ACBs, without loading or
executing the game. Its complete retained receipt is
`GoldenRE-notes/acb/closure-census.json`, SHA256
`97fc99fceaa1ef3b8404ddad926f93eb348dca1da22f89d2b2eb51062c6d9526`;
errors are empty. This supplements, rather than replaces, the original
occurrence-preserving3,631-cue graph census.

The following have zero table rows in every sheet: global AISAC references,
action tracks, sequence/track/synth parameter pallets, block sequences and
blocks. All corresponding installed reference counts are0 and starts are
0xFFFF where present. No plugin/action execution semantics may be inferred
from those empty tables. They are not dependencies of this install profile.
All3,592 sequence and3,605 synth LocalAisacs blobs are empty. Track blobs
are empty in3,425 rows, but **37 are two bytes long**, each referencing one
local AISAC. Therefore the registered ACF's absence of global AISACs does
**not** prove that every cue is AISAC-free.

The37 local controllers occupy three sheets: skill/0A5 has1, snd00_bgm has34,
and bgm_en/snd00_bgm has2. Their38 graph rows comprise23 Type1 and15 Type15.
All controllers are Type1 with RandomRange0, AutoModulationIndex0xFFFF,
DefaultControlFlag0 and DefaultControl0. Local track-reference presence,
not a nonzero default-control flag, determines whether they must execute.
The existing dry profile's empty-blob admission rule deliberately excludes
them until their separate controller semantics are supported.

Looping is also positively present, not an unused-feature closure:
Waveform.LoopFlag2 occurs in165 rows,164 in snd00_bgm and1 in snd04_comse.
The other3,260 waveform rows have LoopFlag1. Each looping row's corresponding
extension and bounds are retained in the receipt. External repeat policy
and loop-stop/release interaction must not be silently erased from A5.

Unused selector types, arbitrary schema omissions, recursive graphs and
unseen event programs are outside this measured install. They remain
explicitly unsupported inputs, not missing prerequisites for reproducing
the measured graphs. All nonempty installed Type3/4 weight arrays use100;
no zero-weight degenerate program was found. The four empty weighted
sequences in bgm_en, cue IDs1752/1753(Type3) and1773/2014(Type4), have
NumTracks0 despite a retained100 weight. The construction function returns
before AISAC attachment, weighted-threshold generation or track creation
when NumTracks0: NSO0x71003FF69C early branch, paired Steam0x66EFA5 test
and0x66EFA9 branch to cleanup. Thus an empty weighted cue does **not**
consume the selector RNG merely because its weight blob is nonempty.

The39 old sheets all report raw Version0x00410000 and format string
`ACB Format/PSP2 Ver.0.41.00 Build:`. Every one has cue ID1, root reference
type2/index0, two Type0 synths, and references synth1 then waveform0;
both synth CommandIndex values are0xFFFF. No old sequence interpreter is
needed for these graphs. Their RIFF/EncodeType11 codec support remains
the separate audio seam's responsibility.

The synth's no-command compatibility predicate is exact, not a blanket
legacy dry-send rule. NSO0x7100401740 calls version accessor0x71003F7A60,
which reads ACB+0x70, and invokes0x7100400B9C only when
`((V >> 18) & 0x3FFF) < 0x49 && (V & 0xFFFF) > 0x112`.
The helper writes numeric bus0=1 and buses1..7=0. For all39 measured old
versions the second predicate is false. Do not apply that helper just
because CommandIndex is absent. Header reader0x71003F09A0 stores the BE32
Version at its state+0x68; that state is ACB+8. ACB+0x68 itself is not this
Version field. This distinguishes the header's Type and Version operands.

## Default RNG: paired algorithm, seeding and ownership

Both permitted builds implement the same four-u32 xorshift recurrence:
NSO0x710041D980–0x710041D9AC; Steam.shared0x6B39EC–0x6B3A1A.
All arithmetic below wraps modulo2^32; right shifts are logical.

```
t = x ^ (x << 11)
r = w ^ (w >> 19) ^ t ^ (t >> 8)
(x, y, z, w) = (y, z, w, r)
return r
```

Seed expansion is NSO0x710041D9B0–0x710041D9F0 and
Steam0x6B3A1C–0x6B3A69. With M=0x6C078965:

```
x = M * (seed ^ (seed >> 30))
y = M * (x ^ (x >> 30)) + 1
z = M * (y ^ (y >> 30)) + 2
w = M * (z ^ (z >> 30)) + 3
```

In particular x is **not** initialized by simply storing seed. The range
callback draws exactly once, computes unsigned
`span = lower <= upper ? upper-lower+1 : 1`, and returns
`lower + r % span`: NSO0x71003D079C, Steam0x62FED8. This is inclusive
modulo reduction, including its bias, not rejection sampling or an
exclusive-upper-bound library distribution. Preserve each caller's
previously specified threshold comparison as well as these bounds.

The callback record has five pointers: work-size, create, destroy, range
draw and seed. Default NSO record0x7100873020 points to
0x3D0748/0x3D0750/0x3D0798/0x3D079C/0x3D07D8.
Steam.sdata record0xA20BE0 points to
0x62FE90/0x62FE98/0x62FED0/0x62FED8/0x62FF08.
Work size is24 bytes. NSO0x3D0630 and Steam0x62FFCC copy the supplied
40-byte record or this default. Therefore this default algorithm must
not be asserted if a caller installs a different RNG callback.

Default creation seeds from the low32 bits of a platform clock value:
NSO0x3D0750→0x420E80 resolves to nn::os::GetSystemTick;
Steam0x62FE98→0x6B1114 obtains an opaque64-bit counter through IAT0x8874B8.
The exact Windows provider identity is not proved here and is not required
to reproduce a supplied seed. Do not label it a particular Windows clock
API by analogy. The allocator's initial four constants are overwritten
by creation's seed callback.

Library initialization creates two global RNG handles
(NSO0x3E0F08, globals0x711D228BE8/0x711D228BF0). Player construction creates
two handles at player+0x240 and+0x248, then **overwrites their creation
seeds**, in that order, with two inclusive0..INT_MAX draws from the selected
global RNG. Paired bodies: NSO0x3C2A70 and
Steam0x60F39F–0x60F3E1. Consequently a fresh timestamp per cue, a seed
hashed from cue ID, or cloning the global state into each player is wrong.
These handles persist with the player; the measured track programs have
no seed/reset command.

Per-player selection is NSO0x3C57A8 and Steam0x6105D4–0x61061A:
normally use+0x240; on the recognized Atom worker use+0x248. The NSO
recognition compares the registered worker object's+0x80 token with
`nn::os::GetThreadNamePointer(nn::os::GetCurrentThread())`, not an integer
thread ID. The Steam pair makes the same token comparison through its
platform helper. Global accessor0x3E1730 selects the corresponding global
handle by the same rule. Playback construction also retains+0x248 for
server-side work. Keep the two streams separate and preserve call order,
including probability43 draws even when its authored value is100.

The recurrence and callback dispatch do not lock or atomically update the
four-word state. Type4 also writes mutable sheet control-work state
(NSO0x3F1D38 writes its BE16 previous-choice field), rather than keeping
an independent previous choice for every cue invocation. The precise
outer concurrent-selection arbitration remains A1 OPEN; this evidence
does not license either racing shared state or inventing a deterministic
cross-thread order. A serialized witness with supplied seeds can now
exercise the exact default recurrence without claiming that remaining
concurrency proof.

## Installed local time-AISACs: clock, graph laws and inheritance

This section closes the graph/controller portion for the37 local track
references measured above, not an arbitrary AISAC plugin implementation.
The paired paths are:

| Operation | Steam.shared RVA | NSO VA |
|---|---|---|
| Track elapsed-control update |0x654D20–0x654DD2|0x71003D6064, time-controller branch|
| Controller control lookup |0x637808–0x6379E6|0x71003C914C|
| Attached graph application |0x638DEC–0x638FA9|0x71003C94E8|
| Graph type→runtime slot |0x6397E8–0x639A5A|0x71003C9640, tables0x710063D81C/0x710063D8F0|
| Graph evaluation/cache |0x668404–0x668803|0x71003F8508|
| Point interpolation |0x66AD70–0x66AF00|0x71003F83E4|
| Contribution setter |0x638FAC–0x63904C|0x71003C9004|
| Gain/angle contribution operator |0x635464,0x6354CA,0x635617|0x71003C78E0|

The34 snd00_bgm controllers use ControlId0..33, ControlType2; the two
bgm_en controllers use ControlId0..1, ControlType2. The skill/0A5 controller
uses old ID2000 and omits ControlType. Reader0x71003F3464 normalizes that
old Type1 ID by adding0x1F830, yielding0x20000. Modern ControlType2 yields
the same `0x20000 | ordinal` time-control domain. This compatibility is
Version-dependent, not permission to treat arbitrary control IDs as
elapsed milliseconds.

Attachment0x7100400514 recognizes Type1 without auto-modulation, stores
the local AISAC row ordinal at track+0xB6 and attaches the referenced
controller to its parameter context. In these measured sheets the
normalized control ID's low16 bits equal that ordinal. No user-supplied
default control is needed: the event interpreter updates the special
control while processing that track.

Let E be playback's signed64-bit accumulated microsecond clock at+0x50,
Q the server quantum in microseconds at global clock object+0x120, and
s the float32 playback clock multiplier at+0x88. The normal elapsed
control target is:

```
NSO:   t = float32((float64(Q)*float64(s) + float64(E)) / 1000.0)
Steam: t = float32((float64(Q)*float64(s) + float64(E)) * C)
C = float64 bits 0x3F50624DD2F1A9FC
```

Thus the graph controls are **milliseconds on the event/playback clock**,
not normalized0..1 values and not waveform sample positions. Preserve
opcode90 clock-speed effects. The different final divide/multiply forms
are recorded rather than claimed bit-identical for every possible clock.
The active-voice helper0x71003D8910 first checks each graph's dispatch
byte through0x71003F8A24. All37 installed GraphTypeIndexes blobs are
`00`(36) or`0000`(1), so every measured graph takes the standard elapsed
control fallback, not the separate parameter-task proxy path. This also
applies when the event program has ended but its live child keeps the
track resident.

There are only two measured graph target types:

| Graph.Type | Runtime slot | Destination representation | Contribution |
|---|---|---|---|
|1,23 rows|0, scalar volume gain|BE signed16 /10000|multiply gain|
|15,15 rows|5, pan/spatial angle|BE signed16 /10 degrees|add angle|

Controls are BE IEEE754 float32 arrays. Destinations are **signed**, also
for Type15:0xFF4C means-18 degrees, not a large positive angle. The two
single-point Type15 graphs are constant-18 and+31 degrees. Graph point
counts are1×2 rows,2×27,3×5 and4×4. Every Curve blob is a sequence of
BE16 values100, with one element per point, and every blob is present.
The permitted interpolation bodies take the linear branch for this
present-curve case; do not introduce an exponential or spline curve.

For ordered points(x[i], y[i]), first decode y with the table's divisor.
At or before x[0], return y[0]; at or after the last x, return its y.
Between adjacent points return, with float32 operations in this order:

```
dy = y[i+1] - y[i]
dx = x[i+1] - x[i]
dt = t - x[i]
result = y[i] + dt * (dy / dx)
```

No fused multiply-add is present in either paired body. A one-point
graph always returns that one destination. For skill/0A5 cue1 this gives
gain1 through334ms, a linear fade from1 to0 over334..918ms, then gain0.
It does not terminate or stop the underlying waveform at918ms.

Graph evaluation checks ControlWorkArea against the current float32 t;
on inequality it evaluates, writes DestinationWorkArea and updates the
control cache; on equality it returns the cached destination. The
serialized negative work values and placeholder destination values are
**not** an authored initial gain/angle. In particular do not initialize
audible parameters from DestinationWorkArea before evaluating its graph.

Contribution application preserves hierarchy and order. The resolved
context walks its+0x38 source chain nearest-first. At each source it
folds that source's authored scalar fields, then applies its attached
controllers, then advances to the next source. Paired end-of-source
calls are Steam0x636025 and NSO0x71003CC5B0; the narrower fold likewise
uses Steam0x635BE4 and NSO0x71003CA8DC. Within each controller list, retain
attachment order and graph-index order. Controller lookup searches the
parent chain before local control values, and returns the first matching
graph when resolving a derived control; it is not a generic last-writer
map.

The contribution setter treats bit pattern0x7FFFFFFF as the unset
sentinel. An unset target receives the graph result; an existing slot0
is multiplied by the graph gain and an existing slot5 has the graph
angle added. It marks the parameter dirty for the normal downstream
voice/mixer update. In particular a track fade does not replace the
sequence volume, category volume or envelope; it contributes to the
already specified scalar product before envelope/channel routing and
bus effects. Angle graphs feed the established pan/spatial law, not
independent L/R gain multipliers.

These graphs are driven by track/server control updates and the existing
downstream render-chunk update contract, not a newly asserted offline
per-output-sample AISAC clock. Exact combined fractional clock/output
conversion stays A6 OPEN. Nonzero random ranges, auto-modulation,
global controllers, parameter-task proxies and other graph types are
absent from these37 references; they require separate support/refusal,
not substitution with the measured Type1/15 laws. Category limit/steal
arbitration remains the non-graph part of A3 OPEN.
