//! User-defined classes and objects (port of upstream lclass.cpp, MIT; see
//! object.zig header). This is upstream's experimental runtime behind
//! DebugLuauUserDefinedClassesRuntime / bytecode version 100.

const std = @import("std");
const o = @import("object.zig");
const mem = @import("mem.zig");
const table = @import("table.zig");
const func = @import("func.zig");
const lstring = @import("string.zig");
const gc = @import("gc.zig");
const ldo = @import("do.zig");
const debug = @import("debug.zig");
const vmu = @import("vmutils.zig");

const State = o.State;
const TValue = o.TValue;
const LuauClass = o.LuauClass;
const LuauObject = o.LuauObject;
const TString = o.TString;

fn newblankclass(L: *State, name: *TString, isopen: bool) *LuauClass {
    const c = mem.newGco(L, LuauClass, @sizeOf(LuauClass), o.T_CLASS);
    const g = L.global;
    c.name = name;
    c.id = if (g.lastclassid == 0) 0 else blk: {
        const id = g.lastclassid;
        g.lastclassid +%= 1;
        break :blk id;
    };
    c.super = null;
    c.staticmembers = null;
    c.privatestaticmembers = null;
    c.memberstooffset = null;
    c.offsettomember = null;
    c.metatable = null;
    c.instancemetatable = null;
    c.numberofinstancemembers = 0;
    c.numberofmembers = 0;
    c.numberofprivatefields = 0;
    c.ownprivatefieldsstart = 0;
    c.numberofprivatestaticmembers = 0;
    c.isopen = isopen;
    c.hasuserinitinchain = false;
    return c;
}

fn setupconstructor(L: *State, c: *LuauClass, env: *o.Table) void {
    const g = L.global;
    c.metatable = table.new(L, 0, 1);
    const ctor = func.newCclosure(L, 0, env);
    ctor.f = constructobject;
    ctor.debugname = lstring.new(L, "luaR_constructobject");
    o.setclvalue(table.setstr(L, c.metatable.?, g.tmname[@intFromEnum(o.TMS.CALL)]), ctor);
    c.metatable.?.readonly = 1;
    const def = func.newCclosure(L, 1, env);
    def.f = defaultcreateobject;
    def.debugname = lstring.new(L, "luaR_defaultcreateobject");
    o.setclassvalue(&def.upvals()[0], c);
    const initIndex = table.getstr(c.memberstooffset.?, lstring.new(L, "__init"));
    if (o.ttisnumber(initIndex)) {
        const off: u32 = @intFromFloat(o.nvalue(initIndex));
        if (off >= c.numberofinstancemembers and off < c.numberofmembers)
            o.setclvalue(&c.staticmembers.?[off - c.numberofinstancemembers], def);
    }
}

/// luaR_newclass (called by the loader with the GC paused).
pub fn newclass(L: *State, name: *TString, memberstooffset: *o.Table, offsettomember: [*]?*TString, ninstance: u32, nstatic: u32, nprivatefields: u8, nprivatemethods: u8, envt: *o.Table) *LuauClass {
    const c = newblankclass(L, name, false);
    const sm = mem.newArray(L, TValue, nstatic, c.gch.memcat);
    for (0..nstatic) |i| sm[i] = .{};
    c.staticmembers = sm;
    c.memberstooffset = memberstooffset;
    c.offsettomember = offsettomember;
    c.numberofinstancemembers = ninstance;
    c.numberofmembers = ninstance + nstatic;
    c.numberofprivatefields = nprivatefields;
    c.ownprivatefieldsstart = ninstance - nprivatefields;
    c.numberofprivatestaticmembers = nprivatemethods;
    c.privatestaticmembers = sm + (nstatic - nprivatemethods);
    setupconstructor(L, c, envt);
    return c;
}

fn registerstaticmember(L: *State, c: *LuauClass, name: *TString, val: *const TValue, offset: u32, staticoffset: u32) void {
    c.staticmembers.?[staticoffset] = val.*;
    c.offsettomember.?[offset] = name;
    o.setnvalue(table.setstr(L, c.memberstooffset.?, name), @floatFromInt(offset));
}

/// luaR_inheritclass
pub fn inheritclass(L: *State, child: *LuauClass, parent: *LuauClass) *LuauClass {
    if (!parent.isopen) debug.runerror(L, "Non-open class '{s}' cannot be extended", .{parent.name.slice()});
    var idx: u32 = 0;
    while (idx < parent.numberofinstancemembers) : (idx += 1) {
        const mn = parent.offsettomember.?[idx] orelse continue;
        if (o.ttisnil(table.getstr(child.memberstooffset.?, mn))) continue;
        debug.runerror(L, "Cannot override instance member '{s}' of parent class '{s}' in child class '{s}'", .{ mn.slice(), parent.name.slice(), child.name.slice() });
    }
    const nc = newblankclass(L, child.name, child.isopen);
    nc.super = parent;
    var numStaticToCopy: u32 = 0;
    const parentPublicEnd = parent.offsettomemberlength();
    idx = parent.numberofinstancemembers;
    while (idx < parentPublicEnd) : (idx += 1) {
        if (o.ttisnil(table.getstr(child.memberstooffset.?, parent.offsettomember.?[idx].?))) numStaticToCopy += 1;
    }
    const numMembers = child.numberofmembers + parent.numberofinstancemembers + numStaticToCopy;
    const otmLen = numMembers - child.numberofprivatestaticmembers;
    const otm = mem.newArray(L, ?*TString, otmLen, nc.gch.memcat);
    for (0..otmLen) |i| otm[i] = null;
    nc.offsettomember = otm;
    nc.numberofmembers = numMembers;
    nc.memberstooffset = table.new(L, 0, @intCast(numMembers - child.numberofprivatefields - parent.numberofprivatefields - child.numberofprivatestaticmembers));
    nc.hasuserinitinchain = parent.hasuserinitinchain;
    var offset: u32 = 0;
    while (offset < parent.numberofinstancemembers) : (offset += 1) {
        const mn = parent.offsettomember.?[offset];
        otm[offset] = mn;
        if (mn) |s| o.setnvalue(table.setstr(L, nc.memberstooffset.?, s), @floatFromInt(offset));
    }
    idx = 0;
    while (idx < child.numberofinstancemembers) : ({
        idx += 1;
        offset += 1;
    }) {
        const mn = child.offsettomember.?[idx];
        otm[offset] = mn;
        if (mn) |s| o.setnvalue(table.setstr(L, nc.memberstooffset.?, s), @floatFromInt(offset));
    }
    nc.numberofprivatefields = child.numberofprivatefields + parent.numberofprivatefields;
    nc.ownprivatefieldsstart = child.ownprivatefieldsstart + parent.numberofinstancemembers;
    const nstatic = numMembers - offset;
    const sm = mem.newArray(L, TValue, nstatic, nc.gch.memcat);
    for (0..nstatic) |i| sm[i] = .{};
    nc.staticmembers = sm;
    const privateMethodOffset = child.numberofmembers - child.numberofinstancemembers - child.numberofprivatestaticmembers + numStaticToCopy;
    nc.privatestaticmembers = sm + privateMethodOffset;
    nc.numberofprivatestaticmembers = child.numberofprivatestaticmembers;
    nc.numberofinstancemembers = offset;
    var copied: u32 = 0;
    idx = parent.numberofinstancemembers;
    while (idx < parentPublicEnd) : (idx += 1) {
        const mn = parent.offsettomember.?[idx].?;
        if (o.ttisnil(table.getstr(child.memberstooffset.?, mn))) {
            registerstaticmember(L, nc, mn, &parent.staticmembers.?[idx - parent.numberofinstancemembers], offset, copied);
            offset += 1;
            copied += 1;
        }
    }
    const childPublicEnd = child.offsettomemberlength();
    idx = child.numberofinstancemembers;
    while (idx < childPublicEnd) : (idx += 1) {
        registerstaticmember(L, nc, child.offsettomember.?[idx].?, &child.staticmembers.?[idx - child.numberofinstancemembers], offset, copied);
        offset += 1;
        copied += 1;
    }
    for (0..child.numberofprivatestaticmembers) |i| nc.privatestaticmembers.?[i] = child.privatestaticmembers.?[i];
    if (parent.instancemetatable) |imt| nc.instancemetatable = table.clone(L, imt);
    setupconstructor(L, nc, getcurrenv(L));
    return nc;
}

fn getcurrenv(L: *State) *o.Table {
    if (L.ci == &L.base_ci[0]) return L.gt;
    return o.curr_func(L).env;
}

/// luaR_addpublicstaticmember (NEWCLASSMEMBER, public)
pub fn addpublicstaticmember(L: *State, c: *LuauClass, name: *TString, value: *const TValue) void {
    const g = L.global;
    const offv = table.getstr(c.memberstooffset.?, name);
    const off: u32 = @intFromFloat(o.nvalue(offv));
    c.staticmembers.?[off - c.numberofinstancemembers] = value.*;
    gc.barrier(L, &c.gch, value);
    if (name == lstring.new(L, "__init")) c.hasuserinitinchain = true;
    var ismeta = name == lstring.new(L, "__tostring");
    var i: usize = 0;
    while (i < o.TM_N and !ismeta) : (i += 1) ismeta = name == g.tmname[i];
    if (ismeta) {
        if (c.instancemetatable == null) {
            c.instancemetatable = table.new(L, 0, 1);
            gc.objbarrier(L, &c.gch, &c.instancemetatable.?.gch);
        } else {
            const iscmp = name == g.tmname[@intFromEnum(o.TMS.EQ)] or name == g.tmname[@intFromEnum(o.TMS.LT)] or name == g.tmname[@intFromEnum(o.TMS.LE)];
            if (iscmp and !o.ttisnil(table.getstr(c.instancemetatable.?, name)))
                debug.runerror(L, "Overriding comparison metamethods is not allowed ('{s}' in class '{s}')", .{ name.slice(), c.name.slice() });
        }
        const imt = c.instancemetatable.?;
        table.setstr(L, imt, name).* = value.*;
        gc.barriert(L, imt, value);
    }
}

/// luaR_addprivatestaticmember (NEWCLASSMEMBER, private)
pub fn addprivatestaticmember(L: *State, c: *LuauClass, pindex: u8, value: *const TValue) void {
    c.privatestaticmembers.?[pindex] = value.*;
    gc.barrier(L, &c.gch, value);
}

pub fn newblankobject(L: *State, c: *LuauClass) *LuauObject {
    const obj = mem.newGco(L, LuauObject, @sizeOf(LuauObject), o.T_OBJECT);
    obj.lclass = c;
    obj.numberofmembers = 0;
    obj.members = null;
    const m = mem.newArray(L, TValue, c.numberofinstancemembers, L.activememcat);
    for (0..c.numberofinstancemembers) |i| m[i] = .{};
    obj.members = m;
    obj.numberofmembers = c.numberofinstancemembers;
    return obj;
}

/// luaR_constructobject: the class's __call metamethod.
pub fn constructobject(Lopt: ?*State) callconv(.c) c_int {
    const L = Lopt.?;
    const c = o.classvalue(&L.base[0]);
    const self = newblankobject(L, c);
    const initIndex = table.getstr(c.memberstooffset.?, lstring.new(L, "__init"));
    const initOffset: u32 = @as(u32, @intFromFloat(o.nvalue(initIndex))) - c.numberofinstancemembers;
    const initFunction = c.staticmembers.?[initOffset];
    const numargs: usize = @intCast(o.idiff(L.top, L.base) - 1);
    ldo.checkstack(L, @intCast(3 + numargs));
    o.setobjectvalue(&L.top[0], self);
    L.top += 1;
    const argsBase = L.top;
    L.top[0] = initFunction;
    L.top += 1;
    o.setobjectvalue(&L.top[0], self);
    L.top += 1;
    var i: usize = 1;
    while (i < numargs + 1) : (i += 1) {
        L.top[0] = L.base[i];
        L.top += 1;
    }
    ldo.call(L, argsBase, 0);
    return 1;
}

/// luaR_defaultcreateobject: the default `__init`.
pub fn defaultcreateobject(Lopt: ?*State) callconv(.c) c_int {
    const L = Lopt.?;
    const cl = o.clvalue(&L.ci.func[0]);
    const c = o.classvalue(&cl.upvals()[0]);
    if (c.hasuserinitinchain)
        debug.runerror(L, "Class {s} must define a constructor because it is derived from a class that defines one", .{c.name.slice()});
    if (c.numberofprivatefields != 0) {
        if (c.super != null)
            debug.runerror(L, "Class {s} must define a constructor because it or a class it inherits from has private fields", .{c.name.slice()});
        debug.runerror(L, "Class {s} must define a constructor because it has private fields", .{c.name.slice()});
    }
    const numargs = o.idiff(L.top, L.base);
    if (numargs != 2) debug.runerror(L, "The constructor of {s} must be called with 2 arguments.  Got {d}", .{ c.name.slice(), numargs });
    if (!o.ttisobject(&L.base[0])) debug.runerror(L, "{s}.__init must be called with an instance of the class as its first argument", .{c.name.slice()});
    const inst = o.objectvalue(&L.base[0]);
    if (inst.lclass != c) debug.runerror(L, "Cannot call {s}.__init on an instance of class {s}", .{ c.name.slice(), inst.lclass.name.slice() });
    ldo.checkstack(L, 1);
    o.setnilvalue(&L.top[0]);
    L.top += 1;
    var idx: u32 = 0;
    while (idx < c.numberofinstancemembers) : (idx += 1) {
        const mn = c.offsettomember.?[idx] orelse continue;
        var key: TValue = .{};
        o.setsvalue(&key, mn);
        vmu.gettable(L, &L.base[1], &key, L.top - 1);
        inst.members.?[idx] = (L.top - 1)[0];
        gc.barrier(L, &inst.gch, &inst.members.?[idx]);
    }
    L.top -= 1;
    return 0;
}

fn classmatchesshape(c: *LuauClass, shape: *o.Table) u8 {
    if (c.hasuserinitinchain or c.numberofprivatefields > 0) return o.LCM_MISMATCH;
    var membercount: u32 = 0;
    var defaultinit = true;
    if (shape.node != o.dummynode()) {
        for (0..@intCast(shape.sizenode())) |i| {
            const n = &shape.node[i];
            if (n.key.tt == o.T_NIL) continue;
            if (n.key.tt != o.T_STRING) return o.LCM_MISMATCH;
            const off = table.getstr(c.memberstooffset.?, @ptrCast(n.key.value.gc));
            if (!o.ttisnumber(off)) return o.LCM_MISMATCH;
            if (@as(u32, @intFromFloat(o.nvalue(off))) >= c.numberofinstancemembers) return o.LCM_MISMATCH;
            membercount += 1;
            if (!(o.ttisnumber(&n.val) and o.nvalue(&n.val) == 0.0 and !std.math.signbit(o.nvalue(&n.val)))) defaultinit = false;
        }
    }
    return if (defaultinit and membercount == c.numberofinstancemembers) o.LCM_MATCH_ZEROED else o.LCM_MATCH;
}

/// luaR_tryconstructobject (CONSTRUCT)
pub fn tryconstructobject(L: *State, target: o.StkId, maybeclass: *const TValue, slotid: u32) void {
    const p = L.ci.p.?;
    if (slotid >= p.feedbackvecsize) debug.runerror(L, "malformed bytecode: bad feedback slot", .{});
    const slot = &p.feedbackvec.?[slotid];
    const shapev = &p.k[slot.pc_or_shape];
    if (!o.ttistable(shapev)) debug.runerror(L, "malformed bytecode: bad construct shape", .{});
    const shape = o.hvalue(shapev);
    if (!o.ttisclass(maybeclass)) {
        o.sethvalue(&target[0], table.clone(L, shape));
        return;
    }
    const c = o.classvalue(maybeclass);
    if (slot.proto_or_classid != c.id) {
        if (c.id == 0) {
            slot.match = o.LCM_MISMATCH;
            slot.proto_or_classid = 0;
        } else {
            slot.match = classmatchesshape(c, shape);
            slot.proto_or_classid = c.id;
        }
    }
    if (slot.match != o.LCM_MISMATCH) {
        const self = newblankobject(L, c);
        for (0..c.numberofinstancemembers) |i| {
            if (slot.match == o.LCM_MATCH_ZEROED) {
                o.setnvalue(&self.members.?[i], 0.0);
            } else {
                self.members.?[i] = table.getstr(shape, c.offsettomember.?[i].?).*;
            }
        }
        o.setobjectvalue(&target[0], self);
    } else {
        o.sethvalue(&target[0], table.clone(L, shape));
    }
}

pub fn isinstance(inst: *const LuauObject, cls: *const LuauClass) bool {
    var c: ?*LuauClass = inst.lclass;
    while (c) |x| : (c = x.super) {
        if (x == cls) return true;
    }
    return false;
}

pub fn issubclass(child: *const LuauClass, ancestor: *const LuauClass) bool {
    var c: ?*const LuauClass = child;
    while (c) |x| : (c = x.super) {
        if (x == ancestor) return true;
    }
    return false;
}

/// luaR_lookupmemberatoffset
pub fn memberatoffset(inst: *LuauObject, offset: u32) *TValue {
    const c = inst.lclass;
    return if (offset < c.numberofinstancemembers) &inst.members.?[offset] else &c.staticmembers.?[offset - c.numberofinstancemembers];
}

pub fn freeclass(L: *State, c: *LuauClass) void {
    const mc = c.gch.memcat;
    if (c.staticmembers) |sm| mem.freeArray(L, TValue, sm, c.numstaticmembers(), mc);
    if (c.offsettomember) |otm| mem.freeArray(L, ?*TString, otm, c.offsettomemberlength(), mc);
    mem.freeGco(L, c, @sizeOf(LuauClass), mc);
}

pub fn freeobject(L: *State, obj: *LuauObject) void {
    const mc = obj.gch.memcat;
    if (obj.members) |m| mem.freeArray(L, TValue, m, obj.numberofmembers, mc);
    mem.freeGco(L, obj, @sizeOf(LuauObject), mc);
}

/// luaG_missingmembererror
pub fn missingmembererror(L: *State, p1: *const TValue, p2: *const TValue) noreturn {
    const tm = @import("tm.zig");
    if (!o.ttisstring(p2))
        debug.runerror(L, "cannot index {s} with a {s}", .{ tm.objtypename(L, p1), tm.objtypename(L, p2) });
    debug.runerror(L, "this {s} does not have a key named '{s}'", .{ tm.objtypename(L, p1), std.mem.sliceTo(o.tsvalue(p2).slice(), 0) });
}
