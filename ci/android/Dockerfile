# Android build and test image for the Persona 4 PC port (docs/android.md).
#   podman build -t p4pc-android ci/android
# Holds the Android SDK (platform 34, build-tools), NDK r27 (clang for x86, x86_64, arm64-v8a), Gradle's JDK, and the emulator with a
# 32-bit x86 system image, so the i386 build of the port can run on Android's x86 ABI today and the same scripts carry to
# arm64-v8a. Retail data is never part of the image; the run script mounts it. The emulator needs /dev/kvm (--device /dev/kvm).
FROM debian:trixie-slim

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates curl unzip xz-utils git python3 cmake ninja-build make nasm patch pkg-config ccache file \
      openjdk-21-jdk-headless \
      libgl1 libpulse0 libnss3 libxcomposite1 libxcursor1 libxdamage1 libxi6 libxrandr2 libxtst6 libxkbfile1 libx11-6 libxext6 libxfixes3 \
      libasound2t64 libdbus-1-3 libxcb1 libxcb-xinerama0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0 libxcb-shape0 libxkbcommon-x11-0 \
 && rm -rf /var/lib/apt/lists/*

ENV ANDROID_HOME=/opt/android-sdk
ENV ANDROID_SDK_ROOT=/opt/android-sdk
ARG CMDLINE_TOOLS=11076708
ARG NDK_VERSION=27.2.12479018
ARG PLATFORM=34
ARG BUILD_TOOLS=34.0.0
# API 30 google_apis x86: the last era with a plain 32-bit image; the emulator runs 32-bit app processes (zygote32) on it.
ARG SYSTEM_IMAGE=system-images;android-30;google_apis;x86
RUN mkdir -p $ANDROID_HOME/cmdline-tools \
 && curl -fsSL -o /tmp/clt.zip https://dl.google.com/android/repository/commandlinetools-linux-${CMDLINE_TOOLS}_latest.zip \
 && unzip -q /tmp/clt.zip -d $ANDROID_HOME/cmdline-tools && mv $ANDROID_HOME/cmdline-tools/cmdline-tools $ANDROID_HOME/cmdline-tools/latest && rm /tmp/clt.zip
ENV PATH=$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$ANDROID_HOME/emulator:$PATH
RUN yes | sdkmanager --licenses >/dev/null \
 && sdkmanager "platform-tools" "platforms;android-${PLATFORM}" "build-tools;${BUILD_TOOLS}" "ndk;${NDK_VERSION}" "emulator" "${SYSTEM_IMAGE}"
ENV ANDROID_NDK_HOME=/opt/android-sdk/ndk/${NDK_VERSION}

# Gradle for the APK (android/): AGP 8.5 wants Gradle 8.7. Its plugin downloads and caches live under GRADLE_USER_HOME, which the build
# script points into the persistent /android-cache volume.
ARG GRADLE_VERSION=8.7
RUN curl -fsSL -o /tmp/gradle.zip https://services.gradle.org/distributions/gradle-${GRADLE_VERSION}-bin.zip \
 && unzip -q /tmp/gradle.zip -d /opt && mv /opt/gradle-${GRADLE_VERSION} /opt/gradle && rm /tmp/gradle.zip
ENV PATH=/opt/gradle/bin:$PATH

# One AVD for the tests (x86, 2 GB, no snapshot: every run is a cold boot).
RUN echo no | avdmanager create avd -n p4pc -k "${SYSTEM_IMAGE}" -d pixel_5 --force \
 && printf 'hw.ramSize=2048\nhw.keyboard=yes\ndisk.dataPartition.size=4096M\nhw.gpu.enabled=yes\nhw.gpu.mode=swiftshader_indirect\n' >> /root/.android/avd/p4pc.avd/config.ini

# FFmpeg's configure compiles host-side helper programs with a host C compiler.
RUN apt-get update && apt-get install -y --no-install-recommends gcc libc6-dev && rm -rf /var/lib/apt/lists/*

# The pinned Zig nightly of the battle simulator (cmake/zig/pin.cmake: keep the version and the x86_64-linux SHA-256 equal to it),
# the official ziglang.org/builds archive whose file name carries the version's "+". The Zig objects use the NDK sysroot's bionic
# headers (cmake/zig/zig_object.py). Last, so the SDK/NDK layers above stay cached; ziglang.org/builds keeps only recent nightlies:
# the built image keeps this one. A host overrides it with -DP4_ZIG=<zig> or P4_ZIG in the environment.
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
WORKDIR /work
