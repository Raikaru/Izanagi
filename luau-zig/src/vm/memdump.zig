//! lua_memorydump: JSON heap dump (port of luaC_dump from upstream lgcdebug.cpp,
//! MIT; see object.zig header).
//!
//! The output target is the host's C `FILE*`, which only the C runtime that owns it
//! can write. When the VM is built with libc linked (the optional C-ABI host bridge),
//! output goes through that libc's `fwrite`. The default libc-free build cannot
//! write an opaque `FILE*`, so the call raises a Lua error there instead.
//!
//! Differences from upstream: object order follows the VM's all-objects list instead
//! of allocator pages, and the reported sizes are this VM's object sizes.

const std = @import("std");
const builtin = @import("builtin");
const o = @import("object.zig");
const debug = @import("debug.zig");
const func = @import("func.zig");
const gc = @import("gc.zig");

const State = o.State;
const TValue = o.TValue;
const GCObject = o.GCObject;

pub const CategoryName = *const fn (?*State, u8) callconv(.c) ?[*:0]const u8;

extern "c" fn fwrite(ptr: [*]const u8, size: usize, n: usize, stream: *anyopaque) usize;

const Out = struct {
    file: *anyopaque,
    buf: [4096]u8 = undefined,
    len: usize = 0,

    fn flush(self: *Out) void {
        if (self.len > 0) _ = fwrite(&self.buf, 1, self.len, self.file);
        self.len = 0;
    }
    fn write(self: *Out, s: []const u8) void {
        for (s) |c| {
            if (self.len == self.buf.len) self.flush();
            self.buf[self.len] = c;
            self.len += 1;
        }
    }
    fn print(self: *Out, comptime fmt: []const u8, args: anytype) void {
        var tmp: [512]u8 = undefined;
        const s = std.fmt.bufPrint(&tmp, fmt, args) catch tmp[0..];
        self.write(s);
    }
};

/// "%p" as the host C runtime prints it.
fn pointer(out: *Out, p: *const anyopaque) void {
    const v = @intFromPtr(p);
    if (builtin.os.tag == .windows) {
        out.print("{X:0>" ++ std.fmt.comptimePrint("{d}", .{@sizeOf(usize) * 2}) ++ "}", .{v});
    } else {
        out.print("0x{x}", .{v});
    }
}

fn dumpref(out: *Out, obj: *const GCObject) void {
    out.write("\"");
    pointer(out, obj);
    out.write("\"");
}

fn dumprefs(out: *Out, data: [*]const TValue, size: usize) void {
    var first = true;
    for (0..size) |i| {
        if (!o.iscollectable(&data[i])) continue;
        if (!first) out.write(",");
        first = false;
        dumpref(out, data[i].value.gc);
    }
}

fn dumpstringdata(out: *Out, data: []const u8) void {
    for (data) |c| {
        const safe = c < 128 and c >= 32 and c != '\\' and c != '"';
        out.write(&[_]u8{if (safe) c else '?'});
    }
}

fn dumpstring(out: *Out, ts: *o.TString) void {
    out.print("{{\"type\":\"string\",\"cat\":{d},\"size\":{d},\"data\":\"", .{ ts.gch.memcat, @sizeOf(o.TString) + ts.len + 1 });
    dumpstringdata(out, ts.slice());
    out.write("\"}");
}

fn dumptable(out: *Out, h: *o.Table) void {
    const nodes: usize = if (h.node == o.dummynode()) 0 else @intCast(h.sizenode());
    const size = @sizeOf(o.Table) + nodes * @sizeOf(o.LuaNode) + @as(usize, @intCast(h.sizearray)) * @sizeOf(TValue);
    out.print("{{\"type\":\"table\",\"cat\":{d},\"size\":{d}", .{ h.gch.memcat, size });
    if (nodes > 0) {
        out.write(",\"pairs\":[");
        var first = true;
        for (0..nodes) |i| {
            const n = &h.node[i];
            if (!o.ttisnil(&n.val) and (o.iscollectable(o.keyAsValue(&n.key)) or o.iscollectable(&n.val))) {
                if (!first) out.write(",");
                first = false;
                if (o.iscollectable(o.keyAsValue(&n.key))) dumpref(out, n.key.value.gc) else out.write("null");
                out.write(",");
                if (o.iscollectable(&n.val)) dumpref(out, n.val.value.gc) else out.write("null");
            }
        }
        out.write("]");
    }
    if (h.sizearray > 0) {
        out.write(",\"array\":[");
        dumprefs(out, h.arr(), @intCast(h.sizearray));
        out.write("]");
    }
    if (h.metatable) |mt| {
        out.write(",\"metatable\":");
        dumpref(out, &mt.gch);
    }
    out.write("}");
}

fn dumpclosure(out: *Out, cl: *o.Closure) void {
    out.print("{{\"type\":\"function\",\"cat\":{d},\"size\":{d}", .{ cl.gch.memcat, func.closureSize(cl) });
    out.write(",\"env\":");
    dumpref(out, &cl.env.gch);
    if (cl.isC != 0) {
        if (cl.debugname) |s| out.print(",\"name\":\"{s}\"", .{std.mem.sliceTo(s.slice(), 0)});
    } else {
        const p = cl.p.?;
        if (p.debugname) |s| out.print(",\"name\":\"{s}\"", .{std.mem.sliceTo(s.slice(), 0)});
        out.write(",\"proto\":");
        dumpref(out, &p.gch);
    }
    if (cl.nupvalues > 0) {
        out.write(",\"upvalues\":[");
        dumprefs(out, cl.upvals(), cl.nupvalues);
        out.write("]");
    }
    out.write("}");
}

fn dumpudata(out: *Out, u: *o.Udata) void {
    out.print("{{\"type\":\"userdata\",\"cat\":{d},\"size\":{d},\"tag\":{d}", .{ u.gch.memcat, gc.udataSize(@intCast(u.len)), u.tag });
    if (u.metatable) |mt| {
        out.write(",\"metatable\":");
        dumpref(out, &mt.gch);
    }
    out.write("}");
}

fn dumpthread(out: *Out, th: *State) void {
    const size = @sizeOf(State) + @sizeOf(TValue) * @as(usize, @intCast(th.stacksize)) + @sizeOf(o.CallInfo) * @as(usize, @intCast(th.size_ci));
    out.print("{{\"type\":\"thread\",\"cat\":{d},\"size\":{d}", .{ th.gch.memcat, size });
    out.write(",\"env\":");
    dumpref(out, &th.gt.gch);
    if (th.finalizers) |f| {
        out.write(",\"finalizers\":");
        dumpref(out, &f.gch);
    }
    var cip: ?*o.Proto = null;
    var ci: [*]o.CallInfo = th.base_ci;
    while (@intFromPtr(ci) <= @intFromPtr(th.ci)) : (ci += 1) {
        if (o.ttisfunction(&ci[0].func[0])) {
            cip = ci[0].p;
            break;
        }
    }
    if (cip) |p| {
        if (p.source) |src| {
            out.write(",\"source\":\"");
            dumpstringdata(out, src.slice());
            out.print("\",\"line\":{d}", .{p.linedefined});
        }
    }
    const used = o.sdiff(th.top, th.stack);
    if (used > 0) {
        out.write(",\"stack\":[");
        dumprefs(out, th.stack, @intCast(used));
        out.write("]");
        out.write(",\"stacknames\":[");
        var first = true;
        var c: [*]o.CallInfo = th.base_ci;
        var v = th.stack;
        while (@intFromPtr(v) < @intFromPtr(th.top)) : (v += 1) {
            if (!o.iscollectable(&v[0])) continue;
            while (@intFromPtr(c) < @intFromPtr(th.ci) and @intFromPtr(v) >= @intFromPtr((c + 1)[0].func)) c += 1;
            if (!first) out.write(",");
            first = false;
            if (v == c[0].func) {
                const cl = o.ci_func(&c[0]);
                if (cl.isC != 0) {
                    out.print("\"frame:{s}\"", .{if (cl.debugname) |d| std.mem.sliceTo(d.slice(), 0) else "[C]"});
                } else {
                    const p = cl.p.?;
                    out.write("\"frame:");
                    if (p.source) |src| dumpstringdata(out, src.slice());
                    out.print(":{d}:{s}\"", .{ p.linedefined, if (p.debugname) |d| std.mem.sliceTo(d.slice(), 0) else "" });
                }
            } else if (o.isLua(&c[0])) {
                const p = c[0].p.?;
                const pc = debug.pcRel(c[0].savedpc, p);
                const reg = o.idiff(v, c[0].base);
                if (findlocal(p, reg, pc)) |name| out.print("\"{s}\"", .{name}) else out.write("null");
            } else {
                out.write("null");
            }
        }
        out.write("]");
    }
    out.write("}");
}

fn findlocal(p: *o.Proto, reg: c_int, pc: c_int) ?[]const u8 {
    const lv = p.locvars orelse return null;
    for (0..@intCast(p.sizelocvars)) |i| {
        if (lv[i].reg == reg and pc >= lv[i].startpc and pc < lv[i].endpc) {
            if (lv[i].varname) |n| return std.mem.sliceTo(n.slice(), 0);
        }
    }
    return null;
}

fn dumpproto(out: *Out, p: *o.Proto) void {
    const size = @sizeOf(o.Proto) + @sizeOf(u32) * @as(usize, @intCast(p.sizecode)) + @sizeOf(*o.Proto) * @as(usize, @intCast(p.sizep)) +
        @sizeOf(TValue) * @as(usize, @intCast(p.sizek)) + @as(usize, @intCast(p.sizelineinfo)) + @sizeOf(o.LocVar) * @as(usize, @intCast(p.sizelocvars)) +
        @sizeOf(?*o.TString) * @as(usize, @intCast(p.sizeupvalues));
    out.print("{{\"type\":\"proto\",\"cat\":{d},\"size\":{d}", .{ p.gch.memcat, size });
    if (p.source) |src| {
        out.write(",\"source\":\"");
        dumpstringdata(out, src.slice());
        out.print("\",\"line\":{d}", .{if (p.abslineinfo) |a| a[0] else 0});
    }
    if (p.sizek > 0) {
        out.write(",\"constants\":[");
        dumprefs(out, p.k, @intCast(p.sizek));
        out.write("]");
    }
    if (p.sizep > 0) {
        out.write(",\"protos\":[");
        for (0..@intCast(p.sizep)) |i| {
            if (i != 0) out.write(",");
            dumpref(out, &p.p[i].gch);
        }
        out.write("]");
    }
    out.write("}");
}

fn dumpupval(out: *Out, uv: *o.UpVal) void {
    out.print("{{\"type\":\"upvalue\",\"cat\":{d},\"size\":{d},\"open\":{s}", .{ uv.gch.memcat, @sizeOf(o.UpVal), if (uv.isOpen()) "true" else "false" });
    if (o.iscollectable(uv.v)) {
        out.write(",\"object\":");
        dumpref(out, uv.v.value.gc);
    }
    out.write("}");
}

fn dumpclass(out: *Out, c: *o.LuauClass) void {
    out.print("{{\"type\":\"class\",\"cat\":{d},\"size\":{d}", .{ c.gch.memcat, @sizeOf(o.LuauClass) });
    out.write(",\"name\":");
    dumpstringdata(out, c.name.slice());
    out.write(",\"super\":");
    if (c.super) |s| dumpref(out, &s.gch) else out.write("null");
    out.write(",\"membernames\":[");
    var dumped = false;
    if (c.offsettomember) |otm| {
        for (0..c.offsettomemberlength()) |i| {
            const n = otm[i] orelse continue;
            if (dumped) out.write(",");
            dumpref(out, &n.gch);
            dumped = true;
        }
    }
    out.write("],\"staticmembers\":[");
    if (c.staticmembers) |sm| dumprefs(out, sm, c.numstaticmembers());
    out.write("],\"metatable\":");
    if (c.metatable) |mt| dumpref(out, &mt.gch) else out.write("null");
    out.write(",\"instancemetatable\":");
    if (c.instancemetatable) |mt| dumpref(out, &mt.gch) else out.write("null");
    out.write(",\"memberstooffset\":");
    dumpref(out, &c.memberstooffset.?.gch);
    out.write("}");
}

fn dumpobject(out: *Out, inst: *o.LuauObject) void {
    out.print("{{\"type\":\"object\",\"cat\":{d},\"size\":{d}", .{ inst.gch.memcat, @sizeOf(o.LuauObject) });
    out.write(",\"class\":");
    dumpref(out, &inst.lclass.gch);
    out.write(",\"members\":[");
    if (inst.members) |m| dumprefs(out, m, inst.numberofmembers);
    out.write("]}");
}

fn dumpobj(out: *Out, obj: *GCObject) void {
    switch (obj.tt) {
        o.T_STRING => dumpstring(out, @ptrCast(obj)),
        o.T_TABLE => dumptable(out, @ptrCast(@alignCast(obj))),
        o.T_FUNCTION => dumpclosure(out, @ptrCast(@alignCast(obj))),
        o.T_USERDATA => dumpudata(out, @ptrCast(@alignCast(obj))),
        o.T_THREAD => dumpthread(out, @ptrCast(@alignCast(obj))),
        o.T_BUFFER => {
            const b: *o.Buffer = @ptrCast(@alignCast(obj));
            out.print("{{\"type\":\"buffer\",\"cat\":{d},\"size\":{d}}}", .{ b.gch.memcat, o.Buffer.header + b.len });
        },
        o.T_PROTO => dumpproto(out, @ptrCast(@alignCast(obj))),
        o.T_UPVAL => dumpupval(out, @ptrCast(@alignCast(obj))),
        o.T_CLASS => dumpclass(out, @ptrCast(@alignCast(obj))),
        o.T_OBJECT => dumpobject(out, @ptrCast(@alignCast(obj))),
        else => unreachable,
    }
}

fn dumpgco(out: *Out, obj: *GCObject) void {
    dumpref(out, obj);
    out.write(":");
    dumpobj(out, obj);
    out.write(",\n");
}

pub fn dump(L: *State, file: ?*anyopaque, categoryName: ?CategoryName) void {
    if (builtin.link_libc) {
        dumpImpl(L, file, categoryName);
    } else {
        debug.runerror(L, "lua_memorydump requires the libc-linked C-ABI build: a C FILE* can only be written by its C runtime", .{});
    }
}

fn dumpImpl(L: *State, file: ?*anyopaque, categoryName: ?CategoryName) void {
    const g = L.global;
    var out: Out = .{ .file = file orelse debug.runerror(L, "lua_memorydump: file is null", .{}) };
    out.write("{\"objects\":{\n");
    dumpgco(&out, &g.mainthread.gch);
    var it = g.allgc;
    while (it) |obj| : (it = obj.next) dumpgco(&out, obj);
    out.write("\"0\":{\"type\":\"userdata\",\"cat\":0,\"size\":0}\n");
    out.write("},\"roots\":{\n");
    out.write("\"mainthread\":");
    dumpref(&out, &g.mainthread.gch);
    out.write(",\"registry\":");
    dumpref(&out, g.registry.value.gc);
    out.write("},\"stats\":{\n");
    out.print("\"size\":{d},\n", .{g.totalbytes});
    out.write("\"categories\":{\n");
    for (0..o.LUA_MEMORY_CATEGORIES) |i| {
        const bytes = g.memcatbytes[i];
        if (bytes != 0) {
            const name: ?[*:0]const u8 = if (categoryName) |cn| cn(L, @intCast(i)) else null;
            if (categoryName != null) {
                out.print("\"{d}\":{{\"name\":\"{s}\", \"size\":{d}}},\n", .{ i, if (name) |n| std.mem.span(n) else "(null)", bytes });
            } else {
                out.print("\"{d}\":{{\"size\":{d}}},\n", .{ i, bytes });
            }
        }
    }
    out.write("\"none\":{}\n");
    out.write("}\n");
    out.write("}}\n");
    out.flush();
}
