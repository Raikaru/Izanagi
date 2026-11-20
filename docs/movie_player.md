# Movie player contract

The game plays movies through the MWPLY API of CRI Sofdec. The port implements that API itself
(`src/movie/mwply.c`) with its own demuxer/decoder, reading through the VFS.
This page lists the game calls, their results, and the retained retail evidence.

## 1. Entry points and what the callers do with them

Only the movie task calls the player. The task is `sdkSfdPlay`, update `movieNativeTaskUpdate` (0x00468ff0,
`decomp/src/Kosaka/Field/k_fldrc_inferred_00468d10.c`), created by `mwPlayerCreate(parent, movie index, name)`
(0x0046a110) or `mwPlayerCreateReset` (0x0046a1f0). It updates once per two vsyncs, 29.97 Hz.

| Call | Where | What the caller does with it |
|---|---|---|
| `mwPlyInitSfdFx(prm)` 0x0050b3f8 | `sdkSndInit` (sdkSnd.c), `kwlnInitSubsystems` | void; boot only. |
| `mwPlyCalcWorkCprmSfd(prm)` 0x00508310 | task state 2, update 10 | the result plus 0x40 is allocated (0x40000 aligned) and stored as `prm->work`; a zero allocation ends the movie (state 0xB). |
| `func_00509268(prm)` (create) | same update | handle; 0 frees the work and image buffers, but the task still calls `mwPlySetFrmSync(0)` and goes on to start the movie (state 3 with player 0); the port never returns 0 unless all 8 slots are taken. |
| `mwPlySetFrmSync(h, 0)` 0x0050a008 | same | stored; 0 lets the player drop frames to catch up. |
| `H_Cdvd_StartPlayback(h, path, loop)` 0x004561a0 | same | starts the movie; `loop` is 1 when the table's loop-player word (entry +0x14) is set; the path is the table's `sfd/P4CTOP1.sfd` string. |
| `mwPlyGetCurFrm(h, &frame)` 0x0050a1c0 | every update in states 3, 4, 5 | `frame.buffer` (+0x00) != 0 means a new frame: `mwPlayerGetCurrent()` (`iGpffffbaf8`) = `frame.frameNumber` (+0x24); the image (+0xc width, +0x10 height, 4 bytes per pixel) is copied to the task's buffer; `iGpffffbaf4`/`iGpffffbaf0` = macroblock width/height (+0x14/+0x18) for the upload; state 3 counts `fadeInFrames` per received frame up to the table's fade-in length (+0x24) and then enters state 4; state 4 enters state 5 (fade-out) when the table's fade-out word (+0x1c) is set and `frameNumber >= s16 table+0x20`; state 5 counts `fadeOutFrames` per received frame up to the table's length (+0x22) and then enters state 6. A NULL buffer changes nothing (the previous image is drawn again). |
| `mwPlyRelCurFrm(h)` 0x0050aae0 | every update after GetCurFrm | ignored result. |
| `func_0050d4d0(h)` (status) | every update after RelCurFrm | 3 (play end) or 4 (error) ends the task at once (state 6); anything else is stored as `playerStatus` when an image is drawn. Other callers: none. |
| `mwPlyReleaseLp(h)` 0x0050ffc8, `func_005097e8(h)` (destroy) | task states 6, 7..0xB and `mwPlayerTaskDestroy` | cleanup; `mwPlyReleaseLp` only when the table's loop word is set. |
| `mwPlayerGetCurrent()` | `edSrUpdate` (Main/OpEd/ed_sr.c) | **equality test** `== 0xCC6` (frame 3270 of the staff roll movie, index 0x11 = P4CT017): starts the credit scroll. A frame number that is skipped makes the scroll never start. |
| `mwPlayerGetTaskName()` + `kwlnTaskGetTaskByName` | `SeqMovie_IsRunning/End` (sequenceList.c) | "a movie is playing" while the task exists; `SeqMovie_Start` = `mwPlayerCreate(0, index, name)`. |

Consequences for the port: the frame number must advance by exactly one per update once frames flow (no skips
while the game keeps up), it must be the same number retail reports, the first frame must arrive after the same
number of updates (the fade-in and fade-out counters and the end of the task are counted in received frames),
and the status must stay 1 or 2 until the movie ends.

## 2. Movie task tests

`movie_task_opening` and `movie_task_p4ct001` exercise the game's movie task
and compare its transitions with expected behavior. They require the player's
own disc image and executable; without them the tests skip. A test of the task
does not by itself establish audiovisual equivalence on every platform.

In a state-aligned lockstep run with `--disc-online` the port instead answers each poll and status read
of a movie before the initial anchor with the live retail run's own (`docs/lockstep.md`, retail's own waits).

## 3. Audio clock

`src/movie/mwply.h`. The audio subsystem gets the movie's handle from `p4pc_movie_audio_current()`, the format from
`p4pc_movie_audio_format`, PCM from `p4pc_movie_audio_pull` (thread-safe), and reports the sample frames the
device has played with `p4pc_movie_audio_played(h, total)`. After the first report the movie clock is
`played / rate`: frame k is shown when its time has come, so the video waits for the audio and never runs
ahead; the player keeps one second of PCM queued ahead. When the audio ends the clock continues from there.
Without a report the clock is one 1/29.97 s per update (`P4PC_MOVIE_CLOCK=tick`) or the monotonic clock. Tested
in `mwply_test` (audio at half speed -> video at half speed; no audio played -> video waits on frame 0).

## 4. Licence check

`ctest ffmpeg_license` prints the licences of libavcodec, libavutil and libswscale; configure with
`-DP4_RELEASE=ON` and it fails unless all three are LGPL-only builds (no `--enable-gpl`, `--enable-nonfree`).

## 5. The frame path (what a movie frame costs)

Retail's player decodes into the IPU's RGB32 macroblock columns, the game copies that buffer to its own image buffer
(`memcpy(work->imageBuffer, frame.buffer, w * h * 4)` in `movieNativeTaskUpdate`) and `gsUploadRasterTiles` DMAs it to the GS. Nothing
else reads those bytes. The port used to build the macroblock columns (libswscale, then a tiling copy), and the upload turned them back
into a linear image (a detiling copy) for OpenGL; with decoding that was about 55% of a New Game run on a GPU. Now:

- **Stamped buffers.** `mwPlyGetCurFrm` holds the decoded picture (a reference to libavcodec's frame, no copy, `p4mpeg2_frame_hold`) and
  hands out a buffer whose first 16 bytes are a stamp (magic "P4MV", player, serial; `src/movie/mwply.c`). The game's copy carries the
  stamp; `p4pc_movie_upload` asks `p4mpeg2_resolve_stamp` for the picture the stamp stands for. The status, frame numbers and sizes the
  game sees are unchanged. `P4PC_MOVIE_TILED=1` makes the player fill the buffer with the macroblock columns as before (the reference
  for the tests and for A/B runs); `P4PC_MOVIE_LOG=<file>` writes one line per `mwPlyGetCurFrm` answer to compare two runs.
- **Lazy conversion.** The picture is converted when the upload draws it, so a frame whose draws are dropped (`P4PC_TURBO=2` outside the
  draw windows) costs the decode (every frame is decoded: references, clock and state advance exactly as before) and nothing else.
- **GPU conversion** (`src/render/p4gl_yuv.c`). The Y, Cb and Cr planes (a third of the RGBA bytes) are uploaded as R8 textures and a
  fragment shader writes the raster's texture. Integer products, shifts and clamping reproduce the pinned FFmpeg7.1.1
  limited-range BT.601 converter: x86 truncates each signed-high16 product; native AArch64 SIMD uses signed-high15 products and a rounded
  final shift. Android's dependency build disables assembler, so its distinct generic-C model folds
  [FFmpeg's table construction](https://raw.githubusercontent.com/FFmpeg/FFmpeg/n7.1.1/libswscale/yuv2rgb.c)
  into integer chroma offsets and a rounded luma lookup rather than using the NEON model or an idealized matrix.
  The shared literal is `p4gl_yuv_shader.h`; unqualified native CPU families retain the reference CPU converter.
  **Supported platforms allocate no64MiB colour-lookup texture and perform no table-generation/upload step.** The program/pass is built on
  the first movie and reused; successful GPU uploads do not allocate CPU RGBA fallback scratch.
  An interior quarter-step before UNORM8 conversion preserves each integer channel under both truncating and rounding framebuffer contexts,
  including llvmpipe inheriting the EE float mode.
  `P4PC_MOVIE_GPU=0` keeps CPU conversion available. The pass preserves cached GL state (`p4gl_gl_pass_begin/end`).
- **Checks.** Disc-independent `movie_colour_cpu` compares all 2^24 Y/U/V triples with the unchanged converter at both the synthetic
  4096x128 geometry and the movie's 640x448 geometry. `movie_colour_gles` and `movie_colour_gl` additionally compare actual shader RGBA8
  readback for the full cube; these are correctness tests, not gameplay performance measurements. `mwply_test` retains its per-frame
  direct-conversion/macroblock-column and player-state checks on P4CT019. `movie_smoke` (GL) compares the uploaded raster with the CPU
  conversion byte for byte on 120 frames, and `P4PC_MOVIE_GPU=0` gives the same PNG. `movie_task_smoke` with
  `P4PC_MOVIE_DRAW_ON=<update ranges>` (draws off elsewhere) gives the same trace and screenshots as drawing every update.
  GPU proof runs with the game's EE floating-point mode, including llvmpipe worker rounding.
  Desktop `movie_colour_generic_cpu/gles/gl` additionally force libswscale's CPU flags off and exhaustively check the Android-style C model.
  The Android candidate-qualification fixture uses real shipping CPU flags/libraries and a standalone EGL GLES3 pbuffer, with no Java activity or
  mocked GL calls. Its `--clock` mode brackets CLOCK_BOOTTIME against CLOCK_MONOTONIC and reports `_SC_CLK_TCK` for process-start attribution;
  this does not assume the phone has never suspended. A build or an unavailable-context skip is not phone GPU qualification.
  On an Adreno650 phone (GLES3.2), the standalone oracle using the exact accepted Debug APK's FFmpeg/SDL libraries
  compared all16,777,216 YUV triples and the640x448 reference geometry with zero integer-shader RGBA mismatches.
  The accepted LUT Debug build took104ms to build its table. A paired55-second paced opening-movie capture had
  first600-flip mean/max33.983/203.432ms on the LUT and33.775/188.785ms on the integer candidate; image capture
  was enabled, so this is not an isolated conversion benchmark. Flip500 readback matched byte-for-byte; captures
  at700/900 differed, and their decoded PTS were not logged, so no same-frame equality is claimed for those captures.

GPU conversion avoids CPU colour conversion for drawn frames; its benefit
depends on the graphics driver and workload. Diagnostic capture and profiling
add overhead and should not be treated as uninstrumented player performance.

## 6. PC movie input

The game player and standalone decode tools use the same host-VFS demux path.
There is no parallel SJRBF/ADXSTM/CVFS movie-file ring or disc-speed model.
The demuxer reports complete packets or EOF; decoder calls do not pump a
shadow CRI server, advance scheduler time, or synthesize device completion.

The existing20-call preparation phase is retained as the MWPLY API contract,
along with frame/audio release, loop, status and per-update playback behavior.
It is not measured host time or a selectable loading delay.
`movie_start` links the same generated tables as the game and keeps traps
fatal, so a host decoder/library symbol cannot silently resolve to a game
trap. `sfd_decode`, `mwply` and the movie-task witnesses retain their existing
decode, status, frame/audio and caller-order checks.
