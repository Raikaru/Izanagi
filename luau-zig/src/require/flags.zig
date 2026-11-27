//! Fast flags defined by the upstream require-by-string and configuration sources
//! (LUAU_FASTFLAGVARIABLE in Require/src/RequireImpl.cpp and Config/src/LuauConfig.cpp,
//! Luau 0.744). Defaults equal the vendored library defaults (false); the upstream
//! `luau` CLI turns every non-experimental "Luau*" flag on (setLuauFlagsDefault), so a
//! CLI host sets both to true. Hosts flip them through `set` (by upstream name).

const std = @import("std");

/// Cyclic requires of modules that use `export` receive a locked placeholder table that
/// is populated with the module's exports once it finishes loading.
pub var LuauCyclicRequireShortCircuit: bool = false;

/// Only restructures Config/src/LuauConfig.cpp at the pinned commit: both branches
/// execute and parse `.config.luau` identically, so this port has a single code path.
pub var LuauRbsConfigAliasResolution: bool = false;

const names = .{ "LuauCyclicRequireShortCircuit", "LuauRbsConfigAliasResolution" };

/// Sets a require-owned flag by its upstream name; returns false for unknown names.
pub fn set(name: []const u8, value: bool) bool {
    inline for (names) |flag| {
        if (std.mem.eql(u8, name, flag)) {
            @field(@This(), flag) = value;
            return true;
        }
    }
    return false;
}

/// Sets every require-owned flag (both are "Luau*" flags), mirroring setLuauFlags(bool).
pub fn setAll(value: bool) void {
    inline for (names) |flag| @field(@This(), flag) = value;
}
