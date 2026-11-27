//! Bytecode builder and serializer.
//!
//! Translated from third_party/luau/Bytecode/src/BytecodeBuilder.cpp and
//! Bytecode/include/Luau/BytecodeBuilder.h (Luau, MIT License; see
//! third_party/luau/LICENSE.txt). Feature flags that affect serialization
//! (LuauEmitCallFeedback, LuauCompileRefactorFeedback, LuauCompileCallFbRemap,
//! LuauCompileFastpcall, DebugLuauUserDefinedClasses) are taken from `flags`
//! and default to off, which yields LBC_VERSION_TARGET blobs with an empty
//! feedback vector exactly like the pinned upstream build.
//! Debug dumping/remarks and runtime validation are not part of this port.

const std = @import("std");
const bc = @import("bytecode.zig");
const flags_mod = @import("flags.zig");
const Opcode = bc.Opcode;
const Allocator = std.mem.Allocator;

pub const LFT_CALLTARGET: u8 = 0;
pub const LFT_CONSTRUCT: u8 = 1;

/// Feedback vector entry; `value` is the call pc for LFT_CALLTARGET or the class shape constant for LFT_CONSTRUCT
const FeedbackSlot = struct { kind: u8, value: u32 };

pub const kMaxRegisterCount: u32 = 255;
pub const kMaxUpvalueCount: u32 = 200;
pub const kMaxLocalCount: u32 = 200;
pub const kMaxInstructionCount: u32 = 1_000_000_000;
pub const kInvalidReg: u8 = 255;
pub const kMaxConstantCount: u32 = 1 << 23;
pub const kMaxClosureCount: u32 = 1 << 15;
pub const kMaxJumpDistance: i32 = 1 << 23;

pub const TableShape = struct {
    pub const kMaxLength: u32 = 32;

    keys: [kMaxLength]i32 = undefined,
    constants: [kMaxLength]i32 = undefined,
    length: u32 = 0,
    has_constants: bool = false,
};

const TableShapeContext = struct {
    pub fn hash(_: TableShapeContext, v: TableShape) u64 {
        // FNV-1a inspired hash (note that we feed integers instead of bytes)
        var h: u32 = 2166136261;
        for (0..v.length) |i| {
            h ^= @bitCast(v.keys[i]);
            h *%= 16777619;
            if (v.has_constants) {
                h ^= @bitCast(v.constants[i]);
                h *%= 16777619;
            }
        }
        return h;
    }
    pub fn eql(_: TableShapeContext, a: TableShape, b: TableShape) bool {
        if (a.length != b.length or a.has_constants != b.has_constants) return false;
        if (!std.mem.eql(i32, a.keys[0..a.length], b.keys[0..b.length])) return false;
        if (a.has_constants and !std.mem.eql(i32, a.constants[0..a.length], b.constants[0..b.length])) return false;
        return true;
    }
};

const ConstantType = enum(u8) { nil, boolean, number, integer, vectorf, vectord, string, import, table, closure, class_shape };

const Constant = union(ConstantType) {
    nil,
    boolean: bool,
    number: f64,
    integer: i64,
    vectorf: [4]f32,
    vectord: [4]f64,
    string: u32,
    import: u32,
    table: u32,
    closure: u32,
    /// index into class_shapes
    class_shape: u32,
};

/// DebugLuauUserDefinedClasses class shape constant (LBC_CONSTANT_CLASS_SHAPE)
pub const ClassShape = struct {
    class_name: i32,
    num_private_instance_props: u8 = 0,
    num_private_static_props: u8 = 0,
    /// We only keep names for public properties. Private fields are indexed by their statically determined offsets
    property_names: []const i32 = &.{},
    method_names: []const i32 = &.{},
};

const ConstantKey = struct {
    type: ConstantType,
    value: u64,
    extra1: u64 = 0,
    extra2: u64 = 0,
    extra3: u64 = 0,
};

const Jump = struct { source: u32, target: u32 };

const DebugLocal = struct { name: u32, reg: u8, startpc: u32, endpc: u32 };
const TypedLocal = struct { type: u8, reg: u8, startpc: u32, endpc: u32 };

const Function = struct {
    data: []u8 = &.{},
    maxstacksize: u8 = 0,
    numparams: u8 = 0,
    numupvalues: u8 = 0,
    isvararg: bool = false,
    debugname: u32 = 0,
    debuglinedefined: i32 = 0,
    typeinfo: []const u8 = &.{},
};

pub const UserdataType = struct {
    name: []const u8,
    name_ref: u32 = 0,
    used: bool = false,
};

pub const StringRef = []const u8;

pub const BytecodeBuilder = struct {
    gpa: Allocator,
    flags: flags_mod.Flags = .{},

    functions: std.ArrayList(Function) = .empty,
    current_function: u32 = ~@as(u32, 0),
    main_function: u32 = ~@as(u32, 0),

    total_instruction_count: usize = 0,
    insns: std.ArrayList(u32) = .empty,
    lines: std.ArrayList(i32) = .empty,
    constants: std.ArrayList(Constant) = .empty,
    protos: std.ArrayList(u32) = .empty,
    jumps: std.ArrayList(Jump) = .empty,

    table_shapes: std.ArrayList(TableShape) = .empty,
    class_shapes: std.ArrayList(ClassShape) = .empty,

    fb_slots: std.ArrayList(FeedbackSlot) = .empty,

    has_long_jumps: bool = false,

    constant_map: std.AutoHashMapUnmanaged(ConstantKey, i32) = .empty,
    table_shape_map: std.HashMapUnmanaged(TableShape, i32, TableShapeContext, 80) = .empty,
    proto_map: std.AutoHashMapUnmanaged(u32, i16) = .empty,

    debug_line: i32 = 0,

    debug_locals: std.ArrayList(DebugLocal) = .empty,
    debug_upvals: std.ArrayList(u32) = .empty,

    typed_locals: std.ArrayList(TypedLocal) = .empty,
    typed_upvals: std.ArrayList(u8) = .empty,

    /// string -> 1-based index; strings are kept in insertion order in `strings`.
    string_table: std.StringHashMapUnmanaged(u32) = .empty,
    strings: std.ArrayList([]const u8) = .empty,

    userdata_types: std.ArrayList(UserdataType) = .empty,

    temp_type_info: std.ArrayList(u8) = .empty,

    bytecode: std.ArrayList(u8) = .empty,

    pub fn init(gpa: Allocator) BytecodeBuilder {
        return .{ .gpa = gpa };
    }

    pub fn beginFunction(self: *BytecodeBuilder, numparams: u8, isvararg: bool) !u32 {
        std.debug.assert(self.current_function == ~@as(u32, 0));
        const id: u32 = @intCast(self.functions.items.len);
        try self.functions.append(self.gpa, .{ .numparams = numparams, .isvararg = isvararg });
        self.current_function = id;
        self.has_long_jumps = false;
        self.debug_line = 0;
        return id;
    }

    fn clearState(self: *BytecodeBuilder) void {
        self.insns.clearRetainingCapacity();
        self.lines.clearRetainingCapacity();
        self.constants.clearRetainingCapacity();
        self.protos.clearRetainingCapacity();
        self.jumps.clearRetainingCapacity();
        self.table_shapes.clearRetainingCapacity();
        self.debug_locals.clearRetainingCapacity();
        self.debug_upvals.clearRetainingCapacity();
        self.typed_locals.clearRetainingCapacity();
        self.typed_upvals.clearRetainingCapacity();
        self.constant_map.clearRetainingCapacity();
        self.fb_slots.clearRetainingCapacity();
        self.table_shape_map.clearRetainingCapacity();
        self.proto_map.clearRetainingCapacity();
    }

    pub fn endFunction(self: *BytecodeBuilder, maxstacksize: u8, numupvalues: u8, flags: u8, cost: u64) !void {
        std.debug.assert(self.current_function != ~@as(u32, 0));
        const func = &self.functions.items[self.current_function];
        func.maxstacksize = maxstacksize;
        func.numupvalues = numupvalues;

        var data: std.ArrayList(u8) = .empty;
        try data.ensureTotalCapacity(self.gpa, 32 + self.insns.items.len * 7);
        try self.writeFunction(&data, self.current_function, flags, cost);
        self.functions.items[self.current_function].data = try data.toOwnedSlice(self.gpa);

        self.current_function = ~@as(u32, 0);
        self.total_instruction_count += self.insns.items.len;
        self.clearState();
    }

    pub fn setMainFunction(self: *BytecodeBuilder, fid: u32) void {
        std.debug.assert(fid < self.functions.items.len);
        self.main_function = fid;
    }

    fn addConstant(self: *BytecodeBuilder, key: ConstantKey, value: Constant) !i32 {
        if (self.constant_map.get(key)) |cached| return cached;
        const id: u32 = @intCast(self.constants.items.len);
        if (id >= kMaxConstantCount) return -1;
        try self.constant_map.put(self.gpa, key, @intCast(id));
        try self.constants.append(self.gpa, value);
        return @intCast(id);
    }

    pub fn addStringTableEntry(self: *BytecodeBuilder, value: []const u8) !u32 {
        const gop = try self.string_table.getOrPut(self.gpa, value);
        // note: bytecode serialization format uses 1-based table indices, 0 is reserved to mean nil
        if (!gop.found_existing) {
            try self.strings.append(self.gpa, value);
            gop.value_ptr.* = @intCast(self.strings.items.len);
        }
        return gop.value_ptr.*;
    }

    pub fn addConstantNil(self: *BytecodeBuilder) !i32 {
        return self.addConstant(.{ .type = .nil, .value = 0 }, .nil);
    }

    pub fn addConstantBoolean(self: *BytecodeBuilder, value: bool) !i32 {
        return self.addConstant(.{ .type = .boolean, .value = @intFromBool(value) }, .{ .boolean = value });
    }

    pub fn addConstantNumber(self: *BytecodeBuilder, value: f64) !i32 {
        return self.addConstant(.{ .type = .number, .value = @bitCast(value) }, .{ .number = value });
    }

    pub fn addConstantInteger(self: *BytecodeBuilder, value: i64) !i32 {
        return self.addConstant(.{ .type = .integer, .value = @bitCast(value) }, .{ .integer = value });
    }

    pub fn addConstantVectorf(self: *BytecodeBuilder, x: f32, y: f32, z: f32, w: f32) !i32 {
        const xb: u64 = @as(u32, @bitCast(x));
        const yb: u64 = @as(u32, @bitCast(y));
        const zb: u64 = @as(u32, @bitCast(z));
        const wb: u64 = @as(u32, @bitCast(w));
        return self.addConstant(.{ .type = .vectorf, .value = xb | (yb << 32), .extra1 = zb | (wb << 32) }, .{ .vectorf = .{ x, y, z, w } });
    }

    pub fn addConstantVectord(self: *BytecodeBuilder, x: f64, y: f64, z: f64, w: f64) !i32 {
        return self.addConstant(.{ .type = .vectord, .value = @bitCast(x), .extra1 = @bitCast(y), .extra2 = @bitCast(z), .extra3 = @bitCast(w) }, .{ .vectord = .{ x, y, z, w } });
    }

    pub fn addConstantString(self: *BytecodeBuilder, value: []const u8) !i32 {
        const index = try self.addStringTableEntry(value);
        return self.addConstant(.{ .type = .string, .value = index }, .{ .string = index });
    }

    pub fn addImport(self: *BytecodeBuilder, iid: u32) !i32 {
        return self.addConstant(.{ .type = .import, .value = iid }, .{ .import = iid });
    }

    pub fn addConstantTable(self: *BytecodeBuilder, shape: TableShape) !i32 {
        if (self.table_shape_map.get(shape)) |cached| return cached;
        const id: u32 = @intCast(self.constants.items.len);
        if (id >= kMaxConstantCount) return -1;
        try self.table_shape_map.put(self.gpa, shape, @intCast(id));
        try self.table_shapes.append(self.gpa, shape);
        try self.constants.append(self.gpa, .{ .table = @intCast(self.table_shapes.items.len - 1) });
        return @intCast(id);
    }

    pub fn addConstantClosure(self: *BytecodeBuilder, fid: u32) !i32 {
        return self.addConstant(.{ .type = .closure, .value = fid }, .{ .closure = fid });
    }

    pub fn addClassShape(self: *BytecodeBuilder, shape: ClassShape) !i32 {
        const id: u32 = @intCast(self.constants.items.len);
        if (id >= kMaxConstantCount) return -1;
        // note: like upstream, class shapes are not deduplicated and the shape list is never cleared between functions
        try self.class_shapes.append(self.gpa, shape);
        try self.constants.append(self.gpa, .{ .class_shape = @intCast(self.class_shapes.items.len - 1) });
        return @intCast(id);
    }

    pub fn addChildFunction(self: *BytecodeBuilder, fid: u32) !i16 {
        if (self.proto_map.get(fid)) |cached| return cached;
        const id: u32 = @intCast(self.protos.items.len);
        if (id >= kMaxClosureCount) return -1;
        try self.proto_map.put(self.gpa, fid, @intCast(id));
        try self.protos.append(self.gpa, fid);
        return @intCast(id);
    }

    pub fn emitABC(self: *BytecodeBuilder, op: Opcode, a: u8, b: u8, c: u8) !void {
        const insn: u32 = @as(u32, @intFromEnum(op)) | (@as(u32, a) << 8) | (@as(u32, b) << 16) | (@as(u32, c) << 24);
        try self.insns.append(self.gpa, insn);
        try self.lines.append(self.gpa, self.debug_line);
    }

    pub fn emitAD(self: *BytecodeBuilder, op: Opcode, a: u8, d: i16) !void {
        const insn: u32 = @as(u32, @intFromEnum(op)) | (@as(u32, a) << 8) | (@as(u32, @as(u16, @bitCast(d))) << 16);
        try self.insns.append(self.gpa, insn);
        try self.lines.append(self.gpa, self.debug_line);
    }

    pub fn emitE(self: *BytecodeBuilder, op: Opcode, e: i32) !void {
        const insn: u32 = @as(u32, @intFromEnum(op)) | (@as(u32, @bitCast(e)) << 8);
        try self.insns.append(self.gpa, insn);
        try self.lines.append(self.gpa, self.debug_line);
    }

    pub fn emitAux(self: *BytecodeBuilder, aux: u32) !void {
        try self.insns.append(self.gpa, aux);
        try self.lines.append(self.gpa, self.debug_line);
    }

    pub fn undoEmit(self: *BytecodeBuilder, op: Opcode) void {
        std.debug.assert(self.insns.items.len > 0);
        std.debug.assert(bc.insnOp(self.insns.items[self.insns.items.len - 1]) == @intFromEnum(op));
        const size: u32 = @intCast(self.insns.items.len);

        // Adjust local ranges referencing this instruction
        var i: usize = 0;
        while (i < self.debug_locals.items.len) {
            const l = &self.debug_locals.items[i];
            // If live range start has been removed, the local never existed
            if (l.startpc == size) {
                _ = self.debug_locals.orderedRemove(i);
                continue;
            }
            if (l.endpc == size) l.endpc -= 1;
            i += 1;
        }

        i = 0;
        while (i < self.typed_locals.items.len) {
            const l = &self.typed_locals.items[i];
            if (l.startpc == size) {
                _ = self.typed_locals.orderedRemove(i);
                continue;
            }
            if (l.endpc == size) l.endpc -= 1;
            i += 1;
        }

        _ = self.insns.pop();
        _ = self.lines.pop();
    }

    pub fn lastInstruction(self: *const BytecodeBuilder) u32 {
        return self.insns.items[self.insns.items.len - 1];
    }

    pub fn emitLabel(self: *const BytecodeBuilder) usize {
        return self.insns.items.len;
    }

    pub fn patchJumpD(self: *BytecodeBuilder, jump_label: usize, target_label: usize) !bool {
        std.debug.assert(jump_label < self.insns.items.len);
        std.debug.assert(bc.isJumpD(@enumFromInt(bc.insnOp(self.insns.items[jump_label]))));
        std.debug.assert(bc.insnD(self.insns.items[jump_label]) == 0);
        std.debug.assert(target_label <= self.insns.items.len);

        const offset: i64 = @as(i64, @intCast(target_label)) - @as(i64, @intCast(jump_label)) - 1;

        if (offset >= std.math.minInt(i16) and offset <= std.math.maxInt(i16)) {
            self.insns.items[jump_label] |= @as(u32, @as(u16, @bitCast(@as(i16, @intCast(offset))))) << 16;
        } else if (@abs(offset) < kMaxJumpDistance) {
            // our jump doesn't fit into 16 bits; we will need to repatch the bytecode sequence with jump trampolines, see expandJumps
            self.has_long_jumps = true;
        } else {
            return false;
        }

        try self.jumps.append(self.gpa, .{ .source = @intCast(jump_label), .target = @intCast(target_label) });
        return true;
    }

    pub fn patchSkipC(self: *BytecodeBuilder, jump_label: usize, target_label: usize) bool {
        std.debug.assert(jump_label < self.insns.items.len);
        std.debug.assert(bc.insnC(self.insns.items[jump_label]) == 0);
        const offset: i64 = @as(i64, @intCast(target_label)) - @as(i64, @intCast(jump_label)) - 1;
        if (offset < 0 or offset > 255) return false;
        self.insns.items[jump_label] |= @as(u32, @intCast(offset)) << 24;
        return true;
    }

    pub fn patchAux(self: *BytecodeBuilder, target_aux: usize, new_value: u32) void {
        self.insns.items[target_aux] = new_value;
    }

    pub fn setFunctionTypeInfo(self: *BytecodeBuilder, value: []const u8) void {
        self.functions.items[self.current_function].typeinfo = value;
    }

    pub fn pushLocalTypeInfo(self: *BytecodeBuilder, ty: u8, reg: u8, startpc: u32, endpc: u32) !void {
        try self.typed_locals.append(self.gpa, .{ .type = ty, .reg = reg, .startpc = startpc, .endpc = endpc });
    }

    pub fn pushUpvalTypeInfo(self: *BytecodeBuilder, ty: u8) !void {
        try self.typed_upvals.append(self.gpa, ty);
    }

    pub fn addUserdataType(self: *BytecodeBuilder, name: []const u8) !u32 {
        try self.userdata_types.append(self.gpa, .{ .name = name });
        return @intCast(self.userdata_types.items.len - 1);
    }

    pub fn useUserdataType(self: *BytecodeBuilder, index: u32) void {
        self.userdata_types.items[index].used = true;
    }

    pub fn setDebugFunctionName(self: *BytecodeBuilder, name: []const u8) !void {
        const index = try self.addStringTableEntry(name);
        self.functions.items[self.current_function].debugname = index;
    }

    pub fn setDebugFunctionLineDefined(self: *BytecodeBuilder, line: i32) void {
        self.functions.items[self.current_function].debuglinedefined = line;
    }

    pub fn setDebugLine(self: *BytecodeBuilder, line: i32) void {
        self.debug_line = line;
    }

    pub fn pushDebugLocal(self: *BytecodeBuilder, name: []const u8, reg: u8, startpc: u32, endpc: u32) !void {
        const index = try self.addStringTableEntry(name);
        try self.debug_locals.append(self.gpa, .{ .name = index, .reg = reg, .startpc = startpc, .endpc = endpc });
    }

    pub fn pushDebugUpval(self: *BytecodeBuilder, name: []const u8) !void {
        const index = try self.addStringTableEntry(name);
        try self.debug_upvals.append(self.gpa, index);
    }

    pub fn getInstructionCount(self: *const BytecodeBuilder) usize {
        return self.insns.items.len;
    }

    pub fn getTotalInstructionCount(self: *const BytecodeBuilder) usize {
        return self.total_instruction_count;
    }

    pub fn getDebugPC(self: *const BytecodeBuilder) u32 {
        return @intCast(self.insns.items.len);
    }

    pub fn getVersion(self: *const BytecodeBuilder) u8 {
        if (self.flags.DebugLuauUserDefinedClasses) return bc.LBC_VERSION_CLASSES;

        if (self.flags.LuauCompileFastpcall) return 14;

        return bc.LBC_VERSION_TARGET;
    }

    /// Adds an LFT_CALLTARGET feedback slot for the call instruction at `pc`
    pub fn addCallTargetSlot(self: *BytecodeBuilder, pc: u32) !u32 {
        try self.fb_slots.append(self.gpa, .{ .kind = LFT_CALLTARGET, .value = pc });
        return @intCast(self.fb_slots.items.len - 1);
    }

    /// Adds an LFT_CONSTRUCT feedback slot for the class shape constant `shapecid`
    pub fn addConstructSlot(self: *BytecodeBuilder, shapecid: u32) !u32 {
        try self.fb_slots.append(self.gpa, .{ .kind = LFT_CONSTRUCT, .value = shapecid });
        return @intCast(self.fb_slots.items.len - 1);
    }

    pub fn getTypeEncodingVersion() u8 {
        return bc.LBC_TYPE_VERSION_TARGET;
    }

    pub fn finalize(self: *BytecodeBuilder) !void {
        std.debug.assert(self.bytecode.items.len == 0);

        for (self.userdata_types.items) |*ty| {
            if (ty.used) ty.name_ref = try self.addStringTableEntry(ty.name);
        }

        var capacity: usize = 16;
        for (self.strings.items) |s| capacity += s.len + 2;
        for (self.functions.items) |f| capacity += f.data.len;
        try self.bytecode.ensureTotalCapacity(self.gpa, capacity);

        const ss = &self.bytecode;
        try writeByte(self.gpa, ss, self.getVersion());
        try writeByte(self.gpa, ss, getTypeEncodingVersion());

        try self.writeStringTable(ss);

        // Write the mapping between used type name indices and their name
        for (self.userdata_types.items, 0..) |ty, i| {
            if (ty.used) {
                try writeByte(self.gpa, ss, @intCast(i + 1));
                try writeVarInt(self.gpa, ss, ty.name_ref);
            }
        }
        // 0 marks the end of the mapping
        try writeByte(self.gpa, ss, 0);

        try writeVarInt(self.gpa, ss, self.functions.items.len);
        for (self.functions.items) |f| {
            try writeVarInt(self.gpa, ss, f.data.len);
            try ss.appendSlice(self.gpa, f.data);
        }

        std.debug.assert(self.main_function < self.functions.items.len);
        try writeVarInt(self.gpa, ss, self.main_function);
    }

    pub fn getBytecode(self: *const BytecodeBuilder) []const u8 {
        return self.bytecode.items;
    }

    fn writeFunction(self: *BytecodeBuilder, ss: *std.ArrayList(u8), id: u32, flags: u8, cost: u64) !void {
        const gpa = self.gpa;
        const func = self.functions.items[id];

        // header
        try writeByte(gpa, ss, func.maxstacksize);
        try writeByte(gpa, ss, func.numparams);
        try writeByte(gpa, ss, func.numupvalues);
        try writeByte(gpa, ss, @intFromBool(func.isvararg));

        try writeByte(gpa, ss, flags);

        if (func.typeinfo.len != 0 or self.typed_upvals.items.len != 0 or self.typed_locals.items.len != 0) {
            // collect type info into a temporary string to know the overall size of type data
            const t = &self.temp_type_info;
            t.clearRetainingCapacity();
            try writeVarInt(gpa, t, func.typeinfo.len);
            try writeVarInt(gpa, t, self.typed_upvals.items.len);
            try writeVarInt(gpa, t, self.typed_locals.items.len);
            try t.appendSlice(gpa, func.typeinfo);
            for (self.typed_upvals.items) |ty| try writeByte(gpa, t, ty);
            for (self.typed_locals.items) |l| {
                try writeByte(gpa, t, l.type);
                try writeByte(gpa, t, l.reg);
                try writeVarInt(gpa, t, l.startpc);
                std.debug.assert(l.endpc >= l.startpc);
                try writeVarInt(gpa, t, l.endpc - l.startpc);
            }
            try writeVarInt(gpa, ss, t.items.len);
            try ss.appendSlice(gpa, t.items);
        } else {
            try writeVarInt(gpa, ss, 0);
        }

        // instructions
        try writeVarInt(gpa, ss, self.insns.items.len);
        for (self.insns.items) |insn| try writeU32(gpa, ss, insn);

        // constants
        try writeVarInt(gpa, ss, self.constants.items.len);
        for (self.constants.items) |c| {
            switch (c) {
                .nil => try writeByte(gpa, ss, bc.LBC_CONSTANT_NIL),
                .boolean => |v| {
                    try writeByte(gpa, ss, bc.LBC_CONSTANT_BOOLEAN);
                    try writeByte(gpa, ss, @intFromBool(v));
                },
                .number => |v| {
                    try writeByte(gpa, ss, bc.LBC_CONSTANT_NUMBER);
                    try writeU64(gpa, ss, @bitCast(v));
                },
                .integer => |v| {
                    try writeByte(gpa, ss, bc.LBC_CONSTANT_INTEGER);
                    if (v < 0) {
                        try writeByte(gpa, ss, 1);
                        try writeVarInt(gpa, ss, ~@as(u64, @bitCast(v)) +% 1);
                    } else {
                        try writeByte(gpa, ss, 0);
                        try writeVarInt(gpa, ss, @as(u64, @bitCast(v)));
                    }
                },
                .vectorf => |v| {
                    try writeByte(gpa, ss, bc.LBC_CONSTANT_VECTOR);
                    for (v) |x| try writeU32(gpa, ss, @bitCast(x));
                },
                .vectord => |v| {
                    try writeByte(gpa, ss, bc.LBC_CONSTANT_VECTORD);
                    for (v) |x| try writeU64(gpa, ss, @bitCast(x));
                },
                .string => |v| {
                    try writeByte(gpa, ss, bc.LBC_CONSTANT_STRING);
                    try writeVarInt(gpa, ss, v);
                },
                .import => |v| {
                    try writeByte(gpa, ss, bc.LBC_CONSTANT_IMPORT);
                    try writeU32(gpa, ss, v);
                },
                .table => |ti| {
                    const shape = self.table_shapes.items[ti];
                    if (shape.has_constants) {
                        try writeByte(gpa, ss, bc.LBC_CONSTANT_TABLE_WITH_CONSTANTS);
                        try writeVarInt(gpa, ss, shape.length);
                        for (0..shape.length) |i| {
                            try writeVarInt(gpa, ss, @as(u32, @bitCast(shape.keys[i])));
                            try writeU32(gpa, ss, @bitCast(shape.constants[i]));
                        }
                    } else {
                        try writeByte(gpa, ss, bc.LBC_CONSTANT_TABLE);
                        try writeVarInt(gpa, ss, shape.length);
                        for (0..shape.length) |i| try writeVarInt(gpa, ss, @as(u32, @bitCast(shape.keys[i])));
                    }
                },
                .class_shape => |v| {
                    const cs = self.class_shapes.items[v];
                    try writeByte(gpa, ss, bc.LBC_CONSTANT_CLASS_SHAPE);
                    try writeVarInt(gpa, ss, @as(u32, @bitCast(cs.class_name)));
                    try writeByte(gpa, ss, cs.num_private_instance_props);
                    try writeByte(gpa, ss, cs.num_private_static_props);
                    try writeVarInt(gpa, ss, cs.property_names.len + cs.num_private_instance_props);
                    try writeVarInt(gpa, ss, cs.method_names.len + cs.num_private_static_props);
                    for (cs.property_names) |name| try writeVarInt(gpa, ss, @as(u32, @bitCast(name)));
                    for (cs.method_names) |name| try writeVarInt(gpa, ss, @as(u32, @bitCast(name)));
                },
                .closure => |v| {
                    try writeByte(gpa, ss, bc.LBC_CONSTANT_CLOSURE);
                    try writeVarInt(gpa, ss, v);
                },
            }
        }

        // child protos
        try writeVarInt(gpa, ss, self.protos.items.len);
        for (self.protos.items) |child| try writeVarInt(gpa, ss, child);

        // debug info
        try writeVarInt(gpa, ss, @as(u32, @bitCast(func.debuglinedefined)));
        try writeVarInt(gpa, ss, func.debugname);

        var has_lines = true;
        for (self.lines.items) |line| {
            if (line == 0) {
                has_lines = false;
                break;
            }
        }

        if (has_lines) {
            try writeByte(gpa, ss, 1);
            try self.writeLineInfo(ss);
        } else {
            try writeByte(gpa, ss, 0);
        }

        const has_debug = self.debug_locals.items.len != 0 or self.debug_upvals.items.len != 0;
        if (has_debug) {
            try writeByte(gpa, ss, 1);
            try writeVarInt(gpa, ss, self.debug_locals.items.len);
            for (self.debug_locals.items) |l| {
                try writeVarInt(gpa, ss, l.name);
                try writeVarInt(gpa, ss, l.startpc);
                try writeVarInt(gpa, ss, l.endpc);
                try writeByte(gpa, ss, l.reg);
            }
            try writeVarInt(gpa, ss, self.debug_upvals.items.len);
            for (self.debug_upvals.items) |name| try writeVarInt(gpa, ss, name);
        } else {
            try writeByte(gpa, ss, 0);
        }

        if (self.flags.LuauEmitCallFeedback or self.flags.DebugLuauUserDefinedClasses) {
            // Feedback Slots (identical encoding for the refactored and deprecated slot storage)
            try writeVarInt(gpa, ss, self.fb_slots.items.len);

            for (self.fb_slots.items) |slot| {
                try writeByte(gpa, ss, slot.kind);
                try writeVarInt(gpa, ss, slot.value);
            }
        } else {
            try writeVarInt(gpa, ss, 0); // Empty feedback vector
        }

        if ((flags & bc.LPF_INLINABLE) != 0) try writeVarInt(gpa, ss, cost);
    }

    fn calcLinesSpan(self: *const BytecodeBuilder) usize {
        const lines = self.lines.items;
        std.debug.assert(lines.len > 0);
        // this function encodes lines inside each span as a 8-bit delta to span baseline
        // span is always a power of two; depending on the line info input, it may need to be as low as 1
        var span: usize = 1 << 24;

        var offset: usize = 0;
        while (offset < lines.len) : (offset += span) {
            var next = offset;
            var min = lines[offset];
            var max = lines[offset];

            while (next < lines.len and next < offset + span) : (next += 1) {
                min = @min(min, lines[next]);
                max = @max(max, lines[next]);
                if (max - min > 255) break;
            }

            if (next < lines.len and next - offset < span) {
                // since not all lines in the range fit in 8b delta, we need to shrink the span
                // next iteration will need to reprocess some lines again since span changed
                span = @as(usize, 1) << @intCast(log2(@intCast(next - offset)));
            }
        }
        return span;
    }

    fn writeLineInfo(self: *BytecodeBuilder, ss: *std.ArrayList(u8)) !void {
        const gpa = self.gpa;
        const lines = self.lines.items;

        // first pass: determine span length
        const span = self.calcLinesSpan();

        // second pass: compute span base
        const baseline_size = (lines.len - 1) / span + 1;
        const baseline = try gpa.alloc(i32, baseline_size);
        defer gpa.free(baseline);

        var offset: usize = 0;
        while (offset < lines.len) : (offset += span) {
            var next = offset;
            var min = lines[offset];
            while (next < lines.len and next < offset + span) : (next += 1) min = @min(min, lines[next]);
            baseline[offset / span] = min;
        }

        // third pass: write resulting data
        const logspan: u5 = @intCast(log2(@intCast(span)));
        try writeByte(gpa, ss, logspan);

        var last_offset: u8 = 0;
        for (lines, 0..) |line, i| {
            const delta = line - baseline[i >> logspan];
            std.debug.assert(delta >= 0 and delta <= 255);
            try writeByte(gpa, ss, @as(u8, @intCast(delta)) -% last_offset);
            last_offset = @intCast(delta);
        }

        var last_line: i32 = 0;
        for (baseline) |b| {
            try writeU32(gpa, ss, @bitCast(b -% last_line));
            last_line = b;
        }
    }

    fn writeStringTable(self: *BytecodeBuilder, ss: *std.ArrayList(u8)) !void {
        try writeVarInt(self.gpa, ss, self.strings.items.len);
        for (self.strings.items) |s| {
            try writeVarInt(self.gpa, ss, s.len);
            try ss.appendSlice(self.gpa, s);
        }
    }

    pub fn getImportId1(id0: i32) u32 {
        std.debug.assert(@as(u32, @bitCast(id0)) < 1024);
        return (1 << 30) | (@as(u32, @bitCast(id0)) << 20);
    }

    pub fn getImportId2(id0: i32, id1: i32) u32 {
        std.debug.assert(@as(u32, @bitCast(id0 | id1)) < 1024);
        return (2 << 30) | (@as(u32, @bitCast(id0)) << 20) | (@as(u32, @bitCast(id1)) << 10);
    }

    pub fn getImportId3(id0: i32, id1: i32, id2: i32) u32 {
        std.debug.assert(@as(u32, @bitCast(id0 | id1 | id2)) < 1024);
        return (3 << 30) | (@as(u32, @bitCast(id0)) << 20) | (@as(u32, @bitCast(id1)) << 10) | @as(u32, @bitCast(id2));
    }

    /// This hashing algorithm should match luaS_hash defined in VM/lstring.cpp for short inputs.
    pub fn getStringHash(key: []const u8) u32 {
        var h: u32 = @truncate(key.len);
        // original Lua 5.1 hash for compatibility (exact match when len<32)
        var i = key.len;
        while (i > 0) : (i -= 1) {
            h ^= (h << 5) +% (h >> 2) +% key[i - 1];
        }
        return h;
    }

    pub fn foldJumps(self: *BytecodeBuilder) void {
        // if our function has long jumps, some processing below can make jump instructions not-jumps (e.g. JUMP->RETURN)
        // it's safer to skip this processing
        if (self.has_long_jumps) return;

        const insns = self.insns.items;
        for (self.jumps.items) |*jump| {
            const jump_label = jump.source;
            const jump_insn = insns[jump_label];

            // follow jump target through forward unconditional jumps
            // we only follow forward jumps to make sure the process terminates
            var target_label: u32 = @intCast(@as(i64, jump_label) + 1 + bc.insnD(jump_insn));
            std.debug.assert(target_label < insns.len);
            var target_insn = insns[target_label];

            while (bc.insnOp(target_insn) == @intFromEnum(Opcode.JUMP) and bc.insnD(target_insn) >= 0) {
                target_label = @intCast(@as(i64, target_label) + 1 + bc.insnD(target_insn));
                std.debug.assert(target_label < insns.len);
                target_insn = insns[target_label];
            }

            const offset: i64 = @as(i64, target_label) - @as(i64, jump_label) - 1;

            // for unconditional jumps to RETURN, we can replace JUMP with RETURN
            if (bc.insnOp(jump_insn) == @intFromEnum(Opcode.JUMP) and bc.insnOp(target_insn) == @intFromEnum(Opcode.RETURN)) {
                insns[jump_label] = target_insn;
            } else if (offset >= std.math.minInt(i16) and offset <= std.math.maxInt(i16)) {
                insns[jump_label] &= 0xffff;
                insns[jump_label] |= @as(u32, @as(u16, @bitCast(@as(i16, @intCast(offset))))) << 16;
            }

            jump.target = target_label;
        }
    }

    /// Returns false on a jump distance overflow (upstream hasLongJumpError).
    pub fn expandJumps(self: *BytecodeBuilder) !bool {
        if (!self.has_long_jumps) return true;

        const gpa = self.gpa;

        // we have some jump instructions that couldn't be patched which means their offset didn't fit into 16 bits
        // our strategy for replacing instructions is as follows: instead of
        //   OP jumpoffset
        // we will synthesize a jump trampoline before our instruction (note that jump offsets are relative to next instruction):
        //   JUMP +1
        //   JUMPX jumpoffset
        //   OP -2
        // the worst-case expansion is 3x, so to be conservative we will repatch all jumps that have an offset >= 32767/3
        const kMaxJumpDistanceConservative: i64 = 32767 / 3;

        std.mem.sort(Jump, self.jumps.items, {}, struct {
            fn lt(_: void, a: Jump, b: Jump) bool {
                return a.source < b.source;
            }
        }.lt);

        const insns = self.insns.items;
        const lines = self.lines.items;
        const remap = try gpa.alloc(u32, insns.len);
        defer gpa.free(remap);

        // Freed on every exit: on success these hold the replaced streams after the swap below.
        var newinsns: std.ArrayList(u32) = .empty;
        defer newinsns.deinit(gpa);
        var newlines: std.ArrayList(i32) = .empty;
        defer newlines.deinit(gpa);
        try newinsns.ensureTotalCapacity(gpa, insns.len);
        try newlines.ensureTotalCapacity(gpa, insns.len);

        var current_jump: usize = 0;
        var pending_trampolines: usize = 0;

        var i: usize = 0;
        while (i < insns.len) {
            const op = bc.insnOp(insns[i]);
            std.debug.assert(op < @intFromEnum(Opcode._COUNT));

            if (current_jump < self.jumps.items.len and self.jumps.items[current_jump].source == i) {
                const j = self.jumps.items[current_jump];
                const offset: i64 = @as(i64, j.target) - @as(i64, j.source) - 1;
                if (@abs(offset) >= kMaxJumpDistanceConservative) {
                    // insert jump trampoline as described above; we keep JUMPX offset uninitialized in this pass
                    try newinsns.append(gpa, @as(u32, @intFromEnum(Opcode.JUMP)) | (1 << 16));
                    try newinsns.append(gpa, @intFromEnum(Opcode.JUMPX));
                    try newlines.append(gpa, lines[i]);
                    try newlines.append(gpa, lines[i]);
                    pending_trampolines += 1;
                }
                current_jump += 1;
            }

            const oplen = bc.getOpLength(@enumFromInt(op));
            // copy instruction and line info to the new stream
            for (0..oplen) |_| {
                remap[i] = @intCast(newinsns.items.len);
                try newinsns.append(gpa, insns[i]);
                try newlines.append(gpa, lines[i]);
                i += 1;
            }
        }

        std.debug.assert(current_jump == self.jumps.items.len);
        std.debug.assert(pending_trampolines > 0);

        // now we need to recompute offsets for jump instructions - we could not do this in the first pass because the offsets are between *target*
        // instructions
        for (self.jumps.items) |j| {
            const offset: i64 = @as(i64, j.target) - @as(i64, j.source) - 1;
            const newoffset: i64 = @as(i64, remap[j.target]) - @as(i64, remap[j.source]) - 1;

            if (@abs(newoffset) + 1 >= kMaxJumpDistance) return false;
            if (@abs(offset) >= kMaxJumpDistanceConservative) {
                // fix up jump trampoline
                const insnt = &newinsns.items[remap[j.source] - 1];
                const insnj = &newinsns.items[remap[j.source]];
                std.debug.assert(bc.insnOp(insnt.*) == @intFromEnum(Opcode.JUMPX));

                // patch JUMPX to JUMPX to target location; note that newoffset is the offset of the jump *relative to OP*, so we need to add 1 to make it
                // relative to JUMPX
                insnt.* &= 0xff;
                insnt.* |= @as(u32, @bitCast(@as(i32, @intCast(newoffset + 1)))) << 8;

                // patch OP to OP -2
                insnj.* &= 0xffff;
                insnj.* |= @as(u32, @as(u16, @bitCast(@as(i16, -2)))) << 16;

                pending_trampolines -= 1;
            } else {
                const insn = &newinsns.items[remap[j.source]];
                std.debug.assert(bc.insnD(insn.*) == offset);
                insn.* &= 0xffff;
                insn.* |= @as(u32, @as(u16, @bitCast(@as(i16, @intCast(newoffset))))) << 16;
            }
        }

        std.debug.assert(pending_trampolines == 0);

        std.mem.swap(std.ArrayList(u32), &self.insns, &newinsns);
        std.mem.swap(std.ArrayList(i32), &self.lines, &newlines);

        for (self.debug_locals.items) |*l| {
            // endpc is exclusive, to get the right remapping, we need to remap the location before the end
            if (l.startpc != l.endpc) l.endpc = remap[l.endpc - 1] + 1 else l.endpc = remap[l.endpc];
            l.startpc = remap[l.startpc];
        }
        for (self.typed_locals.items) |*l| {
            if (l.startpc != l.endpc) l.endpc = remap[l.endpc - 1] + 1 else l.endpc = remap[l.endpc];
            l.startpc = remap[l.startpc];
        }

        // upstream remaps the refactored slot vector only with LuauCompileRefactorFeedback; without it, only the deprecated vector (used when
        // DebugLuauUserDefinedClasses is off) is remapped
        if (self.flags.LuauCompileCallFbRemap and (self.flags.LuauCompileRefactorFeedback or !self.flags.DebugLuauUserDefinedClasses)) {
            for (self.fb_slots.items) |*slot| {
                if (slot.kind == LFT_CALLTARGET) slot.value = remap[slot.value];
            }
        }

        return true;
    }

    /// 0 acts as a special marker for error bytecode (it's equal to LBC_VERSION_TARGET for valid bytecode blobs)
    pub fn getError(gpa: Allocator, message: []const u8) ![]u8 {
        const result = try gpa.alloc(u8, message.len + 1);
        result[0] = 0;
        @memcpy(result[1..], message);
        return result;
    }
};

fn log2(v: u32) u32 {
    std.debug.assert(v != 0);
    var r: u5 = 0;
    while (r < 31 and v >= (@as(u64, 2) << r)) r += 1;
    return r;
}

fn writeByte(gpa: Allocator, ss: *std.ArrayList(u8), value: u8) !void {
    try ss.append(gpa, value);
}

fn writeU32(gpa: Allocator, ss: *std.ArrayList(u8), value: u32) !void {
    var buf: [4]u8 = undefined;
    std.mem.writeInt(u32, &buf, value, .little);
    try ss.appendSlice(gpa, &buf);
}

fn writeU64(gpa: Allocator, ss: *std.ArrayList(u8), value: u64) !void {
    var buf: [8]u8 = undefined;
    std.mem.writeInt(u64, &buf, value, .little);
    try ss.appendSlice(gpa, &buf);
}

fn writeVarInt(gpa: Allocator, ss: *std.ArrayList(u8), value_in: u64) !void {
    var value = value_in;
    while (true) {
        try writeByte(gpa, ss, @as(u8, @truncate(value & 127)) | (@as(u8, @intFromBool(value > 127)) << 7));
        value >>= 7;
        if (value == 0) break;
    }
}
