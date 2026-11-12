# Golden replay of the functions the random cases cannot reach

86 override functions produced no valid random case (their native needs RW engine state: free lists, pipelines, vector tables). `ovdiff -R`
(tools/mipsdiff/replay.c) replays recorded retail calls instead: real arguments and the memory they point at (per-call
pages, global windows, VU0 registers). The golden data (`golden9`, the per-call page snapshots of 23 scenes x plain +
3 mash seeds, and the older `golden` set) is retail memory recorded with the private PCSX2 trace tooling, never committed. Best result of the two sets per function:

| replay result | functions |
|---|---|
| ok | 20 |
| documented difference | 3 |
| mismatch | 0 |
| inconclusive (the retail code in the interpreter answers like the native: the recording lacks state) | 7 |
| crash on every recorded call | 18 |
| no recorded call | 38 |

- **ok** (20): `00394c70`, `00397390`, `003b6f00` (RpRandomSeed), `003b7060` (RpRandom), `003c55f0`, `003cad90`, `003cbf30`, `003cfa80`, `003d3ae0`, `003d72a0`, `003d7a20`, `003d8070`, `00413b80`, `00413e50`, `004143f0`, `0043a978`, `0048a510` (mdlEffectAnchorOnScreen), `004b50f0` (EffBattleAllocWork), `004b55a0`, `004b5620`.
- **documented difference:** `003cf140`, `0048a460` (mdlEffectProjectAnchorDepth), `0048a650` (mdlEffectAnchorScreenScale) (the GL node body replaces the sky2 DMA packet by design).
- **mismatch** (0): none.
- **inconclusive** (7): `00396fb0`, `0039bbd0`, `003a6ba0`, `003a7160`, `003a7f80`, `003a8500`, `003ca740`.
- **crash on every call** (18): `00396750`, `00398350`, `003a2340`, `003a2390`, `003bfd00`, `003c0520`, `003c0700`, `003c5510`, `003c8dd0`, `003c9d40`, `003c9eb0`, `003ca5a0`, `003d0540`, `003d3bd0`, `003d7cf0`, `0049a370` (RingCreateKindInstance), `004a6c00` (mdlEffectCreateGridForRecord), `004a6e70` (mdlEffectRenderFlares). Most are calls at boot (frame 199, the RW plugin constructors and registry init) where the native
  dereferences engine structures that retail builds in its own heap; the page sets were discovered with an earlier native and miss pages the
  current one reads.
- **no recorded call** (38): `001f5680`, `00276420` (frFontRasterUploadGlyph), `003130e0` (fclFuseLowestCandidateAtLeast), `003133b0` (fclFuseLowestCandidateAtMost), `00390680`, `00391400`, `00391c10`, `00396a40`, `003971d0`, `00399530`, `00399db0`, `0039f050`, `003a6df0`, `003bebb0`, `003c0960`, `003c65b0`, `003c66c0`, `003c67b0`, `003c70e0`, `003c9940`, `003caa50`, `003ce9e0`, `003cebb0`, `003ceeb0`, `003d0460`, `003d5330`, `003d5600`, `003d6c50`, `003d6fb0`, `003d7350`, `003d8c00`, `003db490`, `003f24a0`, `003f26c0`, `00488d70` (ParticleDrawBillboards), `004941f0`, `00494740`, `004b4cb0` (LineNovaCreateKindInstance). The scenes recorded so far never run them.
- `ovdiff -R ... -E` replays the same recorded calls through the R5900 interpreter (EE float semantics) on the retail code: where it matches the
  recording and the native does not, the native is wrong; where it answers like the native, state is missing. `00415bc0` (slerp): the native
  differs in 9 of 12 calls, the interpreter is exact or within tolerance on all 12 (EE truncation under cancellation).
- The replay loads recorded function pointers in engine data (`0x00887xxx` vtables) as the port's functions at those retail addresses, the
  sky driver's VU0 multiplies in the engine vector slots included (`0x00402390..0x00402570`, `src/platform/rw_skytran.c`). The interpreter side
  (`-E`, and the rerun that turns a difference the retail code reproduces into an inconclusive call) starts from the recorded VU0 registers, as
  the native does. Over the 27 recorded sets `0048a460` and `0048a510` are exact wherever the camera page was recorded (267 and 276 calls; the
  generic C vector functions the port used before left 77 calls of `0048a460` within tolerance) and inconclusive where it was not; `0048a650`
  reads the camera's frame (camera +4) for the eye position, a page no recording holds, so all its calls are inconclusive.
- Port natives that golden8 flagged in `src/movie`: `00509268`, `005097e8`, `0050d4d0`, `005208c8` (mwply.c, sfmpvp.c) replace Sofdec's player and
  its IPU/libmpeg state with a libavcodec player. Retail returns its static player slot (`0x0074430c`) and keeps the decoder state in the SFD
  object; the port returns a host object, so the retail return values and object words differ by design.

## Paired caller differential

`ovdiff --paired -R golden.jsonl -W windows.jsonl -G <ELF> <address>...`
runs the selected retail EE-assembly body and its native override separately
from the same recorded entry snapshots, arguments, stack and recorded VU state.
Both callers use the same native callees, pinned allocator and logged callbacks.
The dirty-page tracker restores all guest writes between the two executions,
including writes outside the recorded regions; the arena and allocator are
re-seeded independently before each execution.

This mode compares the complete return value, callback order/arguments and
touched guest bytes exactly, without float tolerances or expected-difference
allowlists. As in the existing differential harness, stack-address identities
and native function-pointer representations are canonicalized. The selected
EE caller's own local stack scratch is not a guest-state output; caller-stack
arguments at and above entry SP remain observable. Callback-log or dirty-page
capacity exhaustion, any fault, unsupported execution, a missing target or an
incomplete pair fails the run. `paired exact` is reported separately from
strict golden replay and is not a claim that native GPU callbacks reproduce
retail GS packets.

For callers whose native font callees lock/upload rasters, add
`--paired-device` and use the gates' headless SDL driver
(`SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy P4PC_INPUT=script
P4PC_HIDDEN_WINDOW=1 P4PC_CONFIG=none`). This opt-in initializes the actual
p4gl context before either entry snapshot is loaded. Both sides use the same
production RW raster CREATE, LOCK, UNLOCK and palette handlers, not a callback
that fabricates an output pointer. Recorded raster descriptors must be present.
The production device creates private CPU shadows in the port's extra memory
above retail's 32 MiB and uploads real GL textures; both are destroyed between
callers. Caller bytes are captured before teardown. The real allocator's guest
bins remain live through destruction; only then are caller and cleanup writes
rolled back together. Cleanup-only writes never enter the observation.
The comparison remains over retail guest RAM, returns and callback
logs, not GL texture storage. Raster callbacks log the raster and mode; the
callee-local output-pointer temporary is not a caller input. Device/context
failure is incomplete coverage and fails the run. This switch is rejected
outside paired mode or without global snapshots (`-G`), and does not alter
strict golden execution. Paired logs hold 4096 callbacks; the strict golden
limit remains 256. Paired log exhaustion remains an error.

Strict `-R` / `-E` golden comparison is unchanged. `--paired` cannot be combined
with `-E`, full-callee interpretation (`-F`) or discovery (`-D`). Use it to
qualify an override's caller logic/math/draw arguments against assembly where
the port intentionally supplies native rendering callees. Real renderer/device
captures are still required to qualify the resulting picture.

`mipsdiff_replay_oracle[64]` also checks paired operation with synthetic records,
including false golden post-images, caller-stack inputs and full 64-bit returns.
Private synthetic EE-body mutations independently prove that a wrong return,
a changed guest word and an added allocator callback each produce a paired
mismatch. A real paletted-raster lock also must report its expected callback
mismatch after production destruction, guarding the allocator/rollback lifetime.
No retail bytes are stored in the test source.

Native callee transport retains the complete canonical signature (up to sixteen
parameters / thirty-two i386 words), rather than truncating at eight positional
parameters. EE GP and FP register sequences are extracted independently; GP
overflow uses the caller's eight-byte outgoing stack slots. This matters for
the thirteen-argument sized-sprite call and nine-argument font calls. The ABI
self-test uses the real SDK signatures, verifies a mixed thirteen-argument
native invocation and two consecutive GP spills from a fourteen-argument call.
These execution probes use `P4PC_ELF` to initialize guest memory.


Small by-value record arguments are classified by the existing semantic
Clang layout oracle, using the matching generated mirror and `obj64/abi.json`
(`python tools/mipsdiff/sigs.py --gen build/gen`). Up to eight bytes occupy one
EE GP register; five-to-eight-byte values require two i386 stack words, not one.
The catalog preserves the port64 mirror's existing `sx` integer lowering for
float records. Unlowered float/HFA and unaligned memory classes retain their
native register/stack convention. Larger or unresolved record boundaries are
explicitly unsupported rather than guessed as one integer. The ABI execution
probe consumes both halves of a generic eight-byte record and all following
mixed-class arguments; it also checks the regenerated SDK footer signature.

Recording indexes use 64-bit `off_t` and `ftello`/`fseeko` on i386 as well as
x86-64. A seek error fails the run instead of replaying a different call from
a multi-gigabyte golden file.
