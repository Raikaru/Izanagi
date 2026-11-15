# Building Izanagi

The supported build runs inside the project's Debian container (`ci/Dockerfile`).
It has the pinned clang (llvm-mingw), SDL3, FFmpeg, Wine and everything else the
build needs. The Windows player is cross-compiled from Linux in the same tree;
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

The build context is `ci/`. The image downloads the pinned llvm-mingw release
(checked by SHA-256) and builds SDL3 and an LGPL-only FFmpeg for the Windows targets.

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
submodule is missing. Add `-v /path/to/folder/with/SLUS_217.82:/opt/p4:ro` to the
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
sh ci/macos/configure.sh build/macos-deps build/macos
cmake --build build/macos --target p4pc64        # the player; --target p4_x64 adds every test program
ctest --test-dir build/macos -L '^port64$'        # the tests; those that need the game skip
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
data, saves or captures. The GitHub Actions workflow `.github/workflows/build.yml` runs
all of them for every pushed tag and attaches the packages to the release. See
[release packaging](docs/platform/packaging.md).
