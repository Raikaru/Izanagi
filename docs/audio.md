# Audio

All sound the port makes goes through one mixer (`src/platform/sound/p4snd_mixer.c`, API in `p4snd.h`) and an SDL3 output device. Two producers feed it:

- CRI's ADX (music, voices, movie audio): the game's own CRI code decodes, `AXRNA_*` (`axrna.c`) hands the PCM to a stream voice. Replaces the IOP/SPU2 transfer.
- The IOP sound driver (sound effects): the EE wrappers (`ps2_sndcom.c`) build the retail command packets, `p4sdr_command` (`sdr.c`) is the IOP side, notes are sample voices.

## Output and the mixer

- Device: SDL3 audio stream, 48 kHz stereo float, opened by the first voice (`p4snd_open`). `SDL_AUDIODRIVER=dummy` (or `disk`) selects a headless driver. With no usable device a pacing thread consumes in real time, so stream producers still see a draining buffer.
- `p4snd_render(out, frames)` is the mixer; the SDL callback calls it. Tests call it directly after `p4snd_use_offline()`.
- 64 voices, two kinds. A **stream voice** has a per-channel ring of 16-bit PCM that the producer pushes into; it is resampled linearly from the stream's rate (ADX is 44.1 or 48 kHz) and drains in real time. A **sample voice** plays a decoded mono sample at a rate with loop points and an ADSR envelope (attack, decay, sustain level, sustain fade, release).
- Threads: the game runs on one host thread and calls the API; the SDL audio thread only renders. One mutex guards voice state. The stream rings are single-producer single-consumer with atomic counters, so pushing PCM never waits for the mixer. A tick hook (`p4snd_set_tick_hook`) runs before every 5 ms chunk and is the SE sequencer's clock.
- Gains: `p4snd_cri_gains` maps CRI volume (1/10 dB, 0 full) and pan (-15..15) to left/right gains, both sides full at the centre (balance law). The SPU2 master volume command sets the mixer master gain.

## ADX through CRI

CRI's ADXT is the decomp's (`decomp/src/cri/re4/adx_tlk.c` and the stream joint, decoder and ADXRNA code under it) except for the handle server and the inner create, which are translated from the retail asm: `src/cri/adxt_svr.c` (adxt_ExecHndl `0x004d8698` and its state steps; the GameCube `re4/adx_tsvr.c` is excluded) and `adxt_Create` (`0x004d38a8`, `src/cri/adxt_wrap.c`). On the PS2 the decoded PCM leaves ADXT through PS2RNA (an SPU2 ring behind an IOP transfer). In the port `ADXRNA_*` (compiled decomp, `re4/adx_rnaa.c`) calls `AXRNA_*`, which `axrna.c` defines:

| function | behaviour |
|---|---|
| `AXRNA_Create(sjo, nch)` | a stream voice with `nch` rings of 0x4000 samples (the retail ring size); `sjo` is ADXT's array of one mono 16-bit stream joint per channel. The decomp's `ADXRNA_Create(nch, sj)` front end is called by ADXT with `(sjo, maxnch, work)`, so the two arguments arrive in that order; either order is accepted |
| `AXRNA_ExecServer` | for every handle with the transfer on: moves min(reported room, joint data of every channel) samples from each joint's data line (SJ line 1) into the ring and returns the chunks to the free line (line 0). Every pass then records the voice's played count for the report below |
| `AXRNA_SetTransSw(sw)` | the server pulls from the joints only while it is on |
| `AXRNA_SetPlaySw(sw)` | the voice runs while on. Off with the transfer already off is a stop (empties the ring); with it on, a pause |
| `AXRNA_GetNumData` / `GetNumRoom` | what PS2RNA reports: samples pushed minus those played by the end of the server pass three passes back, and 0x2000 (PS2RNA's 0x4000-byte IOP buffer) minus that. Retail's EE sees the IOP consume a sample three passes after it played; the mixer ring is larger so it can hold the played-but-unreported samples. ADXT's start condition, `ADXT_GetTime` and the end-of-stream drain therefore see what retail's EE sees |
| `AXRNA_SetSfreq`, `SetNumChan`, `SetOutVol` (1/10 dB), `SetOutPan` (-15..15 per channel), `DiscardData`, `SetBitPerSmpl`, `SetAdjsfreqFlg`, `SetStmHdInfo`, `EntryErrFunc`, `Init`, `Finish`, `Destroy` | stored or forwarded to the voice |

Checks: `tests/axrna_test.c` drives `AXRNA_*` on CRI's own ring joints (SJRBF): the ring accounting ADXT reads, the transfer, play, pause and stop, volume, and that a stream longer than the ring comes out intact.

`tests/adx_wav_test.c` plays an ADX from the disc (default `BGM/THEME.ADX`, the title theme; the disc has no `title/TITLE_S.ADX`) through `ADXT_Create`, `ADXT_StartSj` and `ADXT_ExecServer`, pulls the mixer at 1/60 s per tick, writes a WAV, and compares the output with an independent reference decode resampled the way the mixer resamples (within 2 LSB), the played time with the ADX header, and checks it is neither silent nor clipped.

The decode path on the PC is CRI's own code with these repairs for the host: the ADX header readers (`ADX_DecodeInfo` and the `ADX_DecodeInfoEx*` family, `ADX_DecodeFooter`, `ADX_CalcHdrInfoLen`, `ADX_ScanInfoCode`) are port C translated from the retail asm (`src/cri/adx_hdr.c`); the big-endian reads of the ADX magic, footer and frame scales and the length argument of `ADXB_DecodeHeader` are `config/mirror_fixes.txt` rows; and `ADXSJD_GetNumChan`/`GetFmtBps` are exchanged (the decomp's symbol names for 0x004ceda8 and 0x004cf018 are swapped relative to what the retail decode-info step calls).

File-backed ADXT playback uses the ROFS volume devices (`VOL`, `BTL`, `BGM`, `ENV`), registered with CVFS when the volumes attach. A bare `VOL:NAME` is relative to the directory list last selected on that volume by `H_Cdvd_SelectDirectory`; a non-null directory-list argument to an open overrides that selection for that open only. Voice archives use the explicit `\SOUND\` list even while the field has another directory selected. BGM and ENV use their container roots. The host maps the game's directory-list pointers to VFS directories; it does not search all basenames. `rofs_open` replays the distinct names and directory-list selections from the retail CRI trace. ROFS transfer states use CVFS's numbering (0 idle, 1 complete, 2 transferring, 3 error), not ADXF's (3 read complete, 4 error).

The ADXT channel getter has the same symbol-name inversion as the decoder getters: retail `0x004d4c58` / `0x004d4c90` returns channels through `0x004ceda8`, whereas the decomp's marked `ADXT_GetFmtBps` returns format bits. `src/cri/adxt_wrap.c` supplies the full-width channel getter, and `config/native_names.txt` binds both wrapper and inner addresses, as well as the decoder's channel/format getters, to their actual semantics. Returning ADX's 4-bit format as a channel count made `ADXT_ExecHndl` walk beyond the two joints at handle +0x18 into the input-buffer pointer and size; the handle layout and server loop were not the cause.

The `adx_stream_event`, `adx_stream_env`, and `adx_stream_voice` tests run the same WAV/reference-decode checks through the file-server path: `ADXT_StartFname` for event SE and ambience, and an AFS partition plus `ADXT_StartAfs` for voice.

`voiced_event_0417` loads the `saves/story0417` checkpoint and runs `repro_0417_night.walk`: going upstairs starts E176, whose voice wait must release so the story reaches Class 2-2 (field 6 room 6) on 04/18 at flip 25000. It does not prove a futon interaction or after-school free control; `school_0418` continues through the morning contact-information choice and saves after school. `adx_stream_voice` separately captures member 1 of E176's `V176001.AFS` to WAV and compares its PCM with the source ADX decode.

### End of a voice

The PS2 server ends a handle the way `adxt_svr.c` does. PLAYING becomes PLAYEND_WAIT once the decoder has ended and every output joint holds less than 64 bytes; the renderer keeps transferring. Each PLAYEND_WAIT pass pads every output joint with up to `maxdecsmpl` samples of silence and adds them to `flush_nsmpl` (`+0xa8`, cleared by the decoder-information step; retail's sample-time getter `0x004d4630` subtracts it). The handle is PLAYEND when the joints' data plus the renderer's data minus that padding is no longer positive, that is once every real sample has been played. The re4 server instead turned the transfer off and waited for an empty renderer. Retail's inner create leaves no renderer work in front of the file ring, so the battle's two-channel ACSSND voices get a 39-sector ring with a reload threshold of 33; the re4 create gave 31 and 26.

Three things decide the PLAYEND game update, and all three follow the console:

- **Audio.** On a virtual clock the kernel renders each VBLANK period's audio (`p4snd_virtual_tick` as `p4pc_vblank_period_hook`), not one period per flip. Battle runs at two VBLANKs per update, so a 24 kHz voice drains about 800 samples per update, as on retail. Before this the port drained 400, and collision 900's voice (cue 308, 37222 samples) would have ended about 38 updates late. Real-time play is unaffected: the SDL device and the pacer consume in real time.
- **Server.** The CRI server wake-up (`kwlnEnableInterrupts` -> `ADXM_ExecVint`) sits behind the game's GS-field gate on the VBLANK chain ([kernel](platform/threads_kernel.md#vblank-and-clocks)). The ADXT server therefore runs once per battle update and `adxt_vsync_cnt` advances once per update, as measured on retail.
- **Renderer report.** PS2RNA reports its IOP buffer (0x4000 bytes, 0x2000 samples) as the EE last heard of it. The IOP side of PS2RNA is not in the decomp, so AXRNA (`src/platform/sound/axrna.c`) models the report from retail's collision 900 voices (24 kHz, 800.8 samples per 30 Hz server pass): the EE hears of consumption in whole 256-sample blocks, as of 2.75 passes (5.5 VBLANKs) back, and the server transfers no more than the reported room. Only a lag in (2.68, 2.78] passes reproduces all of retail's observations: no consumption seen in the third pass after key-on and 768 samples in the fourth, and PLAYEND 50 passes after key-on for the 37222-sample battle voice, 68 for a 51968-sample navigator line and 86 for a 66482-sample one. A plain three-pass lag ended the last one a pass late.

`tests/adxt_playend_test.c` plays a synthetic 24 kHz voice arriving at a fixed update through the retail create and server, on the kernel's own VBLANK chain and virtual audio clock, and checks the PREP, PLAYING and PLAYEND updates and `flush_nsmpl`.

### File requests and publication

`src/platform/files/sce_adxf.c` implements the retail ADXF operations above ADXSTM, following the matched `libadxe/adx_fs.c` operations and the P4 assembly's newer layout. Handles are the sixteen 0x44-byte records at `0x00715db0`, not shared VFS/ROFS windows. Their pointer fields are guest-address words. A handle owns a normal ADXSTM controller; each buffer read creates a real SJRBF joint, assigns it to the controller and starts a bounded stream. Terminal reads release that joint; caller-supplied joints remain caller-owned. Partition parsing stays in `adxf_partition.c`, with its original retail table and explicit-directory lookup.

ADXF does no host I/O and schedules no completion alarm. Its `GetStat` is a pure read. The ADXF server (`0x004c9398`, inner `0x004c93c0`) copies the controller's state and read position into the handle. Consequently a pass ordered STM then ADXF can publish in the same pass, while ADXF then STM publishes on the next ADXF pass. The physical driver beneath CVFS alone decides when a sector transfer is complete. The RE4 ADXF bodies and duplicate host-owned tables are excluded; the incorrectly recovered `ADXF_ExecServer` at `0x004c6f30` and `ADXSTM_ExecServer` at `0x004c7230` no longer hide unrelated retail functions.

`btlSoundLoadBanks` (`0x001f8480`) synchronously loads the three battle AFS tables before the first frame. Its three polling loops retain their completion checks and error retries, with a generic `p4pc_poll_devices()` safepoint before each partition-status query so timers can schedule the real filesystem thread. This models the EE preemption the retail busy loops relied on; it does not directly run a CRI server or make status queries publish completion.

`adxf_partition`/`adxf_partition64` exercise the production ADXF pool, read/start/stop/server code and a 600-entry AFS table with recording STM/SJ fixtures. Hardware/controller completion and ADXF publication are advanced separately to check both server orders and repeated status reads. `cri_stm`/`cri_stm64` cover blocking bind, bounded start and joint setup; the file-stream WAV tests exercise the real controller/ROFS path and pump the file server while an AFS table is loading.

The AFS WAV fixture interns its environment-supplied archive name into guest storage before calling `ADXF_LoadPartitionNw`: ADXF retains the name in a guest-address field, unlike `ADXT_StartFname`, which copies its input. Running the fixture on a guest stack does not relocate strings returned by `getenv`.

### AHX voices

The twelve battle end voices (`SOUND/BATTLE/EM1xx_END.AHX` on `DATA.CVM`) are AHX: an ADX container (coding 0x10 or 0x11, one channel, 24 kHz) around MPEG audio layer II frames of 1152 samples, played through the same ADXT as the music once `ACSSND_AttachAhx` has given it a decoder (`ADXT_AttachAHX`, a 0x3000 byte work area). `src/cri/ahx.c` is the retail decoder translated from the asm (0x004d9348..0x004de2b0): the bit reader on a stream joint (`AHXBSR`), the frame decoder (`AHXDCD`: header, bit allocation, scale factors with the stream's key, dequantisation), the synthesis filter state (`AHXSBF`, with the matrixing `ahxsbf_mult_flt_int_ex`; the sixteen window rows are PortAsmC's), the stream joint decoder (`AHXSJD`) and the ADXB / ADXT hooks `ADXT_AttachAHX` registers. `ADX_DecodeHeader` and its parts (`src/cri/adx_hdr.c`) read the container header. The object tables and constant tables stay at their retail addresses (the work areas are the game's), so the code reads them by address.

`tests/ahx_wav_test.c` plays all twelve battle end voices the way `sdkSndReq` does (the mode 2 parameter block, `ACSSVR_CreatSnd`, `ACSSND_AttachAhx`, `ACSSND_StartMem`, the ACS server step and `ADXT_ExecServer`) into the mixer, and checks each against an independent decoder: libavcodec's MPEG audio layer II decoder on the file's frames (each starts with the MPEG-2 LSF header `0xfff5e0c0`; the bitrate field is nominal, so a frame is padded to the size the header announces), resampled the way the mixer resamples (linear, 24 kHz to 48 kHz) and gain-fitted. The retail synthesis is fixed point, so the difference is a rounding noise of about 6 LSB rms (max 55) on voices of 4500..12600 rms; the test allows 0.5% rms and 330 LSB. The scale factor select bits are scrambled in the stream with a key (`AHXDCD_SetExtString`, default key string at `0x00759558`); the voices decode in libavcodec without descrambling, so the default key is a no-op for them. Every pure function of the decoder is also compared with the retail instructions by `ovdiff` (`tests/ovdiff_subset.txt`): exact over 40 random cases each, except the matrixing on unaligned pointers, where retail's `lq` masks the address and the native does not (callers pass 32-byte aligned buffers).

Two decomp prototypes were wrong for the host and are `config/mirror_fixes.txt` rows: `ADXB_GetFormat` and `ADXB_GetNumLoop` return a `Sint16` but were declared `Sint32` in `adx_sjd.c` (on i386 the caller then compares a register whose upper half is the handle pointer's, so an AHX stream was handled as plain ADX and every stream looked looped). Port code must not call the decomp's narrow-returning getters (`u8`, `s8`, `s16`) through an `int32_t` prototype for the same reason.

### AIX player and Pro Logic II

`src/cri/aixp.c` is the retail AIX player translated from the asm (0x004de2c0..0x004e1200 and the create variants): `aixp_Create` / `aixp_Create3D` (0x004de4c0, 0x004de540; a handle is 0x26c bytes, four slots at 0x0072acc8), start from a file (`func_004de9f8`, `func_004deb38`, by partition `func_004deb58`), a stream joint (`func_004dec30`) or a memory image (`func_004decb8`), stop and destroy, the getters and volume setters, and the state machine `func_004df850` that waits for the demultiplexer, starts every track ADXT on its ring (`ADXT_StartSj`) and reports the end (state 3). The demultiplexer is the AIXSJD library (0x004e0378..0x004e1230; handle 0x195c bytes, four slots at 0x0072b680): `"AIXF"` header parser (`func_004e0778`; the header is accepted only when the chunk the joint returns is exactly its size, and the joint is asked for 0x1800 bytes, so a header is 0x1800 bytes), packet parser (`"AIXP"`, 16-byte packet header: size, track, byte count) and the loop that moves each packet's bytes into its track's ring (`func_004e0db0`), `"AIXE"` ends the stream. The 3D variant creates its tracks with `func_004d3bc0` / `func_004d3c08`, the ADXT create that also attaches a Pro Logic II encoder (`src/cri/pl2_attach.c`: `func_004eb020` attach with its work area, `func_004eb180` parameters, `func_004ea8c8` create, `func_004eacb8` the 32-sample block encoder, the library init and lock wrappers; the single-sample encoder and the hooks are PortPromotedA's `ci_pl.c`).

The game never reaches this code. The disc has no `.aix` files (the only AIX-era content is the twelve battle `.AHX` voices above), and the game's four ACSSND parameter blocks (`0x008d2a60`, `2a88`, `2ab0`, `2ad8`) all have the ADXT kind and no PL2 encoder, so `ACSSND` never creates an AIXP object; the player is reachable only through the ACSSBAIX vtable at 0x00756a50. It is translated for completeness and checked with `ovdiff` (all pure functions exact over 40 cases; the handful that are not are in `tools/mipsdiff/triage.txt`) and by `tests/aixp_wav_test.c`, which wraps `BGM/THEME.ADX` in an AIX (header, 4096-byte packets, end packet), plays it through the player and the ADXT track into the mixer, and requires the output to equal the reference decode exactly and the player to reach its end state.

The player needs the ADXSTM, ADXT and ADXF lock wrappers it calls (`func_004d1c58` / `func_004d1ff8` / `func_004d2348`: native-name rows in `config/native_names.txt`; `func_004d4170`, `func_004d4778`, `func_004c9b80` in `aixp.c`) and memcmp (`func_0043f778`, bound in `src/platform/ps2_libc.c`).

## Movie audio

The movie player decodes the SFD's ADX track to PCM and exposes the audio lane interface of `src/movie/mwply.h`. `movie_audio.c` is its audio side: a mixer tick hook (registered at program start) that finds the current movie, creates a stream voice once the format is known, pulls PCM into it, and reports the frames the mixer has consumed (`p4pc_movie_audio_played`) so the video follows the audio. A movie that ends keeps its voice until its buffered tail has played. `tests/movie_audio_test.c` checks it against a stand-in for the player (output equals the track resampled to 48 kHz within 2 LSB, played count, end of movie).

## The IOP sound driver

The disc loads these IOP modules (`LIB31/`): `LIBSD.IRX` (SPU2 driver), `SDRDRV.IRX` (the `0x80000701` RPC server, libsd's remote side), `SKSOUND.IRX` (Standard Kit sound foundation), `SKHSYNTH.IRX` + `MODHSYN.IRX` (hardware synthesizer), `SKSESQ.IRX` + `MODSESQ.IRX` (SE sequence player), `SKMIDI`/`MODMIDI` and `SKMSIN`/`MODMSIN` (MIDI, unused by the game), `CRI_ADXI.IRX`. Music and voice are ADX (above); sound effects are banks of VAG samples (`spu/comse`, `spu/bse`, `spu/test`: `.hd` header, `.bd` samples, `.sq` SE sequences) played by the synthesizer under the sequence player.

### Where the game talks to it

`sceSdRemote(rsc, command, ...)` (retail `0x00430f80`, a libsd client) reaches the IOP over SIF RPC. The game uses two commands:

| command | use | status |
|---|---|---|
| `0x8010` sceSdSetParam | start-up, `(1, 0x8010, 0x800, 0xfc0)` and `(.., 0x801, 0xfcc)`; also inside the sdr client start | accepted, no effect (SPU2 registers the mixer does not have) |
| `0x90f0` | every sksound packet, below | implemented |

Retail internals the port replaces (never reached): `0x8130 0x8140 0x8160 0x8170 0x81a0 0x81b0 0x81c0 0x81d0` (libsd transfer and callback commands in `sceSdRemote`'s dispatch), `0xe620 0xe630` (sdr thread start and quit). Any other command prints one line and returns 0.

Start-up (`mdlRenderInitializeBuffers` in `sdkSnd`): `sceSdInit` (`0x00430e28`, 0), `0x0043c008` (0), `0x0043c010(0x20)` (sdr client start, 0), `0x0043bb70` (SPU heap init), `0x0043c0c0` sound control commands, two `sceSdRemote(1, 0x8010, ...)`, then six `0x0043bcb8` SPU heap allocations (0x64000, 0xE1000, 0x19000 x4; the first two are the SPU RAM homes of the comse and bse banks).

### The 0x90f0 packet

64 bytes: word 0 = handler id << 24, word 1 = sub-command, words 2.. = arguments (zero filled). The module with that id answers; the result is >= 0 (a handle or 0) or negative (IOP error code):

| result | meaning |
|---|---|
| -101 | bad argument or handle |
| -102 | argument out of range |
| -201 | bank or sequence data not set up |
| -202 | invalid handle |
| -299 | not playing |
| -310 | unknown command |
| -402 | load failed (bank header or sequence data not readable) |

Handler ids and wrappers (`ps2_sndcom.c`, the retail addresses are the function names):

| id | module | wrapper (packet word 0 / word 1) |
|---|---|---|
| 0 | sksound | `0x0043c0c0` (0 / 2) control, `0x0043c180` (0 / 10) voice transfer |
| 1 | skhsynth | `0x0043c230` (0x01000000 / 0) create, `0x0043c308` (0x01000000 / 1) free |
| 4 | sksesq | `0x0043c3b0` (0x04000000 / 0) create, `0x0043c470` (0x04000000 / 1) free, `0x0043c518` (0x04000000 / 2) play and release, `0x0043c5e8` (0x04000000 / 3) stop |
| 2, 3 | skmidi, skmsin | not used by the game: refused with -310, once logged |

### Command set and status

Argument lists are packet words 2.. in order.

**id 0, sksound**

| sub | arguments | meaning | status |
|---|---|---|---|
| 0, 1 | | no-ops in the module | returns 0 |
| 2 | `kind, 1` / `kind, 2` | start / stop the module's timer tick | returns 0 |
| 2 | `3, 0x80, left, right` | master volume (0..127 per side), sent every frame | sets the mixer master gain |
| 2 | `3, 0x82, on, ..` | effect unit enable (core attributes 2, 3) | implemented (the reverb unit on / off) |
| 2 | `3, 0x83, mode, delay, feedback` | effect mode (sceSdSetEffectAttr; bit 8 clears the work area) | implemented: the preset and work area size of the mode (1 room .. 9 pipe, 0 off); echo and delay take no delay or feedback |
| 2 | `3, 0x84, left, right` | effect volume EVOLL, EVOLR, 0..127 | implemented (`x * 0x7fff / 127`) |
| 2 | `0, 0x8f, value` | core attribute 10 (the digital output mode) | accepted, nothing to do |
| 2 | `3, 0x81, value` | core attributes 6 and 7 (mute) | implemented: mutes the master |
| 3 | `0` | read the tick count | returns 0 |
| 10 | `channel, IOP address, SPU address, bytes` | voice transfer, IOP RAM to SPU RAM (sceSdVoiceTrans, waits for the end) | implemented |

**id 1, skhsynth** (banks: 8 entries, instances: 8)

| sub | arguments | meaning | status |
|---|---|---|---|
| 0 | `1, handle, IOP address, bytes` | set a bank entry's `.hd` | implemented |
| 0 | `2, handle, SPU address, bytes` | set its `.bd` | implemented |
| 0 | `3, handle, hd address, hd bytes, bd SPU address, bd bytes` | both; `handle` -1 allocates; returns the entry | implemented |
| 0 | `5, handle, bank entry, 0..15` | load the bank into the synthesizer (sceHSyn_Load): parses the `.hd`; returns the instance handle; -201 unless the entry has both parts; -402 when the header does not parse | implemented |
| 1 | `1`/`2`/`3, handle` | clear `.hd` / `.bd` / the entry | implemented |
| 1 | `5, handle` | unload an instance (stops its notes, drops its players) | implemented |

**id 4, sksesq** (data: 8 entries, players: 8, plays: 8 per player)

| sub | arguments | meaning | status |
|---|---|---|---|
| 0 | `0, handle, IOP address, bytes` | register `.sq` data (sceSESq_Load); `handle` -1 allocates | implemented |
| 0 | `5, instance handle, sq entry` | create a player on a synthesizer instance; the player handle is the instance handle | implemented |
| 1 | `0, handle` | free sq data (refused while a player uses it) | implemented |
| 1 | `5, handle` | free a player and its plays | implemented |
| 2 | `player, 0 or 7, 10, set, sequence` | select a sequence of a set and start it; returns the play handle (the play's number within the player) | implemented |
| 2 | `player, 2, 10, play handle` | release a play (stops its notes) | implemented |
| 3 | `player, 1, 10` | stop the play numbered by word 5, which is always 0 because the wrapper sends three arguments; -299 when it is not playing | implemented |
| 3 | `player, 0 or 3, 10` | other play switches of sceSESq (not issued by the game) | -310, once logged |

A command whose type word (the third argument) is not 10 is ignored with result 0, as in the module.

### Data formats

- **VAG** (`vag.c`): SPU ADPCM, 16-byte blocks of 28 samples, filter pairs (60,0) (115,-52) (98,-55) (122,-60) per 64, flags end / repeat / loop start. `tests/vag_test.c` checks it against vectors worked by hand.
- **`.hd`, `.sq`** (`hd_bank.c`, layouts in `hd_bank.h`): SCEI chunk files. The SE banks have a sample table (offset in the `.bd`, rate) and timbre sets whose tones give the VAG, volume, pan and the two SPU2 ADSR registers. The `.sq` holds sets of sequences: delta, status events at 1000 ticks per second: note on / off (`0x90`, `0xa0`; set, tone, velocity), a pitch ramp (`0xb0`: set, tone, control, time, value) and meta events (`0xc0`: the `0x7f` marker, and the loop ends `0x00` / `0x01` with a loop id, an offset and a repeat count) and the end (`0xff`). Both are what `modsesq` / `modhsyn` do, read from their disassembly. The ramp: every playing note of the tone changes its pitch offset from where it is now by `value / 100` semitones, up for control `0x0e`, down for `0x0f`, over `time` ticks, and keeps the offset (the module keeps 1/128 semitone and the voice pitch is `2^(offset / 1536)`; the mixer voice's rate follows it, at most four octaves up). The loop end: the first time its id is met it takes the repeat count and jumps, later ones count down and fall through at zero (0 repeats: for ever), so a body runs repeats + 1 times; it jumps to the status byte at `offset - 8` of the event bytes, and the target's own delta is not waited for. The disc uses both: five `bse` sequences have ramps (up and down by 400..3000 cents over 1..2 s) and `comse` sequence 0/10 loops a note 24 times. `tests/sdr_test.c` checks both on a synthetic bank (a square wave: octave up and back down, three notes from two repeats).
- A note is a sample voice: the tone's VAG at its own rate (the sequences carry no pitch), gain = velocity x tone x sequence x set volume, pan from the tone, the ADSR converted to seconds.

### Memories

`sdr_mem.c` holds the IOP's 2 MiB RAM and the SPU2's 2 MiB RAM as arrays. `SifSetDma` (`0x00424708`) copies EE memory into IOP RAM; the voice transfer copies IOP RAM to SPU RAM. The IOP heap (`0x00429d10`, `0x00429d90`, first fit over 0x100000..0x1fffff) and the EE-side SPU heap (`0x0043bb70` init, `0x0043bcb8` alloc, first fit from 0x5010 to 0x1c0000; the first allocations return 0x5010, 0x69010, 0x14a010, ...) give the addresses the game stores.

### Not implemented

`.hd` programs and sample sets (music banks), MIDI and MIDI stream input (`skmidi`, `skmsin`), the echo and delay effect modes' delay and feedback parameters, and any sceSdRemote command other than `0x8010` and `0x90f0`. A scan of the whole disc image finds no content for the first three: no MIDI file (`MThd`), no `.hd` with a program, sample set or sample chunk (every `.hd` has the `Vagi` and `Setb` chunks only: 1604 SE banks), and the only mode the game sets is the hall. The `.sq` events in all of those banks are note on / off, the pitch ramps `0x0e` / `0x0f`, the `0x7f` marker and one loop end; the reader abandons a sequence on any other status or control (none occurs).

### Reverb and tone pitch

The SPU2 effect unit is `p4snd_reverb.c`: the SPU's reverb network (two same-side and two cross-side reflection filters, four comb taps and two all-pass filters per side, in a ring buffer of the preset's work area, the 32 libsd register values, run at 24 kHz with the SPU's 39 tap half-band filter around it) scaled by the effect volume. Sample voices have four routing switches (`p4snd_voice_set_mix`: dry left, dry right, reverb left, reverb right); the stream voices (ADX) are dry only. For the SE tones the switches are byte 19 of the tone record: the synthesizer module copies its bits 0..3 to the SPU2 VMIXL, VMIXR, VMIXEL, VMIXER registers (libsd's switch table maps 0x1800, 0x1a00, 0x1900, 0x1b00 to them) of the voice it starts; 116 of the 253 `bse` tones and some `comse` tones route to the reverb. The same record has the tone's pitch: byte 11 in semitones and byte 12 in 1/128 semitone (signed), on top of the VAG's rate (the module adds them to the note and the bend, and the ramp above is added to the same sum).

The game sets the effect up once, in `mdlRenderInitializeBuffers`: mode `0x105` (hall, work area cleared), effect volume 60 of 127 on both sides, effect on, master volume 127. The presets are data of libsd and are read from `LIB31/LIBSD.IRX` on the disc at the first mode command (nine records of 0x44 bytes, found by the room preset's first registers; the work area sizes are the SPU's). Core 1 (master volume 0 in `sksound`'s init) is not modelled; one unit serves the mixer.

The unit follows PCSX2's SPU2 (`pcsx2/SPU2/Reverb.cpp`, `ReverbResample.cpp`: the left side is calculated on even 48 kHz cycles and the right on odd ones, the address base advances every second cycle, the writes are clamped to 16 bits, the resampler window and the doubled up filter are as there). It is checked against a retail run: `p4hook --trace spu` (the pcsx2-hook of the private trace tooling) from the boot to the title screen records the SPU2 register writes of the retail game. For the hall mode all 32 effect registers it writes (APF1_SIZE .. IN_COEF_R) equal the libsd preset this code loads (the address registers are the preset's values times four), its effect area is the 0x56f0 words of the mode, the effect volume is `60 * 0x7fff / 127 = 0x3c78` (as computed here), the master volume of core 0 is 0x3fff, and the mixer register (MMIX) opens the voices to the dry and the effect bus and closes the core input to the effect bus (the CD / ADMA input is not reverberated; the stream voices here are dry only). A second run, `--drive newgame` to the first field, plays SE: its VMIXL / VMIXR writes set one more voice's bit for each new voice (every SE voice is dry on both sides) while VMIXEL / VMIXER, always written with the same mask, set the bit of only some voices, which is the selective routing the tone byte gives (116 of 253 `bse` tones). What is untested against the console is matching the voices to the tones and the audible level (the trace keeps one case per register form and has no audio output).

**Pitch is the retail integer-table conversion, not host `pow`/`exp2`.**
`src/platform/sound/spu_pitch.c` follows actual `LIBSD.IRX` export 13,
`0x3bd0..0x3ce4`; `MODHSYN.IRX` calls it at `0xa8d4` through import `0xbd64`.
The semitone and fine tables are `floor(32768 * 2^(n/12))` and
`floor(32768 * 2^(n/1536))`, generated with integer roots by
`tools/spu_pitch_table.py`. All 140 independently generated coefficients match
the retail module's tables at `0x54c0`/`0x54d8`. The guest sequencer (including
sounds requested by the game's native Config path) uses this Q12 conversion.
Tone pitch and live bend are added
before the single table lookup; existing mixer pacing and rate-cap policy are
unchanged. A zero pitch freezes source position without stopping the envelope.
The former transcendental expression did not model SPU quantization.

`spu_pitch` checks fixed native vectors, `spu_pitch_coefficients` checks every
coefficient against its mathematical definition, and `spu_pitch_retail`
optionally runs the unchanged IRX instructions (`P4PC_LIBSD_IRX`). The latter
executes all 65,536 signed fine-offset values plus six signed-word extremes:
65,542 exact comparisons, digest `c63bc828366831b6` on native Linux and Windows UCRT. Its
interpreter build has an explicit `P4_MIPSDIFF_RAM_LO=0x1000` for linked IOP
addresses; the ordinary EE interpreter keeps its original `0x00100000` floor.
`mixer_zero_pitch` checks that zero freezes a sample and nonzero pitch resumes it.

## Logging and tests

`P4PC_SOUND_LOG=1` prints every 0x90f0 command and its result. A command the driver does not implement prints one line, once, regardless.

- `vag_test`: decoder vectors (no disc).
- `sebank_test`: every `.hd`, `.bd` and `.sq` of the disc's SE banks through the readers: all VAGs decode and end, every tone names a VAG, every sequence reaches its end marker, every note names a tone (needs `P4PC_ISO`).
- `sdr_test`: a synthetic bank through the EE wrappers (result codes, transfer path, note on / off audio, stop) and, with `P4PC_ISO`, the disc's `spu/test` bank.
- `axrna_test`: the AXRNA renderer on CRI's stream joints (needs `P4PC_ISO` and `P4PC_ELF` to boot the image).
- `adx_wav_test`: ADX through CRI's decoder and `AXRNA` to a WAV (needs `P4PC_ISO` and `P4PC_ELF`).
- `ahx_wav_test`: the twelve AHX battle voices through the game's ACSSND objects (`ACSSND_AttachAhx`), the translated decoder and `AXRNA` to a WAV, compared with libavcodec's layer II decoder (needs `P4PC_ISO` and `P4PC_ELF`).
- `aixp_wav_test`: a one-track AIX built from an ADX of the disc through the translated AIX player and the ADXT track it drives, to a WAV that must equal the reference decode exactly (needs `P4PC_ISO` and `P4PC_ELF`).
- `movie_audio_test`: the movie audio hook (no disc).
