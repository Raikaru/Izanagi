# Android

Izanagi runs on Android using an SDL3 activity and a native `libmain.so`.
The phone build uses `arm64-v8a`; the build also supports emulator ABIs. You
need your own USA game disc image to play. No game data is included in the APK.

## Build

The Android image contains the SDK, NDK and Gradle. From the repository root,
with Docker installed and the `decomp` submodule initialized:

```sh
docker build -t p4pc-android ci/android
docker run --rm -v "$PWD:/lane/src" -v "$PWD/build/android-out:/lane/out" -v p4pc-android-cache:/android-cache \
  -e P4PC_ANDROID_ABI=arm64-v8a -e P4PC_ANDROID_BUILD=debug \
  -e P4PC_ANDROID_VERSION_NAME="$(git describe --tags --always)" -e P4PC_ANDROID_VERSION_CODE="$(git rev-list --count HEAD)" \
  p4pc-android sh /lane/src/tools/android_ci_run.sh
```

The output directory receives the APK and an unstripped native library for
symbolizing crashes. `P4PC_ANDROID_ABI` accepts `x86`, `x86_64` or `arm64-v8a`.
For release packages and signing, see [release packaging](platform/packaging.md).

## Install and play

Install the APK on your device, launch it and choose your disc image with the
system document picker. The app retains permission to access the selected image.
The launcher can also select the game's executable; when omitted, the app obtains
it from the disc image. Keep the image available: the game reads it while playing.
See [Bring your own ISO](bring_your_own_iso.md).

Debug and release builds have separate application IDs and storage. Saves and
settings are kept in app storage, not in the APK. Back up your saves before
uninstalling. Use the on-screen controls or a supported controller; display,
audio and input options are available from the game's Config screen.

## Implementation

The Gradle project and Java activity are in `android/`; native entry and JNI glue
are in `src/platform/android/`. `ci/android/build_deps.sh` builds SDL3 and FFmpeg
for the selected ABI. Android uses position-independent native code and the
Storage Access Framework to access the selected disc image. Log output goes to
logcat under the `p4pc` tag and to the app's `files/boot.log`; the run before is kept as
`boot.prev.log` (`boot.unclean.log` when it did not end by itself). The launcher's
**Export logs** button saves the logs, the settings without paths and the device facts as
one zip through the system file dialog (`LogExport.java`, `LogBundle.java`);
[diagnostics](platform/diagnostics.md) lists what the logs hold.
