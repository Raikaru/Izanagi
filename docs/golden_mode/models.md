# Native Golden models and motion (M8c-1 / M8c-2)

The format authority is the committed [model specification](specs/re/model-formats.md),
including its census and texture annexes. The native parser uses no P4G code,
third-party model loader, PS2 asset substitute, or glTF intermediate.

## Resource and renderer path

`golden_model.c` bounds AMD, both PAC variants, NECK_BENDER archives and all
measured GMO block/command edges. Its owned source graph preserves raw names,
every material/layer/animation command, all physical motions in file order,
options, effect adjuncts (including distinct tags) and outer suffixes. Reference
resolution counts same-type siblings in the specified ancestor scope. Suffixes
are never searched for more objects. PAC children remain bounded named members;
the existing Golden VFS selects the requested member before model conversion.

`golden_model_scene.c` builds each source model's independent bone hierarchy,
resolves DrawPart/material/arrays, unpacks all four UV sets and all six observed
weight lanes, and triangulates lists, alternating strips and fans without deleting
degenerate triangles. Weighted geometry retains its bind vertices and resolved
palette/subset joints, including all six lanes and 127/128 weight sums. Animated
poses use `boneWorld*bindOffset` and inverse-transpose normals without cumulative
rebaking or weight renormalization.

`golden_model_build.c` makes a genuine RenderWare clump/frame hierarchy and
geometry. It uses the existing raster lock/upload, atomic shader, camera and PNG
readback, without the replacement-model service's PS2 skeleton fallback.
`p4_golden_model_load` accepts only Golden mode and an origin-verified Golden VFS
source. A host resource retains the source graph, full archive chain, whole-source
SHA-256 (including all embedded textures) and converter id `amd-gmo-motion@2` until
`p4_golden_model_destroy`. No host pointer is placed in a guest model slot.

`p4_golden_model_attach` binds two owned Golden resources using the explicit
native bone index and helper id. It rejects missing/ambiguous helpers and resource
cycles. Attached children are drawn through their own clump's same atomic path;
destroying either resource detaches it without leaving dangling frame ownership.

The standalone viewer accepts `golden:<Golden VFS path>` with `P4PC_MODE=golden`
and `P4PC_GOLDEN` naming the owner's install. This explicitly selected model-only
witness does not lift the main game's table/script/asset admission gates. PS2
viewer modes and model loading are intentionally unchanged.

## Embedded images

`golden_model_image.c` handles every measured TIM2 tuple (RGB24, RGBA32,
indexed4/RGBA32, indexed8/RGBA32) and uncompressed positive-height 24-bit BMP.
It bounds complete pixel/palette/row spans before allocation, does not pass TIM2
into the TMX decoder, and never expands Golden alpha as PS2 GS alpha. Indexed8
applies the CSM1 permutation once; indexed4 does not. BMP output reverses padded
bottom-up BGR rows. Unmeasured encodings/mips are refused.

DDS binding is **OPEN U06**. Embedded textures are the default. External-only
texture blocks are named refusals, not blank/PS2/DDS guesses. The retained source
contains their original name bytes, including non-ASCII bytes.

## OPEN defaults and refusals

These are provisional choices for static witnesses, not claims of retail BBV:

| Card | Chosen static behavior / retained boundary |
|---|---|
| U01 | Source coordinates unchanged; column-vector `T*Rz*Ry*Rx*S`, XYZW quaternion; preserve strip winding. |
| U02 | Scoped same-type references; reject unresolved static bindings/cycles. Supplemental neck references remain retained, not bound by an incomplete owner hypothesis. |
| U03 | Weights, all UV pairs, colour, normal, position packing; signed fixed normal/position and unsigned fixed UV/weights; offset+scale quantization. Packed colours use low-bit R, then G/B/A. Unrecognized primitive semantics, including the single line draw, are not triangulated as triangles. |
| U04 | Bind/animated `boneWorld*bindOffset`, all lanes, no weight normalization; inverse-transpose normals. |
| U05 | Candidate diffuse RGBA, matte static lighting, embedded straight alpha over with zero-alpha discard, linear/repeat sampler; preserve source draw order. Do not map unresolved enums to PS2 `per3MatAlpha`. Raw material/layer states and all UV sets survive. Multiple layers, additional-UV layer binding and nonidentity UV matrices are named render refusals rather than flattened. |
| U06 | Row-major TIM2; low nibble first; indexed8 CSM1; preserve 0..255 alpha; embedded-only binding. |
| U07/U08/U15 | Binary16 time and values; constant steps, linear vectors, normalized shortest-arc spherical rotation; endpoint clamp and explicit loop wrap. Preserve all physical clips and naming exceptions. File-order base is separate from provisional slot+1 selection; absent clips refuse, empty clips restore rest. Leading `FFFF` time keys are retained and flagged OPEN, not mistaken for corrupt resources. |
| U09 | Helper offset TR/XYZW multiplied by owner bone, distinct helper ids and UserProperty tags; native helper frames follow animated owner bones. |
| U10 | Retain complete neck GMO, 32-byte parameter objects and suffixes; do not animate or reinterpret head parameters. |
| U11 | Retain every effect adjunct/tag; effect execution and motion lifetime coupling belong to M8f/M8c-2. Models requiring an effect adjunct are named render refusals, not apparently complete witnesses with disabled effects. |
| U12 | Accept the caller's explicit VFS identity; no model-id aliases or dependent-resource reordering. |
| U13 | Preserve suffix/options as opaque data, never execute or reinterpret pointer-looking words. |
| U14 | Bounded declaration/rare-command/custom-tag records remain in the graph; unsupported draws get named refusals. |

The spec's legal unmodified-retail owner captures are still required for semantic
qualification. Static native PNG inspection is not retail BBV, nor a claim to lift
R5/R7b/P5/P12/the render half of R3 for every material/effect/animated model.

## Qualification

Synthetic C vectors are registered as `golden_model`; `golden_model_ps2` exercises
the production source seam's PS2 early refusal. `golden_model_census` is optional
and skips without `P4PC_GOLDEN_STEAM` and `P4PC_GOLDEN_INVENTORY`.

For an owner-local exhaustive pass, run `tools/golden/model_native_census.py`
under the CPU governor with `--install`, `--inventory`, `--out`, `--static` and
`--driver <golden_model_cli argv...>`. Optional `--native <DLL>` accelerates only
the existing bounded CRILAYLA reader. Every source length/SHA must match the
frozen census. The pass requires exactly 5,778 objects, every per-type occurrence
count, 25,742 successfully decoded TIM2 and five BMP images; outputs contain
metadata/refusals only. No source payload or PNG is committed or sent remotely.

## Motion bank and playback

`golden_model_motion.c` indexes every motion and animate binding without changing
the source graph. Every measured half-precision constant/linear/spherical key row
is bounded; finite time sequences are checked without sorting away file order.
Duplicate times select the last key at an exact endpoint. The owner-local census
also checks all format/dimension counts, 3,345 target sentinels, 647 external
targets (638 neck material references and nine bone exceptions), and all 122
noninitial naming exceptions. A motion's `model`, physical `ordinal`, raw name,
frame interval/rate and complete target/command/option/reference are distinct.
There is no universal 32-clip capacity: all 35 physical entries remain available.

`p4_golden_model_play` selects an owning model and a physical slot (`-1` is the
base), takes absolute seconds and explicit looping, and forwards all ten MODEL0
record bytes into the native resource. It never treats the fifth halfword as a
motion id or alters the contiguous MODEL0 backing bank or completion ordering.
The caller remains responsible for the committed gameplay-to-slot mapping:
logical 30..33 to 28..31, state-selected 27, protagonist special-state 28,
and **no** logical34-to27 alias. Short files do not acquire invented motions.
For non-table resources, `p4_golden_model_play_physical` selects any actual
physical ordinal, including entries 33 and 34; zero is the base clip.
Scene evaluation updates native bone frames, the complete skin, helper worlds,
geometry bounds and the existing native atomic upload revision.

Playback currently refuses unresolved sentinel/external/supplemental-neck
channels and material/layer channels with unestablished execution semantics,
with the command offset in the error. Their curves are decoded and retained;
they are not silently dropped or counted as animated readiness. NECK_BENDER
parameters and effect adjunct execution/lifetime still require U10/U11 owner
qualification. Effect-dependent models retain the existing M8f render refusal.
These restrictions are independent from the ordinary bone/scale playback path.

Steam measurement additionally found leading binary16 time word `FFFF` in
material curves (constant and linear). It is an explicit OPEN U15 pre-start
marker candidate, not a finite time. The low-level evaluator uses following
finite keys for interpolation, preserves the pre-start row verbatim through
`p4_golden_model_curve_key`, and flags affected channels so production pose
playback cannot silently assume its retail meaning.

`golden_motion` contains synthetic keyframe, antipode, fractional/half-time,
duplicate-time, sentinel, loop/end, six-lane palette/subset, nonuniform normal,
rest/animated helper, empty-clip and all-five-extra-slot vectors. These run with
no game data. `tests/golden_motion_steam_test.py` is an owner-only, governed native
GPU witness harness; its frame strips require actual visual inspection and are
not unmodified-retail BBV or game admission proofs.
The unchanged `golden:` viewer can capture an explicit animated frame by setting
both `P4PC_GOLDEN_MODEL_SLOT` (`-1..31`) and `P4PC_GOLDEN_MODEL_SECONDS` (finite
absolute time). These Golden-resource-only witness controls select nonlooped
playback; malformed/partial controls refuse instead of silently capturing rest.
Normal loads with neither control remain at rest. Runtime code uses the typed
playback API rather than environment variables or a host-time animation clock.
