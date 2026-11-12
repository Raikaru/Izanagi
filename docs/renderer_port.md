# Renderer port: sky2 device layer inventory

Scope: every entry point through which the matched RenderWare 3.7 core (`decomp/src/rw`, `decomp/src/renderware`) and the game reach the PS2 `sky2` device driver. Addresses are retail ELF addresses (`SLUS_217.82`). Option (c) in `decomp/docs/port/renderer.md` keeps the matched RW core and replaces this driver layer with OpenGL 3.3 core.

Asm/C column: `asm` = `INCLUDE_ASM` (MIPS body emitted from the retail ELF); `mark` = referenced by a `FUN_` marker only, with no `INCLUDE_ASM` found in the decomp tree. Unit = decomp source file that owns the address. All sky2 driver units are in `config/excluded_units.txt`: `src/rw/basky.c`, `baskytran.c`, `skyblit.c`, `skyinst.c`, `skyconv.c`, `skinskycommon.c`, `p2core.c`.

## 1. Device block (`RwDevice`, retail static data at 0x0070C220)

The core reads this block through `_rwDeviceGetHandle()` (`func_00401450`, `basky.c`, returns `&D_0070C220`). `RwEngineOpen` copies it into `RWSRCGLOBAL(dOpenDevice)` via the REGISTER request.

| Offset | Field | Retail value | Port action |
|---|---|---|---|
| +0x00 | gammaCorrection | 1.0f | keep |
| +0x04 | fpSystem | 0x004010c0 | replace (section 2) |
| +0x08 | zBufferNear | 0x477fff00 (65535.0) | keep |
| +0x0c | zBufferFar | 0 | keep |
| +0x10 | fpRenderStateSet | 0x003f5070 | replace (section 4) |
| +0x14 | fpRenderStateGet | 0x003f5d90 | replace (section 4) |
| +0x18 | fpIm2DRenderLine | 0 (runtime) | port fills at OPEN |
| +0x1c | fpIm2DRenderTriangle | 0 (runtime) | port fills at OPEN |
| +0x20 | fpIm2DRenderPrimitive | 0 (runtime) | port fills at OPEN |
| +0x24 | fpIm2DRenderIndexedPrimitive | 0 (runtime) | port fills at OPEN |
| +0x28 | fpIm3DRenderLine | 0 (runtime) | port fills at OPEN |
| +0x2c | fpIm3DRenderTriangle | 0 (runtime) | port fills at OPEN |
| +0x30 | fpIm3DRenderPrimitive | 0 (runtime) | port fills at OPEN |
| +0x34 | fpIm3DRenderIndexedPrimitive | 0 (runtime) | port fills at OPEN |

Retail never stores the Im2D/Im3D pointers in the ELF. The retail OPEN handler (`0x3fe700`) is PS2 GS/DMA hardware initialisation (writes to `0x89xxxx`), so the port's OPEN supplies these pointers itself.

## 2. System request dispatcher (`fpSystem`, 0x004010c0, `basky.c`, asm)

Signature: `s32 fpSystem(s32 requestID, void *pOut, void *pInOut, s32 numIn)`. Called from `_rwDeviceSystemRequest` (`badevice.c:459`, `baplcore.c:190`). The dispatcher is a 23-entry switch (`sltiu a0,23`) through the jump table at 0x00753280. Enum names are from `decomp/include/rw/plcore/badevice.h:58-85`.

| ID | Name | Retail handler | Retail behaviour | Port action |
|---|---|---|---|---|
| 0 | OPEN | 0x4011dc | PS2 hardware init (`0x3fe700`) | replace |
| 1 | CLOSE | 0x4011ec | returns 1 | replace |
| 2 | START | 0x4011f4 | `0x3ff5b0` (PS2 start) | replace |
| 3 | STOP | 0x401204 | `0x3f4f30` (PS2 stop) | replace |
| 4 | REGISTER | 0x4011a4 | copies the 64-byte device block (`0x70C220`) to `pOut` | replace; fill Im2D/Im3D first |
| 5 | GETNUMMODES | 0x40110c | writes 90 | replace |
| 6 | GETMODEINFO | 0x40111c | copies a mode record from `0x71B840` | replace |
| 7 | USEMODE | 0x4010fc | `0x3fe470` (PS2 mode set) | replace |
| 8 | FOCUS | 0x40119c | writes gp-relative focus flag | replace |
| 9 | INITPIPELINE | 0x4013a0 | returns 1 (no-op) | replace |
| 10 | GETMODE | 0x40118c | writes gp-relative mode | replace |
| 11 | STANDARDS | 0x4013a8 | `0x3fe210` fills `stdFunc` from table `0x70C140` (section 3) | replace |
| 12 | GETTEXMEMSIZE | 0x401378 | (`0x100000 - used) << 2` when a flag is set, else 0 | replace |
| 13 | GETNUMSUBSYSTEMS | 0x4013d8 | returns 0 | replace |
| 14 | GETSUBSYSTEMINFO | 0x4013d8 | returns 0 | replace |
| 15 | GETCURRENTSUBSYSTEM | 0x4013d8 | returns 0 | replace |
| 16 | SETSUBSYSTEM | 0x4013d8 | returns 0 | replace |
| 17 | FINALIZESTART | 0x401244 | PS2 pipeline init (`0x40d110`, `0x402600`, `0x401aa0`), then the VU0 matrix and vector multiplies into the engine slots (`_rwMatrixSetMultFn(0x402390)`, `_rwVectorSetMultFn(0x402410, 0x402470, 0x402510, 0x402570)`) | replace the pipeline init; install the same multiplies (`src/platform/rw_skytran.c`) |
| 18 | INITIATESTOP | 0x401348 | PS2 teardown (`0x402680`, `0x4015e0`, `0x40d160`) | replace |
| 19 | GETMAXTEXTURESIZE | 0x401368 | writes 1024 | replace |
| 20 | RXPIPELINEREQUESTPIPE | 0x4013d8 | returns 0 | keep 0 |
| 21 | GETMETRICBLOCK | 0x4013d8 | returns 0 | keep 0 |
| 22 | GETID | 0x4013c8 | writes u16 6 | replace |

## 3. Standard functions (`stdFunc`, table at 0x0070C140)

`STANDARDS` (request 11) copies `(rwSTANDARD id, handler)` pairs from `0x0070C140` into `RWSRCGLOBAL(stdFunc)` (`RwStandardFunc[29]`, slot 0 unused). Slot IDs are from `badevice.h:92-121`. The port keeps this layout and writes its own handlers into the same slots.

| ID | Slot name | Handler | Unit | asm |
|---|---|---|---|---|
| 1 | CAMERABEGINUPDATE | 0x003f6a10 | rw/basky.c | asm |
| 2 | RGBTOPIXEL | 0x00407c10 | rw/skyconv.c | asm |
| 3 | PIXELTORGB | 0x00407d10 | rw/skyconv.c | asm |
| 4 | RASTERCREATE | 0x003f98a0 | rw/basky.c | asm |
| 5 | RASTERDESTROY | 0x003fb830 | rw/basky.c | asm |
| 6 | IMAGEGETRASTER | 0x00407ff0 | rw/skyconv.c | asm |
| 7 | RASTERSETIMAGE | 0x0040a500 | rw/skyconv.c | asm |
| 8 | TEXTURESETRASTER | 0x003f7a60 | rw/basky.c | mark |
| 9 | IMAGEFINDRASTERFORMAT | 0x00407df0 | rw/skyconv.c | asm |
| 10 | CAMERAENDUPDATE | 0x003f73d0 | rw/basky.c | asm |
| 11 | SETRASTERCONTEXT | 0x00407bd0 | rw/skyconv.c | mark |
| 12 | RASTERSUBRASTER | 0x003fb3f0 | rw/basky.c | asm |
| 13 | RASTERCLEARRECT | 0x00402d50 | rw/skyblit.c | asm |
| 14 | RASTERCLEAR | 0x00403390 | rw/skyblit.c | mark |
| 15 | RASTERLOCK | 0x003fc5f0 | rw/basky.c | asm |
| 16 | RASTERUNLOCK | 0x003fd230 | rw/basky.c | asm |
| 17 | RASTERRENDER | 0x00403930 | rw/skyblit.c | asm |
| 18 | RASTERRENDERSCALED | 0x004052b0 | rw/skyblit.c | asm |
| 19 | RASTERRENDERFAST | 0x00406b60 | rw/skyblit.c | asm |
| 20 | RASTERSHOWRASTER | 0x003f79b0 | rw/basky.c | asm |
| 21 | CAMERACLEAR | 0x003f7490 | rw/basky.c | asm |
| 22 | HINTRENDERF2B | 0x003fe1f0 | rw/basky.c | mark (returns 1) |
| 23 | RASTERLOCKPALETTE | 0x003fd460 | rw/basky.c | asm |
| 24 | RASTERUNLOCKPALETTE | 0x003fd520 | rw/basky.c | asm |
| 25 | NATIVETEXTUREGETSIZE | 0x003fdd10 | rw/basky.c | asm |
| 26 | NATIVETEXTUREREAD | 0x003fdf20 | rw/basky.c | asm |
| 27 | NATIVETEXTUREWRITE | 0x003fddc0 | rw/basky.c | asm |
| 28 | RASTERGETMIPLEVELS | 0x003fd630 | rw/basky.c | asm |

Every handler lives in an excluded sky2 unit. Callers across `babintex.c`, `bacamera.c`, `badevice.c`, `baimage.c`, `baimras.c`, `baraster.c`, `batextur.c`, `baplcore.c`, `baworobj.c` go through the table, so the port only needs to replace the entries.

## 4. Render state (`fpRenderStateSet` 0x003f5070, `fpRenderStateGet` 0x003f5d90)

Both are asm in `rw/basky.c` (INCLUDE_ASM at lines 271 and 273).

RpSky render-state entry point `RpSkyRenderStateSet` (0x003f6440, `basky.c:275`, asm) has 685 call sites in 79 files. The `RpSkyRenderState` enum (`include/rw/sky2/rwcore.h:5685-5715`) is: 0 NARENDERSTATE, 1 DITHER, 2 ALPHA_1 (raw GS word), 3 ATEST_1 (low 16 bits of TEST_1), 4 FARFOGPLANE, 5 MAXMIPLEVELS (max 7). The game uses ALPHA_1 0x48 and ATEST_1 0x71801 (`src/Graphics/primitive.c:382-383`). The port maps these to GL state: PS2 alpha 0x80 = 1.0, z test, alpha test, fog, cull, and filter/address.

## 5. Other sky2 entry points referenced by the core

- `func_003f6780` (`basky.c:281`): the current RW driver trap. Its body is in `basky.c`.
- `func_003e84a0` (`bacamera.c`, returns NULL): with it `kwlnInitSceneRig` faults at `obj+0x60`. PortSkeleton-2 owns the bacamera un-exclusion.
- `func_003e3870` (batkreg), `func_003e8930`, `func_003e8b10` (`RwEngineOpen`), `func_003e8a10`, `func_003e89c0`, `func_003e8970` (baplcore), `func_003e8a50` (badevice): RW core entry points that call into the device block above.
- `func_0040e200`: default fill value written by `0x3fe210` for unused slots.

## 6. Open items (not yet inventoried)

- Atomic and world-sector instance pipelines: `skyinst.c` builds VU1 packets. Pipeline node addresses and their PS2 implementations are not yet listed. Blocks the viewer's geometry path.
- Native raster format of retail TXD and RMD textures (`rwID_TEXTURENATIVE` 0x15, 23,335 chunks): unverified.
- Pre-instanced geometry: none in the 17,337 RMD geometry structs (`renderer.md` §8.2).
- VU1 microcode: absent from the repo (`renderer.md` §8.5).
- `RpSkyTexCache`, material pipeline: not yet inventoried.

## 7. Findings (TMX decode, Im3D slots, default atomic pipeline)

- TMX decode: `src/render/p4tmx.c` (`include/p4pc/tmx.h`) mirrors `tmxDecodeStream` (`decomp/src/Kernel/sdkUttmx.c`, clang-excluded). Header: `stream[0]==2`, `stream[1]==0`, `TMX0` at +8, data at +0x40. Format byte +0x16 selects 32/24/16/8/4 bpp. Palette count +0x10, palette format +0x11 (0 = RGBA32, 2/10 = RGB555 with entry 0 transparent). Width u16 at +0x12, height u16 at +0x14. The 8-bit palette swaps entries 8..15 with 24..31 within each 32-entry block. 4-bit pixels: low nibble is the even pixel. The port returns -1 for cases retail leaves undefined (indexed without a palette, direct colour with a palette, odd width at 4 bpp).
- Im3D slots (device +0x28..+0x34): the RwIm3D render entries (`RwIm3DRenderIndexedPrimitive` 0x410520, `RwIm3DRenderPrimitive` 0x4106a0) do not call the device table. They store the primitive type and vertex data in a per-Im3D state block addressed as `0x8872E0 + *(gp-18000)`. The port has no Im3D handlers yet.
- Default atomic pipeline: `AtomicDefaultRenderCallBack` (`decomp/src/renderware/world/baclump.c:501`) runs `atomic->pipeline`. `RpAtomicRender` (2147) calls it for atomics inside the frustum. `defaultAtomicPipeline` (`decomp/src/renderware/p2/p2core.c:2165`) builds one packet per RwMaterial, then dispatches to the material pipeline (transform, lighting, render). The leaf that calls the device is not yet identified.
- Im2D (`src/render/p4gl_im2d.c`) samples the raster that RenderWare state TEXTURERASTER (1) names. TEXTUREADDRESS, ADDRESSU and ADDRESSV map to GL wrap modes (WRAP to repeat, MIRROR to mirrored repeat, CLAMP to clamp to edge, BORDER to clamp to border with GL's default transparent black). TEXTUREFILTER NEAREST maps to `GL_NEAREST`, and everything else to `GL_LINEAR`. The device keeps one mip level, so the mip filters fall back to linear. A draw with no raster is untextured. Texture coordinates are perspective-correct through recipZ (`textureProj`), and vertex colour modulates the texel (the vertex colour is 0..255 per channel, the RenderWare convention; the shader divides by 255).
- Sky2 SUBRASTER deviations (`src/render/p4gl_raster.c`): the child borrows the parent's shadow; the child keeps cType 5; no GS page rounding and no TEX0/TEX1; palettes are rejected; a camera parent gets a child-owned shadow; framebuffer copy-to-texture is not implemented; the parent must be destroyed after its children.
- Native textures (`src/render/p4gl_native.c`, `p4gl_nativetex.c`): the PS2 chunk of a TXD or RMD texture is a 64-byte header chunk and a data chunk. Paletted textures (kind 2) carry the GS upload packets of the sky texture cache: PSMT8 and PSMT4 memory images pre-swizzled through PSMCT32 and PSMCT16 uploads, and the CLUT. `func_003fd940` (readTEXFromStream) decodes them into a linear PAL8 / PAL4 raster with its palette (`p4gl_nativetex_decode`; the swizzle formulas are in the file header, `tests/native_tex_test.c` checks both textures of DOBJ044.RMD, decoded images were looked at: clouds). Kind 0 and 1 data is the raster's pixels, with the PS2 alpha doubled. The texture plugin 0x110 callbacks (`func_003f6910` read, `func_003f6990` write) consume and emit the one filter word the retail code kept in the sky raster extension, which the port does not have. Paletted rasters report format PAL8 / PAL4 whatever palette colour format the flags carry (0x2504 is PAL8 | 8888). The raster upload pads to power-of-two since 4acca98 (GS textures are power-of-two).
- `p4pc_viewer` (`src/viewer/p4pc_viewer.c`) loads an RMD with the game's `mdlCreateModelObjectFromData` (type 4), draws the clump with `mdlDrawVisibleAtomics`, and uses the camera from `kwlnCameraCreate(640, 448, 1)`. It reads an offscreen RGBA8 FBO back and writes a PNG. The camera is placed by the engine default; a model placed away from the origin may come out blank.
- `render_smoke` (`tests/render_smoke.c`) covers the offscreen path: the TMX found on disc, a RASTERCREATE/LOCK/UNLOCK round trip, then a full-frame Im2D quad textured from that raster (TEXTURERASTER, nearest filter). Two probed pixels must match the TMX texels that nearest sampling picks. The frame is written as a PNG to argv[1] (default `render_smoke.png`). It boots the game image like the viewer and exits 77 unless both `P4PC_ISO` and `P4PC_ELF` are set.

## 8. Pipeline and node map

Source: retail ELF `SLUS_217.82`. Node definitions are `RxNodeDefinition` structs (`include/rw/sky2/rwcore.h`, ~1275-1320): name at +0, `nodeMethods` at +4 (nodeBody +4, nodeInit +8, nodeTerm +0xC, pipelineNodeInit +0x10, pipelineNodeTerm +0x14, pipelineNodeConfig +0x18, configMsgHandler +0x1C), then io, private data size, editable.

| Node definition | Struct | nodeBody | Getter / referenced from | Verdict |
|---|---|---|---|---|
| PS2All | 0x0070b110 | 0x003cf140 | `RxNodeDefinitionGetPS2All` 0x003cfa70 | Opens a DMA packet: calls `func_0040a800` (0x003cf2d8), which calls `func_003f3eb0` (GIF packet open). Atomic-path terminal and the GL seam. Per-atomic work is in `PS2AllNodeBody` 0x003cf610 (no direct DMA call). |
| PS2AllMat | 0x0070b470 | 0x003d3120 | `RxNodeDefinitionGetPS2AllMat` 0x003d3a80 | pipeNodeInit 0x003d30b0. No DMA call. Calls `0x4125d0` and `0x412520` (unnamed). |
| PS2Im3DFastTransform | 0x0070b0a0 | 0x003cc250 | `fastIm3DTransformPipeCreate` 0x003cc2c0 | No calls in its 112 bytes. Transform only. |
| PS2PTank | 0x007084a0 | 0x003a8840 | `func_003a76c0` | Direct DMA: calls `func_003f3eb0` (0x003a8924). Particle path, not atomic. |

Pipeline builders and executors (call graph from the ELF):

- `RpPDSGetPipe` 0x0039b6e0 (Ballers name) builds pipes lazily. It calls both getters, `RxPipelineLock` 0x00412ca0, `RxLockedPipeAddFragment` 0x00412fb0 and `RxLockedPipeUnlock` 0x004125e0. It has 23 callers.
- `fastIm3DTransformPipeCreate` 0x003cc2c0 calls Lock, AddFragment and Unlock. Its caller `func_003cc370` calls `RpPDSGetPipe` four times.
- `func_003a5280`, `func_003a76c0` and `func_003a8d20` build pipelines with the same getters and Lock/Add/Unlock calls. They have no direct `jal` callers, so they are probably reached through a function-pointer table. `func_003a8d20` calls `RxPipelineExecute` four times.
- `RwIm3DTransform` 0x00410420, `RwIm3DRenderIndexedPrimitive` 0x00410520 and `RwIm3DRenderPrimitive` 0x004106a0 call `RxPipelineExecute` 0x004114d0.
- `_rwIm3DCreatePlatformTransformPipeline` 0x00415930 and `_rwIm3DCreatePlatformRenderPipelines` 0x00415950 are `li v0,1; jr ra` stubs. Their only caller is `_rwIm3DOpen`.

Verified RW pipeline API (prologues and callers checked): `RxPipelineExecute` 0x004114d0, `RxPipelineLock` 0x00412ca0, `RxLockedPipeAddFragment` 0x00412fb0, `RxLockedPipeUnlock` 0x004125e0. `RxLockedPipeAddPath` 0x00413290 is NOT a function entry: it is mid-function inside `RxLockedPipeAddFragment`, so the Ballers address is wrong.

Open: the Im3D render terminal node (no `.csl` name found); the identity of `0x4125d0` and `0x412520`; the pipe id `RpPDSGetPipe` uses for the atomic pipeline; the function-pointer registry that reaches `func_003a5280`/`func_003a76c0`/`func_003a8d20`.

### Renderer lane, replaced at the seam (do not transliterate)

| Address | Name | Role | Replaced by |
|---|---|---|---|
| 0x003cf140 | PS2All nodeBody | DMA producer, calls func_0040a800 | GL atomic node (claimed, `overrides/003cf140.c`) |
| 0x003cf610 | PS2AllNodeBody | per-atomic packet build | GL atomic node (claimed, `overrides/003cf610.c`) |
| 0x0040a800 | func_0040a800 | DMA packet open, calls func_003f3eb0 | not ported; the GL node does not call it |
| 0x003f3eb0 | func_003f3eb0 | GIF packet open (`_rwDMAOpenGIFPkt`) | not ported; called only from DMA producers |
| 0x003a8840 | PS2PTank nodeBody | particle DMA producer | GL particle draw (claimed, `overrides/003a8840.c`, `overrides/003a2290.c`, `src/render/p4gl_ptank.c`) |


## 9. Verified asm facts (PortGL, this session)

- `func_003a8d20` (element walk): the top-level element (`packet+8`) is used as an RpAtomic. It reads `atomic+24` (geometry), `geometry+84` (mesh), `geometry+8` (flags, 0x100000 test), `geometry+12` (lockedSinceLastInst cleared), and `atomic+4` (RwObject.parent, the RwFrame, passed to `0x3e9700`). The inner loop walks a list at `s5+100`; its items hold an object pointer at `+8` whose halfword at `+60` and flag byte at `+2` are checked. That layout does not match RpAtomic (`clump` is at +60), so the list items are not plain atomics. Unresolved.
- `RwObject.parent` of an atomic is the RwFrame: `RwObjectHasFrame` is `{RwObject object; RwLLLink lFrame}` (`rwcore.h:4517`), and `rwObjectGetParent` returns the parent field. The setter's asm (`_rwObjectHasFrameSetFrame`) is not located in the symbol maps; the structural link is from the header.
- `RpSkyRenderStateSet` (0x003f6440) and `RpSkyRenderStateGet` (0x003f6690) are port-owned in `src/render/p4gl_state.c`. Ids 1-5 are recorded in the port's state record; the retail gp-global writes and the GIF packet do not run. Any other id returns 0. They never call `fpRenderStateSet`, so RpSky ids do not collide with the RW ids in the device table.
- `PS2Im3DFastTransform` body (0x003cc250): reads a count (u16 at `+0`), stores a data pointer (`+4`) to `+24`, and checks flag bit 0 at `+12`. Otherwise it calls a DMA allocator through `0x887300`. `fastIm3DTransformPipeCreate` (0x003cc2c0) locks a pipe (0x412ca0), adds the node (0x412fb0, node def `0x70B0A0`), and unlocks (0x4125e0).
- `func_003cc370` (Im3D setup) calls `fastIm3DTransformPipeCreate` (0x003cc2c0), then `RpPDSGetPipe` (0x39b6e0) with ids 4, 6, 5, 7. It stores the pipes into the Im3D state block at `0x8872E0 + *(gp-18000)` (offsets 28-60), then calls `0x410a40`, then `0x410ab0` with (pipe, type) = (s3,3), (s3,5), (s3,4), (s2,1), (s2,2). `func_003cc460` is the teardown. `_rwIm3DCreatePlatformTransformPipeline` (0x415930) and `_rwIm3DCreatePlatformRenderPipelines` (0x415950) are `li v0,1; jr ra` stubs.
- Im3D state block (base `0x8872E0 + *(gp-18000)`, verified from `RwIm3DTransform` 0x410420, `RwIm3DRenderPrimitive` 0x4106a0, the triangle/indexed render at 0x410800, and the setters 0x410a40 and 0x410ab0). Transform pipe at +0. Terminal pipe per primitive type at +4 (type 3, triangle list), +8 (type 5, triangle fan), +12 (type 4, triangle strip), +16 (type 1, line list), +20 (type 2, polyline), +24 (type 6, unused). The setter's jump table is at 0x753410. Transform packet at +64: u16 vertex count, +68 vertex pointer, +72 stride 36 (`RxObjSpace3DVertex`: position @0, colour @12, normal @16, uv @28), +76 flags|0x18, +80 LTM pointer. Render packet at +76: +112 primitive type, +116 index array pointer (indexed/triangle only), +120 vertex count. Render executes the terminal pipe for the type, with the render packet as data.
- Im3D terminal pipes come from `RpPDSGetPipe` (0x39b6e0, PDS lookup at table 0x884ACC, pipe at entry +16). Id 6 backs types 3, 4, 5. Id 7 backs types 1, 2. Ids 4 and 5 are stored at +56 and +60. The node contents of ids 6 and 7 are not set by any code read so far (`_rwIm3DCreatePlatformRenderPipelines` is a stub), so their terminal nodes are still unidentified.
- Im3D transform node (`PS2Im3DFastTransform`, 0x3cc250) writes its output through the DMA allocator, so the transformed vertices are not available on the CPU without VU1 output. The GL plan must transform on the CPU from the raw vertices at +68 and the LTM at +80.
- Im3D (implemented). `decomp/src/renderware/p2/baim3d.c` is compiled decomp C (it stays in `config/units.txt`: excluding it would drop RwIm3DGet/SetTransformPipeline and RwIm3DSetRenderPipeline, which `func_003cc370` still needs), so the six retail entry points are replaced through `overrides/<addr>.c` (`tools/gen_port.py` `apply_overrides` splices the body over the definition after the `// FUN_<ADDR>` marker): RwIm3DTransform 0x410420, RwIm3DEnd 0x4104d0, RwIm3DRenderIndexedPrimitive 0x410520, RwIm3DRenderPrimitive 0x4106a0, RwIm3DRenderTriangle 0x410800 (three indices, primitive type 3), RwIm3DRenderLine 0x410930 (two indices, type 1). sky2hook's address list (decomp issue #2) gives RwIm3DRenderLine = 0x00410800 and no RenderTriangle; the asm says otherwise (0x410800 takes three vertex indices and stores type 3; 0x410930 takes two and stores type 1), so that entry is a mislabel and its "line" hook would draw triangle arguments. The overrides are the function bodies, so a render command dispatcher that reaches them by address or by function pointer reaches the GL path with no hook list to keep in sync.
- Im3D behaviour. The retail state block at `0x8872E0 + *(gp-18000)` (`_rwIm3DModule.globalsOffset`, symbol `ourGlobals`) stays the source of truth: Transform writes the pool (u16 element count, so exactly 65536 vertices is an empty pool; stride 36; stash ltm and flags|0x18) as retail; Render* write the stash (primType, indices, numIndices) as retail. Transform copies the vertices and moves them to clip space with `p4gl_camera_object_matrix` (`src/render/p4gl_im3d.c`; with no open camera update it returns NULL); Render* draw that copy with a GL program and skip `RxPipelineExecute`. Return values: Transform returns the vertex pointer or NULL; Render* return TRUE or FALSE; errors raised through the same `RWERROR`: 50 (E_RX_TOOMANYVERTS, numVerts above 65536), 35 (E_RX_IM3DNOTACTIVE, no Transform pending), 37 (E_RX_INVALIDPRIMTYPE). Indexed counts: line list `n - n % 2`, triangle list `n - n % 3` (C semantics, so a negative odd line count rounds up), other types unchanged; a count of zero or less after rounding draws nothing and returns TRUE; an index at or above the vertex count draws nothing and returns FALSE (retail leaves that undefined). RenderTriangle and RenderLine keep their indices on the stack instead of the RxHeap. ALLOPAQUE (2) forces alpha 1; NOCLIP (4) has no effect (clipping always applies; scaled lines also clip on the CPU before virtual-width expansion, section12). The uv are always read, whatever the VERTEXUV flag says (sky2hook measured the gated read as worse; P4's shadow projection relies on it). Blending and the alpha test follow RpSky ALPHA_1 and TEST_1, depth follows RwRenderState ZTESTENABLE (default on) and ZWRITEENABLE (default off, sky2hook's measured default for draws before the game sets it); triangle CULLMODE follows the render state (below), while lines ignore face culling. `render_smoke` drives the real entry points with a scratch state block and a synthetic camera: textured quad (PNG `<png>.im3d.png`), flags 0, LTM and depth, ALLOPAQUE, indexed rounding, bad indices, RenderTriangle, RenderLine, and the error returns.
- RpSky render state, port-owned (`src/render/p4gl_state.c`, claimed 003f6440 and 003f6690). Retail global writes are kept as the state record; the GIF packet is dropped. GS ALPHA decoding is derived from sky2hook `gsAlphaToBlend`: the register is (A - B) * C / 128 + D, A/B/D from Cs, Cd, 0 and C from As, Ad, FIX; mapped are over (0x44), additive (0x48), subtractive (0x42, reverse subtract), modulate, keep-destination, with FIX as the blend constant alpha (RpSky passes FIX in bits 24-31 of the state word; it is bits 32-39 of the register); all-zero fields are blending off; other combinations log once and use source-alpha over. GS TEST decoding follows sky2hook `setGsAtest`: only AFAIL = KEEP (or ZB_ONLY) discards colour, so ATST NEVER with FB_ONLY (the 0x71801 the game writes) draws every fragment and ALWAYS never discards; the shader codes are 0 (no discard), 2..7 (LESS, LEQUAL, EQUAL, GEQUAL, GREATER, NOTEQUAL) and 8 (NEVER with KEEP), compared on PS2 alpha (`a_gl * 128`, AREF bits 4-11). This fixes the earlier code, which treated ATST 1 as NEVER. Not applied: the destination alpha test (DATE/DATM), the FIX+1 combination A=Cs, B=Cd, C=FIX, D=Cs. Initial ALPHA_1 is 0x44. FARFOGPLANE (id 4) and MAXMIPLEVELS (id 5) are recorded for Get round-trip only (the device uploads one mip level, and max mip defaults to 0, unset). sky2hook reproduces P4's fog as linear fog from `camera->fogPlane` to the FARFOGPLANE float (read through the pointer RpSkyRenderStateSet receives, falling back to the camera far plane) with the FOGCOLOR render state, on atomics only (not Im3D); not ported yet.
- Atomic, Im2D, Im3D and PTank all read the record per draw. FRAME_1 FBMSK and CLAMP_1 set by the mdlEffect emitters (below) apply to every draw path: the mask through `glColorMask` around each draw, the clamp as an override of the RwRenderState addressing in `p4gl_texture_bind`.
- PTank rotation uses the retail float trig cores, not host double `sin`/`cos`.
  GNU R5900 disassembly shows `003abdac -> 0044b7b0` (sinf) followed by
  `003abdb8 -> 0044b610` (cosf), both loading the same float rotation into f12.
  `p4gl_ptank.c` follows that order and argument precision; the rotated-band
  cases in `render_smoke` continue to exercise the resulting quad placement.
- PS2PTank (implemented: `overrides/003a8840.c` the node body, `overrides/003a2290.c` the atomic render callback, `src/render/p4gl_ptank.c`). Derived from sky2hook `src/pipelines/atomic/ptank.cpp` with its layout cross-checked against the retail create code and node body: the PTank record is the pointer at `atomic + iGpffffb610` (RpPTankAtomicExtPrv: maxPCount +0, actPCount +4, lockFlags +0x3c, instFlags +0x40, platFlags +0x44, publicData +0x50); publicData holds nine clusters (data pointer, stride) at +4 + 8*c (0 position, 1 matrix, 2 normal, 3 size, 4 colour, 5 vertex colour, 6 2D rotation, 7 two uv, 8 four uv), live only when their data flag is set (the flag bits are not in slot order: position 1, colour 2, size 4, matrix 8, normal 0x10, rotation 0x20, vertex colour 0x40, uv2 0x80, uv4 0x100), then userData (+0x4c, RpPTankSkyData with the tank's GS ALPHA at +0xb8, CLAMP +0xc0, TEST +0xc8, TEX1 +0xd0), format (+0x50), dataFlags (+0x58), vertexAlphaBlend (+0x64), cCenter, cSize (+0x70), cRotate (+0x78), cColor (+0x7c), cVtxColor[4] (+0x80), cUV[4] (+0x90), cMatrix (+0xb0). The game (effParticle.c) locks position, colour, size, rotation and uv2. The retail node body sends instanced DMA chunks and a VU1 program expands the quads; the port draws the particle arrays directly, so the instance functions (rpPTankSkyIns*) have no consumer. Quads are half-size billboards in the camera plane (width and height `size`), rotated by the 2D rotation, corners top-left, top-right, bottom-right, bottom-left, uv2 as opposite rectangle corners, colour = material colour x texel x (particle colour, else corner colours, else cColor); blending and the alpha test come from the tank's own ALPHA and TEST registers when they decode cleanly (blend only with vertexAlphaBlend), depth test from TEST.ZTE and ZTST, depth write from the ZWRITEENABLE state unless ATST NEVER with FB_ONLY. The 3D sprite generator (platFlags 2) is drawn; the 2D generator (platFlags 0) with only a position places particles in screen space and is skipped with one log line, as in sky2hook, but a tank with a matrix per particle (FldRainUpdate: data flags colour | matrix, platFlags 0) is placed by the matrix and drawn, the centre being matrix row 3 unless a position cluster overrides it. cCenter (USECENTER) and the clone pipeline are not drawn. Open: the plugin create, lock and instance callbacks (RpPTankAtomicCreate 0x3a1730/0x3a1c40, RpPTankAtomicLock 0x3a2770, 0x3a5180, the sky instance setup) are asm-only and unclaimed, so no PTank atomic is created by the port yet; the draw is exercised by `render_smoke` with a synthetic record (textured full-frame billboard, colour, size, quarter-turn rotation, the platFlags skip).
- mdlEffect GS register emitters (P4's own writers, called by the game's effect and blur code, so the renderer must handle them): 0x00489f80 mdlEffectEmitMaskedTagPair (FRAME_1 FBMSK 0x00FFFFFF: alpha-only write mask), 0x0048a000 mdlEffectEmitTagPair (FRAME_1 restore), 0x0048a070 mdlEffectEmitValuePacket (CLAMP_1 = arg), 0x0048a0e0 mdlEffectEmitSlotPacket (CLAMP_1 restore from iGpffffb8e8), 0x001852f0 FldNoiseEmitTagged (FBMSK 0xFF000000: colour-only mask), 0x00185370 FldNoiseEmitPlain (restore). Each retail body opens a two-quadword packet with `func_003f3eb0(0x80000000, 2)` and appends a `0x1000000000008001` tag plus the register write at the packet pointer iGpffffb884; their units (mdlEffect.c, y_misc_inferred_00182310.c) are compiled decomp C, so `overrides/<addr>.c` replaces each body with a call into `src/render/p4gl_mdleffect.c`, which sets the GL mask and clamp state. The base FRAME_1 is iGpffffb8c8 (its high word is FBMSK). The blur filter's CLAMP_1 (0x9FC00A | 0x6FC << 32: region clamp 0..639 by 0..447) reads the frame buffer as a texture; the frame rasters below provide the framebuffer copy.
- skyDrawRaster 0x007649F4 and skyDisplayRaster 0x007649F0 (sky2hook) are two distinct frame rasters whose GS blocks swap on a flip: the draw raster is the frame so far, the display raster the presented one. The port presents one framebuffer (`p4gl_context_present`); `RpSkyGetDisplayBufferRaster` 0x00401b80 is port-owned (`func_00401b80`) and returns the display raster; no retail draw-raster getter exists (gp-18172 is read only by sky internals). See the frame rasters bullet.
- GS-writer audit: every direct `jal 0x3f3eb0` site (`_rwDMAOpenGIFPkt`, the sky DMA ring's packet open: it takes (flags, quadwords), flushes channel 1 when the ring is short and recurses once at 0x3f434c), 65 sites in 45 functions found by scanning the ELF with `mipsel-linux-gnu-objdump -m mips:5900`, owners by `decomp/config/symbol_addrs.txt`. Every function is mapped or ignored; "trap" below means the function is asm-only, so the port's unresolved-symbol trap names it if it is ever reached.
- Mapped, port owns the function (retail GIF writes never run): `_rwSkySetRenderState` 0x3f5070, 13 sites (0x3f5224 0x3f528c 0x3f5314 0x3f53bc 0x3f5468 0x3f5500 0x3f55d8 0x3f56e4 0x3f5800 0x3f591c 0x3f59e0 0x3f5bb8 0x3f5c4c, claimed, fpRenderStateSet); `RpSkyRenderStateSet` 0x3f6440 (0x3f64a4 0x3f64c0 0x3f6518); `SkyCameraBeginUpdate` 0x3f6a10 (0x3f6b20); `SkyCameraEndUpdate` 0x3f73d0 (0x3f740c); `func_003f7490` CAMERACLEAR (0x3f7554); the six mdlEffect/FldNoise emitters above (0x185300, 0x185380, 0x489f90, 0x48a010, 0x48a088, 0x48a0f0); PS2PTank `func_003a8840` (0x3a8924); `func_003cf140` PS2All node (no site in this list).
- Mapped through their only callers, which the port owns (so the function never runs): `func_00401b90` (0x401c50, caller SkyCameraEndUpdate); `RpSkyTexCacheRestore` 0x410080 (0x4100b8, caller SkyCameraBeginUpdate); `func_003f3750` (0x3f376c, caller SkyRasterShowRaster 0x3f79b0 = RASTERSHOWRASTER); `func_0040f1c0` (0x40f3bc, caller 0x3fd520 = RASTERUNLOCKPALETTE); `func_003fe320` (0x3fe3f8, caller 0x3ff5b0, whose only caller is the dispatcher 0x4010c0); `func_0040a800` (0x40a848, callers 0x3a8840 and 0x3cf140, both replaced); `func_003f44c0` 0x3f46dc and `func_003f4730` 0x3f474c (DMA ring wait/send, callers: the PS2All and texcache chains below, SkyRasterCreate 0x3f98a0 via 0x40d720, and RpSkyTexCacheRasterLock 0x40fcd0 via 0x40df50; the last two are claimed); the self-call at 0x3f434c inside `func_003f3eb0` (its recursion after a flush; the function has no caller left).
- Ignored, PS2All and PS2Manager node callbacks (the retained-mode pipeline internals): `func_0039c730` (0x39c794), `func_0039cb80` (0x39cd10 0x39d1f4), `func_0039d360` (0x39d96c 0x39da08), `func_0039dcc0` (0x39e334 0x39e3f8), `func_0039e740` (0x39ed40 0x39eddc), `func_0039f160` (0x39f408), `func_0039f9d0` (0x3a005c), `func_003a03a0` (0x3a04d4 0x3a0538), `func_003a0aa0` (0x3a10bc 0x3a11c4), `func_003d0970` (0x3d0a80), `RwIm3DPS2AllResEntryAllocCallBack` 0x3d2240 (0x3d226c), `func_003d2300` (0x3d2314). Reason: none has a `jal` caller outside the same family (the wrappers 0x3a0260..0x3a0360 and 0x3a1470/0x3a14b0 have none either); their addresses sit only in the pipeline descriptor tables at 0x64f2b4..0x64fe5c that `func_00100670` (kwlnInitEngine) registers with `RpPDS`, and they are invoked through `jalr` from the PS2All node bodies 0x3cf140 (jalr at 0x3cf20c 0x3cf41c 0x3cf4e4 0x3cf4f8 0x3cf524) and 0x3cf610 (0x3cf718 0x3cf7b8), both claimed and replaced by the GL atomic node. `PS2AllMat` 0x3d3120 is a 16-byte stub. If a pipeline outside those two ever runs them, the trap names the function.
- Ignored, PTank internals: `func_003a6650` (0x3a675c 0x3a6950) and `func_003a7a30` (0x3a7b40 0x3a7d30), the Spr2dGen and Spr3dGen instance setups reached through 0x3a6ba0/0x3a7160 and 0x3a7f80/0x3a8500; `func_0039c900` (0x39c928, callers 0x3a93b0 and 0x3a9880) and `func_003a14f0` (0x3a1510), reached only from `func_003a9f40`, which no `jal` calls (its only reference is a data word at 0x7084ec next to the PTank node definition): the PTank clone pipeline, which the port does not draw. Reason: the port draws the particle arrays directly, so the instance and clone chains have no consumer. They are the retail side of the plugin create/lock path noted under PTank above; if the plugin is ever created natively these stay unreached, and if retail create runs they trap by name.
- Ignored, sky raster blits: `func_004026d0` (0x40270c), `spriteRGBAZ` (0x4029bc), `spriteUV` (0x402afc), `func_00402c10` (0x402c1c). Reason: their only callers are `_rwSkyRasterClearRect`, `_rwSkyRasterRender`, `_rwSkyRasterRenderScaled` and `func_00406b60`, the handlers of stdFunc slots 13, 14, 17, 18 and 19 (RASTERCLEARRECT, RASTERCLEAR, RASTERRENDER, RASTERRENDERSCALED, RASTERRENDERFAST); a scan of the ELF for loads of those slot addresses (0x88735c, 0x887360, 0x88736c, 0x887370, 0x887374) finds none, while slots 4 and 15 (RASTERCREATE, RASTERLOCK) are loaded by `RwRasterCreate` 0x3ec590 and `RwRasterLock` 0x3ec6a0, so no retail code calls the blit entry points. The port's STANDARDS leaves those slots at the core default.
- Ignored, texture cache: `RpSkyTexCacheAccessRaster` 0x40f570 (0x40f748; 24 callers: `_rwSkySetRenderState` (claimed), the callbacks above, `func_003a29f0` (PTank) and the sky raster blits). Reason: it uploads a raster to GS texture memory and writes TEX0/CLAMP; the port keeps rasters as GL textures (`src/render/p4gl_raster.c`) and binds them per draw.
- Ignored, VU1 program descriptor: `func_0040a650` (0x40a68c): no `jal` callers; it appears twice in the descriptor table at 0x753330 and 0x753340 next to VU1 code ranges (0x5594a0..0x559600, 0x559030..0x559230) used by the sky instancing that the Im2D handlers replace.
- The callers of the FldNoise emitters (`func_001854f0`, `func_00185620`, `func_00185600`) live in `y_misc_inferred_00182310.c` and `mdlEffect.c` and call the emitters, which the overrides replace.
- Camera and cull. `RwCameraBeginUpdate` stores in `RwCamera.viewMatrix` (+0x20) not the view matrix but one pre-scaled by the view window and mapped to the unit square (`bacamera.c`: right.x = -0.5 / viewWindow.x, up.y = -0.5 / viewWindow.y, at.x = at.y = 0.5): the screen position is x = 0.5 - 0.5 * cx / (viewWindow.x * cz), y = 0.5 - 0.5 * cy / (viewWindow.y * cz), y down, so camera +x is the screen's LEFT. `p4gl_camera_object_matrix` therefore ignores that field: it builds the object-to-camera matrix from the object LTM and the camera frame's own LTM (the camera's parent frame, +80), and the clip x is -cx / viewWindow.x. Using the field as a plain view matrix put every image half a screen off centre. Front faces are counter-clockwise in the displayed image: `CULLMODE` back (2) / front (3) apply with `GL_CCW`; checked on `field/rmd/m001_005.RMD`, a closed lit solid that shows its inside faces under the other sense. `p4gl_cull_apply` runs in the atomic and Im3D paths; Im2D disables cull; `render_smoke` checks a clockwise quad is a back face.
- Frame rasters (`p4gl_frame_rasters_open/present`, `src/render/p4gl_raster.c`). skyDrawRaster (0x007649F4, the frame so far) and skyDisplayRaster (0x007649F0, the presented frame) are RwRasters (640x448, camera type, no pixels) whose GL texture is a row-flipped `glBlitFramebuffer` copy of the framebuffer: the draw raster at every bind (`p4gl_raster_texture`), the display raster at `p4gl_context_present`. This is the copy-to-texture the mdlEffect blur filters need (their CLAMP_1 region clamp reads it). The region bounds of CLAMP_1 are not applied.
- Disc requests. The game's file path (H_Cdvd_*, `sdkCdvd.c`) opens "VOL:NAME" / "BTL:NAME" through the CRI ADXF layer after `H_Cdvd_SelectDirectory`. The port replaces the ADXF open/close/size/sectors/read/state (0x4c85a0, 4c8a60, 4c9670, 4c95f8, 4c9010, 4c9820) with `src/platform/files/sce_adxf.c` over the VFS. Bytes may be read immediately, but completion remains busy until the next scheduler frame boundary. `overrides/00455230.c` records the normalized directory instead of installing a ROFS dir list. `p4pc_platform_init` mounts the disc (sceCdInit never ran in the port's boot).
- Viewer. `p4pc_viewer SLUS_217.82 field/rmd/m001_002.RMD [out.png]` takes a game-relative path. The model load is asynchronous: the viewer starts the load worker (`hsfdInitWorker`), lowers the boot thread (id 1) below it (the thread model is cooperative), pumps `H_Cdvd_UpdateRequests` and `sdkWrapPollRequests` until `mdlPrepareModel` returns 1, frames the camera on the clump's bounding spheres and draws between `RwCameraBeginUpdate` and `EndUpdate`. The async queue statics in `h_sfdply.c` (sSfdQueue, sSfdThreadIds) are placed at retail 0x00764bd0 / 0x00764bc8 by `config/mirror_fixes.txt`. Boot-path facts found on the way: the video mode table is at 0x0070B840 (not 0x0071B840), the RW memory table slots are read at 0x008873E8.., kwlnCameraResizeView's mode buffer is 24 bytes.
- PTank create (port natives, `overrides/003a20e0.c`, `003a2390.c`, `003a3670.c`, `003a3e90.c`; `RpPTankAtomicCreate` 0x3a2340 and the lock 0x3a2770 were already native). `func_003a2390` is the retail `_rpPTankAtomicCreateCustom` transcribed: RpAtomicCreate, the 0x140-byte record, the callback table at 0x708460 copied to record +0x14, the alloc callback, the create callback, then the constants (size 1, centre 0, identity matrix flags, colour and corner colours from 0x763B68, unit-square UVs from 0x707E50, blend state through `fpRenderStateGet`). The two sky callbacks are the port's, not transcriptions: the alloc callback (`func_003a3670`) lays the clusters out (array or structure layout, one 16-aligned block with the user data first at +0x00, ALPHA 0x44 and TEST ZTE|GEQUAL as defaults until `RpPTankAtomicSkyRenderStateSet`) without the DMA chains and VU1 rings of the retail version, and the create callback (`func_003a3e90`) gives every tank the retail "geometry holder" shape (an empty geometry with one fresh material, instancers cleared, pipeline pointer +0x6C from the PTank module) instead of selecting an `rpPTankSkyIns*` setup. The other `rpPTankSkyIns*` rows (0x3a5280, 0x3a72a0, ...) are never reached. `p4pc_viewer ... ptank out.png` creates a four-particle matrix tank through the retail create chain, fills it through the retail lock and draws it through the atomic's render callback (four coloured quads, verified by image). The array/structure builders 0x3a1730 / 0x3a1c40 are only used when a platform has no alloc callback; they stay asm-only.
- World module records. The named RenderWare world code (`RWMATERIALGLOBAL` and the like) and the retail plugin open callbacks that `RpWorldPluginAttach` (0x3ca430) registers must share one `RwModuleInfo` per module; `clumpModule` 0x7647B0, `lightModule` 0x7647C0, `materialModule` 0x7647D0, `sectorModule` 0x7647E8, `worldModule` 0x7647F0 and `worldObjModule` 0x764808 now sit at their retail addresses (`config/port_symbols_manual.txt`) instead of port-owned storage. Without it the first `RpMaterialStreamRead` found a NULL freelist.
- Plugin registries and plugin offsets. The retail RegisterPlugin / RegisterPluginStream wrappers (code1_003c.c, code1_003e.c) pass the registries' retail addresses (atomic 0x70AF70, clump 0x70AF90, geometry 0x70AFB0, light 0x70AFD0, material 0x70AFF0, sector 0x70B040, world 0x70B060, camera 0x70B710, frame 0x70B7A0, image 0x70B7C0, raster 0x70B7E0, texture 0x70B800, texdict 0x70B820), while the named decomp code read its own host copies, so every stream callback and plugin size registered by the game was invisible to the stream readers (a frame's HAnim chunk was skipped, leaving models without a hierarchy). `config/mirror_fixes.txt` makes the decomp's copies `extern` (the initializer stays on an unused static) and the four registries the decomp symbol list does not place are in `config/port_symbols_manual.txt`; the retail data image supplies the initial contents. The same split existed for the HAnim and user-data plugin offsets (`iGpffffb5d8`, `iGpffffb680`, file statics in rphanim.c and rpusrdat.c): they are now the retail globals. A model with no HAnim hierarchy reaches `mdlAttachSlotHierarchy` with a NULL hierarchy, which the PS2 store tolerates (low memory) and the host does not: guarded.
- Image <-> raster handlers (`src/render/p4gl_image.c`, stdFunc 2, 3, 6, 7, 9, 11, from `skyconv.c`): RGBTOPIXEL / PIXELTORGB (retail pixel packing; the 8888 alpha is scaled to 0..0x80), SETRASTERCONTEXT (the retail context globals), IMAGEFINDRASTERFORMAT (texture rasters are 8888, PAL8 or PAL4 by image depth with the size rounded up to a power of two, at most 1024; the retail 8888-or-1555 choice follows the GS memory budget), RASTERSETIMAGE (the image's indices and palette, or its colours converted to the raster format) and IMAGEGETRASTER, plus `RpSkySelectTrueTSClipper` / `TLClipper`. They reach the raster through the device handlers (15, 16, 23, 24) directly. A raster palette holds PS2 alpha (0..0x80, doubled by the upload), an RwImage palette 0..0xFF. `render_smoke` covers them.
- Skinned models. Characters (tc0N.RMD in title.bin, the player and enemy packs) carry the skin plugin (0x116: per-vertex bone indices and weights, skin-to-bone matrices) and an HAnim hierarchy; their geometry is not native (flag 0x01000000 clear), so morph target 0 holds the bind pose. `p4gl_atomic_draw` skins on the CPU (`skin_upload`): per bone `skinToBone[i] * node[i]`, the skin from geometry + the offset at 0x00886498, the hierarchy from atomic + the offset at 0x00886494, node matrices in world space (the game sets the update-LTMs flag, not local-space), vertices re-uploaded each draw and drawn with an identity object matrix. The weights carry the VU1 matrix-cache offsets in their low ten bits (`func_003b85e0`, port-owned in `src/platform/rw_skin_callees.c`), an error of 1e-4 that the weights are normalised over. The viewer steps the model once with `mdlAdvanceModelFromFlags` (the game's per-frame update evaluates the hierarchy); `title/title.bin#tc01.RMD` then draws in the pose of its first animation frame. A GPU skinning path can replace the per-draw upload later.
- Alignment. The retail EE newlib heap and matched RW checked wrappers return 16-byte-aligned payloads (see [CRT boundary](platform/crt_boundary.md)), and the many units that declare their own `RwMatrix` typedef get `aligned(16)` (`config/mirror_fixes.txt`): the native matrix code moves a matrix with `movaps`, which faulted on a 4-byte-aligned stack local in `mdlAdvanceModel`.
- Decomp 0-argument calls. Several decomp bodies call a function with no argument where retail leaves the value in `a0` (`func_003bfae0()` in mdlDrawAtomicIfVisible, `RpMatFXMaterialGetEffects()` in mdlSetupMatFXMaterial); on x86 the callee reads stack garbage. `config/mirror_fixes.txt` rows add the argument as they are found.
- Title frame (`p4pc_viewer SLUS_217.82 title out.png`). The viewer runs the game's own `titleDrawTaskView` state 7 (the settled title) against a hand-built task: a 0x90-byte work area (state words at +0 and +4 both 7, fade timer +0xC = 0x14), the kwln pool and Prim pool, `H_Cdvd_AcquireDirectory` on `title/title.bin`, `sdkSpritePackCreate` on the retail spec at 0x5E5220, then `shdSpriteEnsureLoaded` / `taskLoadSlotModels` until the sprites and the tc01..tc07 models are ready. The camera is the one `kwlnGetCamera` carries in retail (quaternion and translation from the title scene). `P4PC_TITLE_STAGE=1` draws only `titleDrawScene`, `2` only the corridor sprite. `titleTaskUpdate` itself is not run: its sound traps (`ACSSND_*`) are unported. Compared with PortTrace `timeline/title.png` (a late settled frame; the lens-flare phase `f10` and the silhouettes' entry animation are not driven) the frame differs by 1.9% mean absolute (of 255): the silhouettes (flat palette yellow, the white one at right), logo, PRESS ANY BUTTON and ATLUS match; what remains is mostly the lens-flare phase. `P4PC_TITLE_FRAME=n` plays frames 0..n (states 3, 5, 7 with the draw callback's own timers; the one frame at timer 0x55 that starts the title jingle is skipped, sound is unported). Frame 300 (settled) differs from the reference by 1.2%; the intro frames match the reference sequence but not its timing (frames 60/100/150/200 best match timeline frames 0454/0454/0464/0464), and the intro ghosts now match (after the sinf fix below; frames 60/150/255 best match timeline frames at 1.0/3.0/1.9%).
- The title composes in the framebuffer alpha channel, so GS blend semantics matter: (1) the alpha channel receives the SOURCE alpha (`glBlendFuncSeparate(src, dst, ONE, ZERO)`); the GS blends colour only, and later draws read it back as Ad (ALPHA 0x58 = Cs*Ad + Cd masks a flat colour to the silhouettes; 0x52 = Cd - Cs*Ad). (2) Im2D has a Z test: the vertex z is a GS Z in 0..65535 (larger is nearer) mapped to GL depth, ZTESTENABLE/ZWRITEENABLE as set (test off with write on always passes). The title clears Z with a z = 65535 rectangle; its white fade overlay at z = 65534 then fails the test, as on the GS. Tests that draw Im2D without a camera clear must clear depth. (3) The viewer's stand-in headlight (`p4gl_atomic_set_viewer_light`) is off in title mode: the silhouettes are vertex colour times material colour. (4) Paletted textures: a raster palette is the CLUT in GS memory order, and an 8-bit texel reads entry csm1(index) with index bits 3 and 4 swapped (CSM1); `paletteEntry` applies the swap (PAL4 has none), the game's TMX loader stores palettes in that order, and `p4gl_native.c` stores the native decoder's logical palette in it. Found from a PCSX2 GS dump of the title (`PortTrace/build/traces/timeline/title.gs.zst`; logo draw: ALPHA_1 0x44, TEX0 PSMT8 CLUT PSMCT32 CSM1, TFX MODULATE, TCC 1, TEXA 0, TEST ATE GEQUAL 0x7F AFAIL FB_ONLY, TEX1 MMAG/MMIN linear, XYOFFSET 1728/1824, FBA 0): the CLUT bytes in the dump equal the raster palette, so the letters' solid black comes from index 21 reading entry 13, not from any register. It also fixed the corridor's per-texel colour errors (title MAD 3.7% to 1.9%). (5) The viewer PNG is written opaque: the framebuffer alpha is blend state, not coverage.
- The atomic path sets its uniforms after `glUseProgram`: a draw following an Im2D draw used to set them on the Im2D program (GL_INVALID_OPERATION and a stale matrix).
- Half-pixel convention. The GS rasterizes with pixel centres at integer coordinates, GL at +0.5, so the same coordinates must land half a pixel further right and down: Im2D adds 0.5 to x and y in its vertex shader (the nearest sample then sits exactly on a texel boundary, so the fragment shader adds a 1/64 texel bias, which picks the GS's later texel) and `p4gl_camera_object_matrix` adds half a pixel in clip space. Flat rectangles keep their exact pixel coverage (a top-left tie includes the pixel); textured content and 3D edges move by half a pixel. Title MAD 1.88% to 1.27% (measured by sub-pixel shifting first: every region's best shift was half a pixel up-left of the reference content, i.e. ours was half a pixel too far up-left). `p4gl_camera_projection` (PTank; Im3D goes through the matrix) carries it as out[4], out[5] (clip x += z / width, clip y -= z / height).
- RtAnim record rows `func_003d59a0` and `func_003d5e90` are port-owned in `src/platform/rw_rtanim_record.c` (the decomp marker is only in the excluded `code1_003d_cw119.c`, so overrides/ never applied to the real build and the title intro trapped); `func_003d5e40_typed` (the decomp's second declaration of retail 003d5e40, float first) is a swapping wrapper in `rw_rtanim.c`: the generated alias jumped to `func_003d5e40` with the stack arguments in declared order and passed the float as the interpolator pointer.
- Unprototyped `sinf`: `code1_0012.c` and `unplaced_inferred_00121af0.c` declare `extern f32 sinf();` (and the latter calls it with a stray leading pointer argument, m2c's leftover a0). On x86 a call without a prototype promotes the float to double, so the host sinf read the double's low half as the float: the title's ghost fade got brightness -12 and byte-wrapped colours (greenish ghosts). `config/mirror_fixes.txt` gives both a prototype and drops the stray argument. Other unprototyped float-taking externs in the decomp would fail the same way.
- Field data layout. `field/pack/fMMM_NNN.p00` / `.p01` are packs in the title.bin format (0x100-byte NUL-padded name, then the file, entries on 0x80 boundaries). `.p01` holds `fMMM_NNN.rws` first, then the props (`mMMM_0xx.RMD`). The `.rws` is not an RpWorld: it is a texture dictionary (chunk 0x16) followed by RpClumps (chunk 0x10; f007_002: 3 clumps of 137, 66 and 14745 atomics, the last the map, 774 KB). `p4pc_viewer SLUS_217.82 "world:field/pack/f007_002.p01#f007_002.rws" out.png` reads that stream with the game's own RW stream functions and draws every clump through RpClumpRender from a fixed camera (`P4PC_WORLD_CAM` overrides it; the default is the PCSX2 Dojima living-room camera of PortTrace `fieldexp/live`: position (-317.4, 245.0, 421.1), right/up/at from the camera frame, fov 54.2 = view window 0.5118, near 100, far 7000). Status: the room geometry renders from the right place; MAD against `fieldexp/live/field.png` is 29% because (1) most map materials are untextured red in our render (the field's textures come from `field/fldmp_m*.tmx` mip tables that `k_fldMipmap.c` binds at runtime, not from the dictionary), (2) the map is the union of all rooms and the game draws only the visible set (per-atomic visibility the field task applies), (3) there is no lighting (materials are 0xb4 grey and vertex colours prelit: the GS treats 0x80 as 1.0), and (4) the character, shadows and UI are not drawn.
- Field mode (`p4pc_viewer SLUS_217.82 field:7:2 out.png`) boots the game's task machinery in the viewer (`kwlnInitSubsystems`, `kwlnRunFrame`, the boot chain's loader steps without its sequence change, `datInitDefaultGameState`, then `K_SeqFieldRecordStart`, which creates the field root task without an event reader) and runs frames. It reaches the field root state machine (the field pack, the model/field RMDs and field/*.tmx tables are requested) and stops in a crash of `func_003d6350` (an rtanim-area asm row) on the load worker thread. The boot chain's step callbacks are called with a NULL parent because the decomp declares them without arguments. `p4pc_kt_dump()` (callable from gdb) lists the kernel threads and what they wait on; the first blocker found was the CRI vsync handshake (`ADXM_WaitVsync`), fixed on main by the INTC handler chain.
- 128-bit returns. A decomp function returning `u_long128` returns it in registers on the PS2 and is called through tables and callbacks typed with other return types (the effect kinds' `*SetFirstQuad` slots in `D_00713480`). On i386 a `P4_Q128` (a struct) return is a hidden struct-return pointer that the callee pops, so such an indirect call jumped into the stack (field:7:2 died in `EffSpriteSetFirstQuad`). `tools/gen_port.py` now turns every `u_long128`-returning definition and prototype into `void` (and its `return expr;` into `return;`); no decomp caller uses the value.
- Field mode (`field:7:2`) now runs the game's own field root: sequence 9 starts `K_SeqFieldRecordStart`, and the process tree matches the PCSX2 reference's (field root, field camera controler, field event + rotate pc, SceneManager Task/Draw, field filter, collision controler ...). Fixed on the way: the `u_long128` return ABI (above), 16-byte aligned locals in the RW-matrix-calling overrides, the `sCurrScript` host static shadowing `iGpffffb510` (scrGetCmdTimer read NULL), `func_003d5830` (RtAnim free slot called without its argument), and `RpSkyTextureSetMipmapK` / `func_003f68a0` (port-owned in `p4gl_raster.c`, they record TEX1.K/L in the raster slot). With the map's path string fixed (`k_fldrc.c` subtracted 0x6208 from a symbol that already was the address, so the field's .rws path was garbage and nothing parsed) and the collision callbacks' float argument moved to the third host argument (`overrides/00392a50.c`, `00393900.c`, `00394870.c`: MIPS passes it in f12 whatever its position, the host order must match the callee `RpCollisionTriangleCallBack(intersection, triangle, distance, data)`), the game's own field task draws the whole Dojima living room, the protagonist and the location banner: MAD against `PortTrace/build/traces/field/field.png` 13.9% (29% for the `world:` viewer). Remaining differences: the room is darker than the reference (GS vertex colour 0x80 is 1.0: not applied to prelit/material colours), white patches on the tatami (textures), the banner text is garbled (font texture) and the date/time HUD is not drawn.
- Real boot path vs the viewer: the boot's frame capture (`P4PC_CAPTURE_FRAMES`) showed white boxes behind the logo, PRESS ANY BUTTON and ATLUS and lit, shaded silhouettes. The boxes were the PNG's alpha channel (the title composes in the framebuffer alpha; texels the viewer already wrote opaque were written with their GS blend alpha by the capture): the capture is opaque now, like the viewer's PNG. The shading was the atomic path's stand-in headlight, which defaulted on; it is off by default now (the title has no RpLights: silhouettes are vertex colour times material colour) and only `p4pc_viewer`'s single-RMD mode turns it on. Real boot frame 3300 against PortTrace `timeline/title.png`: 1.36% mean absolute difference (was 5.1%). The boot dies afterwards at title teardown (`titleLoaderTeardown` -> `mdlDispatchAnimTableRelease` -> `_rwSListDestroyArray` -> free: a double free of an animation list, the same family as the `D_0070B610` list the movie task frees).
- GS MODULATE and the field's brightness. A textured GS draw computes texel * vertex colour / 128 (the colour byte 0x80 is 1.0, 0xFF is 2.0); an untextured draw shows its colour as it is. The atomic fragment shader scaled textured colours by 1.0 only, so the Dojima living room came out 1.3 to 1.9 times darker than the PCSX2 frame (the GS dump's vertex colours of the room strips peak at 0x80). The scale is applied to the prelit colours only (`prelit_colour` in `p4gl_atomic.c`); scaling every textured colour in the shader washed out the unlit protagonist, whose default vertex colour is white. Dojima field MAD 10.04%. Dojima MAD against `fieldexp`/`field/field.png` 13.9% -> 10.7%; the per-region brightness ratio ours/reference went from 1.25-1.85 to 1.0-1.3. The white patches on the tatami are not fixed: the floor surfaces use the same dark wall raster in our render (debug colouring by raster), so the field's mipmap texture replacement (`FldRcMaterialSetMipmapTex`) is the next suspect.
- Title regression gate (`ctest title_mad`, label gl): `p4pc_viewer title` at `P4PC_TITLE_FRAME=400` against the PCSX2 `timeline/title.png`, mean absolute difference over RGB, failing above 2.0% (measured 1.27%). The reference is a raw 640x448 RGB file `refs/title.rgb` beside the retail image on each check host (`tools/png_to_rgb.py` makes it; never committed); the test skips (77) without P4PC_ISO, P4PC_ELF or that file. The viewer reads `P4PC_REF_RGB` / `P4PC_REF_MAX` for any mode. The number depends on the intro frame: the default `title` frame (state 7 right after the slide-in, no `P4PC_TITLE_FRAME`) is 1.69%, frame 300 is 1.52%, frame 400 is 1.27%; the 1.27% to 1.69% "regression" was a change of frame, not of code (the same commit f3d702f measures 1.69% default and 1.52% at frame 300 as today's main).
- Font glyphs (the garbled banner and message-window text): `frFontRasterUploadGlyph` (retail 00276420) wrote each 32x32 4-bit glyph into the glyph cache's PAL4 raster in the GS PSMT4 block layout (the address table D_0063bce0), the layout the PS2 uploads from a raster buffer as it is. The port's rasters keep PAL4 pixels linear (native textures are un-swizzled when read), so the glyphs read as horizontal streaks. `overrides/00276420.c` writes the pixels linear (16 bytes a row, even pixel in the low nibble); the palette copy is as retail. Checked on the field banner "Living Room" and on the NEW GAME difficulty message window of the real boot (cap_2400). Any other code that writes PAL4/PAL8 pixels through RwRasterLock in the PSMT4/8 block layout needs the same treatment.
- Render-to-texture cameras (`p4gl_camera.c`, `p4gl_raster.c`: `p4gl_raster_target_begin/end`). A camera whose `frameBuffer` (RwCamera +96) is a camera-texture raster (type 5; the field's 128x128 shadow camera) renders into a framebuffer of that raster's size (colour texture + depth renderbuffer, `p4gl_target_override` makes Im2D and the camera matrices use that size), and `RwCameraEndUpdate` copies the result, rows flipped (GL's first row is the bottom, the game's texture coordinates start at the top), into the texture the raster samples as. Without it the Im3D quad that projects the shadow onto the floor sampled an empty texture and drew an opaque white floor polygon over the tatami: Dojima field MAD 10.04% -> 5.68% (the "white tatami patches" were this quad, not a texture or mipmap problem; `FldRcMaterialSetMipmapTex` replaces only 4 overlay materials' textures with `field_mipmap` and is unrelated).
- `initBootChainStep` (decomp/src/Main/init.c) called its step callbacks with no argument; retail passes the task in a0 unchanged and `btlUnitStartLoad` forwards it as the parent of the load task, so the host call took a stack leftover as the parent and the field viewer crashed in `kwlnTaskAddChild` as soon as an unrelated function's stack frame changed size. `config/mirror_fixes.txt` now passes `arg0`.
- RenderWare lighting of atomics (`p4gl_atomic.c`: `gather_lights`, vertex shader `uLitMode`). Geometries with rpGEOMETRYLIGHT (0x20) and normals are lit: the vertex colour is the prelit colour (black without, not the 255/128 scaled prelit of unlit geometry) plus the scene's ambient light times the material's ambient coefficient (RpMaterial.surfaceProps +12), plus, for geometry without prelit colours, the two directional lights (the game's K_Clump_GetLightA/B/C: ambient 0.6 and two directionals, type/subtype/flags at +0..2, colour at +24, direction the light frame's at vector, a surface facing against it is lit) times the diffuse coefficient, clamped to 1, then times the material colour. Found by matching our transformed vertices (prelit, normal) with the GS dump's vertex colours of `field.gs.zst` at the same screen position: a room vertex's GS colour is 90 * min(1, prelit/255 + ~0.5) (90 = the b4 material colour * 128) and a prelit vertex shows no directional term (adding the directionals to prelit geometry raised the MAD from 3.87% to 5.78%, flipping their direction to 4.08%); the unlit-prelit protagonist, lit by ambient + both directionals, matches the reference to 1.02-1.07 per channel. Dojima field MAD 5.68% -> 3.87%.
- Camera-texture rasters are sampled clamped (`p4gl_texture_bind`): the shadow map's surroundings are not the picture; with repeat the floor quad's UVs beyond 0..1 drew a dark wedge on the tatami. The field date/time HUD (`calendarDrawBackdrop`, the `DayMonMain`/`DayMonDraw` tasks) is shown by the field script's `calendarSetMode(1)` command, which the viewer does not run; `field:F:R` calls it at frame 100. The HUD draws correctly (date from the new-game state: 04/10 Su, the reference is 04/11 M). Dojima field MAD 3.87% -> 2.80%.
- `ctest field_mad` (label gl, `tests/scene_mad.sh`): `p4pc_viewer field:7:2` against the PCSX2 `field.png`, failing above 3.2% (measures 2.80%); reference `refs/field.rgb` beside the retail image, like `refs/title.rgb`. `scene_mad.sh <viewer> <out.png> <mode> <ref name> <max MAD> [VAR=value...]` is the generic gate for further scenes.
- Light linkage (`gather_lights`): a light lights atomics only while it is linked into the world's light list (RpWorldAddLight / RpWorldRemoveLight leave the light's own `inWorld` pointers stale, so a light counts when its neighbours still point back at its link). The field's map pass adds the ambient light A and the directional B to the world (`k_fldResource.c` BeginDrawState) and C only for non-world resources; with that, directional light applies to prelit geometry as well and the Dojima room measures 2.43% (C included: 4.24%, no directionals: 2.80%). `overrides/003bb5b0.c` (camera path spline) now calls `func_003e4030` (RwV3dNormalize) with both arguments: it passed one and the second was a stack leftover, which crashed fields with a camera path (10:1, 8:2).
- SkyGL corpus scenes (tge's `p3fes-testdata`, private): each `.p2s` is a PCSX2 savestate; `tar -xf x.p2s eeMemory.bin` gives the EE RAM (file offset = address). Field id and room: word at `*(*(0x7D2510) + 0x38) + 0x18` (u16 field, u16 room); date index (days since April 1) u16 at 0x797B7A, time slot byte at 0x797B7C (0 early morning, 3 daytime, 4 after school, 5 evening); camera object pointer at 0x764B70 (frame ptr at +4, LTM at frame+80, view window at +104, near +128, far +132); lights A/B/C at `gp-17804/-17800/-17796`. `p4pc_viewer field:F:R` takes `P4PC_FIELD_DATE`, `P4PC_FIELD_SLOT` (set before the field starts; the calendar task re-latches) and `P4PC_FIELD_CAMERA` (20 floats: the LTM rows, view window, near, far; the field camera controller is stopped for the last 4 frames and the camera written before each). `docs/skygl_scenes.json` holds the values read from the savestates. With them the port renders the corpus's field scenes from the reference camera (the geometry lines up pixel for pixel); the NPCs, dialog windows, the evening/after-school colour filter tint and some effects are not reproduced yet.
- Post pass and alpha test (found from a GS dump of the corpus scene sky-black-tint, taken from its savestate with `p4hook --state ... --gs-at FRAME:BASE`, new in the PCSX2 hook of the private trace tooling): (1) the field's frame-buffer feedback "bloom" is a full-screen textured Im2D quad (TEX0 TCC = 0, ALPHA 0x48: Cs * As + Cd with the frame's own copy as the texture) drawn after the 3D scene. It was drawn but invisible: its source alpha multiplied the frame copy's alpha channel (which holds blend state, not a picture). Draws of a frame raster whose blend factor is the source alpha now read colour only (`uTextured == 2`). This is the "1.5x too dark" of the corpus field scenes; sky-black-tint 14.8% -> 5.4%. (2) Almost every world draw has TEST AFAIL = FB_ONLY (ATE GEQUAL 127): a fragment that fails the alpha test is not discarded, its colour is written and only its depth is not. The atomic path draws such materials twice (passing fragments with depth, failing fragments colour-only); before, failing fragments wrote depth and left ragged holes in alpha-tested leaf cards (sky-black-tint 5.4% -> 3.0%, save-butterfly-glow 15.5% -> 4.3%). The Dojima room got worse (2.43% -> 3.63%: a dark floor-level surface (`f007_002_06`) now overwrites the floor where the reference keeps it), cause not found; `field_mad` is 3.9% meanwhile. The dialog scenes (rain, riverbank-vignette, arcana-rankup-sparkles) read worse (13.7 / 21.1 / 29.0%) because the viewer runs without the event state and so draws the bloom the reference's event frames do not; the `scene_rain` gate is dropped.
- True-colour native textures (`p4gl_native.c`, `p4gl_nativetex_decode_ct32`): kind 2 data of depth 32 (PSMCT32) is the first IMAGE of the GIF stream in raster order, no palette (PS2 alpha doubled as for kind 0/1). The reader failed for them and returned a NULL texture dictionary, which crashed `FldResource_PollLoads` for `field:22:2` (the Shadow-world field of the shadow-haze scene); read failures now print their reason. `field:22:2` loads and renders (13.6% against shadow-haze: it is an event frame, see the note below).
- Not reproduced by the corpus-scene runs: (a) event frames (dialog windows, shadow-haze, rain, riverbank-vignette, arcana-rankup-sparkles): the event camera task rewrites the camera each frame, so `P4PC_FIELD_CAMERA` (which only stops the field camera controller) does not hold, and the bloom that the reference's event frames do not have is drawn; (b) the protagonist: the viewer spawns it at the field's entry 0, not where the savestate had it (NPCs do appear and stand where the game puts them: save-butterfly-glow shows the shopper and the girl as in the reference); (c) `field:40:0` (dungeon floors, band A: fields 0x28..0x3B room 0): `FldResource_QueueFieldResources` takes the floor's parts from the table the previous floor left (`D_007E8060`) unless bit 31 of the work object's flag word (`iGpffff9db0 + 0x88`) is set, which the retail load task sets right before `fieldSceneLoad` (k_fldEvent.c state 13); the viewer now sets it for band A, so the parts are created from the pack and the load gets past `FldResource_StepPartLoad` (it used to dereference a NULL resource). It then stops in `FldUnit_InstallResources` -> `mdlPrepareModel` on an all-zero model object (ids 0:0, no aux block; `mdlFreeAuxBlock` reads `+0x30C` NULL): a unit record with model id 0, which the floor's units, generated from the dungeon state (floor, seed) the viewer does not set, would not have. Open.
- Event frames by real play (the story walk, `tests/story_gate.sh`: New Game to the 04/15 evening, 233 captures one per 600 flips, `P4PC_CAPTURE_FRAMES`; retail references are PortTrace3's software-GS PNGs of the corpus states, `PortTrace/build/traces/refs/<name>_f<frame>.png`): the port draws every event of 04/11-04/15 (opening, Igor's dream, the dinners, the fog dream, the rainy walk to school, the Midnight Channel, the TV entrance with its tunnel, the Shadow world, the fights, the level-up). Measured against the retail references: shadow-haze (Other Yosuke, field 22:2) 2.06% mean absolute difference at flip 110400, the tunnel of tv-world-enter looks the same (nested rectangles, the chromatic fringe, the vignette; the phase differs, 36%), rain and velvet-windows have no port frame at the retail moment yet (the dialogue line of the reference is passed by the walker's Cross every 24 flips between two captures). Flip numbers move by several hundred between runs of the same script (host, capture list), so a gate needs a state-keyed capture, not a flip. Fixed: the Dojima living room on the 04/15 evening, reached by the continuous story walk (after the TV worlds and battles), drew a WHITE tatami floor. Its camera, lights, fog, textures and prelit colours were identical to the good 04/11 frame; the cause was the render-target table: `p4gl_raster.c` kept eight `RenderTarget` slots and `RASTERDESTROY` never released one, so after eight camera-texture rasters over a long play (every field's shadow camera, the battle and effect targets) the next shadow camera got no target, rendered into nothing, its raster had no texture, and the Im3D shadow quad drew untextured opaque white over the floor (a loaded save has created fewer rasters, so it was fine). `RASTERDESTROY` now deletes the target (framebuffers, depth renderbuffer, colour texture). Found by dumping the camera-texture raster's slot and the target table at the quad (`TGT`: eight keys of destroyed rasters, the live raster's texture 0).
- `P4PC_FIELD_PLAYER="x,y,z"` (`field:F:R`): the protagonist's world position, set through `FldFrameTask_SetPosition` on the player's frame task for the last 12 frames (the camera work block's word 0xA2 is the player resource; +0x220 its frame task). The position comes from a ray through the figure's feet pixel of the reference frame onto the floor (y = 0): for save-butterfly-glow `311.5,0,-2216.1` puts the model within 15 px of the reference's. It is part of the scene_save_butterfly_glow gate (the black floor polygon this once produced was the missing destination alpha test, below).
- Destination alpha test (TEST_1 DATE/DATM, bits 14/15; Im2D, `uDate`): the Dojima "regression" (a black floor polygon from the player's feet) was never FB_ONLY. The shadow caster is drawn into the 128x128 render target (the player in black, alpha 0x80), then six sprites fade the silhouette's alpha with TEST 0x3c803, i.e. DATE = 1, DATM = 1: they draw only where the target's alpha MSB is already set, so only on the silhouette. Without DATE the sprites painted the whole target (alpha 1 outside the silhouette) and the shadow quad drew the target's black over the floor; FB_ONLY only removed the depth writes that had hidden that quad. The Im2D shader now reads a copy of the current target on texture unit 1 for DATE draws. Dojima 3.63% -> **1.72%** (`field_mad` limit 2.0), save-butterfly-glow with `P4PC_FIELD_PLAYER` 3.68% (gate 4.2), sky-black-tint 3.06% (gate 3.5).
- near-fog (field:68:1, an event frame): the fog puffs are `effBlurFilterStepSlotsB`/`TickSlotsA/B` slots whose alpha is `alphaF * (c1 * (c2 - |phase|))` in soft doubles (`fptodp`, `func_0044b310` = fabs, `dpsub`, `func_0044e168`, `dptofp`). The decomp declares those `s32`/`s64`, so the host calls passed and read the wrong words and every puff had alpha 65 (the reference has 2..101). `config/mirror_fixes.txt` now declares the 64-bit bit pattern in the six units that use them, and `overrides/0044b310.c` takes and returns bits. near-fog 16.40% -> 15.78% (gate 16.1). What the reference has and the run does not: the screen filter passes drawn before the puffs (33 alpha-only 20-px strips of a 4-bit texture, a frame sprite blended by destination alpha, 64 vignette strips, 70 grain tiles) and the player; they come from the event script (this is the event frame of "???"), so they need the viewer event state (open item).
- Title sensitivity (root cause found): `title_mad` moved between 1.2166% and 2.0558% when code that never ran was added to `im2d_draw`. A build with clang `-ftrivial-auto-var-init=pattern` gave 1.2166, `=zero` 2.0558 (the CI-less switch is the CMake option `P4_AUTO_INIT`, `tools/p4pc_ci_run.sh` reads `P4PC_AUTO_INIT`), so a local was read before it was written: `sdkSpriteDraw` (decomp `sdkSpr.c`) leaves `source[].z` unwritten (retail: PS2 stack leftovers, which VU arithmetic cannot turn into NaN) and the host's x87 turns 0 * NaN into NaN, so the title's eight glow rings had NaN corners whenever the stack held a NaN (and were drawn at their real positions otherwise). valgrind (memcheck, `--track-origins=yes`, in the CI container after `apt-get install valgrind libc6-dbg:i386`) only named a second, harmless one (`H_Cdvd_ParseArchive` scans `entryPath` past the path). Both locals are now zeroed by `config/mirror_fixes.txt` rows. With the NaNs gone the rings drew over the letterbox bars, a second, real bug that the NaNs had hidden: the rings use TEST `0x33001` / `0x53001` (ATST NEVER, AFAIL = RGB_ONLY), a failing fragment writes colour but not alpha, and our Im2D wrote the alpha, so later rings saw it as destination alpha (blend factor Ad, ALPHA 0x58). Draws with ATST NEVER + RGB_ONLY now mask the alpha channel (`p4gl_frame_mask_apply`). title 1.2131% with `pattern`, `zero` and no init, and with the DATE copy inline in `im2d_draw` again. The last scenes shard of every land builds the viewer with `-ftrivial-auto-var-init=pattern` (`$build/pattern`) and runs `title_mad` and `field_mad` on it. The same sweep (viewer built with `pattern` and with `zero`, ten corpus scenes, frames compared) found one more: `mdlEffectSetUniformScale` (retail 004b13d0, a jump that hands its a0 and f12 on) was an empty call to `mdlEffectSetChildScales` (repaired by a pass-through row), and its caller `mdlEffectListSetUniformScale` declared the pair as (float, pointer) instead of (pointer, float): the fog scene crashed under `pattern`; `mirror_fixes` rows repair both. All corpus scenes now give identical frames under `pattern` and `zero`.
- Weather (`P4PC_FIELD_WEATHER=n`, `field:F:R`): the calendar state object (`iGpffffb1dc`) holds the day's weather at +8 (`calendarPushValue`; `calGetDayEntryByPhase` returns it while the state's days and period match, else the ROM day table's value). The day's weather is rolled at run time, so the viewer's new-game default (the ROM table) differs from a savestate's: date 15 is cloudy (2) in riverbank-vignette and arcana-rankup-sparkles but the table says rain, which is why those frames were 21.1% and 29.0% off. Values read from each corpus savestate (`docs/skygl_scenes.json`, key `weather`: 0 clear, 1 rain, 2 cloudy); the viewer writes the word every frame. riverbank-vignette 21.1% -> 8.8%, arcana-rankup-sparkles 29.0% -> 13.7% (the remaining difference is the event: dialog window and bust-up, sparkles, the characters' poses). Two new gates, `scene_riverbank_vignette` (9.4) and `scene_arcana_rankup_sparkles` (14.2), with refs `refs/p4-riverbank-vignette.rgb` and `refs/p4-arcana-rankup-sparkles.rgb` (`tools/png_to_rgb.py` of the corpus frame; on the three hosts beside the others).
- Dungeon floors (`P4PC_FIELD_STEP="index,param"`, e.g. `6,1023` as in the corpus dungeon savestates; with `field:40:0`): the game enters a floor through the step task (`FldStepTaskCreate`, sequence `K_SeqFldStepStart`), not through the field sequence: its table entry for `index` names the field and room and it creates the field root itself with the retail flags. Without a step task `FldStepIndex()` is 0, so every entry of `FldUnit_RequestSceneResource`'s eight-way loop whose stored value (`D_0079B48C[i + 1]`, 0 in a new game) equals it matches, the two-slot arrays overflow (`kAssertFail` returns on the host, `D_007643C0[2]` is `D_007643C8[0]`) and `FldUnit_InstallResources` prepares a zeroed model: that was the `field:40:0` crash. With the step task the load runs to the field root's state 12, where the dungeon entry script `dng_init_00` (a TV-world cut-in with "Presented by ATLUS") holds it; the corpus frames are from after it. `P4PC_FIELD_TASKS=1` dumps the task tree at the end: the script task `dng_init_00`, `cutinDrawProc` and a movie (`sdkSfdPlay`). With `P4PC_VIRTUAL_CLOCK=1` the movie runs through (the "Presented by ATLUS" TV, the camcorder footage of the first TV-world entry, then a scripted fight: a Yosuke tutorial battle with the whole party in the dungeon), so it is the story's first-visit event, which a new game plays; the corpus savestates are from a later story point (their flags have it done). The dungeon scenes need that story state (a retail save loaded into the port), not a viewer switch.
- Battle scene gate `scene_battle_floor_a`: the corpus battle-floor-a savestate is encounter 512 (Shadow Yosuke, stage f222_001): the encounter record is the one whose field/bgm words equal the packet at battle-context +0xC70 (+0x20/+0x22). `P4PC_START_BATTLE=512` with cross presses every 60 flips from 360 to 1200 (`P4PC_AUTOPRESS_FRAMES=1`) leaves the command menu up from about flip 1300; the frame at flip 1400 (virtual clock) differs 15.65% from the capture (`tests/battle_mad.sh`, `P4PC_CAPTURE_REF` in `p4gl_context.c`; limit 16.5, `refs/p4-battle-floor-a.rgb` beside the others). The same camera, TV frame, menu and hero; what differs is the film grain of the capture, the boss's pose (animation phase) and a few percent of brightness (banners ours +7%, the floor ours darker). The fight itself had stalled at state 8 before: `RtQuatRotate` (func_003dc740) multiplied the half angle with the host's round-to-nearest where the EE truncates, so a 180 degree turn got w = -4.4e-8 instead of the retail +1.9e-7, and `btlUnitRotateTowardUnitPacketUpdate` waited for a turn of 360 degrees minus a hair (found by reading the unit quaternions of the savestate against ours).
- Event state, what is known (open): the event frames run a script procedure of the day scheduler. The savestate's script work list (head `0x764608`, `ScrScriptWork`: label at +0, header +0x110, procedure index +0x12C, next +0x150) holds: rain `sdl04_12_AM_A` (index 89), riverbank-vignette `sdl04_16_AM_A` (113), arcana-rankup-sparkles `sdl04_16_AM_A` plus `155_commu_open` (a community script, header `0x1771c00`), shadow-haze `sdl04_15_PM_C` (111), velvet-windows-blurry `upstair_myroom` (6, a field script); near-fog has none (its "???" box belongs to an event started elsewhere). `sdl<MM>_<DD>_<AM_A|AM_B|PM_A..PM_D>` procedures live in `scheduler/scheduler_<MM>.bf` (DATA.CVM) and are started by the `sdlScheduler` task (`cldSchedulerStart`, after `cldSchedulerInit` allocates its buffer; `cldScheduler.c`, which picks the label from the date index and the time slot). Tried: `cldSchedulerInit(); cldSchedulerStart();` after the field sequence starts, with the corpus story flags (the 704-byte flag array at `0x7FB1CC` copied from the savestate): the procedure starts and ends within ~50 frames (`start <sdl04_12_AM_A>` / `end <sdl04_12_AM_A>`), then AM_B ... PM_D each start and end at once, and the field aborts (state -1, black frame, MAD 35%): the scheduler is not enough on its own, the condition that makes `sdl04_12_AM_A` stay (and show Nanako's window) lives in other state (the story/state tables, the field entry the day script expects), and the flags were identical across all corpus scenes. Not landed.
- Fog-dream tutorial battle (encounter 532, field 222; gate `scene_fog_battle`, `refs/p4-fogbattle.rgb` is the retail GS dump's software-GS screenshot stretched to 640x448): the first port frames looked wrong (8.38% from the retail frame: no pale floor and horizon band, Yu not washed out by the fog, the command bar running past the menu circle, noise over the whole picture). Found by comparing draw lists: `tools/gs_dump.py <dump.gs.zst>` prints a retail GS dump's draws (batches of the same state, bounding box, Z range, vertex colour, texture and its format, blend, TEST, FBMSK) and `P4PC_DRAW_LOG=<flip>` prints the port's (one line per GL draw; `P4PC_DRAW_UPTO=n` / `P4PC_DRAW_SKIP=k` leave draws out to see which one makes a pixel). The retail list opens with a 146 triangle strip of the floor (fogged) and the port's started at the haze strips: the stage is a **BSP world**, rendered by `RwCameraForAllSectorsInFrustum` -> `_rpSectorDefaultRenderCallBack` -> the PS2All node, which the port's atomic path rejected (`node input is object type 255`: a RpWorldSector has type -1 where an RpAtomic has 1; the message printed about twice per frame and was taken for noise). A sector keeps raw vertices, normals, texCoords[0] and preLitLum like a geometry, and its RpMeshHeader (strips, flag 1) groups them by material (RpMesh: indices, count, RpMaterial); `p4gl_atomic_draw` now draws a sector from those (strips become triangles, the odd ones flipped, degenerates dropped; world space, no frame). The second fix: the TV outline and a full-screen quad are drawn with ATE NEVER and **AFAIL = ZB_ONLY** (TEST 0x2001 | aref << 4): every fragment fails and only Z is written. The port discarded them, so the Z that the later ZTST GEQUAL draws test against (the additive noise, the menu's black bar, drawn at a nearer Z than the circle) was missing: the noise covered the whole picture and the bar ran across the screen. The draw is now Z-only (colour mask off, no discard: `zb_only_never` in `p4gl_state.c`). 8.38% -> 3.71%. What remains differs by animation phase (the TV outline's shape, the COMMAND ring's angle) and the retail screenshot's stretch. The encounter-512 stage is the same kind of world: `scene_battle_floor_a` 15.17% -> 14.29% (limit 15.4 -> 14.4); near-fog (a field) is unchanged at 15.47%, its frame lacks the player and the dialogue box and has no dark noisy border that the retail frame shows (the cause of the border is not investigated).

## 10. OpenGL ES 3.0 path (one renderer source for Windows, Linux, macOS, Android and iOS)

`src/render` runs on desktop OpenGL 3.3 core and on the OpenGL ES 3.0 subset of it from the same source. The GL calls and GLSL are the common subset; the few places that differ follow the context that was actually created (`p4gl_gl_is_es`, `p4gl_gl_border_native`). OpenGL ES is the default (`-DP4PC_GLES=ON`); desktop 3.3 core stays a supported path with its own gates (`-DP4PC_GLES=OFF`, or `P4PC_GL_API=gl` for one run; ctests `render_smoke_gl33` and `title_mad_gl33`). ES is the right default because it is what Android has natively, what Mesa gives every Linux machine, what ANGLE gives macOS (Metal), Windows (D3D11, Vulkan) and, with extra plumbing, iOS (§10.4), and because the audit below found nothing the renderer needs that ES 3.0 lacks except `GL_CLAMP_TO_BORDER`, which a shader emulates (and the title frame uses it).

### 10.1 Selecting a path

| Knob | Effect |
|---|---|
| CMake `-DP4PC_GLES=ON\|OFF` | the default API of the build (`P4PC_GL_API_ES_DEFAULT`, `src/render/p4gl_api.h`). ON. If the default ES context cannot be created (a Windows driver without the ES profile, no EGL library) `p4gl_context_open` falls back to desktop 3.3 core and says so; an explicit request below fails instead. Without any context (SDL's offscreen or dummy video driver on Windows has no GL) the game exits 1 at the device's OPEN with the reason (`p4gl_context_open_device`, `tests/gl_open_failure.sh`) |
| `P4PC_GL_API=gl\|gles` | this run's API, whatever the build default is |
| `P4PC_ANGLE=1\|d3d11\|vulkan\|gl\|metal\|swiftshader` | run the ES path on ANGLE (`libEGL` and `libGLESv2` found by the loader, on Windows next to the exe): sets `SDL_HINT_OPENGL_ES_DRIVER=1` (SDL then uses EGL instead of the platform's own ES, which on Windows is WGL's ES profile, i.e. the vendor driver) and, with a name, `ANGLE_DEFAULT_PLATFORM`. Implies `gles` |
| `P4PC_GLES_BORDER=shader\|native` | force the texture-border emulation, or the hardware mode, on any context (the default asks the context: desktop, ES 3.2, `GL_EXT_texture_border_clamp` and `GL_OES_texture_border_clamp` are native) |
| `P4PC_PRESENT=blit` | render into a 640x448 target and blit it to the window at present, so the window's framebuffer is only written (§10.4 says why Metal and EGL want that). Captures read the target |
| `P4PC_GLERR=1` | `glGetError` after every draw, clear, copy, blit and upload, printed with the call (first 40): for bring-up on a new GL implementation |

Every run prints one line with the context it got: `p4gl: <GL_VERSION>, <GL_RENDERER>, <GL_VENDOR> (OpenGL ES|desktop OpenGL, texture border hardware|shader)`. SDL attributes are set before the window exists (`pad_sdl.c` calls `p4gl_api_set_attributes` ahead of `SDL_CreateWindow`): the window's pixel format or EGL config follows them, and the ES path asks for RGBA8888 with a 24-bit depth buffer, because the GS blend state lives in the frame buffer's alpha channel and `glCopyTexSubImage2D` / `glBlitFramebuffer` between the window and the RGBA8 textures need matching formats in ES.

Shaders are one body per stage; `p4gl_gl_compile_shader` prepends `#version 330 core`, or `#version 300 es` with `precision highp float/int/sampler2D` (highp is always available in an ES 3.0 fragment shader), and for fragment stages `p4SampleBorder`. All seven stage sources (Im2D vertex, Im2D fragment, the Im2D fragment with the DATE test, Im3D vertex and fragment, atomic vertex and fragment) were validated as GLSL 3.30 core and GLSL ES 3.00 with glslang before the first run (it found one `#version` line left in a body). There are four programs: Im2D plain, Im2D with DATE, Im3D, atomic.

### 10.2 Audit: every GL feature the renderer uses against ES 3.0

The renderer's whole GL surface is `src/render/p4gl_gl.h` (about 70 entry points, loaded with `SDL_GL_GetProcAddress`). Each row is a feature, where it is used, what ES 3.0 offers and what the port does. "Native" means the ES 3.0 core call is the same call.

| Feature | Used by | ES 3.0 | Port |
|---|---|---|---|
| Textures: RGBA8 / `GL_RGBA` / `GL_UNSIGNED_BYTE`, `glTexImage2D`, `glTexSubImage2D`, `GL_UNPACK_ALIGNMENT` | rasters, native TXD/RMD textures, movie frames, frame copies, targets (`p4gl_raster.c`, `p4gl_nativetex.c`) | native (RGBA8 is a required texture and render format) | none. Paletted PS2 textures are expanded to RGBA8 on the CPU, so no 1/2-channel, integer, float, compressed or sRGB texture is used |
| Texture size | GS textures are padded to power of two | NPOT is native in ES 3.0 | none (the padding stays: the game's UVs are relative to the padded size) |
| Mip levels | the device keeps one level, mip filters fall back to linear | native | none; no `glGenerateMipmap`, no `GL_TEXTURE_MAX_LEVEL`, every texture sets its min filter (a `NEAREST_MIPMAP_LINEAR` default would be incomplete in ES too) |
| Wrap `REPEAT`, `MIRRORED_REPEAT`, `CLAMP_TO_EDGE` | RwTextureAddress, GS CLAMP_1 | native | none |
| Wrap `CLAMP_TO_BORDER` (RwTextureAddress mode 4; the title frame uses it) | `p4gl_texture_bind` | not in ES 3.0: ES 3.2, `GL_EXT_texture_border_clamp` or `GL_OES_texture_border_clamp` | hardware when the context offers it, else the texture is clamped to the edge and the fragment shader does the border: `p4SampleBorder` fetches the texels itself under each axis's wrap mode (repeat, mirror, clamp, transparent-black border) with nearest or bilinear weights. ANGLE's Metal backend does not implement the extension (`textureBorderClampOES = false  // not implemented yet`, `DisplayMtl.mm`), so on Apple the emulation is the path. `render_smoke` compares up to 1369 probes per case with a model of the sampler, for nearest and linear filtering, border with clamp, wrap and mirror on the other axis |
| Sampler state | `glTexParameteri` WRAP_S/T, MIN/MAG | native (no sampler objects used) | none |
| Filters `NEAREST`, `LINEAR` | RwTextureFilter | native | none |
| Texture copy `glCopyTexSubImage2D` from the framebuffer | DATE copy (TEST_1 DATE/DATM, `im2d_dest_alpha`) | native, but the source must have a format compatible with the destination: RGBA8 from an RGBA8 window | the ES window is requested as RGBA8888; with `P4PC_PRESENT=blit` the source is the renderer's own RGBA8 target |
| `glBlitFramebuffer` with the destination rectangle flipped | frame rasters (the frame copy, display raster at present), camera-texture targets | native in ES 3.0: mirrored rectangles are allowed, no scaling or format conversion between RGBA8 and RGBA8 | none |
| Reading the default framebuffer | frame feedback, DATE, captures | defined, but after a swap its contents are undefined (EGL `EGL_BUFFER_DESTROYED` is a legal default; flip-model swap chains and Metal drawables do not keep them) and a Metal drawable is not sampleable unless its layer is configured for it | `P4PC_PRESENT=blit` keeps the window write-only; the offscreen target (`P4PC_OFFSCREEN=1`, `p4gl_context.c`) has been the same idea for captures |
| `glReadPixels` `GL_RGBA`/`GL_UNSIGNED_BYTE` | captures, `p4gl_readback_rgba` | native (the one format ES guarantees for RGBA8) | none |
| Depth: `GL_DEPTH_COMPONENT24` renderbuffer, `DEPTH_TEST`, `LEQUAL`/`ALWAYS`, depth mask | targets, the window | native (24-bit depth is a required renderbuffer format; the EGL config asks for 24) | none. Metal has no 24-bit depth on iOS (D24S8 only on macOS, and not on AMD), so ANGLE has to use a 32-bit float format there: GS Z reaches GL as `(1 - z/65535) * 0.999`, 65536 values that any of these formats holds, but 3D perspective depth tie-breaks (z-fighting) can differ |
| Blending: `glBlendFuncSeparate`, `glBlendEquation` (ADD, REVERSE_SUBTRACT), `glBlendColor` with `CONSTANT_ALPHA` | the GS ALPHA register (`p4gl_gs_alpha_apply`) | native. ES forbids mixing `CONSTANT_COLOR` with `CONSTANT_ALPHA` factors in one blend: only alpha constants are used | none. Dual-source blending is not used: the one combination that would need it (the over-bright `(Cs-Cd)*FIX+Cs`) already falls back to source-alpha blending with a one-time warning on both paths. An exact version could read the destination in the shader the way DATE does (copy, then texel fetch), which is portable; `EXT_blend_func_extended` (ANGLE's Metal backend has it) and framebuffer fetch (Apple GPUs only) are not |
| Alpha test, AFAIL modes (FB_ONLY, RGB_ONLY, ZB_ONLY), alpha ref | `discard` in the fragment shaders; colour mask and depth mask for the failure modes | native (no fixed-function alpha test in either profile) | interpolation is not exact everywhere: AMD's radeonsi (Renoir, Mesa 25.0.7) gives a vertex alpha of 1.0 interpolated across a quad a hair below 1.0, which failed GEQUAL 0x80 (an alpha-tested glyph stem vanished) and kept AA1 lines at full alpha. The test and AA1 read the alpha through `p4GsAlpha` (`p4gl_gl.c`), which takes a value within 1/256 of a whole GS alpha as that alpha |
| Destination alpha test (DATE/DATM) | Im2D program 2: `texelFetch(uDest, ivec2(gl_FragCoord.xy), 0).a >= 0.99` on a copy of the target | `texelFetch` and `gl_FragCoord` are ES 3.0 core (origin lower left in both) | none |
| Frame-feedback quad, +0.5 px rule | Im2D vertex shader adds 0.5 px, fragment shader adds 1/64 texel to a nearest sample; `p4gl_camera_object_matrix` adds half a pixel in clip space | shader arithmetic only, same in both | none. Which texel a nearest sample on an exact texel boundary picks is the one thing the GPU decides: 78 pixels of the title differ by up to 2 levels between hardware and emulated border on Mesa, and single pixels differ between GPUs (§10.3) |
| Primitives: `GL_TRIANGLES`, `TRIANGLE_STRIP`, `TRIANGLE_FAN`, `LINES`, `LINE_STRIP` | Im2D, Im3D, atomic, PTank | native. No `glLineWidth` (width 1 is all ES guarantees), no points, no primitive restart, no `glPolygonMode` | none. Metal has no fan primitive, so ANGLE has to rewrite the index list for fans; lines are rasterized by each API's own rule |
| Index types `GL_UNSIGNED_SHORT`, `GL_UNSIGNED_INT` | Im2D (u16), atomic and Im3D (u32) | native (u32 indices are core in ES 3.0) | none |
| Buffers and vertex arrays: `glBufferData` STREAM/STATIC_DRAW per draw, VAO, float attributes, `layout(location=)` vertex inputs | all draw paths | native (VAO and `layout(location)` are ES 3.0 core; client-side arrays are not used) | none. One `glBufferData` per Im2D draw is cheap on desktop drivers and may not be on Metal: profile there (§10.6) |
| Framebuffer objects, `glFramebufferTexture2D`, renderbuffers, `glCheckFramebufferStatus` | offscreen target, camera textures, copies | native (RGBA8 texture plus D24 renderbuffer is a required-complete combination) | none |
| `glClear` colour and depth, `glViewport`, `glColorMask`, `glCullFace`, `glFrontFace` | camera begin, draws | native | none |
| Integer textures, `usampler`, UBOs, instancing, transform feedback, queries, sync objects, multisampling, `glGetTexImage`, `glDrawBuffer`, `glPolygonMode`, `GL_FRAMEBUFFER_SRGB`, `glDepthRange`, `glClearDepth` | not used | n/a | nothing to port |
| Extension query | `p4gl_gl.c` context probe | `glGetStringi(GL_EXTENSIONS, i)` (ES 3.0 core; the single-string query is removed from desktop core) | `glGetStringi` on both |
| `glGetError` | readback, `P4PC_GLERR` | native | none |

The GS rules are unchanged by the port because none of them is a GL feature: DATE, AFAIL, the feedback quad and the +0.5 px rule are shader arithmetic plus blend, depth and colour-mask state, and the frames below confirm it.

### 10.3 Verification

Numbers are mean absolute difference (percent, over RGB) against the PCSX2 references (`refs/*.rgb` beside the retail image), at `P4PC_VIRTUAL_CLOCK=1`. Tools: `tools/remote_scenes.sh` (re-runs a lane's scene gates on a host with extra environment and keeps the frames), `tools/local_scenes.py` (the same gates on this Windows machine's GPU), `tools/png_diff.py` (frame against frame).

**Linux i386, Mesa 25.0.7 llvmpipe (the check hosts' container): the 7 viewer gates and the 6 game-boot gates.**

| Path | context | result |
|---|---|---|
| desktop 3.3 core | `4.5 (Core Profile) Mesa`, border hardware | the baseline: title 1.1365, field 1.7773, sky-black-tint 3.0260, near-fog 15.4712, save-butterfly-glow 3.3725, riverbank-vignette 8.4630, arcana-rankup-sparkles 12.9090; battle_menu, scene_battle_floor_a (15.1474), camp_menu, save_load, retail_save_load and newgame_to_field (0.7835) pass |
| ES, Mesa's ES 3.2 | `OpenGL ES 3.2 Mesa`, border hardware | the same MAD to four digits on all 7 viewer gates and **0 differing pixels in all 7 frames**; the 6 game-boot gates pass with the same battle (15.1474) and new-game (0.7835) numbers |
| ES held to 3.0 (`MESA_GLES_VERSION_OVERRIDE=3.0`), border in the shader, `P4PC_GLERR=1` | `OpenGL ES 3.0 Mesa`, border shader | no GL error in any scene; 6 frames identical to the desktop frames; the title (the scene that uses border addressing) has 78 pixels off by at most 2 levels, MAD 0.0001% (the hardware filter weighs in 8 bits, the shader in floats) |
| ES under a real X11 window (Xvfb, SDL's x11 driver, EGL) | `OpenGL ES 3.2 Mesa` | context created and the menu harness runs; the window needed no change |

**Windows, native i686 build on an RTX 4080 Laptop GPU (NVIDIA 610.88): the 7 viewer gates.** The limits were set on llvmpipe, so a real GPU's own rasterization moves several scenes past them whatever the GL path (the desktop-GL baseline fails three); what the ES paths must do is track that baseline.

| Gate | desktop GL (WGL) | ES 3.2 (WGL, NVIDIA's ES) | ANGLE D3D11 | ANGLE Vulkan (NVIDIA) | ANGLE Vulkan (SwiftShader) | limit |
|---|---|---|---|---|---|---|
| title_mad | 1.2083 | 1.2083 | 1.2083 | 1.2083 | 1.2136 | 1.25 |
| field_mad | 1.9968 | 1.9968 | 1.7156 | 1.7960 | 2.9082 | 1.9 |
| scene_sky_black_tint | 3.4917 | 3.4917 | 3.0467 | 3.4882 | 3.3841 | 3.2 |
| scene_near_fog | 15.6855 | 15.6855 | 15.6930 | 15.7768 | 15.8859 | 15.8 |
| scene_save_butterfly_glow | 3.8703 | 3.8703 | 3.6565 | 3.8702 | 6.1838 | 3.7 |
| scene_riverbank_vignette | 8.5619 | 8.5619 | 8.8226 | 8.5607 | 21.8160 | 8.8 |
| scene_arcana_rankup_sparkles | 13.2075 | 13.2081 | 13.6213 | 13.6155 | 26.2132 | 13.4 |

The game-boot gates (battle_menu, scene_battle_floor_a, camp_menu, save_load, newgame_to_field; retail_save_load skips here without saves beside the ELF) pass on desktop GL, ES (WGL), ANGLE D3D11 and ANGLE Vulkan on this machine (the shell here has no `grep`, so their MAD lines are not captured; the exit status is the gate). ANGLE is 2.1.28027 (the DLLs of Electron 43.8.0, ia32: `libEGL.dll` 0.39 MB, `libGLESv2.dll` 7.2 MB, D3D11, Vulkan and SwiftShader in one build). Reading the table:
- ES 3.2 on NVIDIA's driver is the desktop frame: 5 of 7 frames are bit-identical. The desktop path itself is not deterministic run to run on this GPU (butterfly 3.8973 on the first run, 3.8703 on two repeats; arcana one pixel between repeats), and ES matches the repeats on butterfly; arcana differs from the desktop frame in 28518 pixels by up to 68 levels.
- ANGLE on D3D11 and on NVIDIA's Vulkan is within 0.5 points of the desktop MADs (D3D11 is better on field, sky-black-tint and butterfly by 0.2 to 0.45 and worse on riverbank and arcana by 0.26 and 0.41; Vulkan equals desktop on four gates and is worse by 0.09 to 0.41 on near-fog and arcana). Against the desktop frame the difference is MAD 0.007 to 0.54%, in single-pixel nearest-sample ties of textured quads and in glyph edges (`png_diff.py --write-diff`), not in missing geometry. Title is bit-identical on both.
- SwiftShader is a CPU rasterizer (the title takes 61 s against 2 s); its larger MADs on riverbank, arcana and butterfly are real frame differences, not investigated, and it is not a shipping target.
- ANGLE's OpenGL backend (`ANGLE_DEFAULT_PLATFORM=gl`, here ES-over-ES on NVIDIA's driver) draws the 3D scenes but not the Im2D sprites and glyphs (title white, no HUD text) with no GL error; ANGLE's GL backend is not a configuration Windows ships and it was not debugged further.

`render_smoke` (every Im2D, Im3D, PTank and border case) passes on desktop GL, ES (WGL), ANGLE D3D11, Vulkan and SwiftShader, each also with `P4PC_GLES_BORDER=shader`; no scene gave a GL error under `P4PC_GLERR=1` on ANGLE D3D11 and Vulkan (title, field:7:2, field:68:1). One finding from the border model: Mesa 25.0.7's llvmpipe (the check hosts) returns the border for linear samples of a mirrored axis combined with a border axis where the sampler model, NVIDIA, ANGLE and the shader agree on texels (Mesa 26.2.4 matches the model), so that one case is checked against the emulation only (`render_smoke`, `emulationOnly`).

`P4PC_PRESENT=blit` (the 640x448 target blitted to the window) gives the same battle frame at flip 419 as the plain window path: 78 pixels differ (at most 5 levels) on desktop GL and 209 on ANGLE D3D11, the size of the run-to-run noise of that frame.

**Windows on Mesa llvmpipe 26.2.4** (mesa-dist-win x86: `opengl32.dll` and `libgallium_wgl.dll` beside the exe, `GALLIUM_DRIVER=llvmpipe`), the same 7 viewer gates: desktop GL (WGL) title 1.2131, field 1.7209, sky-black-tint 3.0330, near-fog 15.6940, save-butterfly-glow 3.7125, riverbank-vignette 8.8404, arcana-rankup-sparkles 13.6614; Mesa's ES 3.2 (WGL) the same on title, field, sky-black-tint, near-fog and arcana, 3.6853 and 8.8353 on butterfly and riverbank; ANGLE's OpenGL backend over Mesa's GL (`P4PC_ANGLE=gl`) title 1.2131, field 1.7209, sky-black-tint 3.0312, near-fog 15.7343, then 3.7125, 8.8404 and 13.6614. So ANGLE's GL backend is sound on Mesa; the white title above is that backend on NVIDIA's ES driver. The limits move with the rasterizer, not with the API: this llvmpipe is 0.4 to 0.75 points above the check hosts' 25.0.7 on riverbank and arcana, and the desktop path fails those two limits and butterfly's here too. ANGLE's Vulkan backend on Mesa's lavapipe (x86 build) crashed inside `vulkan_lvp.dll` on the title and the game-boot gates (field 2.9100, butterfly 6.1882 where it ran): that is the 32-bit lavapipe, not used.

**Not covered here:** ANGLE on Linux (the port is an i386 build, Electron ships no 32-bit Linux ANGLE and an i386 build of ANGLE from source was not attempted), Android devices, and every Apple platform (§10.6).

### 10.4 ANGLE on iOS and macOS: cost and risk

**What it is.** ANGLE (Google, BSD-3) implements OpenGL ES on D3D11, Vulkan, desktop GL and Metal. Its Metal backend is what WebKit uses for WebGL on macOS and iOS ([WebKit's ANGLE for WebGL page](https://trac.webkit.org/wiki/AngleforWebGL)). Here it would give the ES path on macOS and iOS with Metal underneath.

**Binary size (measured, `EdgeFirstAI/angle-package` v2.1.28252, ANGLE commit c053bf8579, ES 3.0):** `libGLESv2` is 12.1 MB for iOS arm64 (3.8 MB zipped), 12.3 MB for the iOS simulator, 15.3 MB for macOS arm64 (4.8 MB zipped); the whole xcframework zip is 12.5 MB. The Windows D3D11-only x64 build of the same commit is 5.6 MB plus a 0.5 MB `libEGL`; Electron's all-backend ia32 `libGLESv2` is 7.2 MB.

**Metal translation limits (from ANGLE's source, `DisplayMtl.mm` and `mtl_common.h`):**
- ES 3.0 is the ceiling (`kMaxSupportedGLVersion = 3.0`), and it needs an Apple GPU family 4 (A11, 2017) or a Mac family 1 GPU; older devices get ES 2.0, which this renderer cannot use (no VAOs, no `texelFetch`). The simulator is ES 3.0.
- No `GL_CLAMP_TO_BORDER` (`textureBorderClampOES = false`): the shader emulation of §10.2 is the path, forced by the capability check, not a choice. `P4PC_GLES_BORDER=shader` plus `MESA_GLES_VERSION_OVERRIDE=3.0` reproduces that path on Mesa (ctest `render_smoke_es30`).
- Depth: iOS gets no 24-bit depth format (`supportsDepth24Stencil8PixelFormat` is false there; macOS has D24S8 except on AMD), so the 24-bit depth ES asks for has to be a 32-bit float format on Metal.
- Shaders go through ANGLE's translator to Metal Shading Language and are compiled when the program is linked (`ProgramMtl::compileMslShaderLibs`). The renderer has four programs, so the first-use stall is small; Metal's shader compiler is a system service and runtime compilation of MSL source is allowed on iOS.
- Metal has no triangle-fan primitive; the fans of Im2D/Im3D cost an index rewrite per draw.
- Reading the window: the default framebuffer is a `CAMetalLayer` drawable. Frame feedback and DATE read it (`glCopyTexSubImage2D`, `glBlitFramebuffer`), and a drawable that must stay readable is slower than a write-only one; `P4PC_PRESENT=blit` renders into the port's own target and blits once per frame, which is also what an SDL GPU renderer would have to do.

**macOS.** SDL3 has an EGL path in its Cocoa driver for ANGLE (`Cocoa_GLES_*`, used when `SDL_HINT_OPENGL_ES_DRIVER=1`, i.e. `P4PC_ANGLE=metal` here), so the ES path needs only `libEGL.dylib` and `libGLESv2.dylib` shipped in the app bundle. Apple's own OpenGL (4.1 core at most, deprecated since macOS 10.14) still runs the desktop 3.3 path, which stays a fallback until Apple removes it.

**iOS.** SDL3's UIKit driver has no EGL code (checked in SDL main: `SDL_uikitopengles.m` mentions no EGL), so `SDL_GL_CONTEXT_PROFILE_ES` there gives Apple's native EAGL ES 3.0 context: it works and runs on Apple's own GL-on-Metal layer, but OpenGL ES is deprecated since iOS 12 and Apple can remove it. Using ANGLE on iOS means the port creates the EGL display and surface itself (`eglGetPlatformDisplay` with ANGLE's Metal platform, `eglCreateWindowSurface` on the `CAMetalLayer` from `SDL_Metal_CreateView`, which is how ANGLE's Metal backend takes a native window) and loads the GL entry points from that EGL instead of `SDL_GL_GetProcAddress`; `p4gl_gl.c` has one resolve function, so this is expected to be about a hundred lines in `p4gl_context.c`. It is not written: it can only be tested on a device (§10.6).

**App Store and sideloading.** ANGLE links into the app as a static library or an embedded, re-signed framework (the EdgeFirst xcframework is signed and notarized under its packager's certificate; an iOS app signs its own copy). It generates no machine code at run time (Metal compiles MSL through the system), so it needs no JIT entitlement and should not affect sideloaded or JIT-less installs. OpenGL ES is deprecated, not removed, and ANGLE's Metal backend is what WebKit ships; whether a given App Store review accepts either was not checked. The risk is upkeep: ANGLE has no stable release line (versions are Chromium commit positions), builds with depot_tools and gn, and tracks Chromium's Metal fixes, so either an in-house pinned build or a third-party package has to be kept current.

**Costs on the frame.** ANGLE's validation and state tracking sit between the renderer and the driver. The renderer already makes few calls (about 72 draws, 290 uniform calls and 72 buffer uploads in a busy menu frame, before the [GL state filter](platform/performance.md#gl-state-and-draw-windows)), which is the case ANGLE handles well; the per-draw `glBufferData` streaming upload is the call to profile on Metal.

### 10.5 When SDL3's GPU API is the better choice

SDL_GPU (3.2) is a thin Vulkan/D3D12/Metal layer with ahead-of-time shaders (SPIR-V, DXBC/DXIL, MSL through SDL_shadercross). Requirements from its documentation: Metal on macOS 10.14+ (Apple Silicon or Intel Mac2 family) and iOS 13+ with an A9 or newer GPU, the simulator unsupported; Vulkan 1.0 with a list of device features on Windows, Linux and "certain Android devices"; D3D12 feature level 11_0 with resource binding tier 2 on Windows 10+.

It is the better renderer when: (1) the Apple platforms matter most and ANGLE's costs (12-15 MB, an upkeep process, ES 3.0 as a ceiling, a Metal translation of shaders at first use, no border clamp, 32-bit float depth) outweigh a rewrite; (2) the GS blend modes outside the ES 3.0 factors (the over-bright `(Cs-Cd)*FIX+Cs` family) must be exact, since a pipeline there can use dual-source blending or any blend the backend has; (3) shaders should be compiled offline, with no first-use hitch; (4) a readable, scalable swapchain and HiDPI control are wanted (SDL_GPU needs the render-to-target-then-blit structure of `P4PC_PRESENT=blit` anyway, because the swapchain texture cannot be sampled).

It costs: the renderer's state model does not map one to one. GPU pipelines are immutable, so blend, depth, cull, colour mask and primitive type (every combination the game sets per draw, the lazy state of `p4gl_gl.c` today) need a pipeline cache; the frame copies (frame rasters, DATE: a copy of the target before a draw that reads it) end the render pass, which is expensive on Metal's tile-based GPUs (SDL's own advice is to begin as few passes as possible); and Android needs the GLES path anyway (SDL_GPU's Vulkan backend runs on certain devices only), so a rewrite would add a renderer rather than replace one. With the ES path working, the recommendation is: ship ES (native on Android, Mesa on Linux, the vendor's ES or ANGLE on Windows, ANGLE on macOS, EAGL or ANGLE on iOS), measure it on Apple hardware, and move to SDL_GPU for Apple only if profiling there shows the ANGLE layer, the first-use compile, the depth format or the EAGL deprecation to be the problem.

### 10.6a Colour output rounding (why hardware GPUs failed three scene gates)

The scene gates were calibrated on Mesa's llvmpipe, and on Windows NVIDIA (WGL, ES and desktop alike) and on the Android emulator (which puts the same NVIDIA driver
behind a translator) three of them failed by 0.1 to 0.3 points (field 1.9968 against 1.9, sky 3.49 against 3.2, butterfly 3.87 against 3.7), while ANGLE's D3D11 and Mesa passed.
Nothing was wrong with the game code: an Android frame and a Windows NVIDIA frame are the same to 0.0001% MAD (96 pixels of 286720 in the field), though the two builds
share neither compiler nor libm. The difference is the framebuffer's float-to-8-bit conversion: a hardware driver rounds to nearest, llvmpipe truncates (its worker
threads inherit the EE's round-toward-zero MXCSR, docs of `include/p4pc/hostmath.h`), and the GS truncates. Hardware frames are brighter by one step on two thirds of the
pixels (sum of differences +691807 against -24627), and the gates measure against GS frames.

`p4gl_gl.c` therefore probes the context once (`probe_output_rounding`: a 4x4 target, a shader that writes 127.6/255, read back: 128 rounds, 127 truncates; `P4PC_OUTPUT_ROUNDING=nearest|trunc`
forces it) and, on a rounding context, compiles the fragment shaders with `P4_ROUND_BIAS`, which takes half a step (0.5/255) off the colour before it is written (Im2D, Im3D and the atomic
shader). On llvmpipe nothing changes (it truncates). Results: Android emulator, all six viewer gates pass (field 1.7890, sky 3.1286, near_fog 15.2975, riverbank 8.4635,
arcana 12.8303, title 1.2093); Windows NVIDIA, ES and desktop GL, all eight pass (butterfly 3.4867); Linux llvmpipe unchanged (field 1.7747). Blending is still the driver's: only
the shader's own output is biased.

**The half step was a tie (fixed 2026-11-06).** The first native Windows run of the Win64 GL host tests (RTX 4080 Laptop, NVIDIA 610.88, ES 3.2 and GL 3.3)
failed `render_smoke`, `movie_smoke` and `mod_resources_render`: wherever the GS writes a whole step the port wrote one step less (white 254, the movie one step dark
on 82 % of its channels, `movie_smoke` 0.821 from the player's frame). A whole step less half a step is exactly n - 1/2, and NVIDIA does not split a step at its half:
a float texture drawn into an RGBA8 target shows that every n - 1/2 + d with d up to d_n still gives n - 1, d_n between 1/2048 and 1/16 of a step depending on n (a
sawtooth with period 16: the conversion keeps about 12 bits of fraction). llvmpipe, which truncates, never compiles the bias. The shaders now truncate the colour
themselves on a rounding context (`p4GsTruncate`, `p4gl_gl.c`): floor(255 c + 1/512) is the GS's 8-bit colour (the 1/512 takes in the float error of a whole step,
every GS fraction is a multiple of 1/256), written as that step less 3/8, which any conversion within 1/8 step of round to nearest turns back into the step. All 22
Win64 GL host tests pass natively; the opening movie in a real run is one step brighter on 82 % of its channels, the player's frames exactly.

Blending is still the driver's, and NVIDIA blends the 8-bit source (an offset below the step changes no blended pixel): a GS alpha blend `(Cs - Cd) * As >> 7 + Cd`
truncates where NVIDIA rounds, so over every 8-bit source and alpha a blend reads 0.48 step brighter than the GS and is exact on 52 % of them. Taking a whole step
off every source had hidden that in blended scenes, at the price of every unblended one. Against the retail frames (NVIDIA, x64): title 0.7900 -> 0.7237 %,
tv_world_enter 1.5559 -> 1.3667, velvet_windows_blurry 0.9753 -> 0.8494, riverbank_vignette 1.2404 -> 1.1276, shadow_haze 1.5238 -> 1.4245, rain 2.9171 -> 2.8343,
newgame_to_field 0.8341 -> 0.7508; near_fog 3.6716 -> 3.8654, field 1.2784 -> 1.3953, sky_black_tint 2.4649 -> 2.6038, fog_battle 3.3445 -> 3.3945, Daidara
5.8288 -> 6.0191 (the blended fog layers, light shafts and the shop's dimmed background). `tests/gate_limits.txt` has their NVIDIA rows from ten runs; the NVIDIA
port frames were recaptured as `*-nvidia-rtx4080-trunc`.

### 10.6b The missing caption: decomp calls to sinf bound to glibc (fixed)

With the EE's MXCSR (round toward zero) the "Living Room" caption of `field_mad` was not drawn, on every platform; the reference frames have it. Traced with
`P4PC_GDB_TRACE`: the caption is queued by `FldPanel2dUpdate` (the panel slides in on `sinf(pi/2 * n / 10)`, n = 0..10, and draws its text only `if (!(sinf(..) < 1.0f))`).
The mirror's `sinf` was glibc's, not the retail core: gen_port's tables treat a host-exported name as the host's, and 33409fa's retail cores (`ee_libm.c`) are reached only by their
`func_` names, so the decomp's own `sinf`, `cosf`, `tanf` and `atanf` calls (the decomp mirror's libm, outside `hostmath.h`'s wrappers) ran glibc under round-toward-zero and
returned 0.99999994 where retail returns 1.0 (the argument is one float step lower under the EE's truncating multiply; the real sine is 1 - 3e-15). `P4PC_EE_ROUNDING=near` and an
MXCSR of 0x8040 hid it for that reason. `include/p4pc/libc_rename.h` now renames those four names to the retail cores (`func_0044b7b0` and so on) for decomp units, as it does
for malloc and strtok; port and host code keep glibc. Results: field 1.7747 -> 1.4894 (llvmpipe), 1.5039 (NVIDIA, Android emulator); sky 3.03 -> 2.58 / 2.68; butterfly 3.49 -> 3.16 /
3.27; title (llvmpipe) 1.21 -> 1.14. The RAM dumps of the two float modes differ in 4373 words instead of 6468; what is left is stack residue and one-ulp results that are legitimately
different between the modes. Still host libm under the EE's mode: the double calls of CRI (`cos` in adx_dcd, `log` in sfd_adxt) and RenderWare's `sqrt` (bavector.c), and doubles
in general (the EE does them in soft float, round to nearest; clang's SSE doubles obey MXCSR); not needed by any scene gate.

### 10.6c Pixel centres on edges, and a camera-texture camera's clear (found on a Galaxy S20+)

**Edge rule.** A GS vertex at an integer coordinate is a pixel centre, so the Im2D shader (`+ 0.5`) puts every sprite and bar edge through the centres of its boundary pixels. The GS includes a
centre on a top or left edge and leaves out one on a bottom or right edge; GL lets the GPU choose, and the choice is not the GS's: a 4 x 4 target with a quad from (1, 1) to (3, 3)
covers rows 2 and 3 under llvmpipe and under the Adreno 650 (`.... .... .##. .##.`, image rows top to bottom), the GS rows 1 and 2 and columns 1 and 2 (the columns agree). The title's letterbox
bars (retail rows 0-41 and 406-447) came out one row low, on the phone one row off at both ends: 1.61% on the device against 1.2% on every other target. `probe_edge_rule`
(`p4gl_gl.c`, beside the output-rounding probe) draws that quad once per context; a mismatch defines `P4_TIE_BIAS`: the vertex shader moves the vertices 1/128 pixel up and left (a few of the
1/256 steps GPUs snap them to, so it survives) and the fragment shader subtracts `1/128 * (dFdx - dFdy)` of the interpolants from the texture coordinates again, the quad being affine on the screen. The
other platforms' numbers moved too, because llvmpipe has the same rule (with the clear below, a Linux host): title 1.1366 -> 0.7172, field 1.4894 -> 1.3896, sky 2.5771 -> 2.4986, butterfly 3.1716 -> 3.0523,
riverbank 8.7199 -> 8.7079; near-fog 15.4667 -> 15.5371 and arcana 13.3181 -> 13.3375 went the other way. On the RTX 4080 (Windows): title 1.2092 -> 0.7899, field 1.5039 -> 1.4039,
sky 2.6806 -> 2.6029, near-fog 15.3062 -> 15.2122, butterfly 3.425 -> 3.1787, riverbank 8.7191 -> 8.7074, arcana 13.2380 -> 13.2579. The `systems_*` gates compare a frame with a reference the port made of
itself, so they moved by 0.1 to 0.2 points (minimap 0.4918 -> 0.6161: limit 0.5 -> 0.7; the references are to be remade).

**RwCameraClear of the shadow camera.** The field's shadow camera renders into a 128 x 128 camera-texture raster, and the game clears it (`RwCameraClear(camera, color, 3)` in `k_shadow.c`) *before* the
camera's update. `CAMERACLEAR` cleared the framebuffer bound at that moment, not the camera's own raster, so the target was never cleared: Mesa hands out zeroed texture memory and the shadow's
silhouettes stayed where the player had been, unnoticed; the Adreno gave back the memory of the last texture that lived there (Yu's uniform: the shadow quad under him came out as a black polygon, growing
over the frames drawn, and his shirt blue). `p4gl_raster_target_clear` clears the raster's own framebuffer for a camera-texture camera (`func_003f7490`); butterfly_glow on the phone went from 6.63% to 3.21%.

### 10.6 What still needs Apple hardware (and other targets) to verify

Not run, and not verifiable on the machines available:
1. **macOS, ANGLE Metal** (the default on macOS, `libEGL.dylib` and `libGLESv2.dylib` in the app bundle, [platform/macos.md](platform/macos.md)): on GitHub's arm64 Macs (Apple's paravirtual GPU) an ES 3.0 context comes up and draws (`gles_context_smoke64`). Not run: the 7 viewer and 6 game-boot gates, `render_smoke` (it exercises the border emulation, which Metal forces), and the frame diff against the desktop 3.3 frames with `png_diff.py`. Check `P4PC_GLERR=1` is silent, then `P4PC_PRESENT=blit` against the plain window on a Retina display (the target is 640x448, the window is not).
2. **macOS native GL 3.3 core** (`P4PC_GL_API=gl`): on the runners Apple's GL 4.1 core profile (the Apple Software Renderer: no GPU behind it in the VM) accepts the renderer's `#version 330 core` header and draws (`gles_context_smoke64` with `P4PC_GL_API=gl`); the gates are unrun.
3. **iOS**: the port's own EGL on a `CAMetalLayer` through ANGLE's Metal back end (`src/render/p4gl_egl.c`, [android.md](android.md) section 4: written, type-checked in CI without an iOS SDK, never compiled for iOS or run), everything that needs an iPhone (shader precision on the GPU, memory), 32-bit float depth against the 24-bit gates, context loss on backgrounding, and the per-draw `glBufferData` cost.
4. **Android devices** (EGL/GLES on Adreno, Mali and PowerVR). Run on the x86 emulator with an RTX 4080 behind its GLES translator ([android.md](android.md)): ES 3.1 context, `P4PC_PRESENT=blit` with the 4:3 fit as the Android default, context loss (SDL's `SDL_EVENT_RENDER_DEVICE_RESET`, `EGL_CONTEXT_LOST` at a swap, and `P4PC_TEST_CONTEXT_LOSS` through the `*_ctxloss` gates). Still to verify on hardware: EGL config choice (RGBA8888 + D24), whether a real pause loses the context, `highp` and NPOT behaviour on each GPU family.
5. **Linux ANGLE** (any backend), and **ANGLE's GL backend** (the Windows failure above; on GitHub's arm64 Macs `P4PC_ANGLE=gl` gets no EGL display).
6. All of these are 64-bit targets, and the renderer's callers are not 64-bit clean yet (the port64 lane); the ES path itself has no pointer-size dependence (`p4gl_*` reads retail structs through `memcpy`).

### 10.7 Reproducing

These numbers come from the project's private gate runners ([platform index](platform.md)). The gates themselves are the
CTest tests matching `title_mad|field_mad|scene_`; they need your disc image (`P4PC_ISO`, `P4PC_ELF`) and the reference
captures, which are not part of this repository. `P4PC_GL_API=gl` or `gles` selects the API (with
`MESA_GLES_VERSION_OVERRIDE=3.0 P4PC_GLES_BORDER=shader` for the ES 3.0 variant), `P4PC_ANGLE=d3d11`, `vulkan` or
`swiftshader` selects an ANGLE back end on Windows, and `tools/png_diff.py` compares two runs' captures.

## 11. Post-processing audit (retail GS dumps against the port's draw log)

Tools: `tools/gs_dump.py` (a PCSX2 `.gs.zst` as a draw list), `tools/gs_post.py` (its post draws: full-screen, frame-reading, alpha-mask passes), `tools/gs_postcmp.py retail.gs.zst port.log [--flip N] [--states]` (both lists as runs of identical passes, or every draw of both grouped by state) and the port switches:
- `P4PC_GS_LOG=<first>,<last>`: one `GSLOG <flip> <kind> ...` line per logical draw (`im2d`, `atomic` per material, `im3d`, `ptank`). Im2D/Im3D/PTank log the GL mode and vertex/index count; atomics log triangle count and mode 0/1/2 for rigid/skinned/world-sector materials. Bounds are diagnostic estimates (atomic bounding sphere, or full screen for skinned meshes and world sectors), not a pixel-ownership buffer. The remaining fields are texture raster (`FRAME` for frame copies), ALPHA/TEST, requested depth test/write, frame mask, cull and raw RenderWare fog enable/colour. For atomics `rgba` is material colour and `z` is geometry flags (0x30 = lit, zero for world sectors); blend is zero when vertex-alpha blending is disabled. Im3D logs its actual non-indexed vertex count and topology, and PTank bounds include the entire tank, not its first 256 vertices. A capture at flip N records draws logged as N-1 (the viewer's last draw is normally 1198).
- `P4PC_GS_SKIP=<a>[-<b>][,<c>...]`: the draws with those indices (the order of the GSLOG lines of a logged frame, from 0) are not drawn, to find which draw makes a pixel (skip a group, diff the capture).
- The same comparison tool also accepts the live `P4PC_DRAW_LOG=<flip>` / `P4PC_DRAW_SKIP=k` format. It normalizes atomic index counts to triangles and preserves the full frame mask. If both logs cover the selected flip, GSLOG is used alone: it distinguishes PTanks and records each atomic material's actual texture, while DRAWLOG labels PTanks as Im3D and its atomic texture field is immediate-mode state.
- `P4PC_EVENT_LOG=N` observes every Nth presented frame: game clock, field/room, date/slot, monotonic host nanoseconds (`host_ns`), camera LTM/view window/clip planes and active script procedure names/indices. It does not override the camera or start an event; use it to locate a corpus moment in a real story walk before choosing a capture flip, or measure a field transition's real duration in a paced run (`P4PC_VIRTUAL_CLOCK=0 P4PC_TURBO=0`).
- `tests/event_scene_state.py RAM.bin SCENE docs/skygl_scenes.json` verifies a real-play snapshot's corpus field/date/slot/weather, procedure/bytecode state, registered presentation phase and meaningful camera components. It reads only guest state, never writes a camera or starts a procedure. State-triggered event gates dump RAM on the selected presented flip, before its present: pixel similarity alone must not let an unrelated viewer or story moment pass.

Dialogue registration follows the **inner EVT/message/portrait state**, not only the enclosing BF command timer: that timer continues during asynchronous asset waits. `P4PC_CAPTURE_STATE=event,PROC,INDEX,PC,ID0,ID1,ID2,FRAME,MESSAGE,ROW,ELAPSED,PORTRAIT,FIRST,LAST` retains the BF procedure/index/PC guard, then requires the exact EVT identity/frame, message record, text-row index and row-relative elapsed value. The message opening transition must be complete (row phase4, transition0x200), and the requested portrait bank must be loaded, visible and settled in fade state3. Neither the observer nor its RAM checker advances an event, completes a read, changes RNG or forces a portrait.

The observer also supports
`rankup,PROC,INDEX,PC,FRAME,COMMUNITY,RANK,RESOURCE,FIRST,LAST`.
`cmmRankUpSequence` has active relation10 and flags0x18; FRAME is its draw-owned
mode at work+8, not its text-fade counter or enclosing BF timer. Drawing must
remain armed while observing that clock. The ordinary Arcana gate retains
its established `port_capture` procedure registration and historical image;
there is no mode-selected alternate scene registration.

Standalone battle gates retain their original scripted presses, fixed capture
flips and pinned references under the port's single PC-speed timing contract.

A read-only repetition of the original fog-battle reference command
(`ng-dreambtl.p2s`, `--gs-at 10`) reproduced PNG SHA256
`927d4acf3cf411f705afc2739c5f033764323c178b840b427b0a178d85406ebc`.
That draw has COMMAND state5/timer106, rather than the savestate's timer96.
An earlier exact-clock diagnostic matched that command state and unit
placement but not actor animation age (1.066666126 versus0.666666508), effect
RNG or RpRandom state. Matching a command clock alone is therefore not a
full semantic certificate or justification for a shader change. The ordinary
fog gate keeps its established fixed-flip capture; save-anchor fidelity
comparisons must establish full state equality (`docs/lockstep.md`).

| Corpus dialogue | EVT ID / frame | Message / row / elapsed | Portrait descriptor |
|---|---|---|---|
| Velvet windows, Igor's “Welcome.” | 154,1,0 / 190 | 16 / 0 / 10 | `0x210100` |
| Shadow haze, Other Yosuke's shopping-district line | 145,1,0 / 628 | 25 / 1 / 7 | `0x230100` |
| Rain, established PC capture | 108,1,0 / 195 | 0 / 0 / 249 | `0x0a0103` |

These are the retained PC capture phases; nominal flips remain the existing
±600 sanity bounds, not replacement predicates. Rain uses center25100,
TV-world entrance71778 and Shadow haze103408. No timing-model removal widens
an image/camera bound or changes an ordinary controller recipe. The last two
were 75594 and 110560 until story voices stopped playing at half speed in
30 fps scenes (audio now renders per VBLANK, as the SPU2 plays;
[audio](audio.md#end-of-a-voice)). The same capture states now come that much
earlier on the walk. The tunnel frame is unchanged (port frames 0%). The haze
noise is in another phase at the same dialogue state: the 0.201% from the old
x64 port frames lies entirely in the haze around Shadow Yosuke, while
portraits, text and the room are identical, and the NVIDIA retail MAD moves
from 1.545% to 1.524%. Its port frames are therefore the new
`port-scene-shadow-haze-realtime-voice-*` pins.

The historical Rain savestate retains rowelapsed69, while the established PC
registration uses249. All three rain-emitter parameter records through+0x3c
agree apart from owning pointers; that history difference does not justify
changing the rain shader, particle size or alpha.

A retail dump is `p4hook --software-gs --gs-at FRAME:BASE --state X.p2s`; it holds two fields, the last nonempty field is the frame. `local_boot.sh` builds the committed tree: commit before a run that needs a new switch. Retail screenshots presented as 640x480 must first be resized to the native 640x448 (LANCZOS for the references below), never cropped or compared as a flat RGB buffer against 640x448. Compare the camera LTM/view window too: the Entrance and seats cameras in the new captures agree with their retail states bit for bit in all meaningful matrix fields. The old file named `ng-0412-evening` is actually a classroom conversation; use PortTrace's `gs/ng-livingroom` for the genuine 04/11 Dojima evening.

The log and GS decoder preserve all 32 FRAME.FBMSK bits, including the alpha-write byte; `--states` keeps the mask value, not just a masked/unmasked boolean. Framebuffer base zero is a real target and is always printed, so alternating fields do not become an unknown target in the pass comparison. Old six-digit zero masks remain readable. The `gs_audit` witness covers framebuffer zero, the high alpha-mask byte and topology/count conventions.

Im2D now converts screen XY to the GS **12.4 grid before** the existing
pixel-centre and probed tie-bias rules. In the original dream-battle dump,
the outline's first vertex is `(32.625,211)` while the matching game geometry
and scale constants produce `(32.663607,211)` before conversion; all first
eight vertices match `floor(xy*16)/16`. The signed/offscreen case is not
guessed from those positive samples: the retained retail Im2D buffer at
`0x955230` holds `(111.2368927,-92.5898895)` and the corresponding GS feedback
quad uses `(111.1875,-92.625)`. Truncating screen-space XY toward zero would
instead give `-92.5625`. The recorded XYOFFSET is `(1728,1824)`, consistent
with truncation after adding the positive GS offset within this guardband.
This does not establish behaviour outside the representable GS range.
`render_smoke` compares fractional rectangles and offscreen diagonals with
explicit GS-grid draws and requires the negative-coordinate probes to
distinguish downward conversion from truncation toward zero. No half-pixel
bias, fog scale, reference or limit is adjusted.

The dream-battle black TV outlines are GS `PRIM=0xc9`: **AA1 lines**, not
hard-edged GL lines. Equal geometry tables and clocks therefore do not imply
equal boundary pixels. The frozen reference SDK (`ec3694914cda4af44e141fcfa64124c889380b55`,
`GSRasterizer.cpp`, lines471–627) splits fractional coverage between two
adjacent minor-axis pixels and uses the diamond-exit rule at endpoints.
Its edge selector disables Z writes; with ABE enabled, coverage replaces
only full source alpha, retaining an already partial source alpha.
The corresponding upstream implementation is
[DrawEdgeLine](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS/Renderers/SW/GSRasterizer.cpp#L471-L627).
Im2D line lists/strips now use instanced endpoint attributes and a coverage
shader on GL3.3/GLES3.0; ordinary triangle programs and VAO divisors stay
separate. Unindexed lines and single indexed lines reuse the original
vertex upload without copying; indexed multi-line draws reuse a gathered
buffer. DATE and alpha-failure replay retain the same coverage path.
`render_smoke` covers horizontal/vertical fractional coverage, endpoint
ownership, reversed/indexed/polyline entry points, partial alpha with and
without ABE, no edge Z writes, and an ordinary triangle draw after lines.

Findings (Dojima living room, classroom, rain, Marukyu seats / 1F / battle, TV-world Entrance, Yomotsu Hirasaka, the Velvet Room, the title menu, the fog dream):
- The noisy fields' post tail has the same families and ordering: alpha mask (`fbmsk ffffff`, AFAIL FB_ONLY), display-frame feedback (`(Cs-Cd)*Ad+Cd`), second mask, noise, 7x8 colour-filter quads (112 triangles), HUD. Distinguish the 14 horizontal 640x32 noise strips from the following 70 64x64 tiles; the earlier notes called the latter strips. Outdoor scenes can add the additive display-raster bloom quad. The title menu has four mask/fill/`Ad` groups and a final textured `Ad` group. `--states` is a state-family inventory, not proof of equal per-triangle coverage: retail clips/culls in VU code and batches strips; the port submits full indexed meshes. ATST ALWAYS and disabled alpha test can be equivalent, and a mask/AFAIL can make a recorded alpha test irrelevant to RGB.
- The filter state (`0x7643A8`, table row of `FldColorCrossfadeStep`) agrees with retail's savestates in every scene the story walk reaches (0x3a living room, 0x69 class, 0x7a evening, 0x68 fog dream, 0x84 rain, 0x97, 0x2a, 0x5, 0x3, 0xb5 Marukyu, 0xf8, 0x30, 0x20 dungeons); the Velvet Room's `(0-Cs)*As+Cd` filter has the same per-cell colours, and the Entrance's 56 quads are 0x6873172b (retail) = 104,115,23,87 (port). The index is 0 in a Marukyu random battle in retail and in the port (no filter quads).
- **Story rain matrix particles:** at the corpus camera/date/procedure (04/12, field 10:1, `sdl04_12_AM_A` 89), the port's three rain PTanks own the over-bright world pixels, amplified by the following display-frame bloom. Disabling the port bloom is an ownership experiment, not a fix: a retail capture with `effBlurFilterDrawSlotPass` bypassed proves retail uses the bloom too. Identical retail/port particle matrix rows have length 29 at camera depth 200; the retail GS quad is 95.625 pixels wide at view window 0.485293627. Matrix rows therefore describe full dimensions, as `cSize` does. Expanding +/- the entire row doubled each dimension and quadrupled rain coverage. The matrix path now expands +/- half the row; frame-copy and bloom behaviour remain unchanged.
- **Story near-fog:** the viewer loads field 68 without its event scripts, so its missing FldNoise group is not evidence that the story omits it. The real story capture at 17140 (`newgame_to_0413.walk`, GS log 17139) contains the masks, display-frame quad, 14 horizontal strips and 70 noise tiles, then glow/halo feedback pairs. Controlled captures of revision `2f334080` with virtual clock and offscreen rendering give retail MAD 5.4337% on Windows/NVIDIA, 6.0527% on Linux/Mesa 25 llvmpipe, and 5.7366% on Windows/Mesa 26 llvmpipe. The Windows/NVIDIA and Linux captures have bit-identical camera LTM, view window and game clock, and the same 375 logical draws, including seven PTanks with identical vertex counts: this is not an animation wall-clock/sub-step drift. The output probe truncates on Linux/Mesa 25 but rounds on the two Windows backends; their frame-feedback/blending arithmetic differs (inter-target MAD 1.2026% NVIDIA/Linux llvmpipe, 0.6911% Windows/Linux llvmpipe). After integrating main's GS quad-edge correction (`9feb85d4`), controlled captures of `321e6a8d` improve to 5.3998% NVIDIA / 6.0273% Linux llvmpipe (1.1993% inter-target). Those corridor-start measurements remain calibration evidence, not a second gate: the canonical corpus `scene_near_fog` in section 11.2 replaces that timing and the viewer-only comparison with stricter full-frame and regional limits.
- **TV Entrance cause:** draw 208 of the aligned 2899 log is the 1152-triangle translucent haze mesh (`f020_001_effect_01`, geometry flags 0x7f, alpha 230/255, vertex alpha 0/255, cull back, no Z write). Draw 207, its 416-triangle companion, contributes no pixels in the bottom-centre region (x 80..399, y 320..447); removing 208 does. Retail emits the clipped haze as a 749-triangle strip batch, white RGB 0x80 with alpha 0..0x73. The material/vertex values were not the cause: the GS interpolates Gouraud RGBA **affinely in screen space**, but the GL shaders were interpolating it with perspective. Large fading haze triangles therefore became too opaque. Atomic and Im3D shaders now transmit colour times clip W and divide by interpolated W; STQ stays perspective-correct, and this works in GLES 3.0 without `noperspective`. The atomic reuses its existing W/fog varying. Full-frame MAD improves 4.190 -> 3.451%; region MAD 8.953 -> 5.898%, and mean region RGB 149,129,96 -> 139,118,91 (retail 127,103,80). Noise/animation and VU clipping leave residual differences; no colour/alpha multiplier or scene special case was added.
- **Material samplers:** atomic materials now use `RwTexture.filterAddressing`, not the last immediate-mode sprite's sampler state. The shared binding code retains border emulation, camera-texture clamp and explicit GS CLAMP overrides. This removes a separate small sampling error: aligned seats MAD 2.449 -> 2.367%, Entrance 4.190 -> 4.082% before the affine-colour fix. The earlier 18.6% seats / 10.8% Entrance figures are not the new registered native-height comparisons. `scene_marukyu_seats` (3.0%) and `scene_tv_entrance` (3.8%) use retail-derived private `systems_seats.rgb` / `systems_entrance.rgb`, not self-generated screenshots. `render_smoke` has a varying-W colour triangle witness and `gs_audit` checks atomic versus immediate-mode primitive counts.
- **Minimap regression reference:** the existing `systems_minimap` full-frame gate changed by 0.584814% against its old facing-only port reference. This is predominantly world-floor sampling, not a minimap HUD defect. A new retail run used the same generated floor seed 0x17de (the retail game's existing debug-seed word `iGpffffb214`, 0x764304, set in a private pre-floor savestate) and the normal walker. Player position (15200, 2, 20400), every meaningful camera matrix field, view window and clip planes match the port gate bit for bit. Against that registered retail frame, old -> corrected port full-frame MAD is 2.3463 -> 2.0781%; the world and floor regions improve as listed below. The original GS cutover introduced `systems_minimap_gs.rgb` as a new port-owned baseline for that verified retail improvement. Its bytes are retained unchanged; the later asynchronous-load cutover uses a separate filename (section 11.2). This is not presented as a pixel-exact retail gate: party poses and noise/glow phase still differ.

### 11.1 Three-dimensional pass coverage and regional ownership

The native Windows GPU captures use the real scene loaders, hidden/offscreen with dummy audio. These are pass-family comparisons, not a claim that an unclipped port mesh must have the same primitive count as retail's VU output.

| Scene | Retail 3D / post passes | Port observation |
|---|---|---|
| Dojima living room, 04/11 evening, rain | Textured room and characters; translucent untextured shadow casters; textured projected shadow receiver; additive effect geometry; 112-triangle colour filter and HUD. No field-noise/frame-feedback tail in this indoor scene. | Weather-aligned viewer `field:7:2`, date 10, slot 5, weather 1: 163 atomic material draws, one Im3D shadow receiver, no PTank draw; 56 filter quads. All pass families present. 1.544% MAD against genuine retail `gs/ng-livingroom.png`; the protagonist's pose and CHECK icon differ. |
| TV Entrance | Textured/lit and outline-like material passes; untextured shadow casters and textured Im3D receiver; additive cones; particle billboards; translucent haze; mask/frame-feedback/noise/filter tail. | Atomic, Im3D and PTank families all logged. Haze interpolation and material sampling corrected; 3.451% MAD, `scene_tv_entrance`. |
| Marukyu seats | Room/party materials and shadows; additive lamp/cone/halo meshes; stage-fog PTank; mask/frame-feedback/noise/filter tail. | Final aligned MAD 2.324% after both fixes, `scene_marukyu_seats`. Draw isolation below distinguishes animated lamp/fog residuals from a missing pass. |
| Marukyu 1F, minimap gate | Dungeon materials, party, neon/glow geometry, mask/frame-feedback/noise/filter tail, minimap/HUD. | Registered seed-0x17de retail run; world-region MAD 1.9130 -> 1.4906%, floor patches improve below. `systems_minimap` retains 0.7% against its refreshed port-owned reference; section 11.2 records the later minimum-asynchronous-load comparison. |
| Zio cut-in, dream tutorial | Character materials and shadow/outline-like passes; two additive textured particle groups, one alpha-blended billboard group; additive untextured energy; halo alpha-mask/frame-feedback pairs; negative colour filter and HUD. | The initial audit logs three PTanks and the other post families. After integrating PortGLES's BSP-sector fix, capture 700 / log 699 contains 24 atomic materials (including one 512-triangle world sector), four PTanks (one alpha, three additive at this phase), and 128 Im2D draws, including all 56 negative-filter quads. Sector logging uses the geometry view's flags and never dereferences a nonexistent morph target. The captured first cast has SP 37; retail `gs/zio_b` is a later cast (SP 29), so no phase-aligned MAD is claimed. The formerly missing BSP stage is now present and covered by PortGLES's `scene_fog_battle` gate; no duplicate stage fix was added. |
| Field 68, story near-fog | Fog/glow billboards and alpha-mask/feedback pairs, shadows, FldNoise and colour filter. | The event-scripted story contains these families. The earlier corridor-start calibration measured 5.400% NVIDIA / 6.027% Linux llvmpipe with matching camera/game-clock inputs. Its gate is retired: the canonical corpus `scene_near_fog` in section 11.2 replaces it with stricter full-frame and regional comparisons. |
| Shuffle Time | Gold patterned background, face-up Persona/blank cards, blue backs, shuffle/stop, enlarged drawn card; layout depends on the selected shuffle mode. | Card art, backing and background render consistently in the captured port sequence. The 12-slot/five-try screen is a legal mode-3 roll, whereas PortTrace's corresponding battle rolled mode 1; no pixel-MAD equivalence or forced-mode fix is claimed. See the live state/probability evidence below. |

Marukyu isolation at capture 5100 / log 5099 (same camera as retail):

| Native-frame region | Baseline retail MAD | Changed by skipping additive atomics 319–355 | Changed by skipping fog PTank 356 |
|---|---:|---:|---:|
| Lamps, x 0..639 / y 0..127 | 3.432% | 12.333% | 0 |
| Stage fog, x 190..439 / y 160..259 | 1.578% | 0.852% | 1.663% |
| Party, x 170..469 / y 230..389 | 1.664% | 2.648% | 0 |

Removing either group worsens the retail comparison (whole-frame 2.324 -> 5.535% without additive geometry, 2.386% without fog): they belong in the scene. The residual lamp/fog pixels are not an absent bloom/noise pass. These logical draw indices belong to this capture, not stable asset IDs.

Registered minimap scene at port capture 8299, retail `systems17de_minimap` (40 vsyncs after the settled matching state); both port columns use the same Linux backend:

| Native-frame region | Old port vs retail MAD | Corrected port vs retail MAD |
|---|---:|---:|
| World, x 155..539 / y 64..415 | 1.9130% | 1.4906% |
| Right floor, x 416..527 / y 256..415 | 2.6776% | 1.8675% |
| Left floor, x 155..209 / y 280..415 | 2.4949% | 1.5683% |
| Far corridor, x 168..469 / y 197..244 | 1.6069% | 1.3157% |

These floor patches exclude the moving party. The gate draw log contains 135 atomic materials and 221 Im2D draws; the first 49 atomic materials draw the dungeon/decorations (including the skinned curtain), followed by the party. Floor texture `f042_001_yuka_01` is used by logical draws 18, 31 and 45. The existing old/new raw references differ by exactly the 0.584814% seen in the failed land check, so the regional retail improvement justifies that baseline change, rather than merely accepting a shifted image.

Zio audit recipe: encounter 532, `P4PC_BATTLE_FIRST=down,cross,cross,cross`, `P4PC_BATTLE_DRIVER=cross,cross,cross`, `P4PC_BATTLE_SKIP=140`, `P4PC_BATTLE_CMD_DELAY=80`. In the current harness, delay 20 sends Down at flip 568 while the introductory message is closing; the wheel appears around 580, so Cross selects Attack. Delay 80 reaches Zio and shows the real Zio/Cleave/Rakukaja list; this is input-phase alignment, not missing skill data. The log/capture window 679..719 / 680..720 covers the first cast.

The held `apollo_game_floors.txt` and `apollo_heaven_floors.txt` are retained with their observed walker limitations in their headers, not advertised as passing multi-floor gates. The club entry/1F/menu scripts were already on main (`4da03770`).

Shuffle Time's six-card preview followed by a 3x4 grid with five tries is a real retail mode, not by itself corrupted card rendering: `btlShuffleSeq4Run` (0037DA60), selected by mode 3 from `btlShuffleRollBelow` (0036E920). Live port RAM has average defeated-enemy level 28, mode/submode/count `(3,0,6)`; the equivalent retail Marukyu battle rolls `(1,1,4)`. Both have the identical probability row at `D_0064E610`: `[30,30,30,10,0]`. A different legal random outcome must not be "fixed" by forcing four cards or forcing the mode. The drawing context is task work **plus 0x70**: its mode/submode/count fields therefore live at task work +0x1F36C/+0x1F370/+0x1F374, not +0x1F2FC directly.

The native scene runner exercised the Entrance and seats retail gates successfully; their limits remain 3.8% and 3.0%. The initial corridor-start fog calibration repeated at 5.3998% on NVIDIA (ten runs, spread 0.0000) versus 6.0273% on Linux llvmpipe. That old gate and its limits are retired, not kept beside the canonical corpus moment: `scene_near_fog` now uses 3.73% NVIDIA / 4.15% Linux full-frame limits and a 3.35% regional limit (section 11.2). The capture-comparison figures above are measured from retained PNGs. Private references are installed beside the retail inputs on two Linux hosts and Windows; no captures, savestates or reference RGB bytes belong in git.

### 11.2 Corpus moments reached by normal story input

`event_scene_gate.sh` runs the normal New Game or checkpoint walk, compares captured pixels with the corpus reference, then validates a snapshot from that same draw against `skygl_scenes.json`: field, room, date, slot, latched weather, active procedure and meaningful camera components. Velvet and riverbank retain the established `save0415_to_river_scene.walk`; their exact message/portrait predicates select Igor's “Welcome.” and Yosuke's first “Yo!”, not a visually similar substitute. The riverbank viewer-only gate is replaced by that story-state gate.

The riverbank's wrong rain/fog was an ordering defect, not a calendar-table or colour-filter defect. Synchronous host reads let resource resolution run in the requesting update before the calendar latched the new day: weather1, filter0x84 and far97134.648 instead of retail's weather2, filter0xe1 and far200000. The next-scheduler-frame boundary is sufficient to preserve this dependency. ADXF, CVFS and raw CDVD retain that minimum asynchronous fence; the decompiled calendar and field-task code is unchanged. See the [PC disc contract](platform/disc_cri.md#pc-disc-contract).

| Normal-play scene / corpus beat | Retail passes | Measured retail MAD, Linux llvmpipe / Windows NVIDIA | Gate |
|---|---|---|---|
| Velvet Room, 04/15 night, Igor “Welcome.” | Room/characters and shadow geometry; blurred animated window materials, negative colour-filter grid, portrait/message/HUD. | 0.940% / 0.981% | `scene_velvet_windows_blurry`, 1.05%; full camera/procedure/weather check. |
| Riverbank, 04/16 morning, Yosuke “Yo!” | Fogged outdoor geometry and characters/shadows; 112-triangle colour filter, display-raster bloom, dialogue/HUD. | 1.216% / 1.245% | `scene_riverbank_vignette`, 1.30%; full camera/procedure/weather check, replacing the viewer-only 8.8% gate. |
| Rain, 04/12 morning, Nanako “You keep going straight from here.” | Three matrix rain PTanks, outdoor/character/shadow materials, colour filter and display-raster bloom, portrait/message/HUD. | 2.672% / 2.921% at capture 25100 after the minimum asynchronous-load ordering and matrix-dimension correction. | `scene_rain`, 3.0%; full camera/procedure/weather check. |
| Arcana rank-up, 04/16 morning, `155_commu_open` | Outdoor materials/shadows; darkening filter and display-frame feedback; additive sparkle billboards and halo alpha-mask/feedback pairs. | 3.593% / 2.101%; full camera/procedure/weather check. Increasing Linux history from 150 to 1200 drawn frames moves MAD only 0.000275 percentage points, excluding a warm-up explanation. | `scene_arcana_rankup_sparkles`, replacing the viewer-only 13.4% gate. |
| TV-world tunnel, 04/14 after school | Opaque grey screen fill; repeated screen-space contour/ring triangles with alpha-mask passes and RGB-separated edges. | 1.569% / 1.565% at normal-play capture 75594. The same state is reached at 71778 since voices play in real time; Windows 1.556% there. The earlier 29% was a different animation flip, not a missing pass. | `scene_tv_world_enter`, 1.65%; field/date/weather/procedure plus active screen-space compositor state. |
| Shadow Yosuke haze, 04/15 after school, “Screw the shopping district...” | Room/characters/shadows, two haze PTanks, mask/display-feedback/noise/filter tail, portrait/dialogue/HUD. | 1.539% / 1.589% at capture 110560. The same state is reached at 103408 since voices play in real time; Windows 1.524% there. | `scene_shadow_haze`, 1.65%; full camera/procedure/weather check. |
| Near-fog, real 04/11 dream corridor near its end | Fog/glow billboards, alpha-mask/frame-feedback pairs, shadows, FldNoise and colour filter. | Fresh canonical retail capture: full-frame 4.064% / 3.676%; upper-fog region `(0,0,640,200)` 3.293% / 2.884%. | Canonical `scene_near_fog`, replacing both the viewer-only gate and the old corridor-start `scene_story_near_fog` timing. Full-frame limit 4.15%, upper-fog limit 3.35%. |

**Only active image state is compared.** The tunnel's `cldDungeonIn` compositor covers the entire image with an opaque 640x480 fill before drawing its screen-space rings. Retail's frame-1 state still has the field camera at identity, while the faster port has installed the Junes background camera; that camera cannot contribute a pixel under the fill. The gate therefore checks the real compositor task (state 1, within five animation frames of retail's 104), in addition to field/date/weather/procedure, rather than constraining an unused background camera. No camera or compositor state is written.

**State-registered real-play captures.** All seven `event_scene_gate.sh` users
take their registration from `port_capture` in `docs/skygl_scenes.json`.
Six match a BF procedure name/index, bytecode PC and command timer; near-fog
matches field 68:1, the player within one unit of `(8640,0,3930)` and the
settled camera basis, then waits 206 flips inside that predicate. The old
capture argument now supplies only a nominal sanity interval (plus/minus
600 flips). A missing state or an out-of-bounds selection fails; there is
no fixed-flip fallback. Turbo draw warm-up follows the same state, and the
selected flip produces both its image and its RAM snapshot. Existing
references, MAD limits and the near-fog regional comparison are unchanged.
The procedure-table index at `ScrScriptWork+0x12c` is not the bytecode PC at
`+0x18`: command timers reset at each instruction. Matching only the index
and timer can select a much earlier dialogue wait, so both are required.

**Historical references are regression witnesses, not whole-machine equality
certificates.** Velvet, rain, Shadow haze and Arcana retain their historical
images and bounds. A replacement requires a qualified save-loaded anchor and
the same replay with full camera/RNG/state equality (`docs/lockstep.md`).
An old `verified` flag from a narrower schema is not such a certificate.
Neither counters/RNG nor reference bytes are rewritten to hide a different
update history; exact PS2 boot pacing is not a port requirement.


**Nearby legal fog heading.** The corridor walk turns the protagonist back and uses the game's Circle recenter control. Its stable heading is 0 degrees, retail's 11.8098 degrees. Per the accepted comparison contract, the gate bounds that legitimate camera to within 12 degrees and 160 horizontal units of retail (height within 0.25), retains exact view/clip/weather/procedure checks, and additionally judges the upper 200 rows: no character or heading-dependent floor pixels occur there. It does not claim pixel-aligned floor/pose equality. `p4-near-fog-corpus.rgb` comes from a fresh software-GS capture of the canonical `near-fog.p2s`, resized from 640x480 to 640x448 with LANCZOS; the old viewer and corridor-start references are not silently overwritten.

The new logical draw inventories are: rain 162 atomics / three PTanks / 116 Im2D; Velvet 165 / two / 73; riverbank 137 / zero / 85; Arcana 150 / two / 77; Shadow haze 152 / two / 280; near-fog 23 / four / 337; TV tunnel three / zero / 230. The tunnel's 78 alpha-mask draws belong to its screen-space rings; its three background atomics are occluded. Arcana includes three alpha-mask draws and three frame reads; near-fog includes 100 alpha-mask draws and 89 frame reads (the repeated glow/halo groups as well as FldNoise). Counts describe these exact captured animation phases, not required equality with clipped retail primitives.

The existing generated-floor visual regressions now pin their original layouts independently of load duration: Marukyu `0x17de`, Void Quest `0x16bc`, Heaven `0x1699`. The latter two seeds were read from the retained synchronous runtime's actual floor-state word at 0x764330; this preserves the reference layouts instead of accepting a new random floor after asynchronous reads shift the build frame. The minimap's new asynchronous reference is separately checked against the same registered retail frame as the original sampler/colour reference.


Ten repeated native NVIDIA runs of the new canonical gates are deterministic to four decimals: near-fog full-frame 3.6758% (upper region 2.8845%), Arcana 2.1008%. `gate_limits.txt` tightens their NVIDIA full-frame limits to 3.73% and 2.15%; Linux keeps 4.15% and 3.65%, respectively. The fog-region bound stays 3.35%. Regional output labels its **region limit** separately, so native runners do not accidentally pair that bound with the full-frame MAD.

A controlled Windows/NVIDIA haze study used an immutable **rebuild** of accepted `532df4da06fa25404d2b81001b47378b3d7c08f8` (the original export bytes were not assumed): 20 strict-solo captures and 20 captures in an up-to-three-worker pool, no completed-image retries. All 40 passed at 1.5460% MAD and decoded RGB was byte-identical (`6edc7da83bd0a647ca4150bb2e51563b171cc916d96a00c749d50ad493971658`). When the new land/live/study priority rule arrived, the last two queued starts were drained before an application began, then resumed under canonical priority admission; their original missing-helper logs and fresh first-image results are retained separately. This qualifies the combined `f4475d2f` guest-base/ASLR and `7ac6ac55` scripted-input fixes, not either fix in isolation. It does not loosen the 1.65% gate.

After the asynchronous cutover, the minimap's seed and every meaningful camera component remain equal to registered retail. Its animated party/noise phase moved, so the old port-owned reference differs by 1.4333%. The new `systems_minimap_async.rgb` reference is justified by the same registered retail frame, not merely by accepting a new port picture:

| Native-frame region | Before GS sampler/colour fix vs retail | GS-fixed, synchronous reads vs retail | GS-fixed, minimum asynchronous reads vs retail |
|---|---:|---:|---:|
| Whole frame | 2.3463% | 2.0781% | 1.6867% |
| World, x 155..539 / y 64..415 | 1.9130% | 1.4906% | 1.5744% |
| Right floor, x 416..527 / y 256..415 | 2.6776% | 1.8675% | 1.6437% |
| Left floor, x 155..209 / y 280..415 | 2.4949% | 1.5683% | 1.2968% |
| Far corridor, x 168..469 / y 197..244 | 1.6069% | 1.3157% | 1.3157% |

The world rectangle includes the moving party and its residual increases slightly; the fixed floor regions improve again. The asynchronous-load cutover selects the new **port-owned regression reference** `systems_minimap_async.rgb`, retaining the 0.7% gate; it does not overwrite `systems_minimap_gs.rgb` used by pre-cutover commits. Reference replacements must use a new filename selected in the same commit, with the private bytes staged on every gate host before submission. All seven corpus scene references remain retail-derived. The device scene runner now carries each test's declared `ENVIRONMENT` (including the canonical fog reference name and fixed dungeon seeds), with the same virtual-clock policy as ctest; the event-state unit witness covers that manifest parsing as well as exact cameras, bounded nearby fog cameras, active TV compositor state and regional pixel exclusion.

`tests/refs.sha256` pins immutable reference filenames and their SHA256s. The asynchronous minimap and canonical fog references are appended under their new names in the same cutover commit as the gate selectors; the binary bytes remain private.

Daidara retains its established flip5299 capture and6% image bound. A game-only
control from published `3f37d69` measured5.8245% against the old golden at
that flip; animated diagonal chains/gears and UI/background age are not
grounds for re-pinning it. The GS colour/sampler retail-region comparison
above remains the independent evidence for the renderer correction.

Other systems retain their established PC input tapes, capture flips, references
and image bounds. Generated floors retain seeds `0x16bc` and `0x1699`.
The model removal neither retimes these captures nor relaxes a bound to accept
a loading splash, preceding menu or unfinished purchase.

The primary-product `scene_*64` twins inherit the same walk, reference, fixed seed, virtual clock and state checks as their i386 reference gates. The NVIDIA fog/Arcana limits name both ABIs explicitly, so the x64 land check retains the stricter GPU bounds.

### 11.3 Fog-dream cross: RGB_ONLY alpha failures must not write Z

The reported broad cross is not a presentation-scale or bilinear tile seam. At
encounter 532's command-menu capture 1700 (draw log 1699), logical atomic draw 13
owns it: the `f222_003_effect` crossed fading haze mesh, material alpha 179,
`TEST_1 = 0x35fb` (GEQUAL 127, AFAIL RGB_ONLY). Omitting that draw removes the
cross; omitting the floor or the eight earlier haze draws does not. A stage-only
capture before this material has no cross. The retail `ng-dreambtl` software-GS
dump uses the same RGB_ONLY failure mode, repeating bilinear texture sampling
(`CLAMP=0`, 128x128 T4), and fading vertex alpha.

Previously RGB_ONLY disabled the shader's alpha test but left depth and alpha
writes enabled. Even transparent parts of intersecting haze triangles therefore
wrote Z and occluded later haze. Atomic, Im2D and Im3D/PTank now separate passing
fragments from colour-writing failures: FB_ONLY failures write RGBA without Z;
RGB_ONLY failures write RGB without either Z or destination alpha. Passing
fragments retain the requested depth and framebuffer masks. `render_smoke`
exercises subthreshold colour, preserved Ad, absence of occlusion, and successful
fragments that still write Z.

`scene_fog_battle` additionally runs `tests/fog_seam.py`: the maximum dark
row/column discontinuity in the reported background region must be at most
3 luminance levels. It averages along the bands to reject animated noise and
compares nearby flanks instead of rejecting the genuine broad fog gradient.
The vertical/horizontal scores are 7.775/6.286 before, 1.991/1.861 after, and
1.147/1.159 in the immutable retail software-GS reference. A 1277x954 window
with 1x internal resolution produces the same native scores as a 640x448
capture: scaling is not the cause. NVIDIA's full-frame retail MAD after the
fix is 3.5512%; the existing reference and 3.9% limit are unchanged.

The retail Attack popup must be judged by animation phase, not an isolated
grey digit: a normal `ng-dreambtl` run with `cross@30:6/90` has a black diamond
at frame 211, a fading grey `6` at 216, and the bright `6` with its yellow
ornament at 221. These are software-GS captures, not hardware-GS approximations.
The native gate-input run likewise has a black diamond at capture 1004,
dark-grey `6` at 1005, light-grey `6` at 1006 and a bright digit with its
yellow bar by 1008. The low-contrast initial digit is its normal fade-in;
no popup recolouring is applied.

Shared renderer changes now add an i386 Linux/llvmpipe land shard running
`scene_fog_battle` and `scene_tv_world_enter`, in addition to the Windows x64
GPU gates. This also runs when the smoke/check scripts themselves change.
The queue compares the checked batch with `origin/main`; unrelated changes
do not add the reference-ABI build. The shard sets
`P4PC_SCENE_ARTIFACTS=/lane/build/gs-smoke`: a failed corpus scene retains its
PNG and guest RAM there, and New Game corpus runs retain their boot log.
Successful runs remove their scratch directory as before. This avoids losing
the compositor phase and image when a parallel reference check fails.

#### Tight port witnesses and independent AFAIL controls

The retail MAD bound is a fidelity witness, not sufficient evidence that a
particular rendering fix remains present. The 4:3 x64 `scene_port_*_llvmpipe64`
and `scene_port_*_nvidia64` gates therefore also compare against **new,
renderer-scoped port frames at 0.05% MAD**. They retain the original scene's
inputs, virtual clock, draw history and capture selection; the seven event
scenes still validate their normal-play state. Field context-loss reuses the
ordinary field's port frame. The original retail references and limits,
including fog's 3.9% and the 3-level discontinuity guard, are unchanged.

Renderer guards require the exact `GL_RENDERER` (`llvmpipe (LLVM 19.1.7, 256
bits)` or `NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2`). An unsupported
renderer exits 77; that is skipped coverage, not a successful comparison.
Port-frame changes require a new immutable filename and causal evidence;
they cannot justify replacing a retail reference or widening its bound.

Two different AFAIL repairs must not be conflated. An isolated pre-`7de79b90`
`ATST NEVER + ZB_ONLY` discard changes the TV outline's noise/menu clipping,
not the reported central background cross. The actual cross control removes
RGB_ONLY from `p4gl_gs_alpha_color_fail` (`bb6216c4`), retaining every other
baseline object and the ZB_ONLY repair. Native NVIDIA captures quantify both:

| Frozen main / isolated one-line control | Retail MAD (3.9% bound) | Port-frame MAD (0.05% bound) | Background cross |
|---|---:|---:|---|
| Accepted `242544c8` main | 3.344575% | 0% | Vertical/horizontal 2.005/1.778 levels |
| Drop NEVER/ZB_ONLY depth | 3.715799% | 0.667893% | Not a cross-restoration control; changes TV/menu clipping |
| Let RGB_ONLY failures write depth | 3.301759% | 0.140004% | 7.775/6.233 levels; visibly restores the cross |

The RGB_ONLY regression even **improves** aggregate retail MAD while making
the bands wrong. The existing regional/smoke witnesses catch that cross;
the strict port frame additionally catches the independent ZB_ONLY masking
regression that the retail gate admitted.

Independent Linux/llvmpipe controls also fail the new witness: ZB_ONLY
rollback is 0.672042% from the port frame and RGB_ONLY rollback 0.147393%,
against main's 0%. Their retail MADs are respectively 3.793862% and
3.358329%, both below the unchanged 3.9% bound (main: 3.410297%).
The actual cross changes from 1.841/1.901 to 8.142/6.691 luminance levels.
Positive qualification covers all 13 fixtures per renderer (12 scene pins
plus context loss), each printing 0.0000%; all 13 NVIDIA fixtures additionally
reproduce identical RGB bytes in a second fresh process. Negative executables
are isolated diagnostic relinks, never production-qualified replacements.

The census below records the original retail gates on accepted `242544c8`
Windows/NVIDIA and `fafe6e0d` Linux/llvmpipe. The latter changes only
documentation/reviewed signatures, not runtime rendering. “Approximately
0.5 percentage points” includes margins through 0.6 on either backend.
TV Entrance is just outside that current band but also receives a witness:
its GS-affine haze/material repair had no dedicated 4:3 port-frame oracle.
All listed scenes retain independent retail checks; a small residual margin
is not itself evidence that rolling back a historical fix must fail.
The field/context-loss pair shares one immutable RGB frame.

| Original retail gate (`64` twin) | llvmpipe MAD / limit | NVIDIA MAD / limit | Smaller margin (percentage points) |
|---|---:|---:|---:|
| `scene_near_fog` | 4.0144% / 4.15% | 3.6716% / 3.73% | 0.0584 |
| `scene_riverbank_vignette` | 1.2115% / 1.30% | 1.2404% / 1.30% | 0.0596 |
| `scene_arcana_rankup_sparkles` | 2.0985% / 3.65% | 2.0843% / 2.15% | 0.0657 |
| `scene_velvet_windows_blurry` | 0.9347% / 1.05% | 0.9753% / 1.05% | 0.0747 |
| `scene_rain` | 2.6685% / 3.0% | 2.9171% / 3.0% | 0.0829 |
| `scene_tv_world_enter` | 1.5618% / 1.65% | 1.5559% / 1.65% | 0.0882 |
| `scene_shadow_haze` | 1.4949% / 1.65% | 1.5452% / 1.65% | 0.1048 |
| `field_mad`, `field_mad_ctxloss` | 1.2193% / 1.6% | 1.2784% / 1.6% | 0.3216 |
| `scene_sky_black_tint` | 2.2911% / 2.8% | 2.4649% / 2.8% | 0.3351 |
| `scene_save_butterfly_glow` | 2.8199% / 3.4% | 3.0615% / 3.4% | 0.3385 |
| `scene_fog_battle` | 3.4103% / 3.9% | 3.3445% / 3.9% | 0.4897 |
| `scene_tv_entrance` | 3.0419% / 3.8% | 3.1839% / 3.8% | 0.6161 |

Title, battle-floor-A and Marukyu seats lie outside that band on both
backends. Title already has strict renderer-scoped widescreen/physical-frame
witnesses; fog's port witness exercises the shared battle-sector/alpha path.
`render_smoke` independently checks numeric GS-affine colour (unequal clip
W), addressing/filter sampling and RGB_ONLY depth/alpha preservation.
Systems fusion, Daidara and minimap already use port-owned references and
are not misreported as retail comparisons in this census.


### 11.4 Address-dependent NPC collision proxies

The native TV-tunnel failures at 37.5473% were a one-flip animation shift,
not a missing rendering pass: intentionally capturing 75595 instead of 75594
reproduced that exact MAD with the same executable bytes. Before the player
diverged at walk flip 51203, ASLR-on/off RAM already differed in the two
living-room NPC collision proxies, not their visible models. Their scale-30
bases had arbitrary yaw (for example about 29.7 versus 3.7 degrees).

`FldFbnApplyTable` (0015f000, `k_fldFBN.c`) passes `angles.y` to
`mdlRotateMatrix` before the first store to that local. Native return-address
bits therefore became a floating-point yaw. Disabling image ASLR made repeated
trajectories identical but is not a shipping fix. Retail `ng-livingroom`,
`ng-free` and `ng-free2` have axis-aligned proxies; both c33/c35 proxies in
`ng-free2` have diagonal `(30,30,30)` and zero off-diagonal basis components.

The mirror repair models the reused EE yaw slot explicitly: start at zero,
retain it across iterations, and let a kind-0 matrix extraction update the
value used by later entries. It does not rotate every proxy to its visible
NPC's heading or reset the carried value on each iteration; retail's mixed
FBN tables include later nonzero carried yaw. ASLR remains enabled.

Keep this three-rule mirror repair only once when integrating older branches:
an identical earlier triplet consumes the later triplet's match patterns and
correctly fails generation. Remove the duplicate import, not the accepted
repair or its exact one-match expectations; no contract rebaseline is needed.

### 11.5 Flood visibility must retain the EE depth register

Pattern-versus-zero story diagnosis exposed another live uninitialised
read in `FldCellFloodFillDirected` (0015b3e0,
`mdlManager_inferred_001560a0.c:2918`). In the paired Castle walk at flip
39139, player position, camera basis and cell `(6,14)` matched exactly.
That cell was kind 7, with north/south exits and camera yaw about 76.59
degrees (axis mask 8). The kind-7/8 depth-zero extension paths therefore
used `nextDepth` without a C assignment. Pattern and zero already disagreed
on band-resource activity; `FldStepUpdateBandRes` propagated that bit to
the gates used by both game collision and navigation. The first returned
walker mask differed at 39147, before the player heading and game RNG.

This is not repaired by arbitrarily resetting depth. Retail keeps
`nextDepth` in callee-saved `$s2`: 0015bb3c loads the outer caller's
alternate-resource (list 0xB) head into it, and all four recursive call
sites copy `$s2` to the depth argument. The mirror initialises the carry
from that head for the root (`incoming == 0`) and from `depth` for recursive
entries, retaining later branch assignments within each invocation.
Only the low byte is consumed; guest/host page alignment preserves it.
List 0xB was null in the observed Castle pair, but the repair preserves
nonzero retail carry too. The eight-argument ABI and pinned matching
decomp remain unchanged.

### 11.6 Guard-camera height must not consume native stack poison

The remaining New Game pattern/zero difference began with actual game RNG
draw 11620: pattern consumed it at flip 117957 in
`btlCameraFrameStatusRoll`, while zero first consumed it at 118609 for skill
damage. A read-only native caller trace confirmed the divergent camera
action belonged to a genus-0 player, not the enemy action seven flips
earlier. The camera's player path in `btlCameraFrameActionCutscene`
(00225ec0, `btlVoice_inferred.c:328`) added a height to `point.y` before
initialising that component. Pattern's NaN forced the special camera cut
and its random roll; the later stream differed by 62 draws and story
control returned 48 flips apart. The old MT-only walk digest could not
identify that first RNG difference.

Retail's instruction 002260f8 reads that baseline at EE `sp+0x134`.
A software-GS `ng-dreambtl` Guard run recorded the exact pre-load word
`00000000` on four calls (relative frames 54, 222, 390 and 1062), using
`--call-trace 002260f8:FILE --call-mem a25:0x138`; `a25` selects SP in the
existing hook parser. No retail memory was changed. The mirror uses the
existing actor-plane `unitCenter.y`, explicitly zeroed earlier in the same
branch, instead of the unwritten `point.y`. Legitimate camera-cut decisions
and RNG calls remain intact; neither the RNG nor a comparison is suppressed.

## Optional widescreen: projection and layout

`video.aspect` (`P4PC_ASPECT`) defaults to `4:3`; `16:9` and `auto`
are opt-in. This is not `video.fit = stretch`: it changes the horizontal
camera extent while retaining the vertical field of view. The setting is
registered with the port settings overlay and applies on restart; `auto`
then follows window resizes without restarting.

The [PCSX2 SLUS-21782/DEDC3B71 patch](https://raw.githubusercontent.com/PCSX2/pcsx2_patches/main/patches/SLUS-21782_DEDC3B71.pnach)
was used as an address lead, not copied into the native executable. Its
`0x0076111c = 0x3fe38e38` is `gAspectRatio = 16/9`, consumed by
`K_View_SetFov` (`0x004577d0`, `Kosaka/k_view.c`) to compute
`viewWindow.x = gAspectRatio * viewWindow.y`. It also patches partial HUD
setup (`0x00104680`), movie geometry (`0x0046957c`), font widths
(`0x002732b4`, `0x00275dbc`) and a camera zoom path (`0x0026c620`).
Those emulator-specific code patches are not applied here.

The native seam is RenderWare's **core callbacks** `CameraBeginUpdate`
(`0x003e7fb0`) and `CameraEndUpdate` (`0x003e7f50`), before frame
synchronisation. Release camera macros bypass the public
`RwCameraBeginUpdate/EndUpdate` wrappers, so hooking those wrappers is not
sufficient. `projectionType` is at camera +20; +16 is the object sync callback.
`p4gl_aspect_camera_begin/end` temporarily multiply only the main perspective
camera's horizontal window by `chosen aspect / (4/3)`. RenderWare therefore
builds the view matrix **and** frustum planes/corners/bounding box from the
same wider window used by the GL atomic, Im3D and particle paths. Field,
battle, event and dungeon camera setup all use this seam. Shadow-camera
texture targets are excluded; nested updates restore their enclosing state.
The original horizontal window and reciprocal are restored byte-for-byte at
EndUpdate (also on a failed BeginUpdate), so setting a FOV repeatedly never
accumulates an aspect multiplier and game-side camera calculations retain
their original input values. The 4:3 branch does not modify camera data.
The same stored planes feed `RwCameraFrustumTestSphere`, used by field NPC
visibility (`FldRcAtomicFrustumTest`), clumps and primitive billboards.

The game's main coordinate grid stays 640x448. At legacy internal scale 1
the storage target is 853x448 for 16:9; presentation corrects the original
PS2 pixel aspect and fits the picture at 16:9, rather than treating the raw
storage dimensions as square pixels. `auto` updates the aspect and target
between completed frames. Frame-raster reads, destination-alpha copies and
feedback/bloom use the full physical target width.

Im2D has a separate UI canvas: centred 4:3 layouts keep their proportions and
original clip rectangle, so oversized dialogue/portrait quads cannot leak into
the additional world area. Clipped shaders are compiled only when used by a
wide UI draw. Explicit ordering-table draw scopes put the calendar/weather,
battle/field party portraits and loading indicator at the right edge, and the
battle command wheel, field location banner and compact dungeon map at the
left. The expanded map stays centred.

PSC foreground panels have a separate pixel-aspect normaliser:
`pscBuildModelMatrix` (`0x0036abd0`; arithmetic at `0x0036ac10..0x0036ac40`)
scales panel Y by `viewport.width * viewWindow.y / (viewport.height * viewWindow.x)`.
That layout calculation reads the camera seam's saved retail X, not its live
Hor+ X; otherwise Shuffle/Arcana cards become shorter by the width multiplier.
The live projection and culling window remain wide throughout the draw.

World/effect draws and framebuffer reads use the complete canvas, not the
centred UI transform. The shared flat/wipe/radial fade callback
(`0x001221a0`), untextured full-width overlays, letterbox bars and the
field's segmented fog/noise and both colour-filter grid layers also cover the
complete target. The party HUD shares a filter layer but retains its own
right-anchored callback scope.
The fade policy follows the producer, not an individual wipe/mesh tile's bounds.
The title photograph is a separate single-image background. It uniformly zooms
about the canvas centre to cover the chosen aspect; the logo, silhouettes and
text retain their existing layout. Nine photograph copies (including a
depth-10 transition) and the compositor's brightness copy use this same rule.
Retail explicitly authors the title aperture as rectangles `{0,0,640,42}` and
`{0,406,640,42}` at `0x005e5590`/`0x005e55a0`: their vertical positions remain
unchanged, but the solid bars span the complete canvas.
For untextured full-canvas layers, the GS half-cell offset remains one native
half-cell instead of being widened with the geometry. This closes the exposed
first column of title bars, fades and solid filter grids without changing
foreground layouts, texture coordinates, frame-feedback phase or the 4:3 path.

Screen-model queues `0x00795050` (battle cut-ins) and
`0x00796520..0x00796640` (the seven generic cinematic cut-in layers) preserve
foreground art proportions. The all-out composition is layered geometry, not
one flattened picture: its orange `haikei_koma` planes, black `koma_moyou` /
`haikei_moyou_kuro` polygons and matching white `siro_line` dividers receive the
same uniform cover transform. Width-only scaling changes their diagonal seams;
leaving the dividers fitted exposes the intentional panel gaps outside 4:3.
Portraits and the central exclamation mark retain their original fitted
transform. These material-level rules live in the renderer; no guest vertices,
textures or event logic are changed.
There are no renderer-inserted 4:3 side mattes.

Shuffle Time is also layered. `shuffle_w.bin`/`shuffle_s.bin` contain separate
sprite packs: `sf_time02.spr` supplies the radial pulse/ring images, which use
uniform centred cover; `sf_time03.spr` record 0 is only a 239x448 ornament
(`{3,2,242,450}` texture bounds), so it stays unstretched at the left edge.
Wave strips, pattern lines and side quads extend their procedural geometry.
The separate card-selection ring, foreground cues, Persona/card art, roster
and labels retain the centred layout. The old late foreground matte and its
producer notifications are removed.

`video.movie_fit` is independent of world/UI aspect. `bars` (default) retains
the movie's original picture and authored aperture; `zoom` uniformly covers
the chosen canvas and crops centrally; `stretch` fills non-uniformly. The
actual Sofdec quad submission alone selects this rule, using its bounds so
both 448-high and 368-high movies work. It does not replace/upscale textures
or mutate the game's vertex array. **Movie fit** is registered in settings
and applies immediately, unlike the restart-only aspect choice.

### Full-screen and 4:3-bounded layer inventory

The private capture helper runs the same cases at both aspects. Paths below
are relative to each aspect's output directory; a 73-card entry/settled-frame
sweep additionally covers the saved field, weather, late-dungeon and cleared
states. This is a construction inventory, not an attempt to stretch every
foreground rectangle into a background.

| Surface / construction | Wide policy | Screenshot witnesses |
|---|---|---|
| Title: nine corridor-photo/transition passes and one brightness pass; separate solid aperture and foreground models/text | Single-image uniform cover; preserve aperture Y and extend solid X; logo, silhouettes and text retain their layout | `title/title.png.win.png` |
| All-out attack: solid intro backing, orange planes, black patterns, white dividers and independent portrait/icon materials | Horizontal fill for the untextured `bord` backing, preserving its Y aperture; uniform cover for panels/dividers/portrait masks, inverse UV scaling for unstretched faces and matched glints; icons remain fitted | `allout/frame_6640.png.win.png`, `allout/frame_6660.png.win.png` |
| Skill/critical eye cut-ins: separate streak, flash, additive layers, eye image, glasses glints and punctuation | Horizontal fill for `back`, `flash`, `kasan01` and `kasan02`, preserving the retail strip height; eye image, glints and punctuation retain their centred, unstretched animation | `allout/frame_6180.png.win.png` |
| Shuffle: two radial background images, a partial left ornament, procedural wave/pattern/side geometry, separate selection ring/cards/cues | Uniform cover for radial images; left-anchor the 239x448 ornament; extend procedural geometry; retain foreground proportions | `training/frame_9160.png.win.png`, `training/frame_20200.png.win.png` |
| Persona-get: real Slime reward card and acquisition message over Shuffle's layered backdrop | Full-canvas backdrop with the same image/procedural rules; card and message remain centred and unstretched | `persona-get/frame_10000.png.win.png` |
| Persona creation/status: solid/patterned backdrop, separate model and text layers | Backdrop covers the canvas; foreground remains a centred, unstretched layout | `fusion/frame_5800.png.win.png`, `training/frame_16600.png.win.png`, `training/frame_21400.png.win.png` |
| Camp menu, skill/item pages and save UI: solid backdrop and animated strip geometry with independent panels | Retain full-canvas backdrop; panels, labels and models keep their proportions | `menu/menu_item.png.win.png`, `evening/frame_4200.png.win.png` |
| Velvet fusion: real room camera, menu panels, full-screen flashes, separate Persona model | Hor+ room; full-canvas solids; centred menu and dialogue foreground | `fusion/frame_4400.png.win.png`, `fusion/frame_5440.png.win.png` |
| Calendar/date transition: black backing, moving day strip, numbers and icons | Backing fills; date typography and strip remain an unstretched composition | `calendar/frame_1628.png.win.png` |
| Social Link: real world beneath darkening, additive sparks, Arcana card and centred text | Full-canvas darkening; preserve the world projection and independent card/text art | `sociallink/frame_7040.png.win.png`, `sociallink/frame_7200.png.win.png` |
| Evening/weather: field lighting/filter layers, date/weather corner and intervening fades | Hor+ world and full-width filters/fades; corner stays right-anchored | `evening/frame_1800.png.win.png`, `evening/frame_2680.png.win.png` |
| TV transitions: composited static/tunnel layers, frame reads and interleaved SFD scenes | Procedural/frame layers cover; actual movies use only the movie-fit policy | `tv/frame_65840.png.win.png`, `tv/frame_68840.png.win.png` |
| Boss event and battle introduction: field/battle cameras plus portraits/dialogue | Hor+ 3D; unchanged foreground proportions and full-width solid transitions | `bossintro/frame_3500.png.win.png`, `bossintro/frame_5900.png.win.png` |
| Late boss introduction: actual Heaven/Namatame event, transformation SFD and Kunino-Sagiri arena | Answer all three native inquiries with legal input; movie retains its requested fit, then Hor+ arena and edge-anchored battle HUD cover the canvas | `latebossintro/frame_6980.png.win.png`, `latebossintro/frame_9020.png.win.png`, `latebossintro/frame_11000.png.win.png` |
| Game over: real Velvet Room camera and fifteen independently faded scrolling text rows | Hor+ room, centred original text; captured by losing a real saved boss fight with legal Guard inputs, not a title-start battle fixture | `gameover/frame_14400.png.win.png` |
| Event letterboxes, flashes, fades and area wipes: solid quads or segmented procedural geometry | Preserve authored Y/timing and cover the full native width, including the first cell | `bossintro/`, `calendar/`, `fusion/` sequences |
| SFD: coded movie pixels on a 448-high or 368-high authored quad | `bars`, uniform centred `zoom`, or non-uniform `stretch`; no content-based black-pixel detection or replacement textures | `movie-bars/frame_800.png.win.png`, `movie-zoom/frame_800.png.win.png`, `movie-stretch/frame_800.png.win.png` |

Compared with the previous wide path, the all-out and Shuffle backgrounds no
longer sit behind opaque 4:3 side mattes. Authored background panels and their
structural dividers cover uniformly; procedural geometry extends independently
of foreground portraits and cards. Single-image backgrounds likewise cover
uniformly rather than inheriting a width-only fill. Other listed foreground
layouts retain their existing policy. The generic solid-fill correction only
adds coverage of native cell zero; it does not change authored vertical apertures,
frame-feedback UV or the default 4:3 transport.

All-out portraits are polygon-clipped EPL meshes (`koma_a` through `koma_d`),
not rectangular sprites. Their original 640-wide outer crop becomes visible
inside a wide panel if only the background/dividers cover. The atomic seam
therefore covers the authored polygon mask and counter-scales its UV mapping.
The dominant authored crop edge determines the image anchor; left/right art
retains its retail screen-edge placement, while top/bottom art stays centred.
For shallow diagonal panels, the side anchor follows that diagonal vertically,
so the image's bottom/top crop cannot float inside the panel. Three-member
panels retain their steep-divider vertical placement. The geometry cache owns
the UV basis and hull topology; only the animated projection is recomputed.
The matching `megane_kiran_*` sparkle translates with its portrait within the
same ordering-table scope. Expanded masks neither repeat the portrait texture
nor sample across its opposite edge: out-of-image UVs are absent and the
remaining filter footprint clamps to the image's texel centres. The 4:3 path
never enables this policy or changes guest vertices, UVs or sampler state.

The all-out intro's untextured `bord` plane is a separate backing, not one of
the `haikei_*` panels. At flip 6640 it covers the complete 4:3 frame, so its X
extent must fill the wide canvas too. Cast/critical `SYKN_*` resources separate
their scrolling streak/flash layers from the eye image; only those backgrounds
widen in X. Neither the strip's height nor the face/glasses/punctuation scales.
The common names `back` and `flash` are classified only in the battle model
ordering table, not in generic circular communication panels.

The complete `SYKN_1..8`, `SYKN_E109` and `SYKN_E112` asset inventory uses
these same four background roles. Two scrolling-width variants share the
same vertical bounds; every `koma_01` eye image has the same centred aperture.
The production hit-row cut-in path selects this kind-4 resource family, not
a separate critical-only layout. The real One More cue is not a full-width
background; the normal saved-battle witnesses preserve its circular shape at
16:9 and 21:9. No cut-in-specific camera, party mutation or asset replacement
is part of the port's layout policy.

The single-image backgrounds found in these audited paths are the title
photograph and the two Shuffle radial images; all use uniform cover. The
239x448 Shuffle ornament, cards, portraits and the circular communication
panels in the generic `cutin/C*_*.EPL` resources are partial foreground/layer
art, not flattened full-screen backgrounds. Pre-rendered SFD pixels likewise
remain movies, including any letterboxing encoded into the picture itself.


`render_smoke` exercises a wide framebuffer, the new horizontal projection
boundary, unchanged vertical projection, exact window restoration, nested
shadow-camera exclusion, centred/right/left-anchored Im2D shader pixels,
split fog/colour-grid and segmented fade coverage, clipped UI overscan,
fitted model foregrounds without inserted side mattes, AA1 coverage in the
transformed UI canvas, and all movie-fit modes at 4:3/16:9/21:9, including the
368-high aperture and immutable producer vertices.
The `widescreen_*_16_9` scene gates use separately named native port-frame
goldens; the established 4:3 retail references are not replaced.
The original wide frames were captured on Mesa llvmpipe. They require the
exact `GL_RENDERER` identity `llvmpipe (LLVM 19.1.7, 256 bits)`; the separate
`_nvidia64` x64 cases and `*-nvidia-rtx4080-trunc.rgb` port frames (10.6a) require
`NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2`. `P4PC_GATE_RENDERER` is an
explicit, case-insensitive exact reference-test scope: another loaded
renderer exits 77 before gameplay rather than comparing incompatible colour
quantization (a family substring cannot accept an ANGLE backend). It is unset
for player runs and the existing retail gates.
`renderer_reference_scope` proves that a mismatched renderer cannot silently pass.
The 0.1% limits and all existing reference bytes remain unchanged. Native
NVIDIA 610.88 retries were byte-identical, but differed from llvmpipe mostly
by 1–3 colour levels (field 0.1705%, fog battle 0.3780%), not pose or timing.

Full-canvas regressions include independent `widescreen_title_cover_*`,
`widescreen_allout_masked_*` and `widescreen_shuffle_full_*` cases at both 16:9
and 21:9. Their `port-wide-title-cover-*`, `port-wide-allout-masked-*` and
`port-wide-shuffle-full-*` references are separately renderer-scoped, not
replacements for the field/battle/dialogue or physical-resolution frames.
The title cases also invoke `title_edges.py` on both the native frame and
presenter image: the top row and the left bar cells must contain **zero**
nonblack pixels. A small image MAD cannot waive that boundary condition.
The independent `widescreen_allout_intro_*` (flip 6640) and
`widescreen_cast_eyes_*` (flip 6180) cases also run at both aspects and both
qualified renderers. Their new `port-wide-allout-intro-*` and
`port-wide-cast-eyes-*` references are captured with one four-frame draw
window per process, not a densely sampled visual-audit sequence.
`cutin_edges.py` additionally requires zero leaked scene pixels through the
opaque orange intro, plus blue strip coverage at both screen edges with its
retail vertical aperture, in both native and presenter captures. Neither a
small image MAD nor an enlarged band can hide a missing or stretched edge.

The shuffle frame is flip 9160 of `save0420_5f_to_training.walk`, an absolute
flip on a timeline that the shared effect RNG (0x922de0) steers. Once
RenderWare's vector-multiply slots ran the sky driver's VU0 routines
(`src/platform/rw_skytran.c`, bit-exact with the PCSX2 recordings), transformed
effect positions moved by an ulp. From flip 7791 the bank drew a different
number of values per frame, and the battle reached Shuffle Time a few frames
later: flip 9160 now shows the title banner rather than the card spread, a
26.2–26.6% MAD against the earlier frames. Story progress is unchanged
(`castle_training_0420` still reaches its destination). Neither timeline is
retail's: that battle's effect draws already depend on PC load timing
(`docs/lockstep.md`). The four frames were recaptured from the new build as
`port-wide-shuffle-full-vu0-{16-9,21-9}-{nvidia-rtx4080,llvmpipe19}`, and the
earlier names stay pinned. Because the capture is tied to an absolute flip,
any change to the effect-RNG draw count moves it again; anchoring it to the
card-spread state would remove that sensitivity.

For private visual qualification, run
`tools/widescreen_capture.sh <absolute-p4pc> OUT 16:9|21:9 [scene ...]`
with `P4PC_ISO` and `P4PC_ELF` set to the private retail inputs and `OUT`
outside git. `P4PC_SAVES_DIR` supplies the named story seeds. The default early/late
boss-introduction and game-over cases also require `P4PC_WIDE_CHECKPOINTS`, the
private 73-card collection with `card1-slot10/mc0` and `card4-slot09/mc0`;
`checkpoint-ID` cases additionally require `P4PC_WIDE_CHECKPOINT_INDEX` pointing
to its `index.json`. The helper uses offscreen SDL, virtual-only input/clock,
aspect-native `scale:1` storage and private save clones, retaining native PNGs
plus actual presenter `.win.png` images.
The `21:9` case selects `auto` at a 1680x720 window. Default cases cover title,
Inaba, Junes, school, dungeon, battle, all-out attack, dialogue, menus, fusion,
calendar, TV, Shuffle Time, Persona-get, Social Link, evening/weather, early and
late boss introductions, game over and all three movie-fit modes.

## 12. Arbitrary physical rendering resolution

`video.resolution` (`P4PC_RESOLUTION`) selects `native`, `WxH`, or `scale:N`.
The default remains `scale:1` and the original direct 640x448 gate path.
Aspect/Hor+ and UI anchoring are independent policies: the game's main
coordinates remain 640x448, while the context resolves exact physical storage.
Native uses the fitted image rectangle in drawable pixels, including high-DPI
output; explicit dimensions permit independent non-integer axis ratios.

The shared resize seam, `p4gl_readback_configure`, takes physical pixel sizes.
Resolution adjustments preview that same config policy without altering its
cache or file. The checked configure variant prepares and copies the candidate
before its persistence callback commits the INI; allocation/copy or persistence
failure retains the old target, pixels, bindings and viewport. Only then do the
presenter's active policy and resizable state switch. The settings witness
rejects an actual out-of-limit target before any save, and uses a real blocked
INI temporary path to check same-size and full row3 callback rollback.
All main color/depth storage, viewports, captures, DATE snapshots and draw/display
raster copies use those actual dimensions. Feedback UVs remain normalized.
Camera-texture rasters retain their guest dimensions and projection but scale
their physical color/depth/output images at the main virtual-pixel density;
nested cameras restore both logical and physical target dimensions. Frame-copy
and camera-clear helpers preserve both read and draw framebuffer bindings.

Main/camera allocations check texture and renderbuffer limits before narrowing
physical-size arithmetic or replacing storage. Camera setup failure returns
failure without consuming a camera-stack entry or silently drawing into the
main target. The fractional witness checks independent target corner colours,
restored viewports/readback, limit rejection and a width that would wrap to one
pixel if narrowed before validation.
Captures check addressable byte counts and share one image plus a scratch row
for the top-down flip, rather than keeping two full-resolution images. The
capture witness round-trips an asymmetric 3x2 gradient through the real frame
capture path, checks raw RGB dimensions/orientation, and covers 1x1 readback.

GS 12.4 quantization and the +0.5 center offset stay in virtual coordinates.
The probed top-left tie correction therefore compensates derivatives by the
actual per-axis/canvas density, not by one physical pixel. The nearest-texel
tie bias of frame/camera textures uses their virtual texel dimensions rather
than changing with backing storage.

Axis-aligned Im2D rectangles retain the GS's **logical-cell coverage**, rather
than exposing the subpixel sliver between a virtual edge and pixel 0. The
unchanged 12.4/+0.5 and canvas transform determine covered samples in the
aspect-native 1x viewport: 640x448 normally, 853x448 at 16:9, 1120x448 at 21:9.
The game-coordinate viewport remains 640x448. Auxiliary rasters retain their
own native dimensions. Each covered native sample owns its entire physical
cell; the default identity case simplifies to `[ceil(left), ceil(right))` and
`[ceil(top), ceil(bottom))`. The nominal-width cache is shared by resolution
selection, render-target density and cell sampling, not inferred from the
requested physical target.

An instanced conservative envelope evaluates this mask explicitly. Lists,
strips and fans retain their authored diagonal/corner attributes, source order,
one DATE snapshot and AFAIL replay; mixed nonrectangular triangles stay on the
continuous path. Scratch/instance data are reused, with no rectangle transport
or shader change at exact aspect-native 1x, including nonidentity UI canvases.
This is not a global half-pixel shift.

Retail sprite colour, Z and atlas UV are evaluated at the original native
sample, inverse-mapped through the canvas; flooring raw game coordinates would
change widescreen 1x portrait/text sampling. Camera/frame feedback UV remain
continuous so a screen-space
copy does not collapse enhanced 3D back to 640x448. Continuous copies retain the
original virtual +0.5 centre's interpolation phase, independently of cell
coverage. The smoke witness compares complete 4:3 and 16:9 native-cell
coverage/texels at 1366x768 and 1277x954, including the title letterbox boundary,
adjacent fractional panels, mixed runs and indexed fans. One-physical-pixel 3D
stripes in both axes
must survive feedback with the same interior UV phase as the continuous shader;
a smooth gradient alone cannot detect this half-virtual-pixel shift.
The real title gates additionally run `tests/title_edges.py` on both the frame
and 1277x954 presenter PNG: row 0 and column 0 inside the top bar must have **zero**
nonblack pixels.
The wrapper uses the runner-selected `P4PC_PYTHON` (the same convention as
`ref_guard.sh`), so a native Windows gate never sends its drive-qualified script
path to an unrelated MSYS `python3`.

Im3D lists/strips retain a one-virtual-pixel stroke at non-native densities.
Portable GLES cannot rely on wide `GL_LINES`, so the renderer clips each segment
in homogeneous space and expands it to a quad in virtual screen coordinates.
Colours retain the existing screen-affine rule and UV retain perspective
interpolation; depth, alpha-failure replay and source draw-log/skip identity are
unchanged. CPU scratch and the immutable GPU index pattern are reused. Exact
1:1 targets retain the original native line path.

For 2D, integer and fractional grids retain the game's NEAREST/LINEAR sampler.
Promoting nearest atlas sprites to linear can bleed adjacent tiles or erase an
opaque glyph stem under a strict alpha comparison. Even FB_ONLY/RGB_ONLY retain
that comparison for Z writes, and filtered alpha can change DATE without ATE.
GS alpha tests, masks, destination alpha and depth therefore retain their
authored texel inputs. This is not an asset-resolution upgrade: glyph/portrait
source textures remain retail. The fractional smoke witness covers authored
nearest/linear sampling, a one-texel stem with GEQUAL128 under KEEP/FB_ONLY/RGB_ONLY,
nested camera targets, indexed/list/strip Im3D strokes with eye-plane clipping,
and an independent rational-area reference oracle.

Scene reference checks never silently skip a scaled frame. Exact-size RGB
references compare directly; 640x448 references compare after exact box/area
integration with integer overlap weights, independently per axis. The fog seam
metric uses that same virtual background region at every physical resolution.
`tools/scale_diff.py` adds fractional area resampling and bounded physical shift
search with edge extension, not `np.roll` across unrelated picture edges.

### 12.1 Physical regression fixtures

The following distinct RGB files are pinned in `tests/refs.sha256`; no existing
retail or widescreen reference is replaced. The same cases have i386 and x64
registrations (the latter append `64` to the test name).

| Gate | Physical target | Witness |
| --- | --- | --- |
| `scene_resolution_title1080` | 1920x1080 | Title400, draw window4 |
| `scene_resolution_title1366_cells` | 1366x768 | Title frame and1277x954 presenter; zero top/left-bar leaks |
| `scene_resolution_title1277_cells` | 1277x954 | Title frame and same-size presenter; zero top/left-bar leaks |
| `scene_resolution_field1366` | 1366x768 | Field7:2, draw window4 |
| `scene_resolution_butterfly1440` | 2560x1440 | Field8:2 butterfly/glow with the fixed date/slot/camera fixture |
| `scene_resolution_fog4k` | 3840x2160 | Encounter532, legacy flip1700; unchanged3.0-level fog discontinuity bound |

These regression fixtures supplement, rather than substitute for, native GPU
qualification across the explicit targets, scale2/scale4 and active aspects.
For `native`, resize/minimize/restore and GL-context replacement must retain a
valid fitted target. A one-to-one presenter crop must equal the main frame and
the surrounding fit bars must stay black; a successful process exit alone is
not this pixel validation.
