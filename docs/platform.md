# Platform contracts

The platform layer replaces PS2 hardware and operating-system seams without changing game logic.
These pages describe the current implementation, settings and invariants, not a release history.
Repository-relative source paths in the pages identify the implementation owners.

- [Boot, threads and kernel](platform/threads_kernel.md): startup, heap, scheduling, interrupts,
  clocks and TIMER0.
- [Disc and CRI files](platform/disc_cri.md): VFS lookup, read-only SDK calls, completion ordering
  and CRI file windows.
- [Content packs](platform/content_packs.md): built-in content identity, original-only texture
  dumping, source lifetimes and asset-distribution restrictions.
- [Input](platform/input.md): libpad packets, player bindings, request deadlines and isolation.
- [Settings](platform/settings.md): INI precedence, persistence and the paused settings interface.
- [Video and presentation](platform/video_presentation.md): resolution, aspect, movie fitting and pacing.
- [Saves and memory cards](platform/saves_memcard.md): libmc2, imports and save integrity.
- [Playing and packages](platform/playing.md): disc selection, user paths and release contents.
- [Release packaging](platform/packaging.md): Linux AppImages, signed Android APK/AAB,
  library licences, private signing identity, migration and native crash logs.
- [Movies](platform/movies.md): Sofdec seams, decoding and audio/video scheduling.
- [Performance and profiling](platform/performance.md): optimized builds, draw windows and observers.
- [Numeric determinism](platform/numeric_determinism.md): floating-point state and cross-target checks.
- [Game CRT boundary](platform/crt_boundary.md): retail bindings, caller inventories, reviewed host
  adapters and fail-closed object guards.
- [Windows reference build](platform/windows.md): i386 address placement, toolchain and host APIs.
- [macOS](platform/macos.md): the arm64 and x86_64 builds, ANGLE on Metal, the app bundle,
  Gatekeeper and what only real Macs can verify.
- [Testing and harnesses](platform/testing.md): headless tools, capture/debug controls and gates.
- [Playthrough coverage](platform/playthrough_coverage.md): story, dungeon and boss gates from
  June to the endings, and the segments still uncovered.

Related contracts: [audio](audio.md), [renderer](renderer_port.md), [port compilation](porting.md),
[64-bit guest memory](port64.md), and [contribution rules](../CONTRIBUTING.md).

Measurements and receipts in these documents were often produced with the project's private
tooling: its land queue and remote/local runners, the nightly jobs, lockstep and the PCSX2
trace tooling. Those scripts, hosts and the evidence paths they name (`<scratch>`, `<host>`,
`<private data>`) are not part of this repository. Build and test with [BUILDING.md](../BUILDING.md).

## Shared native digests

`include/p4pc/content_digest.h` exposes the allocation-free SHA-256 core in
`src/platform/content_digest.c`. Native consumers call `p4_content_digest` with
their actual bytes and receive 64 lowercase hexadecimal characters plus NUL;
an empty input may use `NULL`. Caller-owned begin/update/end state supports
bounded streaming without a second implementation or a whole-input copy.
The API has C linkage for C++ callers and no renderer, package, file-I/O or
worker dependency. It does not enable content capture or modify guest state.
The asset-free `content_digest` fixture checks standard known-answer vectors,
padding/chunk boundaries, unaligned input, output canaries and streaming reuse;
the existing host-test factory supplies the native 64-bit twins.
