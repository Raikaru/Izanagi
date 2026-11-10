#!/bin/sh
# Compile check of the iOS-only renderer files for a real Apple target, with no Apple SDK: clang's own builtin headers, the SDL3 headers of an Android build
# (SDL's public headers are the same on every platform; they read __APPLE__ and TargetConditionals.h), and the libc declarations in ci/ios/stubs written from
# Darwin's documented signatures. Compiles src/render/p4gl_egl.c as arm64-apple-ios (the 64-bit pointer model included) and a unit that uses
# src/render/p4gl_win.h's EGL mapping. It checks syntax, types and the 64-bit cleanliness of the plumbing; it does not link, and says nothing about ANGLE's
# frameworks, the Metal back end or code signing (Apple hardware).
#   sh tools/ios_check.sh <clang> <SDL3 include dir> <include dir with the Khronos EGL/ and KHR/ headers> [source dir]
# (SDL_egl.h includes <EGL/egl.h>; the NDK sysroot or ANGLE's include/ carries the Khronos copy, which is the same file on every platform.)
set -eu
cc=${1:?usage: ios_check.sh <clang> <SDL3 include dir> <Khronos include dir> [source dir]}
sdl=${2:?}
khr=${3:?}
src=${4:-$(cd "$(dirname "$0")/.." && pwd)}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/khr" && cp -r "$khr/EGL" "$khr/KHR" "$tmp/khr/"
cat > "$tmp/win_use.c" <<'EOT'
#include "p4gl_win.h"
static P4GlContext sCtx;
int p4gl_win_probe(SDL_Window *w)
{
    sCtx = p4gl_win_create_context(w);
    if (sCtx == 0 || !p4gl_win_make_current(w, sCtx)) {
        return -1;
    }
    p4gl_win_set_swap_interval(1);
    (void)p4gl_win_proc("glClear");
    (void)p4gl_win_error();
    if (!p4gl_win_swap(w)) {
        return -2;
    }
    p4gl_win_destroy_context(p4gl_win_current_context());
    return (int)(P4GL_WIN_FLAG & 0);
}
EOT
for f in "$src/src/render/p4gl_egl.c" "$tmp/win_use.c"; do
  "$cc" -target arm64-apple-ios13.0 -fsyntax-only -nostdlibinc -Wall -Wextra -Werror -Wno-unused-parameter -DP4PC_GL_VIA_EGL=1 \
    -isystem "$src/ci/ios/stubs" -isystem "$sdl" -isystem "$tmp/khr" -I"$src/src/render" "$f"
done
# Luau's C boundary/test must remain valid for Apple's arm64 pointer model too.
if [ -f "$src/include/p4pc/luau_c.h" ]; then
  "$cc" -target arm64-apple-ios13.0 -fsyntax-only -nostdlibinc -Wall -Wextra -Werror \
    -isystem "$src/ci/ios/stubs" -I"$src/include" -I"$src/third_party/luau/VM/include" \
    "$src/tests/luau_sandbox_test.c"
fi
echo "ios_check: arm64-apple-ios13.0 syntax check passed (no SDK; not a link or a run)"
