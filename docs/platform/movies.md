# Movies

[Platform index](../platform.md) · [Detailed player contract](../movie_player.md) · [Audio](../audio.md)

## Decode and game seam

`src/movie/` replaces the MWPLY API the game calls, not its surrounding movie task.
`mwply.c` uses the PC VFS, SFD demuxer, libavcodec MPEG decoder and ADX decoder; it does
not route movie bytes through CRI audio stream rings. The normal path does not run Sofdec's
SFD ring/MPS/MPV/server stack. Native `sfmpvp.c` retains independently tested Sofdec IPU
put/search/decode behavior for callers of that lower-level seam.

The disc's 23 `DATA/SFD/*.SFD` files contain MPEG program streams: video stream `0xe0`,
640x448 (P3CTOP 640x368), progressive 29.97 fps I/P/B pictures, generally MPEG-1 video
with P3CTOP MPEG-2 extensions. Except P4SR, stream `0xc0` contains 48 kHz stereo ADX.
`p4pc_sfdview --list` or `p4pc_sfdview DATA/SFD/P4CT019.SFD out/ 8 1` inspects/decodes
using the user's `P4PC_ISO`; it writes private PNGs and stream/header counts.

Entry points cover initialization, work sizing, creation (`00509268`), frame sync,
`H_Cdvd_StartPlayback`, get/release current frame, status (`0050d4d0`), loop release and
destroy (`005097e8`). The frame object keeps the game's `0x90`-byte layout: dimensions
at `+0xc/+0x10`, macroblock dimensions `+0x14/+0x18`, rate*1000 at `+0x20`, frame at
`+0x24`. Status is 0 stopped, 1 preparing, 2 playing, 3 ended, 4 error.

The first frame is prepared after 20 GetCurFrm updates. The 29.97 Hz update schedule
returns consecutive pictures while keeping up, no picture when ahead, and catches up to
the newest due frame when late, unless frame sync forbids dropping. The clock prefers
played audio samples, then `P4PC_MOVIE_CLOCK=tick` for deterministic tests, then monotonic
time. Tick mode advances one 1/29.97-second step per GetCurFrm, not per host second.
Audio is decoded/queued as demux reaches it. After audio ends, fallback time continues
from its endpoint. Loops restart until `mwPlyReleaseLp`.

## Upload and invariants

The player normally supplies a 16-byte stamped buffer identifying its held decoded
picture through the game's copy. `movie_upload.c` resolves it and converts only a drawn
frame directly to the raster. This avoids building tiled pictures for undrawn turbo flips.
`P4PC_MOVIE_TILED=1` selects actual tiled buffers for diagnostics. The decoder also provides
Sofdec IPU RGB32/RAW8 layouts: 16x16 macroblocks, column-major tiles, RGB32 alpha `0x80`.
The upload override (`00143ba0`) de-tiles when needed and uploads alpha `0xff` to the
non-CPU-shadow movie raster (`rwRASTERDONTALLOCATE`, `0x584`).

Texture allocation follows GS power-of-two padding: a 640x448 movie uses quad UVs
641/1024 by 449/512. Im2D vertex color is 0..255 and shaders normalize it.
`sceDmaGetChan` / `sceDmaSync` treat host-side DMA channels as idle.

The game's own `movieNativeTaskUpdate` still controls skip/fade/end. Opening movie IDs
`0x15/0x16` accept pad edge mask `0x9ff` (Start, faces, shoulders), not D-pad; others use
Start `0x800`. A task ends in state 6 after skip, table fade-out or player end/error.
An allocation returning no task is not a successfully played movie.

## Picture fitting

`video.movie_fit` (`P4PC_MOVIE_FIT`) defaults to `bars`, preserving the original
4:3 picture and the shorter movie's authored letterboxing. `zoom` uniformly
covers the selected canvas with centred cropping; `stretch` fills it
non-uniformly. Native Config's **Movie fit** row changes this policy live.
The actual movie draw seam applies it without modifying decoded pixels, guest
vertices, camera framing or movie clocks. Black pixels encoded in an SFD remain
part of its picture; the renderer does not guess a content rectangle.


## Coverage

`sfd_decode`, `sfdview_disc`, `sfmpvp` and `mwply` cover demux, display order, frame count,
IPU/RAW8 layout, statuses, loop/release and deterministic scheduling. `movie_smoke` sends
a frame through the actual upload/Im2D/readback path; `movie_task` runs the real task to
completion. Disc tests skip with 77 without `P4PC_ISO`; game/GL tests also require ELF
and a context. `P4PC_MOVIE_INDEX`, `P4PC_MOVIE_SHOTS` and `P4PC_MOVIE_PNG` select the
movie/task capture fixture. See the detailed movie page for audio clocks and acceptance
oracles; historical frame counts are not a substitute for exercising the current player.
