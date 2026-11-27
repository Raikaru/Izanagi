//! Debug library (upstream VM/src/ldblib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio).
//!
//! Exposes debug.info and debug.traceback only (as upstream). Not opened by the p4pc
//! restricted sandbox.

const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const State = api.State;

/// The thread a debug function inspects and the index of its first remaining argument
/// minus one (1 when an explicit thread was passed, 0 otherwise).
const Target = struct { L1: *State, arg: c_int };

fn getthread(L: *State) Target {
    if (api.@"type"(L, 1) == api.TTHREAD) return .{ .L1 = api.tothread(L, 1).?, .arg = 1 };
    return .{ .L1 = L, .arg = 0 };
}

fn db_info(L: *State) c_int {
    const t = getthread(L);
    const L1 = t.L1;
    const arg = t.arg;
    var l1top: c_int = 0;

    // if L1 != L, L1 can be in any state, and therefore there are no guarantees about its stack space
    if (L != L1) {
        // for 'f' option, we reserve one slot and we also record the stack top
        api.rawcheckstack(L1, 1);
        l1top = api.gettop(L1);
    }

    var level: c_int = undefined;
    if (api.isnumber(L, arg + 1)) {
        level = api.tointeger(L, arg + 1);
        aux.argcheck(L, level >= 0, arg + 1, "level can't be negative");
    } else if (arg == 0 and api.isfunction(L, 1)) {
        // convert absolute index to relative index
        level = -api.gettop(L);
    } else {
        aux.argerror(L, arg + 1, "function or level expected");
    }

    const options = aux.checklstringz(L, arg + 2);

    var ar: api.Debug = undefined;
    if (!api.getinfo(L1, level, options.ptr, &ar)) return 0;

    var results: c_int = 0;
    var occurs: [26]bool = @splat(false);

    // the option string is a C string: it ends at the first NUL
    var i: usize = 0;
    while (i < options.len and options[i] != 0) : (i += 1) {
        const c = options[i];
        if (c >= 'a' and c <= 'z') {
            if (occurs[c - 'a']) {
                // restore stack state of another thread as 'f' option might not have been visited yet
                if (L != L1) api.settop(L1, l1top);
                aux.argerror(L, arg + 2, "duplicate option");
            }
            occurs[c - 'a'] = true;
        }

        switch (c) {
            's' => {
                api.pushlstring(L, api.debugShortSrc(&ar));
                results += 1;
            },
            'l' => {
                api.pushinteger(L, ar.currentline);
                results += 1;
            },
            'n' => {
                api.pushlstring(L, api.debugName(&ar) orelse "");
                results += 1;
            },
            'f' => {
                if (L1 == L) {
                    api.pushvalue(L, -1 - results); // function is right before results
                } else {
                    api.xmove(L1, L, 1); // function is at top of L1
                }
                results += 1;
            },
            'a' => {
                api.pushinteger(L, ar.nparams);
                api.pushboolean(L, ar.isvararg != 0);
                results += 2;
            },
            else => {
                // restore stack state of another thread as 'f' option might not have been visited yet
                if (L != L1) api.settop(L1, l1top);
                aux.argerror(L, arg + 2, "invalid option");
            },
        }
    }

    return results;
}

fn db_traceback(L: *State) c_int {
    const t = getthread(L);
    const L1 = t.L1;
    const arg = t.arg;
    var msg: ?[]const u8 = null;
    if (!api.isnoneornil(L, arg + 1)) {
        // luaL_optstring: a C string, so the message ends at its first NUL
        const m = aux.checklstringz(L, arg + 1);
        msg = api.span(m.ptr);
    }
    const level = aux.optinteger(L, arg + 2, if (L == L1) 1 else 0);
    aux.argcheck(L, level >= 0, arg + 2, "level can't be negative");

    aux.traceback(L, L1, msg, level);
    return 1;
}

const dblib = [_]aux.Entry{
    aux.entry("info", db_info),
    aux.entry("traceback", db_traceback),
};

pub fn open(L: *State) c_int {
    aux.register(L, "debug", &dblib);
    return 1;
}
