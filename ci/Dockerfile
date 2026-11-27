# Build and test image for the Persona 4 PC port (BUILDING.md, .github/workflows/build.yml, tools/package_linux.sh, tools/package_win.sh).
# Retail inputs are NOT part of the image or needed to build; tests that need the disc skip without it.
#   docker build -t p4pc-ci ci/
#
# One image builds every desktop target: the Linux i386 and x86_64 builds, and the Windows builds
# (llvm-mingw cross toolchain in /opt/llvm-mingw; SDL3 and FFmpeg for it in /opt/p4win32 and /opt/p4win64;
# Wine to run the Windows host tests). See docs/platform/windows.md.
FROM debian:trixie-slim

ENV DEBIAN_FRONTEND=noninteractive
RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates git python3 cmake ninja-build ccache \
      clang lld llvm binutils gdb \
      gcc-multilib \
      libc6-dev-i386 libc6-i386 \
      libsdl3-dev:i386 libsdl3-0:i386 \
      libavcodec-dev:i386 libavutil-dev:i386 libswscale-dev:i386 \
      libgl1-mesa-dri:i386 libglx-mesa0:i386 libegl-mesa0:i386 libgl1:i386 libegl1:i386 \
      xvfb xauth \
      curl xz-utils make nasm patch \
      wine wine32:i386 \
 && rm -rf /var/lib/apt/lists/*

# llvm-mingw (clang + lld + mingw-w64 UCRT headers and CRT for i686/x86_64). Pinned by release and
# checksum; the Windows build needs this exact toolchain family (lld's /sectionlayout places the
# PS2 RAM section, see src/platform/os/win32_ps2ram.S).
ARG LLVM_MINGW_RELEASE=20261006
ARG LLVM_MINGW_SHA256=5f9c6ed95b2d4bdb2869a488c5fd5857fbdabcf288a0aa3eb1da43f6a08d8ab4
RUN curl -fsSL -o /tmp/llvm-mingw.tar.xz \
      https://github.com/mstorsjo/llvm-mingw/releases/download/${LLVM_MINGW_RELEASE}/llvm-mingw-${LLVM_MINGW_RELEASE}-ucrt-ubuntu-22.04-x86_64.tar.xz \
 && echo "${LLVM_MINGW_SHA256}  /tmp/llvm-mingw.tar.xz" | sha256sum -c - \
 && mkdir -p /opt/llvm-mingw \
 && tar -xJf /tmp/llvm-mingw.tar.xz -C /opt/llvm-mingw --strip-components=1 \
 && rm /tmp/llvm-mingw.tar.xz
ENV P4PC_LLVM_MINGW=/opt/llvm-mingw

# SDL3 and the LGPL FFmpeg subset for i686-w64-mingw32, as DLLs plus import libraries.
COPY win32/build_deps.sh /usr/local/share/p4pc/ci/win32/build_deps.sh
COPY ffmpeg-7.1.1-hca-window-alignment.patch /usr/local/share/p4pc/ci/
RUN sh /usr/local/share/p4pc/ci/win32/build_deps.sh /opt/p4win32 /opt/llvm-mingw \
 && rm -rf /opt/deps-build /opt/dl-cache

# SDL3 and FFmpeg for x86_64 Linux (the 64-bit build, docs/port64.md), beside the i386 ones above. A layer of its own: a host that
# has the layers above rebuilds only this one.
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      libsdl3-dev libsdl3-0 \
      libavcodec-dev libavutil-dev libswscale-dev \
      libgl1-mesa-dri libglx-mesa0 libegl-mesa0 libgl1 libegl1 \
 && rm -rf /var/lib/apt/lists/*

# SDL3 and FFmpeg for x86_64-w64-mingw32 (the Windows x64 build, docs/port64.md 2.13) and the 64-bit Wine that runs its tests. Layers of their
# own, like the x86_64 Linux libraries above.
RUN sh /usr/local/share/p4pc/ci/win32/build_deps.sh /opt/p4win64 /opt/llvm-mingw x86_64-w64-mingw32 \
 && rm -rf /opt/deps-build /opt/dl-cache
RUN apt-get update \
 && apt-get install -y --no-install-recommends wine64 \
 && rm -rf /var/lib/apt/lists/*

# Report tests exercise real PNG composition and replay provenance.
# Keep this last so adding the Python runtime dependency does not rebuild SDL/FFmpeg.
RUN apt-get update \
 && apt-get install -y --no-install-recommends python3-pil \
 && rm -rf /var/lib/apt/lists/*

# Host-only C++17 adapters (Luau and Basis). A final layer preserves the cached
# clang/SDL/FFmpeg builds when adding the i386 C++ standard library headers.
RUN apt-get update \
 && apt-get install -y --no-install-recommends g++-multilib \
 && rm -rf /var/lib/apt/lists/*

# appimagetool needs file(1): tools/package_linux.sh assembles the AppImage inside this image.
RUN apt-get update \
 && apt-get install -y --no-install-recommends file \
 && rm -rf /var/lib/apt/lists/*

# The pinned Zig nightly of the battle simulator (cmake/zig/pin.cmake: keep the version and the x86_64-linux SHA-256 equal to it;
# CMake refuses another version), the official ziglang.org/builds archive whose file name carries the version's "+". It
# cross-compiles every profile's Zig objects (i386, x86_64, AArch64 Linux; i686/x86_64 mingw-w64, UCRT headers). Last, so the cached
# toolchain and SDL/FFmpeg layers above survive a Zig update. ziglang.org/builds keeps only recent nightlies: the built image keeps
# this one. A host overrides it without rebuilding the image: -DP4_ZIG=<zig> or P4_ZIG in the environment.
ARG ZIG_VERSION=0.18.0-dev.935+ead8f5a7a
ARG ZIG_SHA256=76258449f1088b6f80359944f9735a4c96e94246e553111155f0af6457ff8de4
RUN curl -fsSL --proto '=https' --retry 6 --retry-all-errors --retry-delay 10 -o /tmp/zig.tar.xz https://ziglang.org/builds/zig-x86_64-linux-${ZIG_VERSION}.tar.xz \
 && echo "${ZIG_SHA256}  /tmp/zig.tar.xz" | sha256sum -c - \
 && mkdir -p /opt/zig-${ZIG_VERSION} \
 && tar -xJf /tmp/zig.tar.xz -C /opt/zig-${ZIG_VERSION} --strip-components=1 \
 && rm /tmp/zig.tar.xz \
 && test "$(/opt/zig-${ZIG_VERSION}/zig version)" = "${ZIG_VERSION}"
ENV P4_ZIG=/opt/zig-${ZIG_VERSION}/zig

ENV CCACHE_DIR=/ccache
WORKDIR /lane
