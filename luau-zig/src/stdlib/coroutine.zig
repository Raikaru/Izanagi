//! Coroutine library (upstream VM/src/lcorolib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio).
//!
//! Both fast-flag variants are implemented: with the VM flag LuauCoroutineFinally off
//! (vendored default) the library exposes create/running/status/wrap/yield/isyieldable/
//! close/resume; with it on, `finally` is added and resume/wrap/close run finalizers
//! through the VM's luaD_* finalizer machinery.

const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const State = api.State;

// statuses used by the flag-off resume path (upstream CO_STATUS_*)
const CO_STATUS_ERROR: c_int = -1;
const CO_STATUS_BREAK: c_int = -2;

// dead appears twice for LUA_COERR and LUA_COFIN
const statnames = [_][]const u8{ "running", "suspended", "normal", "dead", "dead" };

inline fn coroutineFinally() bool {
    return api.vmflag("LuauCoroutineFinally");
}

fn costatus(L: *State) c_int {
    const co = aux.checkthread(L, 1);
    api.pushlstring(L, statnames[@intCast(api.costatus(L, co))]);
    return 1;
}

fn interruptThread(L: *State, co: *State) c_int {
    // notify the debugger that the thread was suspended
    api.calldebuginterrupt(L, co);
    return api.@"break"(L);
}

fn auxresumefinisherror(L: *State, wrapped: bool) c_int {
    if (wrapped) {
        if (api.isstring(L, -1)) { // error object is a string?
            aux.where(L, 1); // add extra info
            api.insert(L, -2);
            api.concat(L, 2);
        }
        api.@"error"(L); // propagate error
    }
    api.pushboolean(L, false);
    api.insert(L, -2);
    return 2; // return false + error message
}

fn auxresumefinishsuccess(L: *State, co: *State, wrapped: bool) c_int {
    const nres = api.gettop(co);
    if (nres + 1 > api.MINSTACK and !api.checkstack(L, nres + 1))
        aux.errorf(L, "too many results to resume", .{});
    if (!wrapped) api.pushboolean(L, true);
    if (nres != 0) api.xmove(co, L, nres); // move yielded values
    return nres + @intFromBool(!wrapped); // non-wrap version returns additional status
}

fn auxresumefinish(L: *State, co: *State, status: c_int, wrapped: bool) c_int {
    if (status == api.YIELD) return auxresumefinishsuccess(L, co, wrapped);

    // we might have reached another breakpoint in the target coroutine
    if (status == api.BREAK) return interruptThread(L, co);

    // complete finalizers, but only if it wasn't a failure of the resume itself
    const costat = api.status(co);
    if (status == costat and api.hasfinalizers(co)) {
        if (wrapped)
            aux.errorf(L, "cannot run finalizers from a wrapped coroutine", .{});
        api.preparefinalize(L, co, if (costat == api.OK) api.FinallyFinished else api.FinallyError);
        return api.runfinalizers(L, true);
    }

    if (status == api.OK) return auxresumefinishsuccess(L, co, wrapped);

    api.xmove(co, L, 1);
    return auxresumefinisherror(L, wrapped);
}

fn auxresume(L: *State, co: *State, narg: c_int, wrapped: bool) c_int {
    // error handling for edge cases
    if (api.status(co) != api.YIELD) {
        const status = api.costatus(L, co);
        if (status != api.COSUS) {
            _ = aux.pushfmt(L, "cannot resume {s} coroutine", .{statnames[@intCast(status)]});
            if (coroutineFinally()) return auxresumefinisherror(L, wrapped);
            return CO_STATUS_ERROR;
        }
    }

    if (narg != 0) {
        if (!api.checkstack(co, narg))
            aux.errorf(L, "too many arguments to resume", .{});
        api.xmove(L, co, narg);
    } else {
        // coroutine might be completely full already
        if (api.gettop(co) > api.MAXCSTACK)
            aux.errorf(L, "too many arguments to resume", .{});
    }

    api.copysinglestep(co, L);

    const status = api.@"resume"(co, L, narg);

    if (coroutineFinally()) return auxresumefinish(L, co, status, wrapped);

    if (status == 0 or status == api.YIELD) {
        const nres = api.gettop(co);
        if (nres != 0) {
            // +1 accounts for true/false status in resumefinish
            if (nres + 1 > api.MINSTACK and !api.checkstack(L, nres + 1))
                aux.errorf(L, "too many results to resume", .{});
            api.xmove(co, L, nres); // move yielded values
        }
        return nres;
    } else if (status == api.BREAK) {
        return CO_STATUS_BREAK;
    } else {
        api.xmove(co, L, 1); // move error message
        return CO_STATUS_ERROR;
    }
}

fn auxresumecont(L: *State, co: *State) c_int {
    const st = api.status(co);
    if (st == 0 or st == api.YIELD) {
        const nres = api.gettop(co);
        if (!api.checkstack(L, nres + 1))
            aux.errorf(L, "too many results to resume", .{});
        api.xmove(co, L, nres); // move yielded values
        return nres;
    } else {
        api.rawcheckstack(L, 2);
        api.xmove(co, L, 1); // move error message
        return CO_STATUS_ERROR;
    }
}

fn coresumefinish(L: *State, r: c_int) c_int {
    if (r < 0) {
        api.pushboolean(L, false);
        api.insert(L, -2);
        return 2; // return false + error message
    } else {
        api.pushboolean(L, true);
        api.insert(L, -(r + 1));
        return r + 1; // return true + `resume' returns
    }
}

fn coresumey(L: *State) c_int {
    const co = aux.checkthread(L, 1);
    const narg = api.gettop(L) - 1;
    if (coroutineFinally()) return auxresume(L, co, narg, false);
    const r = auxresume(L, co, narg, false);
    if (r == CO_STATUS_BREAK) return interruptThread(L, co);
    return coresumefinish(L, r);
}

fn coresumecont(L: *State, status: c_int) c_int {
    if (coroutineFinally()) {
        // finalizer errored, return false status and the error
        if (status != api.OK) {
            api.pushboolean(L, false);
            api.insert(L, -2);
            return 2;
        }
        const co = aux.checkthread(L, 1);
        // if we haven't completed auxresume yet
        if (api.gettop(L) == 1) return auxresumefinish(L, co, api.status(co), false);
        return api.runfinalizers(L, true);
    }
    const co = aux.checkthread(L, 1);
    // if coroutine still hasn't yielded after the break, break current thread again
    if (api.status(co) == api.BREAK) return interruptThread(L, co);
    const r = auxresumecont(L, co);
    return coresumefinish(L, r);
}

fn auxwrapfinish(L: *State, r: c_int) c_int {
    if (r < 0) {
        if (api.isstring(L, -1)) { // error object is a string?
            aux.where(L, 1); // add extra info
            api.insert(L, -2);
            api.concat(L, 2);
        }
        api.@"error"(L); // propagate error
    }
    return r;
}

fn auxwrapy(L: *State) c_int {
    const co = api.tothread(L, api.upvalueindex(1)).?;
    const narg = api.gettop(L);
    if (coroutineFinally()) return auxresume(L, co, narg, true);
    const r = auxresume(L, co, narg, true);
    if (r == CO_STATUS_BREAK) return interruptThread(L, co);
    return auxwrapfinish(L, r);
}

fn auxwrapcont(L: *State, status: c_int) c_int {
    _ = status;
    const co = api.tothread(L, api.upvalueindex(1)).?;
    if (coroutineFinally()) return auxresumefinish(L, co, api.status(co), true);
    // we might have reached another breakpoint in the target coroutine
    if (api.status(co) == api.BREAK) return interruptThread(L, co);
    const r = auxresumecont(L, co);
    return auxwrapfinish(L, r);
}

fn cocreate(L: *State) c_int {
    aux.checktype(L, 1, api.TFUNCTION);
    const NL = api.newthread(L);
    api.xpush(L, NL, 1); // push function on top of NL
    return 1;
}

fn cowrap(L: *State) c_int {
    _ = cocreate(L);
    api.pushcclosurek(L, aux.wrap(auxwrapy), null, 1, aux.wrapk(auxwrapcont));
    return 1;
}

fn coyield(L: *State) c_int {
    const nres = api.gettop(L);
    return api.yield(L, nres);
}

fn corunning(L: *State) c_int {
    if (api.pushthread(L))
        api.pushnil(L); // main thread is not a coroutine
    return 1;
}

fn coyieldable(L: *State) c_int {
    api.pushboolean(L, api.isyieldable(L));
    return 1;
}

/// Pushes false + the error of a failed coroutine and resets it.
fn closeerrored(L: *State, co: *State) c_int {
    api.pushboolean(L, false);
    const st = api.status(co);
    if (st == api.ERRMEM) {
        api.pushlstring(L, api.MEMERRMSG);
    } else if (st == api.ERRERR) {
        api.pushlstring(L, api.ERRERRMSG);
    } else if (api.gettop(co) != 0) {
        api.xmove(co, L, 1); // move error message
    }
    api.resetthread(co);
    return 2;
}

fn checkclosable(L: *State, co: *State) void {
    const status = api.costatus(L, co);
    if (status != api.COFIN and status != api.COERR and status != api.COSUS)
        aux.errorf(L, "cannot close {s} coroutine", .{statnames[@intCast(status)]});
}

fn coclose(L: *State) c_int {
    const co = aux.checkthread(L, 1);
    checkclosable(L, co);
    const st = api.status(co);
    if (st == api.OK or st == api.YIELD) {
        api.pushboolean(L, true);
        api.resetthread(co);
        return 1;
    }
    return closeerrored(L, co);
}

fn coclosey(L: *State) c_int {
    const co = aux.checkthread(L, 1);
    checkclosable(L, co);
    const st = api.status(co);
    if (st == api.OK or st == api.YIELD) {
        if (api.hasfinalizers(co)) {
            api.settop(L, 1);
            api.preparefinalizestate(L, co, api.FinallyCancelled);
            api.resetthread(co); // can be reused immediately
            return api.runfinalizers(L, true);
        }
        api.pushboolean(L, true);
        api.resetthread(co);
        return 1;
    }
    // finalizers have already been executed for failed coroutines
    return closeerrored(L, co);
}

fn coclosecont(L: *State, status: c_int) c_int {
    if (status != api.OK) {
        api.pushboolean(L, false);
        api.insert(L, -2);
        return 2;
    }
    return api.runfinalizers(L, true);
}

// coroutine.finally(co, callback)
fn cofinally(L: *State) c_int {
    const co = aux.checkthread(L, 1);
    // callback can be any callable object
    if (api.isnoneornil(L, 2))
        aux.errorf(L, "missing argument #{d}", .{@as(c_int, 2)});
    api.addfinalizer(L, co, 2);
    return 0;
}

const co_funcs_DEPRECATED = [_]aux.Entry{
    aux.entry("create", cocreate),
    aux.entry("running", corunning),
    aux.entry("status", costatus),
    aux.entry("wrap", cowrap),
    aux.entry("yield", coyield),
    aux.entry("isyieldable", coyieldable),
    aux.entry("close", coclose),
};

const co_funcs = [_]aux.Entry{
    aux.entry("create", cocreate),
    aux.entry("running", corunning),
    aux.entry("status", costatus),
    aux.entry("wrap", cowrap),
    aux.entry("yield", coyield),
    aux.entry("isyieldable", coyieldable),
    aux.entry("finally", cofinally),
};

pub fn open(L: *State) c_int {
    if (coroutineFinally()) {
        aux.register(L, "coroutine", &co_funcs);
    } else {
        aux.register(L, "coroutine", &co_funcs_DEPRECATED);
    }

    api.pushcclosurek(L, aux.wrap(coresumey), "resume", 0, aux.wrapk(coresumecont));
    api.setfield(L, -2, "resume");

    if (coroutineFinally()) {
        api.pushcclosurek(L, aux.wrap(coclosey), "close", 0, aux.wrapk(coclosecont));
        api.setfield(L, -2, "close");
    }
    return 1;
}
