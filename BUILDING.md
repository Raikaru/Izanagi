# Building Izanagi

The supported build runs inside the project's Debian container (`ci/Dockerfile`).
It has the pinned clang (llvm-mingw), Zig nightly `0.18.0-dev.935+ead8f5a7a`, SDL3, FFmpeg, Wine and everything
else the build needs. The Windows player is cross-compiled from Linux in the same tree;
there is no MSVC build.

## What you need

- Git, and Docker or Podman.
- The `decomp` submodule: the public
  [Persona4-Decompilation](https://github.com/Raikaru/Persona4-Decompilation) at the
  commit this repository pins.

No game files are needed to build. To play, or to run the game tests, you need a
disc image of your own Persona 4 (USA) disc: see [Bring your own ISO](docs/bring_your_own_iso.md).
`.gitignore` excludes disc images, the executable, memory cards and saves; never commit them.

### The retail executable is optional

`tools/gen_port.py tables` binds every word of the game's data that points at a game
function to that function's native replacement. The addresses of those words, and the
SHA-256 of the retail `SLUS_217.82` they come from, are committed in
`config/retail_fnptr_words.txt`, so the build reads no retail input. Only addresses
are in that file; no bytes of the executable reach the build output or the packages.
The player reads the executable and all game data from your disc image at run time.

With `-DP4_RETAIL=/path/to/SLUS_217.82` the tables step also checks your executable's
SHA-256 against that file and rescans it, and the few tests that read the executable
are registered. `python3 tools/gen_port.py fnptr-words --retail SLUS_217.82` rewrites
the file if `DATA_RANGES` or `CODE_RANGES` in `tools/gen_port.py` change.

## 1. Get the source

```sh
git clone --recurse-submodules https://github.com/Raikaru/Izanagi.git
cd Izanagi
# an existing clone: git submodule update --init decomp
```

## 2. Build the container image

```sh
docker build -t p4pc-ci ci/
```

The build context is `ci/`. The image downloads the pinned llvm-mingw and Zig releases
(checked by SHA-256) and builds SDL3 and an LGPL-only FFmpeg for the Windows targets.
The Zig modules implement the battle simulator and party policies against the existing
C game ABI. To use an already installed pinned compiler, pass `-DP4_ZIG=/path/to/zig`;
outside the container, `sh ci/zig/fetch_zig.sh /unused/install/path` installs it.
The nightly download may disappear upstream; set `P4PC_DL_CACHE` to retain the
checksum-verified archive for reproducible installs.

The pinned nightly was published **2026-11-07**. Its x86_64 Linux archive SHA-256 is
`76258449f1088b6f80359944f9735a4c96e94246e553111155f0af6457ff8de4`;
`cmake/zig/pin.cmake` records the other host archives, including native GitHub ARM.
`ci/Dockerfile.arm64`, the desktop image and the Android image install that same nightly.
Provision candidate images under distinct tags; accepted host image pins change only with
the milestone, never by retagging an image under running checks.

An experimental `zig build` graph is under development; it does not replace
the CMake build or its release checks. `zig build zig-graph-sources` checks its
generated compilation database for duplicate object outputs and compares the
port source lists with the CMake fragments for the configured profile.
The export overlay supplies an explicit public-tree mode marker. Without it,
all private tooling fragments are required; with it, only explicitly private
fragment registrations may be omitted. A missing script for an enabled test
remains a failure. The marker identifies the source-tree mode, not a security
or authenticity boundary.
`P4_ZIG=/path/to/zig python tests/zig_tool_graph_test.py` exercises that
registration behavior in synthetic directories; it is not a release check.
The Persona-rating fixture links a test-only adapter to the real Zig policy;
the production policy objects are checked for accidental `p4pc_test_` exports.
Android Zig binding translation disables Bionic fortify overloads only for
`translate-c`; ordinary C compilation retains the NDK's original flags.
The mirror defaults `-Dport64-opt=O2` and `-Dport64w-opt=O2` match the qualified
CMake recipes, rather than CMake's compile-only mirror defaults.
`zig build zig-graph-profiles` compares the effective optimization levels in
the graph manifest against those recipes and is included in the normal graph
checks. For an intentional diagnostic profile, pass
`-Dgraph-cmake-cache=/path/to/CMakeCache.txt` to compare with the actual
corresponding CMake configuration instead. A different optimization level is
still an assertion failure; this option does not waive parity or game tests.
For COFF links, the pinned Zig driver drops the caller's `--gc-sections` even
at O2. The graph's link helper records the raw compiler discovery separately,
then translates that explicit caller policy through the closed one-row mapping
`--gc-sections` → `/OPT:REF` and executes a corrected discovery. It adds no ICF
policy or implicit garbage collection; an unrecognized GC/ICF/OPT policy fails
closed. The final replay still differs from corrected discovery only by its
map arguments, and both final EXE/PDB/map sets must remain byte-identical.
ELF discovery and link arguments are unchanged.
For Linux 64-bit player and viewer executables, the Zig graph links its own
hidden, zero-valued `__dso_handle` storage, matching executable crtbegin
semantics rather than the ELF header address synthesized by the linker.
The graph's Zig compiler shim replays the compiler frontend for LLVM IR output
so the EE float launcher receives the requested IR file, including when a
dependency file is requested; it does not replace the CMake compiler.
For Windows targets only, the shim omits `-fno-pic`: llvm-mingw 23.1.3 ignores
it (identical frontend relocation jobs, symbols, relocations and object bytes
for i686/x86-64 O0/O2 probes and production x64 objects), while Zig rejects it.
ELF targets retain the flag; the real-driver regression checks both paths.
The shim also preserves standalone Clang's default stack-protector policy and
assertion macros: no implicit stack guards or Release-mode `NDEBUG` are added.
Explicit caller stack-protector and `-D`/`-U` policies remain effective.
When MinGW runtime libraries are selected, the C driver uses their matching sibling
`include` directory before Zig's bundled libc headers (but after caller `-I`
paths). This preserves the Windows rule census without changing its baseline:
Zig's bundled `stdio.h` otherwise adds twelve foreign process parameters in
`src/sce/rofs_mai.c`. Caller `-isystem` paths alone do not precede Zig's defaults.
The C++ driver retains Zig's paired libc++ and CRT headers: putting the standalone
C headers first would shadow libc++'s wrappers and break standard C++ headers.
Both build systems store translated Zig C bindings under
`<local-cache>/c-bindings/<SHA256-of-translated-bytes>/`. Their debug source path
does not depend on the caller's temporary object-output directory. A real-Zig
ReleaseSafe regression compares complete objects from two cold caches and distinct
output directories; this does not waive whole-product or raw-map determinism checks.

Binding checkpoint evidence (not a full-branch qualification): the retained
private qualification record `binding-safe-regression.json`
(SHA-256 `24b8fac3a10eb34a7be1eb3a49ffa96ae659d0f8312d48e4a611a60c7a2fd51a`)
records the pinned-Zig ReleaseSafe regression failing with the original helper
and passing with the candidate. The helper SHA-256 was
`350d2db24d66009176d3cc1e10879b2daa948c891b345e0fe68eba17858ae97b`;
the test SHA-256 was `8a81744c6bf756404c5f9c2516e05e29f96f20fdfe2e3d2add3a67d2473dc005`.
The two isolated cold native controls are recorded in
`/home/vxmdm9fntu/p4pc/lanes/zigz1-gc-89dfddaeb-r7/binding-cold-control/control.json`
(SHA-256 `490a1d4924aed118b5e81573d676983b3e4acf8fce69671e5ca6b7c850c73bfb`).
They used original source `33263d83c10180c3f5383fa73dbaa411ba746503` with
only the explicitly hashed helper override, not the full current branch.
Both complete ELFs were byte-identical, SHA-256
`66aabce42cfc39481dd9a13dfdcc7ebca29e093cc6bb8f8a2a5fcd828aba7ef8`;
the raw maps were unequal, SHA-256
`276c23035f163842de3a986543aba92bc494164dfc4c67b8e14b12e3709b3963` and
`bbd8d32541eb14cbb399326b6a5c92040f83a9ad43c32b0aeed28a1e22e97a6a`.
No map normalization or waiver is claimed. The separate Debug fixture failed
whole-object reproducibility; no Debug reproducibility claim is made.

For Android only, Zig's C-declaration translation disables Bionic's fortified
stdio overloads, which translate-c cannot represent; ordinary C compilation
retains its original fortify settings and the imported libc ABI is unchanged.
The POSIX signal boundary admits unaligned function-pointer sentinels such as
`SIG_IGN` without changing their C ABI; production-object tests cover both
AArch64 and x86-64, including architectures that align callable addresses.
ABI-export fixtures use Clang's reported host triple and retain the explicit
x86-64 case. The native GitHub ARM image installs real amd64 libc headers for
that cross-target case; its lane also provisions the pinned NDK's Bionic sysroot
for native Zig declaration translation, without executing x86-64 NDK tools.
The graph-mode fixture consumes the lane's own public-export marker, or runs
the real exporter on an isolated synthetic repository in private checkouts.

The reusable `luau-zig` module also exposes a Zig `State` interface for protected
source execution. The caller owns its allocator and must deinitialize each state
once. Result strings are views anchored in the VM and expire on the next run,
reset, or state destruction. The interface does not make arbitrary native
callbacks safe across Luau's non-local error unwinding; see
[`State.zig`](luau-zig/src/State.zig) for the complete ownership and error contract.

## 3. Configure and build

Start a shell in the container with the checkout mounted:

```sh
docker run --rm -it \
  -v "$PWD:/src" \
  -v p4pc-ccache:/ccache \
  -w /src p4pc-ci bash
```

Inside the container:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_C_COMPILER=/opt/llvm-mingw/bin/clang \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DP4_PORT64=ON -DP4_PORT64_LINK=ON -DP4_PORT64_OPT=O2 -DP4_PORT64W_OPT=O2

# Linux x86_64 player: build/p4pc64
cmake --build build --target p4pc64

# Windows x64 player: build/win64/p4pc64w.exe and the DLLs beside it
cmake --build build --target p4pc64w

# everything, including the i386 builds and the test programs
cmake --build build
```

`P4_DECOMP` defaults to the `decomp/` submodule; configuration fails early if the
submodule is missing. An explicit checkout also supplies the mirror generator's
retail symbol configuration, including when it is outside the source tree.
Add `-v /path/to/folder/with/SLUS_217.82:/opt/p4:ro` to the
`docker run` line and `-DP4_RETAIL=/opt/p4/SLUS_217.82` to the configure line to check
your executable as described above.

The first build compiles the whole decompilation mirror and takes a while. Later
builds are incremental, and the `p4pc-ccache` volume keeps compiler results across
containers and fresh build trees.
The mirror excludes the decoder units explicitly removed by
`config/inactive_movie_decoder.json` from its declared outputs. Their source and
the manifest remain dependencies: changing either still regenerates the mirror.
An unchanged build no longer regenerates it because a deliberately removed
decoder file is missing.

### Linking against your own SDL3 / FFmpeg

`p4pc64` links the container's system SDL3 and FFmpeg unless
`-DP4_LINUX64_PREFIX=<prefix> -DP4_FFMPEG_INC=<prefix>/include` names another set.
Debian's FFmpeg is built with GPL components. That is fine for local use, but
redistributable packages must use the LGPL-only build that
`ci/win32/build_deps.sh <prefix> /usr x86_64-linux-gnu` produces. The Windows
libraries in the image (`/opt/p4win64`) are already LGPL-only.

## 4. Run

The Linux player needs glibc 2.41 or newer on the machine that runs it (the image is
Debian 13), plus Mesa or a vendor OpenGL driver:

```sh
P4PC_ISO=/path/to/your.iso ./build/p4pc64
```

Without `P4PC_ISO` the player asks for the disc image with a file picker and
remembers it in `p4pc.ini`. See [Bring your own ISO](docs/bring_your_own_iso.md).
The Windows player is `build/win64/p4pc64w.exe`; copy it with the DLLs in that
folder to a Windows machine.

## 5. Tests

```sh
sh tools/ci_tests.sh build
```

This runs every registered test under `xvfb-run`, then the Windows tests (label
`port64win`) under Wine in a fresh prefix. `ctest --test-dir build -j4 --output-on-failure`
runs them directly. Tests that need the game skip with exit code 77 unless
`P4PC_ISO` names your disc image and `P4PC_ELF` your `SLUS_217.82`; the game-level
gates (labels `gl`, `scenes`, `scenes64`) also compare against reference captures
that are not part of this repository.

## Android

The Android build uses its own image, `ci/android/Dockerfile` (SDK, NDK, Gradle).
`tools/android_ci_run.sh` is its in-container entry point. It expects the source
at `/lane/src`, a cache volume at `/android-cache`, and writes the APK to `/lane/out`.
Set `P4PC_ANDROID_ABI=arm64-v8a` for phones, and `P4PC_ANDROID_VERSION_NAME` /
`P4PC_ANDROID_VERSION_CODE` for the APK's version. Details: [docs/android.md](docs/android.md).

## macOS

macOS builds natively on a Mac (Apple silicon or Intel: the build is for the Mac's own
architecture), without the container. It needs the Xcode command line tools, CMake,
Ninja and Python 3; `ci/macos/build_deps.sh` fetches the same pinned clang the
container uses and builds SDL3, the LGPL-only FFmpeg and ANGLE's libraries into a prefix:

```sh
sh ci/macos/build_deps.sh build/macos-deps
sh ci/macos/configure.sh build/macos-deps build/macos -DP4_CENSUS_CHECK=ON
cmake --build build/macos --target p4_x64 -- -k 0  # player and test programs
ctest --test-dir build/macos -j "$(sysctl -n hw.ncpu)" --timeout 900 --output-on-failure -L '^port64$'
P4PC_ISO=/path/to/your.iso ./build/macos/p4pc64
```

Only the 64-bit build exists on macOS. Details, the renderer (OpenGL ES on ANGLE's
Metal back end) and what has not been tested on real Macs: [docs/platform/macos.md](docs/platform/macos.md).

## Packages

`sh tools/package_win.sh` and `sh tools/package_linux.sh`, run in the container from
a clean checkout, build and package the Windows x64 zip and the Linux x86_64 AppImage
into `dist/`. They check the shipped DLL/shared-library set, verify that FFmpeg is
LGPL-only, and add `THIRD_PARTY_NOTICES.txt`, `licenses/` and the corresponding
library sources. `sh tools/package_macos.sh` does the same on a Mac for the macOS app
(`dist/p4pc-macos-<arch>-<version>.zip`, ad-hoc signed). Packages never contain game
data, saves or captures. The GitHub Actions workflow `.github/workflows/build.yml` builds
all of them on every push to `main`; a release (`tools/public_sync.sh --tag`) attaches the packages of its `main` run, labelled with the tag. See
[release packaging](docs/platform/packaging.md).
