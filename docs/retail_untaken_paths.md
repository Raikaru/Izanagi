# Retail paths the corpus runs never took (game-code overrides)

The 31 overrides in `0x00180000..0x003A0000` that never ran in any pcsx2hook run of retail (corpus savestates, mash seeds, scripts, New
Game, two retail saves) although a function that ran calls or registers them (`docs/asm_worklist.md`, "Overrides never executed in retail",
class *reachable*). For each: the retail condition that gates the path, read from the callers' decomp C (`decomp/src`) and the override
headers, and the content that should trigger it. Nothing here was triggered in play yet: a lane that plays forward can confirm a row by
getting the function into `hits.tsv` (`p4hook --hooks`, or `ovdiff -R` once the golden holds a call). Generated list: `retail_reach.py`.

**Confidence.** Gates quoted with a file:line come from the callers' decomp C and are read, not traced. The roles of the plugin callbacks
(read / write / getSize / copy) are inferred from the registration call each one appears in (argument order of `func_003c8d30`,
`func_003e5520`, `func_003c2ba0`) and from the override headers; the "trigger" column is the content that should reach them, unconfirmed.

Most of this address range is RenderWare (RtAnim/HAnim, collision, morph geometry) and the plugin callbacks it registers. A plugin
registers up to three stream callbacks (`func_003c8d30`, `func_003e5520`, `func_003c2ba0`: read, write, getSize) plus constructor, destructor
and copy. **Write and getSize callbacks only run when a stream is written or a chunk is sized: the game reads streams, never writes them, so
those rows are not reachable in play at all** (marked *editor/tool path*).

## Plugin stream callbacks

| Address | Role | Gate in retail | Trigger |
|---|---|---|---|
| 00390290 | getSize of plugin 0x11D (registered at boot by `func_00390520` through `func_003c8d30(285, read 0x390350, write 0x3902D0, getSize 0x390290)`) | called by `RwStreamWrite`-side chunk sizing only | editor/tool path: no trigger in play |
| 0038FB50 | collision-tree writer helper, called by the write callback 0x3902D0 | only from the write callback | editor/tool path |
| 00399320, 00399450 | getSize callbacks: `*(obj + gp slot) == 0 ? 0 : 4` (`code1_0039_cw119.c`) | chunk sizing on stream write | editor/tool path |
| 003985B0, 00398970, 003990A0 | the other callbacks (read / write / copy) registered by `func_003999A0` for the geometry/material plugin | read callback: a geometry or material stream that carries that plugin chunk; copy: `RpGeometryClone`/`RpMaterialClone` of such an object | load a model whose geometry has the plugin chunk (per-geometry extension data), then clone it |
| 0039AE90 | copy callback of plugin 0x105 (geometry animation, `GeomAnimCtor` family, `func_003c2b70(0xC, 0x105, ...)`): duplicates the 20-byte entries between two owners; nothing happens when the source count is 0 | a geometry that has animation entries is cloned | clone a geometry-animated model (UV or morph animated atomic) |
| 0039AF40 | read callback of plugin 0x105 | `RpGeometry` stream read of a chunk of that plugin | load a model with geometry animation (a texture-scroll / morph effect model) |
| 00396A40, 00396E80 | stream callbacks of the HAnim plugin 0x11E (`func_003E5520(0x11E, func_00396C00, func_00396A40, func_00396E80)`) | an `RwFrame` with an HAnim chunk is read from a clump stream | load a skinned character clump (party member, NPC) whose frames carry HAnim hierarchies |
| 003954B0, 003962E0, 003963C0, 00396520 | HAnim keyframe callbacks registered by `func_00396750` (`RtAnimRegisterInterpolationScheme` type 1, 0x24-byte keyframes: to-matrix 3954B0, interpolate, add, blend...) | `RtAnimInterpolatorSetCurrentAnim` with a type-1 animation, then `RtAnimInterpolatorAddAnimTime` / blend | play a type-1 HAnim animation through the generic interpolator: the corpus models use the compressed scheme (`func_00415BC0`), uncompressed type-1 animations appear only in specific assets |

## Hierarchy and model code

| Address | Role | Gate in retail | Trigger |
|---|---|---|---|
| 00397480 | node-matrix update for a hierarchy with flag 2 set; `func_00397C40` returns `func_00397480(h)` when `(flags & 2) != 0` (override header, `code1_0039.c`) | hierarchy flag 2 (the alternative update path, set by the model manager for hierarchies attached to a second owner) | a model with a layered hierarchy attached to a parent model (equipment on a character, a boss with parts) |
| 003971D0 | builds the sub-record for a subtree of a hierarchy | `mdlManager.c:4242`: model layers 1..2 whose `+0x120` word is non-zero, copying the hierarchy of layer 0; `mdlManager.c:4548`: cloning a model whose source layer has `flags & 2` | load or clone a model that has a second animation layer (`mdlLayerIsPresent(m, 1)`) |
| 00395290 | reads a per-slot table (chunk type 1, four counts, 20-byte entries) from a stream into an object | `sdkWrap.c:189`, case 5 of the task-state switch on `*(work + ...)`: the wrapper's command 5 ("read this slot table from the stream") | the load task that issues command 5: a stream with that table (per-slot animation/collision table), i.e. a specific asset type |
| 00394FD0, 00395100 | object builders called only by 0x395290 | as above | as above |

## Collision (field world, shadows)

| Address | Role | Gate in retail | Trigger |
|---|---|---|---|
| 00394D70 | world hit test dispatched on the shape type at `arg1 + 0x18`: type 1 → 0x390680, 4 → 0x391400, 3 → 0x391C10, 5 → sphere of `func_003BFAE0` into 0x391C10 | `k_fldFrame.c:364`: `FldFrame` sphere query returns before the call when `collisionWorld == NULL`; `k_spipe_inferred_00179fc0.c:255`: the shadow projection calls it only in the branch that has a collision world `v` (a world atomic's sector tree) | a field whose resource carries a collision world and a moving sphere (the player, an NPC): walking in such a field; shadows projected onto a world |
| 00390680, 00391400, 00391C10 | the per-shape tests (segment, ...) over the two-level triangle tree | the shape type of the query | as above, per shape type (a segment, a sphere, a box) |
| 003941E0 (`MorphGeomForAllTriIntsecLine`), 00394870 (`MorphGeomForAllTriIntsecSphere`) | the same tests for a geometry with several morph targets | `func_00394C70`/`func_00394E70`: `*(geometry + 0x18) >= 2` (morph target count); with fewer the plain `func_00392BF0`/`func_003910..` runs | a hit test against an atomic whose geometry has two or more morph targets (an animated morph model used as a collider) |

## Textures and tables

| Address | Role | Gate in retail | Trigger |
|---|---|---|---|
| 00399DB0 | replaces the texture bound to the type-1 slot of a collision/material record | called from `func_00398D20` (ran): the bump-map branch, only when the material's first texture and the slot's texture are both non-null and the cached object `func_003EF650` is looked up | a bump-mapped material (the same path as 00399470) |
| 00399470 (`GenBumpedTextureName`), 00399530 | builds the combined bump texture name; 00399530 is reached through `func_0039AB10` | a material with a bump-map texture reference | a model with bump-mapped materials loaded (and `func_0039AB10`'s callers) |
| 0039B830 | rebuilds a table at `D_00884ACC` | `code1_0039.c:2263`: the `init` label of the table's accessor, taken when the table is not built yet | the first use of that table (an asset type that needs it) |
| 001F5680 | `func_001F5680`: leaf function that reads the object pointer at retail `0x0076449C` and returns 0x11 or -1 from its u16 at +2 | named in function-pointer tables of `func_001F2CC0` and `func_001F56D0`, which ran: the slot is read only for the index that selects it | the game-code state whose table index selects this slot (not identified: the tables are in `.data` and the consumer `func_001F2CC0` takes the index from a state word) |
