//! Core object model of the Zig Luau VM.
//!
//! Layout and semantics follow upstream Luau (third_party/luau/VM/src/lobject.h,
//! lstate.h), Copyright (c) 2019-2024 Roblox Corporation and Copyright (C) 1994-2008
//! Lua.org, PUC-Rio, MIT licensed (see third_party/luau/LICENSE.txt and
//! lua_LICENSE.txt). The structures are private to the VM; hosts only see opaque
//! `lua_State` pointers through the lua.h ABI.

const std = @import("std");
const builtin = @import("builtin");
const jmp = @import("jmp.zig");

// ---------------------------------------------------------------------------
// Configuration (luaconf.h defaults used by the production build)
// ---------------------------------------------------------------------------

pub const LUA_IDSIZE = 256;
pub const LUA_MINSTACK = 20;
pub const LUAI_MAXCSTACK = 8000;
pub const LUAI_MAXCALLS = 20000;
pub const LUAI_MAXCCALLS = 200;
pub const LUA_BUFFERSIZE = 512;
pub const LUA_UTAG_LIMIT = 128;
pub const LUA_LUTAG_LIMIT = 128;
pub const LUA_MEMORY_CATEGORIES = 256;
pub const LUA_MINSTRTABSIZE = 32;
pub const LUA_VECTOR_SIZE = 3;
pub const EXTRA_SIZE = LUA_VECTOR_SIZE - 2;

pub const LUAI_GCGOAL = 200;
pub const LUAI_GCSTEPMUL = 200;
pub const LUAI_GCSTEPSIZE = 1;

pub const EXTRA_STACK = 5;
pub const BASIC_CI_SIZE = 8;
pub const BASIC_STACK_SIZE = 2 * LUA_MINSTACK;
pub const MAXSSIZE: usize = 1 << 30;
pub const MAX_BUFFER_SIZE: usize = 1 << 30;
pub const MAX_STACK_SIZE: c_int = @intCast((1024 / @sizeOf(TValue)) * 1024 * 1024);
pub const ATOM_UNDEF: i16 = -32768;

pub const UTAG_IDTOR = LUA_UTAG_LIMIT;
pub const UTAG_PROXY = LUA_UTAG_LIMIT + 1;
pub const UTAG_INTERNAL_LIMIT = UTAG_PROXY + 1;
pub const LU_TAG_ITERATOR = LUA_UTAG_LIMIT;

pub const LUA_MEMERRMSG = "not enough memory";
pub const LUA_ERRERRMSG = "error in error handling";

// ---------------------------------------------------------------------------
// lua.h constants
// ---------------------------------------------------------------------------

pub const LUA_MULTRET: c_int = -1;
pub const LUA_REGISTRYINDEX: c_int = -LUAI_MAXCSTACK - 2000;
pub const LUA_ENVIRONINDEX: c_int = -LUAI_MAXCSTACK - 2001;
pub const LUA_GLOBALSINDEX: c_int = -LUAI_MAXCSTACK - 2002;

pub const LUA_OK: c_int = 0;
pub const LUA_YIELD: c_int = 1;
pub const LUA_ERRRUN: c_int = 2;
pub const LUA_ERRSYNTAX: c_int = 3;
pub const LUA_ERRMEM: c_int = 4;
pub const LUA_ERRERR: c_int = 5;
pub const LUA_BREAK: c_int = 6;

pub const LUA_CORUN: c_int = 0;
pub const LUA_COSUS: c_int = 1;
pub const LUA_CONOR: c_int = 2;
pub const LUA_COFIN: c_int = 3;
pub const LUA_COERR: c_int = 4;

pub const LUA_TNONE: c_int = -1;
pub const T_NIL: u8 = 0;
pub const T_BOOLEAN: u8 = 1;
pub const T_LIGHTUSERDATA: u8 = 2;
pub const T_NUMBER: u8 = 3;
pub const T_INTEGER: u8 = 4;
pub const T_VECTOR: u8 = 5;
pub const T_STRING: u8 = 6;
pub const T_TABLE: u8 = 7;
pub const T_FUNCTION: u8 = 8;
pub const T_USERDATA: u8 = 9;
pub const T_THREAD: u8 = 10;
pub const T_BUFFER: u8 = 11;
pub const T_CLASS: u8 = 12;
pub const T_OBJECT: u8 = 13;
pub const T_DEADKEY: u8 = 14;
pub const T_PROTO: u8 = 15;
pub const T_UPVAL: u8 = 16;
pub const T_ALL: u8 = 17;
pub const T_COUNT: usize = T_DEADKEY;

pub const LUA_NOREF: c_int = -1;
pub const LUA_REFNIL: c_int = 0;

// GC options (enum lua_GCOp)
pub const LUA_GCSTOP: c_int = 0;
pub const LUA_GCRESTART: c_int = 1;
pub const LUA_GCCOLLECT: c_int = 2;
pub const LUA_GCCOUNT: c_int = 3;
pub const LUA_GCCOUNTB: c_int = 4;
pub const LUA_GCISRUNNING: c_int = 5;
pub const LUA_GCSTEP: c_int = 6;
pub const LUA_GCSETGOAL: c_int = 7;
pub const LUA_GCSETSTEPMUL: c_int = 8;
pub const LUA_GCSETSTEPSIZE: c_int = 9;
pub const LUA_GCISPAUSED: c_int = 10;

// ---------------------------------------------------------------------------
// Host-visible function types (lua.h)
// ---------------------------------------------------------------------------

pub const lua_CFunction = *const fn (?*State) callconv(.c) c_int;
pub const lua_Continuation = *const fn (?*State, c_int) callconv(.c) c_int;
pub const lua_Alloc = *const fn (?*anyopaque, ?*anyopaque, usize, usize) callconv(.c) ?*anyopaque;
pub const lua_Destructor = *const fn (?*State, ?*anyopaque) callconv(.c) void;
pub const lua_Hook = *const fn (?*State, *lua_Debug) callconv(.c) void;
pub const lua_CageAlloc = *const fn (?*anyopaque, ?*anyopaque, usize, usize, c_int) callconv(.c) ?*anyopaque;

pub const lua_Debug = extern struct {
    name: ?[*:0]const u8 = null,
    what: ?[*:0]const u8 = null,
    source: ?[*:0]const u8 = null,
    short_src: ?[*:0]const u8 = null,
    linedefined: c_int = 0,
    currentline: c_int = 0,
    protoid: c_int = 0,
    bytecodeid: c_int = 0,
    nupvals: u8 = 0,
    nparams: u8 = 0,
    isvararg: u8 = 0,
    userdata: ?*anyopaque = null,
    ssbuf: [LUA_IDSIZE]u8 = undefined,
};

pub const lua_Callbacks = extern struct {
    userdata: ?*anyopaque = null,
    interrupt: ?*const fn (?*State, c_int) callconv(.c) void = null,
    panic: ?*const fn (?*State, c_int) callconv(.c) void = null,
    userthread: ?*const fn (?*State, ?*State) callconv(.c) void = null,
    useratom: ?*const fn (?*State, [*]const u8, usize) callconv(.c) i16 = null,
    userfinalizer: ?*const fn (?*State, ?*State) callconv(.c) void = null,
    debugbreak: ?lua_Hook = null,
    debugstep: ?lua_Hook = null,
    debuginterrupt: ?lua_Hook = null,
    debugprotectederror: ?*const fn (?*State) callconv(.c) void = null,
    onallocate: ?*const fn (?*State, ?*anyopaque, usize, usize, u8, c_int, c_int) callconv(.c) void = null,
    preresume: ?*const fn (?*State) callconv(.c) void = null,
    postresume: ?*const fn (?*State) callconv(.c) void = null,
    onfree: ?*const fn (?*State, ?*anyopaque) callconv(.c) void = null,
};

// ---------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------

pub const Value = extern union {
    gc: *GCObject,
    p: ?*anyopaque,
    n: f64,
    b: c_int,
    l: i64,
    v: [2]f32,
};

pub const TValue = extern struct {
    value: Value = .{ .p = null },
    extra: [EXTRA_SIZE]c_int = @splat(0),
    tt: c_int = T_NIL,
};

pub const StkId = [*]TValue;

pub const nilobject_storage: TValue = .{};
/// Returned for invalid/absent slots; never written to.
pub inline fn nilobject() *TValue {
    return @constCast(&nilobject_storage);
}

pub const GCObject = extern struct {
    tt: u8,
    marked: u8,
    memcat: u8,
    pad: u8 = 0,
    next: ?*GCObject = null, // all-objects list used by the collector
    gclist: ?*GCObject = null, // gray/weak list link used during a collection
};

pub const TString = extern struct {
    gch: GCObject,
    atom: i16,
    pad: u16 = 0,
    hash: u32,
    len: u32,
    hnext: ?*TString, // next string in the intern hash bucket

    pub inline fn data(s: *const TString) [*]u8 {
        return @as([*]u8, @ptrCast(@constCast(s))) + @sizeOf(TString);
    }
    pub inline fn slice(s: *const TString) []u8 {
        return s.data()[0..s.len];
    }
    pub inline fn cstr(s: *const TString) [*:0]const u8 {
        return @ptrCast(s.data());
    }
};

pub const Udata = extern struct {
    gch: GCObject,
    tag: u8,
    pad: [3]u8 = .{ 0, 0, 0 },
    len: c_int,
    metatable: ?*Table,

    pub const header = std.mem.alignForward(usize, @sizeOf(Udata), 16);
    pub inline fn data(u: *const Udata) [*]u8 {
        return @as([*]u8, @ptrCast(@constCast(u))) + header;
    }
    pub inline fn fromData(p: *anyopaque) *Udata {
        return @ptrCast(@alignCast(@as([*]u8, @ptrCast(p)) - header));
    }
};

pub const Buffer = extern struct {
    gch: GCObject,
    len: c_uint,

    pub const header = std.mem.alignForward(usize, @sizeOf(Buffer), 8);
    pub inline fn data(b: *const Buffer) [*]u8 {
        return @as([*]u8, @ptrCast(@constCast(b))) + header;
    }
};

pub const LocVar = extern struct {
    varname: ?*TString,
    startpc: c_int,
    endpc: c_int,
    reg: u8,
};

pub const Proto = extern struct {
    gch: GCObject,
    nups: u8 = 0,
    numparams: u8 = 0,
    is_vararg: u8 = 0,
    maxstacksize: u8 = 0,
    flags: u8 = 0,
    linegaplog2: u8 = 0,
    k: [*]TValue = undefined,
    code: [*]u32 = undefined,
    p: [*]*Proto = undefined,
    lineinfo: ?[*]u8 = null,
    abslineinfo: ?[*]c_int = null,
    locvars: ?[*]LocVar = null,
    upvalues: ?[*]?*TString = null,
    source: ?*TString = null,
    debugname: ?*TString = null,
    debuginsn: ?[*]u8 = null,
    typeinfo: ?[*]u8 = null,
    sizecode: c_int = 0,
    sizep: c_int = 0,
    sizelocvars: c_int = 0,
    sizeupvalues: c_int = 0,
    sizek: c_int = 0,
    sizelineinfo: c_int = 0,
    linedefined: c_int = 0,
    bytecodeid: c_int = 0,
    sizetypeinfo: c_int = 0,
    funid: u32 = 0,
    cost: u64 = 0,
    feedbackvec: ?[*]FeedbackSlot = null,
    feedbackvecsize: u32 = 0,
};

pub const LFT_CALLTARGET: u8 = 0;
pub const LFT_CONSTRUCT: u8 = 1;
pub const LCM_MISMATCH: u8 = 0;
pub const LCM_MATCH: u8 = 1;
pub const LCM_MATCH_ZEROED: u8 = 2;

/// Per-proto runtime feedback slot (upstream FeedbackVectorSlot).
pub const FeedbackSlot = extern struct {
    kind: u8 = 0,
    match: u8 = LCM_MISMATCH, // CONSTRUCT
    pc_or_shape: u32 = 0, // CALL_TARGET pc / CONSTRUCT shape constant
    proto_or_classid: u32 = 0, // CALL_TARGET proto id / CONSTRUCT class id
    hits: u32 = 0,
};

/// User-defined class (upstream LuauClass, experimental classes runtime).
pub const LuauClass = extern struct {
    gch: GCObject,
    name: *TString,
    id: u32,
    super: ?*LuauClass,
    staticmembers: ?[*]TValue,
    privatestaticmembers: ?[*]TValue,
    memberstooffset: ?*Table,
    offsettomember: ?[*]?*TString,
    metatable: ?*Table,
    instancemetatable: ?*Table,
    numberofinstancemembers: u32,
    numberofmembers: u32,
    numberofprivatefields: u32,
    ownprivatefieldsstart: u32,
    numberofprivatestaticmembers: u32,
    isopen: bool,
    hasuserinitinchain: bool,

    pub inline fn offsettomemberlength(c: *const LuauClass) u32 {
        return c.numberofmembers - c.numberofprivatestaticmembers;
    }
    pub inline fn numstaticmembers(c: *const LuauClass) u32 {
        return c.numberofmembers - c.numberofinstancemembers;
    }
};

/// Instance of a user-defined class (upstream LuauObject).
pub const LuauObject = extern struct {
    gch: GCObject,
    lclass: *LuauClass,
    numberofmembers: u32,
    members: ?[*]TValue,
};

pub const UpVal = extern struct {
    gch: GCObject,
    markedopen: u8 = 0,
    v: *TValue,
    value: TValue = .{}, // the value when closed
    threadnext: ?*UpVal = null, // open upvalues list of the owning thread
    // global doubly linked list of all open upvalues (upstream g->uvhead)
    prev: ?*UpVal = null,
    next: ?*UpVal = null,

    pub inline fn isOpen(uv: *const UpVal) bool {
        return uv.v != &uv.value;
    }
};

pub const Closure = extern struct {
    gch: GCObject,
    isC: u8,
    nupvalues: u8,
    stacksize: u8,
    preload: u8,
    env: *Table,
    // C closure fields
    f: ?lua_CFunction = null,
    cont: ?lua_Continuation = null,
    debugname: ?*TString = null,
    // Lua closure field
    p: ?*Proto = null,

    pub const header = std.mem.alignForward(usize, @sizeOf(Closure), @alignOf(TValue));
    /// Upvalues: for C closures the values themselves, for Lua closures either values
    /// (captured by value) or TValues tagged T_UPVAL that reference an UpVal object.
    pub inline fn upvals(c: *const Closure) [*]TValue {
        return @ptrCast(@alignCast(@as([*]u8, @ptrCast(@constCast(c))) + header));
    }
    pub inline fn sizeFor(n: usize) usize {
        return header + n * @sizeOf(TValue);
    }
};

pub const TKey = extern struct {
    value: Value = .{ .p = null },
    extra: [EXTRA_SIZE]c_int = @splat(0),
    tt: c_int = T_NIL,
    next: c_int = 0, // offset to the next node in the collision chain
};

pub const LuaNode = extern struct {
    val: TValue = .{},
    key: TKey = .{},
};

pub var dummynode_storage: LuaNode = .{};
pub inline fn dummynode() [*]LuaNode {
    return @ptrCast(&dummynode_storage);
}

pub const Table = extern struct {
    gch: GCObject,
    tmcache: u8,
    readonly: u8,
    safeenv: u8,
    lsizenode: u8,
    nodemask8: u8,
    sizearray: c_int,
    /// Upstream stores `lastfree` and `aboundary` in a union; the observable `#t`
    /// boundary hints depend on that sharing, so it is kept identical.
    lastfree: c_int,
    metatable: ?*Table,
    array: ?[*]TValue,
    node: [*]LuaNode,

    pub inline fn aboundary(t: *const Table) c_int {
        return t.lastfree;
    }
    pub inline fn setAboundary(t: *Table, v: c_int) void {
        t.lastfree = v;
    }
    pub inline fn sizenode(t: *const Table) c_int {
        return @as(c_int, 1) << @intCast(t.lsizenode);
    }
    pub inline fn arr(t: *const Table) [*]TValue {
        return t.array.?;
    }
};

pub const CallInfo = extern struct {
    base: StkId,
    func: StkId,
    top: StkId,
    p: ?*Proto,
    savedpc: ?[*]const u32, // for Lua functions; `errfunc` below for C functions
    errfunc: c_int,
    nresults: c_int,
    flags: c_uint,
};

pub const CALLINFO_RETURN: c_uint = 1 << 0;
pub const CALLINFO_HANDLE: c_uint = 1 << 1;
pub const CALLINFO_NATIVE: c_uint = 1 << 2;
pub const CALLINFO_OPYIELD: c_uint = 1 << 3;
pub const CALLINFO_PCALL: c_uint = 1 << 4;

pub const SCHEDULED_REENTRY: u8 = 0x7f;

pub const State = extern struct {
    gch: GCObject,
    status: u8,
    activememcat: u8,
    isactive: bool,
    singlestep: bool,
    top: StkId,
    base: StkId,
    global: *Global,
    ci: *CallInfo,
    stack_last: StkId,
    stack: StkId,
    end_ci: *CallInfo,
    base_ci: [*]CallInfo,
    stacksize: c_int,
    size_ci: c_int,
    nCcalls: c_ushort,
    baseCcalls: c_ushort,
    cachedslot: c_int,
    gt: *Table,
    openupval: ?*UpVal,
    namecall: ?*TString,
    userdata: ?*anyopaque,
    finalizers: ?*Table, // coroutine.finally callbacks (LuauCoroutineFinally)
};

pub const StringTable = extern struct {
    hash: [*]?*TString,
    nuse: u32,
    size: c_int,
};

pub const LongJmp = extern struct {
    prev: ?*LongJmp,
    status: c_int,
    buf: jmp.JmpBuf,
};

pub const Global = extern struct {
    strt: StringTable,
    frealloc: lua_Alloc,
    ud: ?*anyopaque,
    GCthreshold: usize,
    totalbytes: usize,
    gcgoal: c_int,
    gcstepmul: c_int,
    gcstepsize: c_int,
    mainthread: *State,
    allgc: ?*GCObject,
    mt: [T_COUNT]?*Table,
    ttname: [T_COUNT]*TString,
    tmname: [TM_N]*TString,
    memerrmsg: *TString,
    errerrmsg: *TString,
    pseudotemp: TValue,
    registry: TValue,
    registryfree: c_int,
    errorjmp: ?*LongJmp,
    rngstate: u64,
    ptrenckey: [4]u64,
    cb: lua_Callbacks,
    memcatbytes: [LUA_MEMORY_CATEGORIES]usize,
    udatagc: [LUA_UTAG_LIMIT]?lua_Destructor,
    udatamt: [LUA_UTAG_LIMIT]?*Table,
    lightuserdataname: [LUA_LUTAG_LIMIT]?*TString,
    udatadirectfields: [UTAG_INTERNAL_LIMIT]?*Table,
    udatadirect: [UTAG_INTERNAL_LIMIT]UdataDirectAccessData,
    ptrenckeynew: [8]u64,
    ptrencactive: bool,
    builtinPcall: ?*Closure,
    builtinXpcall: ?*Closure,
    lastprotoid: u32,
    lastclassid: u32,
    gcrunning: bool,
    gcinprogress: bool,
    // incremental collector state (upstream global_State)
    currentwhite: u8,
    gcstate: u8,
    gray: ?*GCObject,
    grayagain: ?*GCObject,
    weak: ?*GCObject,
    sweepcursor: ?*?*GCObject,
    uvhead: UpVal,
    gcstats: GCStats,
    // LuauGcTraceUdata
    weakregistry: TValue,
    weakregistryfree: c_int,
    weakregistrytop: c_int,
    udatamark: [LUA_UTAG_LIMIT]?lua_UserdataMark,
    embeddergc: ?lua_EmbedderGc,
    // LuauBufferCage
    cagealloc: ?lua_CageAlloc,
    cageud: ?*anyopaque,
};

pub const lua_UserdataDirectAccess = *const fn (?*State, ?*anyopaque, c_int, *u16, c_int) callconv(.c) void;
pub const lua_UserdataDirectNamecall = *const fn (?*State, ?*anyopaque, c_int, *u16, c_int) callconv(.c) c_int;

/// Per-tag direct userdata access callbacks (lua_registeruserdatadirectaccess).
pub const UdataDirectAccessData = extern struct {
    indextm: TValue = .{},
    newindextm: TValue = .{},
    namecalltm: TValue = .{},
    index: ?lua_UserdataDirectAccess = null,
    newindex: ?lua_UserdataDirectAccess = null,
    namecall: ?lua_UserdataDirectNamecall = null,
};

pub const GCStats = extern struct {
    triggerterms: [32]i32 = @splat(0),
    triggertermpos: u32 = 0,
    triggerintegral: i32 = 0,
    atomicstarttotalsizebytes: usize = 0,
    endtotalsizebytes: usize = 0,
    heapgoalsizebytes: usize = 0,
    starttimestamp: f64 = 0,
    atomicstarttimestamp: f64 = 0,
    endtimestamp: f64 = 0,
};

pub const lua_UserdataMark = *const fn (?*State, ?*anyopaque) callconv(.c) void;
pub const lua_EmbedderMark = *const fn (?*State, c_int) callconv(.c) void;
pub const lua_EmbedderGc = *const fn (?*State, ?lua_EmbedderMark) callconv(.c) void;

/// Main thread and global state are allocated together, like upstream `LG`.
pub const LG = extern struct {
    l: State,
    g: Global,
};

// ---------------------------------------------------------------------------
// Metamethod events (ltm.h, ORDER TM)
// ---------------------------------------------------------------------------

pub const TMS = enum(u8) {
    INDEX,
    NEWINDEX,
    MODE,
    NAMECALL,
    CALL,
    ITER,
    LEN,
    EQ,
    ADD,
    SUB,
    MUL,
    DIV,
    IDIV,
    MOD,
    POW,
    UNM,
    LT,
    LE,
    CONCAT,
    TYPE,
    METATABLE,
};
pub const TM_N = 21;

pub const eventnames = [TM_N][:0]const u8{
    "__index",  "__newindex", "__mode", "__namecall", "__call", "__iter", "__len",
    "__eq",     "__add",      "__sub",  "__mul",      "__div",  "__idiv", "__mod",
    "__pow",    "__unm",      "__lt",   "__le",       "__concat", "__type", "__metatable",
};

pub const typenames = [T_COUNT][:0]const u8{
    "nil",    "boolean", "userdata", "number", "integer", "vector",
    "string", "table",   "function", "userdata", "thread", "buffer",
    "class",  "object",
};

// ---------------------------------------------------------------------------
// Value helpers (lobject.h macros)
// ---------------------------------------------------------------------------

pub inline fn ttype(o: *const TValue) u8 {
    return @intCast(o.tt);
}
pub inline fn ttisnil(o: *const TValue) bool {
    return o.tt == T_NIL;
}
pub inline fn ttisnumber(o: *const TValue) bool {
    return o.tt == T_NUMBER;
}
pub inline fn ttisinteger(o: *const TValue) bool {
    return o.tt == T_INTEGER;
}
pub inline fn ttisstring(o: *const TValue) bool {
    return o.tt == T_STRING;
}
pub inline fn ttistable(o: *const TValue) bool {
    return o.tt == T_TABLE;
}
pub inline fn ttisfunction(o: *const TValue) bool {
    return o.tt == T_FUNCTION;
}
pub inline fn ttisboolean(o: *const TValue) bool {
    return o.tt == T_BOOLEAN;
}
pub inline fn ttisuserdata(o: *const TValue) bool {
    return o.tt == T_USERDATA;
}
pub inline fn ttisthread(o: *const TValue) bool {
    return o.tt == T_THREAD;
}
pub inline fn ttisbuffer(o: *const TValue) bool {
    return o.tt == T_BUFFER;
}
pub inline fn ttislightuserdata(o: *const TValue) bool {
    return o.tt == T_LIGHTUSERDATA;
}
pub inline fn ttisvector(o: *const TValue) bool {
    return o.tt == T_VECTOR;
}
pub inline fn ttisupval(o: *const TValue) bool {
    return o.tt == T_UPVAL;
}
pub inline fn iscollectable(o: *const TValue) bool {
    return o.tt >= T_STRING;
}
pub inline fn l_isfalse(o: *const TValue) bool {
    return o.tt == T_NIL or (o.tt == T_BOOLEAN and o.value.b == 0);
}

pub inline fn gcvalue(o: *const TValue) *GCObject {
    return o.value.gc;
}
pub inline fn nvalue(o: *const TValue) f64 {
    return o.value.n;
}
pub inline fn lvalue(o: *const TValue) i64 {
    return o.value.l;
}
pub inline fn bvalue(o: *const TValue) c_int {
    return o.value.b;
}
pub inline fn pvalue(o: *const TValue) ?*anyopaque {
    return o.value.p;
}
pub inline fn lightuserdatatag(o: *const TValue) c_int {
    return o.extra[0];
}
pub inline fn tsvalue(o: *const TValue) *TString {
    return @ptrCast(o.value.gc);
}
pub inline fn hvalue(o: *const TValue) *Table {
    return @ptrCast(@alignCast(o.value.gc));
}
pub inline fn clvalue(o: *const TValue) *Closure {
    return @ptrCast(@alignCast(o.value.gc));
}
pub inline fn uvalue(o: *const TValue) *Udata {
    return @ptrCast(@alignCast(o.value.gc));
}
pub inline fn thvalue(o: *const TValue) *State {
    return @ptrCast(@alignCast(o.value.gc));
}
pub inline fn bufvalue(o: *const TValue) *Buffer {
    return @ptrCast(@alignCast(o.value.gc));
}
pub inline fn upvalue(o: *const TValue) *UpVal {
    return @ptrCast(@alignCast(o.value.gc));
}
pub inline fn classvalue(o: *const TValue) *LuauClass {
    return @ptrCast(@alignCast(o.value.gc));
}
pub inline fn objectvalue(o: *const TValue) *LuauObject {
    return @ptrCast(@alignCast(o.value.gc));
}
pub inline fn ttisclass(o: *const TValue) bool {
    return o.tt == T_CLASS;
}
pub inline fn ttisobject(o: *const TValue) bool {
    return o.tt == T_OBJECT;
}
pub inline fn vvalue(o: *const TValue) [LUA_VECTOR_SIZE]f32 {
    return .{ o.value.v[0], o.value.v[1], @bitCast(o.extra[0]) };
}

pub inline fn setnilvalue(o: *TValue) void {
    o.tt = T_NIL;
}
pub inline fn setnvalue(o: *TValue, x: f64) void {
    o.value = .{ .n = x };
    o.tt = T_NUMBER;
}
pub inline fn setlvalue(o: *TValue, x: i64) void {
    o.value = .{ .l = x };
    o.tt = T_INTEGER;
}
pub inline fn setbvalue(o: *TValue, x: bool) void {
    o.value = .{ .b = @intFromBool(x) };
    o.tt = T_BOOLEAN;
}
pub inline fn setpvalue(o: *TValue, x: ?*anyopaque, tag: c_int) void {
    o.value = .{ .p = x };
    o.extra[0] = tag;
    o.tt = T_LIGHTUSERDATA;
}
pub inline fn setvvalue(o: *TValue, x: f32, y: f32, z: f32) void {
    o.value = .{ .v = .{ x, y } };
    o.extra[0] = @bitCast(z);
    o.tt = T_VECTOR;
}
pub inline fn setgcvalue(o: *TValue, x: *anyopaque, tt: u8) void {
    o.value = .{ .gc = @ptrCast(@alignCast(x)) };
    o.tt = tt;
}
pub inline fn setsvalue(o: *TValue, x: *TString) void {
    setgcvalue(o, x, T_STRING);
}
pub inline fn sethvalue(o: *TValue, x: *Table) void {
    setgcvalue(o, x, T_TABLE);
}
pub inline fn setclvalue(o: *TValue, x: *Closure) void {
    setgcvalue(o, x, T_FUNCTION);
}
pub inline fn setuvalue(o: *TValue, x: *Udata) void {
    setgcvalue(o, x, T_USERDATA);
}
pub inline fn setthvalue(o: *TValue, x: *State) void {
    setgcvalue(o, x, T_THREAD);
}
pub inline fn setbufvalue(o: *TValue, x: *Buffer) void {
    setgcvalue(o, x, T_BUFFER);
}
pub inline fn setclassvalue(o: *TValue, x: *LuauClass) void {
    setgcvalue(o, x, T_CLASS);
}
pub inline fn setobjectvalue(o: *TValue, x: *LuauObject) void {
    setgcvalue(o, x, T_OBJECT);
}
pub inline fn setupvalue(o: *TValue, x: *UpVal) void {
    setgcvalue(o, x, T_UPVAL);
}
pub inline fn setobj(dst: *TValue, src: *const TValue) void {
    dst.* = src.*;
}

/// Node key access.
pub inline fn gkey(n: *LuaNode) *TKey {
    return &n.key;
}
pub inline fn gval(n: *LuaNode) *TValue {
    return &n.val;
}
pub inline fn keyAsValue(k: *const TKey) *const TValue {
    // TKey starts with the same value/extra/tt prefix as TValue.
    return @ptrCast(k);
}
pub inline fn setnodekey(n: *LuaNode, key: *const TValue) void {
    n.key.value = key.value;
    n.key.extra = key.extra;
    n.key.tt = key.tt;
}
pub inline fn getnodekey(dst: *TValue, n: *const LuaNode) void {
    dst.value = n.key.value;
    dst.extra = n.key.extra;
    dst.tt = n.key.tt;
}

// ---------------------------------------------------------------------------
// Pointer helpers
// ---------------------------------------------------------------------------

pub inline fn sdiff(a: anytype, b: @TypeOf(a)) isize {
    const T = @typeInfo(@TypeOf(a)).pointer.child;
    const ai: isize = @bitCast(@intFromPtr(a));
    const bi: isize = @bitCast(@intFromPtr(b));
    return @divExact(ai - bi, @as(isize, @sizeOf(T)));
}
pub inline fn idiff(a: anytype, b: @TypeOf(a)) c_int {
    return @intCast(sdiff(a, b));
}
pub inline fn offs(p: StkId, n: isize) StkId {
    return if (n >= 0) p + @as(usize, @intCast(n)) else p - @as(usize, @intCast(-n));
}
pub inline fn ptrLE(a: anytype, b: @TypeOf(a)) bool {
    return @intFromPtr(a) <= @intFromPtr(b);
}
pub inline fn ptrLT(a: anytype, b: @TypeOf(a)) bool {
    return @intFromPtr(a) < @intFromPtr(b);
}
pub inline fn one(p: StkId) *TValue {
    return &p[0];
}
pub inline fn toStk(p: *TValue) StkId {
    return @ptrCast(p);
}

pub inline fn ci_func(ci: *const CallInfo) *Closure {
    return clvalue(&ci.func[0]);
}
pub inline fn isLua(ci: *const CallInfo) bool {
    return ttisfunction(&ci.func[0]) and ci_func(ci).isC == 0;
}
pub inline fn curr_func(L: *State) *Closure {
    return clvalue(&L.ci.func[0]);
}

pub inline fn lmod(s: u32, size: u32) u32 {
    return s & (size - 1);
}

pub fn ceillog2(x: c_int) c_int {
    return log2(@intCast(x - 1)) + 1;
}
pub fn log2(x: u32) c_int {
    if (x == 0) return -1;
    return 31 - @as(c_int, @clz(x));
}

/// C-style `(int)d` conversion; out of range values produce INT_MIN like x86 cvttsd2si.
pub inline fn num2int(d: f64) c_int {
    if (d >= -2147483648.0 and d < 2147483648.0) return @intFromFloat(d);
    return std.math.minInt(c_int);
}
/// `(unsigned)(long long)d`
pub inline fn num2unsigned(d: f64) c_uint {
    if (d >= -9223372036854775808.0 and d < 9223372036854775808.0) {
        const ll: i64 = @intFromFloat(d);
        return @truncate(@as(u64, @bitCast(ll)));
    }
    return 0;
}
/// `(int64_t)d`
pub inline fn num2long(d: f64) i64 {
    if (d >= -9223372036854775808.0 and d < 9223372036854775808.0) return @intFromFloat(d);
    return std.math.minInt(i64);
}
