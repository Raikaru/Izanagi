//! Upstream Luau feature flags that change VM behavior. Defaults match upstream
//! (all off), which is the configuration used by production. Hosts and tests may
//! flip them before creating states.

pub var LuauCoroutineFinally: bool = false;
pub var DebugLuauUserDefinedClassesRuntime: bool = false;
pub var LuauGcTraceUdata: bool = false;
pub var LuauBufferCage: bool = false;
pub var LuauFastpcall: bool = false;
pub var LuauGcHeapShrinkFix: bool = false;
pub var LuauPcallErrorContinuationReentry: bool = false;
pub var LuauIsYieldableSimplify: bool = false;
/// The table resize always uses the allocation-robust algorithm (identical results;
/// differs from the flag-off path only in table state after an out-of-memory error).
pub var LuauTableRobustOom: bool = false;

const names = .{
    "LuauCoroutineFinally",              "DebugLuauUserDefinedClassesRuntime", "LuauGcTraceUdata",
    "LuauBufferCage",                    "LuauFastpcall",                      "LuauGcHeapShrinkFix",
    "LuauPcallErrorContinuationReentry", "LuauIsYieldableSimplify",            "LuauTableRobustOom",
};

/// C ABI flag setter for hosts/tests: returns 1 if the flag is known, 0 otherwise.
/// DebugLuauUserDefinedClassesRuntime enables the class/object member paths of the
/// classes runtime (bytecode version 100 is always loadable). LuauFastpcall is
/// accepted, but FASTPCALL always takes the regular call path (same results).
pub export fn luau_setfflag(name: [*:0]const u8, value: c_int) c_int {
    const s = @import("std").mem.span(name);
    inline for (names) |n| {
        if (@import("std").mem.eql(u8, s, n)) {
            @field(@This(), n) = value != 0;
            return 1;
        }
    }
    return 0;
}
