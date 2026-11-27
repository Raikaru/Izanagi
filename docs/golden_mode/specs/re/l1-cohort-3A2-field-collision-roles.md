# L1 cohort 3A2: explicit field model roles

**2 D10c / 64 PS2 bytes:** `005efce0` (`per3ShadowCollis`) and `005efd00` (`per4CameraCollis`). This retires the named-material **partition mechanism**, not shadow/camera functionality.

The original retail `1510c0` instructions positively show both keys queried on loaded worlds (`457ce0`) and clumps (`457c90`). The shadow-key branch stores the handle at old resource+`0x10`; the camera-key branch stores at+`0x14`. The complete original words and replacement code are in `l1-cohort-3A2-field-collision-roles-proof.json`.

## Positive successor, not absence

* NSO `1ece80` constructs separate **main**, **hit/atari**, and optional **camera** model filenames. The camera name derives from the hit stem plus `_cam.AMD`, not the main stem.
* `1ee650` accepts those explicit roles; `1ee490` is the single-main variant. In loose mode, main/hit are created directly; camera also requires a nonnull name and the actual existence test returning one.
* Packed loader `1ed0d0` reads three distinct descriptor names at `0/0x80/0x100`, creates native model handles at resource+`0x28/0x38/0x40`, and releases each resource request. Main is created from the returned payload; hit/camera require nonnull data and nonzero size. It uses the **ordinary** archive getter `1d76f0`, not the model-member getter `1d77a0`.
* `1ed200` checks readiness of main then each nonnull hit/camera handle in packed mode, then applies the source's seasonal mesh cleanup. Outside packed mode it returns one; no extra old stream-poll policy is invented.

This positive full construction/loading/readiness graph replaces material-key classification. It is not a no-reference/string-absence claim, a module-label inference, or a mapping from the unrelated five field-object metadata selectors.

Field/room normalization domains and scene-specific single/two/three-model selection remain distinct. The proof records those exact rules, including inverse-normalized main versus forward-normalized hit/camera coordinates and optional camera existence. Do not broaden camera creation to every scene or silently unify the getters.

Actual Steam `.sdata` independently supplies `field/model/f`, `field/model/atari/h` and `_cam.AMD`. An ordinary Steam constructor implementation has **not** been positively located; none is claimed. Steam `1efcc2` is a different scene-model name consumer, not falsely relabeled as the NSO constructor. The NSO provides the full static structural successor proof; it is never a runtime dependency.

Required seam: **`golden-native-driver:field-resource-model-roles`**, **pending-seam**. Runtime must migrate the whole resource construction/load/readiness/handle and collision/camera provider graph, including translated/native/inlined copies. Recorded NSO offsets are witness layout facts, not guest or presumed PC ABI constants. No renamed ASCII keys, guessed flags, PS2 geometry/stream fallback, active old cosmetic values, runtime NSO reads or runtime acceptance claim.
