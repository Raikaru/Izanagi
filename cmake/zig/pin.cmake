# The one Zig build the battle simulator is compiled with: the official nightly 0.18.0-dev.935+ead8f5a7a, and the SHA-256 of each
# official host archive as published in the "master" entry of https://ziglang.org/download/index.json (2026-11-07). The archives are
# https://ziglang.org/builds/zig-<host>-<version>.<tar.xz|zip>, the "+" of the version literal in the file name. ci/zig/fetch_zig.sh
# installs from these (ci/Dockerfile and ci/android/Dockerfile carry the x86_64-linux pair); CMake refuses any other version.
# ziglang.org/builds keeps only recent nightlies: keep the archive in $P4PC_DL_CACHE (fetch_zig.sh checks it against the SHA-256 here).
set(P4_ZIG_VERSION "0.18.0-dev.935+ead8f5a7a")
set(P4_ZIG_URL_BASE "https://ziglang.org/builds")
set(P4_ZIG_SHA256_x86_64-linux   "76258449f1088b6f80359944f9735a4c96e94246e553111155f0af6457ff8de4")
set(P4_ZIG_SHA256_aarch64-linux  "37902e91ffbdbaf3e62bf94ba3c7721da13a7d0dbf89a3378d30f5100d3ac0b2")
set(P4_ZIG_SHA256_x86_64-macos   "95f4939ba491970a74001c291b7e88e73ff3505f5a9115e4e9b9af6eb9fec660")
set(P4_ZIG_SHA256_aarch64-macos  "86917e13b3312d34f0f58d206986a7927dd608ce31f21712cace4758043c80ce")
set(P4_ZIG_SHA256_x86_64-windows "42bc8cf0c1293392ab0d0519e1e556f4f542a867cbeb4d081172482a3851a3d4")
