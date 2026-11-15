# Models: Persona 4's RenderWare data and the glTF pipeline

Persona 4 stores every 3D model (party members, weapons, enemies, Personas,
NPCs, field props and rooms) as RenderWare 3.7 streams inside Atlus RMD files.
This page documents that format from the decomp and the disc, the P4 glTF
convention, the exporter `tools/models/rw_to_gltf.py` (a model from your own
disc to glTF 2.0 for Blender), the runtime loader that lets a mod ship glTF
in place of a disc model, the Blender workflow and the limits.

Converted files come from your disc. Keep them for local editing; they never
belong in this repository or in a published mod. Mods ship their own authored
or synthetic models.

Decomp citations name the source file (under `decomp/`) and the retail
function address (`FUN_xxxxxxxx`, or the `func_xxxxxxxx` symbol).

## Where models are on the disc

The VFS (`src/platform/files/vfs.c`) indexes the disc's CVM containers. Each
CVM is an ISO 9660 volume at offset 0x1800 behind a `CVMH` header.
`tools/models/p4disc.py` reads them the same way. Disc keys are
`<container>/<path>`, such as `DATA/MODEL/WEAPON/WP001.RMD`.

| Directory | Contents | Files |
|---|---|---|
| `DATA/MODEL/WEAPON/WPxxx.RMD` | weapons, one per weapon item id; Yosuke's are `WPxxx_R.RMD` / `WPxxx_L.RMD` | 331 |
| `DATA/MODEL/PACK/BCxxx[_C1,_C9].PAC` | party members (`bcNNN.RMD` + `bcNNN.acx`) and their costume variants | 15 |
| `DATA/MODEL/PACK/WP070..WP095[_C1].PAC` | Chie's whole-body models, chosen by her equipped item | 76 |
| `BTL/MODEL_BTL/PACK/EMxxx.PAC` | enemies (`emNNN.RMD` + `emNNN.se`) | 336 |
| `BTL/MODEL_BTL/PERSONA/PSxxx.RMD` | Personas in battle | 256 |
| `DATA/MODEL/NPC`, `NPC2` | NPCs (`Nnnn_v.RMD`) | 1,537 |
| `DATA/FIELD/RMD`, `DATA/FIELD/GRMD`, `DATA/MODEL/FACILITYP` | field props and facility models | 639 |

PAC files are flat packs. Each entry is a 252-byte NUL-padded name, a
little-endian u32 size and the payload, padded to the next 64-byte boundary.
Field `.P00`/`.P01` packs use the same layout with 128-byte alignment.

The battle builds model paths in `btlBossResolveModelName`
(`src/Battle/btlBoss_inferred_002305c0.c`, FUN_002308A0) from format strings
in the ELF: `%splayer/bc%03X.RMD` (0x006356B0), `_c1` (0x006356D0) and `_c9`
(0x00635690) variants picked by calendar window and flag 0x1438,
`%sweapon/wp%03X.RMD` (0x006356F0) for model type 7 and 10, and
`%senemy/em%03X.RMD` (0x00635730) under `model_btl/`. For ids that are packs,
`mdlResolvePacPath` (`src/Graphics/Model/mdlManager_inferred_00230c00.c`,
FUN_00230C00) rewrites the extension to `pac` under `model/pack/`
(`%spack%s`, 0x006357C8). For Chie (unit 3), the equipped item 0x70..0x95
selects `wp%03X` as her whole body model.

## RMD container

An RMD is a sequence of top-level RenderWare chunks. Each chunk is a u32 type,
a u32 payload size and a u32 library id. `mdlReadAuxChunks`
(`src/Graphics/Model/mdlManager.c:6285`, FUN_0047B0C0) reads them in order
until it reaches a type-0 chunk or the end of the stream. RenderWare chunks
carry library id 0x1C020037 (RW 3.7.0.2, build 0x37). Atlus chunks
(0xF0F000xx) carry 0x40000000.

| Chunk | Loader behaviour (FUN_0047B0C0) |
|---|---|
| 0xF0F000F0 | u16 animation slot capacity of the current layer |
| 0x16 texture dictionary | read (func_003E6A90); its texture names join the model's texture list |
| 0x23 PI texture dictionary | read (func_003DC370), same |
| 0x10 clump | the model's clump (func_003C0F20: clump version must be 0x35000..0x37002) |
| 0x1B animation | RtAnim animation (func_003D53C0) stored in the current slot |
| 0x2B UV animation dictionary | before the clump: the model's base dictionary; after it: the current slot's (func_003D6350) |
| 0xF0F00006 | attach table: u16 count, then per entry s32 attach id, s32 HAnim node id, 64-byte RwMatrix |
| 0xF0F00005 | 64-byte RwMatrix for the current slot |
| 0xF0F00004 | next slot |
| 0xF0F00002 | next layer (slot 0) |
| 0xF0F00003 | u16 source slot: the current slot copies its animation, matrix, material and UV animations and effects |
| 0xF0F00001 | the current slot gets the shared placeholder animation (0x00922BC0) |
| 0xF0F00080..83 | material animation channel 0..3 for the current slot (`mdlMatAnimLoadChunk`, `src/Graphics/Model/mdlMatAnim.c:376`, FUN_004800D0) |
| 0xF0F000E0 | effect block: a 20-byte header (u16 first, u16 last, u32 length, u32 skip, flags, reserved), then `skip` bytes and the `length`-byte payload. The loader never reads the chunk size, and retail writes a size without `skip`. |
| 0xF0F000E1 | effect copy for the current slot |
| 0xF0F000D0 | blend group: allocates a 0x4C-byte blend record (weight and progress 1.0), copies the chunk payload into it at +0x3C, then reads the four 0x1B animation chunks that follow as its animations |
| 0xF0F00007 / 08 | start frame A / B of the current slot |
| 0xF0F00009 | the next animation becomes the layer's base animation |
| 0xF0F00070 | 0x2B8-byte slot property record (`mdlBuildScaledSlotRecords`) |

Retail order is: slot capacity, texture dictionary, clump, attach table, then
one group per slot (slot matrix, animation, material and UV animation chunks,
next slot). Slots that reuse another slot's animation hold only an alias.

`rw_to_gltf.py --check "*.RMD*" "*.PAC#*" "*.P0[01]#*"` converts all 6,326
RMDs on the disc: the loose files, including the `.RMD_` variants, and the
RMD members of every PAC and field pack. Two of them each hold one animation
of another registered RtAnim type (not HAnim keyframes). That animation is
kept raw in the sidecar.

## RenderWare 3.7 objects used by P4

### Clump (0x10)

The clump holds a struct with the atomic, light and camera counts (P4 models
have no lights or cameras), a frame list (0x0E), a geometry list (0x1A), one
atomic (0x14) per mesh, and an extension.

- **Frames.** Each frame is a 3x3 rotation (right, up and at rows) plus a
  position, a parent index and flags. RenderWare multiplies row vectors
  (`v * M`), so a frame's world matrix is its local matrix times its parent's.
  Each frame has an extension that can hold the HAnim plugin (0x11E) and user
  data (0x11F).
- **HAnim (0x11E).** Version 0x100 and the frame's node id. The hierarchy
  root also stores the node count, flags, keyframe size (36) and one
  (id, index, flags) row per node in depth-first order, where flag 1 is pop
  and flag 2 is push. `func_00397470` reads a frame's node id
  (`src/promoted/code1_0039.c:574`). The skin's bone *i* is HAnim node *i*.
- **Atomics.** Each atomic is a frame index, a geometry index, flags (5) and
  an extension with right-to-render (0x1F: the skin or MatFX pipeline id and
  data) and the MatFX atomic flag (0x120).

### Geometry (0x0F)

- The geometry struct is a format word, the triangle, vertex and morph target
  counts, then the vertex data. Format bits: 0x1 tristrip, 0x2 positions,
  0x4 / 0x80 one or two UV sets, 0x8 prelit colours, 0x10 normals,
  0x20 light, 0x40 modulate material colour; bits 16-23 give the UV set
  count. Retail formats are 0x10037, 0x10077, 0x1007F, 0x1003F, 0x7B, 0x73
  and 0x200FB.
- No P4 model geometry is pre-instanced: the 0x01000000 native flag is clear
  in all 17,337 geometry structs (`decomp/docs/port/renderer.md` §12.7).
  Geometry is stored generically and instanced at load.
- Vertex data comes in this order: prelit RGBA8, UV sets, then triangles as
  two u32 words, `v0 << 16 | v1` and `v2 << 16 | material`. One morph target
  follows: a bounding sphere, has-positions and has-normals flags, positions,
  normals. Front faces are counter-clockwise in `v0, v1, v2` order. On the
  protagonist's body, 1,870 of 1,881 triangle normals agree with the vertex
  normals.
- **Material list (0x08).** A count and one index per slot (-1 for a new
  material), then the material chunks (0x07). Each material holds flags, an
  RGBA8 colour, a textured flag and surface properties (ambient, specular,
  diffuse). It can carry a texture (0x06: filter and addressing word, name,
  mask, sky mipmap extension 0x110) and plugins: user data (0x11F), MatFX
  (0x120), UV animation (0x135: a struct with a slot mask and one 32-byte
  animation name per set bit), and right-to-render.
- **Geometry plugins.** Bin mesh (0x50E) holds one triangle strip per
  material (flags 1 on every retail geometry). Skin (0x116) and collision
  (0x11D) appear on some field props. User data holds `FVF.UserData`.
- **Skin (0x116).** The bone count, used-bone count, maximum weights per
  vertex (1-4) and a pad. Then the used-bone list, four u8 bone indices and
  four float weights per vertex, and one inverse bone matrix per bone (16
  floats; the fourth column holds uninitialised exporter memory). The split
  data (bone limit, mesh count, RLE count and their tables) comes last. The
  inverse matrices map the vertices' bind space to each bone. In character
  models that space is not the frame hierarchy's rest pose. For the
  protagonist, `inverse x boneLTM` is one constant matrix for every bone: X
  and Y swapped and Z negated, which turns the vertex data X-up, plus a
  translation of (0, 89.05, -3.85). The port skins on the CPU
  (`docs/renderer_port.md`, skinned models).
- **MatFX (0x120).** The material effect, then two passes: environment map
  (coefficient, frame-buffer alpha flag, optional texture), UV transform,
  dual, or bump map. Retail uses environment maps, UV transform and dual UV
  transform. `mdlMatFXSetEnvMapStateCallback`
  (`src/Graphics/Model/mdlManager.c`, FUN_00477430) sets the environment-map
  state at run time.

### Texture dictionaries (0x16) and PS2 textures (0x15)

The dictionary struct holds a u16 texture count and a u16 device id (always
6). Each native texture contains:

- a struct with the platform id `PS2\0` and the filter and addressing word
  (filter in bits 0-7, U addressing in bits 8-11, V in bits 12-15);
- name and mask strings;
- a struct holding two struct chunks: a 64-byte raster header and the data;
- the sky mipmap extension (0x110, one word).

Raster header word 0 is the width, word 1 the height and word 2 the depth.
Word 3 holds the RwRasterCreate flags in its low 16 bits (0x2504
PAL8|8888|texture, 0x4504 PAL4, 0x0504 32-bit, 0x8000 mipmaps) and the data
kind in its high 16 bits. The remaining words are the GS TEX0/TEX1/MIPTBP
registers and sizes.

Every model texture is kind 2: the data is the GIF stream that the sky
texture cache replays. PSMT8 and PSMT4 pixels are uploaded through PSMCT32
and PSMCT16 (pre-swizzled), and the CLUT is the last upload (CSM1, with bits
3 and 4 of the 8-bit index swapped). PS2 alpha runs from 0 to 0x80. The port
decodes this in `readTEXFromStream` (`src/render/p4gl_native.c`, retail
0x003FD940) and `src/render/p4gl_nativetex.c`, whose header gives the
formulas. `tools/models/ps2tex.py` is the same decoder in Python: on
DOBJ044.RMD it reproduces the index CRCs that `tests/native_tex_test.c`
expects from the C decoder. Retail model textures are 8-bit (most) or 4-bit,
with a few 32-bit ones on Personas and NPCs. They are at most 128 pixels for
party members and weapons and at most 512 for field props.

### Animations (0x1B)

RtAnim animations of HAnim keyframes. The header is version 0x100, scheme id,
keyframe count, flags and duration. Times are in seconds (retail keys fall on
1/30-second steps).

- **Scheme 1** (36-byte RpHAnimStdKeyFrame): time, quaternion (x, y, z, w),
  translation, previous keyframe.
- **Scheme 2**, compressed, used by all but one retail animation (24 bytes in
  memory): time, the quaternion and translation as 16-bit floats (sign bit,
  4-bit exponent for 2^-15..2^0, 11-bit mantissa), previous keyframe; then
  the translation offset and scale (6 floats), so that `t = offset + scale *
  packed`.
- "Previous" is stored as a byte offset: index times the in-memory keyframe
  size. The first two keyframes of every node come first in node order. Each
  later keyframe continues the node of the keyframe it names.
- Quaternions use the standard convention: a RenderWare rotation's rows are
  the columns of the quaternion's column-vector matrix. On the protagonist's
  24 animations, 861 of 2,232 first keyframes match their bone's frame
  exactly under this convention, against 572 under the conjugate. The first
  translations sit within 0.006 of the frame positions.
- `func_003D53C0` (`overrides/003d53c0.c`) allocates the object as 0x18
  bytes, plus the scheme's custom data, plus count x keyframe size.

### P4 user data conventions

| Name | On | Meaning (consumer) |
|---|---|---|
| `name` (string) | materials, geometries | the artist's name (exported as the glTF material name) |
| `per3MatAlpha` (int) | materials | draw category 0-9. An atomic takes its materials' highest-ranked category (rank table 0x00712530: 0, 1, 7, 8, 5, 6, 9, 2, 4, 3) and goes into that bucket (`sdkClumpCollectMaterialFlags`, `src/sdkClump.c:73`, FUN_00462960; `sdkClumpAddAtomicCategory`, FUN_00462780) |
| `per3FogDisable` (any) | materials | the atomic is drawn without fog (same function) |
| `per3MatSPambient` / `diffuse` / `specular` (float) | materials | the material's lighting vector (`mdlSetAtomicMaterialVector`, FUN_00477510) |
| `per3modelMatColor` (int, packed RGB plus a flag byte) | materials | the tint that model colour changes scale (`mdlMatColorScaleCallback`, FUN_00476E90) |
| `per4MipmapTex` | materials | field textures swapped for the mip texture (`FldRcMaterialSetMipmapTex`, FUN_0018C610) |
| `per4DungeonDoor` | materials | marks dungeon door clumps (`src/Graphics/Model/mdlManager_inferred_001560a0.c:2892`) |
| `per3HelperId` (int) | frames | attach point id, used when a model has no attach table (below) |
| `FVF.UserData` (int) | geometries | exporter vertex-format tag |

## Weapon attachment

A battle character binds its weapon as a child model in one of its five
attached-model slots (12 bytes each from model + 0x28C).

1. `mdlAttachWeaponModelSlots`
   (`src/Graphics/Model/mdlManager_inferred_00230c00.c:61`, FUN_00230D30)
   runs for type-1 (party) models. The weapon id is the unit's equipped item
   (`datGetEquippedItemByUnit(unit, 0)`), and the weapon is model type 7 with
   that id: `model/weapon/wpXXX.RMD`.
   - Yosuke (unit 2) loads `wp%03X_R.RMD` (0x006357E0) into slot 0 and
     `wp%03X_L.RMD` (0x00635800) into slot 1, with id + 0x3E8.
   - Chie (unit 3) attaches nothing, because the weapon is part of her body
     model.
   - Everyone else gets slot 0 through `mdlAttachSlotModelFindOrCreate`
     (`src/Graphics/Model/mdlManager.c:6121`, FUN_0047AC90), or
     `mdlAttachSlotModelFromResource` (FUN_0047AAA0) from a path.
2. `mdlSetAttachedSlotValue` (FUN_0047ADF0) stores the attach id: 500
   (0x1F4) for slot 0, the right hand, and 501 (0x1F5) for slot 1, the left
   hand.
3. Every update (`src/Graphics/Model/mdlManager.c:3595-3609`), each attached
   model whose attach id is not -1 gets its matrix from
   `mdlCopyIndexedMatrix(owner, id, mdlGetMatrix(child))`
   (`src/Graphics/Model/mdlManager.c:5854`, FUN_0047A510).
   - With an attach table (chunk 0xF0F00006), `mdlMatrixFromTable` finds the
     entry with that id and the frame whose HAnim node id matches the entry's
     node id (`mdlFindFrameById`, FUN_00475B10). The weapon's matrix is then
     `entry.matrix x frameLTM`.
   - Without a table, the frame whose `per3HelperId` user data equals the id
     (`K_Clump_FindIntUsrDataFrame`, `src/Kosaka/k_clump/k_clump.c:254`,
     FUN_00457F40; the name string is at 0x007131D8) gives its LTM directly.
   - The protagonist's table maps 500 to HAnim node 1021, his right hand,
     with an offset matrix. It also has entries 0, 1, 2 and 1000 that other
     attachments use.
4. The weapon model's root frame therefore sits on the grip. Retail weapons
   have a root frame (the offset from the grip), an HAnim hierarchy root
   frame (node id 0) that holds the atomic, and their own 27-28 animation
   slots aligned with the character's battle animation slots. Most slots
   alias slot 0, which has a two-keyframe static pose. A weapon has its own
   attach table (ids 100/101 for trails and effects, or 0/1/2/500/1000 for
   Chie's body).

## Retail budgets

Measured on the disc with `tools/models/rwmodel.py` (median / 95th
percentile / maximum per model).

| Category | Triangles | Bones | Weights per vertex | Textures | Largest texture | Anim slots |
|---|---|---|---|---|---|---|
| Weapons (331) | 106 / 2,893 / 3,067 | 2 / 73 / 73 | 0-3 | 1 / 10 / 10 | 128 | 27-28 |
| Party members (15) | 1,939 / 2,707 / 2,707 | 76 / 99 / 99 | 3 | 10 | 128 | 28 |
| Enemies (336) | 2,542 / 3,884 / 8,429 | 54 / 130 / 241 | 2-4 | 3 / 4 / 10 | 256 | 19-20 |
| Personas (256) | 2,476 / 3,676 / 5,000 | 83 / 116 / 214 | 2-4 | 3 / 16 / 18 | 256 | 7-9 |
| NPCs (sample of 180) | 1,899 / 2,950 / 3,843 | 85 / 94 / 106 | 3 | 6 / 8 / 10 | 256 | 11-32 |
| Field props (sample of 125) | 196 / 1,716 / 3,968 | 4 / 30 / 73 | 0-2 | 1 / 5 / 17 | 512 | 1-5 |

The upper weapon figures are Chie's whole-body models.

## The P4 glTF convention (version 1)

`rw_to_gltf.py` writes, and model tooling reads, glTF 2.0 as follows.

- **Units and axes:** 1 glTF unit = 1 P4 unit (about a centimetre), Y up,
  right-handed, the same as P4. No scale node is added.
- **Frames → nodes:**
  - A frame's node carries translation, rotation and scale. The exact
    RenderWare rows and position are kept in `extras.p4.matrix`, and are
    authoritative while the node's TRS still reproduces them.
  - Bones are named `bone_<HAnim id>`, attach helpers `helper_<id>`, other
    frames `frame_<n>`.
  - `extras.p4` also holds `hanimId`, `hanimIndex`, `hanimFlags`,
    `helperId`, the user data, and the hierarchy's node table on the root.
  - A frame matrix sheared beyond 0.05 becomes a `matrix`, except on a bone.
    A bone's animation replaces its matrix, so the bone gets an
    orthonormalised TRS and a warning.
- **Atomics → meshes:**
  - A static atomic's mesh sits on its frame's node. A skinned atomic gets a
    scene-root node, since glTF ignores a skinned mesh's transform, and
    `extras.p4.atomic.frame` keeps its RenderWare frame.
  - There is one primitive per material, built from the RenderWare triangle
    list in order, with degenerate triangles dropped.
  - Attributes: `POSITION`, unit `NORMAL`, `TEXCOORD_0/1`, `COLOR_0` (prelit
    colours, normalised u8), and `JOINTS_0` / `WEIGHTS_0` (u8 bone indices
    and float weights, with zero-weight joints set to 0).
- **Skins:**
  - Joints are the HAnim nodes in HAnim order, so a glTF joint index is the
    RpSkin bone index.
  - `inverseBindMatrices` are the RpSkin matrices with the last row set to
    (0, 0, 0, 1). The original fourth-column words are in
    `extras.p4.matrixPads`.
- **Animations:**
  - One glTF animation per animation chunk, named `L<layer>S<slot>` after the
    first slot that uses it. Each bone gets a LINEAR rotation channel and a
    LINEAR translation channel.
  - `extras.p4` records the scheme, duration, flags and compressed offset and
    scale.
  - Compressed rotation samplers also retain their original packed values and
    snapshots of the exported times and normalized rotations in `extras.p4`.
    They preserve an unchanged key exactly; edited keys use their new values.
    Tools that discard these extras still produce valid animations.
  - The slot layout (aliases, placeholders, blend groups, per-slot matrices
    and chunks) is in the sidecar.
- **Materials** (so models look right without guessing):
  - `baseColorTexture` is the material's texture as a PNG with straight
    alpha: PS2 alpha is doubled, and 0x80 or more reads 255.
  - `baseColorFactor` is the material colour divided by 255. Metallic is 0
    and roughness is 1.
  - Geometry without the light flag (0x20) or without normals is prelit:
    those materials carry `KHR_materials_unlit`, and the prelit colours are
    `COLOR_0`, which multiplies the texture and factor (P4 draws them as
    vertex colour x material colour x texture).
  - Lit geometry keeps the core material and takes the game's lights.
  - `alphaMode` is BLEND when the texture has partial alpha or the material
    alpha is below 255, MASK (cutoff 0.5) when the texture alpha is only 0
    and 255, and OPAQUE otherwise.
  - Samplers map the RenderWare filter (nearest, linear, mip variants) and
    addressing (wrap, mirror, clamp).
- **P4 material state in `extras.p4`:**
  - `drawCategory` (`per3MatAlpha`), `fogDisable`, `tint`
    (`per3modelMatColor` as hex), `surfaceProperties`, `color` (raw bytes),
    `lighting` (`lit` or `prelit`) and the full `userData`.
  - `matfx`: the effect and its passes. An environment map pass gives its
    coefficient, frame-buffer alpha flag and texture (name, filter, glTF
    texture index).
  - `uvAnimations`: slot and name.
  - `plugins`: any plugin the converter does not interpret, in base64.
- **Sidecar `<name>.p4model.json`** (format `p4model-sidecar`, version 1)
  holds what glTF has no place for:
  - the source name, size and SHA-256;
  - the RenderWare library id;
  - every top-level chunk in order (the clump, dictionary and animations as
    references to the glTF, the Atlus chunks in base64);
  - the slot layout with glTF animation indices;
  - the attach table;
  - every texture's PS2 format, raster flags and header words;
  - converter warnings.

## Exporting from your disc

```sh
python tools/models/rw_to_gltf.py --list DATA/MODEL/WEAPON          # list disc keys
python tools/models/rw_to_gltf.py --out out DATA/MODEL/WEAPON/WP001.RMD
python tools/models/rw_to_gltf.py --out out "DATA/MODEL/PACK/BC001.PAC#bc001.RMD"
python tools/models/rw_to_gltf.py --out out --glb "BTL/MODEL_BTL/PACK/EM0*.PAC#*"
python tools/models/rw_to_gltf.py --check "*.RMD" "*.PAC#*"           # convert everything in memory
```

The disc image is specified by `P4PC_ISO` (`--iso` overrides). A spec is a
disc key or glob, a pack member (`key#member`, or
`key#*` for every RMD in a pack), or a local RMD or pack file. Each model goes
to `out/<name>/`: `<name>.gltf`, `.bin` and one PNG per texture (or one
`.glb`), plus the sidecar. Pack members are named `<pack>-<member>`.

In Blender, use File > Import > glTF 2.0. The model is about 100 times larger
than a human in metres. Set Scene > Units > Unit Scale to 0.01 so that
measurements read in metres, and raise the viewport clip end. Skinned models
import with their armature and one action per animation.

`tests/models_export_test.py` covers the texture codecs and the converter on
synthetic RMDs that `tests/models_fixture.py` builds. With the disc present,
it also covers a sample of weapons, party members, an enemy, a Persona, an NPC
and field props. The Khronos glTF validator (2.0.0-dev.3.10) reports no
errors or warnings on a 194-model sample: weapons, party members, Chie's
packs, enemies, Personas, NPCs, field props, facility and symbol models.
Blender 5.2 imports the protagonist, his golf club, an enemy and two field
models from it with textures, armatures and actions. The protagonist renders
posed by his first battle animation.

## Runtime glTF models

Mods ship glTF; the port loads it directly. A `[resource-model]` declaration
([files.md](files.md#authored-model-resources)) names a `.glb` or `.gltf`
and the disc model it replaces. No modder touches a RenderWare file.

**Boot.** Early CPU preparation (`src/platform/mods/mod_models_boot.c`)
resolves `replaces` on the disc and reads the glTF and every file it names
through issued package file plans. It then cooks the glTF into a compact
internal model (`src/platform/models/p4model_cook.c`, format in
`p4model.h`):

- Static meshes are baked into the space of the frame they attach to, as
  triangle lists with positions, unit normals, UV sets and RGBA8 vertex
  colours. Skinned meshes (`schema=2`) stay in skin space, with up to four
  joint weights per vertex and one inverse bind matrix per joint; their
  joints are named by HAnim node id (`bone_<id>`).
- Each glTF animation named `L<layer>S<slot>` (the names `rw_to_gltf`
  writes) becomes a slot animation: one track per `bone_<id>` node, its
  rotation and translation channels sampled at the union of their key times
  (LINEAR, spherical for rotations, or STEP) from 0 to the animation's end, a
  bone without channels held at its node's rest pose. Values are packed as
  RtCompressedKeyFrames, the scheme retail animations use: 16-bit floats,
  translations relative to a per-animation offset and scale (the stored
  `extras.p4` range when it still covers them). They apply from `schema=3`.
- Each PNG texture becomes an exact 256-entry palette (PAL8) when it has at
  most 256 colours, as nearly all retail textures are, else straight 32-bit
  RGBA. The cooker never converts lossily.
- Materials carry their RenderWare colour, surface properties, texture and P4
  user data.

**Cache.** The cooked model is cached under
`<settings directory>/model-cache/<SHA-256>.p4mc`. The hash covers the
cooker version and every input byte; `P4PC_MODEL_CACHE` names another
directory, or `none`. Later starts load the cache instead of parsing JSON
and decoding PNGs, which keeps phone start times low. A cache file is
trusted only after its structure, a body CRC-32 and the hash all check;
anything else is a miss and the glTF is cooked again. The runtime phase
freezes the override table keyed by disc key.

**Loading.** `mdlReadAuxChunks` (`overrides/0047b0c0.c`, FUN_0047B0C0) asks
the table for the file of each model it reads (the HCdvd path, plus
`#<member>` for a pack entry). It does so only when the table is nonempty.
For an overridden model:

1. The game still reads the replaced RMD. The texture dictionary and clump
   are read inline, as with model flag 0x4000.
2. Right after the clump is read, `p4_model_override_apply`
   (`src/platform/models/p4model_build.c`, the one RenderWare adapter) runs before
   any later chunk binds to the clump's materials:
   - It creates the rasters and textures (`RwRasterCreate`,
     `RwTextureCreate`), the materials (`RpMaterialCreate`,
     `RpMaterialSetTexture`, user data) and the geometries
     (`RpGeometryCreate`, triangles and materials, bounding sphere,
     `RpGeometryUnlock`).
   - A skinned mesh is built the way retail skinned geometry is:
     `RpGeometryStreamRead` reads a geometry chunk in retail RMD form
     (`src/platform/models/p4model_stream.c`) from an
     `RwStreamOpen(rwSTREAMMEMORY)` stream, and the skin plugin's reader
     builds the `RpSkin`. RenderWare 3.7 keeps no `RpSkinCreate` in P4's
     executable. The joints map onto the replaced model's HAnim hierarchy by
     node id, so skin bone indices are hierarchy node indices. Inverse bind
     matrices become the skin-to-bone matrices. The skin is not split (the
     split data drives PS2 VU1 matrix batching; the port skins on the CPU).
   - It removes and destroys the clump's atomics.
   - It adds one atomic per cooked mesh on its frame. A skinned atomic
     gets `RpSkinAtomicSetType` (generic) and the clump's hierarchy
     (`RpSkinAtomicSetHAnimHierarchy`). The game attaches the hierarchy to
     every atomic again when it sets the model up.
3. The rest of the file then loads as usual: animation slots and aliases,
   the attach table, effects, UV animations and material animations (these
   find the new materials by their `name`). For a `schema=3` override, an
   animation chunk bound for a slot the glTF has an `L<layer>S<slot>`
   animation for is skipped, and `p4_model_override_animation` builds that
   slot's animation the way retail ones are built: `RtAnimAnimationStreamRead`
   reads an RtAnimAnimation chunk in retail RMD form
   (`p4model_stream.c`) from a memory stream. Its keyframes follow RtAnim's
   order: every node's first key, every node's second key, then each later
   key by the time of its node's previous key, ties by node index, with
   previous-keyframe offsets as retail stores them. Tracks map onto the
   replaced model's hierarchy by HAnim id. A layer's base animation and
   blend groups stay the file's.

The model therefore keeps the replaced model's skeleton and gameplay data.
For a weapon, that means its grip, its 27-28 animation slots and its
effect anchors. When a mesh's frame or a weighted bone does not exist in
the replaced model, the model keeps its retail look and the log names the
missing frame or bone. When an animation has no track for a node of the
replaced model's hierarchy, that slot keeps the file's animation and the log
names the node.

**Placing meshes.** A static mesh attaches to the nearest ancestor-or-self
node named `bone_<HAnim id>`, `helper_<per3HelperId>` or `frame_0`. Without
one, it attaches to the clump's root frame, and its vertices are taken in
scene space. The frames themselves come from the replaced model: moving a
bone node in Blender has no effect; moving a mesh relative to its bone does.
A skinned mesh hangs from the frame of the replaced model's first skinned
atomic. Its node's transform is ignored, as glTF specifies, and its
vertices follow its joints.

**Materials** follow the P4 glTF convention above:

- `baseColorFactor` and `baseColorTexture` (TEXCOORD_0) set the material
  colour and texture. The sampler's magnification filter and wrap modes
  become the RenderWare filter and addressing.
- `KHR_materials_unlit` on all of a mesh's materials, or a mesh without
  normals, gives a prelit geometry (no light flag). `COLOR_0` is its vertex
  colour.
- The material name becomes the `name` user data.
- `extras.p4.drawCategory` (0-9), `fogDisable`, `tint` (hex) and
  `surfaceProperties` become `per3MatAlpha`, `per3FogDisable`,
  `per3modelMatColor` and the surface properties.

**Budgets.** These come from the replaced model's category, with headroom
above the retail figures in "Retail budgets":

| Target | Triangles | Meshes | Vertices per mesh | Textures | Largest texture | Texture bytes | glTF bytes |
|---|---|---|---|---|---|---|---|
| Weapon (`DATA/MODEL/WEAPON/`) | 4,096 | 8 | 65,535 | 8 | 256 | 512 KiB | 16 MiB |
| Character (party, enemy, Persona, NPC, symbol) | 16,384 | 64 | 65,535 | 32 | 512 | 2 MiB | 64 MiB |
| Field and other | 32,768 | 256 | 65,535 | 64 | 512 | 4 MiB | 64 MiB |

Textures must be PNG, with power-of-two sizes from 8 up to the largest
size. Texture bytes count palettes (1 KiB each) and pixels as P4 stores
them.

**Development seam.** `P4PC_VIEWER_MODEL=<model.glb> p4pc_viewer SLUS_217.82
model/weapon/wp004.RMD out.png` cooks the glTF with the target's budget and
draws the overridden model. `P4PC_VIEWER_DUMP=<file.json>` writes the loaded
clump's in-memory atomics (frame, geometry flags, vertices, normals, UVs,
prelit colours, triangles, materials; for a skinned atomic the `RpSkin`, with
its used bones, bone indices, weights and skin-to-bone matrices, and the
atomic's hierarchy node ids) for comparing an override with the retail
model. The seam applies slot animations, as `schema=3` does; the viewer
draws the model after one animation step.

**Tests.**

- `tests/models_roundtrip_test.py` cooks synthetic RMDs, and with the disc
  exported retail weapons and field props, through `model_cook_cli` (the
  boot handler's cooker). It compares the result with the RMD it came from:
  positions exact in frame space, unit normals within 1e-6, UVs, prelit
  colours and triangles exact, and textures and material colours and names
  exact.
- For skinned meshes it re-exports a synthetic skinned RMD and, with the
  disc, a Persona, a facility person, a multi-atomic NPC and a Shadow. It
  writes the geometry chunk the builder reads (`model_cook_cli --stream`)
  and checks the skin extension against the RMD's own, byte for byte:
  bone count, used bones in retail order, bone indices, weights, and
  skin-to-bone matrices with their pads. Only the split data differs, and
  the bone index of a zero weight, which nothing reads.
- For animations it streams every slot animation of those models and of the
  sampled weapons (`model_cook_cli --anim-stream`) and checks it against the
  RMD's: the same keyframes in the same order with the same previous offsets,
  times and translations, the same duration, offset and scale, and identical
  packed rotations for unchanged compressed keys. An edited-key test checks
  that retained snapshots do not override edits. A round-toward-zero test
  checks the decimal range bits and restoration of the caller's FP mode. An
  authored animation checks sampling, STEP keys, rest poses and keyframe order.
- The same test checks the refusals (malformed, oversize texture and
  triangle count, non-power-of-two, JPEG, a weighted joint that is not a
  bone, weights without a skin, missing image, CUBICSPLINE or scaling
  animations, two animations for one slot) and the content-hash cache,
  including a damaged cache file.

## Blender workflow

1. **Start from the model you replace:**
   `python tools/models/rw_to_gltf.py --glb --out work DATA/MODEL/WEAPON/WP004.RMD`.
   This gives the frame names (`frame_0`, `bone_0`) and the retail mesh as a
   reference. A model made from scratch needs only a mesh on a node named
   after the bone it belongs to; `tools/models/make_test_weapon.py` writes
   one.
2. **Import** with File > Import > glTF 2.0. P4 units are about a
   centimetre: set Scene > Units > Unit Scale to 0.01 and raise the viewport
   clip end.
3. **Edit.** Recolour the PNG, or remodel the mesh, keeping it parented to
   its `bone_<n>` empty or bone. A weapon's grip is the origin of `bone_0`,
   and the retail blade or head extends along +Y. Keep materials simple: one
   base colour texture, an optional colour factor and vertex colours.
   Custom properties under `p4` set the P4 material state. A skinned mesh
   (a character's body) keeps the imported armature: weight-paint it to
   the existing `bone_<id>` bones, at most four per vertex. New bones are
   not used. Export it in its rest pose.
   To change an animation, edit the action named after its slot
   (`L0S03`) on the imported armature: rotate and move the existing bones,
   without scaling them. Delete the actions you do not change; the ones you
   keep replace their slots.
4. **Export** with File > Export > glTF 2.0, as a `.glb`:
   - Include: Custom Properties (keeps `extras.p4`).
   - Transform: +Y Up.
   - Data: Mesh with UVs and Normals, Vertex Colors only when used,
     Materials as Export, Images as PNG. For a skinned mesh, also Skinning,
     with at most 4 bone influences.
   - No Draco or meshopt compression, since the game refuses required
     extensions it cannot use.
   - Animation: only for changed animations, exported as Actions under
     their names. Keep Always Sample Animations on: Blender then writes
     linear keys, while unsampled Bezier keys become CUBICSPLINE, which is
     refused. Without animations the replaced model's own play.
5. **Make the mod:** a folder with `mod.ini`, the `.glb`, and a
   `[resource-model]` section with `schema=1` for static meshes, `schema=2`
   for skinned meshes or `schema=3` for animations. In `[mod]`, declare
   `requires = manifest.resource-model@1`, `@2` or `@3` respectively, using
   the full key for each requirement. Install it in the mods directory and
   enable it.
6. **Check it:** the start log prints
   `resource-model <key>: <file> replaces <disc key> (<n> triangles, cooked)`,
   with the number of slot animations, then `... applied` when the model
   loads and one `animation L<layer>S<slot> replaces the file's` line per
   replaced slot. A refusal names the bound. The viewer seam above shows the
   model without starting a battle.

### Look

A replaced model goes through the same lighting and fog path as the retail
model, so it only fits in if its values fit. Measured over the disc's 1,007
weapon textures (331 weapon RMDs), with PortSeamlessUI's review of the
in-game proof:

- **Material.** Retail weapons are lit. They have normals and no vertex
  colours, the material colour is white, and fog is on. Keep those defaults:
  export normals, leave `KHR_materials_unlit` and `fogDisable` off, and
  don't add an emissive look.
- **Value range.** The median texture brightness (luma) of a weapon
  texture is 55 to 171 (10th to 90th percentile, median 98). Highlights reach
  about 198. WP004, the New Game default weapon (item 4), sits at 79 with
  highlights at 158. Saturated colours and near-white areas glow through the
  battle fog; keep accents at most one step brighter than the base.
- **Texture.** 64 to 128 pixels is typical. Use a few large blocks of value
  rather than fine patterns, which turn to noise at battle distance.
- **Silhouette.** A blade only one or two pixels wide shimmers at 640x448.
  Make thin parts about 1.5 times thicker than looks right in Blender. A
  curve or taper helps the outline read. Retail weapons have a median of 58
  triangles; the weapon budget allows 4,096.

`tools/models/make_test_weapon.py` follows these numbers.

## Limits (resource-model schemas 1 to 3)

- Schema 1 takes static meshes; schema 2 adds skinned meshes (glTF skin,
  `JOINTS_0`/`WEIGHTS_0`); schema 3 adds slot animations. Morph targets and
  more than four weights per vertex are refused. A mesh on a bone moves
  rigidly with that bone.
- A skinned mesh's weighted joints must be bones of the replaced model's
  HAnim hierarchy (`bone_<id>`). Its skin is not split into the PS2's
  64-bone VU1 batches; the port's CPU skinning does not need them.
- The replaced model's frames, HAnim skeleton, attach table and effects stay,
  and so do its animation slots: an animation replaces one that exists (a
  slot the file reads an animation chunk into), never adds a slot. New bones
  or frames are not used. Animations named otherwise than `L<layer>S<slot>`,
  and channels on nodes other than `bone_<id>`, are ignored. Bones animate by
  rotation and translation only; scale channels other than 1 and
  CUBICSPLINE interpolation are refused. A layer's base animation and blend
  group animations are not replaced.
- Only `TEXCOORD_0` is sampled. `KHR_texture_transform` is accepted and
  ignored. MatFX environment maps and UV animations are not created for the
  new materials.
- Models the game creates from memory rather than from a file (the title
  screen's pack) have no disc key and cannot be replaced.
- New model ids (a weapon item with no retail model) need the item
  expansion's own model id. This section replaces existing models only.
