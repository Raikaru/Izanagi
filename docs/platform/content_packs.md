# Built-in content packs

Content replacement is a port feature, not a script mod. The approved design is
`local://content-packs-design.md`; implementation shares package discovery,
precedence and source provenance with the file-package loader.

## Stable texture keys

`p4_texture_key` (`include/p4pc/texture_key.h`) returns the lowercase SHA-256
identifier for `p4tex-v1/<hash>`. The exact input bytes are:

1. ASCII `P4TEX` followed by byte 1.
2. Five little-endian u32 values: logical width, height, source format,
   palette format, palette entry count.
3. Packed top-down decoded RGBA8, exactly width * height * 4 bytes.
4. Packed canonical RGBA8 palette in logical index order, including unused
   entries: 16 for PAL4, 256 for PAL8, zero for direct color.

Source format numbers are 1 ARGB1555, 2 RGB565, 3 ARGB4444, 4 L8, 5 RGBA8888,
6 RGB888, 7 RGB555, 8 PAL4, 9 PAL8. Palette format numbers are 0 none,
1 RGBA8888, 2 RGB555-derived. Alpha is the decoded host RGBA8 convention;
PS2 alpha expansion/CLUT swizzling must already have happened. Native structs,
stride, padding, addresses, filename, sampler state and replacement dimensions
are not hashed. A pixel/palette decoding change requires a new key schema.
The key routine allocates nothing and does not touch GL or caller memory.

The same allocation-free SHA-256 implementation is public through
`include/p4pc/content_digest.h`. Native consumers use `p4_content_digest` for
bounded immutable buffers, or caller-owned `P4ContentDigest` with
`p4_content_digest_begin/update/end` for streaming inputs such as ISO fingerprints.
Chunk boundaries do not affect the lowercase hexadecimal result. The public
header has C/C++ linkage guards; there is no private-header compatibility alias.

Research: [PCSX2 texture replacements](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS/Renderers/HW/GSTextureReplacements.cpp)
separate texture/CLUT identity and queue encoding/loading off the GS thread;
[Dolphin texture naming](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/TextureInfo.cpp)
includes dimensions, console texture/TLUT hashes and format. Our decoded-image
key deliberately is not filename-compatible with either emulator. Palette
wildcards are not supported.

## Original-only dump worker

Set `P4PC_TEXTURE_DUMP=<private-output-directory>` before starting the player or
viewer to capture original textures encountered after startup. Leave it unset
for the normal path: no source-index allocation, observers, hashing, workers or
dump-directory access occurs. This does not install/enable any Luau mod. Use the
player's ISO/ELF through the normal VFS; local automated runs remain headless and
go through the admission helper. A viewer can traverse a model, title or field
using its existing arguments. Unvisited assets are not claimed to be dumped.
Normal viewer completion drains the writer and fails on incomplete output.
The native `p4_content_finish` API provides the same result to other tools.

Native battle-worker mode bypasses mod/content startup and content finalization,
including when the dump environment variable is set. Its renderless raster
fast path remains unchanged. Use the player/viewer with a headless GL context
for texture capture; a no-device battle run is not dump qualification.

The platform restart initializer stays first in each executable's outer entry,
before image/game startup or content's exit-handler registration. Capture drains
before ordinary viewer exit/return, retaining a nonzero viewer result and changing
only success to failure for incomplete output. It does not disable/reorder the
platform restart tail, emulate re-entry or change the scheduled fresh-process
restart contract.

The VFS read observer supplies the actual disc/mod/developer provider and rank;
no second package resolver or basename guessing exists. Reads into managed
non-stack guest buffers retain chunked SHA-256 source snapshots. Native malloc
staging is attributable only during an explicit importer-owned read scope, ended
before release on every path. Guest heap release/resizing invalidates source
metadata before poisoning/protection/free, without changing EE allocator layout
or decisions. Byte-copy observers propagate only to those controlled lifetimes;
unobserved/compiler-emitted writes invalidate attribution when snapshots differ.
Lookup is a sorted interval index; verification hashes at most the relevant
64 KiB source chunks, not the entire containing resource for each copied header.
Attribution pressure evicts old proof and returns unknown rather than inventing
an owner. Host allocations of unknown lifetime, coroutine stacks, hardware and
native-image windows are not treated as persistent asset buffers.
The membership predicate is implemented once in native `src/platform/guest.c`;
the shared guest header declares it without a body, so capture-only pointer
casts are not parsed/countable expressions in every mirrored EE unit.

Native owned-memory clients invalidate before unmap/free through
`p4pc_bytes_invalidate(destination,count)`, declared with the canonical byte
observer in `ps2_libc.h`. It sends the existing clear/invalidation event without
writing, copying, hashing or allocating; a missing observer bypasses the callback.
This is distinct from clearing the allocation and does not change its bytes.

Native and TMX imports publish source metadata only after successful decode;
the renderer keys the exact final RGBA8 conversion before any GL padding.
TMX's original palette encoding is retained, while its canonical image format
is the final raster format. Dynamic glyph cache upload 0x00276420 excludes its
raster before the first pixel unlock. Camera/render targets, frame feedback,
movie producers and camera subrasters are excluded before use. Raster destroy,
slot reuse and service shutdown release all port-only source/key metadata.
Palette-only uploads retain the asset origin but get their own content key.
Neither guest raster dimensions/data nor sampling/render state is modified.

The native service in `include/p4pc/texture_dump.h` is owned by the game/tool
thread. Start with an explicit private output root, submit decoded images,
then finish to drain/join and obtain the final success/incomplete result.
An empty root is disabled and performs no filesystem access. It writes
`textures/p4tex-v1/<hash>.png` and `inventory.jsonl`. Each image row includes
format/dimensions, canonical palette bytes, source name, origin and whether
it may be replaced. Submitted caller buffers are copied before returning;
encoding and writes run on the worker. The 24 MiB staging cap includes active
and queued images. Backpressure in this explicit diagnostic mode prevents
one-frame animation variants being dropped. The bounded identity index admits
at most 65,535 distinct keys per service run; exhaustion reports incomplete,
never silently drops output.

Only identified original-disc images are accepted. Static disc font images
can be dumped with `replaceable:false`; framebuffer, movie, procedural,
overridden and unknown sources are excluded and counted by reason. Synthetic
bytes are allowed only by an explicit tool/test initialization argument, never
by normal player configuration. Callers must establish provenance before
submission; the dump service does not infer it from a raster's address/type.
The raster unlock at retail 0x003fd230 and palette unlock at 0x003fd520 are the
canonical post-conversion capture boundary; the native import seam is
0x003fd940 and TMX decode is 0x00463930.
Source tracking and raster uploads run on the thread that enables tracking.
The image conversion path asserts that ownership and retains two bounded
render-owned provenance copies across synchronous locks and uploads: source
index entries themselves may move as tracked buffers are modified.

PNG output is staged and renamed. Resuming an existing directory re-encodes
and verifies an existing PNG byte-for-byte rather than overwriting it; changed
or corrupt output stops the dump visibly. Disk, encoding, allocation, size or
index failures produce an incomplete result. A failed entry is never counted
as completed. Inventory write/flush/close failures also fail completion.
The shared level-1 PNG encoder uses miniz's immutable CRC table, so a dump
worker and an ordinary capture can encode concurrently without lazy-init races
or changing PNG bytes.

## Explicitly authored native resources

`p4gl_raster_set_authored_rgba` publishes original author-supplied RGBA8 to a
fresh, standalone deferred 8888 texture on the game/current-context thread.
It requires the real winning `OVERRIDE` provider, nonempty resource key and
rank; descriptor ownership is not substituted for the actual byte provider.
Existing uploads, movie producers, camera/framebuffer rasters, indexed formats
and marked procedural resources cannot be retagged through this entry point.
Logical dimensions must match the raster; padded dimensions must fit the
device's `GL_MAX_TEXTURE_SIZE` before any allocation or pixel read.

The native raster owns one actual guest-readable, power-of-two-padded RGBA
shadow, not merely a GL image. Successful publication clears `DONTALLOCATE`,
sets the real row stride, preserves all 0..255 alpha values and zeroes padding.
Host decode input is copied and is not retained. The const native
`p4gl_raster_authored_shadow` accessor returns the shadow and immutable allocation
stride/rows without a lock, allocation, conversion, GL call or reupload. A guest
image may borrow it with that stride instead of making a second canonical
pixel copy. A READ lock also exposes the same shadow. The image must not free
raster-owned pixels.
`RASTERDESTROY` releases the shadow and its native provenance exactly once.
This explicit authored ownership works without global source tracking;
original-only dumps still exclude it when tracking is enabled.

## Image container boundaries

`src/platform/mods/p4content_image.c` inspects input through an exact random-read
callback supplied by the common loader's contained file handle. It does not
open paths, discover packages or select providers. File magic selects PNG,
DDS or KTX2, never an extension alone. Inspection and in-place canonical RGBA
transforms allocate no memory.

PNG inspection checks the IHDR CRC, dimensions/color encoding, chunk extents,
consecutive IDATs and the final IEND boundary. APNG containers are rejected:
game animation is represented by separately keyed source frames, not by an
independently clocked replacement animation. Payload decoding remains responsible
for full chunk/DEFLATE integrity.

DDS follows the [Microsoft container layout](https://learn.microsoft.com/windows/win32/direct3ddds/dds-header):
RGBA/BGRA8 and BC1/2/3/7, including complete tiny compressed blocks for NPOT
images. Declared base-row pitch is honored; lower uncompressed mips are tightly
packed. Legacy DXT2/4 and DX10 premultiplied/opaque alpha are explicit metadata,
not guessed from pixel values. Other encodings, arrays, cubes and volume/HDR
images produce errors.

KTX2 follows the [Khronos specification](https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html):
every header/index/DFD/KVD/SGD/mip range is bounded and disjoint. DFD block
geometry, supported LDR models, channel layout and supercompression must agree.
Both current and historical zero-plane descriptors for supercompressed data
are recognized. Orientation defaults to top-left `rd`; other valid directions
and `KTXswizzle` are retained, including composition with BGRA/Basis channel
storage. Premultiplication is undone before the final component mapping.
Color transfer hints do not change the engine's established color-byte space.

The decoder-independent ceiling is 16384 pixels per axis and 15 mip levels;
device/cache limits may be lower. KTX2 DFD/KVD metadata is capped at 4/64 KiB,
with bounded key parsing and no retained unknown-value allocations. Inspection
does not imply a selected image fits the actual GPU or that a compressed
payload has already been decoded.

## Replacement mip preparation

`src/render/p4content_mips.c` provides allocation-free worker-side RGBA8 area
reduction. Independent horizontal/vertical ratios and odd dimensions integrate
every source texel, rather than discarding an odd final row/column. Filtering
averages premultiplied RGB/alpha and stores straight RGBA; transparent results
have zero RGB. Engine color bytes are not gamma/profile transformed.
Positive decoder row strides are accepted directly and padding is ignored, so
generating mips need not first compact a decoder-owned padded RGBA frame.

Atlas rectangles are validated in logical-source coordinates. Pixel-center
projection assigns each replacement texel to at most one cell, including
noninteger scales. Each cell-owned mip region filters only that cell's source
pixels, replicating its edges; unclaimed regions retain ordinary filtering.
Caller-owned, budgeted workspace is bounded by 4096 cells and sorted without
additional allocation. Inputs/output/workspace cannot alias.

An atlas cannot preserve several cells in a single coarse texel. Each cell has
a highest consecutive representable mip level; the replacement sampler must
clamp its LOD and filtering coordinates to that cell's projected region. A
cell that disappears at one level cannot become eligible again just because a
later mip's center happens to land inside it. Unrepresentable base cells are
an incompatible variant, not a reason to sample neighboring artwork.

## Asset-free tests

`texture_key` checks independent hashlib vectors, pointer independence,
palette-only changes and invalid dimensions/formats. `texture_dump` checks
the production worker with original synthetic RGBA/palette patterns, independent
PNG decompression and key reconstruction, immutable staging, deduplication,
source exclusions, resumable output and preservation of corrupt existing output.
These do not need an ISO, a GL context or an audio device.

`content_source` exercises real byte-copy/move/clear observers, source mutation,
mixed-provider rejection, explicit host scopes, chunk boundaries and safe
metadata eviction. `texture_capture` uses the production renderer/guest heap
with generated RGBA/PAL8 images: disabled and capture-enabled guest raster bytes
and every framebuffer byte must match, CSM1 keys must match independent canonical
pixels, palette-only changes acquire a new key, dynamic rasters stay excluded
and recycled raster pointers have no stale metadata. It needs the player's ELF
and a headless GL context, but contains/emits no retail texture fixture.
The same fixture exercises authored NPOT guest shadows with tracking off/on,
unchanged full-range alpha, transparent row padding, host-copy ownership,
read locks/unlocks, real provider metadata and refused movie/frame/procedural
retagging. These author-generated pixels are never labeled disc content.

`content_mips` checks independent rational-integration vectors, alpha-weighted
color, odd and thin images, nonuniform reduction, noninteger atlas boundaries,
cell-edge isolation, maximum cell LOD, invalid bounds and nonaliasing contracts.
It uses no game assets, GL context, codec library or worker allocation.

`content_image` covers real PNG/DDS/KTX2 byte-layout inspection, header corruption,
overlapping/overflowed/truncated mip ranges, input errors, NPOT compressed sizes,
orientation, composed component maps, premultiplied alpha and unchanged padding.
Its files and pixels are original synthetic data; it needs no game, GL or audio.

## Legal boundary

Dump and replace only content from the player's own legitimately obtained disc.
The project supplies tools and original synthetic fixtures, not Atlus textures,
music, voices, fonts or game data. Do not distribute extracted Atlus assets,
encoded derivatives, soundtrack exports or disc images. Replacement authors
must hold the rights to the files they distribute; dumping is not a blanket
legal authorization. Dumps, inventories containing asset bytes and captures
stay outside source, CI fixtures and release packages and are never uploaded
automatically.
