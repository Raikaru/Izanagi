# Golden cue sheets and HCA audio (M8d)

## Provenance and licence gate

The cue/container readers are original project implementations from public
format facts and measurements of the owner's local Steam build 10836033.
No protected executable code, RE notes or third-party ACB/AWB implementation
is an implementation input. Game data and decoded WAVs stay on the owner PC.

Public framing references (facts only, no copied implementation):
- [CRI ACB](https://reverseengineering.wiki/index.php/Format:CRI_ACB)
- [AFS2/AWB](https://reverseengineering.wiki/index.php/Format:AFS2_(AWB))
- [HCA](https://reverseengineering.wiki/index.php/Format:HCA)
- [Cue graph](https://reverseengineering.wiki/index.php/Fact:CRI_ACB/behavior/Cue_to_waveform_chain)

Audio decoding uses the already shipped **FFmpeg 7.1.1** `libavcodec` HCA
decoder under **LGPL-2.1-or-later**, with the HCA-only alignment patch below.
Exact upstream tarball SHA-256:
`733984395e0dbbe5c046abda2dc49a5544e7e0e1e2366bba849222ae9e3a03b1`.
Review inputs are FFmpeg tag `n7.1.1`, `libavcodec/hcadec.c`,
`libavcodec/hca_data.h`, `libavformat/hca.c` (extradata protocol only),
and the upstream configure dependency declarations. The decoder uses FFmpeg's
internal CRC, floating-point DSP and transform utilities; no external library,
new vendored source or new runtime library is introduced. No decoder code is
copied into the port. It remains dynamically linked and replaceable.

The only added configure component is `--enable-decoder=hca`. Existing
`--enable-decoder=mpeg2video --enable-parser=mpegvideo
--enable-decoder=adpcm_adx`, avcodec/avutil/swscale and shared-library policy
stay intact. `--disable-everything`, avformat/device/filter/swresample,
program/network/autodetection exclusions stay intact. GPL, nonfree and version3
components stay disabled; `ffmpeg_license_test` preserves the LGPL checks and
also requires HCA in release builds. Package notices explicitly identify the
modified FFmpeg; its original source, exact recipe and patch ship together.

## Container contract

An ACB is an `@UTF` Header row whose binary cells hold nested cue, waveform,
synth and sequence tables and optionally `AwbFile`. Numeric values are big
endian. Data-cell offsets are relative to the table's data area. Reuse the
CPK UTF reader; bound each accessed binary cell independently. Cue names
index CueTable, and CueTable references a waveform, synth or sequence graph;
a waveform's ID addresses its memory or external wave bank, not a row index.
Shared waves, multi-wave cues and nested synth references are not interchangeable
with the first waveform in the file.

AFS2 has a 16-byte little-endian header, variable-width ID and offset tables,
`count + 1` offsets, alignment and a 16-bit HCA subkey. Round each start up to
alignment; the next raw offset is its end (do not round that end up). Validate
table sizes, monotonic extents, IDs and every selected payload. Older ACBs can
embed ITOC-only CPK banks; these use the existing CPK framing.

HCA tags are compared after clearing bit 7. The `fmt` chunk supplies channels,
sample rate, block count, encoder delay and terminal padding. Every block has
1024 source samples; audible duration is `(blocks * 1024 - delay - padding) /
rate`. `comp`/`dec` supply fixed block size. Loop positions are source block
indices with start delay/end padding. Bounded packet reads and header/block
CRC checks precede successful decode; unknown framing is refused explicitly.

## Private key source (owner decision D4)

The user enters `[golden] hca_key` in their own `p4pc.ini`: 1–16 hexadecimal
digits, optional `0x`. Empty or zero means **no key**. Keyed audio is refused
before opening a decoder without a user key. Malformed key settings produce
only a generic error. There is no embedded key, public-key fallback, derivation,
key CLI argument or key environment variable.

FFmpeg 7.1.1 consumes HCA header bytes followed by ten extradata bytes:
big-endian u64 user key, then big-endian u16 AWB subkey. This deliberately
does not enable libavformat or its `hca_lowkey`/`hca_highkey` options.
Generic setting getters redact the private key; the decoder uses a dedicated
numeric getter. Keys are not included in diagnostics, source fingerprints,
WAV receipts or capture settings. Lockstep config snapshots blank only the
secret value and preserve all non-secret bytes.
Numeric temporary copies are explicitly cleared after handoff. The owner's
INI, live config and FFmpeg decoder retain the key as needed; raw OS
register/stack crash dumps are not a secure-memory boundary. The redaction
contract covers structured diagnostics, crash metadata and captures, not
arbitrary process-memory access (Main's privacy ruling).

FFmpeg 7.1.1 implements cipher 0 and 56, not cipher 1. The adapter explicitly
refuses cipher 1 rather than inventing a decoder; it is absent in this install.

The mixer producer seam is `p4snd_golden_cue_start/update/done/destroy`.
The caller retains the parsed ACB/bank and pumps from its producer thread;
no HCA decode runs in the SDL mixer callback. The existing mixer/resampler,
music/effects/voice categories, output device and gain path are reused.
The seam executes the fixed-rate profile in
[`acb-sequence-playback.md`](specs/re/acb-sequence-playback.md): Type0 starts
all track/reference occurrences, relative millisecond waits accumulate, note-ons
are nonblocking, and end markers do not stop children. Nested origins, repeated
references and delayed zero-sample children retain independent lifetimes.
An empty cue completes without inventing a waveform. Output-frame delays are
applied by the existing mixer; decoding remains entirely on the producer.

`p4_golden_acb_resolve` is a topology inventory, not a playback predicate.
`p4_golden_acb_plan` emits selected occurrences, starts and a finite max-end
bound, or an explicit named refusal. It validates terminal events, payload
widths, selectors, clock ratios, optional indices and actual recursion.
Every alternative is validated; only selected notes launch. Notes are sorted
chronologically with authored order retained at equal starts.

M8d2 implements the parameter-aware A3/A4-D dry profile: opcode87 and
zero-range89 gain products, BE32 category65, named111 send products, and
explicitly skipped112. `p4_golden_acf_qualify` checks supplied ACF bytes against
the two measured hashes; `p4_golden_acb_set_dry_acf` attaches the snapshot.
Category IDs2/8/9 have measured unity gain and capacities4/1/2. One cue, not
each waveform, occupies a category instance; live/queued children retain it.
Capacity exhaustion, ACF replacement and unresolved same-group arbitration
refuse instead of stealing. Start/update/destroy must be producer-serialized.
This mode requires known unity CRI caller/category controls and initially
clear wet-DSP state; normal host mixer controls still apply exactly once.

The complete DSP laws are now specified. Wet/envelope/spatial or other
parameter paths outside this implemented profile return **`pending:M8d3-dsp`**,
an implementation obligation, **not an A4 spec gap**. M8d3 owns those kernels
and parameter laws, including the now-specified local time-AISAC curves.
Nonempty `LocalAisacs` blobs therefore use the same pending implementation
label, not a stale A3 spec-gap claim or an invented count column.
Actual unresolved concurrency/category arbitration and loop/fractional-time
policies retain A1/A3/A5/A6 refusals. Unseen types/schemas and malformed graphs
retain A2/A7/A8 unsupported/invalid-input refusals; the spec's newer closure
census proves those three are not missing semantics in the measured install.
Retail hierarchy column-name sets are checked completely, separately from the
explicit port fixture family.
The39 measured old sheets resolve to RIFF/EncodeType11. Their refusal reports
the independently missing **CODEC11** decoder route even though the modern
profile does not interpret their parameter schema; they are not stale A7 gaps.

Type3/4 selection requires `p4_golden_acb_set_random` with a caller-supplied
inclusive-integer callback. The seed/stream algorithms are now specified, but
default concurrent worker draw arbitration remains unresolved (A1).
Type4 previous choice is per sheet/sequence row, not a global wave ID; starts
and planning on one sheet must be producer-thread serialized. Replacing the
callback resets that state as an explicit port policy, not a retail reset claim.
Unseen/degenerate selector cases and nonzero fine waits refuse. Missing named
Type/command columns retain the explicitly labeled synthetic port fixture
profile (Type0/absent commands), not a claim of valid retail authoring.

Finite group duration is the latest scheduled waveform end, retaining an event
program's later terminal time where applicable. Each waveform duration is
`ceil(NumSamples*48000/SamplingRate)` and each millisecond is48 output frames.
`CueTable.Length` is not a cutoff. Completion requires both the mixer-clock
bound and final per-voice drain. Multi-wave external loop overrides refuse A5;
the existing single-wave loop policy and explicit ADX voice path are retained.

The producer owns an independent decoder/ring per selected waveform and reuses
one update-local scratch buffer (empty cues allocate no PCM buffer). Group play/pause is one mixer-lock
transition, so voices in a group start on the same output frame. A packet
failure stops and flushes the entire group; successful completion requires
every decoder to reach its bound and every ring to drain.

## Installed framing census (2026-11-12)

Owner-local metadata measurement against every matching row of the M7c
inventory, with each object's SHA-256 checked before parsing, found:

| Object/table | Count |
|---|---:|
| ACB files | 630 |
| External AFS2 AWB files | 1,100 |
| Embedded AFS2 / ITOC CPK / empty banks | 589 / 39 / 2 |
| Cue rows / cue-name rows | 3,631 / 3,631 |
| Waveform rows / synth rows | 3,425 / 3,605 |
| Sequence rows / track rows | 3,592 / 3,462 |
| Cue ReferenceType 3 (sequence) / 2 (synth) | 3,592 / 39 |
| HCA waveform EncodeType 2 / other EncodeType 11 | 3,386 / 39 |
| Memory / streamed waveform rows | 2,361 / 1,064 |

Counts retain nested and installed-locale duplicates. ACB distribution:
594 in `data.cpk`, 12 each in `data_c.cpk`, `data_e.cpk`, `data_k.cpk`.
M7c's independent HCA header census measured all 3,386 as cipher 0 (clear).
No real-file key is needed or derived. Keyed refusal/supplied-key protocol
qualification therefore uses synthetic fixtures in addition to real clear
Steam decode. EncodeType 11 is not misrepresented as HCA.

An owner-local follow-up census of all 3,631 cue graphs at `42351a3d7` found
the following reference-edge counts. Repeated references are counted, not
deduplicated; this is topology, **not** a claim that all referenced waves
play simultaneously. The owner-local audio receipt retains every cue row.

| Referenced waves | Cue count |
|---|---:|
| 0 | 707 |
| 1 | 2,563 |
| 2 | 183 |
| 3 | 123 |
| 4 | 25 |
| 5 | 23 |
| 6 | 2 |
| 7 | 2 |
| 8 | 2 |
| 11 | 1 |

75 newer headers declare 96 columns; their trailing `PaddingArea`,
`StreamAwbTocWork`, `StreamAwbAfs2Header` are outside the declared row width.
516 headers declare 64 columns and 39 declare 31. The additive selected-column
UTF opener reads Header's needed cells and validates their bounds while not
accessing those unused work columns. The original strict CPK UTF opener is
unchanged. All accessed nested tables use the ordinary UTF reader.


The 1,100 external AWBs contain 51,050 bank entries: 1,064 HCA and 49,986
legacy ADX-signature entries (`80000024` in the M7c metadata inventory).
The 39 older memory-CPK waveform payloads are RIFF, not HCA. These payloads
are retained as bounded bank spans; this milestone does not mislabel them
as decoded HCA. In total the native bank census covers 53,411 entries,
including 3,386 HCA and 50,025 non-HCA spans.

A zero-file ITOC partition can still contain a valid nonempty UTF packet with
zero rows. The memory-CPK reader validates that packet against `FilesH == 0`;
it does not confuse an empty table with a missing binary cell.

## Actual Golden voice route

The explicit English `sound/adx2/en/v*.awb` voice banks contain ordinary ADX,
not HCA and not an accompanying ACB. Main's measured-format ruling requires
the voice witness to use one of these real banks; a character SE is not
relabeled as voice.

`golden_adx.c` bounds the ADX header, fixed 18-byte/channel packets and original
sample count, then incrementally feeds the existing **unchanged**
`src/movie/adx_dec.c`. The queue stays bounded and the last 32-sample packet
is trimmed to the header's sample count. Encrypted/unsupported ADX framing is
refused. No new codec or decoder implementation is introduced.
`p4snd_golden_awb_voice_start` looks up a numeric AWB ID and reuses the existing
stream voice in the voice category. Both Golden start APIs refuse PS2 before
dereferencing input buffers or creating a decoder/device.

## Cue duration and loop authority

CRI's public [waveform information contract](https://game.criware.jp/manual/native/adx2_en/latest/structCriAtomExWaveformInfoTag.html)
defines the cue waveform's `num_samples` as its total sample count. The ACB
`NumSamples` is therefore the cue playback bound; the HCA header bounds how
many decoded samples are available, and may include a trailing codec guard.
The adapter refuses a cue that asks for more than HCA can produce, and does
not expose the unused guard as part of the cue's WAV or mixer duration.
`WaveformExtensionDataTable` supplies sample-coordinate loop overrides
`[LoopStart, LoopEnd)`; absent overrides use HCA's bounded loop metadata.

Measured example, without payload: BGM cue1, bank wave30, 48kHz stereo.
ACB: 4,511,762 samples, extension loop [716,938, 4,511,762).
HCA: 4,409 blocks, encoder delay2,038, padding650, 4,512,128 available samples.
HCA's own loop metadata resolves to the same ACB interval. The extra366
decoded guard samples are not part of this cue. This is a generic
container-duration rule, not a cue-ID exception. The synthetic suite covers
shorter cue bounds, refused overlong bounds and independent ACB loop ranges.
The real music witness selects a looping stereo track longer than10s rather
than relabeling a short sound stored in the same BGM bank.

## HCA SIMD window alignment patch

`ci/ffmpeg-7.1.1-hca-window-alignment.patch` is a one-line LGPL-2.1-or-later
change to upstream `libavcodec/hca_data.h`: `DECLARE_ALIGNED(32)` on the HCA
window constant. No coefficients, arithmetic, other decoder or CPU dispatch
changes. The plain array landed at RVA0x6bdf8 (8 modulo16) in the reviewed
llvm-mingw i686 DLL. Its SIMD `vector_fmul_window` executed `movaps` on that
unaligned pointer and faulted; this is not a port stack-alignment workaround.

Every dependency recipe applies the same checked-in patch after unpacking the
pinned tarball. A byte-for-byte installed patch sentinel invalidates old cached
deps even when the decoder option list is unchanged. Shipping corresponding
sources preserve `ci/<platform>/build_deps.sh` and `ci/ffmpeg-7.1.1-hca-window-alignment.patch`
relative to each other, with notices identifying FFmpeg as modified. The
stereo HCA decode regression runs on Win32 as well as the other CPU lanes.
The fix is upstreamable; it has not been submitted upstream in this session.
