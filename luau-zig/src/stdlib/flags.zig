//! Fast flags defined by the upstream library sources (LUAU_FASTFLAGVARIABLE /
//! LUAU_DYNAMIC_FASTFLAGVARIABLE in lmathlib/lstrlib/ltablib/lintlib/linit).
//! Defaults equal the vendored Luau 0.744 defaults (all false). Hosts flip them through
//! `set` (by upstream name) before opening the libraries, mirroring FFlag::X assignment.
//! Flags the VM also reads live in vm.fflags (src/vm/flags.zig: LuauCoroutineFinally,
//! DebugLuauUserDefinedClassesRuntime, LuauFastpcall, ...) and are read via `api.vmflag`.

const std = @import("std");

// LuauFastpcall is defined in lbaselib upstream but read by the VM's FASTPCALL; its single
// source of truth is vm.fflags.LuauFastpcall.
pub var FixMathNoisePrecision: bool = false;
pub var LuauOptimizeStringSplit: bool = false; // dynamic
pub var LuauOptimizeStringGsub: bool = false; // dynamic
pub var LuauTableFindImprovement: bool = false; // dynamic
pub var LuauIntegerLibrary: bool = false;
pub var LuauSandboxFreezesVectorMetatable: bool = false;

/// Sets a library-owned flag by its upstream name; returns false for unknown names.
pub fn set(name: []const u8, value: bool) bool {
    inline for (.{
        "FixMathNoisePrecision",
        "LuauOptimizeStringSplit",
        "LuauOptimizeStringGsub",
        "LuauTableFindImprovement",
        "LuauIntegerLibrary",
        "LuauSandboxFreezesVectorMetatable",
    }) |flag| {
        if (std.mem.eql(u8, name, flag)) {
            @field(@This(), flag) = value;
            return true;
        }
    }
    return false;
}
