//! Class library (upstream VM/src/lclasslib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation).
//!
//! Upstream opens it from luaL_openlibs only under DebugLuauUserDefinedClassesRuntime.
//! The instance/class relations come from the VM's class runtime (luau_isinstance,
//! luau_pushclassof).

const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const State = api.State;

/// class.isinstance(obj: any, class: class): boolean
fn class_isinstance(L: *State) c_int {
    aux.checkany(L, 1);
    aux.checktype(L, 2, api.TCLASS);
    if (api.@"type"(L, 1) != api.TOBJECT) {
        api.pushboolean(L, false);
        return 1;
    }
    api.pushboolean(L, api.isinstance(L, 1, 2));
    return 1;
}

fn class_classof(L: *State) c_int {
    aux.checkany(L, 1);
    if (api.@"type"(L, 1) != api.TOBJECT) {
        api.pushnil(L);
        return 1;
    }
    api.pushclassof(L, 1);
    return 1;
}

const classlib = [_]aux.Entry{
    aux.entry("isinstance", class_isinstance),
    aux.entry("classof", class_classof),
};

pub fn open(L: *State) c_int {
    aux.register(L, "class", &classlib);
    return 1;
}
