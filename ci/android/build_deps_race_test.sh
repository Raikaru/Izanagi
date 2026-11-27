#!/bin/sh
# Run by ctest as android_build_deps_race (tests/cmake/000-registration.cmake). Exercises the real dependency script with fake tools, without downloading or compiling.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/bin" "$tmp/cache"
export TEST_ROOT=$tmp ANDROID_NDK_HOME=$tmp/ndk PATH="$tmp/bin:$PATH"
cat > "$tmp/bin/curl" <<'EOF'
#!/bin/sh
while [ "$1" != -o ]; do shift; done
: > "$2"
EOF
cat > "$tmp/bin/sha256sum" <<'EOF'
#!/bin/sh
cat >/dev/null
EOF
cat > "$tmp/bin/tar" <<'EOF'
#!/bin/sh
while [ "$1" != -C ]; do shift; done
dir=$2
case "$dir" in
  */sdl) mkdir -p "$dir/android-project" ;;
  */ffmpeg)
    # the HCA window patch (ci/ffmpeg-7.1.1-hca-window-alignment.patch, applied with git apply --unidiff-zero) edits line 65 of this file
    mkdir -p "$dir/libavcodec"
    awk 'BEGIN { for (i = 1; i < 65; i++) print "/* line " i " */"; print "static const float window[128] =" }' > "$dir/libavcodec/hca_data.h"
    cat > "$dir/configure" <<'CONFIGURE'
#!/bin/sh
printf '%s\n' configure >> "$TEST_ROOT/ffmpeg-count"
CONFIGURE
    chmod +x "$dir/configure" ;;
esac
EOF
cat > "$tmp/bin/cmake" <<'EOF'
#!/bin/sh
case "$1" in
  -S) printf '%s\n' configure >> "$TEST_ROOT/sdl-count"; : > "$TEST_ROOT/started"; sleep 2 ;;
  --install) mkdir -p "$TEST_ROOT/cache/deps/x86/lib"; : > "$TEST_ROOT/cache/deps/x86/lib/libSDL3.so" ;;
esac
EOF
cat > "$tmp/bin/make" <<'EOF'
#!/bin/sh
if [ "${1:-}" = install ]; then
  mkdir -p "$TEST_ROOT/cache/deps/x86/lib"
  : > "$TEST_ROOT/cache/deps/x86/lib/libavcodec.so"
fi
EOF
chmod +x "$tmp/bin/"*
sh "$root/ci/android/build_deps.sh" "$tmp/cache/deps" x86 > "$tmp/first.log" 2>&1 &
first=$!
while [ ! -f "$tmp/started" ]; do
  if ! kill -0 "$first" 2>/dev/null; then cat "$tmp/first.log"; exit 1; fi
  sleep 0.1
done
sh "$root/ci/android/build_deps.sh" "$tmp/cache/deps" x86 > "$tmp/second.log" 2>&1 &
second=$!
wait "$first" || { cat "$tmp/first.log"; exit 1; }
wait "$second" || { cat "$tmp/second.log"; exit 1; }
sdl=$(wc -l < "$tmp/sdl-count")
ffmpeg=$(wc -l < "$tmp/ffmpeg-count")
[ "$sdl" -eq 1 ] && [ "$ffmpeg" -eq 1 ]
[ -f "$tmp/cache/deps/x86/ffmpeg.opts" ]
[ -d "$tmp/cache/deps/sdl-java" ]
for work in "$tmp/cache"/deps-build-x86.??????; do [ ! -e "$work" ]; done
printf 'concurrent invocations=2 SDL builds=%s FFmpeg builds=%s\n' "$sdl" "$ffmpeg"
