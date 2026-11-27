//! Basic library (upstream VM/src/lbaselib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio).
//!
//! pcall/xpcall are C closures with continuations (lua_pcallyieldable); with the
//! LuauFastpcall flag (VM-owned, vm.fflags) they are also registered as the VM's builtins.

const std = @import("std");
const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const ctype = @import("ctype.zig");
const cstrto = @import("cstrto.zig");
const State = api.State;

fn writestring(s: []const u8) void {
    // upstream: fwrite(s, 1, l, stdout); output failures are ignored there as well
    const io = std.Io.Threaded.global_single_threaded.io();
    std.Io.File.stdout().writeStreamingAll(io, s) catch {};
}

fn print(L: *State) c_int {
    const n = api.gettop(L);
    var i: c_int = 1;
    while (i <= n) : (i += 1) {
        const s = aux.tolstring(L, i);
        if (i > 1) writestring("\t");
        writestring(s);
        api.pop(L, 1);
    }
    writestring("\n");
    return 0;
}

fn tonumber(L: *State) c_int {
    const base = aux.optinteger(L, 2, 10);
    if (base == 10) {
        if (api.tonumberx(L, 1)) |n| {
            api.pushnumber(L, n);
            return 1;
        }
        aux.checkany(L, 1);
    } else {
        const full = aux.checklstring(L, 1);
        aux.argcheck(L, 2 <= base and base <= 36, 2, "base out of range");
        // the C string ends at the first embedded NUL
        const s1 = full[0 .. std.mem.indexOfScalar(u8, full, 0) orelse full.len];
        const r = cstrto.strtoull(s1, @intCast(base));
        if (r.end != 0) {
            var e = r.end;
            while (e < s1.len and ctype.isspace(s1[e])) e += 1;
            if (e == s1.len) {
                api.pushnumber(L, @floatFromInt(r.value));
                return 1;
            }
        }
    }
    api.pushnil(L);
    return 1;
}

fn @"error"(L: *State) c_int {
    const level = aux.optinteger(L, 2, 1);
    api.settop(L, 1);
    if (api.isstring(L, 1) and level > 0) {
        aux.where(L, level);
        api.pushvalue(L, 1);
        api.concat(L, 2);
    }
    api.@"error"(L);
}

fn getmetatable(L: *State) c_int {
    aux.checkany(L, 1);
    if (!api.getmetatable(L, 1)) {
        api.pushnil(L);
        return 1;
    }
    _ = aux.getmetafield(L, 1, "__metatable");
    return 1;
}

fn setmetatable(L: *State) c_int {
    const t = api.@"type"(L, 2);
    aux.checktype(L, 1, api.TTABLE);
    aux.argexpected(L, t == api.TNIL or t == api.TTABLE, 2, "nil or table");
    if (aux.getmetafield(L, 1, "__metatable"))
        aux.errorf(L, "cannot change a protected metatable", .{});
    api.settop(L, 2);
    api.setmetatable(L, 1);
    return 1;
}

fn getfunc(L: *State, opt: bool) void {
    if (api.isfunction(L, 1)) {
        api.pushvalue(L, 1);
    } else {
        var ar: api.Debug = undefined;
        const level = if (opt) aux.optinteger(L, 1, 1) else aux.checkinteger(L, 1);
        aux.argcheck(L, level >= 0, 1, "level must be non-negative");
        if (!api.getinfo(L, level, "f", &ar))
            aux.argerror(L, 1, "invalid level");
        if (api.isnil(L, -1))
            aux.errorf(L, "no function environment for tail call at level {d}", .{level});
    }
}

fn getfenv(L: *State) c_int {
    getfunc(L, true);
    if (api.iscfunction(L, -1)) {
        api.pushvalue(L, api.GLOBALSINDEX);
    } else {
        api.getfenv(L, -1);
    }
    api.setsafeenv(L, -1, false);
    return 1;
}

fn setfenv(L: *State) c_int {
    aux.checktype(L, 2, api.TTABLE);
    getfunc(L, false);
    api.pushvalue(L, 2);
    api.setsafeenv(L, -1, false);
    if (api.isnumber(L, 1) and api.tonumber(L, 1) == 0) {
        // change environment of current thread
        _ = api.pushthread(L);
        api.insert(L, -2);
        _ = api.setfenv(L, -2);
        return 0;
    } else if (api.iscfunction(L, -2) or !api.setfenv(L, -2)) {
        aux.errorf(L, "'setfenv' cannot change environment of given object", .{});
    }
    return 1;
}

fn rawequal(L: *State) c_int {
    aux.checkany(L, 1);
    aux.checkany(L, 2);
    api.pushboolean(L, api.rawequal(L, 1, 2));
    return 1;
}

fn rawget(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    aux.checkany(L, 2);
    api.settop(L, 2);
    _ = api.rawget(L, 1);
    return 1;
}

fn rawset(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    aux.checkany(L, 2);
    aux.checkany(L, 3);
    api.settop(L, 3);
    api.rawset(L, 1);
    return 1;
}

fn rawlen(L: *State) c_int {
    const tt = api.@"type"(L, 1);
    aux.argcheck(L, tt == api.TTABLE or tt == api.TSTRING, 1, "table or string expected");
    api.pushinteger(L, api.objlen(L, 1));
    return 1;
}

fn gcinfo(L: *State) c_int {
    api.pushinteger(L, api.gc(L, api.GCCOUNT, 0));
    return 1;
}

fn @"type"(L: *State) c_int {
    aux.checkany(L, 1);
    api.pushlstring(L, api.span(api.typename(L, api.@"type"(L, 1))));
    return 1;
}

fn typeof(L: *State) c_int {
    aux.checkany(L, 1);
    api.pushlstring(L, aux.typenameOf(L, 1));
    return 1;
}

pub fn next(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    api.settop(L, 2);
    if (api.next(L, 1)) return 2;
    api.pushnil(L);
    return 1;
}

fn pairs(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    api.pushvalue(L, api.upvalueindex(1));
    api.pushvalue(L, 1);
    api.pushnil(L);
    return 3;
}

pub fn inext(L: *State) c_int {
    var i = aux.checkinteger(L, 2);
    aux.checktype(L, 1, api.TTABLE);
    i +%= 1;
    api.pushinteger(L, i);
    _ = api.rawgeti(L, 1, i);
    return if (api.isnil(L, -1)) 0 else 2;
}

fn ipairs(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    api.pushvalue(L, api.upvalueindex(1));
    api.pushvalue(L, 1);
    api.pushinteger(L, 0);
    return 3;
}

fn assert(L: *State) c_int {
    aux.checkany(L, 1);
    if (!api.toboolean(L, 1)) {
        const msg = aux.optlstring(L, 2, "assertion failed!");
        // formatted through "%s": stops at the first embedded NUL
        aux.errorf(L, "{s}", .{msg[0 .. std.mem.indexOfScalar(u8, msg, 0) orelse msg.len]});
    }
    return api.gettop(L);
}

fn select(L: *State) c_int {
    const n = api.gettop(L);
    if (api.@"type"(L, 1) == api.TSTRING and api.tolstringz(L, 1).?.ptr[0] == '#') {
        api.pushinteger(L, n - 1);
        return 1;
    }
    var i = aux.checkinteger(L, 1);
    if (i < 0) {
        i = n +% i;
    } else if (i > n) {
        i = n;
    }
    aux.argcheck(L, 1 <= i, 1, "index out of range");
    return n - i;
}

fn pcally(L: *State) c_int {
    aux.checkany(L, 1);
    return api.pcallyieldable(L, api.gettop(L) - 1, api.MULTRET, 0);
}

fn pcallcont(L: *State, status: c_int) c_int {
    api.rawcheckstack(L, 1);
    if (status == 0) {
        // results occupy the whole frame; prepend the 'true' status
        api.pushboolean(L, true);
        api.insert(L, 1);
        return api.gettop(L);
    } else {
        // the error object is on top; put 'false' right before it
        api.pushboolean(L, false);
        api.insert(L, -2);
        return 2;
    }
}

fn xpcally(L: *State) c_int {
    aux.checktype(L, 2, api.TFUNCTION);
    // swap function & error function
    api.pushvalue(L, 1);
    api.pushvalue(L, 2);
    api.replace(L, 1);
    api.replace(L, 2);
    // at this point the stack looks like err, f, args
    return api.pcallyieldable(L, api.gettop(L) - 2, api.MULTRET, 1);
}

fn xpcallcont(L: *State, status: c_int) c_int {
    if (status == 0) {
        // xpcall had an 'errfunc' before the results, so we just replace it with 'true' status
        api.pushboolean(L, true);
        api.replace(L, 1);
        return api.gettop(L);
    } else {
        api.rawcheckstack(L, 1);
        api.pushboolean(L, false);
        api.insert(L, -2);
        return 2;
    }
}

fn tostring(L: *State) c_int {
    aux.checkany(L, 1);
    _ = aux.tolstring(L, 1);
    return 1;
}

fn newproxy(L: *State) c_int {
    const t = api.@"type"(L, 1);
    aux.argexpected(L, t == api.TNONE or t == api.TNIL or t == api.TBOOLEAN, 1, "nil or boolean");
    const needsmt = api.toboolean(L, 1);
    _ = api.newuserdatatagged(L, 0, api.UTAG_PROXY);
    if (needsmt) {
        api.newtable(L);
        api.setmetatable(L, -2);
    }
    return 1;
}

const base_funcs = [_]aux.Entry{
    aux.entry("assert", assert),
    aux.entry("error", @"error"),
    aux.entry("gcinfo", gcinfo),
    aux.entry("getfenv", getfenv),
    aux.entry("getmetatable", getmetatable),
    aux.entry("next", next),
    aux.entry("newproxy", newproxy),
    aux.entry("print", print),
    aux.entry("rawequal", rawequal),
    aux.entry("rawget", rawget),
    aux.entry("rawset", rawset),
    aux.entry("rawlen", rawlen),
    aux.entry("select", select),
    aux.entry("setfenv", setfenv),
    aux.entry("setmetatable", setmetatable),
    aux.entry("tonumber", tonumber),
    aux.entry("tostring", tostring),
    aux.entry("type", @"type"),
    aux.entry("typeof", typeof),
};

fn auxopen(L: *State, name: [*:0]const u8, comptime f: fn (*State) c_int, comptime u: fn (*State) c_int) void {
    api.pushcfunction(L, aux.wrap(u), null);
    api.pushcclosure(L, aux.wrap(f), name, 1);
    api.setfield(L, -2, name);
}

pub fn open(L: *State) c_int {
    // set global _G
    api.pushvalue(L, api.GLOBALSINDEX);
    api.setglobal(L, "_G");

    // open lib into global table
    aux.register(L, "_G", &base_funcs);
    api.pushlstring(L, "Luau");
    api.setglobal(L, "_VERSION");

    // `ipairs' and `pairs' need auxiliary functions as upvalues
    auxopen(L, "ipairs", ipairs, inext);
    auxopen(L, "pairs", pairs, next);

    api.pushcclosurek(L, aux.wrap(pcally), "pcall", 0, aux.wrapk(pcallcont));
    if (api.vmflag("LuauFastpcall")) api.setbuiltinpcall(L, -1, false);
    api.setfield(L, -2, "pcall");

    api.pushcclosurek(L, aux.wrap(xpcally), "xpcall", 0, aux.wrapk(xpcallcont));
    if (api.vmflag("LuauFastpcall")) api.setbuiltinpcall(L, -1, true);
    api.setfield(L, -2, "xpcall");

    return 1;
}
